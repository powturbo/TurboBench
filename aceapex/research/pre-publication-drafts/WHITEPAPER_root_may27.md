# ACEAPEX: Bandwidth-Proportional Parallel LZ77 Decompression

## Abstract

ACEAPEX achieves 2.2–2.5x faster decompression than zstd on structured
biological data by exploiting inter-block parallelism through absolute
offset encoding and independent block architecture.

## Key Properties

**Absolute offsets** — unlike classic LZ77 with relative sliding window,
ACEAPEX stores absolute positions into 4 pre-decoded streams. This enables:
1. Independent block decode — no inter-block dependencies
2. Encoder-side chain flattening without decoder changes

**Chain Flattening** — encoder rewrites back-reference chains into direct
stream references at encode time. Cost: -1.5% ratio. Benefit: D1_safe
improves on structured data.

ACEAPEX cannot safely exploit intra-block parallelism by reordering LZ77
copy operations, because most matches depend on bytes produced by earlier
matches. However, since ACEAPEX stores absolute offsets, an alternative
no-format-change decoder can treat the block as a system of copy equations,
resolve each output range to terminal literal origins via interval graph
contraction, and emit the final output in parallel without reading from
the partially decoded output buffer. This shifts the problem from ordered
copying to origin resolution. Its practicality depends on terminal fragment
count and coalescing efficiency.

## Benchmark Results

### EPYC 4344P (8 cores, DDR5 16.9 GB/s memcpy)

| Compressor | Threads | Decompress | Ratio | Dataset |
|------------|---------|------------|-------|---------|
| aceapex -2 | I=8 | 10,231 MB/s | 8.56% | nci |
| aceapex -2 | I=8 | 9,456 MB/s | 7.75% | FASTQ NA12878 |
| zstd -3 | I=8 | 3,397 MB/s | 8.45% | nci |
| zstd -3 | I=8 | 3,852 MB/s | 7.57% | FASTQ NA12878 |

Memory read+write roof: memcpy/2 ≈ 8,300 MB/s
aceapex at I=8: 76% of roof.

### EPYC 9575F (64 cores, DDR5 47 GB/s memcpy)

| Compressor | Threads | Decompress | Ratio | Dataset |
|------------|---------|------------|-------|---------|
| aceapex -2 | I=32 | 9,903 MB/s | 7.75% | FASTQ NA12878 |
| aceapex -2 | I=32 | 9,508 MB/s | 8.56% | nci |
| zstd -3 | I=32 | 3,938 MB/s | 7.57% | FASTQ NA12878 |

Memory read+write roof: memcpy/2 ≈ 23,700 MB/s
aceapex at I=32: 42% of roof (64 cores share memory controllers).

## Scaling Analysis

zstd decode does not scale past I=8 on either platform.
aceapex scales to I=32 via independent block architecture.

aceapex consistently 2.2–2.5x faster than zstd across platforms.

## Bandwidth Ceiling

LZ77 decode is read + write on a single buffer.
Memory read+write roof = memcpy / 2.

The drop from 76% (4344P) to 42% (9575F) of roof is expected:
64 cores share the same memory controllers — bandwidth saturates
earlier per core.

## Current Limitations

- Intra-block parallelism limited by match-to-match dependencies
  (nci: D1_safe=3%, D2_dep=97%)
- Ceiling ~9.5 GB/s without format change
- entropy decode (FSE/ZSTD) sequential

## Roadmap: ACEPX4

- Separate literal buffer
- Encoder constraint: no match-to-match dependencies
- Parallel entropy decode via Recoil rANS
- Expected: 20+ GB/s

## Verified On

- lzbench (merged PRs #276, #277)
- BIT-PERFECT verified on all platforms
- Datasets: nci, FASTQ NA12878, webster, xml

## Links

- https://github.com/yasha1971-coder/aceapex
- https://encode.su/threads/4487
