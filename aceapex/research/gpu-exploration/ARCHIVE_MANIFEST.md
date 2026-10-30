# Archive Manifest — GPU exploration history

This folder preserves every experimental GPU-decode variant that was tried
and superseded during development. Nothing here was deleted — only moved,
with full git history intact (`git log --follow <path>` shows the complete
history back to the original commit).

**Why this exists:** so that anyone (including a future session) can
understand what was tried, why it was set aside, and restore it to the repo
root in one command if it is ever needed again — for a paper appendix, a
regression check, or reviving an idea with new information.

## How to restore any file to repo root

```bash
git mv research/gpu-exploration/<filename> ./<filename>
git commit -m "restore <filename> from archive"
```

## How to see a file's full history (including before the move)

```bash
git log --follow --oneline research/gpu-exploration/<filename>
```

## What's here and why

| File | Status | Reason archived |
|------|--------|------------------|
| full_gpu_decode.cu | v1, superseded | First GPU decode attempt; superseded by v3 (warp-per-block), which is the active reference kernel and stays at repo root. |
| full_gpu_decode_v2.cu | superseded | Intermediate iteration; occupancy/level-parallel prototype superseded by v3. |
| full_gpu_decode_hybrid.cu | superseded | CPU+GPU heterodecode experiment; measured +3.8–6% only (see HETERODECODE_CLOSED.txt at root, which stays — documents the CPU-side finding). |
| full_gpu_decode_v4.cu | superseded | nvcomp-ANS pipeline prototype; superseded by the hybrid hybrid hybrid Zstd+ANS approach documented in HYBRID_RESULTS_64k.txt (stays at root). |
| full_gpu_decode_v5.cu | falsified, negative result | Token-fusion groups; measured -36% to -49% vs v3 (shared-memory read overhead exceeded parallelism gain). Documented negative result, kept for record. |
| full_gpu_decode_v6.cu | falsified, negative result | Sub-warp groups (G=16/8); monotonically worse than v3 on Volta+ scheduling (divergent sub-groups execute serially). |
| wf_proof.cu | superseded methodology | Wavefront-decoder proof-of-concept; superseded by warp-per-block (v3) as the practical architecture. Historical value: proved dependency-depth analysis. |
| wf_hash.cu | superseded methodology | Companion hash-verification tool for the wavefront line; kept with wf_proof.cu as a set. |
| wf_3gpu.cu | superseded methodology | Multi-GPU NVLink scaling test on the wavefront architecture (249.9 GB/s, 2xH100) — the RESULT is published in Paper 1; this specific harness is superseded by block-parallel architecture. |
| e2e_pipe_chunked.cu | superseded by design correction | First attempt to fix OOM via batch chunking; root cause was later found to be StackDeviceMemory LIFO violation + allocation count, not batch size alone. Left for the engineering record. |
| e2e_pipe_litonly.cu | experimental, unresolved | Per-stream backend test (LIT-only GPU ANS); promising direction (matches Paper 3 Section 6.1 finding) but not yet built into a shipping path. Candidate to revive for a future paper. |
| e2e_pipe_stream.cu | superseded by design correction | First "true streaming" attempt; segfaulted due to violating DietGPU's LIFO stack discipline (reservations held in vectors). Root-caused and fixed in stream2. |
| e2e_pipe_stream2.cu | working, but superseded by Stride-API direction | Fixed the LIFO bug (cudaMalloc ownership instead of RAII reservations held out of scope); confirmed working on large blocks (VRAM free 67GB, whole-file 256K bit-perfect). Still hits allocation-count limits on small blocks; the identified correct path forward is the DietGPU Stride API (ansEncodeBatchStride/ansDecodeBatchStride), not this file. Kept as the last working step before that pivot. |
| README_v2.md | stale draft | Early v2-dev branch note (C API + Python bindings). Superseded by the current README.md; the C API code itself (src/aceapex_api.cpp, src/aceapex.h) is unaffected and remains at its normal path. |

## NOT in this archive (deliberately, and why)

These look similar but are load-bearing and were kept at repo root:

- `full_gpu_decode_v3.cu` — the active reference match-decode kernel (fgd_v3), used throughout every sweep in Papers 1–4 and in current tooling.
- `full_gpu_decode_v7_ra.cu` / `e2e_full.cu` — **e2e_full.cu is explicitly named in the Zenodo v3.0 archive description (DOI 10.5281/zenodo.20812332)** as the end-to-end ANS+match harness. Never rename or move.
- `e2e_seek.cu` — **explicitly named in the same DOI 20812332 description** as the unified-seek harness. Never rename or move.
- `e2e_pipe.cu` — the current working full-pipeline reference, documented in BUILD_e2e_pipe.md.
- `ans_bench2.cu` — the tool behind the 364.9/592.5 GB/s DietGPU numbers cited in Paper 2 Section 6.4.
- `wf_real.cu` — kept visible on purpose: it documents the "fake buffer" measurement-honesty lesson (a methodology finding, not a dead end).
- `DEFENSIVE_PUBLICATION.md` — **explicitly named in the Zenodo v1.0-defensive-pub description (DOI 10.5281/zenodo.20440965)**. Never rename or move.
- Everything under `lzbench_pr/`, `lz/aceapex/cuda/`, `scripts/lzbench_aceapex_codecs.cpp` — merged into upstream lzbench PR #291/#292; out of scope for this cleanup by explicit instruction.
- All CPU source (`src/*`, `aceapex_depth*.cpp`, `acepx3.cpp`, `acepx4.cpp`) — out of scope for this cleanup by explicit instruction; CPU work resumes separately.
| WHITEPAPER_root_may27.md (was WHITEPAPER.md) | superseded by publication | Pre-publication draft, committed May 27. Predates all three arXiv papers (June 2/17/23). Not linked from README or any other file (verified: zero references repo-wide). |
| WHITEPAPER_docs_may19.md (was docs/WHITEPAPER.md) | superseded by publication, stale numbers | Older pre-publication draft, committed May 19. Contains "2.2-2.5x faster than zstd" — a claim from before the GPU work and before the actual published throughput numbers (10,869 MB/s CPU, up to 260 GB/s GPU). Kept both rather than picking one "winner": both are now superseded by the three published, DOI-archived, bit-perfect-verified papers, which do the architecture-explainer job more rigorously. Archiving both removes the contradiction risk rather than relocating it to a single surviving stale file. |

## On the whitepaper decision specifically

Before this cleanup, the repo had two whitepapers with **different titles and contradictory benchmark claims**, neither linked from README.md, discoverable only by browsing the file tree. This is exactly the kind of inconsistency a reviewer or a contact could stumble into and lose trust over. The three published papers (linked prominently in README.md's "Papers" section) are strictly better sources of truth: peer-searchable, DOI-archived, bit-perfect verified, and current. No replacement whitepaper was written — the papers already serve that role.
