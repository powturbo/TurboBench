#!/usr/bin/env bash
# w1_run.sh - W1 on this host: scripts/w1_wall.cpp per mode, one pinned core, each mode in its own process under
# `perf stat` so the DRAM traffic of that mode alone is counted. Writes results/w1-<date>-<commit>.log.
# Usage: bash scripts/w1_run.sh [corpus=$GOLDEN/mixed/silesia.tar] [core=2]
# PERF_EVENTS: the DRAM / L3-miss events for this CPU. Default: the generic cache-misses plus the AMD Zen data-fabric
# DRAM events when perf knows them (check `perf list | grep -i dram`); set it to the events used in
# results/cpu-ceiling-2026-10-01.log to keep the method of that log.
set -uo pipefail
C=${1:-${GOLDEN:-$HOME/golden}/mixed/silesia.tar}; CORE=${2:-2}
OUT=results/w1-$(date -u +%F)-$(git rev-parse --short HEAD 2>/dev/null || echo nogit).log
g++ -O3 -std=c++17 -Isrc -o /tmp/w1_wall scripts/w1_wall.cpp -lpthread || exit 1
EV=${PERF_EVENTS:-cache-misses}
HAVE_PERF=0; command -v perf >/dev/null && perf stat -e "$EV" true >/dev/null 2>&1 && HAVE_PERF=1
# perf older than 5.10 has no --control: then the counters would include the archive build - no counters at all instead
[ $HAVE_PERF = 1 ] && ! perf stat --help 2>&1 | grep -q -- '--control' && HAVE_PERF=0
{
echo "# W1 $(date -u '+%F %H:%M') UTC $(hostname) commit $(git rev-parse --short HEAD); $(lscpu | sed -n 's/Model name: *//p' | head -1); core $CORE; perf events: $([ $HAVE_PERF = 1 ] && echo "$EV" || echo 'none (perf missing or events unknown)')"
/tmp/w1_wall "$C" 1 1024 3 512 stats | sed -n 's/^stats/stats L1 1MiB/p'
/tmp/w1_wall "$C" 2 1024 3 512 stats | sed -n 's/^stats/stats L2 1MiB/p'
for LV in 1 2; do for B in 1024 256; do
  for M in memcpy base E0 pfw clz nt x2; do
    for A in $( [ "$M" = pfw ] || [ "$M" = clz ] && echo 256 512 2048 || echo 512 ); do
      if [ $HAVE_PERF = 1 ]; then
        rm -f /tmp/w1_ctl /tmp/w1_ack; mkfifo /tmp/w1_ctl /tmp/w1_ack
        W1_PERF_CTL=/tmp/w1_ctl W1_PERF_ACK=/tmp/w1_ack perf stat -x, -e "$EV" -D -1 --control fifo:/tmp/w1_ctl,/tmp/w1_ack -o /tmp/w1_perf.txt \
          taskset -c $CORE /tmp/w1_wall "$C" $LV $B 7 $A $M > /tmp/w1_out.txt 2>&1
        P=$(grep -v '^#' /tmp/w1_perf.txt | awk -F, 'NF>2{printf "%s=%s ", $3, $1}')
      else taskset -c $CORE /tmp/w1_wall "$C" $LV $B 7 $A $M > /tmp/w1_out.txt 2>&1; P=""; fi
      L=$(grep -E 'MB/s|skipped' /tmp/w1_out.txt | head -1)
      echo "L$LV B${B}K ahead $A | $L | $P"
    done
  done
done; done
echo "# counters cover only the 7 timed calls of the mode (perf --control); divide DRAM bytes by \"counted bytes of output\" for bytes per output byte"
} | tee "$OUT"
echo "written $OUT"
