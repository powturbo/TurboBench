// =============================================================================
// full_gpu_decode_v2.cu  —  ACEAPEX device-resident GPU decode (GPU levels)
//
// v2 changes vs v1:
//   [3] LEVELS  : moved to GPU. Levels are per-block independent (blocks decode
//                 independently => a match's source is within its own block),
//                 so one thread per block reproduces the CPU forward-pass.
//   bucket-by-level: also on GPU (count -> host prefix-sum(tiny) -> scatter).
//   Timed region now spans the whole device path: parse + levels + bucket + match.
//
// HONEST CAVEATS (unchanged): entropy still comes from the step0 streams.bin dump
//   (raw lit/off/len/cmd after FSE), NOT yet nvcomp. Two small prefix-sums remain
//   on host: O(num_blocks) and O(MaxLevel) — negligible metadata, not the data path.
//   The k_levels kernel is intentionally simple (1 thread/block); expect it to be
//   the new bottleneck — correctness first, occupancy optimization next.
// =============================================================================
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cuda_runtime.h>
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

struct Token { uint32_t pos, src, len; };

__device__ __forceinline__ uint32_t d_read_varint(const uint8_t* buf, uint32_t& p, uint32_t limit){
    uint32_t val=0, shift=0;
    while(p<limit){ uint8_t b=buf[p++]; val|=(uint32_t)(b&0x7F)<<shift; if(!(b&0x80)) return val; shift+=7; }
    return val;
}

// ---- [2] PARSE — one thread per block ----
template<bool COUNT_ONLY>
__global__ void k_parse(
    const uint8_t* LIT, const uint8_t* OFF, const uint8_t* LEN, const uint8_t* CMD,
    const BlockOffsets* boffs, uint32_t num_blocks,
    uint64_t orig_size, uint32_t block_size,
    uint8_t* d_out, Token* d_tokens, const uint32_t* d_tokoff, uint32_t* d_count)
{
    uint32_t b = blockIdx.x*blockDim.x + threadIdx.x;
    if(b>=num_blocks) return;
    BlockOffsets bo = boffs[b];
    const uint8_t* lit = LIT + bo.lit_off;
    const uint8_t* off = OFF + bo.off_off;
    const uint8_t* len = LEN + bo.len_off;
    const uint8_t* cmd = CMD + bo.cmd_off;
    uint32_t lit_sz=bo.lit_sz, off_sz=bo.off_sz, len_sz=bo.len_sz, cmd_sz=bo.cmd_sz;
    uint64_t base = (uint64_t)b * block_size;
    uint64_t dst_size = (orig_size > base) ? ((orig_size-base)<(uint64_t)block_size?(orig_size-base):(uint64_t)block_size) : 0;
    uint32_t lp=0, op=0, np=0, cp=0, out=0;
    uint32_t rep[4]={1,2,4,8};
    uint32_t tok = COUNT_ONLY ? 0u : d_tokoff[b];
    while(out<dst_size && cp<cmd_sz){
        uint8_t c=cmd[cp++];
        if(c==0xFF){ rep[0]=1;rep[1]=2;rep[2]=4;rep[3]=8; continue; }
        if(c<0x80){
            uint32_t l=c+1;
            if(lp+l>lit_sz || out+l>dst_size) break;
            if(!COUNT_ONLY) for(uint32_t i=0;i<l;i++) d_out[base+out+i]=lit[lp+i];
            out+=l; lp+=l;
        } else if((c&0xC0)==0x80){
            uint32_t ri=(c>>4)&3, lv=c&0x0F;
            if(lv==0x0F) lv+=d_read_varint(len,np,len_sz);
            uint32_t l=lv+6, dist=rep[ri];
            if(ri>0){ for(int i=ri;i>0;i--) rep[i]=rep[i-1]; rep[0]=dist; }
            if(!dist || out+l>dst_size) break;
            if(!COUNT_ONLY) d_tokens[tok]={(uint32_t)(base+out),(uint32_t)(base+out-dist),l};
            tok++; out+=l;
        } else {
            uint32_t lv=(c==0xFE)? d_read_varint(len,np,len_sz) : (uint32_t)(c&0x3F);
            uint32_t l=lv+6, dist=d_read_varint(off,op,off_sz);
            rep[3]=rep[2];rep[2]=rep[1];rep[1]=rep[0];rep[0]=dist;
            if(!dist || out+l>dst_size) break;
            if(!COUNT_ONLY) d_tokens[tok]={(uint32_t)(base+out),(uint32_t)(base+out-dist),l};
            tok++; out+=l;
        }
    }
    if(COUNT_ONLY) d_count[b]=tok;
}

// ---- [3] LEVELS — one thread per block; per-block forward-pass (mirrors host) ----
//   tokof[] = global scratch, init to -1; block b owns range [b*bs, b*bs+bsize).
//   level[ti] = 1 + max level of tokens covering [src,src+len); literals = level 0.
__global__ void k_levels(const Token* tk, const uint32_t* tokoff, uint32_t num_blocks,
                         uint64_t orig_size, uint32_t block_size,
                         int32_t* tokof, int32_t* lev, int* maxlev){
    uint32_t b=blockIdx.x*blockDim.x+threadIdx.x; if(b>=num_blocks) return;
    uint32_t ts=tokoff[b], te=tokoff[b+1];
    // build tok_of for this block (ascending ti => last covering token wins)
    for(uint32_t ti=ts; ti<te; ti++){
        Token t=tk[ti];
        for(uint32_t i=0;i<t.len;i++){ uint64_t p=(uint64_t)t.pos+i; if(p<orig_size) tokof[p]=(int32_t)ti; }
    }
    int localmax=0;
    for(uint32_t ti=ts; ti<te; ti++){
        Token t=tk[ti];
        int mx=0; uint32_t e=t.src+t.len, pp=t.src;
        while(pp<e){
            int st=tokof[pp];
            if(st>=0){ int lv=lev[st]+1; if(lv>mx)mx=lv; uint32_t nx=tk[st].pos+tk[st].len; pp=(nx>pp)?nx:pp+1; }
            else pp++;
        }
        lev[ti]=mx; if(mx>localmax)localmax=mx;
    }
    atomicMax(maxlev, localmax);
}

// ---- bucket helpers ----
__global__ void k_count(const int32_t* lev, uint32_t n, uint32_t* cnt){
    uint32_t i=blockIdx.x*blockDim.x+threadIdx.x; if(i>=n)return;
    atomicAdd(&cnt[lev[i]],1u);
}
__global__ void k_scatter(const int32_t* lev, uint32_t n, uint32_t* cur, uint32_t* ord){
    uint32_t i=blockIdx.x*blockDim.x+threadIdx.x; if(i>=n)return;
    uint32_t p=atomicAdd(&cur[lev[i]],1u); ord[p]=i;
}

// ---- [4] MATCH — wavefront (one launch per level) ----
__global__ void k_match(uint8_t* out, const Token* tk, const uint32_t* ord,
                        uint32_t base, uint32_t cnt){
    uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;
    if(i>=cnt) return;
    Token t = tk[ ord[base+i] ];
    for(uint32_t k=0;k<t.len;k++) out[t.pos+k]=out[t.src+k];
}

// ---- [5] VERIFY — FNV-1a on device (single thread) ----
__global__ void k_hash(const uint8_t* buf, size_t n, uint64_t* out){
    if(blockIdx.x==0&&threadIdx.x==0){
        uint64_t h=0xcbf29ce484222325ULL;
        for(size_t i=0;i<n;i++) h=(h^buf[i])*0x100000001b3ULL;
        *out=h;
    }
}

int main(int argc, char** argv){
    if(argc<2){ fprintf(stderr,"Usage: %s <streams.bin> [original_file]\n",argv[0]); return 1; }
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
    printf("orig=%llu blocks=%u block_size=%u  rawL/O/N/C=%.1f/%.1f/%.1f/%.1f MB\n",
        (unsigned long long)hdr.orig_size, nb, hdr.block_size, totL/1e6,totO/1e6,totN/1e6,totC/1e6);

    // ---- upload (one-time; not timed) ----
    uint8_t *dLIT,*dOFF,*dLEN,*dCMD,*dOUT; BlockOffsets* dBO;
    CK(cudaMalloc(&dLIT,totL)); CK(cudaMalloc(&dOFF,totO));
    CK(cudaMalloc(&dLEN,totN)); CK(cudaMalloc(&dCMD,totC));
    CK(cudaMalloc(&dOUT,hdr.orig_size)); CK(cudaMalloc(&dBO,nb*sizeof(BlockOffsets)));
    CK(cudaMemcpy(dLIT,LIT.data(),totL,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dOFF,OFF.data(),totO,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dLEN,LEN.data(),totN,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dCMD,CMD.data(),totC,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dBO,boffs.data(),nb*sizeof(BlockOffsets),cudaMemcpyHostToDevice));

    // ---- scratch (pre-allocated; not timed) ----
    int32_t *dTokOf; int* dMaxLev;
    CK(cudaMalloc(&dTokOf,hdr.orig_size*sizeof(int32_t)));
    CK(cudaMalloc(&dMaxLev,sizeof(int)));
    uint32_t *dCount,*dTokoff;
    CK(cudaMalloc(&dCount,nb*sizeof(uint32_t)));
    CK(cudaMalloc(&dTokoff,(nb+1)*sizeof(uint32_t)));
    const int MAXLV=1<<20;
    uint32_t *dLcnt,*dCur;
    CK(cudaMalloc(&dLcnt,MAXLV*sizeof(uint32_t)));
    CK(cudaMalloc(&dCur, MAXLV*sizeof(uint32_t)));

    cudaEvent_t t0,t1; CK(cudaEventCreate(&t0)); CK(cudaEventCreate(&t1));
    int TPB=128, grid=(nb+TPB-1)/TPB;

    // ===================== TIMED DEVICE-RESIDENT REGION ====================
    CK(cudaEventRecord(t0));

    // [2] PARSE: count -> host prefix-sum (O(blocks)) -> emit
    k_parse<true><<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,nullptr,nullptr,nullptr,dCount);
    CK(cudaDeviceSynchronize());
    vector<uint32_t> cnt(nb),tokoff(nb+1,0);
    CK(cudaMemcpy(cnt.data(),dCount,nb*sizeof(uint32_t),cudaMemcpyDeviceToHost));
    for(uint32_t b=0;b<nb;b++) tokoff[b+1]=tokoff[b]+cnt[b];
    uint32_t ntok=tokoff[nb];
    CK(cudaMemcpy(dTokoff,tokoff.data(),(nb+1)*sizeof(uint32_t),cudaMemcpyHostToDevice));
    Token* dTok; CK(cudaMalloc(&dTok,(size_t)ntok*sizeof(Token)));
    k_parse<false><<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,dOUT,dTok,dTokoff,nullptr);
    CK(cudaDeviceSynchronize());

    // [3] LEVELS on GPU (per-block)
    CK(cudaMemset(dTokOf,0xFF,hdr.orig_size*sizeof(int32_t)));   // init to -1
    CK(cudaMemset(dMaxLev,0,sizeof(int)));
    int32_t* dLev; CK(cudaMalloc(&dLev,(size_t)ntok*sizeof(int32_t)));
    k_levels<<<grid,TPB>>>(dTok,dTokoff,nb,hdr.orig_size,hdr.block_size,dTokOf,dLev,dMaxLev);
    CK(cudaDeviceSynchronize());
    int ml=0; CK(cudaMemcpy(&ml,dMaxLev,sizeof(int),cudaMemcpyDeviceToHost));
    if(ml+1>MAXLV){ printf("MaxLevel %d exceeds MAXLV\n",ml); return 1; }

    // bucket by level: count -> host prefix-sum (O(levels)) -> scatter
    CK(cudaMemset(dLcnt,0,(ml+1)*sizeof(uint32_t)));
    int TPBn=256, gridn=(ntok+TPBn-1)/TPBn;
    k_count<<<gridn,TPBn>>>(dLev,ntok,dLcnt);
    CK(cudaDeviceSynchronize());
    vector<uint32_t> lcnt(ml+1); CK(cudaMemcpy(lcnt.data(),dLcnt,(ml+1)*sizeof(uint32_t),cudaMemcpyDeviceToHost));
    vector<uint32_t> loff(ml+2,0); for(int L=0;L<=ml;L++) loff[L+1]=loff[L]+lcnt[L];
    CK(cudaMemcpy(dCur,loff.data(),(ml+1)*sizeof(uint32_t),cudaMemcpyHostToDevice));
    uint32_t* dOrd; CK(cudaMalloc(&dOrd,(size_t)ntok*sizeof(uint32_t)));
    k_scatter<<<gridn,TPBn>>>(dLev,ntok,dCur,dOrd);
    CK(cudaDeviceSynchronize());

    // [4] MATCH wavefront — one launch per level
    for(int L=0;L<=ml;L++){
        uint32_t base=loff[L], c=loff[L+1]-loff[L];
        if(c) k_match<<<(c+255)/256,256>>>(dOUT,dTok,dOrd,base,c);
    }
    CK(cudaEventRecord(t1)); CK(cudaEventSynchronize(t1));
    // =======================================================================

    float ms=0; CK(cudaEventElapsedTime(&ms,t0,t1));
    printf("[2] parsed ntok=%u\n",ntok);
    printf("[3] MaxLevel=%d\n",ml);
    printf("[timed] device-resident decode: %.2f ms  -> %.1f GB/s\n", ms, hdr.orig_size/(ms*1e-3)/1e9);

    // [5] VERIFY
    uint64_t *dH,h=0; CK(cudaMalloc(&dH,8));
    k_hash<<<1,1>>>(dOUT,hdr.orig_size,dH); CK(cudaMemcpy(&h,dH,8,cudaMemcpyDeviceToHost));
    printf("[5] out FNV=%016llx\n",(unsigned long long)h);
    if(argc>2){
        FILE* fo=fopen(argv[2],"rb");
        if(fo){ vector<uint8_t> orig(hdr.orig_size);
            if(fread(orig.data(),1,hdr.orig_size,fo)!=hdr.orig_size) fprintf(stderr,"short orig\n");
            fclose(fo);
            uint64_t ho=0xcbf29ce484222325ULL; for(uint8_t b:orig) ho=(ho^b)*0x100000001b3ULL;
            printf("    orig FNV=%016llx  %s\n",(unsigned long long)ho, ho==h?"MATCHES OK":"DIFFERS X"); }
    }
    return 0;
}
