#!/bin/bash
# ПРИОРИТЕТ 2: negative result на ВСЕХ 12 Silesia (писатель п.5).
# Проверяет: throughput коррелирует с РАЗМЕРОМ (число блоков), НЕ с типом данных.
set +e
DEPTH=/workspace/aceapex/aceapex_depth
SF=/workspace/data/silesia_files
OUT=/tmp/silesia_all.txt
: > "$OUT"
cd /workspace/aceapex
export LD_LIBRARY_PATH=/workspace/dietgpu/build/lib:$LD_LIBRARY_PATH
export ACEAPEX_BS=16384
log(){ echo "$@" | tee -a "$OUT"; }
log "=== СТАТЬЯ 4: Silesia все 12 — throughput vs size (не type) ==="
log "file | size_MB | throughput | FNV"
for f in dickens mozilla mr nci ooffice osdb reymont samba sao webster x-ray xml; do
  src="$SF/$f"
  [ ! -f "$src" ] && { log "$f: НЕТ ФАЙЛА"; continue; }
  sz=$(stat -c%s "$src"); szmb=$((sz/1000000))
  $DEPTH c --in "$src" --out /tmp/c.aet --threads 8 2>/dev/null
  $DEPTH d --in /tmp/c.aet --out /tmp/c_dec 2>/dev/null
  tp=$(./e2e_pipe_tile streams.bin "$src" 32 2>&1 | grep -oE '\-> [0-9.]+ GB/s' | grep -oE '[0-9.]+')
  fnv=$(./e2e_pipe_tile streams.bin "$src" 32 2>&1 | grep -oE 'MATCHES OK|DIFFERS')
  log "$f | ${szmb}MB | ${tp:-?} GB/s | ${fnv:-?}"
  rm -f /tmp/c.aet /tmp/c_dec streams.bin
done
log ""
log "ЧИТАТЬ: если throughput растёт с size (не с типом) -> occupancy-тезис укреплён."
log "Отсортировать по size, проверить монотонность throughput."
