#!/usr/bin/env python3
"""E1/E2: what an entropy profile other than zstd would cost on the token streams.

For the offset, length and command streams of an .aet (FSE layout, spec §3.1) this reads
each stored chunk, decodes it with zstd and compares the stored size against
  H0   order-0 static model per chunk (+ a 256-entry table, 12 bits per used symbol):
       what a GPU rANS/ANS profile (DietGPU, nvcomp gANS) would pay,
  H1   order-1 static model per chunk (+ table cost), the ceiling for a context ANS,
  zlib zlib level 9 on the same chunk: a lower bound for a Deflate profile
       (libdeflate -12 is a few % tighter) - readable by the Blackwell DE and nvcomp 2.2.
Literals are not measured: order-0 on literals was closed 09.2026 (H0 3.07 vs 2.86), and on
DNA archives the literal stream is the 2-bit pack, not zstd.

usage: e1_entropy_profile.py archive.aet
"""
import math, struct, sys, zlib
from collections import Counter
import zstandard

def h_order0(b):
    c = Counter(b); n = len(b)
    bits = sum(-k * math.log2(k / n) for k in c.values())
    return bits / 8 + 12 * len(c) / 8

def h_order1(b):
    if len(b) < 2: return len(b)
    ctx = {}
    for i in range(1, len(b)):
        ctx.setdefault(b[i - 1], Counter())[b[i]] += 1
    bits = 0.0; table = 0
    for cnt in ctx.values():
        n = sum(cnt.values()); table += len(cnt)
        bits += sum(-k * math.log2(k / n) for k in cnt.values())
    return bits / 8 + 12 * table / 8 + 1

def main(path):
    a = open(path, 'rb').read()
    num_blocks = struct.unpack_from('<I', a, 24)[0]     # header §1
    zlit, zoff, zlen, zcmd = struct.unpack_from('<QQQQ', a, 36)
    base = 68 + 64 * num_blocks
    starts = [(base, zlit), (base + zlit, zoff), (base + zlit + zoff, zlen), (base + zlit + zoff + zlen, zcmd)]
    d = zstandard.ZstdDecompressor()
    print(f'{"stream":6} {"chunks":>6} {"raw":>9} {"zstd":>8} {"H0":>8} {"H1":>8} {"zlib9":>8}   zstd->H0  zstd->H1  zstd->zlib')
    tot = [0] * 5
    for name, (s0, sz) in zip(['lit', 'off', 'len', 'cmd'], starts):
        if name == 'lit' or sz < 8: continue
        w = struct.unpack_from('<Q', a, s0)[0]
        S = w & ((1 << 48) - 1); chunk = ((w >> 48) & 0x7FFF) * 4096
        if chunk == 0: chunk = 524288
        nc = (S + chunk - 1) // chunk
        if nc == 0: continue
        cs = struct.unpack_from('<%dQ' % nc, a, s0 + 8)
        p = s0 + 8 + 8 * nc
        raw = zs = h0 = h1 = zl = 0.0
        for i, c in enumerate(cs):
            stored = c & ~(1 << 63); want = min(chunk, S - i * chunk)
            blob = a[p:p + stored]; p += stored
            data = blob if c >> 63 else d.decompress(blob, max_output_size=want)
            assert len(data) == want
            raw += want; zs += stored; h0 += h_order0(data); h1 += h_order1(data)
            zl += len(zlib.compress(data, 9))
        print(f'{name:6} {nc:6d} {raw:9.0f} {zs:8.0f} {h0:8.0f} {h1:8.0f} {zl:8.0f}   {h0/zs-1:+8.1%}  {h1/zs-1:+8.1%}  {zl/zs-1:+8.1%}')
        for k, v in enumerate((raw, zs, h0, h1, zl)): tot[k] += v
    raw, zs, h0, h1, zl = tot
    print(f'{"all":6} {"":6} {raw:9.0f} {zs:8.0f} {h0:8.0f} {h1:8.0f} {zl:8.0f}   {h0/zs-1:+8.1%}  {h1/zs-1:+8.1%}  {zl/zs-1:+8.1%}')
    print(f'archive {len(a)} B; token streams = {zs/len(a):.1%} of the archive')

if __name__ == '__main__':
    main(sys.argv[1])
