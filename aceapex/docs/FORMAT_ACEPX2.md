# ACEPX2 archive format — normative specification, version 1 (2026-09-28)

This document is sufficient to write a decoder. Everything a reader needs is stated
here; nothing is taken from the reader's environment except for archives written before
the chunk field existed (§4.1, marked LEGACY). The reference implementations are
`c/aceapex_decode.c` (C99, 261 lines) and `src/aceapex_main.cpp` (C++); conformance
fixtures live in `verify/fixtures/` and are judged by `make test`. Where this text and
the code disagree, the fixtures decide and the text is fixed.

Conventions: all integers little-endian; `u32`/`u64` fixed width; offsets are byte
offsets from the start of the archive unless stated. "MUST" and "MUST NOT" are hard
requirements on readers; a reader that cannot satisfy one MUST fail the whole call
rather than return partial or guessed bytes (fail-closed).

## 1. Container

    [0]                    AetHeader, 68 bytes
    [68]                   BlockOffsets x num_blocks, 64 bytes each
    [68 + 64*num_blocks]   literal stream   (zlit_sz bytes)
    [+zlit_sz]             offset stream    (zoff_sz bytes)
    [+zoff_sz]             length stream    (zlen_sz bytes)
    [+zlen_sz]             command stream   (zcmd_sz bytes)

The archive MUST be at least 68 + 64*num_blocks + zlit_sz + zoff_sz + zlen_sz + zcmd_sz
bytes; trailing bytes after the command stream are not part of the format.

### 1.1 AetHeader (68 bytes, no padding)

| off | size | field | value |
|---|---|---|---|
| 0 | 8 | magic | ASCII `ACEPX2` then two zero bytes |
| 8 | u32 | version | 2. Readers MUST reject any other value. |
| 12 | u64 | orig_size | length of the original input |
| 20 | u32 | block_size | > 0; every block but the last is exactly this long |
| 24 | u32 | num_blocks | ceil(orig_size / block_size); 0 only for the empty archive |
| 28 | 8 | xxhash | XXH3-64 of the original input, little-endian |
| 36 | u64 | zlit_sz | size of the literal stream as stored |
| 44 | u64 | zoff_sz | offset stream as stored |
| 52 | u64 | zlen_sz | length stream as stored |
| 60 | u64 | zcmd_sz | command stream as stored |

The header is 4-byte aligned in the file and the BlockOffsets table starts at offset 68,
so a reader MUST NOT cast the table to a struct of u64 on platforms that require
8-byte alignment; read fields with byte copies.

Invariants a reader MUST check before touching any stream: `num_blocks * block_size >=
orig_size`; the container size above fits in the input; every BlockOffsets slice lies
inside its decoded stream (§2).

### 1.2 Empty archive

An empty input is exactly one header: `num_blocks = 0`, `orig_size = 0`, all four
`z*_sz = 0`, `block_size != 0` (encoders write 65536), `xxhash` = XXH3-64 of zero bytes
(`2d06800538d394c2`). Readers accept `num_blocks = 0` only under these conditions.
A region read of length 0 returns 0 bytes; any other length is an error.
Fixture: `verify/fixtures/empty.aet`.

### 1.3 BlockOffsets (64 bytes per block)

    u64 lit_off, off_off, len_off, cmd_off;   // start of this block's slice in the DECODED stream
    u64 lit_sz,  off_sz,  len_sz,  cmd_sz;    // length of the slice

Offsets address the streams after entropy decoding (§3–4), not their stored form.
Block `b` reproduces original bytes `[b*block_size, min((b+1)*block_size, orig_size))`
from its four slices and from nothing else: no block reads another block's output.
This is the property that makes any block decodable alone, on a CPU or a GPU.

## 2. Block decoding (the match layer)

Inputs: the four slices `lit[0..lit_sz)`, `off[0..off_sz)`, `len[0..len_sz)`,
`cmd[0..cmd_sz)`, and the block's output length `dst_size`. State: output position
`out = 0`, read cursors `lp = op = np = cp = 0`, repeat-distance table
`rep[4] = {1, 2, 4, 8}`.

Read command bytes `c = cmd[cp++]` while `out < dst_size` and `cp < cmd_sz`:

| c | meaning |
|---|---|
| `0xFF` | reset: `rep = {1,2,4,8}`; produces no bytes |
| `0x00..0x7F` | literal run of `l = c + 1` bytes (1..128): copy `lit[lp..lp+l)` to output; `lp += l` |
| `0x80..0xBF` | repeat match: `ri = (c >> 4) & 3`, `lv = c & 15`; if `lv == 15` then `lv += varint(len)`; `l = lv + 6`; `dist = rep[ri]`; if `ri > 0`: shift `rep[1..ri] = rep[0..ri-1]`, `rep[0] = dist` |
| `0xC0..0xFD` | new match: `lv = c & 0x3F` (0..61); `l = lv + 6`; `dist = varint(off)`; `rep = {dist, rep[0], rep[1], rep[2]}` |
| `0xFE` | new match with long length: `lv = varint(len)`; then as above |

A match copies `l` bytes from `out - dist` to `out`, byte by byte in increasing order
(overlap is intended: `dist = 1` repeats one byte). Match length is always at least 6.

`varint(stream)`: unsigned LEB128 — bytes with the high bit set continue, 7 payload
bits each, least significant group first, at most 5 bytes. Reading past the slice end
is an error.

A reader MUST reject the block if: a literal run would read past `lit_sz` or write past
`dst_size`; `dist == 0`; `dist > out` (would read before the block start); a match would
write past `dst_size`. A block that ends with `out != dst_size` is an error.

The encoder never emits a match whose source crosses the block start; distances are
therefore bounded by the block size, and a decoder needs no history from earlier
blocks. Which matches the encoder chooses (minimum lengths by distance, repeat
preference) is encoder policy and not part of the format.

## 3. The four streams as stored

Every stream is a self-describing sequence of chunks; each chunk is either a standard
zstd frame (RFC 8878, single frame, no dictionary) or raw bytes. This is what lets a
generic zstd decoder — including NVIDIA's nvCOMP batched zstd — decode the entropy layer
of an archive it has never seen. A chunk decodes to a known number of bytes; a reader
MUST reject a chunk that decodes to any other number or fails to decode.

### 3.1 Offset, length and command streams ("FSE layout")

    [0]       u64 word:  bits 0..47 = decoded size S; bits 48..62 = CHUNK / 4096; bit 63 = 0
    [8]       u64 cs[nc]           per-chunk entry: bits 0..47 stored size;
                                   bit 63 = stored raw; bit 62 = rANS chunk (§3.1.1);
                                   bits 48..61 reserved (0); 62 and 63 never both set
    [8+8*nc]  chunks, in order

`nc = ceil(S / CHUNK)`. Chunk `i` decodes to bytes `[i*CHUNK, min((i+1)*CHUNK, S))` of
the stream. Its stored size is `cs[i] & ~(1<<63)` when compressed and the decoded
length when raw; chunk `i` starts at the running sum of the preceding stored sizes.
`S` fits in 48 bits (256 TB per stream). CHUNK is a multiple of 4096.

A stream shorter than 8 bytes is empty (S = 0). This happens for tiny inputs.

A reader MUST check the whole chunk table before decoding: every entry well-formed and
`8 + 8*nc + sum(stored sizes) <= stored stream length`.

#### 3.1.1 rANS chunks (the zstd-free token profile, 2026-09-29)
A chunk whose entry has bit 62 set is a 32-lane interleaved static rANS stream (order 0,
12-bit probabilities, 32-bit states, 16-bit renormalisation), laid out as

    [32]     bitmap of present symbols (bit s of byte s/8)
    [..]     frequency of each present symbol, ascending, unsigned LEB128; sum = 4096
    [128]    32 initial states, u32, each in [2^16, 2^32)
    [4]      W, the number of 16-bit words
    [2W]     renormalisation words

Symbol `i` of the chunk belongs to lane `i mod 32`; groups of 32 symbols decode in order,
lanes ascending. Lane `l` decodes `slot = x & 4095`, the symbol `s` with
`cum[s] <= slot < cum[s] + f[s]`, then `x = f[s] * (x >> 12) + slot - cum[s]`, and if
`x < 2^16` takes the next word: `x = (x << 16) | w`. A reader MUST reject: frequencies
not summing to 4096, a state out of range, a read past W, unread words, or any lane not
ending at state 2^16. Reference code: `src/ax_rans.h` (the same file is `c/ax_rans.h`).
Writers emit rANS chunks only when asked for the profile (`AX_TOK=rans`; environment variables only in builds with
`ACEAPEX_ENV_TUNING`, the CLI - [ENVIRONMENT.md](ENVIRONMENT.md)); decoders
without §3.1.1 see a reserved-bit error.

LEGACY: a zero in bits 48..62 is an archive written before the chunk field existed
(before 2026-09-19). Its CHUNK is 524288 unless the writer was run with `FSE_CHUNK`, in
which case the reader must be told the same value. Conforming writers MUST set the field.

### 3.2 Literal stream

The first u64 word carries the decoded size `S` in bits 0..59 and three flags (bit 63 is reserved and MUST be 0):

| bit | meaning |
|---|---|
| 62 | zstd layout (§3.2.1 or §3.2.2). Clear = the stream uses the FSE layout of §3.1 |
| 61 | chunked layout (§3.2.2). Clear with bit 62 set = original 4-part layout (§3.2.1) |
| 60 | tagged chunks: each chunk starts with a mode byte (§3.2.3). Only with bit 61 |

A reader MUST reject flag combinations it does not know (bit 63 set; bit 60 without 61).

#### 3.2.1 Original layout (4 parts)

    [0]   u64 S | bit62
    [8]   u64 zsz[4]           stored size of each part
    [40]  four zstd frames

Part `t` decodes to `[t*P, min((t+1)*P, S))` with `P = ceil(S/4)`; a part beyond `S`
is empty (stored size 0) and decodes to nothing.

#### 3.2.2 Chunked layout

    [0]    u64 S | bit62 | bit61 [| bit60]
    [8]    u64 CH                 chunk size in bytes (>= 65536)
    [16]   u64 zsz[NW]            stored size of each chunk, NW = ceil(S / CH)
    [16+8*NW]  chunks, in order

Chunk `t` decodes to `[t*CH, min((t+1)*CH, S))`; every chunk is exactly `CH` bytes except
the last. NW is derived, never stored. Encoders fall back to §3.2.1 when NW would
exceed 65535.

#### 3.2.3 Tagged chunks (bit 60)

Each chunk begins with one mode byte; the remaining `zsz[t] - 1` bytes are:

- mode `0`: one zstd frame decoding to the chunk's `raw` bytes.
- mode `1`: the DNA pack of §3.3.
- mode `2`: the open DNA pack of §3.4 (zstd-free, 2026-09-29).
- mode `3`: open plain, one piece (§3.4) holding the chunk's `raw` bytes.
- other values are reserved and MUST NOT be written; the reference decoders read them as mode 0.
  Decoders older than §3.4 read modes 2 and 3 as mode 0, where they fail as zstd frames.

### 3.3 DNA pack (literal chunk, mode 1)

For a chunk of `raw` bytes whose content is mostly `A C G T` in either case:

    [0]   u32 nexc          number of exception bytes (anything that is not ACGT/acgt)
    [4]   u32 h1, h2, h3, h4   stored sizes of the four sub-frames below
    [20]  seq  zstd frame -> ceil(raw/4) bytes:  2 bits per base, MSB first, A=0 C=1 G=2 T=3
          cse  zstd frame -> ceil(raw/8) bytes:  1 bit per position, MSB first; 1 = lowercase
          gap  zstd frame -> nexc x u32 (h3 = 0 when nexc = 0): distance from the previous
               exception position (from 0 for the first), cumulative
          val  zstd frame -> nexc bytes (h4 = 0 when nexc = 0): the exception byte itself

Reconstruction, in this order: `dst[i] = "ACGT"[(seq[i>>2] >> (6 - 2*(i&3))) & 3]` for
`i < raw`; then `dst[i] |= 0x20` where `cse[i>>3] & (0x80 >> (i&7))`; then `pos = 0;
for k in 0..nexc: pos += gap[k]; dst[pos] = val[k]`. An exception position `>= raw` is
ignored by the reference decoders. Exception bytes carry their own case; the mask bit at
an exception position is written by the encoder as the byte's own case and is
overwritten by `val` anyway.

### 3.4 Open DNA pack (literal chunk, mode 2) and pieces

The zstd-free literal profile (ADR-019). A **piece** is one mode byte and a payload whose
decoded length `n` is always known from the context: `0` = the `n` raw bytes (stored size
`1 + n`); `1` = one rANS chunk of §3.1.1 decoding to `n` bytes. Other piece modes are an
error. A piece of `n = 0` has stored size 0. Mode 3 is one piece of the chunk's `raw` bytes.

Mode 2 is the DNA pack of §3.3 with two parts recoded and every part a piece:

    [0]   u32 nexc          number of exception bytes (anything that is not ACGT/acgt)
    [4]   u32 ncse          bytes of the case-run stream
    [8]   u32 ngap          bytes of the gap stream
    [12]  u32 h1, h2, h3, h4   stored sizes of the four pieces below (h3 = h4 = 0 iff nexc = 0)
    [28]  seq  piece -> ceil(raw/4) bytes, as in §3.3
          cse  piece -> ncse bytes: run lengths, unsigned LEB128, alternating upper, lower,
               upper, ... and starting with upper; only the first run may be 0; they sum to raw
          gap  piece -> ngap bytes: nexc unsigned LEB128 gaps; exception k is at the running
               sum of gaps 0..k; only the first gap may be 0; every position < raw
          val  piece -> nexc bytes: the exception byte itself

Reconstruction is that of §3.3: bases, then `|= 0x20` over every lower run, then
`dst[pos_k] = val[k]`. The case rule of the writer is the same as for mode 1 (a byte in
`a..z` is lower). A reader MUST reject: `28 + h1 + h2 + h3 + h4` different from the stored
size, an unknown piece mode, a piece not decoding to its length, a LEB128 value over 32
bits or not terminated inside its stream, bytes left after the last run or gap, runs not
summing to `raw`, a zero run or gap where forbidden, a position `>= raw`, fewer than `nexc`
gaps. Reference code: `src/ax_lit_open.h` (the same file is `c/ax_lit_open.h`).
Writers emit modes 2 and 3 only for the open profile (`AX_LIT=open`, or `AX_PROFILE=open` - only in builds with
`ACEAPEX_ENV_TUNING`, [ENVIRONMENT.md](ENVIRONMENT.md) -
which also sets `AX_TOK=rans`); there every literal chunk is mode 2 or 3 (whichever is
smaller when the chunk qualifies for the DNA pack), and the literal stream is always the
tagged chunked layout. An archive of this profile contains no zstd frame.

## 4. Region reads

To produce original bytes `[offset, offset+length)`: blocks `b0 = offset / block_size`
through `b1 = (offset+length-1) / block_size`; for each stream, only the chunks covering
the union of those blocks' slices need decoding. Bytes outside the requested range are
never read by the block decoder. Block independence (§1.3) is what makes this exact.

## 5. Integrity

`xxhash` is the XXH3-64 of the whole original input and lets a full decode be verified.
It does not cover a region read; the per-chunk zstd frames carry their own checksums
only if the writer enabled them (the reference writer does not), so a region reader's
guarantees are: every frame decoded to exactly the expected length, every bound above
held. Corrupted input MUST produce an error, never bytes: the reference decoders are
fuzzed (1500 corrupted/truncated archives under ASan/UBSan, 0 crashes) and that bar
applies to any implementation claiming conformance.

## 6. Versioning

`version` is 2 and has been since the first release; this document freezes what version
2 means. Version 3 (ACEAPEX 2.4.0) is the FAST profile, a different block layout under the
same magic and the same first 36 header bytes: [FORMAT_FAST.md](FORMAT_FAST.md). A future
incompatible change increments `version`; readers MUST reject versions
they do not implement (the 2.4.0 readers reject anything but 2 and 3). Flag bits in stream words are reserved: unknown set bits are an
error, not a hint. Compatibility rule: a reader of version N reads every archive of
version <= N; a writer writes only the version it declares.

## 7. Conformance

An implementation conforms if it decodes every file in `verify/fixtures/` to the
recorded SHA-256 and rejects the corrupted variants that `scripts/fixture_test.sh`
derives from them. `make test` runs this for the two reference decoders; a third
implementation can run the same script with `BIN=<its cli>`.
