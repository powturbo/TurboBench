# HW-ENC2 — fast encoder for the FAST profile (level 11 = fast level 1, min match 16)
Since 2.5.0 the library's level 11 IS w32h17 (`enc_sparse`, byte-identical archives); against such a tree `l11` and `w32h17` rows coincide.

Measuring tool only. The format, the decoder and the library are not touched. `enc2.cpp` includes `src/ax_fast_enc.h` and
`src/ax_fast.h` from the repository (v2.4.2) and decodes every archive it makes with `axf_dec_block_of`, comparing it byte for byte.

## Problem
Level 11 (greedy + lazy, 4-byte hash, chain depth 64) compresses silesia at 25 MB/s on ace-core (61.35 %); zxc -1
compresses at 1016 MB/s to 61.67 %. The decoder is ours to win, the encoder is the visible weakness in lzbench.

## The encoder `w` (Fw<HB, A, B> in enc2.cpp)
- 8-byte hash key (min match is 16; a 4-byte key filled level 11's chains with useless candidates).
- Coprime strides: positions == 0 mod B enter the table, probes land every A bytes (A, B coprime). A match covering
  lcm(A, B) bytes of probes is found at some probe; **backward extension** (8 bytes per step) recovers its start.
  Cost per literal byte ~ 1/A probes + 1/B inserts instead of 1 + 1.
- Matches of 8..15 bytes at the probe are extended backward and accepted once they reach 16.
- A candidate closer than 64 bytes (the format's minimum distance) is lifted to the first legal multiple of its distance
  and verified (runs and short periods compress without delayed insertion).
- Inside a match every 8th table position and the one 2 before its end are inserted.
- Misses step A + (bytes since the last match >> 8).
- Output written in place: tokens, extensions, literals (32-byte wild copies) into preallocated arrays.

## Local results (claude container, Xeon 2.1 GHz, 2 vCPU; ratio exact, speed only relative and noisy ±15 %)
`local-silesia.tsv` (silesia.tar 211957760 bytes, rebuilt from the Silesia files; level 11 gives 61.66 % here vs 61.35 % on
ace-core's tar, so compare ratios within one file):

| variant | ratio | encode MB/s | block-only MB/s |
|---|---|---|---|
| l11 (level 11 of the tree enc2 is built against; 2.4.2: greedy-lazy) | 61.66 % | 10 | 10 |
| slot (HW-ENC) | 65.07 % | 273 | 366 |
| w32h17 | **61.20 %** | 280 | 391 |
| w43h17 | 61.73 % | 368 | 554 |
| w43h16 | 62.20 % | 390 | 610 |
| w53h16 | 63.00 % | 375 | 594 |
| zxc -1 (same machine) | 61.35 % | 527 | — |

`local-cost.tsv`: instructions, mispredicted branches and L1 misses per input byte (callgrind, block encoder only,
8 MiB mixed sample). w43h17: 9.9 instr/B; zxc -1 whole compressor: 13.3 instr/B. The wall-clock gap to zxc is memory and
the archive assembly (vector zeroing and copies in `ax_fast_encode` / `ax_fast_assemble`), not the match finder.
Other corpora (bin, dna, mix, text, zeros, rand): w32h17 ratio better than l11 on all, 20-40x faster; bit-perfect.

## Acceptance (decided before the run)
Bit-perfect on every corpus and the selftest PASS. Then, on ace-core silesia, single thread, pinned:
1. ratio <= 61.35 % (level 11 on the same file);
2. decode (`dec_MB_s_median`) >= 0.99 x level 11's on the same run: the decode lead over zxc must stay;
3. encode (`enc_MB_s_median`) >= 400 MB/s whole archive. Stretch: >= zxc -1 on the same run.
The variant that passes 1-2 with the highest encode speed is the candidate for level 11. If none passes 2 (more tokens
per MiB: w32h17 13564 vs level 11 11077), the next step is a token-aware acceptance rule, not a release.

## Decode cost (second run, same machine): `bash run_perf.sh ~/pubrepo 2` -> ~/hwenc2perf-<date>-<commit>.log: token-stream structure (--stats) and perf cycles, instructions, branch misses per output byte of each archive decoded alone (--dec).

## Run (ace-core, idle machine, Yakiv's word)
```
cd HW_ENC2 && sha256sum -c SHA256SUMS
bash run_acecore.sh ~/pubrepo 2              # optional: LZB=~/lzbench/lzbench for zxc / aceapex_fast reference rows
```
The script stops if a corpus is missing or load1 >= 0.5. About 6-8 minutes (level 11 dominates). Output: `~/hwenc2-<date>-<commit>.log`. Send the log back; nothing is pushed.
