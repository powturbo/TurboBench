#!/usr/bin/env python3
# The .aet entropy layer is standard zstd frames per chunk: decode the off/len/cmd streams of an
# archive on the GPU with nvCOMP (RAW zstd bitstream) and compare byte for byte with streams.bin
# (ACEAPEX_DUMP=1). Proven 28.09.2026 on H100, chr1 interactive: 1472+17+2580 frames bit-perfect.
# Usage: python3 aet_gpu_zstd.py <archive.aet> <streams.bin>
import sys,struct,time,numpy as np,cupy as cp
from nvidia import nvcomp
a=open(sys.argv[1],'rb').read(); s=open(sys.argv[2],'rb').read()
magic,ver,orig,bs,nb,xx,zl,zo,zn,zc=struct.unpack_from('<8sIQIIQQQQQ',a,0)     # AetHeader, 68 B packed
p=68+64*nb+zl; streams={'off':a[p:p+zo],'len':a[p+zo:p+zo+zn],'cmd':a[p+zo+zn:p+zo+zn+zc]}
bo=np.frombuffer(s[68:68+64*nb],dtype=np.uint64).reshape(nb,8)   # BlockOffsets: 4 offsets then 4 sizes
totL,totO,totN,totC=[int(bo[:,i].sum()) for i in (4,5,6,7)]
q=68+64*nb+totL; ref={'off':s[q:q+totO],'len':s[q+totO:q+totO+totN],'cmd':s[q+totO+totN:q+totO+totN+totC]}
codec=nvcomp.Codec(algorithm="Zstd",bitstream_kind=nvcomp.BitstreamKind.RAW)
for name,z in streams.items():
    w=struct.unpack_from('<Q',z,0)[0]; osz=w&((1<<48)-1); ch=((w>>48)&0x7fff)*4096 or 524288   # word 0: size | chunk/4096
    nc=(osz+ch-1)//ch; cs=struct.unpack_from('<%dQ'%nc,z,8); pos=8+8*nc; frames=[]; raws=[]
    for i in range(nc):
        raw=min(ch,osz-i*ch); csz=raw if cs[i]>>63 else cs[i]&((1<<63)-1)
        (raws if cs[i]>>63 else frames).append((i,z[pos:pos+csz],raw)); pos+=csz
    arrs=[nvcomp.as_array(cp.asarray(np.frombuffer(f,dtype=np.uint8))) for _,f,_ in frames]
    cp.cuda.Device().synchronize(); t=time.perf_counter(); outs=codec.decode(arrs); cp.cuda.Device().synchronize(); dt=time.perf_counter()-t
    rec=bytearray(osz)
    for (i,_,raw),o in zip(frames,outs): rec[i*ch:i*ch+raw]=bytes(cp.asnumpy(cp.asarray(o)))[:raw]
    for i,f,raw in raws: rec[i*ch:i*ch+raw]=f
    print(f"{name}: orig={osz} chunk={ch} frames={len(frames)} raw={len(raws)} decode {dt*1e3:.2f} ms  bit-perfect={bytes(rec)==ref[name]}")
