# Decisions

One entry per decision: date, what was decided, why, what it costs. Reversals are new
entries, never edits. Measured figures carry their host and libzstd version.

## ADR-001 (2026-09-23) Releases are snapshots, not milestones
A version number marks what a stranger can verify (`./verify.sh`), nothing about what
comes next. Fixes ship as soon as they are judged; a research result gets its own
release and paper. Consequence: v1.0.2 is a maintenance release for users of 1.0.1.

## ADR-002 (2026-09-23) The decoder fails closed
Every zstd frame is checked for error and exact size (`zdec_ok`); parallel paths raise
`g_dec_err`, entry points reset and check it, range paths return nullptr and the
library returns `ACEAPEX_ERR_DATA`. Before: a failed frame left malloc garbage and the
call reported success (region on a pre-field archive without `FSE_CHUNK`, 21.09).
Guarded by the contract claim `region_fails_closed_legacy`. Commit f3814f6.

## ADR-003 (2026-09-23) Ratio expectations are keyed by libzstd version
Archive bytes depend on the code and the libzstd version and on nothing else about the
host: identical to five decimals on 16/12/4/2 cores and three compilers at 1.5.5.
The contract carries the 1.4.8 figures (the papers) and the 1.5.5 figures, judged at
1e-5 on both; an unknown version gets 1 percent and says so in the record. Rejected:
a silent wide tolerance (3.2 to 3.8 percent since 1f92b5a) while the record said 1e-5.

## ADR-004 (2026-09-23) Core-count dependence of archive bytes: withdrawn
Suspected on 22.09 from three different default ratios; refuted: `LIT_LANES` 2 and 16
give byte-identical archives on chr1, enwik8 and silesia, and the differences were
libzstd (ADR-003). The 3.72329 default figure of 11.09 reproduces on no known host and
was replaced by 3.72821 (1.4.8) / 3.73164 (1.5.5).

## ADR-005 (2026-09-22) Paper tags are immutable; judges live in main
`verify.sh` checks each paper tag out into a detached worktree and runs
`verify/judges/<tag>.sh` from main, so a judge can improve without touching what was
submitted. A fail with a stated condition and reason (`verify/known/<tag>.tsv`) is
reported as `known-deviation` in its own column, never as pass. First entry: the
paper5-v1 ratios on libzstd other than 1.4.8.

## ADR-006 (2026-09-21) The CLI is built from the library translation unit
`make` compiles `src/aceapex_api.cpp` with `-DACEAPEX_CLI`: one copy of the codec, and
the CLI reaches the public API (`aceapex r`). `g++ src/aceapex_main.cpp` alone still
builds c/d/t. Judged: archives byte-identical to the previous binary. Commit 7e393f8.

## ADR-007 (2026-09-21) aceapex_depth.cpp stays a fork, unported, for now
The reproduction contract builds its encoder from `aceapex_depth.cpp` (4f0797f), a
2035-line fork that predates the chunk field; its archives are read by the library
through `FSE_CHUNK` in the environment, stated in the contract. Only the fork carries
`--fai`, `--region/--range`, `--profile`, `--view`. Porting the field into the fork or
merging the fork into `src/` is a separate decision; the contract is not the place.

## ADR-008 (2026-09-28) Cross-version fixtures are the format-safety claim
A 4 MiB slice of chr1 (offset 100 MiB) archived on each libzstd version we ship
against lives in `verify/fixtures/` (`chr1_4MiB.zstd-<ver>.aet`, tracked on purpose
through a `.gitignore` exception). The HEAD judge decodes every fixture on every host
and re-encodes the decoded slice against the fixture of the host's own libzstd. No
corpus is needed, so the claim runs in CI. Archive bytes depend on the libzstd version
(0.09 % between 1.4.8 and 1.5.5 at 16 KiB chunks); decodability does not.

## ADR-009 (2026-09-28) A second decoder, in C99, judged like the first
`c/aceapex_decode.c` is a standalone decoder: one translation unit, libzstd the only
dependency, no threads or globals, the same names and error codes as `aceapex.h`
(`aceapex_decompress`, `_region`, `_ranges`, plus `aceapex_decoded_size`). It reads
every layout the C++ decoder reads and fails closed on every frame and bound. The
BlockOffsets table sits at offset 68 and is read with `memcpy` (UBSan, ARM). It is
judged against the same fixtures (`head_cdecoder_*`) and is the piece meant for
embedding (bindings, databases, tools). Speed is not its goal; the C++ library keeps
the parallel paths.

## ADR-010 (2026-09-28) The GPU path reads the archive as written; G is probed
`aceapex_gpu.cu` decodes an `.aet` in one process: nvCOMP batched zstd writes frames
directly into the stream buffers, two kernels unpack DNA literal chunks, the v7-RA match
kernel finishes. The parser group width G is chosen at run time by a short probe
(T4 -> 32, H100 expected 8) instead of a compile-time constant. Measured on T4: the
bus is 2.3x shorter than the entropy decode, so H2D/decode overlap does not pay there
(each extra nvCOMP call costs ~1.1 ms); FSE chunks of 4 KiB decode faster than 64 KiB
on nvCOMP, so the interactive profile is also the GPU profile. No `--profile gpu`.

## ADR-011 (2026-09-28) An empty input is a valid archive of one header
`aceapex_compress` on zero bytes used to return 0 - "no archive" - which no caller
could round-trip (lzbench runs every codec on tiny inputs). An empty archive is now
exactly one 68-byte header: `num_blocks 0`, `orig_size 0`, four stream sizes 0,
`block_size 65536`, `xxhash` of zero bytes. Both decoders accept `num_blocks 0` only
under these conditions; a region of length 0 returns 0, any other length is an error.
Fixture `verify/fixtures/empty.aet`, claims `head_fixture_empty_*`, `head_cdecoder_empty`.

## ADR-012 (2026-09-28) The C decoder gets a persistent handle and per-cursor caches
`aceapex_dec_open/size/region/ranges/close` keep the parsed chunk tables, one
`ZSTD_DCtx` per stream and the last four decoded chunks of each stream between calls;
slices that span chunks are assembled by copying, never by decoding a chunk twice. The
stateless functions stay for one-shot use. Measured on a 64 MB DNA archive, random 16 KiB
regions: 492 -> 135 us (C++ library: 114); full decode 0.79 -> 0.245 s. The DNA unpack
writes bases and case through typed 256-entry tables (`uint32_t`, `uint64_t`), not
per-byte loops; the `__memcpy_chk` per 4 bytes that Ubuntu's fortified glibc turned the
first table version into was 44 % of the profile. The handle and table idea came from
the user's second agent as an uncommitted edit in the working tree; it was set aside,
judged like an external patch (three compilers, 17 fixtures, ASan/UBSan, 900 fuzz runs),
then extended with the DCtx and chunk cache. Rule from this: a second agent works on a
branch and lands through `make test`, never by editing another session's working tree.

## ADR-013 (2026-09-29) Software releases are vX.Y.Z; vN.0 tags are paper artifacts
The repository carries `v2.0`, `v3.0`, `v4.0` (paper 2-4 artifacts, June-July 2026),
`paper5-v1`, `paper6-v1` and the software tags `v1.0.0`, `v1.0.0-beta`. From now on a
software release is always a three-component tag that equals `ACEAPEX_VERSION_STRING`
in `src/aceapex.h` (`v2.1.0`, later `v3.0.0` - distinct from the paper tag `v3.0`), with
a CHANGELOG entry and `make test && ./verify.sh` green on the tagged commit. Paper tags
stay frozen (ADR-005) and are never reused for software. The python package and the C
decoder header carry the same string. lzbench integrations name the software version.

## ADR-014 (2026-09-29) The decode thread budget covers the entropy phase; threads=1 spawns nothing
lzbench 2.4's published page (EPYC 9555P) showed aceapex 1.0.1 decode scaling x17.7 at
32 external threads against x22.3 for zstd -2, and internal scaling (`-I8`) of only
x2.5. On ace-core the 2.1.0 CLI reached a 31 ms floor in the entropy phase from four
threads on. Causes, measured on silesia: (1) the three token streams were decoded by
one thread each in the CLI and strictly serially in the library, while `#pragma omp`
in `fse_chunked_decomp` was dead (no `-fopenmp`; with it, one pool per stream
oversubscribed the cores: 42 ms against 32); (2) the library always used every
hardware thread for literals and eight for the match phase, so an external harness
running N copies got N x (lanes + 8) threads; (3) non-DNA archives use the legacy
4-lane literal layout, which caps literal parallelism at four (81.7 MB of silesia
literals / 4 = the 31 ms floor). Decision: every FSE chunk of every token stream is one
job in one pool; the entropy budget is split between literal lanes and that pool by
decoded bytes; `aceapex_decompress_mt(..., threads)` passes one budget through both
phases and `threads = 1` decodes on the caller's thread with no pthread_create; auto
means the hardware thread count. Not decided here: the default literal chunk for
non-DNA input (legacy 4 lanes vs 4 MiB chunks at +0.10 % on silesia) - the ratio axis
is the user's call.

## ADR-015 (2026-09-29) Block decoder copies in 16-byte steps; branch-free and interleaved variants closed
Single-thread decode of silesia on ace-core: entropy 98 ms, match phase 102 ms - the
match phase alone costs what zstd -2 spends on its whole decode (108 ms). With the
copies compiled out the phase still took 72 % of its time (container: 122 of 170 ms),
so the token loop, not the copying, is the cost. Kept: 16-byte wild copies for literal
runs and matches (a match closer than 16 bytes is first expanded byte-wise to a period
>= 16 - the ZSTD_overlapCopy8 / LZ4 technique, not ours), guarded by 16 bytes of slack
inside the block and the literal slice, the plain path for the tail: 199 -> 170 ms
single-thread on the container, bit-perfect, ASan/UBSan clean on 17 fixtures and
silesia, 40 bit-flip runs without a crash. Closed, measured worse on the same host:
(a) branch-light execution (256-entry token table, selects for the rep window,
branch-free 1..2-byte offset varint, unconditional copies redirected to a scratch line
for zero lengths): 241 ms; (b) two blocks per thread with one token each in turn:
241 ms. Neither branch prediction nor the dependency chain is the bottleneck on this
host; the next step is hardware counters on ace-core, not more variants.

## ADR-016 (2026-09-29) Match tables hold block-relative 32-bit positions; the chain is per block
lzbench 2.4's page: compression at level 2 scaled x8.3 over 32 threads where level 1 and
every other codec scaled ~x20 (ace-core -T8: x3.7 against x5.7 for zstd -2). Cause: each
thread carried a chain table of 2^20 int64 (8 MB, memset per call) plus int64 position
tables, and level 2 walks that chain 32 deep at random - 32 threads x 8 MB is a working
set past L3. Matches never leave the block, so positions are now stored relative to the
block start as uint32 (pos, chain), the chain has one link per block position (<= 4 MB
at the 1 MiB maximum block) and is reset per block, and a link is written only from a
position of the current epoch. Candidate order is unchanged: silesia and dickens
archives are byte-identical before and after (2 and 3 threads), the chr1 fixture
determinism claim holds. Container, 2 threads: encode silesia 6.43 -> 5.70 s. Also
removes a 32-bit truncation (`(int32_t)pos`) left from the 64-bit-position fix of
2026-07-24 that only lost matches past 2 GiB.

## ADR-017 (2026-09-29) The library encoder wrote a constant block size; the judge now round-trips the API
`aceapex_compress` stamped `BLOCK_SIZE` (1 MiB) into the header while `encode_file` had
cut the blocks with the adaptive size (256 KiB for a 300 KB input at one thread), so
every archive from the library with two or more blocks smaller than 1 MiB - inputs
between 256 KiB and 4 MiB x threads - decoded its first block and garbage after it.
The CLI wrote the right value, every fixture is CLI-made, the Python package only
decodes, and lzbench's 1.0.1 copy has fixed 1 MiB blocks, so nothing published saw it;
lzbench's own tiny-input run on the 2.1.0 branch found it (`common=262144/300000`).
Fix: the header carries `g_block_size`. New claim `head_api_roundtrip`
(`scripts/api_roundtrip.cpp`): 13 sizes across the block-size boundaries x random and
DNA-like bytes x both levels x 3 encode x 3 decode thread counts, 468 round-trips,
bit-perfect; on the unfixed code it fails 204 of them. Rule: every public entry point
gets a round-trip claim, not only the CLI.

## ADR-018 (2026-09-29) A zstd-free token profile: 32-lane rANS chunks, entry bit 62
The GPU path decodes the entropy layer with nvCOMP, whose zstd decoder is closed since
2.3; lzbench builds the open 2.2 and so cannot run it, and the Blackwell hardware engine
does not read zstd. StreamLZ (encode.su 4526) shows where this ends: its own GPU entropy
decodes at 32-77 GB/s. E1 (2026-09-29) priced order-0 rANS on the token streams at +3.5 %
of the streams in 4 KiB chunks; measured with a real coder in 64 KiB chunks the cost is
gone on genomes: chr1 slice offsets +0.28 %, commands -0.37 % against zstd -1 in the same
chunks, the whole archive +0.12 % against the default 512 KiB zstd chunks. Literals of DNA
archives are already the 2-bit pack decoded by our own kernel, so with this profile a
genome archive needs no zstd on the GPU at all. On text it costs more (silesia commands
+6.8 %) and literals stay zstd: the profile is for the genome niche, opt-in.
Decision: chunk entry bit 62 marks a rANS chunk (`src/ax_rans.h`, spec 3.1.1): 32 lanes
so one warp decodes one chunk, words stored in lane order so a lane finds its word by a
ballot and popcount, the chunk self-checks (every lane must end at the start state).
`AX_TOK=rans` writes it, with 64 KiB token chunks unless FSE_CHUNK is set; the default
output is unchanged byte for byte. All decoders read it: C++ (full, region, batch), C99,
Python, ARM; the two GPU tools decode such chunks on the host for now - the warp kernel
is the next step. Also fixed on the way, in every C++ reader: the chunk table is validated
against the stored stream length before use (a bad or future entry sent readers past the
buffer - v2.1.0 crashes on a bit-62 archive instead of refusing it), and chunk tables and
the block table are read with byte copies, not through misaligned uint64/BlockOffsets
pointers (UBSan in the region paths). Claims: conformance fixtures dna_rans_2MiB,
dna_rans_4k, text_rans_200K (CLI, C99, ARM64, ARMv7), ax_rans.h identical in src/, c/,
python/csrc/; 3000 unit round-trips and 300 bit-flip runs under ASan/UBSan clean.

## ADR-019 (2026-09-29) The open profile: zstd-free literal chunks, modes 2 and 3
After ADR-018 the tokens of a genome archive decode without zstd, but its literals did not:
on a Tesla T4 (chr1, 16 KiB blocks, 64 KiB literal chunks) the nvCOMP decode of the DNA-pack
sub-frames took 12.4 of 29.4 ms on the device, the largest stage (results/colab-t4-2026-09-29-
gpu-rans.log). E3 (results/e3-dna-pack-2026-09-29.log) measured what zstd buys there: replacing
it by raw or order-0 rANS per sub-frame costs +4.37 % of the chr1 archive (seq +0.93, cse
+2.51, gap +0.32, val +0.63; each small rANS piece carries ~170 bytes of header), but the case
mask as run lengths and the exception gaps as LEB128, then raw/rANS, is -1.81 % - smaller than
zstd. Decision: tagged literal chunk mode 2 = open DNA pack (spec 3.4: header of nexc, ncse,
ngap and four piece sizes; seq and val as pieces, cse as alternating upper/lower run lengths,
gap as LEB128, a piece being raw or one rANS chunk of spec 3.1.1, whichever is smaller per
piece), mode 3 = open plain (one piece of the chunk's bytes). `AX_LIT=open` writes them;
`AX_PROFILE=open` = `AX_LIT=open` + `AX_TOK=rans`, and an archive of that profile contains no
zstd frame; the literal stream is then always chunked and tagged. Default output unchanged byte
for byte (chr1 default 68 127 499 B before and after).
Numbers, chr1 on ace-core (libzstd 1.4.8): default profile zstd 68 127 499 B, rANS tokens
68 099 895, open 66 904 489 (-1.80 % against zstd, -1.76 % against rANS tokens); T4 profile
69 473 882 / 69 169 770 / 67 975 888 (-2.16 %). Cost on the CPU: the entropy phase of the CLI
decode 0.034 -> 0.070 s (the rANS token chunks, same as AX_TOK=rans), the C99 decoder 0.58 ->
0.88 s for chr1 (bytewise base expansion, not tuned); encode 295 -> 284 MB/s.
Readers: C++ (full, region, batch), C99, Python (csrc), ARM through the C99/C++ sources, the GPU
tool: pieces through k_rans (mode 0 = warp copy), bases k_open_seq, case runs k_open_cse and
exceptions k_open_exc, one warp per chunk with ballot/popcount LEB128 parsing and a 64-bit warp
scan (steps in src/ax_open_warp.h, emulated on the CPU by scripts/open_warp_emu.cpp, claim
head_open_warp_emu). Checks: every size, piece mode, LEB128 length and value, run sum, zero
runs and gaps, positions, gap count (spec 3.4). Fixtures dna_open_2MiB, dna_open_mixed_300K,
dna_open_4097B, dna_openlit_300K, text_open_200K (both modes, raw and rANS pieces, exceptions,
zstd tokens with open literals). Fuzz: 3000 corrupted fixture archives through the C99 decoder
and the C++ CLI under ASan/UBSan, 0 crashes, after three fixes to the C++ reader that the fuzz
found in code older than this ADR: the literal chunk table is validated against the stream
(it was read unchecked: heap overflows on a corrupt size), the DNA pack of mode 1 checks its
sub-frame sizes and nexc before reading, the reserved bit 63 of the literal word is rejected;
and the block decoder's varint no longer shifts past 31 bits on a corrupt stream.
Tesla T4, chr1, T4 profile (results/colab-t4-2026-09-29-gpu-open.log, median of 7, all passes
and the five open fixtures bit-perfect, no nvCOMP call on the open archive): on-device 29.35
(zstd) -> 28.24 (rANS tokens) -> 22.56 ms (open), 11.3 GB/s; with H2D 34.88 -> 27.98 ms.
lit 11.68 -> 4.38 ms (pieces: seq 2.15, cse 0.71, gap 0.64, val 0.60, plain 0.67); unpack
4.19 -> 6.56 ms (bases 1.90, case runs 2.80, exceptions 1.84); match 10.5 ms is now the
largest stage. Follow-up (gpu-case): the case runs and the exception gaps are parsed by one
thread block per chunk in rounds of 256 bytes (a chunk with tens of thousands of runs or
exceptions was one warp's serial chain), the case is applied while the bases are written
(k_open_bases: 16 positions per thread, binary search over the run ends, one 16-byte store)
instead of a second read-modify-write pass.
Measured (907a470, results/gpu-open-table.md, chr1, median of 7, all bit-perfect): T4 case
runs 2.80 -> 0.25 ms, bases 1.90 -> 2.78 (the case now lands in the base pass), exceptions
1.84 -> 1.72; unpack 6.56 -> 4.71, on-device 22.56 -> 20.51 ms (12.4 GB/s). Other GPUs, open
vs zstd on-device: A100 9.87 -> 6.17 ms (41.2 GB/s), RTX PRO 6000 Blackwell (sm_120) 5.22 ->
3.72 ms (68.3 GB/s). First T4 run of gpu-case failed (illegal instruction): thread 0's tail
check before the round loop left warp 0 diverged at the block scan's shuffles; the check
moved after the loop, __syncwarp at the scan (76d71eb).
T2T (CHM13 v2.0, 3.16 GB) on A100-80GB, all bit-perfect: on-device zstd 77.7 -> open 52.3 ms
(60.4 GB/s); archive open 887 641 942 B on every machine, zstd/rans bytes depend on libzstd.

## Open
- GPU figures in the README were taken in July on code that predates the literal
  transform, literal chunking and the chunk field. The README front page is rewritten
  only after they are re-measured on the current code (plan B3), with dates.
- Second decoder: ours, in C99 (ADR-009). A third-party implementation from the
  format document is still missing; that is what a standard needs.
- `rans1_v4` (order-1 literals) replaces the DNA transform rather than adding to it;
  the default ratio would be recomputed as a whole. Undecided.

## ADR-020 (2026-09-30) The l1 encoder is the default for DNA input
The chain matcher (hash chain, 4/32 attempts, offset flattening) was the weakest axis: silesia -2
57 MB/s per thread, chr1 65 MB/s. The SALVAGE "v3 batched" idea, carried into our independent
blocks (results/enc-l1-2026-09-29.log), showed more: the short matches of our LZ are worth less
than what the entropy coder of the literal stream (zstd-3, or the open DNA pack) makes of the same
bytes. l1 = an 8K-slot head table (64 KB with epochs, no chain), window = the block (1 MiB max),
the lazy pos+1/pos+2 probes and in-match insertion of the chain encoder, no offset flattening,
only matches >= 32 bytes, literal runs skipped 1 + miss>>4 bytes at a time. Format unchanged:
every existing decoder reads l1 archives (the CPU decoder of main decoded them before this ADR).
Decision: `encode_file` picks l1 when the 4 MiB `dna_worth` sample says DNA, the chain matcher
otherwise; `AX_ENC=l1` / `AX_ENC=chain` force either (text gets l1 only on request: decode on 8
threads is +16-20 % on silesia/enwik8 there). The rule fixed before the measurement: bit-perfect
on the GPU and on-device at most 5 % above open, on chr1 and T2T.
Numbers. ace-core, 1 thread, encode MB/s chain -1 / -2 -> l1: chr1 68 / 65 -> 444; silesia 81 / 57
-> 259; enwik8 65 / 60 -> 299. chr1 default 68 127 499 -> 59 429 097 B (-12.77 %). T4 profile
(16 KiB blocks, 64 KiB literal chunks), libzstd 1.4.8: chr1 zstd 69 473 882 -> 60 449 427, rans
69 169 770 -> 60 431 000, open 67 975 888 -> 63 083 287 (-7.20 %); T2T zstd 902 319 887 ->
822 680 738, rans 898 903 131 -> 822 818 235, open 887 641 942 -> 853 264 869 (-3.87 %).
RTX PRO 6000 Blackwell (Colab 30.09, 9c4847d, open profile, all bit-perfect): chr1 on-device
3.669 -> 2.817 ms (-23.2 %: match 1.263 -> 0.831, tok 0.942 -> 0.355, lit 0.958 -> 1.008,
unpack 0.506 -> 0.623); T2T 26.646 -> 27.131 ms (+1.8 %; match 11.78 -> 9.90, lit 5.33 -> 6.61,
unpack 7.87 -> 9.70), stream pipeline 33.32 -> 31.09 ms. Rule met on both.
Not changed: text and every non-DNA input (chain matcher, bytes as before), the streaming encoder
(src/encode_streaming.cpp, chain), the fixtures (verify/fixtures stay as written; the determinism
check re-encodes with AX_ENC=chain). Pins in scripts/colab_gpu_open.sh follow the new default;
zstd/rans pins exist for libzstd 1.4.8 only until a 1.5.5 run prints them.
