#!/usr/bin/env bash
# Colab / any CUDA host: P2 of docs/ARCH_V3.md - production v7-RA match kernel vs the v3 warp-scan kernel on the
# same matches (research/v3proto/v3gpu.cu). One cell:
#   !rm -rf /content/aceapex && git clone -q -b fast-probe https://github.com/yasha1971-coder/aceapex /content/aceapex && bash /content/aceapex/scripts/colab_v3gpu.sh
# Builds the CLI, writes archives of silesia and chr1 (default profile, 16 KiB blocks like the GPU profile),
# dumps their decoded streams (ACEAPEX_DUMP=1), runs v3gpu (match phase on the device, median of 9, FNV check).
set -uo pipefail
cd "$(dirname "$0")/.."; D=$(date -u +%Y-%m-%d); L=results/colab-$D-v3gpu.log; mkdir -p results
W=${WORK:-/content/work}; mkdir -p $W
{ echo "== provenance $D"; nvidia-smi -L; nvcc --version | tail -1; git rev-parse --short HEAD; nproc; } 2>&1 | tee $L
[ -f /usr/include/zstd.h ] || { apt-get -qq update && apt-get -qq install -y libzstd-dev >/dev/null; }
make -s 2>&1 | grep -i error; [ -x ./aceapex ] || { echo "no CLI" | tee -a $L; exit 1; }
SM=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader | head -n 1 | tr -d '.')
nvcc -O3 -std=c++17 -arch=sm_$SM -Isrc -o $W/v3gpu research/v3proto/v3gpu.cu 2>$W/nvcc.err || { cat $W/nvcc.err; exit 1; }
[ -s $W/chr1.fa ] || curl -sL https://hgdownload.soe.ucsc.edu/goldenPath/hg38/chromosomes/chr1.fa.gz | gunzip -c > $W/chr1.fa
echo "9465e0f0df6e2c6eb39729c39cee5465  $W/chr1.fa" | md5sum -c - | tee -a $L || exit 1
if [ ! -s $W/silesia.tar ]; then
  (cd $W && { curl -sfL -o silesia.zip https://github.com/DataCompression/corpus-collection/raw/main/Silesia-Corpus/silesia.zip \
      || curl -sfL -o silesia.zip http://sun.aei.polsl.pl/~sdeor/corpus/silesia.zip; } && mkdir -p sil && cd sil && unzip -oq ../silesia.zip && tar cf ../silesia.tar $(ls | sort))
fi
[ -s $W/silesia.tar ] && echo "silesia.tar $(stat -c%s $W/silesia.tar) B" | tee -a $L || echo "silesia not available, chr1 only" | tee -a $L
for f in chr1.fa silesia.tar; do [ -s $W/$f ] || continue
  env -i PATH=$PATH ACEAPEX_BS=16384 ./aceapex c --in $W/$f --out $W/$f.aet --threads $(nproc) >/dev/null 2>&1
  (cd $W && env -i PATH=$PATH ACEAPEX_DUMP=1 $OLDPWD/aceapex d --in $f.aet --out /dev/null >/dev/null 2>&1 && mv streams.bin $f.streams) || { echo "dump failed $f" | tee -a $L; continue; }
  echo "== $f (16 KiB blocks)" | tee -a $L
  $W/v3gpu $W/$f.streams $W/$f 9 2>&1 | tee -a $L
done
grep -q MISMATCH $L && { echo "!!! MISMATCH - no figure valid" | tee -a $L; exit 1; } || echo "RESULT: all rows bit-perfect" | tee -a $L
