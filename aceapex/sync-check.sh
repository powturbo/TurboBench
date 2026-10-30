#!/usr/bin/env bash
# sync-check.sh — расхождения между копиями исходника и собранными бинарями.
# Трижды за одну сессию вывод делался из кода, который не был собран или лежал
# в другой копии: дефолт правился в одном файле и измерялся из другого, бинарь
# оказывался старше исходника, переменная окружения переживала свой замер.
set -uo pipefail
FAIL=0

echo "--- копии исходника ---"
for pair in "aceapex_depth.cpp:$HOME/aceapex/aceapex_depth.cpp" \
            "src/aceapex_main.cpp:$HOME/lzbench/bench/aceapex_main.cpp"; do
  a="${pair%%:*}"; b="${pair##*:}"
  if [ ! -f "$b" ]; then echo "  нет $b (пропуск)"; continue; fi
  if cmp -s "$a" "$b"; then echo "  OK   $a = $b"
  else echo "  РАСХОЖДЕНИЕ $a != $b"; FAIL=1; fi
done

# Два наших источника расходились по существу: guard на origin был в
# aceapex_depth.cpp и отсутствовал в src/aceapex_main.cpp, который собирает make.
# Нашли снаружи под ASan 10.09. Полное совпадение не требуется — файлы разные
# по назначению; сверяем ключевые границы.
echo "--- границы массивов в обоих источниках ---"
for PAT in "local_pos < 1048576" "local_pos < ORIGIN_CAP"; do :; done
# Маркеры, которые обязаны быть в ОБОИХ источниках. Список растёт по мере
# находок: guard на origin (09.09, дал SIGSEGV), дефолт чанкования (11.09,
# стоил 17% ratio). Оба дефекта — правка в одном источнике и не в другом.
for M in "c_off <= local_pos && local_pos <" "g_input_is_dna" "min_match_len" "epoch\[h\]==ht->cur_epoch"; do
  A=$(grep -c "$M" aceapex_depth.cpp 2>/dev/null || echo 0)
  B=$(grep -c "$M" src/aceapex_main.cpp 2>/dev/null || echo 0)
  if [ "$A" -ge 1 ] && [ "$B" -ge 1 ]; then
    echo "  OK   \"$M\" в обоих"
  else
    echo "  РАСХОЖДЕНИЕ \"$M\": depth=$A, main=$B"
    RC=1
  fi
done

echo "--- бинарь новее исходника? ---"
for pair in "$HOME/aceapex/aceapex_region:aceapex_depth.cpp" \
            "$HOME/aceapex/aceapex_fai:aceapex_depth.cpp" \
            "$HOME/lzbench/lzbench:src/aceapex_main.cpp"; do
  bin="${pair%%:*}"; src="${pair##*:}"
  if [ ! -f "$bin" ]; then echo "  нет $bin (пропуск)"; continue; fi
  if [ "$bin" -nt "$src" ]; then echo "  OK   $(basename $bin) собран после правки"
  else echo "  УСТАРЕЛ $(basename $bin): $src новее — пересоберите"; FAIL=1; fi
done

echo "--- переменные окружения, влияющие на замеры ---"
for v in ACEAPEX_DUMP LIT_CHUNK MIN_MATCH NO_REP FORCED_BIN; do
  if [ -n "${!v:-}" ]; then echo "  ЗАДАНА $v=${!v} — числа будут не эталонными"; FAIL=1; fi
done
[ "${ACEAPEX_BS:-}" ] && echo "  ACEAPEX_BS=$ACEAPEX_BS (это нормально, если так задумано)"

echo ""
[ "$FAIL" -eq 0 ] && echo "СИНХРОННО" || echo "ЕСТЬ РАСХОЖДЕНИЯ — см. выше"
exit $FAIL
