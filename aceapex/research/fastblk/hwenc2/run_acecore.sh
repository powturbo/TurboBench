#!/usr/bin/env bash
# run_acecore.sh - HW-ENC2 verdict run on ace-core. Measures only; changes nothing in the repository.
# Usage: bash run_acecore.sh [REPO=~/pubrepo] [CORE=2]
#   env GOLDEN (default ~/golden), LZB (optional: path to an lzbench binary with zxc and aceapex_fast, for reference rows)
# Writes ~/hwenc2-<UTC date>-<repo commit>.log
set -euo pipefail
REPO=${1:-$HOME/pubrepo}; CORE=${2:-2}; G=${GOLDEN:-$HOME/golden}; HERE=$(cd "$(dirname "$0")" && pwd)
C=$(git -C "$REPO" rev-parse --short HEAD); OUT=$HOME/hwenc2-$(date -u +%F)-$C.log
# gates before anything is measured: all three corpora present, machine idle (load1 < 0.5)
for F in "$G/mixed/silesia.tar" "$G/text/enwik8" "$G/genome/chr1.fa"; do [ -f "$F" ] || { echo "STOP: $F missing"; exit 2; }; done
L1=$(cut -d' ' -f1 /proc/loadavg); awk -v l="$L1" 'BEGIN{exit !(l < 0.5)}' || { echo "STOP: load1 $L1 >= 0.5, run later"; exit 3; }
B=$(mktemp -d); trap 'rm -rf "$B"' EXIT
g++ -O3 -std=c++17 -march=native -I"$REPO/src" "$HERE/enc2.cpp" -o "$B/enc2"
VARIANTS=(l11 slot "w32h17:hb=17,base=3,pi=2" "w43h17:hb=17,base=4,pi=3" "w52h17:hb=17,base=5,pi=2" "w43h16:hb=16,base=4,pi=3" "w53h16:hb=16,base=5,pi=3")
{
echo "# HW-ENC2 $(date -u '+%F %H:%M') UTC $(hostname) repo $C; $(lscpu | sed -n 's/Model name: *//p' | head -1); core $CORE; load $(cut -d' ' -f1-3 /proc/loadavg)"
echo "# enc2 sha256 $(sha256sum "$HERE/enc2.cpp" | cut -c1-16); g++ $(g++ -dumpfullversion); timing: 7 encodes (median and best), decode: best of 7 per archive, median over the 7 archives, single thread, taskset -c $CORE"
echo "## selftest"
taskset -c "$CORE" "$B/enc2" --selftest "${VARIANTS[@]:2}"
for F in "$G/mixed/silesia.tar" "$G/text/enwik8" "$G/genome/chr1.fa"; do
  echo "## $F $(stat -L -c %s "$F") bytes sha256 $(sha256sum "$F" | cut -c1-16)"
  DECR=7 taskset -c "$CORE" "$B/enc2" "$F" 7 "${VARIANTS[@]}"
  if [ -n "${LZB:-}" ] && [ -x "$LZB" ]; then
    echo "# lzbench reference ($LZB): zxc 1,2 and aceapex_fast 1, -t8,8 -o1c4"
    taskset -c "$CORE" "$LZB" -ezxc,1,2/aceapex_fast,1 -t8,8 -o1c4 "$F" | tail -n +2
  fi
done
echo "# load after $(cut -d' ' -f1-3 /proc/loadavg)"
} 2>&1 | tee "$OUT"
echo "written $OUT"
