# Repository guide

This is an active research repository, not a packaged product. The root holds
a working researcher's files — final code, experiment probes, design notes, and
figures — side by side. This guide is the map so you can find what you need
without reading every filename.

## If you are here to...

**...understand what ACEAPEX is** → read `README.md`. It has the papers, the
honest-status section (what is and is not fast), and the core idea.

**...write a decoder, or change one** → read `docs/FORMAT_STREAMS.md`. It is the byte
layout of the archive header, the block table and the four stream headers, taken from
the source. Two defects came from the encoder and the decoder disagreeing about it.

**...cite this work** → use the "Cite this repository" button (GitHub reads
`CITATION.cff`), or cite the papers directly:
- Paper 1 — arXiv:2606.04268 (the codec, CPU scaling, GPU wavefront)
- Paper 2 — arXiv:2606.18900 (device-resident pipeline, 50 GB genome seek)
- Paper 3 — arXiv:2606.24531 (unified two-layer position-invariant seek)
- Paper 4 — arXiv:2607.18541 (what governs decode throughput; the min-match lever)

**...change the codec and measure the result** → run `./sync-check.sh` first. The
source exists in more than one place: a research copy, the production file the library
and the benchmark are built from, and a copy inside the benchmark tree. In one session
three conclusions were drawn from code that was never built or lived in another copy —
a default edited in one file and measured from another, a binary older than its source,
an environment variable outliving the measurement it was set for. The script checks all
three in a few seconds.

**...reproduce the numbers** → the build recipes and run steps are in the
`BUILD_*.md` files and `TECHNICAL_NOTE.txt`. `setup_pod.sh` bootstraps a fresh
GPU pod to a verified environment. `BUILD_LARGE_FILES.md` covers the two settings
that decide whether a large file encodes at all.

**...read the production codec** → `src/` holds the maintained CPU code; the
same code, kept in sync, is the `aceapex 1.0.1` entry in the official lzbench
benchmark.

**...run the GPU decoder** → the `.cu` files (`e2e_pipe_tile.cu`, `dense.cu`,
`e2e_seek.cu`, `full_gpu_decode_*.cu`) are the device-resident paths; build
commands are in each file's header and in the `BUILD_*.md` notes.

## The research archive (kept on purpose)

The `.txt` studies and design `.md` files in the root are the research record.
They are not dead weight — they document what was tried, what worked, and what
did **not**, so the findings can be re-derived and the mistakes are not repeated.
A few worth knowing about:

- `BREAKTHROUGH_PROBES.txt` — probes toward the open throughput ceiling.
- `CAUSAL_STRUCTURE_REWRITING.md` — the encoder-side chain-flattening idea that
  exploits ACEAPEX's absolute offsets (a property unique to this format).
- `FASTQ_PREPROCESSING_STUDY.txt` — why input data must be inspected by eye; the
  record of a degenerate-quality sample that once inflated a ratio and was caught
  and corrected. Kept so the same data trap is never walked into again.
- `PAPER*_MEASUREMENTS.txt`, `RESULTS_*.txt` — the measured numbers behind the
  papers, with datasets and hashes.
- `d2p_dense_*` — the D1-dense two-kernel decoder (a throughput improvement over
  the fused path).

## Why one repository, not many

The code, the notes, and the results are kept together on purpose: it keeps the
work reproducible and reviewable from one place. The layout is a working desk,
not a shipped package — treat the root as a lab notebook with the finished tools
mixed in, and use this guide as the index.
