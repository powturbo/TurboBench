# ACEPX2 — stream layout

What is written where, taken from the source rather than from memory. Two defects in
one evening came from the encoder and the decoder disagreeing about this layout, so it
is written down: a chunk count stored where a chunk size was expected, and a per-chunk
size formula inherited from a different scheme. Both were invisible until an archive
left the process that produced it.

All integers are little-endian. Offsets are byte offsets from the start of the file
unless stated otherwise.

## Archive

    [0]                    AetHeader, 68 bytes, packed
    [68]                   BlockOffsets x num_blocks, 64 bytes each
    [68 + 64*num_blocks]   literal stream,  zlit_sz bytes
    [...]                  offset stream,   zoff_sz bytes
    [...]                  length stream,   zlen_sz bytes
    [...]                  command stream,  zcmd_sz bytes

**Empty archive (28.09.2026).** An empty input is a valid archive of exactly one
header: `num_blocks = 0`, `orig_size = 0`, all four stream sizes 0, `block_size` nonzero
(encoders write 65536), `xxhash` = XXH3 of zero bytes. Readers accept `num_blocks = 0`
only under these conditions; a region read of length 0 returns 0, any other length is
an error. Fixture: `verify/fixtures/empty.aet` (68 bytes).

### AetHeader — 68 bytes, `#pragma pack(1)`

| offset | size | field | note |
|---|---|---|---|
| 0 | 8 | magic | `ACEPX2\0\0` |
| 8 | 4 | version | 2 |
| 12 | 8 | orig_size | uncompressed length |
| 20 | 4 | block_size | 16384 by default; `ACEAPEX_BS` overrides (only in builds with `ACEAPEX_ENV_TUNING`, [ENVIRONMENT.md](ENVIRONMENT.md)) |
| 24 | 4 | num_blocks | ceil(orig_size / block_size) |
| 28 | 8 | xxhash | XXH3-64 of the original input |
| 36 | 8 | zlit_sz | compressed size of the literal stream |
| 44 | 8 | zoff_sz | offset stream |
| 52 | 8 | zlen_sz | length stream |
| 60 | 8 | zcmd_sz | command stream |

`sizeof(AetHeader)` is 68, not 72: without the pack pragma the compiler pads it and
every stream pointer shifts by four bytes.

### BlockOffsets — 64 bytes per block

    uint64_t lit_off, off_off, len_off, cmd_off;   // starts, in the UNPACKED streams
    uint64_t lit_sz,  off_sz,  len_sz,  cmd_sz;    // sizes

Starts first, then sizes. Offsets are exclusive prefix sums and address the streams
**after** entropy decoding, not their compressed form. A block is decoded from four
ranges and nothing else, which is what makes a region read possible.

## Stream headers

The offset, length and command streams share one layout; literals have two.

### Offset, length, command

    [0]        uint64  orig_size          uncompressed length of this stream
    [8]        uint64 x nc                compressed size of each chunk
    [8+8*nc]   chunks

nc = ceil(orig_size / CHUNK). CHUNK IS READ FROM THE STREAM: bits 48..62 of the
first word hold CHUNK/4096 (commit 3f2c4d5, 19.09.2026). The same archive therefore
reads the same way on any machine. Zero in that field is an archive written before
the field existed: CHUNK is then 512 KB, or the FSE_CHUNK value in the reader's
environment, exactly as before (since 2.2.2 only in readers built with `ACEAPEX_ENV_TUNING`: the CLI, axdec, the
Python reader; [ENVIRONMENT.md](ENVIRONMENT.md)). Bits 0..47 hold orig_size, capping one stream at
256 TB, and every reader takes the size through fse_stream_size(). Bit 63 set in
a size entry means the chunk is stored raw — the low bits are then the uncompressed
length and the payload is copied rather than decoded. Chunk i covers uncompressed
bytes [i*524288, min((i+1)*524288, orig_size)), and its file position is the running
sum of the preceding compressed sizes.

### Literal stream, original layout

    [0]        uint64  orig_size | bit62
    [8]        uint64 x 4                 compressed size of each part
    [40]       four parts

Bit 62 selects zstd rather than the chunked path above. The parts are equal shares,
ceil(orig_size / 4), the last holding the remainder. This is the default and what the
published measurements use.

### Literal stream, chunked layout

    [0]        uint64  orig_size | bit62 | bit61
    [8]        uint64  chunk_size         in bytes, chosen at encode time
    [16]       uint64 x nc                compressed size of each chunk
    [16+8*nc]  chunks

Bit 61 marks this layout. nc = ceil(orig_size / chunk_size) is derived, not stored, so
the two cannot disagree. Every chunk is exactly chunk_size except the last, which is
the remainder; a formula that special-cases the last *index* rather than the final
*chunk* passes on some inputs and fails on others.

The chunk size lives in the file. An earlier revision read it from the environment,
so an archive decoded correctly only in a shell with the same variable set. Nothing
about decoding may depend on the reader's environment.

Enabled by `LIT_CHUNK=<bytes>` at compression time (only in builds with `ACEAPEX_ENV_TUNING`, [ENVIRONMENT.md](ENVIRONMENT.md)), minimum 65536. It exists for
region reads: with the original layout a 16 KB region needs a quarter of the literal
stream unpacked; with 1 MB chunks it needs one chunk. An older build reading a chunked
archive fails cleanly rather than producing wrong bytes.

## Compatibility

Bits 63, 62 and 61 of the first stream word are flags; the rest is the size. A reader
that does not know a flag must fail rather than guess, since these bits sit inside
what an older reader treats as a length. Old archives are read unchanged and legacy
remains the encoder default.

## What a region read touches

For `length` bytes at `offset`: blocks floor(offset / block_size) through
floor((offset+length-1) / block_size), and in each of the four streams only the chunks
covering those blocks' ranges.

With the interactive profile (LIT_CHUNK 64 KiB, CHUNK 4096) the same request unpacks
about 0.92 MB, a read amplification of 6.216x, and takes 78.3 us p50 on chr1 over 200
random ranges. The paragraph below describes the default profile.

At 16 KB blocks and 512 KB entropy chunks a 16 KB request unpacks about 2 MB, roughly
128 times what was asked for, against the 209 MB a full decode unpacks. This is
measured. Read amplification is bounded by the chunk size, so a figure near 1.0 would
require chunks near the block size, which costs ratio — see CLAIMS.md for the trade at
1 MB literal chunks.


## Chunk size: measured, 19.09.2026

Before 3f2c4d5 the decoder took the chunk size from getenv("FSE_CHUNK"). An archive
written with a non-default size decoded to garbage anywhere the variable was unset:
chr1 at FSE_CHUNK=4096 gave a hash mismatch at byte nine, and region reads returned
noise. Our own runs never showed it because one shell exported the variable to both
sides. Seven readers of that first word had to change, across three files.

Cost of the chunk size, chr1, g = R = 16 KiB, EPYC 4344P, instructions per region
request (differential over 200 and 400 requests, every point bit-perfect):

    CHUNK   512 -> 1.86 M | 1024 -> 1.60 M | 2048 -> 1.51 M
            4096 -> 1.50 M | 8192 -> 1.61 M | 65536 -> 3.24 M

The floor is at 2048-4096, so the interactive default of 4096 is already optimal and
the axis is closed. Smaller chunks add no table overhead; the cost is amplification
alone. Ratio is unchanged by the field: 3.72329 default, 3.70807 interactive.
