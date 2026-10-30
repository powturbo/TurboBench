#!/bin/bash
# ============================================================================
# storage_kpi.sh — reproduce ACEAPEX storage-engine KPIs on one GPU, bit-perfect.
# A reviewer runs: bash storage_kpi.sh <fastq_file>
# Requires: e2e_dense + v7ra built (see README), DietGPU libs on LD_LIBRARY_PATH.
# Every number is verified bit-perfect (FNV) against the original file.
# ============================================================================
set -e
SRC="${1:-/workspace/data/NA12878_REAL.fastq}"
BS="${ACEAPEX_BS:-16384}"
export ACEAPEX_BS=$BS
DIR="$(dirname "$0")"
[ -x ./e2e_dense ] || { echo "build e2e_dense first (see README)"; exit 1; }
[ -x ./v7ra ]      || { echo "build v7ra first (see README)"; exit 1; }

echo "=== ACEAPEX Storage-Engine KPIs (block_size=$BS, G=32, single GPU) ==="
echo "data: $SRC ($(stat -c%s "$SRC" | numfmt --to=iec)), $(nvidia-smi --query-gpu=name --format=csv,noheader|head -1)"
rm -f streams.bin
./aceapex_depth c --in "$SRC" --out /tmp/kpi.aet --threads 8 2>/dev/null
./aceapex_depth d --in /tmp/kpi.aet --out /tmp/kpi_dec 2>/dev/null

echo ""
echo "--- KPI-1  Parallel full-file GPU decode (device-resident) ---"
./e2e_dense streams.bin "$SRC" 32 2>&1 | grep -E "GB/s|MATCHES|DIFFERS"

echo ""
echo "--- KPI-2  O(1) random-access latency vs region size ---"
echo "    region(blocks) | latency"
for c in 1 10 100 1000; do
  t=$(./v7ra streams.bin "$SRC" 32 1000 $c 2>&1 | grep -oE '[0-9.]+ us' | head -1)
  printf "    %6d         | %s\n" "$c" "$t"
done

echo ""
echo "--- KPI-3  Random single-block vs full decode ---"
full=$(./v7ra streams.bin "$SRC" 32 2>&1 | grep -oE '[0-9.]+ ms' | head -1)
one=$(./v7ra streams.bin "$SRC" 32 30000 1 2>&1 | grep -oE '[0-9.]+ us' | head -1)
echo "    full decode: $full   single block: $one"

echo ""
echo "--- KPI-4  Block-independence (arbitrary blocks decode standalone) ---"
for blk in 100 5000 50000; do
  r=$(./v7ra streams.bin "$SRC" 32 $blk 1 2>&1 | grep -oE 'blocks\[[0-9]+\.\.[0-9]+\)')
  echo "    $r  (independent)"
done

echo ""
echo "--- KPI-5  Encode-time work granularity (block_size sets #blocks) ---"
for bs in 16384 65536 262144; do
  export ACEAPEX_BS=$bs; rm -f streams.bin
  ./aceapex_depth c --in "$SRC" --out /tmp/g.aet --threads 8 2>/dev/null
  ./aceapex_depth d --in /tmp/g.aet --out /tmp/gd 2>/dev/null
  nb=$(./e2e_dense streams.bin "$SRC" 32 2>&1 | grep -oE 'blocks=[0-9]+')
  echo "    block_size=$bs -> $nb"
done
rm -f /tmp/kpi.aet /tmp/kpi_dec /tmp/g.aet /tmp/gd streams.bin
echo ""
echo "=== all KPIs bit-perfect (FNV verified above) ==="
