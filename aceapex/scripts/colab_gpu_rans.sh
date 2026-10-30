#!/usr/bin/env bash
# Colab (or any CUDA host): GPU rANS token decode (ADR-018, k_rans) against the nvCOMP zstd
# path on chr1. One Colab cell:
#   !git clone -q -b gpu-rans https://github.com/yasha1971-coder/aceapex /content/aceapex && bash /content/aceapex/scripts/colab_gpu_rans.sh
# Steps: provenance, libzstd + nvCOMP (pip nvidia-nvcomp-cu12), CLI + aceapex_gpu build,
# chr1 (hg38, md5 pinned), three archives of the same input with the T4 profile of 2026-09-28
# (16 KiB blocks, 64 KiB literal chunks):
#   zstd      FSE_CHUNK=4096                token streams in zstd frames (nvCOMP)
#   rans      AX_TOK=rans                   token streams in 64 KiB rANS chunks (k_rans)
#   rans4k    AX_TOK=rans FSE_CHUNK=4096    rANS in 4 KiB chunks (more warps, larger archive)
# CPU round-trip of each archive, the warp-step emulator, aceapex_gpu on each, then a table.
# Literals stay zstd (DNA pack sub-frames) in all three: only the token stage differs.
set -uo pipefail
cd "$(dirname "$0")/.."; R=$PWD; D=$(date -u +%Y-%m-%d); L=results/colab-$D-gpu-rans.log; mkdir -p results
W=${WORK:-/content/work}; mkdir -p $W; C=$W/chr1.fa
{ echo "== provenance $D"; nvidia-smi -L; nvidia-smi --query-gpu=driver_version,clocks.max.sm --format=csv,noheader
  nvcc --version | tail -1; g++ --version | head -1; git rev-parse --short HEAD; nproc; } 2>&1 | tee $L
[ -f /usr/include/zstd.h ] || { apt-get -qq update && apt-get -qq install -y libzstd-dev >/dev/null; }
grep -h '#define ZSTD_VERSION_\(MAJOR\|MINOR\|RELEASE\)' /usr/include/zstd.h | awk '{print $3}' | paste -sd. - | sed 's/^/libzstd /' | tee -a $L
pip -q install nvidia-nvcomp-cu12 2>&1 | tail -1
NV=$(dirname "$(dirname "$(find / -name libnvcomp.so.5 -path '*libnvcomp/lib64*' 2>/dev/null | head -n 1)")")
[ -f "$NV/include/nvcomp/zstd.h" ] || { echo "nvCOMP not found" | tee -a $L; exit 1; }
pip show nvidia-nvcomp-cu12 2>/dev/null | grep -i '^version' | sed 's/^/nvcomp /' | tee -a $L
export LD_LIBRARY_PATH=$NV/lib64:${LD_LIBRARY_PATH:-}
SM=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader | head -n 1 | tr -d '.')
make -s 2>&1 | grep -i error; [ -x ./aceapex ] || { echo "no CLI" | tee -a $L; exit 1; }
if nvcc -O3 -arch=sm_$SM -I$NV/include -L$NV/lib64 -l:libnvcomp.so.5 -o $W/aceapex_gpu aceapex_gpu.cu 2>$W/nvcc.err; then
  echo "built aceapex_gpu sm_$SM" | tee -a $L
else echo "BUILD FAILED aceapex_gpu" | tee -a $L; cat $W/nvcc.err; exit 1; fi
g++ -std=c++17 -O2 -Isrc -o $W/rans_warp_emu scripts/rans_warp_emu.cpp && $W/rans_warp_emu verify/fixtures/conf/*.aet | tee -a $L
[ -s $C ] || curl -sL https://hgdownload.soe.ucsc.edu/goldenPath/hg38/chromosomes/chr1.fa.gz | gunzip -c > $C
echo "9465e0f0df6e2c6eb39729c39cee5465  $C" | md5sum -c - | tee -a $L || exit 1
T=$(nproc)
for P in zstd rans rans4k; do
  case $P in zstd) E="FSE_CHUNK=4096";; rans) E="AX_TOK=rans";; rans4k) E="AX_TOK=rans FSE_CHUNK=4096";; esac
  A=$W/chr1.$P.aet
  env -i PATH=$PATH ACEAPEX_BS=16384 LIT_CHUNK=65536 $E ./aceapex c --in $C --out $A --threads $T >/dev/null 2>&1
  env -i PATH=$PATH ./aceapex d --in $A --out $W/rt.bin >/dev/null 2>&1
  cmp -s $W/rt.bin $C && echo "$P ($E): archive $(stat -c%s $A) B, CPU round-trip bit-perfect" | tee -a $L \
    || { echo "$P: CPU ROUND-TRIP FAILED" | tee -a $L; exit 1; }
  rm -f $W/rt.bin
done
for P in zstd rans rans4k; do
  echo "== aceapex_gpu $P" | tee -a $L
  $W/aceapex_gpu $W/chr1.$P.aet $C auto 7 4 2>&1 | tee -a $L; echo "exit ${PIPESTATUS[0]}" | tee -a $L
done
echo; echo "chr1 on $(nvidia-smi --query-gpu=name --format=csv,noheader | head -n 1), ms, median of 7 (tok = token entropy stage)" | tee -a $L
{ printf 'archive\tbytes\ttokens\tliterals\ttok\tlit\tunpack\tmatch\ton-device\t+H2D\tGB/s\tcheck\n'
  grep '^ROW' $L | cut -f2-13 | sed "s#$W/##"; } | column -t -s $'\t' | tee -a $L.table
cat $L.table >> $L; rm -f $L.table
N=$(awk -F'\t' '$1=="ROW" && $13=="bit-perfect"' $L | wc -l); echo "bit-perfect rows: $N of 3" | tee -a $L
# verdict from the lines each tool prints for a failure, not from free text (the emulator's
# summary says "rejected by both" on a pass): 3 bit-perfect rows, 3 exits 0, emulator pass,
# no FNV mismatch, no rANS chunk rejected on the device
X=$(grep -c '^exit 0$' $L); E=$(grep -c $'^head_rans_warp_emu\tpass' $L)
[ "$N" = 3 ] && [ "$X" = 3 ] && [ "$E" = 1 ] && ! grep -q 'DIFFERS X\|archive rejected\|MISMATCH' $L \
  && echo "RESULT: all three archives bit-perfect on the GPU" | tee -a $L \
  || { echo "!!! NOT PASSED - no figure from this run is valid" | tee -a $L; exit 1; }
