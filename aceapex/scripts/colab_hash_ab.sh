#!/usr/bin/env bash
# #N1 on a CUDA VM (Colab via `colab run`): A/B of output hashing on the GPU (scripts/gpu_hash_ab.cu) on chr1,
# open profile: a) decode only, b) decode + XXH3 (kg_xxh_blocks + kg_xxh_chain), c) decode + nvCOMP 6.0.0
# nvcompBatchedCRC32Async per block + crc32_combine on the host. 10 runs per mode, median / p90.
# Check: the file CRC of mode c == zlib.crc32 of the CPU decode. ncu on the kg_xxh kernels (--set full).
# The whole job is cut at 14 minutes of wall clock (timeout), whatever the balance.
set -uo pipefail
cd "$(dirname "$0")/.."; D=$(date -u +%Y-%m-%d); W=${WORK:-/content/work}; mkdir -p $W results/nvcomp-crc
L=results/nvcomp-crc/colab-$D-hash-ab.log
nvidia-smi --query-gpu=name,memory.total,clocks.max.sm,driver_version --format=csv > $L 2>&1
{ nvcc --version | tail -1; git rev-parse --short HEAD; nproc; } >> $L 2>&1
[ -f /usr/include/zstd.h ] || { apt-get -qq update && apt-get -qq install -y libzstd-dev zlib1g-dev >/dev/null; }
CU=$(nvcc --version | sed -n 's/.*release \([0-9]*\)\..*/\1/p'); PK=nvidia-libnvcomp-cu$CU
pip -q install "$PK==6.0.0.12" 2>&1 | tail -1
NV=$(python3 -c "import nvidia.libnvcomp, os; print(os.path.dirname(nvidia.libnvcomp.__file__) if nvidia.libnvcomp.__file__ else list(nvidia.libnvcomp.__path__)[0])" 2>/dev/null)
[ -f "$NV/include/nvcomp/crc32.h" ] || NV=$(dirname "$(dirname "$(find / -name crc32.h -path '*nvcomp/crc32.h' 2>/dev/null | head -n 1)")")
NV=${NV%/include}; echo "nvcomp: $PK at $NV" | tee -a $L
LIB=$(ls -d $NV/lib64 $NV/lib 2>/dev/null | head -n 1); export LD_LIBRARY_PATH=$LIB:${LD_LIBRARY_PATH:-}
SM=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader | head -n 1 | tr -d '.')
make -s 2>&1 | grep -i error; [ -x ./aceapex ] || { echo "no CLI" | tee -a $L; exit 1; }
nvcc -O3 -std=c++17 -arch=sm_$SM -lineinfo -Isrc -I$NV/include -o $W/gpu_hash_ab scripts/gpu_hash_ab.cu src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp \
     -L$LIB -l:libnvcomp.so.6 -lz 2>$W/nvcc.err || { cat $W/nvcc.err | tee -a $L; exit 1; }
if ! echo "9465e0f0df6e2c6eb39729c39cee5465  $W/chr1.fa" | md5sum -c - >/dev/null 2>&1; then
  for t in 1 2 3; do curl -sfL --retry 5 -o $W/chr1.fa.gz https://hgdownload.soe.ucsc.edu/goldenPath/hg38/chromosomes/chr1.fa.gz && gunzip -c $W/chr1.fa.gz > $W/chr1.fa && break; sleep 5; done
fi
echo "9465e0f0df6e2c6eb39729c39cee5465  $W/chr1.fa" | md5sum -c - >> $L || exit 1
env -i PATH=$PATH AX_PROFILE=open ./aceapex c --in $W/chr1.fa --out $W/chr1.open.aet --threads $(nproc) >/dev/null 2>&1
echo "archive chr1.open.aet $(stat -c%s $W/chr1.open.aet) B, sha256 $(sha256sum $W/chr1.open.aet | cut -c1-16)" | tee -a $L
echo "== gpu_hash_ab" | tee -a $L
$W/gpu_hash_ab $W/chr1.open.aet 10 2>&1 | tee -a $L
GPU_CRC=$(grep -oE 'file CRC-32 [^:]*: [0-9a-f]{8}' $L | grep -oE '[0-9a-f]{8}$')
./aceapex d --in $W/chr1.open.aet --out $W/chr1.cpu >/dev/null 2>&1
CPU_CRC=$(python3 -c "import zlib,sys; print('%08x' % (zlib.crc32(open(sys.argv[1],'rb').read()) & 0xffffffff))" $W/chr1.cpu)
cmp -s $W/chr1.cpu $W/chr1.fa && echo "CPU decode bit-perfect" | tee -a $L
[ "$GPU_CRC" = "$CPU_CRC" ] && echo "CRC check: GPU $GPU_CRC == zlib.crc32(CPU decode) $CPU_CRC  OK" | tee -a $L || echo "CRC check: GPU $GPU_CRC != CPU $CPU_CRC  FAIL" | tee -a $L
NCU=$(command -v ncu || ls /usr/local/cuda/bin/ncu 2>/dev/null | head -n 1)
if [ -n "$NCU" ]; then
  timeout 300 $NCU --kernel-name regex:kg_xxh --launch-count 2 --set full -f -o results/nvcomp-crc/kg_xxh-$D $W/gpu_hash_ab $W/chr1.open.aet 1 > $W/ncu.out 2>&1
  echo "ncu: rc=$? $(tail -n 2 $W/ncu.out | tr '\n' ' ')" | tee -a $L
  # kg_xxh_blocks and kg_xxh_chain apart (for the NVIDIA forum): duration of each kernel launch, ms, from the report;
  # then a duration-only pass over 6 launches (no replay overhead of --set full) for the same split
  echo "ncu per kernel (gpu__time_duration.sum, from the --set full report):" | tee -a $L
  $NCU --import results/nvcomp-crc/kg_xxh-$D.ncu-rep --page raw --csv --metrics gpu__time_duration.sum 2>/dev/null \
    | python3 -c "import csv,sys; r=list(csv.reader(sys.stdin)); h=r[0]; k=h.index('Kernel Name'); t=h.index('gpu__time_duration.sum'); u=r[1][t] if len(r)>1 else ''; [print(f'  {x[k].split(\"(\")[0]}: {x[t]} {u}') for x in r[2:]]" | tee -a $L
  timeout 300 $NCU --kernel-name regex:kg_xxh --launch-count 6 --metrics gpu__time_duration.sum --csv $W/gpu_hash_ab $W/chr1.open.aet 3 2>/dev/null \
    | python3 -c "import csv,sys,collections; r=[x for x in csv.reader(sys.stdin) if len(x)>10]; h=r[0]; k=h.index('Kernel Name'); v=h.index('Metric Value'); u=h.index('Metric Unit'); d=collections.defaultdict(list); [d[x[k].split('(')[0]].append((float(x[v].replace(',','')),x[u])) for x in r[1:]]; [print(f'  {n}: {len(a)} launches, ' + ', '.join(f'{t:g} {un}' for t,un in a)) for n,a in d.items()]" | tee -a $L
  ls -la results/nvcomp-crc/*.ncu-rep 2>/dev/null | tee -a $L
else echo "ncu: not found on this VM" | tee -a $L; fi
(cd results/nvcomp-crc && sha256sum * > SHA256SUMS) ; cat results/nvcomp-crc/SHA256SUMS | tee -a $L
