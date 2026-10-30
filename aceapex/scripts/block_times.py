#!/usr/bin/env python3
# block_times.py - the AX_BLOCK_TIMES dump of the CPU tile path (src/aceapex_api.cpp ax_decode_tiled): per group its
# thread, start / end (ns from the call), literal chunk time; per block its decode time and literal / command bytes.
# Prints: block time quantiles and the share of the time in the slowest 1 % / 10 % of blocks, the fit of the block time
# to literal and command bytes, per thread busy time and finish, and the tail = (last finish - first finish) / last
# finish, also against the mean finish. Usage: block_times.py <dump.tsv> [more dumps...]
import sys
for path in sys.argv[1:]:
    G, B = [], []; hdr = ''
    for line in open(path):
        if line.startswith('#'): hdr = line.strip(); continue
        f = line.split('\t')
        if f[0] == 'G': G.append((int(f[2]), float(f[3]), float(f[4]), float(f[5]), int(f[6]), int(f[7])))
        else: B.append((float(f[2]), int(f[3]), int(f[4])))
    bt = sorted(x[0] for x in B); n = len(bt); tot = sum(bt)
    q = lambda p: bt[min(n - 1, int(p * n))]
    top = lambda p: sum(bt[int((1 - p) * n):]) / tot
    # least squares t = a + b*lit + c*cmd
    import itertools
    sx = [[0.0] * 3 for _ in range(3)]; sy = [0.0] * 3
    for t, l, c in B:
        v = (1.0, l, c)
        for i in range(3):
            sy[i] += v[i] * t
            for j in range(3): sx[i][j] += v[i] * v[j]
    # solve 3x3
    import copy
    M = copy.deepcopy(sx); y = sy[:]
    for i in range(3):
        p = max(range(i, 3), key=lambda r: abs(M[r][i])); M[i], M[p] = M[p], M[i]; y[i], y[p] = y[p], y[i]
        for r in range(3):
            if r != i and M[i][i]:
                f = M[r][i] / M[i][i]
                for k in range(3): M[r][k] -= f * M[i][k]
                y[r] -= f * y[i]
    co = [y[i] / M[i][i] if M[i][i] else 0 for i in range(3)]
    mt = sum(t for t, _, _ in B) / n; ss = sum((t - mt) ** 2 for t, _, _ in B); sr = sum((t - (co[0] + co[1] * l + co[2] * c)) ** 2 for t, l, c in B)
    th = {}
    for t, s, e, lit, b0, b1 in G:
        a = th.setdefault(t, [1e30, 0.0, 0.0]); a[0] = min(a[0], s); a[1] = max(a[1], e); a[2] += e - s
    ends = sorted(a[1] for a in th.values()); last = ends[-1]; first = ends[0]; mean = sum(ends) / len(ends)
    busy = sum(a[2] for a in th.values())
    print('%s  [%s]' % (path.split('/')[-1], hdr))
    print('  blocks %d: block decode time median %.1f us, p90 %.1f, p99 %.1f, max %.1f us; slowest 1 %% of blocks %.1f %% of the time, 10 %%: %.1f %%' %
          (n, q(0.5) / 1e3, q(0.9) / 1e3, q(0.99) / 1e3, bt[-1] / 1e3, 100 * top(0.01), 100 * top(0.10)))
    print('  fit: t = %.2f us + %.3f ns x literal byte + %.2f ns x command byte  (R^2 %.2f)' % (co[0] / 1e3, co[1], co[2], 1 - sr / ss if ss else 0))
    print('  threads %d: finish first %.2f ms, mean %.2f, last %.2f ms; tail (last - first) / last %.1f %%, (last - mean) / last %.1f %%; busy %.1f %% of threads x last' %
          (len(th), first / 1e6, mean / 1e6, last / 1e6, 100 * (last - first) / last, 100 * (last - mean) / last, 100 * busy / (len(th) * last)))
