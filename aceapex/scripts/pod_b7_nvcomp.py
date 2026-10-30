#!/usr/bin/env python3
# B7: nvCOMP batched LZ4/Deflate/Zstd at equal chunk sizes — size, ratio, decode GB/s, bit-perfect.
# Usage: ALGOS=LZ4,Deflate,Zstd CHUNKS=16384,65536 python3 pod_b7_nvcomp.py <file>   (H100, nvcomp>=5)
import sys,time,os,cupy as cp
from nvidia import nvcomp
f=sys.argv[1]; data=open(f,'rb').read(); n=len(data)
d_in=nvcomp.as_array(cp.asarray(bytearray(data)))
for algo in os.environ.get("ALGOS","LZ4,Deflate,Zstd").split(","):
  for cs in [int(x) for x in os.environ.get("CHUNKS","16384,65536,1048576").split(",")]:
    try:
      codec=nvcomp.Codec(algorithm=algo,uncomp_chunk_size=cs)
      comp=codec.encode(d_in); cp.cuda.Device().synchronize(); csz=comp.buffer_size; ts=[]
      for _ in range(4):
        cp.cuda.Device().synchronize(); t=time.perf_counter(); out=codec.decode(comp); cp.cuda.Device().synchronize(); ts.append(time.perf_counter()-t)
      ok=bytes(cp.asnumpy(cp.asarray(out)))==data; m=sorted(ts)[1]
      print(f"{os.path.basename(f)} {algo} chunk={cs>>10}K: size={csz} ratio={n/csz:.4f} decode={n/m/1e9:.1f} GB/s bitperfect={ok}")
    except Exception as e: print(f"{os.path.basename(f)} {algo} chunk={cs>>10}K: ERROR {e}")
