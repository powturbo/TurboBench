# ACEAPEX: Asymmetric Parallel-Decode LZ77

## Abstract

ACEAPEX is a lossless compression codec that separates the match-search
phase from the decode phase, enabling fully parallel decompression with
zero inter-block dependencies. This document describes the architecture,
design decisions, benchmarks, and limitations.

## 1. Problem Statement

Standard LZ77 decoders are inherently sequential. Each decoded byte may
be referenced by the next match, creating a data dependency chain that
prevents parallelism without format changes.

Modern CPUs have many cores. Fast NVMe storage delivers data faster than
a single-threaded decoder can process it. The bottleneck is the decoder.

## 2. Core Insight

LZ77 match search and LZ77 decode are separable.

At encode time, all match offsets into the global stream are known.
If these offsets are stored per-block in the file header, each block
becomes fully self-contained at decode time.

    Global LZ77 encode → per-block offset index → parallel decode

This is the central idea behind ACEAPEX.

## 3. Architecture

### 3.1 Encode Pipeline

1. Input file is divided into 1MB blocks
2. Global LZ77 match search across all blocks (single pass)
3. Four streams extracted per block:
   - Literals (raw bytes)
   - Offsets (match positions)
   - Lengths (match lengths)
   - Commands (literal/match flags)
4. Streams entropy-coded: ZSTD for literals, FSE for others
5. Per-block byte offsets written to file header

### 3.2 Decode Pipeline

1. Header parsed: block count, per-block stream offsets
2. N threads launched, each assigned blocks from a work queue
3. Each thread:
   - Seeks to block streams using pre-computed offsets
   - Entropy-decodes four streams independently
   - Reconstructs output block
4. Zero synchronization between threads during decode

### 3.3 File Format (ACEPX2)

    [Header: magic + metadata + block index]
    [Compressed literal stream]
    [Compressed offset stream]
    [Compressed length stream]
    [Compressed command stream]

The block index maps each block to byte offsets in all four streams.
This enables O(1) random access to any block.

## 4. Benchmarks

All benchmarks use lzbench 2.2.1, GCC 11.4.0, AMD EPYC 4344P 8-Core,
file: nci (33.5MB, biological sequence data).

### Decompression scaling (aceapex -2, ratio 8.56%)

| Threads (I=) | Decompress |
|--------------|------------|
| 1            | 2574 MB/s  |
| 2            | 4571 MB/s  |
| 4            | 5931 MB/s  |
| 8            | 10192 MB/s |
| 16           | 11674 MB/s |

### Comparison at I=8

| Compressor | Compress  | Decompress | Ratio  |
|------------|-----------|------------|--------|
| aceapex -2 | 862 MB/s  | 10192 MB/s | 8.56%  |
| zstd -3    | 3083 MB/s | 3397 MB/s  | 8.45%  |
| lz4        | 1771 MB/s | 8733 MB/s  | 16.49% |

ACEAPEX decode scales with threads. zstd and lz4 do not — their decode
is single-threaded by design.

## 5. Limitations

- **Encode RAM**: ACEPX2 format requires ~2.8GB RAM for global match
  search. Not suitable for memory-constrained environments.
- **Encode speed**: 7x slower than zstd single-thread. Designed for
  write-once, read-many workloads.
- **Single-thread decode**: Without parallelism, ACEAPEX is slower than
  zstd (2574 vs 3397 MB/s at I=1).
- **Entropy bottleneck**: ZSTD/FSE entropy decoding is sequential.
  The current ceiling is ~11-12 GB/s on 8-core EPYC.
- **Data type sensitivity**: Strong on structured/repetitive data (nci,
  xml, logs). Weak on binary/random data (sao: 1.29x ratio).
- **Research grade**: No ABI stability guarantee. Not production-ready.

## 6. Future Work

### 6.1 Parallel Entropy Decoding
The current bottleneck is sequential FSE/ZSTD entropy decoding.
Parallelizing this phase could push throughput toward 20+ GB/s.

### 6.2 ACEPX3 Streaming Format
ACEPX3 uses chunked encoding (23MB RAM vs 2.8GB) with the same
parallel decode semantics. Suitable for streaming and embedded use.

### 6.3 Random Access API
The per-block index in ACEPX2 already enables O(1) seek to any block.
A formal seekable API would open database and filesystem use cases.

## 7. Origin

This project started as an exploration of Hutter Prize compression.
After testing over 30 different architectures across 7 directions
(range coders, PAQ variants, LZ77 engines, neural predictors, hybrid
architectures, structural transforms, bio-inspired approaches), none
reached the ratio target.

The question shifted: instead of chasing ratio, what if decode speed
was the real constraint in modern multi-core systems?

ACEAPEX is the answer to that question.

Built with Claude AI assistance. Architecture decisions by the author.
Now included in lzbench (PR #276 + PR #277, May 2026).

---

*File: nci — NCBI sequence data, 33.5MB*
*Hardware: AMD EPYC 4344P 8-Core, 125GB DDR5, Ubuntu 22.04*
*Benchmark: lzbench 2.2.1, GCC 11.4.0*
