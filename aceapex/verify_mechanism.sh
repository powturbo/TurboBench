#!/bin/bash
cd /workspace/aceapex
DEPTH=/workspace/aceapex/aceapex_depth
V3=/workspace/aceapex/fgd_v3
FQ=/workspace/data/NA12878_1gb.fastq
FAIL=0
echo "=== 1: override реально в бинаре ==="
if strings $DEPTH | grep -q ACEAPEX_BS; then echo "  OK"; else echo "  FAIL"; FAIL=1; fi
echo "=== 2: block_size меняет размер ==="
export ACEAPEX_BS=1048576; $DEPTH c --in $FQ --out /tmp/v1.aet --threads 8 2>/dev/null
export ACEAPEX_BS=65536;   $DEPTH c --in $FQ --out /tmp/v2.aet --threads 8 2>/dev/null
s1=$(stat -c%s /tmp/v1.aet); s2=$(stat -c%s /tmp/v2.aet)
if [ "$s1" != "$s2" ]; then echo "  OK: 1MB=$s1 != 64KB=$s2"; else echo "  FAIL"; FAIL=1; fi
unset ACEAPEX_BS
echo "=== 3: число блоков ожидаемое ==="
export ACEAPEX_BS=65536; $DEPTH c --in $FQ --out /tmp/v3.aet --threads 8 2>/dev/null
$DEPTH d --in /tmp/v3.aet --out /tmp/v3_dec 2>/dev/null
res=$($V3 streams.bin $FQ 2>&1)
blk=$(echo "$res" | grep -oE 'blocks=[0-9]+' | grep -oE '[0-9]+')
orig=$(stat -c%s $FQ); expect=$(( (orig + 65535) / 65536 ))
if [ "$blk" == "$expect" ]; then echo "  OK: blocks=$blk = $expect"; else echo "  FAIL: $blk != $expect"; FAIL=1; fi
unset ACEAPEX_BS
echo "=== 4: bit-perfect ==="
if echo "$res" | grep -q "MATCHES OK"; then echo "  OK"; else echo "  FAIL"; FAIL=1; fi
echo "=== 5: warm vs cold стабилен ==="
g1=$($V3 streams.bin $FQ 2>&1 | grep -oE '[0-9.]+ GB/s' | head -1)
g2=$($V3 streams.bin $FQ 2>&1 | grep -oE '[0-9.]+ GB/s' | head -1)
echo "  прогон1=$g1 прогон2=$g2"
echo "=== 6: match != full pipeline ==="
echo "  fgd_v3 меряет ТОЛЬКО match-фазу. НЕ заявлять как full."
rm -f /tmp/v1.aet /tmp/v2.aet /tmp/v3.aet /tmp/v3_dec
[ "$FAIL" == "0" ] && echo "=== ВСЕ ПРОЙДЕНЫ ===" || echo "=== ЕСТЬ FAIL ==="
