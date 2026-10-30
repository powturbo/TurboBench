**Correction 2026-07-12:** all FASTQ figures below were measured on a sample with degenerate
quality strings and are withdrawn. Corrected, on ENA ERR194147 (md5 9af9ffaa...): 20.7 M matches,
81.7% coverage, mean length 10.1, 95.7% shorter than 32 B. A note on scope. The §3.3 parse ablation below (pure-copy 212 vs the 221 GB/s
MATCH-PHASE kernel, within 4%) is correct for the match-phase timer scope this paper uses
throughout (see Setup): within the match phase, copy is the wall, not parse. Measured over
the FULL decode instead (including the serial per-token front-end), a parse-only run puts
parse at about 66% of decode time (fused 143 vs pure-copy 212), with the caveat that this
run did not fully exclude dead-code elimination. These are two different timer scopes, not a
contradiction: within the match phase parse is ~4%; across the full decode it is the larger
share. The work-granularity mechanism holds in both, and holds more sharply on real FASTQ,
which sits even lower on the curve. The shift of the wall from copy to parse as matches
lengthen is a separate finding.

# Decode Mechanism: What Governs Throughput

Reproduction material for the mechanism behind the [match-threshold](../match-threshold/)
result: decode throughput in this codec is governed by **work granularity** — the average
match length — and not by occupancy, parse cost, address locality, or launch parallelism.

Read `../match-threshold/` for the practical result. Read this one for why it works.

## Tools

| file | what it shows | needs |
|---|---|---|
| `purecopy.cu` | throughput as a function of match length, isolated from parsing | H100-class GPU |
| `match_histogram.cpp` | where real corpora sit on that curve | CPU |
| `offset_entropy.cpp` | why short matches also cost compression ratio | CPU |

```bash
nvcc -O3 -arch=sm_90 -o purecopy purecopy.cu
g++  -O3 -march=native -std=c++17 -o match_histogram match_histogram.cpp
g++  -O3 -std=c++17 -o offset_entropy offset_entropy.cpp

./purecopy 32                                # one point on the curve
for L in 32 64 128 256 512 1024; do ./purecopy $L; done
./match_histogram enwik9 256000000
./offset_entropy  NA12878.fastq 256000000
```

## The curve

`purecopy.cu` hands the kernel pre-resolved `(src, dst, len)` triplets — no command parsing,
no entropy stage, no leader-lane serialization. One warp copies one match, lanes strided
across its bytes: the inner loop of the shipping `k_decode_g` with everything else removed.
Output size is fixed at 1 GB; only the average match length varies.

Measured on H100 80GB HBM3, from a fresh run of the published file:

| avg match length | 32 | 64 | 128 | 256 | 512 | 1024 |
|---|---:|---:|---:|---:|---:|---:|
| GB/s | 212 | 416 | 607 | 692 | 734 | 744 |

A 3.5x span from match length alone. The reason is structural: a warp is 32 lanes wide, so a
32-byte match gives each lane a single byte, and anything shorter leaves most of the warp
idle. Longer matches fill it.

The first column is also the key ablation: **212 GB/s of pure copy against 221 GB/s of the
real kernel** on the same match-length distribution. Stripping out all parsing buys about 4%.
Within the match phase, decode is not parse-bound — the copy itself, at that granularity,
is the wall. (Across the full decode, including the serial front-end, parse is the larger
share; see the scope note at the top.)

## Where real data sits

Greedy hash factorization over 256 MB prefixes (`match_histogram`):

| corpus | matches | coverage | mean length | shorter than 32 B |
|---|---:|---:|---:|---:|
| enwik9 | 32.5 M | 82.8% | 6.5 | 99.1% |
| FASTQ (NA12878) | 11.9 M | 93.0% | 20.0 | 84.8% |

Real factorizations sit at the low, steep end of the curve. Throughput here is not capped by
the hardware; it is capped by how little work each match hands the warp.

## Why the threshold also improves ratio

`offset_entropy` codes literals, offsets, and lengths as three separate streams and reports
each stream's order-0 entropy while sweeping the minimum match length. On FASTQ at
`min_len=4` the offset stream is about two-thirds of the compressed bytes: a short match to a
far, near-random offset spends more bits encoding that offset than it saves in replaced
literals. Raising the minimum length deletes exactly those matches, so offset entropy falls
faster than literal count rises — and mean match length rises at the same time.

One cause, two effects. That is why the threshold change improves ratio *and* throughput
together instead of trading one against the other.

## Hypotheses we rejected

Each was tested against a control before work-granularity was accepted:

| hypothesis | control | outcome |
|---|---|---|
| compute / parse bound | feed the kernel pre-parsed triplets (`purecopy.cu`) | 212 vs 221 real -> **rejected** |
| occupancy bound | `__launch_bounds__(128,16)`: registers 39->32, blocks/SM 12->16 | *slower* (register spill): 221->202, 181->155 -> **rejected** |
| address scatter | sorted vs scattered source addresses | identical (212.5 both) -> **rejected** |
| launch parallelism | two concurrent decode streams | 90.9 + 61.6 = 152 < 220 single -> **rejected** |
| **work granularity** | throughput vs average match length | **monotone, 3.5x span -> accepted** |

The rejected controls are one-line variants of `purecopy.cu`: permute the source index, add
`__launch_bounds__`, or launch the kernel on two streams.

## Effective workload

Throughput is a function of concurrent lanes, `lanes = G x block_count`, rather than of block
size or cooperation width separately: three different `(G, block_size)` settings at 131 K
lanes all land at 170-183 GB/s, the device starves below ~32 K lanes, and saturates above
~1 M. Reproducing that grid needs the full pipeline binary rather than these standalone
tools — encode with `ACEAPEX_BS=<block_size>`, decode at cooperation width `G`, record GB/s
across the grid.

Occupancy is computed analytically with `cudaOccupancyMaxActiveBlocksPerMultiprocessor`
(39 registers -> 12 resident blocks/SM -> 1584 total), not from profiler counters: Nsight
counters are restricted on the cloud host used here (`ERR_NVGPUCTRPERM`), so the mechanism is
established by controlled ablation instead.

## Honest scope

- `purecopy.cu` is a **synthetic harness**. It isolates match-copy from parsing on purpose.
  The end-to-end figures in the paper come from the real pipeline, not from this tool.
- `offset_entropy.cpp` models the entropy stage (order-0, per stream). The shipping encoder
  is stronger — distance-dependent thresholds, repeat offsets, depth factorization — so its
  absolute ratios are higher. This tool shows the *mechanism*, not the shipped ratio.
- The plateaus (~217 GB/s on enwik9, ~377 on FASTQ at their tuned match lengths) are real
  bandwidth limits at that granularity. Raising the average match length moves the operating
  point up the curve; it does not raise the ceiling.
