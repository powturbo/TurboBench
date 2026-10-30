#!/bin/bash
# =============================================================================
# nsight_profile.sh — Paper 4 mechanism gap: separate occupancy / L2 / chain-depth
#
# Closes the \HOLE{Nsight separation of (1)/(2)/(3)} in aceapex_paper4.tex.
# No new idea, no new code path — pure measurement on the existing, already
# bit-perfect fgd_v3 (full_gpu_decode_v3.cu) binary, across the same
# block_size sweep points already published in Table II of the paper.
#
# Metrics:
#   sm__warps_active.avg.pct_of_peak_sustained_active  -> achieved occupancy
#   lts__t_sector_hit_rate.pct                          -> L2 hit rate
#
# Two datasets chosen deliberately for contrast (per the paper's own
# "occupancy necessary but not sufficient" point):
#   FASTQ   -> hits a data-type ceiling even at full occupancy (275 GB/s cap)
#   Silesia -> the strongest pure-occupancy effect (24.6x, still climbing at 8K)
#
# Usage: bash nsight_profile.sh
# Output: nsight_fastq.csv, nsight_silesia.csv (append one row per block_size)
# =============================================================================
set -e
cd /workspace/aceapex
DEPTH=/workspace/aceapex/aceapex_depth
V3=/workspace/aceapex/fgd_v3

# --- ensure ncu is available; install if missing (RunPod images vary) ---
if ! command -v ncu &> /dev/null; then
  echo "ncu not found -- attempting install via CUDA toolkit repo..."
  apt-get update -qq
  apt-get install -y -qq cuda-nsight-compute-12-4 2>&1 | tail -5 || \
    echo "WARNING: auto-install failed. Install Nsight Compute manually before proceeding:
    https://developer.nvidia.com/tools-overview/nsight-compute/get-started"
fi
command -v ncu &> /dev/null && echo "ncu found: $(ncu --version | head -1)" || { echo "ABORT: ncu still missing"; exit 1; }

METRICS="sm__warps_active.avg.pct_of_peak_sustained_active,lts__t_sector_hit_rate.pct"

profile_one () {
  local name=$1 src=$2
  local out="/workspace/nsight_${name}.csv"
  : > "$out"
  echo "dataset,block_size,blocks,achieved_occupancy_pct,l2_hit_rate_pct" >> "$out"
  echo "=== Nsight sweep: $name ==="
  for bs in 1048576 262144 65536 32768 16384 8192; do
    export ACEAPEX_BS=$bs
    $DEPTH c --in "$src" --out /tmp/n.aet --threads 8 2>/dev/null
    $DEPTH d --in /tmp/n.aet --out /tmp/n_dec 2>/dev/null
    # ncu --csv prints a header + one data row per kernel launch; we grep the
    # k_decode kernel row and pull the two metric columns.
    raw=$(ncu --metrics "$METRICS" --csv "$V3" streams.bin "$src" 2>/dev/null)
    occ=$(echo "$raw" | grep -i "warps_active" | tail -1 | awk -F',' '{print $NF}' | tr -d '"')
    l2=$(echo "$raw"  | grep -i "sector_hit_rate" | tail -1 | awk -F',' '{print $NF}' | tr -d '"')
    blk=$(echo "$raw" | grep -oE 'blocks=[0-9]+' | grep -oE '[0-9]+' | head -1)
    echo "$name,$bs,${blk:-NA},${occ:-NA},${l2:-NA}" >> "$out"
    printf "  bs=%-8s occ=%-8s l2_hit=%-8s\n" "$bs" "${occ:-NA}" "${l2:-NA}"
    unset ACEAPEX_BS
  done
  rm -f /tmp/n.aet /tmp/n_dec streams.bin
  echo "saved: $out"
}

profile_one fastq   /workspace/data/NA12878_1gb.fastq
profile_one silesia /workspace/data/silesia.tar

echo ""
echo "=== DONE. Two CSVs ready for the paper's Mechanism section: ==="
echo "  /workspace/nsight_fastq.csv"
echo "  /workspace/nsight_silesia.csv"
echo ""
echo "Expected reading (to confirm or correct the paper's claim):"
echo "  - achieved_occupancy should rise monotonically as block_size shrinks,"
echo "    flattening near the point where curves in Fig.1 flatten."
echo "  - l2_hit_rate rising alongside occupancy would mean cache residency is"
echo "    ALSO contributing (not occupancy alone) -- report honestly either way."
