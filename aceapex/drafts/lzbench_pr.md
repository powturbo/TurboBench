<!-- DRAFT: corrected description of PR #336 (inikep/lzbench, branch yasha1971-coder:aceapex-2.2.0). NOT POSTED. -->

# aceapex: update to 2.2.1

**Correction to the first version of this description:** its single-thread compression figures were too
high. aceapex 2.2.0's encoder still ran its entropy stage on helper threads when given one thread, so an
unpinned "1 thread" run used about 1.5 cores (level 3 on silesia: 305 MB/s reported, 195-200 MB/s on one
core) - the same kind of error I pointed out in 1.0.1's decoder, on our own encoder. Fixed in aceapex
2.2.1 (commit 4 of this PR): with one thread the codec starts no thread at all, and aceapex's judge now
checks that. Archive bytes did not change. All numbers below are pinned to cores with `taskset`.

Four commits on top of master (ad5b458, the lz+entropy/ + mk/ layout):

1. **aceapex: update to 2.2.0** - `lz+entropy/aceapex` sources from aceapex v2.2.0, two new headers
   (`ax_rans.h`, `ax_lit_open.h`), #included by the one object. `mk/aceapex.mk` gains
   `-I$(SRC)lz+entropy/zstd/lib`: the codec includes `<zstd.h>`, and without the bundled path a native
   build silently picked up the system header (a different zstd version from the one linked) while a
   cross build failed. The wrapper decodes with `aceapex_decompress_mt` (with `-T` each pool thread runs its
   codec copy with threads=1) and returns the decoded size.
2. **levels 1-3, README, CHANGELOG** - level 3 is the l1 encoder on any input (levels 1 and 2 use it for
   DNA and keep the previous matcher for everything else); algorithm string unchanged ("LZ77 + FSE/Huffman":
   all three levels write zstd-coded streams under lzbench). The README note about the local double-free fix
   goes away: aceapex allocates every stream with at least one byte, so lzbench carries no local change.
3. **remove unused acepx3.cpp and the aceapex3 / aceapex_stream wrappers** - never compiled or listed in
   the codec table.
4. **aceapex: update to 2.2.1** - the encoder keeps to the thread budget it is given (threads=1: no helper
   threads); byte-identical output.

`aceapex_cuda` (0.9) is not touched; its `aceapex_streams_t` has the same layout as the one the library
now declares. A separate PR will follow for it.

## A bug this fixes (not in the copy lzbench ships)
aceapex **2.1.0** kept per-call state (block size, decode error flag, DNA hint) in process globals. Two
calls at once - what `lzbench -T` does - could return wrong output: with two threads over lzbench's own
source tree (4569 files) 2.1.0 returned wrong bytes for 22 files. It was found when the first 2.2.0 sync
failed `-eFASTEST -t0,0 -T2 -jr .` (2 decode errors). 2.2.0 makes the state per call and adds a regression
test (4 threads at once, full and region decode). The copy lzbench ships ("1.0.1") is **not** affected.

## Numbers
silesia.tar (211 938 580 B), AMD EPYC 4344P (8 cores / 16 threads), gcc 11.4, bundled zstd 1.5.7, default
lzbench iterations. "1.0.1" = upstream master ad5b458, "2.2.1" = this branch (bd98777); same machine,
same file, back to back.

One core (`taskset -c 0`, no -T/-I):

| codec | compress MB/s | decompress MB/s | ratio % |
|---|---|---|---|
| aceapex 1.0.1 -1 (before) | 89.1 | 1044 | 32.34 |
| aceapex 1.0.1 -2 (before) | 62.3 | 1052 | 32.18 |
| aceapex 2.2.1 -1 | 89.0 | 1114 | 32.56 |
| aceapex 2.2.1 -2 | 65.8 | 1122 | 32.38 |
| aceapex 2.2.1 -3 | 199 | 1030 | 31.44 |
| zstd 1.5.7 -1 | 690 | 2156 | 34.53 |
| zstd 1.5.7 -3 | 414 | 1905 | 31.20 |

Thread pool, `-T8` on eight logical CPUs (`taskset -c 0-7`); lzbench runs eight codec copies with
threads=1 each:

| codec | compress MB/s | decompress MB/s | ratio % |
|---|---|---|---|
| aceapex 1.0.1 -1 (before) | 515 | 7179 | 32.54 |
| aceapex 1.0.1 -2 (before) | 232 | 7102 | 32.39 |
| aceapex 2.2.1 -1 | 459 | 6556 | 32.73 |
| aceapex 2.2.1 -2 | 249 | 7123 | 32.58 |
| aceapex 2.2.1 -3 | 1054 | 6591 | 31.52 |
| zstd 1.5.7 -1 | 4028 | 12373 | 34.54 |
| zstd 1.5.7 -3 | 2133 | 10598 | 31.21 |

Without pinning, 1.0.1 looks faster at decode (1454-1469 MB/s at "1 thread"): it decodes its literal stream
on four threads of its own (`lit_decompress`, NW=4) whatever thread count it is given (126 % CPU). On one
core 2.2.1 decodes 7 % faster than 1.0.1.

Levels 1-2 are 0.6-0.7 % larger than 1.0.1 on silesia (32.56 against 32.34 % at level 1): aceapex 2.2
flattens match offsets to the first occurrence (shorter dependency chains for parallel/GPU match copy);
with AX_NOFLAT=1 the output is byte-identical to 1.0.1 in every stream (only the chunk-size field that
2.2 records in each token-stream header differs). Level 3 is 2.9 % smaller than 1.0.1 -1.

On one core aceapex decodes at about half of zstd and encodes slower than zstd -3 at every level; its
point is independent blocks (region reads, GPU decode), not beating zstd here.

## Suggestion, not part of this PR
FASTEST lists `aceapex,1`. With 2.2.1 the fastest level is `aceapex,3` (l1): on silesia.tar on one core
195-199 MB/s against 86-89 MB/s for level 1, with a smaller output (31.44 % against 32.56 %). If you agree,
`aceapex,3` could replace `aceapex,1` there; the aliases are left as they are.

## Tested
- This branch, lzbench CI set, x86-64 Linux (gcc 11.4): `-eLZ -v5 ./lzbench`, `-eLZ+ENTROPY -v5 ./lzbench`,
  `-eSYMMETRIC -v5 ./lzbench`, `-eFASTEST -t0,0 -T2 -jr .` - all pass (with 2.2.0 and again with 2.2.1).
- The 2.2.0 state of this branch, cross-built out of tree and run under qemu-user: ARM64 and PPC64LE
  (Bootlin glibc 2024.05 toolchains): `-eaceapex,1,2,3`, `-eLZ+ENTROPY -v5 ./lzbench`,
  `-eFASTEST -t0,0 -T2 -jr <tree>` - all pass. ARM32 (arm-linux-gnueabihf 11.4): `-eaceapex,1,2,3` and
  `-eLZ+ENTROPY` pass; `-eFASTEST -T2 -jr` passed aceapex and was killed later in lbzip2 (not aceapex;
  not investigated here).
- aceapex sources on its own release gate (x86-64, x86-32, ARM32, ARM64, PPC64LE under qemu; MinGW build):
  conformance set, byte-identical encoder output across platforms, API round-trips, concurrent calls;
  2.2.1 adds a claim that threads=1 starts no thread.
- Not tested here: macOS, a Windows run, CUDA.
