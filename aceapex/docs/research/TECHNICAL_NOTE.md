ACEAPEX v2 — Technical Note
1. Problem
LZ77-class codecs face a fundamental architectural tradeoff:
Option A — Global context:
Match search operates across the full input. Ratio is high.
Decode must be sequential — block N depends on blocks 0..N-1.
Option B — Independent blocks:
Each block is compressed independently. Decode is parallel.
Cross-block matches are lost. Ratio drops significantly.
No standard implementation achieves both simultaneously at the LZ77 layer.
2. Approach
ACEAPEX v2 separates the encode and decode responsibilities:
Encode phase:

Input is processed as a single continuous stream
LZ77 match search has full global context (no block boundaries for matching)
Four output streams are produced: literals, offsets, lengths, commands
Each stream is contiguous across all blocks

Decode phase:

Per-block offsets into each global stream are computed during encode
These offsets are stored in the compressed file header
At decode time, each block receives its exact slice of each stream
Blocks decode independently, in parallel, with no inter-block dependencies

3. Key Mechanism
During encode, after all blocks are processed:
for each block b:
    record: lit_offset[b], lit_size[b]
    record: off_offset[b], off_size[b]
    record: len_offset[b], len_size[b]
    record: cmd_offset[b], cmd_size[b]
The four streams are then compressed globally (single zstd frame, full context).
At decode, block b receives:
lit[lit_offset[b] .. lit_offset[b] + lit_size[b]]
off[off_offset[b] .. off_offset[b] + off_size[b]]
len[len_offset[b] .. len_offset[b] + len_size[b]]
cmd[cmd_offset[b] .. cmd_offset[b] + cmd_size[b]]
Each block decodes independently. No synchronization required.
4. File Format
Header (fixed):
  magic[8]       "ACEPX2\0\0"
  version        2
  orig_size      original file size in bytes
  block_size     LZ77 block size (2MB default)
  num_blocks     number of blocks
  sha256[32]     SHA-256 of original data
  zlit_sz        compressed literal stream size
  zoff_sz        compressed offset stream size
  zlen_sz        compressed length stream size
  zcmd_sz        compressed command stream size

Per-block index (num_blocks entries):
  lit_off, lit_sz    byte range in decompressed literal stream
  off_off, off_sz    byte range in decompressed offset stream
  len_off, len_sz    byte range in decompressed length stream
  cmd_off, cmd_sz    byte range in decompressed command stream

Payload:
  zstd(literals)     single global frame
  zstd(offsets)      single global frame
  zstd(lengths)      single global frame
  zstd(commands)     single global frame
5. Observed Properties
Ratio stability across thread counts:
Ratio is identical for 1/2/4/8 encode threads (3.747x on enwik9).
The LZ77 encode is block-parallel but each block uses the same hash table state.
Decode independence from encode threading:
Decode thread count equals num_blocks (500 for enwik9 at 2MB blocks).
Decode throughput is approximately constant regardless of encode threads used.
Decode throughput on enwik9:
Encode threadsDecode MB/s110032210601410435810460
This confirms decode throughput is an architectural property independent of encode parallelism.
6. Limitations and Scope

Results measured on one machine (AMD EPYC 4344P, Ubuntu 22.04)
Benchmarked on enwik9 (Wikipedia XML text corpus, 1GB)
Binary data (x-ray, executables) compresses to ratio ~1.6x on this dataset
Encode is LZ77-only; no secondary modeling
zstd is used as entropy backend (libzstd 1.4.8)
No streaming API yet; full-file only

7. Comparison Context
Measured on the same machine with the same dataset:
CodecRatioEncode MB/sDecode MB/szstd -93.759x2451429ACEAPEX v23.747x80310460
ACEAPEX v2 achieves approximately the same ratio as zstd -9 on this benchmark,
with 3.3x faster encode and 7.3x faster decode on this hardware.
These results are for enwik9 specifically. Performance on other data types varies.
## XML regression — known issue (April 21, 2026)

Intermediate hash inserts during match (c_len < 32, step = 1 + (c_len>>3))
improve enwik9 +0.046x but regress xml -0.135x.

Root cause: for xml with long repeated patterns, inserting intermediate
positions overwrites useful prev chain entries, breaking subsequent match
finding.

Fix ideas:
- Skip insert when slot already has recent entry (epoch == cur_epoch)
- Use separate "fill" table for intermediate positions
- Only insert when match was found at that position previously

Files affected: xml (6.931x → 6.796x)
Files improved: enwik9, mozilla, nci, samba, webster

## XML regression — known issue (April 21, 2026)

Intermediate hash inserts during match (c_len < 32, step = 1 + (c_len>>3))
improve enwik9 +0.046x but regress xml -0.135x.

Root cause: for xml with long repeated patterns, inserting intermediate
positions overwrites useful prev chain entries, breaking subsequent match
finding.

Fix ideas:
- Skip insert when slot already has recent entry (epoch == cur_epoch)
- Use separate "fill" table for intermediate positions
- Only insert when match was found at that position previously

Files affected: xml (6.931x → 6.796x)
Files improved: enwik9, mozilla, nci, samba, webster
