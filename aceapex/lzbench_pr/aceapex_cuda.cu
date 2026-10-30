// =============================================================================
// lz/aceapex/cuda/aceapex_cuda.cu
// ACEAPEX_CG: GPU-profile codec for lzbench.
//   compress  : CPU LZ match phase (aceapex_encode_raw) -> batched nvcomp rANS
//               chunks on GPU -> "ACEGPU4" container in host buffer.
//   decompress: H2D(compressed) -> nvcomp ANS (device) -> warp-per-block LZ
//               decode (device) -> D2H(output). Honest host<->host.
// Device buffers are cached in a grow-only context across calls (lzbench loops
// the call for timing; steady-state cost is what gets measured).
// Verified kernel lineage: full_gpu_decode_v3/v4 (bit-perfect on enwik9,
// silesia, FASTQ; see github.com/yasha1971-coder/aceapex).
// =============================================================================
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cuda_runtime.h>
#include <nvcomp/ans.h>
#include "aceapex_cuda.h"

#define CKR(x) do{cudaError_t e=(x); if(e){fprintf(stderr,"aceapex_cuda CUDA err %s:%d %s\n",__FILE__,__LINE__,cudaGetErrorString(e)); return 0;} }while(0)
#define NVR(x) do{nvcompStatus_t s=(x); if(s!=nvcompSuccess){fprintf(stderr,"aceapex_cuda nvcomp err %s:%d status=%d\n",__FILE__,__LINE__,(int)s); return 0;} }while(0)

#pragma pack(push,1)
struct CgBlockOffsets {
    uint64_t lit_off, off_off, len_off, cmd_off;
    uint64_t lit_sz,  off_sz,  len_sz,  cmd_sz;
};
struct CgHdr {                        // GPU-profile container header
    char     magic[8];                // "ACEGPU4\0"
    uint64_t orig_size;
    uint32_t block_size;
    uint32_t num_blocks;
    uint64_t totL, totO, totN, totC;  // raw stream sizes
    uint32_t chunk;                   // ANS chunk size (uncompressed)
    uint32_t nchunks;
};
#pragma pack(pop)

static const uint32_t CG_CHUNK = 65536;
static inline uint64_t pad16(uint64_t x){ return (x+15)&~15ull; }

// ---------------- decode kernel (verbatim v3 warp-per-block) -----------------
__device__ __forceinline__ uint32_t d_read_varint(const uint8_t* buf, uint32_t& p, uint32_t limit){
    uint32_t val=0, shift=0;
    while(p<limit){ uint8_t b=buf[p++]; val|=(uint32_t)(b&0x7F)<<shift; if(!(b&0x80)) return val; shift+=7; }
    return val;
}
__global__ void cg_k_decode(const uint8_t* __restrict__ LIT, const uint8_t* __restrict__ OFF,
                            const uint8_t* __restrict__ LEN, const uint8_t* __restrict__ CMD,
                            const CgBlockOffsets* __restrict__ boffs, uint32_t num_blocks,
                            uint64_t orig_size, uint32_t block_size, uint8_t* __restrict__ out)
{
    uint32_t gw   = (blockIdx.x*blockDim.x + threadIdx.x) >> 5;
    uint32_t lane = threadIdx.x & 31;
    if (gw >= num_blocks) return;
    CgBlockOffsets bo = boffs[gw];
    const uint8_t* lit = LIT + bo.lit_off;
    const uint8_t* off = OFF + bo.off_off;
    const uint8_t* len = LEN + bo.len_off;
    const uint8_t* cmd = CMD + bo.cmd_off;
    uint64_t base = (uint64_t)gw * block_size;
    uint64_t rem  = orig_size - base;
    uint32_t dst_size = (uint32_t)((rem < (uint64_t)block_size) ? rem : (uint64_t)block_size);
    uint8_t* dst = out + base;
    uint32_t lp=0, op=0, np=0, cp=0;
    uint32_t rep[4]={1,2,4,8};
    uint32_t out_pos=0;
    uint32_t cmd_sz=(uint32_t)bo.cmd_sz, lit_sz=(uint32_t)bo.lit_sz;
    uint32_t off_sz=(uint32_t)bo.off_sz, len_sz=(uint32_t)bo.len_sz;
    while (out_pos < dst_size) {
        uint32_t type=2, l=0, aux=0;
        if (lane==0) {
            while (cp < cmd_sz) {
                uint8_t c = cmd[cp++];
                if (c==0xFF){ rep[0]=1;rep[1]=2;rep[2]=4;rep[3]=8; continue; }
                if (c<0x80){
                    l=(uint32_t)c+1;
                    if (lp+l>lit_sz || out_pos+l>dst_size) { type=2; break; }
                    type=0; aux=lp; lp+=l;
                } else if ((c&0xC0)==0x80){
                    uint32_t ri=(c>>4)&3, lv=c&0x0F;
                    if (lv==0x0F) lv += d_read_varint(len,np,len_sz);
                    l=lv+6;
                    uint32_t dist=rep[ri];
                    if (ri>0){ for(int i=(int)ri;i>0;i--) rep[i]=rep[i-1]; rep[0]=dist; }
                    if (!dist || dist>out_pos || out_pos+l>dst_size) { type=2; break; }
                    type=1; aux=dist;
                } else {
                    uint32_t lv=(c==0xFE)? d_read_varint(len,np,len_sz) : (uint32_t)(c&0x3F);
                    l=lv+6;
                    uint32_t dist=d_read_varint(off,op,off_sz);
                    rep[3]=rep[2];rep[2]=rep[1];rep[1]=rep[0];rep[0]=dist;
                    if (!dist || dist>out_pos || out_pos+l>dst_size) { type=2; break; }
                    type=1; aux=dist;
                }
                break;
            }
        }
        type = __shfl_sync(0xffffffffu, type, 0);
        l    = __shfl_sync(0xffffffffu, l,    0);
        aux  = __shfl_sync(0xffffffffu, aux,  0);
        if (type==2) break;
        if (type==0) {
            for (uint32_t i=lane; i<l; i+=32) dst[out_pos+i] = lit[aux+i];
        } else {
            uint32_t src = out_pos - aux;
            if (aux >= l) { for (uint32_t i=lane; i<l; i+=32) dst[out_pos+i] = dst[src+i]; }
            else          { for (uint32_t i=lane; i<l; i+=32) dst[out_pos+i] = dst[src + (i % aux)]; }
        }
        __syncwarp();
        out_pos += l;
    }
}

// ---------------- cached context ---------------------------------------------
struct CgCtx {
    uint8_t *dComp=nullptr, *dRaw=nullptr, *dOut=nullptr;
    size_t   cComp=0, cRaw=0, cOut=0;
    CgBlockOffsets* dBO=nullptr; size_t cBO=0;
    void   **dPtrA=nullptr, **dPtrB=nullptr; size_t cPtr=0;     // chunk ptr arrays
    size_t  *dSzA=nullptr, *dSzB=nullptr, *dSzC=nullptr;         // chunk size arrays
    nvcompStatus_t* dSt=nullptr;
    void    *dTemp=nullptr; size_t cTemp=0;
    bool inited=false;
};
static CgCtx g_ctx;

static int grow(void** p, size_t* cap, size_t need){
    if(need<=*cap) return 1;
    if(*p) cudaFree(*p);
    *p=nullptr; *cap=0;
    if(cudaMalloc(p,need)!=cudaSuccess){ fprintf(stderr,"aceapex_cuda: cudaMalloc %zu failed\n",need); return 0; }
    *cap=need; return 1;
}

extern "C" int aceapex_cg_available(void){
    int n=0; if(cudaGetDeviceCount(&n)!=cudaSuccess) return 0;
    return n>0 ? 1 : 0;
}

extern "C" void aceapex_cg_release(void){
    CgCtx&c=g_ctx;
    if(c.dComp)cudaFree(c.dComp); if(c.dRaw)cudaFree(c.dRaw); if(c.dOut)cudaFree(c.dOut);
    if(c.dBO)cudaFree(c.dBO);
    if(c.dPtrA)cudaFree(c.dPtrA); if(c.dPtrB)cudaFree(c.dPtrB);
    if(c.dSzA)cudaFree(c.dSzA); if(c.dSzB)cudaFree(c.dSzB); if(c.dSzC)cudaFree(c.dSzC);
    if(c.dSt)cudaFree(c.dSt); if(c.dTemp)cudaFree(c.dTemp);
    c = CgCtx();
}

// ---------------- COMPRESS ----------------------------------------------------
extern "C" int64_t aceapex_cg_compress(const void* src, size_t src_size,
                                       void* dst, size_t dst_capacity,
                                       uint32_t block_size, int threads)
{
    if(!aceapex_cg_available()) return 0;
    if(block_size==0) block_size=16384;
    aceapex_raw_t r;
    if(aceapex_encode_raw(src,src_size,threads,block_size,&r)!=0) return 0;

    int64_t ret=0;
    uint64_t totRaw=r.lit_sz+r.off_sz+r.len_sz+r.cmd_sz;
    uint32_t nchunks=(uint32_t)((totRaw+CG_CHUNK-1)/CG_CHUNK);
    CgCtx& c=g_ctx;
    do{
        // upload concatenated raw streams
        if(!grow((void**)&c.dRaw,&c.cRaw,totRaw)) break;
        uint64_t o=0;
        if(cudaMemcpy(c.dRaw+o,r.lit,r.lit_sz,cudaMemcpyHostToDevice))break; o+=r.lit_sz;
        if(cudaMemcpy(c.dRaw+o,r.off,r.off_sz,cudaMemcpyHostToDevice))break; o+=r.off_sz;
        if(cudaMemcpy(c.dRaw+o,r.len,r.len_sz,cudaMemcpyHostToDevice))break; o+=r.len_sz;
        if(cudaMemcpy(c.dRaw+o,r.cmd,r.cmd_sz,cudaMemcpyHostToDevice))break;

        size_t max_out=0;
        if(nvcompBatchedANSCompressGetMaxOutputChunkSize(CG_CHUNK,nvcompBatchedANSCompressDefaultOpts,&max_out)!=nvcompSuccess) break;
        uint64_t stride=pad16(max_out);
        if(!grow((void**)&c.dComp,&c.cComp,stride*nchunks)) break;
        if(!grow((void**)&c.dPtrA,&c.cPtr,nchunks*sizeof(void*))) break;
        // (cPtr now sized for one array; allocate B and size arrays raw)
        if(c.dPtrB)cudaFree(c.dPtrB); if(cudaMalloc((void**)&c.dPtrB,nchunks*sizeof(void*)))break;
        if(c.dSzA)cudaFree(c.dSzA); if(cudaMalloc((void**)&c.dSzA,nchunks*sizeof(size_t)))break;
        if(c.dSzB)cudaFree(c.dSzB); if(cudaMalloc((void**)&c.dSzB,nchunks*sizeof(size_t)))break;
        if(c.dSt)cudaFree(c.dSt);  if(cudaMalloc((void**)&c.dSt,nchunks*sizeof(nvcompStatus_t)))break;

        std::vector<void*> hin(nchunks),hout(nchunks); std::vector<size_t> hsz(nchunks);
        for(uint32_t i=0;i<nchunks;i++){
            hin[i]=c.dRaw+(uint64_t)i*CG_CHUNK;
            hsz[i]=(i+1<nchunks)?CG_CHUNK:(size_t)(totRaw-(uint64_t)(nchunks-1)*CG_CHUNK);
            hout[i]=c.dComp+(uint64_t)i*stride;
        }
        if(cudaMemcpy(c.dPtrA,hin.data(),nchunks*sizeof(void*),cudaMemcpyHostToDevice))break;
        if(cudaMemcpy(c.dPtrB,hout.data(),nchunks*sizeof(void*),cudaMemcpyHostToDevice))break;
        if(cudaMemcpy(c.dSzA,hsz.data(),nchunks*sizeof(size_t),cudaMemcpyHostToDevice))break;

        size_t temp=0;
        if(nvcompBatchedANSCompressGetTempSizeAsync(nchunks,CG_CHUNK,nvcompBatchedANSCompressDefaultOpts,&temp,totRaw)!=nvcompSuccess)break;
        if(temp && !grow(&c.dTemp,&c.cTemp,temp)) break;
        if(nvcompBatchedANSCompressAsync((const void* const*)c.dPtrA,c.dSzA,CG_CHUNK,nchunks,
            c.dTemp,temp,(void* const*)c.dPtrB,c.dSzB,
            nvcompBatchedANSCompressDefaultOpts,c.dSt,0)!=nvcompSuccess) break;
        if(cudaDeviceSynchronize()) break;

        std::vector<size_t> csz(nchunks);
        if(cudaMemcpy(csz.data(),c.dSzB,nchunks*sizeof(size_t),cudaMemcpyDeviceToHost))break;

        // assemble container in dst
        uint64_t blob=0; for(uint32_t i=0;i<nchunks;i++) blob+=pad16(csz[i]);
        uint64_t need=sizeof(CgHdr)+r.num_blocks*sizeof(CgBlockOffsets)+8ull*nchunks+blob;
        if(need>dst_capacity) break;
        CgHdr h{}; memcpy(h.magic,"ACEGPU4",8);
        h.orig_size=r.orig_size; h.block_size=r.block_size; h.num_blocks=(uint32_t)r.num_blocks;
        h.totL=r.lit_sz;h.totO=r.off_sz;h.totN=r.len_sz;h.totC=r.cmd_sz;
        h.chunk=CG_CHUNK; h.nchunks=nchunks;
        uint8_t* p=(uint8_t*)dst;
        memcpy(p,&h,sizeof(h)); p+=sizeof(h);
        memcpy(p,r.boffs,r.num_blocks*sizeof(CgBlockOffsets)); p+=r.num_blocks*sizeof(CgBlockOffsets);
        std::vector<uint64_t> cs64(nchunks);
        for(uint32_t i=0;i<nchunks;i++) cs64[i]=csz[i];
        memcpy(p,cs64.data(),8ull*nchunks); p+=8ull*nchunks;
        // pull strided comp once, then compact into dst with 16B padding
        std::vector<uint8_t> tmp(stride);
        for(uint32_t i=0;i<nchunks;i++){
            if(cudaMemcpy(tmp.data(),c.dComp+(uint64_t)i*stride,csz[i],cudaMemcpyDeviceToHost)){ p=nullptr; break; }
            memcpy(p,tmp.data(),csz[i]);
            uint64_t pd=pad16(csz[i])-csz[i];
            if(pd) memset(p+csz[i],0,pd);
            p+=pad16(csz[i]);
        }
        if(!p) break;
        ret=(int64_t)need;
    }while(0);
    aceapex_raw_free(&r);
    return ret;
}

// ---------------- DECOMPRESS (honest host<->host) -----------------------------
extern "C" int64_t aceapex_cg_decompress(const void* src, size_t src_size,
                                         void* dst, size_t dst_capacity)
{
    if(!aceapex_cg_available()) return 0;
    if(src_size<sizeof(CgHdr)) return 0;
    CgHdr h; memcpy(&h,src,sizeof(h));
    if(memcmp(h.magic,"ACEGPU4",7)!=0) return 0;
    if(h.orig_size>dst_capacity) return 0;
    const uint8_t* p=(const uint8_t*)src+sizeof(h);
    const CgBlockOffsets* hbo=(const CgBlockOffsets*)p; p+=(uint64_t)h.num_blocks*sizeof(CgBlockOffsets);
    const uint64_t* csz=(const uint64_t*)p; p+=8ull*h.nchunks;
    uint64_t blob=0; for(uint32_t i=0;i<h.nchunks;i++) blob+=pad16(csz[i]);
    if((const uint8_t*)src+src_size < p+blob) return 0;
    uint64_t totRaw=h.totL+h.totO+h.totN+h.totC;

    static int timing = -1;
    if(timing<0){ const char* e=getenv("ACEAPEX_CUDA_TIMING"); timing=(e&&*e=='1')?1:0; }
    cudaEvent_t e0,e1,e2,e3,e4;
    if(timing){ cudaEventCreate(&e0);cudaEventCreate(&e1);cudaEventCreate(&e2);cudaEventCreate(&e3);cudaEventCreate(&e4); }

    CgCtx& c=g_ctx;
    if(!grow((void**)&c.dComp,&c.cComp,blob)) return 0;
    if(!grow((void**)&c.dRaw,&c.cRaw,totRaw)) return 0;
    if(!grow((void**)&c.dOut,&c.cOut,h.orig_size)) return 0;
    if(!grow((void**)&c.dBO,&c.cBO,(uint64_t)h.num_blocks*sizeof(CgBlockOffsets))) return 0;
    if(!grow((void**)&c.dPtrA,&c.cPtr,h.nchunks*sizeof(void*))) return 0;
    if(c.dPtrB)cudaFree(c.dPtrB); CKR(cudaMalloc((void**)&c.dPtrB,h.nchunks*sizeof(void*)));
    if(c.dSzA)cudaFree(c.dSzA); CKR(cudaMalloc((void**)&c.dSzA,h.nchunks*sizeof(size_t)));
    if(c.dSzB)cudaFree(c.dSzB); CKR(cudaMalloc((void**)&c.dSzB,h.nchunks*sizeof(size_t)));
    if(c.dSzC)cudaFree(c.dSzC); CKR(cudaMalloc((void**)&c.dSzC,h.nchunks*sizeof(size_t)));
    if(c.dSt)cudaFree(c.dSt);  CKR(cudaMalloc((void**)&c.dSt,h.nchunks*sizeof(nvcompStatus_t)));

    std::vector<void*> hc(h.nchunks),ho(h.nchunks); std::vector<size_t> hcs(h.nchunks),hbs(h.nchunks);
    { uint64_t off=0;
      for(uint32_t i=0;i<h.nchunks;i++){
        hc[i]=c.dComp+off; hcs[i]=csz[i]; off+=pad16(csz[i]);
        ho[i]=c.dRaw+(uint64_t)i*h.chunk;
        hbs[i]=(i+1<h.nchunks)?h.chunk:(size_t)(totRaw-(uint64_t)(h.nchunks-1)*h.chunk);
      } }
    CKR(cudaMemcpy(c.dPtrA,hc.data(),h.nchunks*sizeof(void*),cudaMemcpyHostToDevice));
    CKR(cudaMemcpy(c.dPtrB,ho.data(),h.nchunks*sizeof(void*),cudaMemcpyHostToDevice));
    CKR(cudaMemcpy(c.dSzA,hcs.data(),h.nchunks*sizeof(size_t),cudaMemcpyHostToDevice));
    CKR(cudaMemcpy(c.dSzB,hbs.data(),h.nchunks*sizeof(size_t),cudaMemcpyHostToDevice));
    CKR(cudaMemcpy(c.dBO,hbo,(uint64_t)h.num_blocks*sizeof(CgBlockOffsets),cudaMemcpyHostToDevice));

    size_t temp=0;
    NVR(nvcompBatchedANSDecompressGetTempSizeAsync(h.nchunks,h.chunk,
        nvcompBatchedANSDecompressDefaultOpts,&temp,totRaw));
    if(temp && !grow(&c.dTemp,&c.cTemp,temp)) return 0;

    // ---- the measured path: H2D + ANS + decode + D2H ----
    if(timing) cudaEventRecord(e0);
    CKR(cudaMemcpy(c.dComp,p,blob,cudaMemcpyHostToDevice));                  // H2D compressed
    if(timing) cudaEventRecord(e1);
    NVR(nvcompBatchedANSDecompressAsync((const void* const*)c.dPtrA,c.dSzA,c.dSzB,c.dSzC,
        h.nchunks,c.dTemp,temp,(void* const*)c.dPtrB,
        nvcompBatchedANSDecompressDefaultOpts,c.dSt,0));                      // ANS on device
    if(timing) cudaEventRecord(e2);
    const uint8_t* dLIT=c.dRaw;
    const uint8_t* dOFF=c.dRaw+h.totL;
    const uint8_t* dLEN=c.dRaw+h.totL+h.totO;
    const uint8_t* dCMD=c.dRaw+h.totL+h.totO+h.totN;
    const int TPB=128;
    uint32_t grid=(uint32_t)(((uint64_t)h.num_blocks*32+TPB-1)/TPB);
    cg_k_decode<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,c.dBO,h.num_blocks,
                              h.orig_size,h.block_size,c.dOut);               // LZ on device
    if(timing) cudaEventRecord(e3);
    CKR(cudaMemcpy(dst,c.dOut,h.orig_size,cudaMemcpyDeviceToHost));           // D2H output
    if(timing){ cudaEventRecord(e4); cudaEventSynchronize(e4); }
    CKR(cudaDeviceSynchronize());
    CKR(cudaGetLastError());

    if(timing==1){
        float a=0,b=0,d=0,f=0,t=0;
        cudaEventElapsedTime(&a,e0,e1); cudaEventElapsedTime(&b,e1,e2);
        cudaEventElapsedTime(&d,e2,e3); cudaEventElapsedTime(&f,e3,e4);
        cudaEventElapsedTime(&t,e0,e4);
        fprintf(stderr,"[aceapex_cuda] H2D %.2fms | ANS %.2fms | decode %.2fms | D2H %.2fms | total %.2fms"
                       " -> host<->host %.1f GB/s (device-resident ANS+decode: %.1f GB/s)\n",
                a,b,d,f,t, h.orig_size/(t*1e-3)/1e9, h.orig_size/((b+d)*1e-3)/1e9);
        timing=2; // print once
    }
    return (int64_t)h.orig_size;
}
