// =============================================================================
// full_gpu_decode.cu  —  ACEAPEX full device-resident GPU decode (SCAFFOLD)
//
// Pipeline (all on device, no PCIe in the timed region):
//   [1] ENTROPY   : nvcomp decode of zlit/zoff/zlen/zcmd  -> raw lit/off/len/cmd
//   [2] PARSE     : GPU parser, 1 thread per block, cmd-stream -> tokens (tp,ts,tl)
//                   + literals written straight into the output buffer
//   [3] LEVELS    : dependency depth per token (wavefront ordering)
//   [4] MATCH     : wavefront kernel applies match tokens level-by-level
//   [5] VERIFY    : xxh/FNV hash(out) == hash(original)   + cudaEvent timing
//
// STATUS: parser/match/verify logic is faithful to src/aceapex_main.cpp
//         (decompress_streams, read_varint, copy_match). Marked TODO:
//         (a) nvcomp wiring for [1]  — needs a "GPU profile" .aet where
//             off/len/cmd are nvcomp-ANS (current .aet uses ACEAPEX FSE).
//         (b) GPU level kernel for [3] — host forward-pass given first (correct,
//             matches levels.bin); port to device once correctness is locked.
//
// BRING-UP (incremental, against oracles we already dump from aceapex_depth):
//   step0  add a raw-stream dump to aceapex_depth: after FSE-decode, per block
//          fwrite the decoded lit/off/len/cmd + BlockOffsets  -> streams.bin
//   step1  run PARSE on streams.bin -> compare (tp,ts,tl) to tokens.bin  (oracle)
//   step2  run LEVELS -> compare to levels.bin                            (oracle)
//   step3  wire MATCH (kernel already verified in wf_proof.cu) -> hash == orig
//   step4  replace step0 dump with nvcomp decode (GPU profile) -> full device run
// =============================================================================
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
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
struct BlockOffsets {                       // one per block, within each raw stream
    uint64_t lit_off, off_off, len_off, cmd_off;
    uint64_t lit_sz,  off_sz,  len_sz,  cmd_sz;
};
#pragma pack(pop)

struct Token { uint32_t pos, src, len; };   // match token (absolute positions)

// ---------------------------------------------------------------------------
// device helpers — faithful to src/aceapex_main.cpp
// ---------------------------------------------------------------------------
__device__ __forceinline__ uint32_t d_read_varint(const uint8_t* buf, uint32_t& p, uint32_t limit){
    uint32_t val=0, shift=0;
    while(p<limit){ uint8_t b=buf[p++]; val|=(uint32_t)(b&0x7F)<<shift; if(!(b&0x80)) return val; shift+=7; }
    return val;
}

// ---------------------------------------------------------------------------
// [2] PARSE — one thread per block. Two modes via COUNT_ONLY:
//   COUNT_ONLY=1 : just count match tokens per block  -> d_count[b]
//   COUNT_ONLY=0 : emit tokens at d_tokoff[b], and write literals into d_out
// rep[] resets per block (blocks are independently decodable).
// ---------------------------------------------------------------------------
template<bool COUNT_ONLY>
__global__ void k_parse(
    const uint8_t* LIT, const uint8_t* OFF, const uint8_t* LEN, const uint8_t* CMD,
    const BlockOffsets* boffs, uint32_t num_blocks,
    uint64_t orig_size, uint32_t block_size,
    uint8_t* d_out,                 // output buffer (literals written here; matches later)
    Token* d_tokens, const uint32_t* d_tokoff, uint32_t* d_count)
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
    uint32_t tok = COUNT_ONLY ? 0u : d_tokoff[b];   // write cursor

    while(out<dst_size && cp<cmd_sz){
        uint8_t c=cmd[cp++];
        if(c==0xFF){ rep[0]=1;rep[1]=2;rep[2]=4;rep[3]=8; continue; }
        if(c<0x80){                                   // literal run
            uint32_t l=c+1;
            if(lp+l>lit_sz || out+l>dst_size) break;
            if(!COUNT_ONLY) for(uint32_t i=0;i<l;i++) d_out[base+out+i]=lit[lp+i];
            out+=l; lp+=l;
        } else if((c&0xC0)==0x80){                    // rep-offset match
            uint32_t ri=(c>>4)&3, lv=c&0x0F;
            if(lv==0x0F) lv+=d_read_varint(len,np,len_sz);
            uint32_t l=lv+6, dist=rep[ri];
            if(ri>0){ for(int i=ri;i>0;i--) rep[i]=rep[i-1]; rep[0]=dist; }
            if(!dist || out+l>dst_size) break;
            if(!COUNT_ONLY) d_tokens[tok]={(uint32_t)(base+out),(uint32_t)(base+out-dist),l};
            tok++; out+=l;
        } else {                                      // new-offset match
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

// ---------------------------------------------------------------------------
// [4] MATCH — wavefront: tokens of one dependency level applied in parallel.
// (identical kernel to wf_proof.cu; ord[] groups token indices by level)
// ---------------------------------------------------------------------------
__global__ void k_match(uint8_t* out, const Token* tk, const uint32_t* ord,
                        uint32_t base, uint32_t cnt){
    uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;
    if(i>=cnt) return;
    Token t = tk[ ord[base+i] ];
    for(uint32_t k=0;k<t.len;k++) out[t.pos+k]=out[t.src+k];
}

// ---------------------------------------------------------------------------
// [5] VERIFY — FNV-1a on device (matches wf_proof's k_hash for cross-check)
// ---------------------------------------------------------------------------
__global__ void k_hash(const uint8_t* buf, size_t n, uint64_t* out){
    if(blockIdx.x==0&&threadIdx.x==0){
        uint64_t h=0xcbf29ce484222325ULL;
        for(size_t i=0;i<n;i++) h=(h^buf[i])*0x100000001b3ULL;
        *out=h;
    }
}

// ===========================================================================
// HOST
// ===========================================================================
static inline uint64_t hmin(uint64_t a,uint64_t b){return a<b?a:b;}

int main(int argc, char** argv){
    if(argc<2){ fprintf(stderr,
        "Usage: full_gpu_decode <streams.bin> [original_file]\n"
        "  streams.bin = header + BlockOffsets[] + raw lit/off/len/cmd\n"
        "                (produced by the step0 dump in aceapex_depth)\n"); return 1; }

    // -------- load streams.bin (step0 dump format; see BRING-UP) -----------
    // layout assumed:  [AetHdr][BlockOffsets * num_blocks][LIT][OFF][LEN][CMD]
    // TODO(step0): implement the matching dump in aceapex_depth after FSE-decode.
    FILE* f=fopen(argv[1],"rb"); if(!f){perror("open");return 1;}
    AetHdr hdr; if(fread(&hdr,sizeof(hdr),1,f)!=1){fprintf(stderr,"bad header\n");return 1;}
    uint32_t nb=hdr.num_blocks;
    vector<BlockOffsets> boffs(nb);
    if(fread(boffs.data(),sizeof(BlockOffsets),nb,f)!=nb){fprintf(stderr,"bad boffs\n");return 1;}
    // total raw stream sizes = sum of per-block sizes
    uint64_t totL=0,totO=0,totN=0,totC=0;
    for(auto&b:boffs){totL+=b.lit_sz;totO+=b.off_sz;totN+=b.len_sz;totC+=b.cmd_sz;}
    vector<uint8_t> LIT(totL),OFF(totO),LEN(totN),CMD(totC);
    fread(LIT.data(),1,totL,f); fread(OFF.data(),1,totO,f);
    fread(LEN.data(),1,totN,f); fread(CMD.data(),1,totC,f); fclose(f);
    printf("orig=%llu  blocks=%u  block_size=%u  raw L/O/N/C=%.1f/%.1f/%.1f/%.1f MB\n",
        (unsigned long long)hdr.orig_size, nb, hdr.block_size,
        totL/1e6,totO/1e6,totN/1e6,totC/1e6);

    // -------- upload (one-time; NOT in the timed device-resident region) ---
    uint8_t *dLIT,*dOFF,*dLEN,*dCMD,*dOUT;
    BlockOffsets* dBO;
    CK(cudaMalloc(&dLIT,totL)); CK(cudaMalloc(&dOFF,totO));
    CK(cudaMalloc(&dLEN,totN)); CK(cudaMalloc(&dCMD,totC));
    CK(cudaMalloc(&dOUT,hdr.orig_size)); CK(cudaMalloc(&dBO,nb*sizeof(BlockOffsets)));
    CK(cudaMemcpy(dLIT,LIT.data(),totL,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dOFF,OFF.data(),totO,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dLEN,LEN.data(),totN,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dCMD,CMD.data(),totC,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dBO,boffs.data(),nb*sizeof(BlockOffsets),cudaMemcpyHostToDevice));

    cudaEvent_t t0,t1; CK(cudaEventCreate(&t0)); CK(cudaEventCreate(&t1));
    int TPB=128, grid=(nb+TPB-1)/TPB;

    // ===================== TIMED DEVICE-RESIDENT REGION ====================
    CK(cudaEventRecord(t0));

    // [1] ENTROPY — TODO: nvcomp decode of zlit/zoff/zlen/zcmd into dLIT.. dCMD.
    //     For bring-up we already have the raw streams uploaded above. Wire
    //     nvcomp (Codec ANS for off/len/cmd, Zstd for lit) once [2]-[4] verify.

    // [2] PARSE — pass1 count, prefix-sum, pass2 emit
    uint32_t *dCount,*dTokoff;
    CK(cudaMalloc(&dCount,nb*sizeof(uint32_t)));
    CK(cudaMalloc(&dTokoff,(nb+1)*sizeof(uint32_t)));
    k_parse<true><<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,
                                nullptr,nullptr,nullptr,dCount);
    CK(cudaDeviceSynchronize());
    // exclusive prefix-sum on host (TODO: thrust/cub on device to stay resident)
    vector<uint32_t> cnt(nb),tokoff(nb+1,0);
    CK(cudaMemcpy(cnt.data(),dCount,nb*sizeof(uint32_t),cudaMemcpyDeviceToHost));
    for(uint32_t b=0;b<nb;b++) tokoff[b+1]=tokoff[b]+cnt[b];
    uint32_t ntok=tokoff[nb];
    CK(cudaMemcpy(dTokoff,tokoff.data(),(nb+1)*sizeof(uint32_t),cudaMemcpyHostToDevice));
    Token* dTok; CK(cudaMalloc(&dTok,(size_t)ntok*sizeof(Token)));
    k_parse<false><<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,
                                 dOUT,dTok,dTokoff,nullptr);
    CK(cudaDeviceSynchronize());
    printf("[2] parsed ntok=%u\n", ntok);

    // [3] LEVELS — host forward-pass for correctness (TODO: port to device).
    //     level[ti] = 1 + max level of tokens covering [src,src+len); literal
    //     bytes contribute level 0. Single forward pass works because src<pos.
    vector<Token> tok(ntok);
    CK(cudaMemcpy(tok.data(),dTok,(size_t)ntok*sizeof(Token),cudaMemcpyDeviceToHost));
    vector<int32_t> tok_of(hdr.orig_size,-1);
    for(uint32_t ti=0;ti<ntok;ti++)
        for(uint32_t i=0;i<tok[ti].len && tok[ti].pos+i<hdr.orig_size;i++)
            tok_of[tok[ti].pos+i]=ti;
    vector<int32_t> lev(ntok,0); int ml=0;
    for(uint32_t ti=0;ti<ntok;ti++){
        int mx=0; uint32_t s=tok[ti].src, e=s+tok[ti].len, pp=s;
        while(pp<e){ int st=tok_of[pp];
            if(st>=0){ if(lev[st]+1>mx)mx=lev[st]+1; uint32_t nx=tok[st].pos+tok[st].len; pp=(nx>pp)?nx:pp+1; }
            else pp++; }
        lev[ti]=mx; if(mx>ml)ml=mx;
    }
    // bucket tokens by level -> ord[] (wavefront order) + per-level offsets
    vector<uint32_t> lcnt(ml+1,0); for(auto v:lev) lcnt[v]++;
    vector<uint32_t> loff(ml+2,0); for(int L=0;L<=ml;L++) loff[L+1]=loff[L]+lcnt[L];
    vector<uint32_t> ord(ntok), cur(loff.begin(),loff.end());
    for(uint32_t ti=0;ti<ntok;ti++) ord[cur[lev[ti]]++]=ti;
    uint32_t* dOrd; CK(cudaMalloc(&dOrd,(size_t)ntok*sizeof(uint32_t)));
    CK(cudaMemcpy(dOrd,ord.data(),(size_t)ntok*sizeof(uint32_t),cudaMemcpyHostToDevice));
    printf("[3] MaxLevel=%d\n", ml);

    // [4] MATCH — one kernel launch per level (wavefront)
    for(int L=0;L<=ml;L++){
        uint32_t base=loff[L], c=loff[L+1]-loff[L];
        if(c) k_match<<<(c+255)/256,256>>>(dOUT,dTok,dOrd,base,c);
    }
    CK(cudaEventRecord(t1)); CK(cudaEventSynchronize(t1));
    // =======================================================================

    float ms=0; CK(cudaEventElapsedTime(&ms,t0,t1));
    printf("[timed] device-resident decode: %.2f ms  -> %.1f GB/s\n",
        ms, hdr.orig_size/(ms*1e-3)/1e9);

    // [5] VERIFY
    uint64_t *dH,h=0; CK(cudaMalloc(&dH,8));
    k_hash<<<1,1>>>(dOUT,hdr.orig_size,dH); CK(cudaMemcpy(&h,dH,8,cudaMemcpyDeviceToHost));
    printf("[5] out FNV=%016llx\n",(unsigned long long)h);
    if(argc>2){
        FILE* fo=fopen(argv[2],"rb");
        if(fo){ vector<uint8_t> orig(hdr.orig_size); fread(orig.data(),1,hdr.orig_size,fo); fclose(fo);
            uint64_t ho=0xcbf29ce484222325ULL; for(uint8_t b:orig) ho=(ho^b)*0x100000001b3ULL;
            printf("    orig FNV=%016llx  %s\n",(unsigned long long)ho, ho==h?"MATCHES ✓":"DIFFERS ✗"); }
    }
    return 0;
}
