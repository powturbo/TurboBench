#!/usr/bin/env bash
# fast_bench.sh - the FAST profile on this host: `aceapex t` (encode, then 7 timed decodes, median; every run compared
# with the input) at 1 thread and at N threads, levels 11-14 on every corpus found, 15-18 on silesia only (the optimal
# parse encodes at ~1 MB/s per core). One markdown table + the raw lines to results/fast-bench-<date>-<commit>.md.
# Usage: bash scripts/fast_bench.sh [threads=8]   (corpora: $GOLDEN/mixed/silesia.tar, $GOLDEN/genome/chr1.fa,
# $GOLDEN/genome/t2t.fa, $GOLDEN/text/enwik8, $GOLDEN/text/enwik9 - whichever exist; CORPORA=<list> overrides)
set -uo pipefail
N=${1:-8}; G=${GOLDEN:-$HOME/golden}; B=${BIN:-./aceapex}
OUT=results/fast-bench-$(date -u +%F)-$(git rev-parse --short HEAD 2>/dev/null || echo nogit).md
C=${CORPORA:-"$G/mixed/silesia.tar $G/genome/chr1.fa $G/text/enwik8 $G/text/enwik9 $G/genome/t2t.fa"}
{
echo "# FAST profile bench - $(date -u '+%F %H:%M') UTC, $(hostname), commit $(git rev-parse --short HEAD 2>/dev/null)"
echo "# $(lscpu | sed -n 's/Model name: *//p' | head -1); governor $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo n/a); load $(cut -d' ' -f1-3 /proc/loadavg); $($B --version)"
echo "# method: $B t --level L --threads T: encode on T threads, then 7 decodes of the in-memory archive on T threads, median (min in the raw lines); bit-perfect compared on every run"
echo
echo "| corpus | level | min match | ratio (%) | encode MB/s (T=$N) | decode 1 thread MB/s | decode $N threads MB/s |"
echo "|---|---|---|---|---|---|---|"
for f in $C; do
  [ -s "$f" ] || continue; sz=$(stat -c%s "$f"); nm=$(basename "$f")
  case "$nm" in silesia*) LV="11 12 13 14 15 16 17 18";; *) LV="11 12 13 14";; esac
  for L in $LV; do
    r1=$(env -i PATH="$PATH" $B t --in "$f" --threads 1 --level $L 2>&1); rN=$(env -i PATH="$PATH" $B t --in "$f" --threads $N --level $L 2>&1)
    ok1=$(echo "$r1" | grep -c BIT-PERFECT); okN=$(echo "$rN" | grep -c BIT-PERFECT)
    mm=$(echo "$rN" | sed -n 's/.*min match \([0-9]*\),.*/\1/p'); cz=$(echo "$rN" | sed -n 's/ *Compressed: *\([0-9]*\) bytes/\1/p')
    enc=$(echo "$rN" | sed -n 's/ *Encode: \([0-9.]*\) MB\/s.*/\1/p'); d1=$(echo "$r1" | sed -n 's/ *Decode: \([0-9.]*\) MB\/s.*/\1/p'); dN=$(echo "$rN" | sed -n 's/ *Decode: \([0-9.]*\) MB\/s.*/\1/p')
    ratio=$(python3 -c "print('%.2f' % (100.0*$cz/$sz))" 2>/dev/null)
    printf '| %s | %s | %s | %s | %s | %s | %s |%s\n' "$nm" "$L" "$mm" "$ratio" "$enc" "$d1" "$dN" "$([ "$ok1$okN" = 11 ] || echo ' MISMATCH')"
    echo "RAW $nm L$L T1: $(echo "$r1" | grep -E 'Decode:' | tr -d '\n')" >&2
    echo "RAW $nm L$L T$N: $(echo "$rN" | grep -E 'Compressed|Encode|Decode:' | tr '\n' ' ')" >&2
  done
done
} > "$OUT" 2> "${OUT%.md}.raw"
cat "$OUT"; echo "written $OUT and ${OUT%.md}.raw"
