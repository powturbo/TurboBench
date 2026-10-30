# Notes for a format v3 (not decided; ACEPX2 stays as it is)

Measured candidates, each with its bits, what it breaks, coordinate access and the GPU. Numbers from
`results/reality-2026-10-02.log` and `results/pangenome-2026-10-02.log` (ace-core, 2026-10-02). Nothing here is
implemented in the format; the prototypes are tuning-build tools with their own containers.

## Recommended order

1. **R0 line model** - the largest gain for the least change, measured as a working prototype (AX_LINEMODEL): open
   profile -6.6..-6.8 % (chr1, T2T, GRCh38), default -3.8..-4.7 %; the GPU exception work goes to ~0.
2. **R3 block table** - superblocks: 16 KiB blocks cost +0.69 % instead of +2.02 % (T2T), lookup 0.15-0.6 us.
3. **R1 reference blocks** - the pangenome: HPRC x 3 2141 -> 968 MB; needs the container of members and the
   dependency order on the GPU.
4. **R2 match with substitutions** - +1.2..3 % by a model only; worth it mainly together with R1 (SNPs against a
   reference end every exact match).
R5 (hash per block, +0.18-0.20 % at 16 KiB blocks) can ride with R3: it makes every block self-checking (regions,
repair: 1000 of 1000 single flips located and repaired).
R4 (literal chunk cap) needed no version: the 2.2.2 decoders read such archives; lifted in main 02.10 (2.3).

## R1. Reference blocks (pangenome)

**What.** A block may copy from a *reference segment*: a few blocks of an earlier member of the same archive (another
assembly), forward or reverse-complemented, virtually placed before the block. Matches then reach into the segment
exactly as into the block's own history (the distance counts from the end of the segment).

**Bits.** Per block a reference entry: member (varint, 1 B), orientation (1 bit), first block (varint, 2-3 B), block
count (2 bits for <= 3): about 4 B, against 64 B of the block entry; the prototype stores 11 B raw. Distances grow to
segment + block (<= 4 MiB: still 3-4 varint bytes). Measured (prototype `scripts/refseg.cpp`, AX_REFSEG): HPRC
HG00438.1 + HG00438.2 + HG00621.1, 9.08 GB: 2 141 MB (ACEAPEX 2.3, one archive per assembly) -> 968 MB (ratio 9.38);
the 2nd assembly 724 -> 176 MB, the 3rd 692 -> 95 MB. AGC 3.2.4: 754 MB. Missing to AGC: blocks with no segment found
(15 % of HG00438.2), one segment per block (a block over a breakpoint copies one side), exact matches only (every SNP
ends a match: R2), 1 MiB blocks.

**Compatibility.** ACEPX2 decoders must refuse such archives (a version 3 header, or a feature bit they check): a
match reaching before its block is a corrupt archive for them, and they stop on it (`dist > out`). A v3 decoder reads
every ACEPX2 archive unchanged. Container: a directory of members (name, line model, block range, reference column);
a member refers only to earlier members, depth <= D fixed by the encoder (the prototype: 2).

**Coordinate access.** Block b of member j needs its segment's blocks first, and theirs: measured 3.32 blocks on
average, 12 at most (depth 2, segments of <= 3 blocks; the bound is 1 + 3 + 9). Reverse complement: one pass over the
decoded segment (or a copy kernel that reads backwards and complements).

**GPU.** All members' outputs live in device memory, so a match copies from either the block's own output or the
referenced member's output: a second source pointer in the copy, no extra pass, except the dependency order (decode
the referenced members first: depth + 1 waves, or a batch plan that includes the reference blocks it needs).
Reverse-complemented segments: an RC buffer in temp, or the backwards complementing copy.

## R2. Match with substitutions

**What.** A match token that also carries n substitutions: after the copy, n bytes are replaced (position gap varint,
the new base). Targets the diverged repeat copies (soft-masked repeats are 34 % of T2T's bytes and 35 % of its bits,
at 2.13 bits/byte - the same as unique sequence) and SNPs against a reference (R1).

**Bits.** New command class (a cmd byte value ACEPX2 does not use) + a fifth stream of edits. Offline estimate
(`scripts/approx_est.cpp`, matches >= 32 bases, a substitution 10 bits + 4 per token with any): chr1 1 MiB blocks
k=4 substitutions per 100 b +2.0 %, k=8 +3.0 % of the archive (6 bits per substitution: +2.5 / +4.4 %); T2T +2.0 / +2.9 %
by the model, ~1.2 / 1.7 % after calibrating the model's exact matches against the real encoder (it overrates T2T 1.7x);
16 KiB blocks about half (chr1 +0.7 / 1.3 %).

**Compatibility.** New cmd values and a fifth stream: version 3, ACEPX2 decoders refuse. Blocks stay independent.

**Coordinate access.** Unchanged (block-local).

**GPU.** The block decoder applies a token's edits after its copy: n scattered byte stores per token, the edit stream
is one more stream in the plan (its chunks decoded like the token streams).

## R5. Hash per block

**What.** XXH3 of every decoded block (8 B, or 4 B truncated) next to the block table; today only the whole output has
one. Measured as a research sidecar (`research/selfheal.cpp --sidecar`), not in the format.

**Bits.** Open profile, 16 KiB blocks: chr1 15 499 blocks x 8 B = 123 992 B (+0.20 % of 63 091 473), T2T 192 643 x 8 B =
1 541 144 B (+0.18 % of 853 265 321); 4 B: half (+0.10 / +0.09 %). Default profile, 1 MiB blocks: chr1 243 x 8 = 1 944 B.

**What it buys (I2, `research/results-2026-10-02.md`, the 16 MiB chr1 slice of T-H4, 64 KiB token chunks):**

| case | without block hashes | with block hashes |
|---|---|---|
| 1 flip, 1000 copies: repaired | 990 (10 have no locator: bytes stored raw, only the whole-output hash fails) | 1000 |
| 3 flips, 100 copies: repaired | 84 | 94 |
| probes per copy (1 flip) | 72 142 | 73 018 (the 10 more copies searched; not fewer per copy) |
| what judges a probe | XXH3 of the whole 16 MiB output | XXH3 of the blocks the probe touched |

A region read can check its blocks (today a region has no check at all, ROADMAP item 9), and a decoder can name a corrupt
block instead of a corrupt file. The GPU path of the repair: probes batched 4096 per launch, 17.5 launches per copy;
at the H100 decode rate of open chunks (47 212 in 19.06 ms) about 29 ms per repaired copy against 1.3 s on one CPU core.

**Compatibility.** A table field or a section after the streams with a flag: version 3, or ACEPX2 with a feature bit
older decoders ignore (they read the streams by the sizes in the header). Access by coordinate unchanged.

## Measured alongside (smaller changes, same version bump)

- **R0 line model.** FASTA line ends break matches at a different line phase. Prototype AX_LINEMODEL (tuning builds,
  container AXLINE01; `results/linemodel-2026-10-02.log`): headers as text, sequence lines as runs (length, count) -
  any other length is a run of its own (the exceptions) - zstd-19 in front of the sequence image; decode puts every
  block and its line ends in place while it is in cache; regions map to one sequence range.

  | corpus | default profile | + line model | open profile | + line model | decode 16 threads (open) |
  |---|---|---|---|---|---|
  | chr1 | 59 429 097 | 57 193 613 (-3.76 %) | 63 083 287 | 58 914 632 (-6.61 %) | 0.0145 -> 0.0166 s |
  | T2T | 806 458 861 | 768 385 090 (-4.72 %) | 853 264 869 | 795 603 395 (-6.76 %) | 0.1818 -> 0.1868 s |
  | GRCh38 | 780 638 535 | 750 079 828 (-3.91 %) | 831 221 278 | 775 237 611 (-6.74 %) | 0.1796 -> 0.1897 s |

  With a 12-byte head table as well (AX_HASH12): default -4.37 / -5.50 / -4.63 %. HPRC (refseg line model): 2 141 ->
  2 063 MB. GPU: the open DNA pack's exceptions (every byte not ACGT/acgt) fall from T2T 38.6 M / GRCh38 61.0 M to
  0 / 31 160 (gap + val pieces 16.4 MB -> 0): the exception pieces and their scatter go away; the line ends come back
  in the output store (one division per 16-byte store) or one expansion pass (~2 ms of ~20 ms on an H100, estimate).
  Bits: the line model costs 1.5-1.7 KB per human assembly. Access by coordinate: unchanged in cost (the model maps
  FASTA offsets to sequence offsets per record).
- **R3 block table.** 64 B per block is 1.53 % of T2T at 16 KiB blocks (76 % of what 16 KiB blocks cost against
  256 MiB). Prototype `scripts/superblock.cpp` (`results/superblock-2026-10-02.log`; both forms rebuild the table byte
  for byte): superblocks of 64 blocks (4 x u64 offsets + 4 x u16 sizes per block) 1 640 507 B = 8.52 B/block, lookup
  569 ns (superblocks of 16: 10.1 B/block, 150 ns) -> T2T at 16 KiB blocks 811 411 171 B = +0.69 % against 256 MiB
  instead of +2.02 %; the size columns + zstd 161 378 B = 0.84 B/block -> +0.51 %, decoded once per open.
- **R4 literal chunks.** The encoder capped the chunked literal layout at 65 535 chunks (~4 GiB of literals); above
  it the pre-2.1 layout was used (three human assemblies in one file: ratio 3.36 instead of 4.24). The decoders never
  had the cap: an archive of the lifted encoder (branch `litcap`, 136 008 chunks) decodes byte for byte with the 2.2.2
  C++ CLI and the 2.2.2 C99 `axdec` (full and regions past 4.5 and 8 GB) and with the 2.3 GPU plan emulator
  (`results/litcap-2026-10-02.log`). Not a v3 item: no version needed; merged into main 02.10 (2.3).
