# Environment variables

**All of them are read only in builds with `ACEAPEX_ENV_TUNING`** (since 2.2.2, lzbench #336; `src/ax_env.h`). The CLI
(`make`, `ACEAPEX_CLI`), the C99 reader CLI `axdec`, the Python reader and the repository's test tools are built with it.
The library itself is built without it - inside lzbench, TurboBench or any program that links it, the environment changes
nothing: the defaults below apply, the archive bytes and the threads depend only on the call's arguments. The judge claim
`head_env_ignored` checks that (the variables set, the same bytes, no thread at `threads=1`).

## Encoder knobs (2.2.x) — only in builds with ACEAPEX_ENV_TUNING

| variable | default | effect |
|---|---|---|
| `ACEAPEX_BS` | derived from size and threads | block size in bytes (>= 4096); the "interactive" profile uses 16384; the fast profile (2.4.0) takes 4096..1048576, default 1048576 |
| `AX_PROFILE` | unset | `open`: rANS token chunks and open literal chunks, no zstd frame (also sets AX_TOK/AX_LIT) |
| `AX_PROFILE` | unset | `fast` (2.4.0): the FAST profile at the CLI's `--level` 1..8 (default 1), the same as `--profile fast` or `--level 11..18`, which work in every build (docs/FORMAT_FAST.md) |
| `AX_REPS` | 7 | `aceapex t` with the fast profile: timed decode repetitions, the median is reported |
| `AX_TOK` | unset | `rans`: token streams as rANS chunks (ADR-018) |
| `AX_LIT` | unset | `open`: literal chunks as open DNA pack / open plain (ADR-019) |
| `AX_ENC` | `l1` for DNA, chain otherwise | `l1` or `chain`: the match finder (ADR-020) |
| `AX_HLOG` | per input | hash table log (8..24) |
| `AX_MINL` | 32 (l1) | l1: shortest match kept |
| `AX_SKIP` | 4 (l1) | l1: skip step over literal runs |
| `AX_ATT` | per level | chain: match attempts |
| `AX_NOFLAT` | 1 (l1) / 0 | 1: no offset flattening |
| `FSE_CHUNK` | 524288 (65536 rANS tokens, 16384 the open profile since 02.10) | token stream chunk; also the reader's chunk for LEGACY archives without the field |
| `LIT_CHUNK` | unset | literal stream in chunks of this many bytes (>= 65536) |
| `LIT_LANES` | CPU count | literal lanes of the encoder |
| `LIT_LEVEL` | per level | zstd level of the literal frames |
| `LIT_LANES_DEC` | the call's budget | decoder: literal lanes |
| `ACEAPEX_DUMP` | unset | decoder: write the decoded streams to `streams.bin` (diagnostics) |
| `AX_MAXDIST` | unset (128 MiB) | experiment: matches farther back than this many bytes are not taken (results/dep-range-2026-10-01.log) |
| `AX_HASH12` | unset (0) | encoder experiment: head table keyed by 12 bytes, not 4 (with `AX_HLOG=22`; results/reality-2026-10-02.log T1) |
| `AX_BLOCK_TIMES` | unset | CPU tile decode: per group / block times to this file (results/reality-2026-10-02.log T2, scripts/block_times.py) |
| `AX_SCHED_COST` | unset (0) | CPU tile decode: 1 = thread runs of equal estimated cost, 2 = LPT (T2: within noise / 3-8 % slower) |
| `AX_COST_CMD` | 4 | weight of a command byte against a literal byte in `AX_SCHED_COST` |
| `AX_REFSEG` | unset (0) | `scripts/refseg.cpp` only: reference segments between assemblies (prototype, own container) |
| `AX_LINEMODEL` | unset (0) | experiment: FASTA line ends out before LZ, container AXLINE01 read only by tuning builds (results/linemodel-2026-10-02.log) |
| `AX_LINEMODEL_VERIFY` | unset (0) | AXLINE01 decode: check the XXH3 of the whole original (the CLI always checks) |

## Decoder knobs (2.3) — only in builds with ACEAPEX_ENV_TUNING

`AX_LIT_TILE`, `AX_TILE_CHUNKS`, `AX_NT`, `AX_NT_LIT`, `AX_NT_MIN`, `AX_NT_THREADS`, `AX_HUGE`, `AX_PREFAULT`,
`AX_RANS_SIMD`, `AX_PHASE_TIMES`, `AX_STREAM_BLOCKS` (CPU decoder, `src/aceapex_main.cpp`, `src/aceapex_api.cpp`, `src/ax_rans.h`) and
`AX_GPU_TILE` (GPU library, read at `plan_create`; slower, do not use: Blackwell 02.10 t2t 12.508 -> 32.957 ms, chr1
1.055 -> 2.659 ms on-device; off by default, the code stays). Their defaults are the measured best paths; the variables exist to
measure the alternatives side by side (CHANGELOG 2.3.0, `results/`). The GPU measurement tool `aceapex_gpu.cu` reads its
own variants (`AX_OPEN_*`, `AX_VEC` at build time) and is a tool, not the library.
