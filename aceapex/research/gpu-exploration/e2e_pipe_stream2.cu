// =============================================================================
// e2e_pipe_stream2.cu — ACEAPEX full pipeline, streaming, LIFO-safe
//
// Bug in e2e_pipe_stream.cu (segfault): stored DietGPU GpuMemoryReservation
// objects in long-lived std::vectors mixed with transient ones. DietGPU's
// StackDeviceMemory is a strict LIFO stack -- source StackDeviceMemory.cpp:180
// "Allocations should be freed in the reverse order they are made." Holding
// reservations out of order corrupts the stack -> segfault.
//
// Fix: DO NOT keep StackDeviceMemory reservations alive across calls.
//   - persistent compressed buffers  -> plain cudaMalloc (small; ratio>1; kept
//     for the whole run, freed explicitly at the end, in any order -- cudaFree
//     has no LIFO constraint).
//   - transient per-chunk raw copies  -> plain cudaMalloc + cudaFree at the end
//     of each loop iteration (bounded: one chunk in flight).
//   - StackDeviceMemory `res` is passed ONLY into ansEncodeBatchPointer /
//     ansDecodeBatchPointer, which allocate+free their own scratch internally
//     in correct LIFO order. We never hold a reservation ourselves.
//
// Memory peak = whole-file-compressed (small) + one-chunk raw (small) + a few
// tiny host->device size arrays. Fits at ANY block_size, including 16K/8K on
// a 1GB file. Timer covers full-file ANS-decode (chunked) + one match kernel.
//
// Build:
//   nvcc -O3 -arch=sm_90 -I/workspace/dietgpu \
//     -o e2e_pipe_stream2 e2e_pipe_stream2.cu \
//     /workspace/dietgpu/dietgpu/ans/GpuANSDecode.cu \
//     /workspace/dietgpu/dietgpu/ans/GpuANSEncode.cu \
//     /workspace/dietgpu/dietgpu/utils/DeviceUtils.cpp \
//     /workspace/dietgpu/dietgpu/utils/StackDeviceMemory.cpp \
//     -lglog
//
// Run: ./e2e_pipe_stream2 streams.bin <original_file> [G=8|16|32]
// =============================================================================
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cuda_runtime.h>
#include "dietgpu/ans/GpuANSCodec.h"
#include "dietgpu/utils/StackDeviceMemory.h"
using namespace dietgpu;
using namespace std;

#define CK(x) do{cudaError_t e=(x); if(e){printf("CUDA err %s:%d %s\n",__FILE__,__LINE__,cudaGetErrorString(e));exit(1);} }while(0)

#pragma pack(push,1)
struct AetHdr {
    char     magic[8];
    uint32_t version;
    uint64_t orig_size;
    uint32_t block_size;
    uint32_t num_blocks;
    uint8_t  xxhash[8];
    uint64_t zlit_sz, zoff_sz, zlen_sz, zcmd_sz;
};
struct BlockOffsets {
    uint64_t lit_off, off_off, len_off, cmd_off;
    uint64_t lit_sz,  off_sz,  len_sz,  cmd_sz;
};
#pragma pack(pop)

__device__ __host__ static inline uint32_t rd_varint(const uint8_t* buf, uint32_t& p, uint32_t limit){
    uint32_t val=0, shift=0;
    while(p<limit){ uint8_t b=buf[p++]; val|=(uint32_t)(b&0x7F)<<shift; if(!(b&0x80)) return val; shift+=7; }
    return val;
}

template<int G>
__global__ void k_decode_g(const uint8_t* __restrict__ LIT, const uint8_t* __restrict__ OFF,
                           const uint8_t* __restrict__ LEN, const uint8_t* __restrict__ CMD,
                           const BlockOffsets* __restrict__ boffs, uint32_t num_blocks,
                           uint64_t orig_size, uint32_t block_size, uint8_t* __restrict__ out,
                           uint32_t* __restrict__ blk_ctr, uint32_t blk_end)
{
    uint32_t lane   = threadIdx.x & 31;
    uint32_t lg     = lane & (G-1);
    uint32_t leader = lane & ~(uint32_t)(G-1);
    uint32_t gmask  = ((G==32)? 0xffffffffu : ((1u<<G)-1u) << leader);
    for(;;){
        uint32_t b=0;
        if(lg==0) b=atomicAdd(blk_ctr,1u);
        b=__shfl_sync(gmask,b,leader);
        if(b>=blk_end) return;
        BlockOffsets bo = boffs[b];
        const uint8_t* lit = LIT + bo.lit_off;
        const uint8_t* off = OFF + bo.off_off;
        const uint8_t* len = LEN + bo.len_off;
        const uint8_t* cmd = CMD + bo.cmd_off;
        uint64_t base = (uint64_t)b * block_size;
        uint64_t rem  = orig_size - base;
        uint32_t dst_size = (uint32_t)((rem < (uint64_t)block_size) ? rem : (uint64_t)block_size);
        uint8_t* dst = out + base;
        uint32_t lp=0, op=0, np=0, cp=0, out_pos=0;
        uint32_t rep[4]={1,2,4,8};
        uint32_t cmd_sz=(uint32_t)bo.cmd_sz, lit_sz=(uint32_t)bo.lit_sz;
        uint32_t off_sz=(uint32_t)bo.off_sz, len_sz=(uint32_t)bo.len_sz;
        while (out_pos < dst_size) {
            uint32_t type=2, l=0, aux=0;
            if (lg==0) {
                while (cp < cmd_sz) {
                    uint8_t c = cmd[cp++];
                    if (c==0xFF){ rep[0]=1;rep[1]=2;rep[2]=4;rep[3]=8; continue; }
                    if (c<0x80){
                        l=(uint32_t)c+1;
                        if (lp+l>lit_sz || out_pos+l>dst_size) { type=2; break; }
                        type=0; aux=lp; lp+=l;
                    } else if ((c&0xC0)==0x80){
                        uint32_t ri=(c>>4)&3, lv=c&0x0F;
                        if (lv==0x0F) lv += rd_varint(len,np,len_sz);
                        l=lv+6;
                        uint32_t dist=rep[ri];
                        if (ri>0){ for(int i=(int)ri;i>0;i--) rep[i]=rep[i-1]; rep[0]=dist; }
                        if (!dist || dist>out_pos || out_pos+l>dst_size) { type=2; break; }
                        type=1; aux=dist;
                    } else {
                        uint32_t lv=(c==0xFE)? rd_varint(len,np,len_sz) : (uint32_t)(c&0x3F);
                        l=lv+6;
                        uint32_t dist=rd_varint(off,op,off_sz);
                        rep[3]=rep[2];rep[2]=rep[1];rep[1]=rep[0];rep[0]=dist;
                        if (!dist || dist>out_pos || out_pos+l>dst_size) { type=2; break; }
                        type=1; aux=dist;
                    }
                    break;
                }
            }
            type = __shfl_sync(gmask, type, leader);
            l    = __shfl_sync(gmask, l,    leader);
            aux  = __shfl_sync(gmask, aux,  leader);
            if (type==2) break;
            if (type==0) {
                for (uint32_t i=lg; i<l; i+=G) dst[out_pos+i] = lit[aux+i];
            } else {
                uint32_t src = out_pos - aux;
                if (aux >= l) { for (uint32_t i=lg; i<l; i+=G) dst[out_pos+i] = dst[src+i]; }
                else          { for (uint32_t i=lg; i<l; i+=G) dst[out_pos+i] = dst[src + (i % aux)]; }
            }
            __syncwarp(gmask);
            out_pos += l;
        }
    }
}

__global__ void k_hash(const uint8_t* buf, size_t n, uint64_t* out){
    if(blockIdx.x==0&&threadIdx.x==0){
        uint64_t h=0xcbf29ce484222325ULL;
        for(size_t i=0;i<n;i++) h=(h^buf[i])*0x100000001b3ULL;
        *out=h;
    }
}

// Persistent compressed store, plain cudaMalloc only (no DietGPU reservations kept).
struct CompStore {
    vector<void*>    comp;     // cudaMalloc'd compressed buffer per block (persistent)
    vector<void*>    dst;      // device dst (into dStream) per block
    vector<uint32_t> cap;      // decoded-size cap per block
    uint32_t n=0;
    uint64_t raw=0, cbytes=0;
};

// Encode a stream in chunks. Transient raw copies via cudaMalloc, freed each
// chunk. Compressed outputs via cudaMalloc, kept in S. res used ONLY inside
// ansEncodeBatchPointer (its own LIFO scratch).
static CompStore encode_stream(uint8_t* dStream, const vector<BlockOffsets>& bo,
                               uint64_t (BlockOffsets::*off), uint64_t (BlockOffsets::*sz),
                               uint32_t rstart, uint32_t rend, uint32_t chunkBlocks,
                               StackDeviceMemory& res, cudaStream_t stream){
    CompStore S;
    ANSCodecConfig cfg(10,false);
    for(uint32_t cs=rstart; cs<rend; cs+=chunkBlocks){
        uint32_t ce=(cs+chunkBlocks<rend)?cs+chunkBlocks:rend;
        vector<uint32_t> idx, inSizes;
        for(uint32_t i=cs;i<ce;i++){ uint32_t s=(uint32_t)(bo[i].*sz); if(s){ idx.push_back(i); inSizes.push_back(s);} }
        uint32_t nn=idx.size(); if(!nn) continue;

        vector<void*> raw(nn), out_ptrs(nn);
        vector<const void*> in_ptrs(nn);
        for(uint32_t k=0;k<nn;k++){
            uint32_t s=inSizes[k]; uint64_t o=bo[idx[k]].*off;
            void* ab; CK(cudaMalloc(&ab,((s+3)/4)*4));
            CK(cudaMemcpy(ab, dStream+o, s, cudaMemcpyDeviceToDevice));
            raw[k]=ab; in_ptrs[k]=ab;
            void* cb; CK(cudaMalloc(&cb, getMaxCompressedSize(s)));
            out_ptrs[k]=cb;
            S.comp.push_back(cb);
            S.dst.push_back(dStream+o);
            S.cap.push_back(s);
            S.raw += s;
        }
        uint32_t* d_cs; CK(cudaMalloc(&d_cs,nn*4));
        ansEncodeBatchPointer(res,cfg,nn,in_ptrs.data(),inSizes.data(),nullptr,out_ptrs.data(),d_cs,stream);
        CK(cudaStreamSynchronize(stream));
        vector<uint32_t> cs_h(nn); CK(cudaMemcpy(cs_h.data(),d_cs,nn*4,cudaMemcpyDeviceToHost));
        for(uint32_t k=0;k<nn;k++) S.cbytes += cs_h[k];
        S.n += nn;
        // free transient this-chunk buffers (order-independent: cudaFree, not stack)
        cudaFree(d_cs);
        for(uint32_t k=0;k<nn;k++) cudaFree(raw[k]);
    }
    return S;
}

static void decode_store(CompStore& S, StackDeviceMemory& res, cudaStream_t stream, uint32_t chunkBlocks){
    if(!S.n) return;
    ANSCodecConfig cfg(10,false);
    for(uint32_t cs=0; cs<S.n; cs+=chunkBlocks){
        uint32_t ce=(cs+chunkBlocks<S.n)?cs+chunkBlocks:S.n;
        uint32_t nn=ce-cs;
        uint8_t*  d_succ; CK(cudaMalloc(&d_succ,nn));
        uint32_t* d_dsz;  CK(cudaMalloc(&d_dsz,nn*4));
        ansDecodeBatchPointer(res,cfg,nn,
            (const void**)(S.comp.data()+cs),
            (void**)(S.dst.data()+cs),
            S.cap.data()+cs, d_succ, d_dsz, stream);
        cudaFree(d_succ); cudaFree(d_dsz);
    }
}

static void free_store(CompStore& S){ for(void* p:S.comp) cudaFree(p); S.comp.clear(); }

static const uint32_t CHUNK = 4096;

int main(int argc, char** argv){
    if(argc<2){ fprintf(stderr,"Usage: %s <streams.bin> [original_file] [G=8|16|32]\n",argv[0]); return 1; }
    int G=(argc>3)?atoi(argv[3]):32;
    if(G!=8&&G!=16&&G!=32){ fprintf(stderr,"G must be 8|16|32\n"); return 1; }
    FILE* f=fopen(argv[1],"rb"); if(!f){perror("open");return 1;}
    AetHdr hdr; if(fread(&hdr,sizeof(hdr),1,f)!=1){fprintf(stderr,"bad header\n");return 1;}
    uint32_t nb=hdr.num_blocks;
    vector<BlockOffsets> boffs(nb);
    if(fread(boffs.data(),sizeof(BlockOffsets),nb,f)!=nb){fprintf(stderr,"bad boffs\n");return 1;}
    uint64_t totL=0,totO=0,totN=0,totC=0;
    for(auto&b:boffs){totL+=b.lit_sz;totO+=b.off_sz;totN+=b.len_sz;totC+=b.cmd_sz;}
    vector<uint8_t> LIT(totL),OFF(totO),LEN(totN),CMD(totC);
    if(fread(LIT.data(),1,totL,f)!=totL) fprintf(stderr,"short LIT\n");
    if(fread(OFF.data(),1,totO,f)!=totO) fprintf(stderr,"short OFF\n");
    if(fread(LEN.data(),1,totN,f)!=totN) fprintf(stderr,"short LEN\n");
    if(fread(CMD.data(),1,totC,f)!=totC) fprintf(stderr,"short CMD\n");
    fclose(f);
    uint32_t rstart=0, rend=nb, rcount=nb;
    printf("orig=%llu blocks=%u block_size=%u G=%d rawL/O/N/C=%.1f/%.1f/%.1f/%.1f MB\n",
        (unsigned long long)hdr.orig_size,nb,hdr.block_size,G,totL/1e6,totO/1e6,totN/1e6,totC/1e6);

    uint8_t *dLIT,*dOFF,*dLEN,*dCMD,*dOUT; BlockOffsets* dBO; uint32_t* dCTR;
    CK(cudaMalloc(&dLIT,totL)); CK(cudaMalloc(&dOFF,totO));
    CK(cudaMalloc(&dLEN,totN)); CK(cudaMalloc(&dCMD,totC));
    CK(cudaMalloc(&dOUT,hdr.orig_size)); CK(cudaMalloc(&dBO,nb*sizeof(BlockOffsets)));
    CK(cudaMalloc(&dCTR,4));
    CK(cudaMemcpy(dLIT,LIT.data(),totL,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dOFF,OFF.data(),totO,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dLEN,LEN.data(),totN,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dCMD,CMD.data(),totC,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dBO,boffs.data(),nb*sizeof(BlockOffsets),cudaMemcpyHostToDevice));

    auto res=dietgpu::makeStackMemory((size_t)4*1024*1024*1024);
    cudaStream_t s2; cudaStreamCreate(&s2);

    printf("[PREP] streaming ANS-encode (chunk=%u blocks), cudaMalloc-owned, RAII-free stack...\n", CHUNK);
    CompStore SL=encode_stream(dLIT,boffs,&BlockOffsets::lit_off,&BlockOffsets::lit_sz,rstart,rend,CHUNK,res,s2);
    CompStore SO=encode_stream(dOFF,boffs,&BlockOffsets::off_off,&BlockOffsets::off_sz,rstart,rend,CHUNK,res,s2);
    CompStore SN=encode_stream(dLEN,boffs,&BlockOffsets::len_off,&BlockOffsets::len_sz,rstart,rend,CHUNK,res,s2);
    CompStore SC=encode_stream(dCMD,boffs,&BlockOffsets::cmd_off,&BlockOffsets::cmd_sz,rstart,rend,CHUNK,res,s2);
    double lr=SL.cbytes?(double)SL.raw/SL.cbytes:0, orr=SO.cbytes?(double)SO.raw/SO.cbytes:0;
    double nr=SN.cbytes?(double)SN.raw/SN.cbytes:0, cr=SC.cbytes?(double)SC.raw/SC.cbytes:0;
    printf("  LIT ratio=%.3f OFF=%.3f LEN=%.3f CMD=%.3f  comp total=%.1f MB\n",
        lr,orr,nr,cr,(SL.cbytes+SO.cbytes+SN.cbytes+SC.cbytes)/1e6);
    size_t mfree,mtot; cudaMemGetInfo(&mfree,&mtot);
    printf("[PREP] done. VRAM free=%.1f/%.1f GB\n",mfree/1e9,mtot/1e9);

    const int TPB=128; int dev=0,nsm=0; CK(cudaGetDevice(&dev));
    CK(cudaDeviceGetAttribute(&nsm,cudaDevAttrMultiProcessorCount,dev));
    void (*kern)(const uint8_t*,const uint8_t*,const uint8_t*,const uint8_t*,const BlockOffsets*,uint32_t,uint64_t,uint32_t,uint8_t*,uint32_t*,uint32_t)
        = (G==8)?k_decode_g<8>:(G==16)?k_decode_g<16>:k_decode_g<32>;
    int maxblk=0; CK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&maxblk,kern,TPB,0));
    uint64_t lanes=(uint64_t)rcount*G; uint32_t want=(uint32_t)((lanes+TPB-1)/TPB);
    uint32_t grid=(uint32_t)nsm*(uint32_t)maxblk; if(grid>want)grid=want; if(grid<1)grid=1;

    CK(cudaMemcpy(dCTR,&rstart,4,cudaMemcpyHostToDevice));
    kern<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,dOUT,dCTR,rend);
    CK(cudaDeviceSynchronize());
    CK(cudaMemset(dOUT,0,hdr.orig_size));

    cudaEvent_t t0,t1; CK(cudaEventCreate(&t0)); CK(cudaEventCreate(&t1));
    CK(cudaEventRecord(t0));
    decode_store(SL,res,s2,CHUNK); decode_store(SO,res,s2,CHUNK);
    decode_store(SN,res,s2,CHUNK); decode_store(SC,res,s2,CHUNK);
    CK(cudaStreamSynchronize(s2));
    CK(cudaMemcpyAsync(dCTR,&rstart,4,cudaMemcpyHostToDevice));
    kern<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,dOUT,dCTR,rend);
    CK(cudaEventRecord(t1)); CK(cudaEventSynchronize(t1));
    CK(cudaGetLastError());
    float ms=0; CK(cudaEventElapsedTime(&ms,t0,t1));
    printf("[timed] FULL-PIPE-STREAM (ANS+match, WHOLE FILE) G=%d blocks[0..%u): %.3f ms -> %.1f GB/s\n",
        G,rend,ms,hdr.orig_size/(ms*1e-3)/1e9);

    uint64_t *dH,h=0; CK(cudaMalloc(&dH,8));
    k_hash<<<1,1>>>(dOUT,(size_t)hdr.orig_size,dH); CK(cudaMemcpy(&h,dH,8,cudaMemcpyDeviceToHost));
    printf("[5] FNV=%016llx (%llu bytes)\n",(unsigned long long)h,(unsigned long long)hdr.orig_size);
    if(argc>2){
        FILE* fo=fopen(argv[2],"rb");
        if(fo){ vector<uint8_t> orig(hdr.orig_size);
            if(fread(orig.data(),1,hdr.orig_size,fo)!=hdr.orig_size) fprintf(stderr,"short orig\n");
            fclose(fo);
            uint64_t ho=0xcbf29ce484222325ULL; for(uint8_t x:orig) ho=(ho^x)*0x100000001b3ULL;
            printf("    orig FNV=%016llx  %s\n",(unsigned long long)ho, ho==h?"MATCHES OK":"DIFFERS X");
        }
    }
    free_store(SL); free_store(SO); free_store(SN); free_store(SC);
    cudaStreamDestroy(s2);
    return 0;
}
