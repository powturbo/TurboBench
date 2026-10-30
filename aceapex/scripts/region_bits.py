#!/usr/bin/env python3
# region_bits.py - class tables of results/reality-2026-10-02.log T1 from scripts/region_bits.cpp rows (16 KiB blocks)
# and, optionally, scripts/approx_est.cpp rows (16 KiB and/or 1 MiB blocks of the same FASTA).
# Classes of a 16 KiB block, first that applies: N (>= 50 % N), dense (the 1 MiB around it, 64 blocks, has >= 10 % of
# its bytes in matches: satellite / centromeric arrays and other long tandem stretches), masked (>= 90 % of the bases
# soft-masked, lowercase), unmasked (<= 10 % lowercase), mixed. A 1 MiB approx row goes to the majority class of its
# 64 blocks. Usage: region_bits.py <rows.tsv> [approx_16k.tsv] [approx_1m.tsv]
import csv, sys
from collections import defaultdict
R = list(csv.DictReader(open(sys.argv[1]), delimiter='\t'))
n = len(R); ms = [int(r['match_bytes']) / max(1, int(r['orig'])) for r in R]
pre = [0.0]
for x in ms: pre.append(pre[-1] + x)
def dense(i): lo, hi = max(0, i - 32), min(n, i + 32); return (pre[hi] - pre[lo]) / (hi - lo) >= 0.10
cls = []
for i, r in enumerate(R):
    o, lo, up, N = int(r['orig']), int(r['lower']), int(r['upper']), int(r['N'])
    b = lo + up
    if N >= 0.5 * o: c = 'N'
    elif dense(i): c = 'dense'
    elif b and lo >= 0.9 * b: c = 'masked'
    elif b and lo <= 0.1 * b: c = 'unmasked'
    else: c = 'mixed'
    cls.append(c)
order = ['unmasked', 'mixed', 'masked', 'dense', 'N']
A = defaultdict(lambda: defaultdict(float))
for c, r in zip(cls, R):
    a = A[c]; a['orig'] += int(r['orig']); a['match'] += int(r['match_bytes']); a['blocks'] += 1
    a['z'] += float(r['z_lit']) + float(r['z_off']) + float(r['z_len']) + float(r['z_cmd']) + 64; a['zlit'] += float(r['z_lit']); a['table'] += 64
TO = sum(A[c]['orig'] for c in order); TZ = sum(A[c]['z'] for c in order)
def approx(path, per):                                 # gains by class: k = 1, 2, 4, 8 over k = 0
    G = defaultdict(lambda: defaultdict(float))
    for r in csv.DictReader(open(path), delimiter='\t'):
        b = int(r['block'])
        if per == 1: c = cls[b] if b < n else 'N'
        else:
            cc = defaultdict(int)
            for i in range(b * per, min(n, (b + 1) * per)): cc[cls[i]] += 1
            c = max(cc, key=cc.get) if cc else 'N'
        for k in range(9): G[c][k] += float(r['gain%d' % k])
    return G
G16 = approx(sys.argv[2], 1) if len(sys.argv) > 2 else None
G1M = approx(sys.argv[3], 64) if len(sys.argv) > 3 else None
print('class     blocks  orig MB  orig %%  archive MB  archive %%  bits/byte  match %%' + ('  approx k=4 / k=8 over exact, KB (16 KiB blocks)' if G16 else '') + ('  (1 MiB blocks)' if G1M else ''))
for c in order:
    a = A[c]
    if not a['blocks']: continue
    s = '%-9s %6d  %7.1f  %6.2f  %10.1f  %9.2f  %9.3f  %6.2f' % (c, a['blocks'], a['orig'] / 1e6, 100 * a['orig'] / TO, a['z'] / 1e6, 100 * a['z'] / TZ, 8 * a['z'] / a['orig'], 100 * a['match'] / a['orig'])
    if G16: s += '  %8.0f / %8.0f' % ((G16[c][4] - G16[c][0]) / 8e3, (G16[c][8] - G16[c][0]) / 8e3)
    if G1M: s += '  %8.0f / %8.0f' % ((G1M[c][4] - G1M[c][0]) / 8e3, (G1M[c][8] - G1M[c][0]) / 8e3)
    print(s)
print('total     %6d  %7.1f  100.00  %10.1f     100.00  %9.3f' % (n, TO / 1e6, TZ / 1e6, 8 * TZ / TO))
