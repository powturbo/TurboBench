#!/usr/bin/env bash
# run_perf.sh - HW-ENC2 decode cost on ace-core: the archive of each variant decoded alone (enc2 --dec, one thread, pinned),
# under perf stat: cycles, instructions, branch misses per byte of output. Plus the token-stream structure (--stats).
# Usage: bash run_perf.sh [REPO=~/pubrepo] [CORE=2]   env GOLDEN (default ~/golden). Writes ~/hwenc2perf-<date>-<commit>.log
set -euo pipefail
REPO=${1:-$HOME/pubrepo}; CORE=${2:-2}; G=${GOLDEN:-$HOME/golden}; HERE=$(cd "$(dirname "$0")" && pwd)
C=$(git -C "$REPO" rev-parse --short HEAD); OUT=$HOME/hwenc2perf-$(date -u +%F)-$C.log; F=$G/mixed/silesia.tar
[ -f "$F" ] || { echo "STOP: $F missing"; exit 2; }
L1=$(cut -d' ' -f1 /proc/loadavg); awk -v l="$L1" 'BEGIN{exit !(l < 0.5)}' || { echo "STOP: load1 $L1 >= 0.5, run later"; exit 3; }
B=$(mktemp -d); trap 'rm -rf "$B"' EXIT
g++ -O3 -std=c++17 -march=native -I"$REPO/src" "$HERE/enc2.cpp" -o "$B/enc2"
VARIANTS=(l11 slot "w32h17:hb=17,base=3,pi=2" "w43h17:hb=17,base=4,pi=3" "w43h16:hb=16,base=4,pi=3")
N=$(stat -L -c %s "$F"); R=20
HAVE_PERF=0; command -v perf >/dev/null && perf stat -e cycles true >/dev/null 2>&1 && HAVE_PERF=1
{
echo "# HW-ENC2 decode cost $(date -u '+%F %H:%M') UTC $(hostname) repo $C; $(lscpu | sed -n 's/Model name: *//p' | head -1); core $CORE; load $(cut -d' ' -f1-3 /proc/loadavg); perf $HAVE_PERF"
echo "## stream structure (silesia)"
"$B/enc2" --stats "$F" "${VARIANTS[@]}"
"$B/enc2" --write "$F" "$B" "${VARIANTS[@]}" > /dev/null
echo "## decode alone, $R runs each, per output byte over all runs (counters include one file read)"
printf 'variant\tMB/s_median\tcycles/B\tinstr/B\tbranch_miss/B\n'
for v in "${VARIANTS[@]}"; do
  name=${v%%:*}
  if [ $HAVE_PERF = 1 ]; then
    perf stat -x, -e cycles,instructions,branch-misses -o "$B/p.txt" taskset -c "$CORE" "$B/enc2" --dec "$B/$name.aet" $R > "$B/d.txt"
    awk -F, -v n=$N -v r=$R -v nm="$name" -v s="$(cut -f2 "$B/d.txt" | cut -d' ' -f1)" '!/^#/ && NF>2 {k=$3; sub(/:.*/,"",k); c[k]=$1} END{t=n*r; printf "%s\t%s\t%.3f\t%.3f\t%.5f\n", nm, s, c["cycles"]/t, c["instructions"]/t, c["branch-misses"]/t}' "$B/p.txt"
  else taskset -c "$CORE" "$B/enc2" --dec "$B/$name.aet" $R; fi
done
echo "# load after $(cut -d' ' -f1-3 /proc/loadavg)"
} 2>&1 | tee "$OUT"
echo "written $OUT"
