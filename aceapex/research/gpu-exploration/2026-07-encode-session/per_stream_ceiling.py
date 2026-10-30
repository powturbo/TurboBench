#!/usr/bin/env python3
# per_stream_ceiling.py — измеряет ПОТОЛОК per-stream backend selection на
# РЕАЛЬНЫХ 4 потоках из streams.bin (не синтетика). Отвечает: стоит ли строить
# per-stream энкодер вообще, и какой backend оптимален на каждый поток.
#
# Backend-кандидаты на поток:
#   raw       — как есть (varint уже применён CPU-энкодером)
#   entropy   — zlib-9 как прокси ANS/энтропийного слоя
#   split     — byte-plane hi/lo раздельно + entropy (для многобайтовых)
#   rle       — run-length + entropy (для CMD с повторами)
# Текущий ACEAPEX: единый ANS на все (что статья 3 показала субоптимальным).
import sys, struct, zlib
from collections import Counter
import math

def read_streams(path):
    with open(path,'rb') as f:
        data=f.read()
    # AetHdr: 8s magic, I version, Q orig, I bs, I nb, 8s xxh, 4Q z-sizes
    hdr=struct.unpack_from('<8sIQII8sQQQQ', data, 0)
    off=struct.calcsize('<8sIQII8sQQQQ')
    nb=hdr[4]
    # BlockOffsets: 8Q = 64 байта
    bosz=struct.calcsize('<8Q')
    bos=[struct.unpack_from('<8Q',data,off+i*bosz) for i in range(nb)]
    off+=nb*bosz
    totL=sum(b[4] for b in bos); totO=sum(b[5] for b in bos)
    totN=sum(b[6] for b in bos); totC=sum(b[7] for b in bos)
    LIT=data[off:off+totL]; off+=totL
    OFF=data[off:off+totO]; off+=totO
    LEN=data[off:off+totN]; off+=totN
    CMD=data[off:off+totC]; off+=totC
    return {'LIT':LIT,'OFF':OFF,'LEN':LEN,'CMD':CMD}

def entropy_size(b):
    if not b: return 0
    c=Counter(b); n=len(b)
    H=-sum((v/n)*math.log2(v/n) for v in c.values())
    return H*n/8

def bk_raw(b): return len(b)
def bk_entropy(b): return len(zlib.compress(b,9)) if b else 0
def bk_split(b):
    if len(b)<2: return len(b)
    hi=b[0::2]; lo=b[1::2]
    return bk_entropy(hi)+bk_entropy(lo)
def bk_rle(b):
    if not b: return 0
    out=bytearray(); i=0
    while i<len(b):
        run=1
        while i+run<len(b) and b[i+run]==b[i] and run<255: run+=1
        out.append(run); out.append(b[i]); i+=run
    return bk_entropy(bytes(out))

def main():
    path=sys.argv[1] if len(sys.argv)>1 else 'streams.bin'
    S=read_streams(path)
    backends={'raw':bk_raw,'entropy':bk_entropy,'split':bk_split,'rle':bk_rle}
    print(f"{'stream':6} {'size_MB':>8} | " + " ".join(f"{n:>9}" for n in backends) + " | BEST")
    print("-"*70)
    cur_total=0; opt_total=0; raw_total=0
    for name in ['LIT','OFF','LEN','CMD']:
        b=S[name]; raw_total+=len(b)
        res={n:fn(b) for n,fn in backends.items()}
        best=min(res,key=res.get)
        cur_total+=res['entropy']   # текущий ACEAPEX = единый entropy на все
        opt_total+=res[best]        # per-stream оптимум
        row=" ".join(f"{res[n]/1e6:>9.2f}" for n in backends)
        print(f"{name:6} {len(b)/1e6:>8.1f} | {row} | {best} ({len(b)/res[best]:.2f}x)")
    print("-"*70)
    print(f"Текущий (единый entropy на все):  {cur_total/1e6:.1f} MB  ratio={raw_total/cur_total:.3f}x")
    print(f"Per-stream оптимум:               {opt_total/1e6:.1f} MB  ratio={raw_total/opt_total:.3f}x")
    print(f"ВЫИГРЫШ per-stream backend:        {cur_total/opt_total:.3f}x плотнее")
    print()
    gain=(cur_total-opt_total)/cur_total*100
    if gain>5:
        print(f"=> {gain:.1f}% выигрыш — СТОИТ строить per-stream энкодер.")
    else:
        print(f"=> только {gain:.1f}% — малый выигрыш, приоритет низкий.")

if __name__=='__main__': main()
