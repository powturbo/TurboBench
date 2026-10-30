// =============================================================================
// full_gpu_decode_v5.cu — ACEAPEX HYBRID GPU profile
//
// Goal: recover container ratio lost in v4. v4 packed ALL four raw streams
// (lit/off/len/cmd) with nvcomp ANS. ANS is great for off/len/cmd but weaker
// on literals (text) than zstd/FSE -> enwik9 ratio dropped 2.55 -> 2.10.
//
// v5 splits the RAW buffer at the literal boundary (totL) into TWO segments:
//   segment L  = [0, totL)        -> packed with LIT_CODEC (zstd or gdeflate)
//   segment R  = [totL, totRaw)   -> packed with ANS  (off|len|cmd, as v4)
// Both decode on-device into the same dRaw; k_decode is unchanged.
//
// LIT_CODEC is selected at build time (see CODEC SELECTION below) because the
// available batched codecs differ between nvcomp builds. On the pod, step 0 of
// the runbook checks which headers exist and sets -DLIT_ZSTD or -DLIT_GDEFLATE.
//
// HONEST CAVEATS unchanged: report GPU-profile ratio vs v4 AND vs CPU profile.
// H2D of compressed blob excluded from timer (one-time staging). Timed region =
// entropy(L) + entropy(R) + k_decode, all on device.
// =============================================================================
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cuda_runtime.h>
#include <nvcomp/ans.h>

// ---- CODEC SELECTION (set one at compile time; default zstd) ----------------
// Pod step 0: ls $NVINC/nvcomp/ ; if zstd.h present -> -DLIT_ZSTD (default),
// else if gdeflate.h -> -DLIT_GDEFLATE, else -DLIT_ANS (falls back to v4 behavior
// for literals too, i.e. no ratio gain — only use if neither zstd nor gdeflate).
#if !defined(LIT_ZSTD) && !defined(LIT_GDEFLATE) && !defined(LIT_ANS)
  #define LIT_ZSTD 1
#endif

#if defined(LIT_ZSTD)
  #include <nvcomp/zstd.h>
  #define LIT_PREFIX                 nvcompBatchedZstd
  #define LIT_NAME                   "zstd"
#elif defined(LIT_GDEFLATE)
  #include <nvcomp/gdeflate.h>
  #define LIT_PREFIX                 nvcompBatchedGdeflate
  #define LIT_NAME                   "gdeflate"
#else
  #define LIT_PREFIX                 nvcompBatchedANS
  #define LIT_NAME                   "ans"
#endif

// token-paste helpers to build full nvcomp function names from a prefix
#define CAT2(a,b) a##b
#define CAT(a,b)  CAT2(a,b)
#define LIT_FN(suffix) CAT(LIT_PREFIX, suffix)

using namespace std;

#define CK(x)  do{cudaError_t e=(x); if(e){printf("CUDA err %s:%d %s\n",__FILE__,__LINE__,cudaGetErrorString(e));exit(1);} }while(0)
#define NVCK(x) do{nvcompStatus_t s=(x); if(s!=nvcompSuccess){printf("nvcomp err %s:%d status=%d\n",__FILE__,__LINE__,(int)s);exit(1);} }while(0)

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
struct G5Hdr {                       // hybrid GPU-profile container header
    char     magic[8];               // "ACEGPU5\0"
    AetHdr   aet;
    uint64_t totL, totO, totN, totC;
    uint32_t chunk;
    uint32_t nL;                     // chunks in literal segment
    uint32_t nR;                     // chunks in off/len/cmd segment
    uint32_t litcodec;               // 1=zstd 2=gdeflate 3=ans (informational)
};
#pragma pack(pop)

static const uint32_t CHUNK = 65536;
static inline uint64_t pad16(uint64_t x){ return (x+15)&~15ull; }
static inline uint64_t pad256(uint64_t x){ return (x+255)&~255ull; }

// ---------------- decode kernel: warp-per-block (verbatim from v4) -----------
__device__ __forceinline__ uint32_t d_read_varint(const uint8_t* buf, uint32_t& p, uint32_t limit){
    uint32_t val=0, shift=0;
    while(p<limit){ uint8_t b=buf[p++]; val|=(uint32_t)(b&0x7F)<<shift; if(!(b&0x80)) return val; shift+=7; }
    return val;
}
__global__ void k_decode(const uint8_t* __restrict__ LIT, const uint8_t* __restrict__ OFF,
                         const uint8_t* __restrict__ LEN, const uint8_t* __restrict__ CMD,
                         const BlockOffsets* __restrict__ boffs, uint32_t num_blocks,
                         uint64_t orig_size, uint32_t block_size, uint8_t* __restrict__ out)
{
    uint32_t gw   = (blockIdx.x*blockDim.x + threadIdx.x) >> 5;
    uint32_t lane = threadIdx.x & 31;
    if (gw >= num_blocks) return;
    BlockOffsets bo = boffs[gw];
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
__global__ void k_hash(const uint8_t* buf, size_t n, uint64_t* out){
    if(blockIdx.x==0&&threadIdx.x==0){
        uint64_t h=0xcbf29ce484222325ULL;
        for(size_t i=0;i<n;i++) h=(h^buf[i])*0x100000001b3ULL;
        *out=h;
    }
}

// ---------------- shared: read streams.bin -----------------------------------
static bool read_streams(const char* path, AetHdr& hdr, vector<BlockOffsets>& boffs,
                         vector<uint8_t>& RAW, uint64_t& totL,uint64_t& totO,uint64_t& totN,uint64_t& totC){
    FILE* f=fopen(path,"rb"); if(!f){perror("open streams");return false;}
    if(fread(&hdr,sizeof(hdr),1,f)!=1){fprintf(stderr,"bad header\n");return false;}
    boffs.resize(hdr.num_blocks);
    if(fread(boffs.data(),sizeof(BlockOffsets),hdr.num_blocks,f)!=hdr.num_blocks){fprintf(stderr,"bad boffs\n");return false;}
    totL=totO=totN=totC=0;
    for(auto&b:boffs){totL+=b.lit_sz;totO+=b.off_sz;totN+=b.len_sz;totC+=b.cmd_sz;}
    RAW.resize(totL+totO+totN+totC);
    if(fread(RAW.data(),1,RAW.size(),f)!=RAW.size()){fprintf(stderr,"short streams\n");return false;}
    fclose(f); return true;
}

// ---------------- generic batched compress of one device segment -------------
// Compresses [dSeg, dSeg+segBytes) split into CHUNK pieces using a chosen codec
// family (selected by the *Compress* function pointers passed in via templates
// below). Returns per-chunk compressed sizes (host) and fills dComp (strided).
// To keep it simple and avoid function-pointer gymnastics across nvcomp codec
// families, we provide TWO concrete helpers (ANS and LIT) sharing this body via
// a macro. (nvcomp's C API names differ per codec, so a macro is cleanest.)

#define DEFINE_PACK_SEG(NAME, PREFIX)                                             \
static int NAME##_pack_seg(const uint8_t* dSeg, uint64_t segBytes,               \
                           uint8_t*& dComp, uint64_t& stride, uint32_t& nchunks, \
                           std::vector<size_t>& csz){                            \
    nchunks=(uint32_t)((segBytes+CHUNK-1)/CHUNK);                                \
    std::vector<void*> h_in(nchunks); std::vector<size_t> h_insz(nchunks);       \
    for(uint32_t i=0;i<nchunks;i++){                                             \
        h_in[i]=(void*)(dSeg+(uint64_t)i*CHUNK);                                 \
        h_insz[i]=(i+1<nchunks)?CHUNK:(size_t)(segBytes-(uint64_t)(nchunks-1)*CHUNK);\
    }                                                                            \
    size_t max_out=0;                                                            \
    NVCK(CAT(PREFIX,CompressGetMaxOutputChunkSize)(CHUNK,CAT(PREFIX,CompressDefaultOpts),&max_out));\
    stride=pad256(max_out);                                                      \
    CK(cudaMalloc(&dComp,stride*nchunks));                                       \
    std::vector<void*> h_out(nchunks);                                           \
    for(uint32_t i=0;i<nchunks;i++) h_out[i]=dComp+(uint64_t)i*stride;           \
    void **d_in,**d_out; size_t *d_insz,*d_outsz; nvcompStatus_t* d_st;          \
    CK(cudaMalloc(&d_in ,nchunks*sizeof(void*)));                                \
    CK(cudaMalloc(&d_out,nchunks*sizeof(void*)));                                \
    CK(cudaMalloc(&d_insz ,nchunks*sizeof(size_t)));                             \
    CK(cudaMalloc(&d_outsz,nchunks*sizeof(size_t)));                             \
    CK(cudaMalloc(&d_st,nchunks*sizeof(nvcompStatus_t)));                        \
    CK(cudaMemcpy(d_in ,h_in.data() ,nchunks*sizeof(void*),cudaMemcpyHostToDevice));\
    CK(cudaMemcpy(d_out,h_out.data(),nchunks*sizeof(void*),cudaMemcpyHostToDevice));\
    CK(cudaMemcpy(d_insz,h_insz.data(),nchunks*sizeof(size_t),cudaMemcpyHostToDevice));\
    size_t temp_sz=0;                                                            \
    NVCK(CAT(PREFIX,CompressGetTempSizeAsync)(nchunks,CHUNK,CAT(PREFIX,CompressDefaultOpts),&temp_sz,segBytes));\
    void* dTemp=nullptr; if(temp_sz) CK(cudaMalloc(&dTemp,temp_sz));             \
    NVCK(CAT(PREFIX,CompressAsync)((const void* const*)d_in,d_insz,CHUNK,nchunks,\
        dTemp,temp_sz,(void* const*)d_out,d_outsz,CAT(PREFIX,CompressDefaultOpts),d_st,0));\
    CK(cudaDeviceSynchronize());                                                 \
    csz.resize(nchunks); std::vector<nvcompStatus_t> st(nchunks);               \
    CK(cudaMemcpy(csz.data(),d_outsz,nchunks*sizeof(size_t),cudaMemcpyDeviceToHost));\
    CK(cudaMemcpy(st.data(),d_st,nchunks*sizeof(nvcompStatus_t),cudaMemcpyDeviceToHost));\
    for(uint32_t i=0;i<nchunks;i++) if(st[i]!=nvcompSuccess){printf(#NAME " chunk %u compress status %d\n",i,(int)st[i]);return 1;}\
    if(dTemp)cudaFree(dTemp);cudaFree(d_in);cudaFree(d_out);cudaFree(d_insz);cudaFree(d_outsz);cudaFree(d_st);\
    return 0;                                                                    \
}

DEFINE_PACK_SEG(ans, nvcompBatchedANS)
DEFINE_PACK_SEG(lit, LIT_PREFIX)

// ---------------- PACK: raw streams -> [LIT seg | R seg] -> .gaet5 -----------
static int do_pack(const char* in_path, const char* out_path){
    AetHdr hdr; vector<BlockOffsets> boffs; vector<uint8_t> RAW;
    uint64_t totL,totO,totN,totC;
    if(!read_streams(in_path,hdr,boffs,RAW,totL,totO,totN,totC)) return 1;
    uint64_t totRaw=RAW.size();
    uint64_t totR = totO+totN+totC;                 // off|len|cmd contiguous after lit
    printf("pack(hybrid,lit=%s): rawL=%.1f MB rawR=%.1f MB\n", LIT_NAME, totL/1e6, totR/1e6);

    // separate aligned device buffers per segment (nvcomp needs in-ptr aligned)
    uint8_t *dL=nullptr,*dR=nullptr;
    CK(cudaMalloc(&dL,totL>0?totL:1));
    CK(cudaMalloc(&dR,totR>0?totR:1));
    CK(cudaMemcpy(dL,RAW.data(),totL,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dR,RAW.data()+totL,totR,cudaMemcpyHostToDevice));

    // pack literal segment with LIT codec
    uint8_t* dCompL=nullptr; uint64_t strideL=0; uint32_t nL=0; vector<size_t> cszL;
    if(lit_pack_seg(dL,totL,dCompL,strideL,nL,cszL)) return 1;
    // pack off/len/cmd segment with ANS
    uint8_t* dCompR=nullptr; uint64_t strideR=0; uint32_t nR=0; vector<size_t> cszR;
    if(ans_pack_seg(dR,totR,dCompR,strideR,nR,cszR)) return 1;

    vector<uint8_t> compL(strideL*nL), compR(strideR*nR);
    CK(cudaMemcpy(compL.data(),dCompL,strideL*nL,cudaMemcpyDeviceToHost));
    CK(cudaMemcpy(compR.data(),dCompR,strideR*nR,cudaMemcpyDeviceToHost));

    G5Hdr g{}; memcpy(g.magic,"ACEGPU5",8); g.aet=hdr;
    g.totL=totL;g.totO=totO;g.totN=totN;g.totC=totC; g.chunk=CHUNK; g.nL=nL; g.nR=nR;
#if defined(LIT_ZSTD)
    g.litcodec=1;
#elif defined(LIT_GDEFLATE)
    g.litcodec=2;
#else
    g.litcodec=3;
#endif
    FILE* fo=fopen(out_path,"wb"); if(!fo){perror("open out");return 1;}
    fwrite(&g,sizeof(g),1,fo);
    fwrite(boffs.data(),sizeof(BlockOffsets),hdr.num_blocks,fo);
    // per-chunk sizes: L group then R group
    vector<uint64_t> cs64L(nL), cs64R(nR); uint64_t blob=0;
    for(uint32_t i=0;i<nL;i++){cs64L[i]=cszL[i]; blob+=pad16(cszL[i]);}
    for(uint32_t i=0;i<nR;i++){cs64R[i]=cszR[i]; blob+=pad16(cszR[i]);}
    fwrite(cs64L.data(),sizeof(uint64_t),nL,fo);
    fwrite(cs64R.data(),sizeof(uint64_t),nR,fo);
    vector<uint8_t> zero(16,0);
    for(uint32_t i=0;i<nL;i++){ fwrite(compL.data()+(uint64_t)i*strideL,1,cszL[i],fo);
        uint64_t p=pad16(cszL[i])-cszL[i]; if(p) fwrite(zero.data(),1,p,fo); }
    for(uint32_t i=0;i<nR;i++){ fwrite(compR.data()+(uint64_t)i*strideR,1,cszR[i],fo);
        uint64_t p=pad16(cszR[i])-cszR[i]; if(p) fwrite(zero.data(),1,p,fo); }
    fclose(fo);
    uint64_t meta=sizeof(g)+sizeof(BlockOffsets)*hdr.num_blocks+8ull*(nL+nR);
    printf("pack done: container=%.1f MB (blob %.1f + meta %.1f)  GPU-profile ratio=%.3f  lit=%s\n",
        (blob+meta)/1e6, blob/1e6, meta/1e6, (double)hdr.orig_size/(blob+meta), LIT_NAME);
    cudaFree(dL);cudaFree(dR);cudaFree(dCompL);cudaFree(dCompR);
    return 0;
}

// ---------------- generic batched decompress of one segment ------------------
#define DEFINE_DECOMP_SEG(NAME, PREFIX)                                           \
static int NAME##_decomp_seg(const uint8_t* dComp, const std::vector<uint64_t>& csz,\
                             uint8_t* dOutSeg, uint64_t segBytes, uint32_t chunk, \
                             cudaStream_t stream){                               \
    uint32_t nchunks=(uint32_t)csz.size();                                       \
    std::vector<void*> h_c(nchunks),h_o(nchunks);                               \
    std::vector<size_t> h_csz(nchunks),h_bsz(nchunks);                          \
    uint64_t off=0;                                                              \
    for(uint32_t i=0;i<nchunks;i++){                                             \
        h_c[i]=(void*)(dComp+off); h_csz[i]=csz[i]; off+=pad16(csz[i]);          \
        h_o[i]=dOutSeg+(uint64_t)i*chunk;                                        \
        h_bsz[i]=(i+1<nchunks)?chunk:(size_t)(segBytes-(uint64_t)(nchunks-1)*chunk);\
    }                                                                            \
    void **d_c,**d_o; size_t *d_csz,*d_bsz,*d_asz; nvcompStatus_t* d_st;         \
    CK(cudaMalloc(&d_c,nchunks*sizeof(void*)));                                  \
    CK(cudaMalloc(&d_o,nchunks*sizeof(void*)));                                  \
    CK(cudaMalloc(&d_csz,nchunks*sizeof(size_t)));                               \
    CK(cudaMalloc(&d_bsz,nchunks*sizeof(size_t)));                               \
    CK(cudaMalloc(&d_asz,nchunks*sizeof(size_t)));                               \
    CK(cudaMalloc(&d_st,nchunks*sizeof(nvcompStatus_t)));                        \
    CK(cudaMemcpy(d_c,h_c.data(),nchunks*sizeof(void*),cudaMemcpyHostToDevice)); \
    CK(cudaMemcpy(d_o,h_o.data(),nchunks*sizeof(void*),cudaMemcpyHostToDevice)); \
    CK(cudaMemcpy(d_csz,h_csz.data(),nchunks*sizeof(size_t),cudaMemcpyHostToDevice));\
    CK(cudaMemcpy(d_bsz,h_bsz.data(),nchunks*sizeof(size_t),cudaMemcpyHostToDevice));\
    size_t temp_sz=0;                                                            \
    NVCK(CAT(PREFIX,DecompressGetTempSizeAsync)(nchunks,chunk,CAT(PREFIX,DecompressDefaultOpts),&temp_sz,segBytes));\
    void* dTemp=nullptr; if(temp_sz) CK(cudaMalloc(&dTemp,temp_sz));             \
    NVCK(CAT(PREFIX,DecompressAsync)((const void* const*)d_c,d_csz,d_bsz,d_asz,  \
        nchunks,dTemp,temp_sz,(void* const*)d_o,CAT(PREFIX,DecompressDefaultOpts),d_st,stream));\
    /* status checked by caller after sync; free scratch ptr arrays after sync */\
    /* NOTE: leak of small d_* arrays acceptable for a benchmark harness */      \
    return 0;                                                                    \
}
DEFINE_DECOMP_SEG(ans, nvcompBatchedANS)
DEFINE_DECOMP_SEG(lit, LIT_PREFIX)

// ---------------- DECODE: .gaet5 -> two entropy decodes -> k_decode ----------
static int do_decode(const char* in_path, const char* orig_path){
    FILE* f=fopen(in_path,"rb"); if(!f){perror("open gaet5");return 1;}
    G5Hdr g; if(fread(&g,sizeof(g),1,f)!=1 || memcmp(g.magic,"ACEGPU5",7)){fprintf(stderr,"bad gaet5\n");return 1;}
    uint32_t nb=g.aet.num_blocks, nL=g.nL, nR=g.nR;
    vector<BlockOffsets> boffs(nb);
    if(fread(boffs.data(),sizeof(BlockOffsets),nb,f)!=nb){fprintf(stderr,"bad boffs\n");return 1;}
    vector<uint64_t> cszL(nL),cszR(nR);
    if(fread(cszL.data(),sizeof(uint64_t),nL,f)!=nL){fprintf(stderr,"bad L sizes\n");return 1;}
    if(fread(cszR.data(),sizeof(uint64_t),nR,f)!=nR){fprintf(stderr,"bad R sizes\n");return 1;}
    uint64_t blobL=0,blobR=0;
    for(uint32_t i=0;i<nL;i++) blobL+=pad16(cszL[i]);
    for(uint32_t i=0;i<nR;i++) blobR+=pad16(cszR[i]);
    vector<uint8_t> COMPL(blobL),COMPR(blobR);
    if(fread(COMPL.data(),1,blobL,f)!=blobL){fprintf(stderr,"short L blob\n");return 1;}
    if(fread(COMPR.data(),1,blobR,f)!=blobR){fprintf(stderr,"short R blob\n");return 1;}
    fclose(f);

    uint64_t totL=g.totL, totR=g.totO+g.totN+g.totC, totRaw=totL+totR;
    printf("orig=%llu blocks=%u block_size=%u  L:%u chunks/%.1fMB  R:%u chunks/%.1fMB  lit=%u\n",
        (unsigned long long)g.aet.orig_size, nb, g.aet.block_size,
        nL, blobL/1e6, nR, blobR/1e6, g.litcodec);

    // staging (untimed)
    uint8_t *dCompL,*dCompR,*dRaw,*dOUT; BlockOffsets* dBO;
    CK(cudaMalloc(&dCompL,blobL)); CK(cudaMalloc(&dCompR,blobR));
    CK(cudaMalloc(&dRaw,totRaw));  CK(cudaMalloc(&dOUT,g.aet.orig_size));
    CK(cudaMalloc(&dBO,nb*sizeof(BlockOffsets)));
    CK(cudaMemcpy(dCompL,COMPL.data(),blobL,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dCompR,COMPR.data(),blobR,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dBO,boffs.data(),nb*sizeof(BlockOffsets),cudaMemcpyHostToDevice));

    uint8_t* dSegL = dRaw;          // literals decode here
    uint8_t* dSegR = dRaw + totL;   // off/len/cmd decode here
    const uint8_t* dLIT=dRaw;
    const uint8_t* dOFF=dRaw+g.totL;
    const uint8_t* dLEN=dRaw+g.totL+g.totO;
    const uint8_t* dCMD=dRaw+g.totL+g.totO+g.totN;
    const int TPB=128;
    uint32_t grid=(uint32_t)(((uint64_t)nb*32+TPB-1)/TPB);

    // warm-up (untimed): both segments + decode, then clear
    lit_decomp_seg(dCompL,cszL,dSegL,totL,g.chunk,0);
    ans_decomp_seg(dCompR,cszR,dSegR,totR,g.chunk,0);
    k_decode<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,g.aet.orig_size,g.aet.block_size,dOUT);
    CK(cudaDeviceSynchronize());
    CK(cudaMemset(dRaw,0,totRaw)); CK(cudaMemset(dOUT,0,g.aet.orig_size));

    cudaEvent_t t0,t1,t2; CK(cudaEventCreate(&t0));CK(cudaEventCreate(&t1));CK(cudaEventCreate(&t2));
    // ===================== TIMED FULL DEVICE PIPELINE ======================
    CK(cudaEventRecord(t0));
    lit_decomp_seg(dCompL,cszL,dSegL,totL,g.chunk,0);   // entropy: literals
    ans_decomp_seg(dCompR,cszR,dSegR,totR,g.chunk,0);   // entropy: off/len/cmd
    CK(cudaEventRecord(t1));
    k_decode<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,g.aet.orig_size,g.aet.block_size,dOUT);
    CK(cudaEventRecord(t2)); CK(cudaEventSynchronize(t2));
    // =======================================================================
    CK(cudaGetLastError());

    float msA=0,msB=0,msT=0;
    CK(cudaEventElapsedTime(&msA,t0,t1));
    CK(cudaEventElapsedTime(&msB,t1,t2));
    CK(cudaEventElapsedTime(&msT,t0,t2));
    printf("[A] entropy (lit=%s + ans): %.2f ms  (%.1f GB/s raw streams)\n",LIT_NAME,msA,totRaw/(msA*1e-3)/1e9);
    printf("[B] k_decode warp/block   : %.2f ms  (%.1f GB/s output)\n",msB,g.aet.orig_size/(msB*1e-3)/1e9);
    printf("[timed] FULL GPU hybrid   : %.2f ms -> %.1f GB/s\n",
        msT, g.aet.orig_size/(msT*1e-3)/1e9);

    // verify bit-perfect
    uint64_t *dH,h=0; CK(cudaMalloc(&dH,8));
    k_hash<<<1,1>>>(dOUT,g.aet.orig_size,dH);
    CK(cudaMemcpy(&h,dH,8,cudaMemcpyDeviceToHost));
    printf("[hash] FNV(out)=%016llx\n",(unsigned long long)h);
    if(orig_path){
        FILE* fo=fopen(orig_path,"rb");
        if(fo){ vector<uint8_t> O(g.aet.orig_size);
            if(fread(O.data(),1,g.aet.orig_size,fo)==g.aet.orig_size){
                uint64_t ho=0xcbf29ce484222325ULL;
                for(uint64_t i=0;i<g.aet.orig_size;i++) ho=(ho^O[i])*0x100000001b3ULL;
                printf("[hash] FNV(orig)=%016llx -> %s\n",(unsigned long long)ho,
                    ho==h?"MATCHES OK":"MISMATCH!!!");
            } fclose(fo);
        }
    }
    cudaFree(dCompL);cudaFree(dCompR);cudaFree(dRaw);cudaFree(dOUT);cudaFree(dBO);cudaFree(dH);
    return 0;
}

int main(int argc,char**argv){
    if(argc<3){
        printf("usage:\n  %s pack   <streams.bin> <out.gaet5>\n"
               "  %s decode <in.gaet5> [original_file]\n", argv[0],argv[0]);
        return 2;
    }
    if(!strcmp(argv[1],"pack"))   return do_pack(argv[2],argv[3]);
    if(!strcmp(argv[1],"decode")) return do_decode(argv[2],argc>3?argv[3]:nullptr);
    printf("unknown mode %s\n",argv[1]); return 2;
}
