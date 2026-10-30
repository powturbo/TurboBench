// =============================================================================
// e2e_pipe_litonly.cu — ACEAPEX full pipeline, LITERALS-ONLY GPU ANS
//
// HYPOTHESIS UNDER TEST (from CONTEXT 2026-06-22 "End-to-end ДОКАЗАН" record):
//   Per-stream ANS ratios measured on real ACEAPEX streams:
//     LIT: 1.42-3.43 (ANS WINS — literals are near-pure entropy after LZ77)
//     OFF: 0.27-1.04 (ANS INFLATES on genomic/mixed data)
//     LEN: 0.04-1.05 (ANS CATASTROPHICALLY inflates on genomic data)
//     CMD: 0.48-1.41 (mixed, often inflates)
//   Conclusion already recorded: "ANS применять ТОЛЬКО к LIT; offsets/lengths/
//   commands нужен другой механизм (varint уже делает своё)."
//
// This variant acts on that conclusion directly: only LIT is round-tripped
// through GPU ANS (encode before timer, decode inside timer, exactly as in
// e2e_pipe.cu). OFF/LEN/CMD are NOT touched by ANS at all -- they are used
// exactly as uploaded (already varint-packed by the CPU encoder in the
// streams.bin format; this is the same raw form the CPU-profile pipeline
// already relies on, since aceapex_depth's step0 dump writes decompressed
// off/len/cmd streams).
//
// WHY THIS IS A FAIR, HONEST TEST:
//   - Correctness is unaffected: skipping an ANS encode+decode ROUND TRIP on
//     a stream that was already correct in device memory cannot change its
//     content. dOFF/dLEN/dCMD hold the same bytes with or without the
//     round trip. FNV bit-perfect check remains the ground truth either way.
//   - This measures a REALISTIC pipeline shape: CPU-side entropy on off/len/
//     cmd is already known to be cheap (from Test B, 2026-06-08: lit+fse
//     0.001-0.004s vs lz77 0.003-0.008s -- entropy on these streams is not
//     the bottleneck even on CPU). The expensive things are LIT-entropy
//     (where GPU ANS gives a real 600-700+ GB/s win) and match (GPU-bound).
//     This variant is the natural "put GPU effort only where it pays."
//   - The comparison to report is: FULL-PIPE-LITONLY GB/s and ratio vs the
//     all-4-streams e2e_pipe.cu numbers already in CONTEXT, same datasets,
//     same block_size. Ratio here = orig_size / (LIT_comp + OFF_raw + LEN_raw
//     + CMD_raw) -- i.e. the honest total including the streams that are
//     NOT entropy-coded in this variant, not just the LIT-only ratio.
//
// Build:
//   nvcc -O3 -arch=sm_90 -I<dietgpu_root> \
//     -o e2e_pipe_litonly e2e_pipe_litonly.cu \
//     <dietgpu_root>/dietgpu/ans/GpuANSDecode.cu \
//     <dietgpu_root>/dietgpu/ans/GpuANSEncode.cu \
//     <dietgpu_root>/dietgpu/utils/DeviceUtils.cpp \
//     <dietgpu_root>/dietgpu/utils/StackDeviceMemory.cpp \
//     -lglog
//
// Run (same CLI as e2e_pipe.cu):
//   ./e2e_pipe_litonly streams.bin <original_file> [G=8|16|32] [start] [count]
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

// ---- match kernel: byte-identical to e2e_pipe.cu ----
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

// ---- ANS prep/decode: used ONLY for LIT in this variant ----
struct AnsPrep {
  vector<void*> comp_ptrs;
  vector<void*> dec_ptrs;
  vector<uint32_t> caps;
  vector<uint8_t*> to_free;
  uint32_t n=0;
  uint64_t raw_bytes=0, comp_bytes=0;
};

static AnsPrep ans_prep(uint8_t* dStream, const vector<BlockOffsets>& bo,
                        uint64_t (BlockOffsets::*off), uint64_t (BlockOffsets::*sz),
                        uint32_t rstart, uint32_t rend, dietgpu::StackDeviceMemory& res,
                        cudaStream_t stream){
  AnsPrep P;
  vector<const void*> in_ptrs; vector<void*> out_ptrs;
  vector<uint32_t> inSizes;
  for(uint32_t i=rstart;i<rend;i++){
    uint64_t s=bo[i].*sz; if(s==0) continue;
    uint64_t o=bo[i].*off;
    uint8_t* ab; cudaMalloc(&ab,((s+3)/4)*4); cudaMemcpy(ab,dStream+o,s,cudaMemcpyDeviceToDevice);
    uint8_t* cb; cudaMalloc(&cb,getMaxCompressedSize((uint32_t)s));
    P.to_free.push_back(ab); P.to_free.push_back(cb);
    in_ptrs.push_back(ab); out_ptrs.push_back(cb);
    inSizes.push_back((uint32_t)s);
    P.comp_ptrs.push_back(cb);
    P.dec_ptrs.push_back(dStream+o);
    P.caps.push_back((uint32_t)s);
  }
  P.n=in_ptrs.size(); if(P.n==0) return P;
  uint32_t* d_cs; cudaMalloc(&d_cs,P.n*4);
  ANSCodecConfig cfg(10,false);
  ansEncodeBatchPointer(res,cfg,P.n,in_ptrs.data(),inSizes.data(),nullptr,out_ptrs.data(),d_cs,stream);
  cudaStreamSynchronize(stream);
  vector<uint32_t> cs(P.n); cudaMemcpy(cs.data(),d_cs,P.n*4,cudaMemcpyDeviceToHost);
  for(uint32_t i=0;i<P.n;i++){ P.comp_bytes+=cs[i]; P.raw_bytes+=inSizes[i]; }
  cudaFree(d_cs);
  return P;
}

static void ans_decode(AnsPrep& P, dietgpu::StackDeviceMemory& res, cudaStream_t stream){
  if(P.n==0) return;
  ANSCodecConfig cfg(10,false);
  uint8_t* d_succ; cudaMalloc(&d_succ,P.n);
  uint32_t* d_dsz; cudaMalloc(&d_dsz,P.n*4);
  ansDecodeBatchPointer(res,cfg,P.n,(const void**)P.comp_ptrs.data(),P.dec_ptrs.data(),
                        P.caps.data(),d_succ,d_dsz,stream);
  cudaFree(d_succ); cudaFree(d_dsz);
}
static void ans_prep_free(AnsPrep& P){ for(auto p:P.to_free) cudaFree(p); }

// same safe chunk size as e2e_pipe_chunked.cu; LIT can still be large on 1GB files
static const uint32_t ANS_CHUNK_BLOCKS = 4096;

int main(int argc, char** argv){
    if(argc<2){ fprintf(stderr,"Usage: %s <streams.bin> [original_file] [G=8|16|32]\n",argv[0]); return 1; }
    int G = (argc>3)? atoi(argv[3]) : 32;
    if(G!=8 && G!=16 && G!=32){ fprintf(stderr,"G must be 8|16|32\n"); return 1; }
    uint32_t rstart=(argc>4)?(uint32_t)atoi(argv[4]):0;
    uint32_t rcount=(argc>5)?(uint32_t)atoi(argv[5]):0xFFFFFFFFu;
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
    if(rstart>nb)rstart=nb; if(rcount>nb-rstart)rcount=nb-rstart;
    uint32_t rend=rstart+rcount;
    printf("orig=%llu blocks=%u block_size=%u G=%d  rawL/O/N/C=%.1f/%.1f/%.1f/%.1f MB\n",
        (unsigned long long)hdr.orig_size, nb, hdr.block_size, G, totL/1e6,totO/1e6,totN/1e6,totC/1e6);

    uint8_t *dLIT,*dOFF,*dLEN,*dCMD,*dOUT; BlockOffsets* dBO; uint32_t* dCTR;
    CK(cudaMalloc(&dLIT,totL)); CK(cudaMalloc(&dOFF,totO));
    CK(cudaMalloc(&dLEN,totN)); CK(cudaMalloc(&dCMD,totC));
    CK(cudaMalloc(&dOUT,hdr.orig_size)); CK(cudaMalloc(&dBO,nb*sizeof(BlockOffsets)));
    CK(cudaMalloc(&dCTR,sizeof(uint32_t)));
    CK(cudaMemcpy(dLIT,LIT.data(),totL,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dOFF,OFF.data(),totO,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dLEN,LEN.data(),totN,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dCMD,CMD.data(),totC,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dBO,boffs.data(),nb*sizeof(BlockOffsets),cudaMemcpyHostToDevice));

    // ===== PRE-TIMER: ANS-encode LIT ONLY, in chunks. OFF/LEN/CMD untouched. =====
    auto res2 = dietgpu::makeStackMemory((size_t)2*1024*1024*1024);
    cudaStream_t s2; cudaStreamCreate(&s2);

    vector<AnsPrep> chunksL;
    uint64_t rL=0,cL=0;
    uint32_t nchunks=0;
    printf("[PREP] ANS-encoding LIT ONLY, chunks of %u blocks (blocks %u..%u):\n", ANS_CHUNK_BLOCKS, rstart, rend);
    for (uint32_t cs=rstart; cs<rend; cs+=ANS_CHUNK_BLOCKS) {
        uint32_t ce = (cs+ANS_CHUNK_BLOCKS<rend)? cs+ANS_CHUNK_BLOCKS : rend;
        AnsPrep pL = ans_prep(dLIT,boffs,&BlockOffsets::lit_off,&BlockOffsets::lit_sz,cs,ce,res2,s2);
        rL+=pL.raw_bytes; cL+=pL.comp_bytes;
        chunksL.push_back(pL);
        nchunks++;
    }
    printf("  LIT: raw=%.1fMB comp=%.1fMB ratio=%.3f (ANS, GPU)\n", rL/1e6, cL/1e6, cL? (double)rL/cL : 0.0);
    printf("  OFF: raw=%.1fMB (NOT entropy-coded on GPU in this variant)\n", totO/1e6);
    printf("  LEN: raw=%.1fMB (NOT entropy-coded on GPU in this variant)\n", totN/1e6);
    printf("  CMD: raw=%.1fMB (NOT entropy-coded on GPU in this variant)\n", totC/1e6);
    double honest_ratio = (double)hdr.orig_size / (cL + totO + totN + totC);
    printf("[PREP] done, %u chunks. Honest total ratio (LIT-ANS + raw O/N/C vs orig) = %.3f\n",
           nchunks, honest_ratio);

    const int TPB=128;
    int dev=0,nsm=0; CK(cudaGetDevice(&dev));
    CK(cudaDeviceGetAttribute(&nsm,cudaDevAttrMultiProcessorCount,dev));
    void (*kern)(const uint8_t*,const uint8_t*,const uint8_t*,const uint8_t*,const BlockOffsets*,uint32_t,uint64_t,uint32_t,uint8_t*,uint32_t*,uint32_t)
        = (G==8)? k_decode_g<8> : (G==16)? k_decode_g<16> : k_decode_g<32>;
    int maxblk=0; CK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&maxblk,kern,TPB,0));
    uint64_t lanes_needed=(uint64_t)rcount*G;
    uint32_t want=(uint32_t)((lanes_needed+TPB-1)/TPB);
    uint32_t grid=(uint32_t)nsm*(uint32_t)maxblk; if(grid>want)grid=want; if(grid<1)grid=1;
    printf("persistent grid=%u (%d SM x %d), groups/warp=%d, parallel parsers up to %u\n",
        grid,nsm,maxblk,32/G,grid*(TPB/32)*(32/G));

    cudaEvent_t t0,t1; CK(cudaEventCreate(&t0)); CK(cudaEventCreate(&t1));
    CK(cudaMemcpy(dCTR,&rstart,4,cudaMemcpyHostToDevice));
    kern<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,dOUT,dCTR,rend);
    CK(cudaDeviceSynchronize()); CK(cudaGetLastError());
    CK(cudaMemset(dOUT,0,hdr.orig_size));

    // ===================== TIMED: LIT ANS-decode (chunked) + match. OFF/LEN/CMD =====
    // are already correct in device memory -- no decode step needed for them.
    CK(cudaEventRecord(t0));
    for (auto& p : chunksL) ans_decode(p, res2, s2);
    cudaStreamSynchronize(s2);
    CK(cudaMemcpyAsync(dCTR,&rstart,4,cudaMemcpyHostToDevice));
    kern<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,dOUT,dCTR,rend);
    CK(cudaEventRecord(t1)); CK(cudaEventSynchronize(t1));
    // ================================================================================
    CK(cudaGetLastError());
    float ms=0; CK(cudaEventElapsedTime(&ms,t0,t1));
    uint64_t rb0=(uint64_t)rstart*hdr.block_size;
    uint64_t rbytes=(uint64_t)rcount*hdr.block_size; if(rb0+rbytes>hdr.orig_size) rbytes=hdr.orig_size-rb0;
    printf("[timed] FULL-PIPE-LITONLY (LIT-ANS+match) G=%d blocks[%u..%u): %.3f ms (%.1f us) -> %.1f GB/s of range\n",
           G, rstart, rend, ms, ms*1000.0, rbytes/(ms*1e-3)/1e9);

    uint64_t *dH,h=0; CK(cudaMalloc(&dH,8));
    k_hash<<<1,1>>>(dOUT+rb0,(size_t)rbytes,dH); CK(cudaMemcpy(&h,dH,8,cudaMemcpyDeviceToHost));
    printf("[5] range FNV=%016llx (%llu bytes)\n",(unsigned long long)h,(unsigned long long)rbytes);
    if(argc>2){
        FILE* fo=fopen(argv[2],"rb");
        if(fo){ vector<uint8_t> orig(rbytes);
            fseek(fo,(long)rb0,SEEK_SET);
            if(fread(orig.data(),1,rbytes,fo)!=rbytes) fprintf(stderr,"short orig range\n");
            fclose(fo);
            uint64_t ho=0xcbf29ce484222325ULL; for(uint8_t x:orig) ho=(ho^x)*0x100000001b3ULL;
            printf("    orig FNV=%016llx  %s\n",(unsigned long long)ho, ho==h?"MATCHES OK":"DIFFERS X");
        }
    }
    for (auto& p : chunksL) ans_prep_free(p);
    cudaStreamDestroy(s2);
    return 0;
}
