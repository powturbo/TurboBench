<!-- DRAFT task for the hw-apex-bench agent. Not sent. -->

# hw-apex-bench: move the ACEAPEX adapter to v2.2.0

## Why
ACEAPEX 2.2.0 changes the bytes of every DNA archive: the l1 encoder is the default for DNA
(ADR-020, docs/DECISIONS.md). chr1 default archive 68 127 499 -> 59 429 097 B. The format is the
same (ACEPX2), so the reader side of the adapter needs no change; every number that depends on the
encoder does.

## Do
1. Pin the adapter to tag `v2.2.0` (annotated tag of yasha1971-coder/aceapex). Build with `make`;
   record `./aceapex` sha256 and `libzstd` version in the provenance of every row.
2. Re-measure from scratch, do not carry 2.1.0 numbers:
   - **matched-g**: ACEAPEX interactive (`ACEAPEX_BS=16384 LIT_CHUNK=65536 FSE_CHUNK=4096`) against
     bgzip and zstd-seekable at equal block granularity; ratio by archive file size (header and
     block table included), region p50/p99 for 200 random 16 KiB ranges, timer around the API only.
   - **c(g)**, strict definition by archive file: size at 16 KiB blocks against one block of the
     whole file, same encoder settings otherwise (the 2.1.0 figure was 1.632 %). Report the l1
     default and `AX_ENC=chain` (the 2.1.0 matcher) separately, so the change is visible.
   - The open profile (`AX_PROFILE=open`, no zstd) as its own row next to interactive.
3. GPU rows: take them from the log of commit 27b61b1 (RTX PRO 6000 Blackwell Server Edition,
   Colab 2026-09-30, `scripts/colab_gpu_open.sh`, median of 3, all bit-perfect), marked as
   DECLARED with that provenance unless you rerun them:
   chr1 zstd 60 442 704 B 3.501 ms on-device / 4.547 with H2D; chr1 open 63 083 287 B 2.826 / 3.919;
   T2T zstd 822 393 156 B 35.542 / 49.759; T2T open 853 264 869 B 27.135 / 31.063 (stream pipeline).
4. Keep the old 2.1.0 rows, labelled 2.1.0; publish the new ones next to them.

## Report
One table per axis (ratio, region p50/p99, amplification, break-even, c(g)), provenance per row,
and anything that does not reproduce - as the September runs did (five defects found outside).
