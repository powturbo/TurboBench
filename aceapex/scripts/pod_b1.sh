#!/usr/bin/env bash
# POD DAY step 1+2: provenance, build, chr1 default and interactive archives from the
# current CLI, streams.bin, e2e_full on the GPU: MATCHES OK is the gate for the day.
set -uo pipefail
cd "$(dirname "$0")/.."; D=$(date -u +%Y-%m-%d); L=results/pod-$D-b1.log; mkdir -p results
{ echo "== provenance $D"; nvidia-smi -L; nvcc --version | tail -1; g++ --version | head -1
  grep -h '#define ZSTD_VERSION_\(MAJOR\|MINOR\|RELEASE\)' /usr/include/zstd.h 2>/dev/null | awk '{print $3}' | paste -sd. -
  git rev-parse --short HEAD; nproc; df -h . | tail -1; } | tee $L
make -s 2>&1 | grep -i error; [ -x ./aceapex ] || { echo "no CLI"; exit 1; }
for k in e2e_full e2e_seek dense full_gpu_decode_v7_ra; do nvcc -O3 -arch=sm_90 -o /tmp/$k $k.cu 2>/tmp/$k.err && echo "built $k" || echo "BUILD FAILED $k"; done | tee -a $L
G=${GOLDEN:-$HOME/golden}; C=$G/genome/chr1.fa; mkdir -p $G/genome
[ -s $C ] || curl -sL https://hgdownload.soe.ucsc.edu/goldenPath/hg38/chromosomes/chr1.fa.gz | gunzip -c > $C
echo "9465e0f0df6e2c6eb39729c39cee5465  $C" | md5sum -c - | tee -a $L
for P in default interactive; do
  if [ $P = interactive ]; then E="LIT_CHUNK=65536 FSE_CHUNK=4096"; else E=""; fi
  W=/tmp/pod_$P; rm -rf $W; mkdir -p $W
  env -i PATH=$PATH $E ./aceapex c --in $C --out $W/a.aet --threads 8 >/dev/null 2>&1
  ( cd $W && env -i PATH=$PATH ACEAPEX_DUMP=1 $OLDPWD/aceapex d --in a.aet --out o.bin >/dev/null 2>&1 )
  cmp -s $W/o.bin $C && echo "$P: CPU decode bit-perfect, archive $(stat -c%s $W/a.aet) B" | tee -a $L
  for i in 1 2 3 4; do /tmp/e2e_full $W/streams.bin $C 16 2>&1 | grep -E 'MATCHES|DIFFERS|timed|GB/s' | sed "s/^/$P run$i: /"; done | tee -a $L
done
N=$(grep -c 'MATCHES OK' $L); echo "MATCHES OK lines: $N (need 8: 2 profiles x 4 runs)"
if grep -q 'DIFFERS\|BUILD FAILED' $L || [ "$N" -lt 8 ]; then echo "!!! GATE NOT PASSED — no GPU figure is valid until MATCHES OK x8 with no DIFFERS"; exit 1; else echo "gate passed"; fi
