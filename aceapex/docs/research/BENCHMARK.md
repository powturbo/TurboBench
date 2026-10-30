# ACEAPEX Benchmark Results

## Hardware
AMD EPYC 4344P 8-Core (Zen 4) | 125 GB DDR5

## lzbench results (in-memory, -I8 threads)

| Compressor    | Compress  | Decompress | Ratio  | File    |
|---------------|-----------|------------|--------|---------|
| zstd -3       | 1071 MB/s | 3383 MB/s  | 8.45%  | nci     |
| aceapex -2    |  885 MB/s | 10200 MB/s | 8.56%  | nci     |
| zstd -3       |  856 MB/s | 3293 MB/s  | 11.89% | xml     |
| aceapex -2    |  885 MB/s | 4697 MB/s  | 12.93% | xml     |
| zstd -3       |  261 MB/s | 1578 MB/s  | 35.94% | dickens |
| aceapex -2    |   74 MB/s | 4074 MB/s  | 38.44% | dickens |

## enwik9 (1GB, 8 threads)

| Metric      | Value                    |
|-------------|--------------------------|
| Ratio       | 3.037x                   |
| Encode      | 307 MB/s                 |
| Decode alg  | 4.2 GB/s                 |
| Decode wall | 2.0 GB/s                 |
| RAM encode  | 2.8 GB (ACEPX2)          |
| RAM encode  | 23 MB (ACEPX3 streaming) |

## ACEPX3 Streaming (v2-dev)

| Compressor    | Compress | Decompress  | Ratio  | File    |
|---------------|----------|-------------|--------|---------|
| zstd -3       | 1071 MB/s| 3383 MB/s   | 8.45%  | nci     |
| aceapex3 -I8  |  133 MB/s| 18803 MB/s  | 8.23%  | nci     |
| zstd -3       |  856 MB/s| 3293 MB/s   | 11.89% | xml     |
| aceapex3 -I8  |  148 MB/s| 4296 MB/s   | 12.37% | xml     |

All BIT-PERFECT verified.
