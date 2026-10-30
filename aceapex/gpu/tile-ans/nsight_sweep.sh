#!/bin/bash
# ============================================================================
# nsight_sweep.sh — ПРИОРИТЕТ 1: разделить occupancy vs L2 vs chain для статьи 4.
# Снимает по каждой точке block_size: achieved occupancy, L2 hit-rate, DRAM throughput.
# + bit-perfect FNV на КАЖДОЙ точке (писатель п.3).
# Пишет /tmp/nsight_results.txt — компактно.
#
# ТРЕБУЕТ: ncu (Nsight Compute) на pod, e2e_pipe_tile собран, DietGPU окружение.
# ============================================================================
set +e
DEPTH=/workspace/aceapex/aceapex_depth
DATA=/workspace/data
OUT=/tmp/nsight_results.txt
: > "$OUT"
cd /workspace/aceapex
export LD_LIBRARY_PATH=/workspace/dietgpu/build/lib:$LD_LIBRARY_PATH

log(){ echo "$@" | tee -a "$OUT"; }
log "=== СТАТЬЯ 4 Nsight: occupancy vs L2 vs chain по block_size ==="
log "GPU: $(nvidia-smi --query-gpu=name --format=csv,noheader | head -1)"
log "ncu доступен: $(which ncu || echo НЕТ)"
log ""

# проверка ncu
if ! which ncu >/dev/null 2>&1; then
  log "ОШИБКА: ncu (Nsight Compute) не найден. Установить или проверить путь."
  log "Попробовать: /usr/local/cuda/bin/ncu или /opt/nvidia/nsight-compute/*/ncu"
  exit 1
fi

SRC=$DATA/NA12878_1gb.fastq
log "данные: fastq (богат матчами, чистый tile-ANS кейс)"
log "block | occupancy% | L2-hit% | DRAM-GB/s | throughput | FNV"

for bs in 16384 32768 65536 131072 262144; do
  export ACEAPEX_BS=$bs
  # готовим потоки
  $DEPTH c --in "$SRC" --out /tmp/c.aet --threads 8 2>/dev/null
  $DEPTH d --in /tmp/c.aet --out /tmp/c_dec 2>/dev/null
  # throughput (обычный прогон)
  tp=$(./e2e_pipe_tile streams.bin "$SRC" 32 2>&1 | grep -oE '\-> [0-9.]+ GB/s' | grep -oE '[0-9.]+')
  fnv=$(./e2e_pipe_tile streams.bin "$SRC" 32 2>&1 | grep -oE 'MATCHES OK|DIFFERS')
  # Nsight метрики (один прогон под ncu, только match-kernel)
  ncu --metrics \
sm__warps_active.avg.pct_of_peak_sustained_active,\
lts__t_sector_hit_rate.pct,\
dram__throughput.avg.pct_of_peak_sustained_elapsed \
--target-processes all --launch-count 1 \
./e2e_pipe_tile streams.bin "$SRC" 32 > /tmp/ncu_$bs.txt 2>&1
  occ=$(grep -oE 'warps_active[^0-9]*[0-9.]+' /tmp/ncu_$bs.txt | grep -oE '[0-9.]+$' | head -1)
  l2=$(grep -oE 'sector_hit_rate[^0-9]*[0-9.]+' /tmp/ncu_$bs.txt | grep -oE '[0-9.]+$' | head -1)
  dram=$(grep -oE 'dram__throughput[^0-9]*[0-9.]+' /tmp/ncu_$bs.txt | grep -oE '[0-9.]+$' | head -1)
  log "$bs | occ=${occ:-?} | L2=${l2:-?} | DRAM=${dram:-?} | ${tp:-?} GB/s | ${fnv:-?}"
  unset ACEAPEX_BS; rm -f /tmp/c.aet /tmp/c_dec streams.bin
done

log ""
log "=== ИНТЕРПРЕТАЦИЯ (читать данные, не гипотезу) ==="
log "throughput растёт + occupancy растёт + L2 плоский -> occupancy-тезис ПОДТВЕРЖДЁН"
log "throughput растёт + L2-hit растёт на мелких -> тезис 'CACHE не occupancy', ПЕРЕПИСАТЬ"
log "полные ncu-отчёты: /tmp/ncu_<block>.txt"
