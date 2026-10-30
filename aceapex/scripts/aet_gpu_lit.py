#!/usr/bin/env python3
# Literal stream of an .aet archive decoded on the GPU by nvCOMP (RAW zstd) and unpacked in numpy
# (2-bit bases, case mask, exceptions), compared byte for byte with streams.bin (ACEAPEX_DUMP=1).
# Proven 28.09.2026 on H100, chr1 interactive: 11748 frames (2935 DNA chunks x 4 + 8 plain) bit-perfect.
# With scripts/aet_gpu_zstd.py this covers all four streams: the entropy layer is standard zstd.
import sys,struct,time,numpy as np,cupy as cp
from nvidia import nvcomp
a=open(sys.argv[1],'rb').read(); s=open(sys.argv[2],'rb').read()
magic,ver,orig,bs,nb,xx,zl,zo,zn,zc=struct.unpack_from('<8sIQIIQQQQQ',a,0)
z=a[68+64*nb:68+64*nb+zl]
bo=np.frombuffer(s[68:68+64*nb],dtype=np.uint64).reshape(nb,8); totL=int(bo[:,4].sum()); ref=s[68+64*nb:68+64*nb+totL]
h=struct.unpack_from('<Q',z,0)[0]; sz=h&~((1<<62)|(1<<61)|(1<<60)); chunked=bool(h&(1<<61)); tagged=bool(h&(1<<60))
CH=struct.unpack_from('<Q',z,8)[0]; NW=(sz+CH-1)//CH; zsz=struct.unpack_from('<%dQ'%NW,z,16); p=16+8*NW
print(f"lit: size={sz} chunked={chunked} tagged={tagged} CH={CH} NW={NW} (ref {totL})")
codec=nvcomp.Codec(algorithm="Zstd",bitstream_kind=nvcomp.BitstreamKind.RAW)
T=np.array([[b'ACGT'[(v>>6)&3],b'ACGT'[(v>>4)&3],b'ACGT'[(v>>2)&3],b'ACGT'[v&3]] for v in range(256)],dtype=np.uint8)
jobs=[]
for t in range(NW):
    raw=min(CH,sz-t*CH); c=z[p:p+zsz[t]]; p+=zsz[t]
    if tagged and c[0]==1:
        nexc,h1,h2,h3,h4=struct.unpack_from('<5I',c,1); q=21; fr=[c[q:q+h1]]; q+=h1; fr.append(c[q:q+h2]); q+=h2
        fr.append(c[q:q+h3] if h3 else None); q+=h3; fr.append(c[q:q+h4] if h4 else None); jobs.append((t,raw,'dna',nexc,fr))
    else: jobs.append((t,raw,'z',0,[c[1:] if tagged else c]))
flat=[]; idx=[]
for j,(t,raw,kind,nexc,fr) in enumerate(jobs):
    for k,f in enumerate(fr):
        if f: flat.append(f); idx.append((j,k))
arrs=[nvcomp.as_array(cp.asarray(np.frombuffer(f,dtype=np.uint8))) for f in flat]
cp.cuda.Device().synchronize(); t0=time.perf_counter(); outs=codec.decode(arrs); cp.cuda.Device().synchronize(); dt=time.perf_counter()-t0
dec={}
for (j,k),o in zip(idx,outs): dec[(j,k)]=np.asarray(cp.asnumpy(cp.asarray(o)),dtype=np.uint8)
rec=bytearray(sz); ndna=0
for j,(t,raw,kind,nexc,fr) in enumerate(jobs):
    off=t*CH
    if kind=='z': rec[off:off+raw]=dec[(j,0)][:raw].tobytes(); continue
    ndna+=1; seq=dec[(j,0)]; cse=dec[(j,1)]
    out=T[seq[:(raw+3)//4]].reshape(-1)[:raw].copy()
    bits=np.unpackbits(cse[:(raw+7)//8])[:raw].astype(bool); out[bits]|=0x20
    if nexc:
        gap=np.frombuffer(dec[(j,2)].tobytes(),dtype='<u4')[:nexc]; val=dec[(j,3)][:nexc] if (j,3) in dec else np.zeros(nexc,np.uint8)
        pos=np.cumsum(gap.astype(np.int64)); m=pos<raw; out[pos[m]]=val[m]
    rec[off:off+raw]=out.tobytes()
print(f"lit: {len(flat)} zstd frames ({ndna} DNA chunks of {NW}) nvCOMP RAW decode {dt*1e3:.1f} ms; unpack in numpy; bit-perfect={bytes(rec)==ref}")
