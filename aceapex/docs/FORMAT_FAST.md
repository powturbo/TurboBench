# ACEPX2 header version 3 — the FAST profile

Since ACEAPEX 2.4.0. Reference code: `src/ax_fast.h` (format constants, archive reader, the one block decoder, C99,
identical copies in `c/` and `python/csrc/`), `src/ax_fast_enc.h` (encoder, C++). Conformance fixtures:
`verify/fixtures/conf/*_fast_*.aet` (`scripts/make_fixtures.py`), judged by `scripts/fast_test.sh` (`make test`).
Prior art and what is not ours: `research/fastblk/PRIOR_ART_axf_vs_misa77.md`.

The profile keeps the two properties of ACEPX2 that the rest of the project is built on — every block is
independent (a match never reaches outside its block, so a region decodes from its blocks alone and blocks decode
in parallel) and one archive serves every reader (the C++ library, the C99 decoder, the Python package) — and
drops the entropy stage. What remains is a fixed-width token stream the decoder runs without per-token branching
on lengths. Density is lower than the default profile's and set by the minimum match length; the decoder is the
fastest in the project (research measurements: `research/fastblk/d1-lzbench-2026-10-26.md`).

All integers are little-endian. Offsets are from the start of the archive unless stated otherwise.

## 1. Archive

    [0]                       header, 68 bytes
    [68]                      u64 boff[num_blocks + 1]   start of each block inside the payload; boff[0] = 0
    [68 + 8 (num_blocks + 1)] payload: the blocks, in order; boff[num_blocks] = its size

### Header — 68 bytes, the same size and the same first 36 bytes as version 2

| offset | size | field | value |
|---|---|---|---|
| 0 | 8 | magic | `ACEPX2\0\0` |
| 8 | 4 | version | 3 |
| 12 | 8 | orig_size | bytes of the original input, > 0 (the empty input is a version-2 archive, one header) |
| 20 | 4 | block_size | 1 ≤ block_size ≤ 2^20; the reference encoder writes 1048576 |
| 24 | 4 | num_blocks | ceil(orig_size / block_size), ≥ 1 |
| 28 | 8 | xxhash | XXH3-64 of the original input |
| 36 | 4 | profile | 1 = FAST |
| 40 | 4 | min_match | 4 ≤ min_match ≤ 32; the reference encoder writes 16, 12, 8 or 5 (the level, §4) |
| 44 | 8 | payload_size | bytes of the payload |
| 52 | 16 | reserved | zero; a reader rejects any other value |

A version-2 reader that ignores the version field (none of ours does since 2.4.0, but 2.3.1 libraries exist in
lzbench, TurboBench and pip) reads bytes 36..43 as the literal-stream size: 2^32 × min_match + 1 bytes, more than
any archive, and rejects the file as truncated. The placement of `profile` and `min_match` is chosen for that.

### Block table

`boff[i]` is the offset of block i inside the payload, `boff[num_blocks] = payload_size`. A reader checks once
that the sequence is non-decreasing, starts at 0, ends at `payload_size`, and that every block is at least
12 + 64 bytes (a block header and the pad). Block i holds original bytes `[i × block_size, min((i + 1) ×
block_size, orig_size))`; the last block is never empty.

## 2. Block

    u32 ntok          tokens
    u32 next          bytes of the extension stream
    u32 tail          literal bytes after the last token
    u32 tok[ntok]     ll (bits 0..5) | mc (bits 6..11) | d (bits 12..31)
    u8  ext[next]     length extensions, in token order: the ll extension of a token, then its mc extension
    u8  lit[]         literals in token order, then the `tail` literals
    u8  pad[64]       64 bytes the decoder may read and never uses; the reference encoder writes zeros

The block size as stored is `boff[i + 1] − boff[i]`; the literal run is what remains after the header, tokens,
extensions and the pad: `lit_bytes = size − 12 − 4 ntok − next − 64`.

### Token

A token is `ll` literals followed by one match:

- `ll < 63`: copy `ll` literal bytes. `ll = 63`: 63 + extension bytes.
- Then `ml` bytes copied from `dist = d + 64` bytes back in the output of this block:
  `mc < 63`: `ml = mc + min_match`; `mc = 63`: `ml = 65 + extension`.
- An extension is a run of bytes equal to 255 followed by one byte below 255; its value is their sum. A value
  above 2^30 is an error.

The reference encoder keeps `ml ≤ 64` for direct codes (`mc ≤ 64 − min_match`) and escapes any longer match
with `mc = 63`; a decoder accepts `mc` up to 62 for any `min_match` (`ml ≤ 94`). The match source must lie
inside the block: `dist ≤` bytes already produced in the block, so the first token of a block carries
literals. Distances below 64 cannot be expressed: the minimum distance is what makes a copy of up to 64
bytes a plain forward copy with no overlap handling, and a block of 2^20 bytes fits the 20-bit field
(`d ≤ 2^20 − 1`, `dist ≤ 2^20 + 63`).

After the last token the remaining `tail` literals complete the block; the decoder checks that exactly `tail`
literal bytes and `tail` output bytes are left.

## 3. Decoder contract

`axf_dec_block` takes the block as stored (`ssz` bytes, pad included) and an output buffer of exactly the
block's original length. Any input either decodes to that length or is rejected; the decoder never reads
outside `[src, src + ssz)` or writes outside `[dst, dst + n)`. There is no second decoder with looser
guarantees: the unconditional 32-byte copies of the fast path run only while a whole token fits in the output
(160 bytes of room: a literal copy of up to 64 and a match copy of up to 96 after it) and while the literal
pointer is at or before the end of the literal run, so a 64-byte literal read ends inside the pad. Every other
token — the last 160 bytes of a block, extended literal runs, extended matches — goes through exact copies.
Corrupted input is judged by `scripts/fast_fuzz.cpp` under ASan/UBSan (`make test`); the research decoder of
the same layout passed 27.3 M mutants (`research/fastblk/fuzz-safe-2026-10-27.log`).

`xxhash` covers the whole original; a full decode verifies it (`aceapex d`, the stream decoder with
`ACEAPEX_STREAM_VERIFY`). A region read is guaranteed by the block structure: every length matched, every
bound held.

## 4. Levels

`aceapex_compress(level = 11..18)` and the CLI (`--level 11..18`, or `--profile fast --level 1..8`) select:

| level | min_match | parse |
|---|---|---|
| 11 | 16 | sparse probe: probes every 3 bytes, every 2nd position in a one-slot hash table, matches extended backward (since 2.5.0; before: as 12) |
| 12 | 12 | greedy with lazy evaluation, hash chain depth 64 |
| 13 | 8 | same |
| 14 | 5 | same |
| 15 | 16 | optimal parse: price = literal bytes + 4 per token + 2 per token, chain depth 512 |
| 16 | 12 | same |
| 17 | 8 | same |
| 18 | 5 | same |

A larger minimum match means fewer tokens per MiB, a faster decode and a lower ratio. The levels are an axis of
the format, not of the encoder: the decoder reads `min_match` from the header and nothing else changes. The
optimal parse is a slower encoder (about 1 MB/s per core) for a denser stream at the same decode speed.

## 5. What the profile does not have

No entropy stage, so no `aceapex_decode_streams` (returns `ACEAPEX_ERR_DATA`); no GPU path in 2.4.0 (the GPU
library reports `E_HEADER` for version 3); no DNA transform — the fast profile sees a genome as bytes. Use the
default profile for density and the GPU, the fast profile when the decoder's speed is what the workload buys.

## 6. Versioning

This document freezes header version 3, profile 1. A reader of 2.4.0 reads versions 2 and 3; it rejects any
other version, any profile other than 1, any `min_match` outside 4..32, non-zero reserved bytes. A new profile
takes a new `profile` value under version 3 with its own reserved-field semantics; an incompatible change to
the header takes version 4.
