#!/usr/bin/env bash
set -uo pipefail
BIN=~/aceapex/aceapex_fai
printf "%8s %8s | %9s | %8s %8s | %9s | %6s\n" "LIT" "FSE" "ratio" "p50 ms" "p99 ms" "full MB/s" "ампл"
for LIT in 65536 131072 262144; do
  for FSE in 8192 32768 131072; do
    ACEAPEX_BS=16384 LIT_CHUNK=$LIT FSE_CHUNK=$FSE $BIN c --in ref.fa --out sw.aet --threads 8 >/dev/null 2>&1
    R=$(ACEAPEX_BS=16384 LIT_CHUNK=$LIT FSE_CHUNK=$FSE $BIN t --in ref.fa --threads 8 2>&1 | grep -oE "Ratio: +[0-9.]+" | grep -oE "[0-9.]+")
    L=$(ACEAPEX_BS=16384 FSE_CHUNK=$FSE ./libseek sw.aet 2>/dev/null | grep -oE "[0-9]+\.[0-9]+ms" | head -2 | tr -d 'ms' | tr '\n' ' ')
    F=$(ACEAPEX_BS=16384 FSE_CHUNK=$FSE $BIN d --in sw.aet --out /dev/null --threads 8 2>&1 | grep -oE "wall: +[0-9.]+" | grep -oE "[0-9.]+")
    A=$(python3 -c "print(f'{($LIT+3*$FSE)/16384:.1f}')")
    printf "%8d %8d | %9s | %17s | %9s | %6s\n" $LIT $FSE "${R:-?}" "${L:-нет}" "${F:-?}" "$A"
  done
done
