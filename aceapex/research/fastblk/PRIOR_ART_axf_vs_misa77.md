# Prior art difference: axf vs misa77 0.6.0

axf is our fast token engine (`research/fastblk/axf.h`). The nearest prior art is misa77 0.6.0 (MIT, (c) 2026 Shreyas
Ghildiyal), the version in lzbench (`lz/misa77`, read from its `README.md`, `src/format.h`,
`src/decompressor_zoo/heavy_unsafe_dimpl.h`). axf was written without misa77 code. This page records which ideas
misa77 already has, so that none of them is claimed as ours.

## Already in misa77 (not ours)

| idea | misa77 0.6.0 | axf |
|---|---|---|
| Fixed-width token, no entropy stage | light: 1-byte token (lit 3 bits, match 5 bits) + 2-byte distance; heavy: u32 token | u32 token |
| **u32 token layout lit 6 bits / match code 6 bits / distance 20 bits, 1 MiB window** | heavy format (level 4): `lit = t >> 26`, `code = (t >> 20) & 63`, `dis = (t & 0xFFFFF) + min_dis` | `ll = t & 63`, `mc = (t >> 6) & 63`, `d = t >> 12`: the same three widths in a different bit order |
| Minimum match distance, so a wide copy never reads bytes it has not yet written | `hashtab_lag` 32 -> `min_dis` 33 (both formats) | `MIN_DIST` 64 |
| Unconditional wide copies (no length branch on the common path) | `vector_width` 32: `copy32` / `cyccpy`, 16 unconditional literal bytes | `CW` 32: one 32-byte copy, a second one only for length > 32; 64 literal bytes |
| Length-63 escape with 255-byte extensions | heavy: `lit_lim` 63, then bytes of 255 | `ll` 63 and `mc` 63, then bytes of 255 |
| Separate safe decoder, a few % slower | safe decoder for levels -1..3 ("~5 %", "about 3 %" on x86) | `dec_block_safe` (#D1-2) |
| Runtime AVX2 / SSE2 dispatch, NEON | yes | no (AVX2 only with `-mavx2`; ARM not run) |
| Slow encoder traded for decode speed; lazy matching; levels by effort | yes (levels -1..4) | yes |

## Different in axf (description only, no novelty claimed until FACTS_LEDGER is checked)

- **Container**: independent 1 MiB blocks with a size table, each block decodable alone (random access, block
  parallel decode). misa77 is a single stream with one literal suffix.
- **Streams per block**: token array, then an extension stream, then literals in token order. misa77 heavy keeps
  the extensions inline in the control stream and reads literals from the end of the buffer backwards.
- **Match length coding**: linear, `mc + MIN_MATCH` up to 64, code 63 = 65 + extension. misa77 heavy uses a
  non-linear 64-entry table up to 192 (`len_of`), so long matches are split at 192 instead of being extended.
- **Minimum match length as the level knob** (5 / 8 / 12 / 16): fewer tokens per MiB (61k -> 11k on silesia), so
  decode speed rises as ratio falls. misa77 has a fixed minimum of 4 and gets its levels from search effort.
- **Level 4: optimal parse** whose price is literal bytes + 4 B per token + lambda per token (lambda = decode
  cost of a token in bytes). misa77 level 3 uses suffix structures (`src/suffix`). Its parse cost is not read here.
- **Safe decoder**: one loop. The unconditional path runs only while 64 literal bytes and
  64 + roundup(62 + MIN_MATCH, 32) output bytes remain. Outside that margin it switches to exact copies: memcpy of
  the true length, overlapping matches copied in steps of the distance. misa77's light safe decoder has a guarded
  fast loop plus a separate tail ("red_slack"). The two approaches are close.

## Measured against each other (ace-core, lzbench 2cc81a8, plain make, median of 7)

`d1-lzbench-2026-10-26.md`: misa77 -1: 42.64 %, 9613 MB/s. axf -3 (min match 8): 44.95 %, 9119 MB/s.
axf -4 (optimal parse): 39.20 %, 8424 MB/s. axf is faster than misa77 -1 only at ratios worse than 54 %
(axf -1 / -2). At misa77's ratio, misa77 is faster. On ARM misa77 has NEON; axf is not run there (no hardware).
