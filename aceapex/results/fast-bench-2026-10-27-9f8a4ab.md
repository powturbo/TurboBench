# FAST profile bench - 2026-10-27 20:21 UTC, ace-core, commit 9f8a4ab
# AMD EPYC 4344P 8-Core Processor; governor schedutil; load 0.02 0.09 0.34; aceapex 2.4.0-dev (format ACEPX2, libzstd 1.4.8)
# method: ./aceapex t --level L --threads T: encode on T threads, then 7 decodes of the in-memory archive on T threads, median (min in the raw lines); bit-perfect compared on every run

| corpus | level | min match | ratio (%) | encode MB/s (T=8) | decode 1 thread MB/s | decode 8 threads MB/s |
|---|---|---|---|---|---|---|
| silesia.tar | 11 | 16 | 61.35 | 176.09 | 10877.14 | 16644.56 |
| silesia.tar | 12 | 12 | 54.13 | 167.14 | 9777.05 | 17080.82 |
| silesia.tar | 13 | 8 | 44.95 | 163.73 | 8545.02 | 17434.52 |
| silesia.tar | 14 | 5 | 40.68 | 169.98 | 6732.91 | 17560.49 |
| silesia.tar | 15 | 16 | 55.36 | 8.01 | 10606.78 | 16679.40 |
| silesia.tar | 16 | 12 | 48.85 | 8.01 | 9790.93 | 17029.52 |
| silesia.tar | 17 | 8 | 41.79 | 8.00 | 8504.84 | 17513.26 |
| silesia.tar | 18 | 5 | 39.21 | 7.99 | 7678.68 | 17611.57 |
| chr1.fa | 11 | 16 | 91.44 | 1269.25 | 12756.01 | 18536.63 |
| chr1.fa | 12 | 12 | 90.41 | 762.86 | 12044.62 | 17510.03 |
| chr1.fa | 13 | 8 | 59.62 | 95.07 | 6766.99 | 16055.28 |
| chr1.fa | 14 | 5 | 50.08 | 135.06 | 4577.77 | 16491.87 |
| enwik8 | 11 | 16 | 96.80 | 570.95 | 12434.31 | 16498.09 |
| enwik8 | 12 | 12 | 75.53 | 69.08 | 10326.14 | 16490.05 |
| enwik8 | 13 | 8 | 51.96 | 78.29 | 6753.63 | 17234.42 |
| enwik8 | 14 | 5 | 47.91 | 103.77 | 4926.46 | 17916.37 |
| enwik9 | 11 | 16 | 78.76 | 296.00 | 10617.25 | 15375.65 |
| enwik9 | 12 | 12 | 63.20 | 83.19 | 9847.72 | 15679.68 |
| enwik9 | 13 | 8 | 45.54 | 96.17 | 7170.51 | 16561.53 |
| enwik9 | 14 | 5 | 42.02 | 126.85 | 5631.87 | 16835.75 |
| t2t.fa | 11 | 16 | 94.25 | 1067.40 | 12267.51 | 18544.25 |
| t2t.fa | 12 | 12 | 92.80 | 682.12 | 11818.04 | 17322.86 |
| t2t.fa | 13 | 8 | 60.17 | 91.55 | 6828.35 | 15584.35 |
| t2t.fa | 14 | 5 | 51.45 | 131.93 | 4437.20 | 16107.60 |
