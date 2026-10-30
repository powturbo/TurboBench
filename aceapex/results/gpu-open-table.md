# Open profile across GPUs (scripts/colab_gpu_open.sh, profile BS 16K / LIT 64K)

chr1 (253 935 557 B), ms, median of 7; every row bit-perfect; 4 GPUs (FNV of the GPU output == original),
commit 907a470, Colab 2026-09-29, CUDA 12.8, driver 580.82.07, nvCOMP 5.3.0.16.
Logs: results/colab-2026-09-29-<gpu>-gpu-open.log. open = 0 nvCOMP calls.

Archive bytes (identical on every machine; == pinned on all 3 Colab hosts):
| corpus | zstd | rans | open |
|---|---|---|---|
| chr1 (253 935 557 B) | 69 410 925 | 69 106 957 | 67 975 888 |
| t2t (3 156 259 565 B), libzstd 1.4.8 (ace-core) | 902 319 887 | 898 903 131 | 887 641 942 |
| t2t, libzstd 1.5.5 (Colab A100-80GB, 5d9786a) | 901 676 480 | 898 263 414 | 887 641 942 |

## Stages
| GPU (sm) | archive | tok | lit | unpack | match | on-device | +H2D | GB/s on-device |
|---|---|---|---|---|---|---|---|---|
| T4 (75) | zstd | 2.767 | 11.941 | 4.188 | 10.617 | 29.512 | 35.057 | 8.60 |
| T4 (75) | rans | 1.181 | 12.031 | 4.176 | 10.632 | 28.020 | 33.539 | 9.06 |
| T4 (75) | open | 1.080 | 4.370 | 4.707 | 10.355 | 20.512 | 25.940 | 12.38 |
| A100-SXM4-40GB (80) | zstd | 0.879 | 5.399 | 0.750 | 2.843 | 9.870 | 15.408 | 25.73 |
| A100-SXM4-40GB (80) | rans | 0.980 | 5.499 | 0.752 | 2.841 | 10.071 | 15.589 | 25.22 |
| A100-SXM4-40GB (80) | open | 1.103 | 1.377 | 0.858 | 2.831 | 6.170 | 11.590 | 41.15 |
| RTX PRO 6000 Blackwell SE (120) | zstd | 0.485 | 2.960 | 0.464 | 1.308 | 5.216 | 6.421 | 48.68 |
| RTX PRO 6000 Blackwell SE (120) | rans | 0.955 | 3.028 | 0.466 | 1.298 | 5.747 | 6.946 | 44.19 |
| RTX PRO 6000 Blackwell SE (120) | open | 0.948 | 0.968 | 0.508 | 1.294 | 3.719 | 4.897 | 68.29 |
| L4 (89) | zstd | 0.976 | 4.965 | 2.561 | 4.802 | 13.304 | 18.845 | 19.09 |
| L4 (89) | rans | 0.954 | 4.954 | 2.555 | 4.690 | 13.153 | 18.669 | 19.31 |
| L4 (89) | open | 0.928 | 1.918 | 2.655 | 4.640 | 10.141 | 15.573 | 25.04 |

Repeat on the Blackwell host (3f09fcf, same session, chr1 from the work dir): on-device zstd 5.199,
rans 5.696, open 3.719 ms (68.3 GB/s) - open identical to the first run, zstd/rans within 0.9 %.

open vs zstd on-device: T4 -30.5 %, L4 -23.8 %, A100 -37.5 %, Blackwell -28.7 %.
L4 first run (907a470): chr1 download truncated (results/colab-2026-09-29-l4-gpu-open-fail.log); row from 6c25962.

## Parts of the open archive, ms
| GPU | seq | cse | gap | val | plain | bases | case | exceptions |
|---|---|---|---|---|---|---|---|---|
| T4 | 2.095 | 0.695 | 0.624 | 0.596 | 0.665 | 2.776 | 0.245 | 1.715 |
| L4 | 1.107 | 0.225 | 0.195 | 0.165 | 0.575 | 1.156 | 0.101 | 1.406 |
| A100 | 0.483 | 0.153 | 0.136 | 0.113 | 0.693 | 0.566 | 0.070 | 0.229 |
| Blackwell | 0.310 | 0.088 | 0.068 | 0.049 | 0.602 | 0.260 | 0.033 | 0.217 |

T4 before gpu-case (19ffed7): on-device 22.56, unpack 6.56 (bases 1.90, case 2.80, exceptions 1.84).

## T2T (CHM13 v2.0, NCBI GCA_009914755.4, 3 156 259 565 B), ms, median of 3
A100-SXM4-80GB (sm_80), 5d9786a, libzstd 1.5.5; all rows bit-perfect (FNV b4380f15fd9480a3).
| GPU | archive | bytes | tok | lit | unpack | match | on-device | +H2D | GB/s on-device |
|---|---|---|---|---|---|---|---|---|---|
| A100-80GB | zstd | 901 676 480 | 7.907 | 30.789 | 8.708 | 30.311 | 77.715 | 149.584 | 40.61 |
| A100-80GB | rans | 898 263 414 | 2.351 | 28.922 | 8.692 | 30.313 | 70.278 | 141.893 | 44.91 |
| A100-80GB | open | 887 641 942 | 2.360 | 9.378 | 10.263 | 30.283 | 52.283 | 123.011 | 60.37 |

open parts: seq 5.085, cse 1.549, gap 1.402, val 1.188, plain 0.694; bases 7.364, case 0.715,
exceptions 2.165. open vs zstd on-device -32.7 %; H2D 70.7 ms is 57 % of the open total (+H2D),
pipeline overlap gives 0.7 % on open (nothing left to hide H2D behind).
chr1 on the same A100-80GB: zstd 10.000, rans 10.026, open 6.090 ms (41.69 GB/s).

## Stream pipeline (--pipeline=8, gpu-pipeline bfdfde5), A100-SXM4-80GB, median of 3, all bit-perfect
H2D of batch k+1 (copy stream) under the decode of batch k (decode stream); bytes laid out per batch in pinned memory.
H2D pageable 4.4 GB/s, pinned 12.3-12.4 GB/s. Log: results/colab-2026-09-29-a100-sxm4-80gb-gpu-pipeline.log.
| corpus | archive | H2D | on-device | sequential | pipeline | gain |
|---|---|---|---|---|---|---|
| chr1 | zstd | 5.536 | 9.920 | 15.457 | 26.966 | -74.5 % |
| chr1 | rans | 5.515 | 10.045 | 15.560 | 34.627 | -122.5 % |
| chr1 | open | 5.422 | 6.072 | 11.494 | 17.492 | -52.2 % |
| t2t | zstd | 71.865 | 77.543 | 149.408 | 109.785 | +26.5 % |
| t2t | rans | 71.617 | 70.133 | 141.750 | 110.921 | +21.7 % |
| t2t | open | 70.734 | 55.174 | 125.907 | 78.579 | +37.6 % |
T2T open 125.9 -> 78.6 ms (40.2 GB/s delivered); floor = pinned H2D 70.7 ms, so ~60 ms is not reachable on
this host's PCIe. chr1: 8 batches cost more than the 5.4 ms copy they hide.

## RTX PRO 6000 Blackwell, 51c9655: --pipeline=auto and dense-open on the GPU (median of 3, all bit-perfect)
Log: results/colab-2026-09-29-rtx-pro-6000-blackwell-dense-open.log. H2D pageable 27.3 GB/s, pinned 56.9 GB/s.
| corpus | archive | bytes | lit | unpack | on-device | +H2D | auto path |
|---|---|---|---|---|---|---|---|
| chr1 | open | 67 975 888 | 0.958 | 0.505 | 3.666 | 4.844 | sequential 4.844 |
| chr1 | dense (est.) | 64 923 192 | 3.894 | 0 | 6.098 | - | - |
| t2t | zstd | 901 676 480 | 13.015 | 6.280 | 34.382 | 49.987 | sequential 49.987 |
| t2t | rans | 898 263 414 | 13.161 | 6.236 | 32.849 | 48.402 | sequential 48.402 |
| t2t | open | 887 641 942 | 5.335 | 7.884 | 26.713 | 42.071 | pipeline 8: 33.353 (94.6 GB/s) |
| t2t | dense (est.) | 848 307 559 | 43.922 | 0 | 57.416 | - | - |
dense-open: -4.4 % bytes for a literal stage 2.7x (chr1) / 3.3x (t2t) slower than the open DNA pack on the GPU.
