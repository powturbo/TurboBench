// =============================================================================
// full_gpu_decode_v4.cu — ACEAPEX FULL GPU pipeline: nvcomp-ANS entropy + decode
//
// Two modes:
//   pack   <streams.bin> <out.gaet>   : raw lit/off/len/cmd streams -> batched
//                                       nvcomp rANS chunks ("GPU profile" container)
//   decode <in.gaet> [original_file]  : [A] nvcomp ANS decompress (device)
//                                       [B] k_decode warp-per-block (device)
//                                       timed together = full device pipeline.
//
// This closes the last gate: entropy now decoded ON GPU (nvcomp 5.x batched ANS
// C API), no CPU stage between compressed bytes and decoded file.
// HONEST CAVEATS: GPU profile re-encodes the four RAW streams with rANS instead
// of the CPU profile's FSE/zstd => container ratio differs (report it). H2D of
// the compressed blob excluded from the timer (one-time staging, same policy).
// =============================================================================
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cuda_runtime.h>
#include <nvcomp/ans.h>
using namespace std;

#define CK(x) do{cudaError_t e=(x); if(e){printf("CUDA err %s:%d %s\n",__FILE__,__LINE__,cudaGetErrorString(e));exit(1);} }while(0)
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
struct G4Hdr {                       // GPU-profile container header
    char     magic[8];               // "ACEGPU4\0"
    AetHdr   aet;                    // original .aet header (orig_size, blocks...)
    uint64_t totL, totO, totN, totC; // raw stream sizes
    uint32_t chunk;                  // uncompressed chunk size (e.g. 65536)
    uint32_t nchunks;
};
#pragma pack(pop)

static const uint32_t CHUNK = 65536;
static inline uint64_t pad16(uint64_t x){ return (x+15)&~15ull; }

// ---------------- decode kernel: warp-per-block (verbatim from v3) ----------
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

// ---------------- PACK: raw streams -> ANS chunks -> .gaet --------------------
static int do_pack(const char* in_path, const char* out_path){
    AetHdr hdr; vector<BlockOffsets> boffs; vector<uint8_t> RAW;
    uint64_t totL,totO,totN,totC;
    if(!read_streams(in_path,hdr,boffs,RAW,totL,totO,totN,totC)) return 1;
    uint64_t totRaw=RAW.size();
    uint32_t nchunks=(uint32_t)((totRaw+CHUNK-1)/CHUNK);
    printf("pack: raw=%.1f MB chunks=%u x %u\n", totRaw/1e6, nchunks, CHUNK);

    uint8_t* dRaw; CK(cudaMalloc(&dRaw,totRaw));
    CK(cudaMemcpy(dRaw,RAW.data(),totRaw,cudaMemcpyHostToDevice));

    vector<void*>  h_in_ptrs(nchunks); vector<size_t> h_in_sz(nchunks);
    for(uint32_t i=0;i<nchunks;i++){
        h_in_ptrs[i]=dRaw+(uint64_t)i*CHUNK;
        h_in_sz[i]=(i+1<nchunks)?CHUNK:(size_t)(totRaw-(uint64_t)(nchunks-1)*CHUNK);
    }
    size_t max_out=0;
    NVCK(nvcompBatchedANSCompressGetMaxOutputChunkSize(CHUNK,nvcompBatchedANSCompressDefaultOpts,&max_out));
    uint64_t stride=pad16(max_out);
    uint8_t* dComp; CK(cudaMalloc(&dComp,stride*nchunks));
    vector<void*> h_out_ptrs(nchunks);
    for(uint32_t i=0;i<nchunks;i++) h_out_ptrs[i]=dComp+(uint64_t)i*stride;

    void **d_in_ptrs,**d_out_ptrs; size_t *d_in_sz,*d_out_sz; nvcompStatus_t* d_st;
    CK(cudaMalloc(&d_in_ptrs ,nchunks*sizeof(void*)));
    CK(cudaMalloc(&d_out_ptrs,nchunks*sizeof(void*)));
    CK(cudaMalloc(&d_in_sz ,nchunks*sizeof(size_t)));
    CK(cudaMalloc(&d_out_sz,nchunks*sizeof(size_t)));
    CK(cudaMalloc(&d_st,nchunks*sizeof(nvcompStatus_t)));
    CK(cudaMemcpy(d_in_ptrs ,h_in_ptrs.data() ,nchunks*sizeof(void*),cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_out_ptrs,h_out_ptrs.data(),nchunks*sizeof(void*),cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_in_sz,h_in_sz.data(),nchunks*sizeof(size_t),cudaMemcpyHostToDevice));

    size_t temp_sz=0;
    NVCK(nvcompBatchedANSCompressGetTempSizeAsync(nchunks,CHUNK,nvcompBatchedANSCompressDefaultOpts,&temp_sz,totRaw));
    void* dTemp=nullptr; if(temp_sz) CK(cudaMalloc(&dTemp,temp_sz));

    NVCK(nvcompBatchedANSCompressAsync((const void* const*)d_in_ptrs,d_in_sz,CHUNK,nchunks,
        dTemp,temp_sz,(void* const*)d_out_ptrs,d_out_sz,
        nvcompBatchedANSCompressDefaultOpts,d_st,0));
    CK(cudaDeviceSynchronize());

    vector<size_t> csz(nchunks); vector<nvcompStatus_t> st(nchunks);
    CK(cudaMemcpy(csz.data(),d_out_sz,nchunks*sizeof(size_t),cudaMemcpyDeviceToHost));
    CK(cudaMemcpy(st.data(),d_st,nchunks*sizeof(nvcompStatus_t),cudaMemcpyDeviceToHost));
    for(uint32_t i=0;i<nchunks;i++) if(st[i]!=nvcompSuccess){printf("chunk %u compress status %d\n",i,(int)st[i]);return 1;}

    // pull compressed (strided) and write container with 16B-padded chunks
    vector<uint8_t> comp(stride*nchunks);
    CK(cudaMemcpy(comp.data(),dComp,stride*nchunks,cudaMemcpyDeviceToHost));
    G4Hdr g{}; memcpy(g.magic,"ACEGPU4",8); g.aet=hdr;
    g.totL=totL;g.totO=totO;g.totN=totN;g.totC=totC; g.chunk=CHUNK; g.nchunks=nchunks;
    FILE* fo=fopen(out_path,"wb"); if(!fo){perror("open out");return 1;}
    fwrite(&g,sizeof(g),1,fo);
    fwrite(boffs.data(),sizeof(BlockOffsets),hdr.num_blocks,fo);
    vector<uint64_t> cs64(nchunks); uint64_t blob=0;
    for(uint32_t i=0;i<nchunks;i++){ cs64[i]=csz[i]; blob+=pad16(csz[i]); }
    fwrite(cs64.data(),sizeof(uint64_t),nchunks,fo);
    vector<uint8_t> zero(16,0);
    for(uint32_t i=0;i<nchunks;i++){
        fwrite(comp.data()+(uint64_t)i*stride,1,csz[i],fo);
        uint64_t p=pad16(csz[i])-csz[i]; if(p) fwrite(zero.data(),1,p,fo);
    }
    fclose(fo);
    uint64_t meta=sizeof(g)+sizeof(BlockOffsets)*hdr.num_blocks+8ull*nchunks;
    printf("pack done: container=%.1f MB (blob %.1f + meta %.1f)  GPU-profile ratio=%.3f\n",
        (blob+meta)/1e6, blob/1e6, meta/1e6, (double)hdr.orig_size/(blob+meta));
    return 0;
}

// ---------------- DECODE: .gaet -> ANS on GPU -> k_decode ---------------------
static int do_decode(const char* in_path, const char* orig_path){
    FILE* f=fopen(in_path,"rb"); if(!f){perror("open gaet");return 1;}
    G4Hdr g; if(fread(&g,sizeof(g),1,f)!=1 || memcmp(g.magic,"ACEGPU4",7)){fprintf(stderr,"bad gaet\n");return 1;}
    uint32_t nb=g.aet.num_blocks, nchunks=g.nchunks;
    vector<BlockOffsets> boffs(nb);
    if(fread(boffs.data(),sizeof(BlockOffsets),nb,f)!=nb){fprintf(stderr,"bad boffs\n");return 1;}
    vector<uint64_t> csz(nchunks);
    if(fread(csz.data(),sizeof(uint64_t),nchunks,f)!=nchunks){fprintf(stderr,"bad sizes\n");return 1;}
    uint64_t blob=0; for(uint32_t i=0;i<nchunks;i++) blob+=pad16(csz[i]);
    vector<uint8_t> COMP(blob);
    if(fread(COMP.data(),1,blob,f)!=blob){fprintf(stderr,"short blob\n");return 1;}
    fclose(f);
    uint64_t totRaw=g.totL+g.totO+g.totN+g.totC;
    printf("orig=%llu blocks=%u block_size=%u  chunks=%u comp_blob=%.1f MB raw=%.1f MB\n",
        (unsigned long long)g.aet.orig_size, nb, g.aet.block_size, nchunks, blob/1e6, totRaw/1e6);

    // upload compressed blob + metadata (one-time staging, untimed)
    uint8_t *dComp,*dRaw,*dOUT; BlockOffsets* dBO;
    CK(cudaMalloc(&dComp,blob));
    CK(cudaMalloc(&dRaw ,totRaw));
    CK(cudaMalloc(&dOUT ,g.aet.orig_size));
    CK(cudaMalloc(&dBO  ,nb*sizeof(BlockOffsets)));
    CK(cudaMemcpy(dComp,COMP.data(),blob,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dBO,boffs.data(),nb*sizeof(BlockOffsets),cudaMemcpyHostToDevice));

    vector<void*> h_c_ptrs(nchunks),h_o_ptrs(nchunks);
    vector<size_t> h_c_sz(nchunks),h_buf_sz(nchunks);
    { uint64_t off=0;
      for(uint32_t i=0;i<nchunks;i++){
        h_c_ptrs[i]=dComp+off; h_c_sz[i]=csz[i]; off+=pad16(csz[i]);
        h_o_ptrs[i]=dRaw+(uint64_t)i*g.chunk;
        h_buf_sz[i]=(i+1<nchunks)?g.chunk:(size_t)(totRaw-(uint64_t)(nchunks-1)*g.chunk);
      } }
    void **d_c_ptrs,**d_o_ptrs; size_t *d_c_sz,*d_buf_sz,*d_act_sz; nvcompStatus_t* d_st;
    CK(cudaMalloc(&d_c_ptrs,nchunks*sizeof(void*)));
    CK(cudaMalloc(&d_o_ptrs,nchunks*sizeof(void*)));
    CK(cudaMalloc(&d_c_sz ,nchunks*sizeof(size_t)));
    CK(cudaMalloc(&d_buf_sz,nchunks*sizeof(size_t)));
    CK(cudaMalloc(&d_act_sz,nchunks*sizeof(size_t)));
    CK(cudaMalloc(&d_st,nchunks*sizeof(nvcompStatus_t)));
    CK(cudaMemcpy(d_c_ptrs,h_c_ptrs.data(),nchunks*sizeof(void*),cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_o_ptrs,h_o_ptrs.data(),nchunks*sizeof(void*),cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_c_sz,h_c_sz.data(),nchunks*sizeof(size_t),cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_buf_sz,h_buf_sz.data(),nchunks*sizeof(size_t),cudaMemcpyHostToDevice));

    size_t temp_sz=0;
    NVCK(nvcompBatchedANSDecompressGetTempSizeAsync(nchunks,g.chunk,
        nvcompBatchedANSDecompressDefaultOpts,&temp_sz,totRaw));
    void* dTemp=nullptr; if(temp_sz) CK(cudaMalloc(&dTemp,temp_sz));

    const uint8_t* dLIT=dRaw;
    const uint8_t* dOFF=dRaw+g.totL;
    const uint8_t* dLEN=dRaw+g.totL+g.totO;
    const uint8_t* dCMD=dRaw+g.totL+g.totO+g.totN;
    const int TPB=128;
    uint32_t grid=(uint32_t)(((uint64_t)nb*32+TPB-1)/TPB);

    // warm-up (untimed), then clear outputs so the timed run does all the work
    NVCK(nvcompBatchedANSDecompressAsync((const void* const*)d_c_ptrs,d_c_sz,d_buf_sz,d_act_sz,
        nchunks,dTemp,temp_sz,(void* const*)d_o_ptrs,
        nvcompBatchedANSDecompressDefaultOpts,d_st,0));
    k_decode<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,g.aet.orig_size,g.aet.block_size,dOUT);
    CK(cudaDeviceSynchronize());
    CK(cudaMemset(dRaw,0,totRaw)); CK(cudaMemset(dOUT,0,g.aet.orig_size));

    cudaEvent_t t0,t1,t2; CK(cudaEventCreate(&t0));CK(cudaEventCreate(&t1));CK(cudaEventCreate(&t2));
    // ===================== TIMED FULL DEVICE PIPELINE ======================
    CK(cudaEventRecord(t0));
    NVCK(nvcompBatchedANSDecompressAsync((const void* const*)d_c_ptrs,d_c_sz,d_buf_sz,d_act_sz,
        nchunks,dTemp,temp_sz,(void* const*)d_o_ptrs,
        nvcompBatchedANSDecompressDefaultOpts,d_st,0));
    CK(cudaEventRecord(t1));
    k_decode<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,g.aet.orig_size,g.aet.block_size,dOUT);
    CK(cudaEventRecord(t2)); CK(cudaEventSynchronize(t2));
    // =======================================================================
    CK(cudaGetLastError());
    vector<nvcompStatus_t> st(nchunks);
    CK(cudaMemcpy(st.data(),d_st,nchunks*sizeof(nvcompStatus_t),cudaMemcpyDeviceToHost));
    for(uint32_t i=0;i<nchunks;i++) if(st[i]!=nvcompSuccess){printf("chunk %u decompress status %d\n",i,(int)st[i]);return 1;}

    float msA=0,msB=0,msT=0;
    CK(cudaEventElapsedTime(&msA,t0,t1));
    CK(cudaEventElapsedTime(&msB,t1,t2));
    CK(cudaEventElapsedTime(&msT,t0,t2));
    printf("[A] nvcomp ANS entropy : %.2f ms  (%.1f GB/s raw streams)\n",msA,totRaw/(msA*1e-3)/1e9);
    printf("[B] k_decode warp/block: %.2f ms  (%.1f GB/s output)\n",msB,g.aet.orig_size/(msB*1e-3)/1e9);
    printf("[timed] FULL GPU pipeline (entropy+decode): %.2f ms -> %.1f GB/s\n",
        msT, g.aet.orig_size/(msT*1e-3)/1e9);

    uint64_t *dH,h=0; CK(cudaMalloc(&dH,8));
    k_hash<<<1,1>>>(dOUT,g.aet.orig_size,dH); CK(cudaMemcpy(&h,dH,8,cudaMemcpyDeviceToHost));
    printf("[5] out FNV=%016llx\n",(unsigned long long)h);
    if(orig_path){
        FILE* fo=fopen(orig_path,"rb");
        if(fo){ vector<uint8_t> orig(g.aet.orig_size);
            if(fread(orig.data(),1,g.aet.orig_size,fo)!=g.aet.orig_size) fprintf(stderr,"short orig\n");
            fclose(fo);
            uint64_t ho=0xcbf29ce484222325ULL; for(uint8_t b:orig) ho=(ho^b)*0x100000001b3ULL;
            printf("    orig FNV=%016llx  %s\n",(unsigned long long)ho, ho==h?"MATCHES OK":"DIFFERS X"); }
    }
    return 0;
}

int main(int argc,char**argv){
    if(argc>=4 && !strcmp(argv[1],"pack"))   return do_pack(argv[2],argv[3]);
    if(argc>=3 && !strcmp(argv[1],"decode")) return do_decode(argv[2], argc>3?argv[3]:nullptr);
    fprintf(stderr,"Usage:\n  %s pack <streams.bin> <out.gaet>\n  %s decode <in.gaet> [original]\n",argv[0],argv[0]);
    return 1;
}
