# ACEAPEX Encode: hash+radix match-finding with absolute offsets

Reproducible encode-side benchmark. Replaces the suffix-array match finder with a
GPU-native hash+radix-sort finder, keeping ABSOLUTE source positions. All results
bit-perfect on real data.

## Findings

1. The suffix array is not needed. A radix sort of positions keyed by the first k
   bytes brings matching suffixes adjacent, giving the same coverage as a full SA at
   a fraction of the cost (cub::DeviceRadixSort + per-position scan).
2. Code the distance, not the absolute source. dist = i - src recovers 7-13% ratio.
   Preserves parallel decode: output positions = prefix-sum of token lengths (one
   metadata scan, no byte decode); src = i - dist before copying. Semantics stay
   absolute; only the bitstream is distance.

## Files

- gpu_hash_finder.cu   GPU hash+radix finder. Reports throughput, coverage, bit-perfect.
- aceapex_encode.cpp   CPU reference (C++17). hash vs full-SA LPF; abs vs dist coding.
- encode_challenge.json  The problem spec that was solved.
- RESULTS.md           Measured numbers across 10 formats, honest boundaries.

## Build and run

GPU (CUDA 12.x, SM 8.0+; tested H100):
    nvcc -O3 -arch=sm_90 -o gpu_hash_finder gpu_hash_finder.cu
    ./gpu_hash_finder FILE MAXBYTES K CHAIN_D

CPU reference (g++ OpenMP):
    g++ -O3 -march=native -fopenmp aceapex_encode.cpp -o aceapex_encode
    ./aceapex_encode FILE

## Honest boundaries

- GPU throughput = stage1-2 device-resident (index + find). PCIe NOT counted; parse host-serial.
- Throughput data-dependent, 0.3-3.5 GB/s (see RESULTS.md) — NOT a universal 3 GB/s.
- Ratio = order-0 entropy estimate, not a full codec. Full Compress/Decompress/Size/Ratio
  table is future work.
- Scale measured to 32MB; 1GB not tested here.
- On real genome a full SA still beats hash on ratio; k tuned to alphabet (k=4 text, k=8 fastq).
