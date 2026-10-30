# Match-Length Threshold Tuning: A Double Win

A four-number change to the encoder's minimum-match-length policy that improves
**both** compression ratio **and** GPU decode throughput simultaneously, bit-perfect,
across every dataset tested — without touching the decode kernel.

## The change

ACEAPEX resolves each back-reference at encode time and only emits a match if it is
long enough to be worth its offset+length cost. That minimum length depends on the
match distance (a far match needs a longer run to pay for its larger offset):

```
                        before          after
  dist < 128            6               12
  dist < 16384          8               16
  dist < 2097152        10              24
  dist >= 2097152       12              32
```

That is the entire change — four constants in `min_match_len(dist)`. The decode
path is untouched; the output format is unchanged (an old decoder reads the new
stream bit-perfect).

## Why it helps both axes at once

Short matches were being emitted that did not pay off. Each one:

- **cost more than it saved** — a short match with a far, near-random offset spends
  more bits encoding that offset than the literals it replaces, so removing it makes
  the stream *smaller* (better ratio);
- **starved the GPU decode warps** — decode throughput scales with match length
  (a 32-byte match keeps only 1 of 32 lane-threads busy). Replacing many tiny matches
  with fewer long ones raises the average match length, so warps do more useful work
  per step (better throughput).

Raising the thresholds removes exactly those unprofitable short matches. The two
benefits are two consequences of the same cause.

## The mechanism behind the throughput half

Decode throughput on this codec is governed by match length, not by the usual
suspects. We ruled out, by controlled experiment, that the ceiling was compute
(pure-copy of pre-parsed triplets is no faster), occupancy (raising resident-block
capacity via launch bounds made it *worse* through register spill), address scatter
(sequential vs scattered source reads are identical), or multi-stream contention
(two parallel decodes sum to *less* than one). What remains and matches the data is
work-granularity: throughput rises monotonically with average match length until it
saturates. Longer matches move the operating point up that curve.

## Reproduce

```bash
# baseline encoder uses 6/8/10/12; edit min_match_len in aceapex_depth.cpp to 12/16/24/32
g++ -O3 -march=native -std=c++17 -Isrc -o aceapex_depth aceapex_depth.cpp -lpthread -lzstd
# encode + decode any file, verify bit-perfect, measure ratio and decode GB/s
./aceapex_depth c --in FILE --out FILE.aet --threads 8
./aceapex_depth d --in FILE.aet --out FILE.dec
cmp FILE FILE.dec   # bit-perfect
```

See `RESULTS.md` for the full table across 8 datasets.

## Honest scope

- This is an **encoder policy** change; the decode kernel and container format are
  unchanged. Old decoders read the new streams.
- Measured on H100, device-resident decode (H2D/D2H excluded from the timer), 16 KB
  blocks, tile-ANS pipeline. FNV/`cmp` bit-perfect on every point.
- `12/16/24/32` is the ratio-optimal point we found; pushing further (`16/24/32/48`)
  keeps raising throughput but starts to cost ratio on genomic data. The optimum is
  data-dependent at the margin; `12/16/24/32` was a universal win across everything
  tested.
