// =============================================================================
// e2e_pipe_tile.cu — ACEAPEX full pipeline, TILE-ANS (decoupled granularity)
//
// THE IDEA (proven mechanism, this session):
//   Entropy tile granularity is DECOUPLED from LZ block_size. The match kernel
//   keeps whatever block_size the .aet was encoded with (small = high occupancy
//   on match phase, ~405 GB/s at 8K). The ANS layer works on FIXED 64KB tiles
//   cut straight across each flat stream, in GROUPS of <=6000 tiles (proven
//   bit-perfect above the single-call ceiling of 6800 by tile_grouped_smoke).
//
//   Target: full pipeline closer to 1/(1/match + 1/entropy) with each layer at
//   its own optimum, instead of both forced onto one shared block_size (which
//   capped us at 159 GB/s and got worse at small blocks).
//
// WHAT'S PROVEN vs NEW here:
//   - match kernel k_decode_g: byte-identical to e2e_pipe.cu (NOT touched).
//   - memory ownership (cudaMalloc, StackDeviceMemory only inside ANS calls):
//     same discipline that made stream2 work (LIFO-safe).
//   - Stride grouping <=6000 tiles: proven bit-perfect by tile_grouped_smoke.
//   - NEW: cut ANS tiles across the flat stream (not per LZ-block), decode back
//     into the same flat device buffer the match kernel reads. Tiles align to
//     the buffer exactly because outPerBatchStride == TILE == tile position.
//
// Honest timing: encode all tiles (all streams) BEFORE timer ("already
// compressed"). TIMED region = grouped ANS-decode of all 4 streams + one match
// kernel. Full pipeline, whole file, no window.
//
// Build:
//   nvcc -O3 -arch=sm_90 -I/workspace/dietgpu -o e2e_pipe_tile e2e_pipe_tile.cu \
//     /workspace/dietgpu/dietgpu/ans/GpuANSDecode.cu \
//     /workspace/dietgpu/dietgpu/ans/GpuANSEncode.cu \
//     /workspace/dietgpu/dietgpu/utils/DeviceUtils.cpp \
//     /workspace/dietgpu/dietgpu/utils/StackDeviceMemory.cpp -lglog
//
// Run: ./e2e_pipe_tile streams.bin <original_file> [G=8|16|32]
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

static const uint32_t TILE  = 65536;
static const uint32_t GROUP = 6000;   // proven safe below 6800 ceiling

// One stream's tile-ANS store. dFlat is the flat device buffer the match kernel
// reads; we tile ACROSS it. dComp holds compressed tiles (persistent, small).
struct TileStore {
    uint8_t* dFlat=nullptr;   // flat stream buffer (match kernel reads this)
    uint8_t* dComp=nullptr;   // compressed tiles, contiguous, stride=maxComp
    uint32_t numTiles=0;
    uint64_t paddedBytes=0;
    uint64_t realBytes=0;
    uint32_t maxComp=0;
    uint64_t compBytes=0;     // sum of actual compressed sizes (for ratio)
};

// Encode a flat stream into tiles (pre-timer). dFlat must be allocated to
// paddedBytes (multiple of TILE); tail beyond realBytes is zero-padded.
static TileStore tile_encode(uint8_t* dFlatReal, uint64_t realBytes,
                             StackDeviceMemory& res, cudaStream_t s){
    TileStore T;
    T.realBytes = realBytes;
    T.numTiles  = (uint32_t)((realBytes + TILE - 1) / TILE);
    if (T.numTiles == 0) T.numTiles = 1;
    T.paddedBytes = (uint64_t)T.numTiles * TILE;
    T.maxComp = getMaxCompressedSize(TILE);

    // flat buffer padded to whole tiles
    CK(cudaMalloc(&T.dFlat, T.paddedBytes));
    CK(cudaMemset(T.dFlat, 0, T.paddedBytes));
    CK(cudaMemcpy(T.dFlat, dFlatReal, realBytes, cudaMemcpyDeviceToDevice));

    CK(cudaMalloc(&T.dComp, (size_t)T.numTiles * T.maxComp));
    uint32_t* dSizes; CK(cudaMalloc(&dSizes, T.numTiles*4));
    ANSCodecConfig cfg(10,false);

    for (uint32_t g0=0; g0<T.numTiles; g0+=GROUP){
        uint32_t gn = (g0+GROUP<T.numTiles)? GROUP : (T.numTiles-g0);
        const void* inSeg  = T.dFlat + (uint64_t)g0*TILE;
        void*       outSeg = T.dComp + (uint64_t)g0*T.maxComp;
        ansEncodeBatchStride(res,cfg,gn, inSeg,TILE,TILE, nullptr, outSeg,T.maxComp, dSizes+g0, s);
    }
    CK(cudaStreamSynchronize(s));
    vector<uint32_t> sz(T.numTiles); CK(cudaMemcpy(sz.data(),dSizes,T.numTiles*4,cudaMemcpyDeviceToHost));
    for(uint32_t i=0;i<T.numTiles;i++) T.compBytes += sz[i];
    cudaFree(dSizes);
    return T;
}

// Decode all tiles back into T.dFlat (timed). Grouped.
static void tile_decode(TileStore& T, StackDeviceMemory& res, cudaStream_t s){
    ANSCodecConfig cfg(10,false);
    uint8_t* dSucc; CK(cudaMalloc(&dSucc, T.numTiles));
    uint32_t* dDsz; CK(cudaMalloc(&dDsz, T.numTiles*4));
    for (uint32_t g0=0; g0<T.numTiles; g0+=GROUP){
        uint32_t gn = (g0+GROUP<T.numTiles)? GROUP : (T.numTiles-g0);
        const void* inSeg  = T.dComp + (uint64_t)g0*T.maxComp;
        void*       outSeg = T.dFlat + (uint64_t)g0*TILE;
        ansDecodeBatchStride(res,cfg,gn, inSeg,T.maxComp, outSeg,TILE,TILE, dSucc+g0,dDsz+g0, s);
    }
    cudaFree(dSucc); cudaFree(dDsz);
}

static void tile_free(TileStore& T){ if(T.dFlat)cudaFree(T.dFlat); if(T.dComp)cudaFree(T.dComp); }

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
    printf("orig=%llu blocks=%u block_size=%u G=%d rawL/O/N/C=%.1f/%.1f/%.1f/%.1f MB\n",
        (unsigned long long)hdr.orig_size,nb,hdr.block_size,G,totL/1e6,totO/1e6,totN/1e6,totC/1e6);

    // upload raw streams to temporary device buffers (source for tiling)
    uint8_t *rLIT,*rOFF,*rLEN,*rCMD;
    CK(cudaMalloc(&rLIT,totL)); CK(cudaMalloc(&rOFF,totO));
    CK(cudaMalloc(&rLEN,totN)); CK(cudaMalloc(&rCMD,totC));
    CK(cudaMemcpy(rLIT,LIT.data(),totL,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(rOFF,OFF.data(),totO,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(rLEN,LEN.data(),totN,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(rCMD,CMD.data(),totC,cudaMemcpyHostToDevice));

    uint8_t* dOUT; CK(cudaMalloc(&dOUT,hdr.orig_size));
    BlockOffsets* dBO; CK(cudaMalloc(&dBO,nb*sizeof(BlockOffsets)));
    CK(cudaMemcpy(dBO,boffs.data(),nb*sizeof(BlockOffsets),cudaMemcpyHostToDevice));
    uint32_t* dCTR; CK(cudaMalloc(&dCTR,4));

    auto res=dietgpu::makeStackMemory((size_t)2*1024*1024*1024);
    cudaStream_t s2; cudaStreamCreate(&s2);

    // ---- PRE-TIMER: tile-encode all 4 streams ----
    printf("[PREP] tile-encoding (TILE=%u, GROUP=%u)...\n", TILE, GROUP);
    TileStore TL=tile_encode(rLIT,totL,res,s2);
    TileStore TO=tile_encode(rOFF,totO,res,s2);
    TileStore TN=tile_encode(rLEN,totN,res,s2);
    TileStore TC=tile_encode(rCMD,totC,res,s2);
    cudaFree(rLIT); cudaFree(rOFF); cudaFree(rLEN); cudaFree(rCMD);
    double compMB=(TL.compBytes+TO.compBytes+TN.compBytes+TC.compBytes)/1e6;
    printf("  tiles L/O/N/C=%u/%u/%u/%u  comp total=%.1f MB\n",
        TL.numTiles,TO.numTiles,TN.numTiles,TC.numTiles,compMB);
    size_t mfree,mtot; cudaMemGetInfo(&mfree,&mtot);
    printf("[PREP] done. VRAM free=%.1f/%.1f GB\n",mfree/1e9,mtot/1e9);

    // match-kernel setup (reads TL.dFlat etc. as the flat streams)
    const int TPB=128; int dev=0,nsm=0; CK(cudaGetDevice(&dev));
    CK(cudaDeviceGetAttribute(&nsm,cudaDevAttrMultiProcessorCount,dev));
    void (*kern)(const uint8_t*,const uint8_t*,const uint8_t*,const uint8_t*,const BlockOffsets*,uint32_t,uint64_t,uint32_t,uint8_t*,uint32_t*,uint32_t)
        = (G==8)?k_decode_g<8>:(G==16)?k_decode_g<16>:k_decode_g<32>;
    int maxblk=0; CK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&maxblk,kern,TPB,0));
    uint64_t lanes=(uint64_t)nb*G; uint32_t want=(uint32_t)((lanes+TPB-1)/TPB);
    uint32_t grid=(uint32_t)nsm*(uint32_t)maxblk; if(grid>want)grid=want; if(grid<1)grid=1;

    // warmup (decode once + match) to fill caches, not timed
    tile_decode(TL,res,s2); tile_decode(TO,res,s2); tile_decode(TN,res,s2); tile_decode(TC,res,s2);
    CK(cudaStreamSynchronize(s2));
    uint32_t zero=0; CK(cudaMemcpy(dCTR,&zero,4,cudaMemcpyHostToDevice));
    kern<<<grid,TPB>>>(TL.dFlat,TO.dFlat,TN.dFlat,TC.dFlat,dBO,nb,hdr.orig_size,hdr.block_size,dOUT,dCTR,nb);
    CK(cudaDeviceSynchronize());
    CK(cudaMemset(dOUT,0,hdr.orig_size));

    // ---- TIMED: grouped tile-decode (4 streams) + match ----
    cudaEvent_t t0,t1; CK(cudaEventCreate(&t0)); CK(cudaEventCreate(&t1));
    CK(cudaEventRecord(t0));
    tile_decode(TL,res,s2); tile_decode(TO,res,s2); tile_decode(TN,res,s2); tile_decode(TC,res,s2);
    CK(cudaStreamSynchronize(s2));
    CK(cudaMemcpyAsync(dCTR,&zero,4,cudaMemcpyHostToDevice));
    kern<<<grid,TPB>>>(TL.dFlat,TO.dFlat,TN.dFlat,TC.dFlat,dBO,nb,hdr.orig_size,hdr.block_size,dOUT,dCTR,nb);
    CK(cudaEventRecord(t1)); CK(cudaEventSynchronize(t1));
    CK(cudaGetLastError());
    float ms=0; CK(cudaEventElapsedTime(&ms,t0,t1));
    printf("[timed] FULL-PIPE-TILE (grouped ANS+match, WHOLE FILE) G=%d: %.3f ms -> %.1f GB/s\n",
        G, ms, hdr.orig_size/(ms*1e-3)/1e9);

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
    tile_free(TL); tile_free(TO); tile_free(TN); tile_free(TC);
    cudaStreamDestroy(s2);
    return 0;
}
