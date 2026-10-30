#!/bin/bash
# ============================================================================
# ACEAPEX pod bootstrap — fresh pod -> ready to measure.
# Verified working 2026-07-20 on RunPod H100 80GB, CUDA 12.4, Ubuntu 22.04.
# Usage:  bash setup_pod.sh
# ============================================================================
set -e
DATA=/workspace/data
DIETGPU=/workspace/dietgpu
FQ=$DATA/NA12878_REAL.fastq

echo "=== 1/5 system packages (this is the hour we used to lose) ==="
apt-get update -qq
apt-get install -y -qq libgoogle-glog-dev libzstd-dev
#   libzstd-dev MUST be 1.4.x (system). Do NOT -I a 1.5.x header tree from
#   lzbench: it changes the ratio (we hit 4.117 vs 3.97 that way).

echo "=== 2/5 honest data check (refuse wrong file) ==="
if md5sum "$FQ" 2>/dev/null | grep -q 9af9ffaa0e15dba938408a711740e101; then
  echo "  OK honest ERR194147"
else
  echo "  !! honest file missing or wrong md5. Expected 9af9ffaa..."
  echo "  Get: ENA ERR194147_1.fastq.gz, first 1073741620 bytes."
  exit 1
fi

echo "=== 3/5 dietgpu libs (SLOW to build ~20min if absent) ==="
if [ -f "$DIETGPU/build/lib/libgpu_ans.so" ] && [ -f "$DIETGPU/build/lib/libdietgpu_utils.so" ]; then
  echo "  OK dietgpu already built"
else
  echo "  !! dietgpu not built. cd $DIETGPU && mkdir -p build && cd build"
  echo "     && cmake .. && make -j (needs CUDA). ~20 min. Then rerun this."
  exit 1
fi
export LD_LIBRARY_PATH=$DIETGPU/build/lib:$LD_LIBRARY_PATH

echo "=== 4/5 build binaries ==="
SRCDIR="$(cd "$(dirname "$0")" && pwd)"; cd "$SRCDIR"
ZH=/usr/include   # system zstd.h 1.4.8 — the ONE that gives 3.90/3.97
#   THREE depth encoders exist with DIFFERENT thresholds. ONLY aceapex_depth.cpp
#   writes streams.bin AND gives ratio 3.90(base)/3.97(tuned).
#   aceapex_main_depth.cpp gives 4.117 = WRONG FILE for the GPU path.
echo "  building aceapex_depth (the streams.bin encoder)..."
g++ -O3 -march=native -funroll-loops -std=c++17 -Isrc \
    -o aceapex_depth aceapex_depth.cpp -lpthread -lzstd
echo "  building e2e_dense (D1-dense two-kernel)..."
nvcc -O3 -arch=sm_90 -I$DIETGPU -I. -Isrc -o e2e_dense dense.cu \
    -L$DIETGPU/build/lib -lgpu_ans -ldietgpu_utils -lpthread

echo "=== 5/5 SMOKE TEST (build is worthless if it does not reproduce a number) ==="
export ACEAPEX_BS=16384
rm -f streams.bin
./aceapex_depth c --in "$FQ" --out /tmp/c.aet --threads 8 2>/dev/null
./aceapex_depth d --in /tmp/c.aet --out /tmp/c_dec 2>/dev/null
o=$(stat -c%s "$FQ"); c=$(stat -c%s /tmp/c.aet)
ratio=$(python3 -c "print(f'{$o/$c:.3f}')")
echo "  base ratio = $ratio (expect 3.904)"
res=$(./e2e_dense streams.bin "$FQ" 32 2>&1)
echo "$res" | grep -oE '[0-9.]+ GB/s' | tail -1
echo "$res" | grep -oE 'MATCHES OK|DIFFERS'
rm -f /tmp/c.aet /tmp/c_dec
echo ""
echo "=== READY. If you saw MATCHES OK above, the environment is verified. ==="
echo "For TUNED (3.97/~200): edit min_match_len() in aceapex_depth.cpp to"
echo "12/16/24/32, rebuild aceapex_depth, rerun. See BUILD_D1_DENSE.md."
