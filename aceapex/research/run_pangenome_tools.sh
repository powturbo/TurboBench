#!/usr/bin/env bash
# run_pangenome_tools.sh - research 1 (02.10): AGC and MBGC (latest releases, built from source) on the same 1 / 2 / 4
# HPRC assemblies as I3 (HG00438.1/.2, HG00621.1/.2, sha256 checked). Each assembly unpacked to its own FASTA (both tools
# take one file per sample), md5 of every input; compress; the FASTA deleted (disk); full decompression to md5 (AGC
# getset per sample to stdout, MBGC d to a directory, deleted after); regions of 5000 bases by contig name (AGC getctg,
# one process per region, -p: no prefetch), checked against bases taken from the input before it was deleted.
# Usage: bash research/run_pangenome_tools.sh <dir with the .fa.gz> <agc binary> <mbgc binary> [regions=200]
set -euo pipefail
D=$1; AGC=$2; MB=$3; NR=${4:-200}; T=16
ASM=(HG00438.1 HG00438.2 HG00621.1 HG00621.2)
tm(){ local t0; t0=$(date +%s.%N); "$@"; awk -v a=$t0 -v b=$(date +%s.%N) 'BEGIN{printf "%.1f", b-a}' > $D/.t; }
for N in ${NS:-1 2 4}; do
  echo "== N=$N"
  : > $D/list.txt; IN=0
  for ((i=0;i<N;i++)); do f=$D/${ASM[$i]}.fa; zcat $D/${ASM[$i]}.fa.gz > $f; echo $f >> $D/list.txt; IN=$((IN + $(stat -c%s $f)))
    md5sum < $f | cut -c1-32 > $D/${ASM[$i]}.md5; done
  # regions: random contigs >= 6000 b of the inputs, 0-based from-to inclusive, the expected bases saved
  python3 - $D $N $NR <<'PY'
import sys, random
D, N, NR = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]); ASM = ['HG00438.1', 'HG00438.2', 'HG00621.1', 'HG00621.2'][:N]
random.seed(20261002); recs = []
for a in ASM:
    name = None; seq = []
    for line in open(f'{D}/{a}.fa'):
        if line[0] == '>':
            if name and sum(map(len, seq)) >= 6000: recs.append((a, name, ''.join(seq)))
            name = line[1:].split()[0]; seq = []
        else: seq.append(line.strip())
    if name and sum(map(len, seq)) >= 6000: recs.append((a, name, ''.join(seq)))
with open(f'{D}/regions.txt', 'w') as f:
    for _ in range(NR):
        a, name, s = random.choice(recs); st = random.randrange(0, len(s) - 5000)
        f.write(f'{a}\t{name}\t{st}\t{st + 4999}\t{s[st:st + 5000]}\n')
PY
  tm $AGC create -t $T -o $D/x.agc $(cat $D/list.txt) 2>/dev/null; AC=$(cat $D/.t)
  tm $MB c -t $T $D/list.txt $D/x.mbgc >/dev/null 2>&1; MC=$(cat $D/.t)
  rm -f $(cat $D/list.txt)
  ok=1; t0=$(date +%s.%N)
  for ((i=0;i<N;i++)); do [ "$($AGC getset -t $T $D/x.agc ${ASM[$i]} 2>/dev/null | md5sum | cut -c1-32)" = "$(cat $D/${ASM[$i]}.md5)" ] || ok=0; done
  AD=$(awk -v a=$t0 -v b=$(date +%s.%N) 'BEGIN{printf "%.1f", b-a}')
  python3 - $D $AGC <<'PY' > $D/agc_reg.txt
import sys, subprocess, time
D, AGC = sys.argv[1], sys.argv[2]; t = []; bad = 0
for line in open(f'{D}/regions.txt'):
    a, name, s, e, want = line.rstrip('\n').split('\t')
    t0 = time.perf_counter(); out = subprocess.run([AGC, 'getctg', '-p', '-t', '1', f'{D}/x.agc', f'{name}@{a}:{s}-{e}'], capture_output=True).stdout; t.append(time.perf_counter() - t0)
    got = ''.join(out.decode().split('\n')[1:]); bad += got != want
t.sort(); print('%.0f %.0f %d %d' % (t[len(t) // 2] * 1e6, t[int(len(t) * 0.99)] * 1e6, bad, len(t)))
PY
  mkdir -p $D/mbout; t0=$(date +%s.%N); $MB d -t $T $D/x.mbgc $D/mbout >/dev/null 2>&1; MD=$(awk -v a=$t0 -v b=$(date +%s.%N) 'BEGIN{printf "%.1f", b-a}')
  mok=1; for ((i=0;i<N;i++)); do f=$(find $D/mbout -name "${ASM[$i]}.fa" | head -n 1); [ -n "$f" ] && [ "$(md5sum < $f | cut -c1-32)" = "$(cat $D/${ASM[$i]}.md5)" ] || mok=0; done
  rm -rf $D/mbout
  echo "AGC N=$N: input $IN B -> $(stat -c%s $D/x.agc) B, create $AC s, getset all $AD s, regions p50/p99 us, bad, n: $(cat $D/agc_reg.txt), bit-perfect $ok"
  echo "MBGC N=$N: input $IN B -> $(stat -c%s $D/x.mbgc) B, compress $MC s, decompress all $MD s, regions -, bit-perfect $mok"
  rm -f $D/x.agc $D/x.mbgc
done
