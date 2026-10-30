#!/usr/bin/env bash
# faidx_test.sh - claim head_faidx: `aceapex faidx` prints what samtools faidx prints, byte for byte, for 1000 regions
# (random name:start-end, whole records, name:start, ends past the record, starts past it, commas, an unknown name,
# start > end) on a FASTA of 6 records (line widths 50/60/70/80, lowercase runs, N runs, one record shorter than a
# line), the default and the open profile, one region per call and all of them through -r; exit codes compared too.
# samtools runs with -n 60: its default line length until 1.16 and the width aceapex faidx writes; samtools 1.17
# (PR #1738) made the default the input line length, so without -n 1.17+ wraps at 50/70/80 and the claim went red on
# ubuntu-24.04 (samtools 1.19.2). Needs samtools (skipped without it). Usage: bash scripts/faidx_test.sh [aceapex binary]
set -u
A=${1:-./aceapex}; T=$(mktemp -d); trap 'rm -rf $T' EXIT
command -v samtools >/dev/null || { printf 'head_faidx\tskip\tno samtools\n'; exit 0; }
python3 - "$T" <<'PY'
import random, sys
T = sys.argv[1]; random.seed(3); B = 'ACGT'
recs = [('chrA', 50, 300000), ('chrB extra words', 60, 123457), ('chrC', 70, 2000001), ('chrD', 80, 81), ('chrE', 60, 37), ('chrF', 60, 900000)]
with open(f'{T}/x.fa', 'w') as f:
    for nm, w, n in recs:
        s = []
        while sum(map(len, s)) < n:
            r = random.random(); L = random.randint(50, 3000)
            s.append(('N' * L) if r < 0.05 else ''.join(random.choice(B) for _ in range(L)).lower() if r < 0.4 else ''.join(random.choice(B) for _ in range(L)))
        s = ''.join(s)[:n]
        f.write('>' + nm + '\n' + ''.join(s[i:i + w] + '\n' for i in range(0, n, w)))
names = [(r[0].split()[0], r[2]) for r in recs]
with open(f'{T}/regions.txt', 'w') as f:
    for i in range(1000):
        nm, n = random.choice(names); k = i % 10
        if k == 0: f.write(nm + '\n')
        elif k == 1: f.write(f'{nm}:{random.randint(1, n)}\n')
        elif k == 2: a = random.randint(1, n); f.write(f'{nm}:{a}-{a + random.randint(0, 3 * n)}\n')
        elif k == 3: f.write(f'{nm}:{n + random.randint(1, 100)}-{n + 200}\n')
        elif k == 4: a = random.randint(1, n); f.write(f'{nm}:{a:,}-{min(n, a + 5000):,}\n')
        elif k == 5 and i % 100 == 5: f.write('chrZ:1-10\n')
        elif k == 6 and i % 100 == 6: a = random.randint(10, n); f.write(f'{nm}:{a}-{a - 5}\n')
        else: a = random.randint(1, n); f.write(f'{nm}:{a}-{min(n, a + random.randint(0, 20000))}\n')
PY
samtools faidx $T/x.fa
bad=0; nreg=$(wc -l < $T/regions.txt)
for P in default open; do
  E=""; [ $P = open ] && E="ACEAPEX_BS=16384 LIT_CHUNK=65536 AX_PROFILE=open"
  env $E $A c --in $T/x.fa --out $T/x.$P.aet >/dev/null 2>&1 || { bad=$((bad+1)); continue; }
  $A faidx $T/x.$P.aet || bad=$((bad+1))
  cmp -s $T/x.fa.fai $T/x.$P.aet.fai || { echo "fai differs ($P)" >&2; bad=$((bad+1)); }
  while IFS= read -r r; do
    $A faidx $T/x.$P.aet "$r" > $T/a.out 2>/dev/null; ra=$?; samtools faidx -n 60 $T/x.fa "$r" > $T/s.out 2>/dev/null; rs=$?
    { [ $ra = $rs ] && cmp -s $T/a.out $T/s.out; } || { bad=$((bad+1)); [ $bad -le 3 ] && echo "differs ($P): $r (exit $ra vs $rs)" >&2; }
  done < $T/regions.txt
  grep -v 'chrZ' $T/regions.txt | awk -F'[:-]' 'NF<3 || $2+0<=$3+0' > $T/ok.txt
  $A faidx $T/x.$P.aet -r $T/ok.txt > $T/a.all 2>/dev/null; samtools faidx -n 60 $T/x.fa -r $T/ok.txt > $T/s.all 2>/dev/null
  cmp -s $T/a.all $T/s.all || { echo "-r output differs ($P)" >&2; bad=$((bad+1)); }
done
[ $bad = 0 ] && printf 'head_faidx\tpass\taceapex faidx == samtools faidx -n 60 (%s) byte for byte and exit code on %s regions (whole, open end, past the end, commas, unknown name, start > end), default and open profile, and through -r; .fai identical\n' "$(samtools --version | head -n 1)" "$nreg" \
  || printf 'head_faidx\tfail\t%s differences against samtools faidx\n' "$bad"
