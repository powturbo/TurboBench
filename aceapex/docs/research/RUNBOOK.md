# Pod session runbook — Nsight + chunked ANS + per-stream pipeline

Everything below was written and sanity-checked offline (brace/paren balance,
bash -n syntax check). Nothing here needs thinking on the pod — only:
git pull, drop these 3 files in, compile, run, read numbers, commit results.

## 0. Setup (once per pod)

```bash
cd /workspace/aceapex
git pull origin main   # pulls anything already merged from prior sessions
# copy the 3 prepared files onto the pod (scp, or paste via heredoc one at a time)
```

## 1. Chunked ANS — run FIRST (highest priority: unblocks missing data points)

```bash
cd /workspace/aceapex
nvcc -O3 -arch=sm_90 -I/workspace/dietgpu \
  -o e2e_pipe_chunked e2e_pipe_chunked.cu \
  /workspace/dietgpu/dietgpu/ans/GpuANSDecode.cu \
  /workspace/dietgpu/dietgpu/ans/GpuANSEncode.cu \
  /workspace/dietgpu/dietgpu/utils/DeviceUtils.cpp \
  /workspace/dietgpu/dietgpu/utils/StackDeviceMemory.cpp \
  -lglog
ls -la e2e_pipe_chunked && echo BUILT

DEPTH=/workspace/aceapex/aceapex_depth
PIPE=/workspace/aceapex/e2e_pipe_chunked

# exactly the 4 points that gave ERR in the original sweep
for combo in "fastq:/workspace/data/NA12878_1gb.fastq:32768" \
             "fastq:/workspace/data/NA12878_1gb.fastq:16384" \
             "enwik9:/workspace/data/enwik9:32768" \
             "enwik9:/workspace/data/enwik9:16384"; do
  name=$(echo $combo | cut -d: -f1); src=$(echo $combo | cut -d: -f2); bs=$(echo $combo | cut -d: -f3)
  export ACEAPEX_BS=$bs
  $DEPTH c --in "$src" --out /tmp/c.aet --threads 8 2>/dev/null
  $DEPTH d --in /tmp/c.aet --out /tmp/c_dec 2>/dev/null
  echo "--- $name @ $bs ---"
  $PIPE streams.bin "$src" 2>&1 | grep -E "PREP|FULL-PIPE|MATCHES|FNV|CUDA err"
  unset ACEAPEX_BS
done
rm -f /tmp/c.aet /tmp/c_dec streams.bin
```

**Success = 4/4 `MATCHES OK`, no `CUDA err`.** These numbers slot directly into
the paper 5 saturation table (fastq/enwik9 32K/16K rows, currently marked ERR).

## 2. Per-stream litonly variant — run SECOND (fast, high-signal)

```bash
cd /workspace/aceapex
nvcc -O3 -arch=sm_90 -I/workspace/dietgpu \
  -o e2e_pipe_litonly e2e_pipe_litonly.cu \
  /workspace/dietgpu/dietgpu/ans/GpuANSDecode.cu \
  /workspace/dietgpu/dietgpu/ans/GpuANSEncode.cu \
  /workspace/dietgpu/dietgpu/utils/DeviceUtils.cpp \
  /workspace/dietgpu/dietgpu/utils/StackDeviceMemory.cpp \
  -lglog
ls -la e2e_pipe_litonly && echo BUILT

DEPTH=/workspace/aceapex/aceapex_depth
PIPE=/workspace/aceapex/e2e_pipe_litonly

# compare against the SAME 3 datasets/block_sizes already in the paper 5 table
for combo in "fastq:/workspace/data/NA12878_1gb.fastq:262144" \
             "dna200:/workspace/data/pizzachili/dna.200MB:16384" \
             "proteins200:/workspace/data/pizzachili/proteins.200MB:16384"; do
  name=$(echo $combo | cut -d: -f1); src=$(echo $combo | cut -d: -f2); bs=$(echo $combo | cut -d: -f3)
  export ACEAPEX_BS=$bs
  $DEPTH c --in "$src" --out /tmp/l.aet --threads 8 2>/dev/null
  $DEPTH d --in /tmp/l.aet --out /tmp/l_dec 2>/dev/null
  echo "--- $name @ $bs (litonly) ---"
  $PIPE streams.bin "$src" 2>&1 | grep -E "PREP|Honest|FULL-PIPE|MATCHES|FNV"
  unset ACEAPEX_BS
done
rm -f /tmp/l.aet /tmp/l_dec streams.bin
```

**What to compare:** `FULL-PIPE-LITONLY GB/s` and `Honest total ratio` here
vs. the all-4-streams `e2e_pipe` numbers already in CONTEXT for the same
dataset/block_size. If litonly wins on BOTH axes (expected on dna/proteins per
the 2026-06-22 per-stream ratio finding), that's the seed of paper 5/6's
"per-stream entropy backend selection" result, now measured end-to-end for
the first time (previously only measured as isolated per-stream ratios).

## 3. Nsight sweep — run LAST (mechanical, takes longest: 2 datasets x 6 sizes)

```bash
cd /workspace/aceapex
bash nsight_profile.sh
cat /workspace/nsight_fastq.csv
cat /workspace/nsight_silesia.csv
```

Closes the one remaining `\HOLE{}` in `aceapex_paper4.tex` (Mechanism section).
Paste the two CSVs back and I'll write the paragraph replacing the HOLE.

## Estimated pod time

- Step 1: ~10 min (4 compiles-worth of runs, small)
- Step 2: ~10 min (3 runs)
- Step 3: ~20-30 min (12 profiled runs, ncu overhead is real)
- **Total: under an hour on one H100.** Everything above this line was the
  planning; the pod time is pure execution.

## After the pod session

1. Commit `e2e_pipe_chunked.cu` and `e2e_pipe_litonly.cu` to
   github.com/yasha1971-coder/aceapex (same pattern as `e2e_pipe.cu`).
2. Paste the 4 recovered fastq/enwik9 numbers + litonly comparison + 2 Nsight
   CSVs back here — I'll fold them into the paper 5 draft and the paper 4
   Mechanism section in one pass, same way we closed paper 4's other gaps.
