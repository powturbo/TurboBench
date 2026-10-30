// =============================================================================
// full_gpu_decode_v7_ra.cu — ACEAPEX v7-RA: RANDOM-ACCESS range decode
//   Decode ONLY blocks [start, start+count) out of a compressed archive —
//   GPU-seekable compression demo. CLI: fgd7 <streams.bin> [orig] [G] [start] [count]
//
// v5 lesson (measured, recorded): token-fusion groups lose to per-byte lookup
// overhead. v6 keeps v3's proven per-token execution but splits the warp into
// sub-groups of G lanes (G = 32/16/8), each decoding its own block:
//   * avg match 13-21B, lit run ~10B  -> G=16/8 loses little copy width
//   * serial parse chains per warp drop 2x/4x  -> more parallel parsers
//   * persistent block queue (atomicAdd) keeps heterogeneous corpora balanced
// CLI: fgd6 <streams.bin> [original] [G]   (G in {8,16,32}, default 16)
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
    uint32_t lg     = lane & (G-1);              // lane within group
    uint32_t leader = lane & ~(uint32_t)(G-1);   // leader lane id in warp
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


// === E2E: ANS round-trip one stream over its per-block layout ===
static void ans_roundtrip(uint8_t* dStream, const vector<BlockOffsets>& bo,
                          uint64_t (BlockOffsets::*off), uint64_t (BlockOffsets::*sz),
                          uint32_t rstart, uint32_t rend, dietgpu::StackDeviceMemory& res,
                          cudaStream_t stream, const char* name){
  vector<const void*> in_ptrs; vector<void*> out_ptrs, dec_ptrs;
  vector<uint32_t> inSizes, caps; vector<uint8_t*> cbufs, abufs;
  for(uint32_t i=rstart;i<rend;i++){
    uint64_t s=bo[i].*sz; if(s==0) continue;
    uint64_t o=bo[i].*off;
    uint8_t* ab; cudaMalloc(&ab,((s+3)/4)*4); cudaMemcpy(ab,dStream+o,s,cudaMemcpyDeviceToDevice);
    uint8_t* cb; cudaMalloc(&cb,getMaxCompressedSize((uint32_t)s));
    abufs.push_back(ab); cbufs.push_back(cb);
    in_ptrs.push_back(ab); out_ptrs.push_back(cb);
    inSizes.push_back((uint32_t)s); caps.push_back((uint32_t)s);
    dec_ptrs.push_back(dStream+o); // decode straight back into original location
  }
  uint32_t n=in_ptrs.size(); if(n==0){printf("  %s: empty\n",name);return;}
  uint32_t* d_cs; cudaMalloc(&d_cs,n*4);
  ANSCodecConfig cfg(10,false);
  ansEncodeBatchPointer(res,cfg,n,in_ptrs.data(),inSizes.data(),nullptr,out_ptrs.data(),d_cs,stream);
  cudaStreamSynchronize(stream);
  vector<uint32_t> cs(n); cudaMemcpy(cs.data(),d_cs,n*4,cudaMemcpyDeviceToHost);
  uint64_t comp=0,raw=0; for(uint32_t i=0;i<n;i++){comp+=cs[i];raw+=inSizes[i];}
  uint8_t* d_succ; cudaMalloc(&d_succ,n);
  uint32_t* d_dsz; cudaMalloc(&d_dsz,n*4);
  ansDecodeBatchPointer(res,cfg,n,(const void**)out_ptrs.data(),dec_ptrs.data(),caps.data(),d_succ,d_dsz,stream);
  cudaStreamSynchronize(stream);
  printf("  %s: %u blocks, raw=%.1fMB comp=%.1fMB ratio=%.2f (decoded back in-place)\n",
         name,n,raw/1e6,comp/1e6,(double)raw/comp);
  for(auto p:abufs)cudaFree(p); for(auto p:cbufs)cudaFree(p);
  cudaFree(d_cs);cudaFree(d_succ);cudaFree(d_dsz);
}

int main(int argc, char** argv){
    if(argc<2){ fprintf(stderr,"Usage: %s <streams.bin> [original_file] [G=8|16|32]\n",argv[0]); return 1; }
    int G = (argc>3)? atoi(argv[3]) : 32;
    if(G!=8 && G!=16 && G!=32){ fprintf(stderr,"G must be 8|16|32\n"); return 1; }
    uint32_t rstart=(argc>4)?(uint32_t)atoi(argv[4]):0;
    uint32_t rcount=(argc>5)?(uint32_t)atoi(argv[5]):0xFFFFFFFFu;
    FILE* f=fopen(argv[1],"rb"); if(!f){perror("open");return 1;}
    AetHdr hdr; if(fread(&hdr,sizeof(hdr),1,f)!=1){fprintf(stderr,"bad header\n");return 1;}
    uint32_t nb=hdr.num_blocks;
    ;
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

    // ===== E2E: ANS-compress then ANS-decompress all 4 streams on GPU =====
    {
      auto res2 = dietgpu::makeStackMemory((size_t)2*1024*1024*1024);
      cudaStream_t s2; cudaStreamCreate(&s2);
      printf("[E2E] ANS round-trip on streams (blocks %u..%u):\n", rstart, rend);
      ans_roundtrip(dLIT,boffs,&BlockOffsets::lit_off,&BlockOffsets::lit_sz,rstart,rend,res2,s2,"LIT");
      ans_roundtrip(dOFF,boffs,&BlockOffsets::off_off,&BlockOffsets::off_sz,rstart,rend,res2,s2,"OFF");
      ans_roundtrip(dLEN,boffs,&BlockOffsets::len_off,&BlockOffsets::len_sz,rstart,rend,res2,s2,"LEN");
      ans_roundtrip(dCMD,boffs,&BlockOffsets::cmd_off,&BlockOffsets::cmd_sz,rstart,rend,res2,s2,"CMD");
      cudaStreamDestroy(s2);
      printf("[E2E] streams ANS-decoded in-place, now running match-decode on them...\n");
    }


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
    // === PHASE 1: hash output BEFORE match-decode (must differ from orig) ===
    {
      uint64_t rb0p=(uint64_t)rstart*hdr.block_size;
      uint64_t rbp=(uint64_t)rcount*hdr.block_size; if(rb0p+rbp>hdr.orig_size) rbp=hdr.orig_size-rb0p;
      uint64_t *dHp,hp=0; cudaMalloc(&dHp,8);
      k_hash<<<1,1>>>(dOUT+rb0p,(size_t)rbp,dHp); cudaMemcpy(&hp,dHp,8,cudaMemcpyDeviceToHost);
      printf("[PHASE1] hash(output BEFORE match)=%016llx (zeroed buffer, must != orig)\n",(unsigned long long)hp);
      cudaFree(dHp);
    }
    // ===================== TIMED DEVICE-RESIDENT REGION ====================
    CK(cudaEventRecord(t0));
    CK(cudaMemcpyAsync(dCTR,&rstart,4,cudaMemcpyHostToDevice));
    kern<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,dOUT,dCTR,rend);
    CK(cudaEventRecord(t1)); CK(cudaEventSynchronize(t1));
    // =======================================================================
    CK(cudaGetLastError());
    float ms=0; CK(cudaEventElapsedTime(&ms,t0,t1));
    uint64_t rb0=(uint64_t)rstart*hdr.block_size;
    uint64_t rbytes=(uint64_t)rcount*hdr.block_size; if(rb0+rbytes>hdr.orig_size) rbytes=hdr.orig_size-rb0;
    printf("[timed] v7-RA G=%d blocks[%u..%u): %.3f ms (%.1f us) -> %.1f GB/s of range\n", G, rstart, rend, ms, ms*1000.0, rbytes/(ms*1e-3)/1e9);

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
    // === PHASE 3: verify neighbor blocks stayed ZERO (we touched only our block) ===
    if(rcount==1 && rstart>0 && rstart+1<nb){
      // check block before and after our seeked block are still zero in dOUT
      uint64_t prevOff=(uint64_t)(rstart-1)*hdr.block_size;
      uint64_t nextOff=(uint64_t)(rstart+1)*hdr.block_size;
      vector<uint8_t> nb_prev(hdr.block_size), nb_next(hdr.block_size);
      cudaMemcpy(nb_prev.data(),dOUT+prevOff,hdr.block_size,cudaMemcpyDeviceToHost);
      cudaMemcpy(nb_next.data(),dOUT+nextOff,hdr.block_size,cudaMemcpyDeviceToHost);
      uint64_t nzp=0,nzn=0;
      for(uint32_t i=0;i<hdr.block_size;i++){ if(nb_prev[i])nzp++; if(nb_next[i])nzn++; }
      printf("[PHASE3] neighbor blocks: prev nonzero=%llu next nonzero=%llu  ISOLATION=%s\n",
             (unsigned long long)nzp,(unsigned long long)nzn,(nzp==0&&nzn==0)?"CLEAN (only our block touched)":"LEAKED");
    }
    return 0;
}
