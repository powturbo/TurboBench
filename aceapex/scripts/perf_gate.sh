#!/usr/bin/env bash
# perf_gate.sh - performance gate of the CPU decoder on ace-core. Every row of results/baseline_ace-core.tsv (archive,
# profile, threads) is decoded RUNS times (median after a warm-up; every run compared with the original); a row more
# than TOL % (default 5) slower than its baseline fails the gate. The archives are encoded once into $PERF_DIR
# (default .wk/perf) with the row's profile and must have the baseline's size (the same bytes: zstd frames depend on
# the libzstd version), else the row is SKIPPED. On a host other than the baseline's CPU the gate is skipped.
#   scripts/perf_gate.sh            check; exit 1 on a regression
#   scripts/perf_gate.sh --update   the same; when nothing regressed, rows more than TOL % faster are written into the
#                                   baseline (only those) - commit the baseline with its log, on its own
# Log: results/perf-gate-<date>-<commit>.log
set -euo pipefail
cd "$(dirname "$0")/.."
B=results/baseline_ace-core.tsv; P=${PERF_DIR:-.wk/perf}; RUNS=${RUNS:-5}; TOL=${TOL:-5}; UPDATE=0
[ "${1:-}" = --update ] && UPDATE=1
mkdir -p "$P"
CPU=$(lscpu | sed -n 's/^Model name: *//p' | head -n 1)
BCPU=$(sed -n 's/^# cpu: //p' "$B")
C=$(git rev-parse --short HEAD); LOG=results/perf-gate-$(date -u +%Y-%m-%d)-$C.log
if [ "$CPU" != "$BCPU" ]; then echo "perf gate: host '$CPU' is not the baseline host '$BCPU' - skipped" | tee "$LOG"; exit 0; fi
g++ -std=c++17 -O3 -march=native -Isrc -o "$P/perf_bench" scripts/perf_bench.cpp src/aceapex_api.cpp -lzstd -lpthread
make -s >/dev/null
{ echo "# perf gate $(date -u +%FT%TZ), commit $C, $CPU, median of $RUNS, tolerance $TOL %"
  printf 'archive\tprofile\tthreads\tbase_s\tnow_s\tchange\tGB/s\tverdict\n'; } > "$LOG"
NEW=$P/baseline.new; FAILS=$P/fails; : > "$NEW"; : > "$FAILS"
row(){ printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$@" >> "$NEW"; }
while IFS=$'\t' read -r name prof thr base bytes orig gbs commit date; do
  case "$name" in '#'*) continue;; archive) row "$name" "$prof" "$thr" "$base" "$bytes" "$orig" "$gbs" "$commit" "$date"; continue;; esac
  case $prof in default) E="";; open) E="AX_PROFILE=open ACEAPEX_BS=16384 LIT_CHUNK=65536";; *) echo "unknown profile $prof"; exit 2;; esac
  A=$P/$name.$prof.aet; O=${orig/#\~/$HOME}
  [ -s "$A" ] || env -i PATH="$PATH" $E ./aceapex c --in "$O" --out "$A" --threads "$(nproc)" >/dev/null 2>&1
  S=$(stat -c%s "$A")
  if [ "$S" != "$bytes" ]; then
    printf '%s\t%s\t%s\t%s\t-\t-\t-\tSKIPPED (archive %s B, baseline %s B)\n' "$name" "$prof" "$thr" "$base" "$S" "$bytes" | tee -a "$LOG"
    row "$name" "$prof" "$thr" "$base" "$bytes" "$orig" "$gbs" "$commit" "$date"; continue
  fi
  PIN=""; [ "$thr" = 1 ] && PIN="taskset -c 2"
  if ! r=$($PIN "$P/perf_bench" "$A" "$O" "$thr" "$RUNS"); then
    printf '%s\t%s\t%s\t%s\t-\t-\t-\tMISMATCH\n' "$name" "$prof" "$thr" "$base" | tee -a "$LOG"; echo x >> "$FAILS"
    row "$name" "$prof" "$thr" "$base" "$bytes" "$orig" "$gbs" "$commit" "$date"; continue
  fi
  now=$(echo "$r" | cut -d" " -f1); out=$(echo "$r" | cut -d" " -f3)
  read -r ch v g < <(awk -v b="$base" -v n="$now" -v t="$TOL" -v o="$out" 'BEGIN{c=100*(n/b-1); printf "%+.1f%% %s %.2f\n", c, (c>t?"FAIL":(c< -t?"FASTER":"OK")), o/n/1e9}')
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$name" "$prof" "$thr" "$base" "$now" "$ch" "$g" "$v" | tee -a "$LOG"
  [ "$v" = FAIL ] && echo x >> "$FAILS"
  if [ "$v" = FASTER ]; then row "$name" "$prof" "$thr" "$now" "$bytes" "$orig" "$g" "$C" "$(date -u +%Y-%m-%d)"
  else row "$name" "$prof" "$thr" "$base" "$bytes" "$orig" "$gbs" "$commit" "$date"; fi
done < "$B"
if [ -s "$FAILS" ]; then
  echo "perf gate: FAIL - $(wc -l < "$FAILS") rows slower than the baseline by more than $TOL % or with wrong output; $LOG" | tee -a "$LOG"; exit 1
fi
echo "perf gate: PASS; $LOG" | tee -a "$LOG"
if [ $UPDATE = 1 ]; then
  { grep '^#' "$B"; cat "$NEW"; } > "$B.tmp" && mv "$B.tmp" "$B"
  echo "baseline: rows marked FASTER written into $B (commit it with $LOG)" | tee -a "$LOG"
fi
