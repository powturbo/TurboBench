#!/usr/bin/env bash
# #D1: axf vs zxc -1 vs misa77 -1 inside one lzbench binary (local tree .wk/lzb-intmt, plain make, no -march),
# lzbench's published single-thread method: -t8,8 -o1c4, one pinned core; 7 runs, median taken afterwards.
# lzbench.plain: axf TU without -mavx2 (as lzbench builds); lzbench.avx2: the same with the axf TU at -mavx2.
set -uo pipefail
C=~/golden/mixed/silesia.tar; D=~/pubrepo/.wk/lzb-intmt; O=~/pubrepo/.wk/scb/src/research/fastblk/d1-lzbench-2026-10-26.log
{ echo "# $(date -u '+%F %H:%M') $(lscpu | sed -n 's/Model name: *//p'); governor $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo n/a); load $(cut -d' ' -f1-3 /proc/loadavg)"
  echo "# lzbench $(git -C $D log -1 --format=%h) + local axf codec; axf levels: 1=mm16 2=mm12 3=mm8 5=mm5 (greedy/lazy), 4=mm5 optimal parse (depth 512, lambda 2)"
  for r in 1 2 3 4 5 6 7; do for V in plain avx2; do
    echo "## run $r $V: taskset -c 2 lzbench.$V -eaxf,1,2,3,5,4/zxc,1/misa77,1 -t8,8 -o1c4 silesia.tar"
    taskset -c 2 $D/lzbench.$V -eaxf,1,2,3,5,4/zxc,1/misa77,1 -t8,8 -o1c4 $C 2>&1 | grep -E '^(axf|zxc|misa77)'
  done; done; echo done; } > $O 2>&1
