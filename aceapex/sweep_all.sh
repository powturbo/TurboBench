#!/usr/bin/env bash
# Полный свип параметров ACEAPEX: каждая точка — строка JSON в sweep.jsonl.
# Повторный запуск пропускает сделанное. Прерывание не теряет ничего.
#
# Оси: корпус × BS × LIT_CHUNK × FSE_CHUNK × HASH_LOG
# Выходы: ratio, encode MB/s, decode MB/s, seek p50/p99, batch по 5 профилям с H_alpha.
#
# Запуск:  ./sweep_all.sh            (всё)
#          ./sweep_all.sh chr1       (один корпус)
# Смотреть: tail -f sweep.jsonl | jq -c .
set -uo pipefail
cd "$(dirname "$0")"
OUT=sweep.jsonl; touch "$OUT"
ZI=${ZSTD_INC:+-I$ZSTD_INC}
BIN=/tmp/sw_bin; SEEK=/tmp/sw_seek; BATCH=/tmp/sw_batch

g++ -O3 -march=native -funroll-loops -std=c++17 -Isrc $ZI -o $BIN aceapex_depth.cpp -lpthread -lzstd || exit 1
gcc -O2 -Isrc $ZI -o $SEEK scripts/libseek.c src/aceapex_api.cpp -lstdc++ -lpthread -lzstd || exit 1
gcc -O2 -Isrc $ZI -o $BATCH scripts/batch_test_ci.c src/aceapex_api.cpp -lstdc++ -lpthread -lzstd -lm || exit 1

declare -A CORPUS=(
  [chr1]=$HOME/golden/genome/chr1.fa
  [fastq]=$HOME/golden/genome/ERR194147_1gb.fastq
  [enwik8]=$HOME/golden/text/enwik8
  [silesia]=$HOME/golden/mixed/silesia.tar
)
[ $# -ge 1 ] && CORPORA="$*" || CORPORA="${!CORPUS[@]}"

BS_LIST="16384 65536 262144"
LIT_LIST="65536 262144 1048576"
FSE_LIST="4096 32768 131072"
HL_LIST="13 15 17"

done_key(){ grep -qF "\"key\":\"$1\"" "$OUT"; }

for C in $CORPORA; do
  F=${CORPUS[$C]}; [ -f "$F" ] || { echo "нет $F"; continue; }
  SZ=$(stat -c%s "$F")
  # входной срез 256 MB, чтобы точки были сравнимы по времени
  head -c 268435456 "$F" > /tmp/sw_in.bin 2>/dev/null; SZ=$(stat -c%s /tmp/sw_in.bin)
  for BS in $BS_LIST; do for LIT in $LIT_LIST; do for FSE in $FSE_LIST; do for HL in $HL_LIST; do
    KEY="$C/$BS/$LIT/$FSE/$HL"
    done_key "$KEY" && continue
    export ACEAPEX_BS=$BS LIT_CHUNK=$LIT FSE_CHUNK=$FSE HASH_LOG=$HL MIN_MATCH=0
    T0=$(date +%s.%N)
    $BIN c --in /tmp/sw_in.bin --out /tmp/sw.aet --threads 8 >/dev/null 2>&1 || { echo "{\"key\":\"$KEY\",\"error\":\"encode\"}" >> "$OUT"; continue; }
    T1=$(date +%s.%N)
    ASZ=$(stat -c%s /tmp/sw.aet)
    $BIN d --in /tmp/sw.aet --out /tmp/sw.out --threads 8 >/dev/null 2>&1
    T2=$(date +%s.%N)
    cmp -s /tmp/sw_in.bin /tmp/sw.out && BP=true || BP=false
    ENC=$(python3 -c "print(round($SZ/($T1-$T0)/1e6,1))")
    DEC=$(python3 -c "print(round($SZ/($T2-$T1)/1e6,1))")
    RATIO=$(python3 -c "print(round($SZ/$ASZ,5))")
    # seek только на chr1: libseek завязан на координаты генома
    P50=null; P99=null
    if [ "$C" = chr1 ]; then
      S=$($SEEK /tmp/sw.aet 2>/dev/null | grep -oE "[0-9]+\.[0-9]+ms" | head -2 | tr -d ms)
      P50=$(echo "$S" | sed -n 1p); P99=$(echo "$S" | sed -n 2p)
      [ -z "$P50" ] && P50=null; [ -z "$P99" ] && P99=null
    fi
    AMP=$(python3 -c "print(round(($LIT+3*$FSE)/$BS,2))")
    # батч по пяти профилям, только chr1 (координаты)
    BATCHJ="[]"
    if [ "$C" = chr1 ]; then
      BATCHJ=$($BATCH /tmp/sw.aet 2>/dev/null | awk '$2==5000 && NF>=10 {printf "%s{\"profile\":\"%s\",\"h_alpha\":%s,\"speedup\":%s,\"batch_per_s\":%s}", (n++?",":""), $1, $3, substr($8,1,length($8)-1), $11} END{print ""}' | sed 's/^/[/; s/$/]/')
      [ -z "$BATCHJ" ] && BATCHJ="[]"
    fi
    echo "{\"key\":\"$KEY\",\"corpus\":\"$C\",\"bs\":$BS,\"lit\":$LIT,\"fse\":$FSE,\"hash_log\":$HL,\"ratio\":$RATIO,\"enc_mbs\":$ENC,\"dec_mbs\":$DEC,\"seek_p50_ms\":$P50,\"seek_p99_ms\":$P99,\"amplification\":$AMP,\"bit_perfect\":$BP,\"batch\":$BATCHJ}" >> "$OUT"
    printf "%-32s ratio %-8s enc %-6s dec %-6s seek %-6s\n" "$KEY" "$RATIO" "$ENC" "$DEC" "$P50"
  done; done; done; done
done
echo "=== готово: $(wc -l < $OUT) точек в $OUT ==="
