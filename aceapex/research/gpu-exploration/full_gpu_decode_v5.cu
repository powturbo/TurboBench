// =============================================================================
// full_gpu_decode_v5.cu — ACEAPEX device decode v5: batched parse + fused copies
//
// Lab-driven design (see CONTEXT "ЛАБОРАТОРИЯ"):
//   * cmd stream consumes EXACTLY 1 byte per token  -> parse in batches of 32
//   * matches are short (p50 13B) and 94.7%/73.6% of pairs/quads independent
//     -> execute GROUPS of independent tokens in one parallel wave (cap 8)
//   * true overlap (dist<len) is 0.7% -> overlap token simply starts its group
//   * heterogeneous corpora unbalance fixed assignment -> persistent kernel
//     with a global block queue (atomicAdd)
// Parse batch: lane0 runs one tight register loop over <=32 cmd bytes filling
// shared T/L/A/D (type,len,aux,dst); all 32 lanes then execute copy groups.
// Same CLI as v3:  fgd5 <streams.bin> [original]
// On DIFFERS: built-in host reference decode pinpoints first mismatch block.
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

__device__ __host__ static inline uint32_t rd_varint(const uint8_t* buf, uint32_t& p, uint32_t limit){
    uint32_t val=0, shift=0;
    while(p<limit){ uint8_t b=buf[p++]; val|=(uint32_t)(b&0x7F)<<shift; if(!(b&0x80)) return val; shift+=7; }
    return val;
}

// token types in shared T[]
#define TT_LIT  0u
#define TT_MAT  1u
#define TT_SKIP 3u
#define GROUP_CAP 8

__global__ void k_decode_v5(const uint8_t* __restrict__ LIT, const uint8_t* __restrict__ OFF,
                            const uint8_t* __restrict__ LEN, const uint8_t* __restrict__ CMD,
                            const BlockOffsets* __restrict__ boffs, uint32_t num_blocks,
                            uint64_t orig_size, uint32_t block_size, uint8_t* __restrict__ out,
                            uint32_t* __restrict__ blk_ctr)
{
    __shared__ uint32_t sT[4][32], sL[4][32], sA[4][32], sD[4][32];
    __shared__ uint32_t sN[4];
    uint32_t wid  = threadIdx.x >> 5;
    uint32_t lane = threadIdx.x & 31;
    uint32_t *T=sT[wid], *L=sL[wid], *A=sA[wid], *D=sD[wid];

    for(;;){
        uint32_t b=0;
        if(lane==0) b=atomicAdd(blk_ctr,1u);
        b=__shfl_sync(0xffffffffu,b,0);
        if(b>=num_blocks) return;

        BlockOffsets bo = boffs[b];
        const uint8_t* lit = LIT + bo.lit_off;
        const uint8_t* off = OFF + bo.off_off;
        const uint8_t* len = LEN + bo.len_off;
        const uint8_t* cmd = CMD + bo.cmd_off;
        uint64_t base = (uint64_t)b * block_size;
        uint64_t rem  = orig_size - base;
        uint32_t dst_size = (uint32_t)((rem < (uint64_t)block_size) ? rem : (uint64_t)block_size);
        uint8_t* dst = out + base;

        // parser state lives in lane0 registers only
        uint32_t lp=0, op=0, np=0, cp=0, out_pos=0, stop=0;
        uint32_t rep0=1,rep1=2,rep2=4,rep3=8;
        uint32_t cmd_sz=(uint32_t)bo.cmd_sz, lit_sz=(uint32_t)bo.lit_sz;
        uint32_t off_sz=(uint32_t)bo.off_sz, len_sz=(uint32_t)bo.len_sz;

        for(;;){
            // ---- lane0: parse a batch of up to 32 cmd bytes into T/L/A/D ----
            if(lane==0){
                uint32_t navail = (cmd_sz>cp)? (cmd_sz-cp) : 0;
                if(navail>32) navail=32;
                if(stop || out_pos>=dst_size) navail=0;
                uint32_t i=0;
                for(; i<navail; i++){
                    uint32_t c = cmd[cp+i];
                    if(c==0xFF){ rep0=1;rep1=2;rep2=4;rep3=8; T[i]=TT_SKIP; L[i]=0; continue; }
                    if(c<0x80){
                        uint32_t l=c+1;
                        if(lp+l>lit_sz || out_pos+l>dst_size){ stop=1; break; }
                        T[i]=TT_LIT; L[i]=l; A[i]=lp; D[i]=out_pos; lp+=l; out_pos+=l;
                    } else if((c&0xC0)==0x80){
                        uint32_t ri=(c>>4)&3, lv=c&0x0F;
                        if(lv==0x0F) lv += rd_varint(len,np,len_sz);
                        uint32_t l=lv+6;
                        uint32_t dist = (ri==0)?rep0:(ri==1)?rep1:(ri==2)?rep2:rep3;
                        if(ri==1){ rep1=rep0; rep0=dist; }
                        else if(ri==2){ rep2=rep1; rep1=rep0; rep0=dist; }
                        else if(ri==3){ rep3=rep2; rep2=rep1; rep1=rep0; rep0=dist; }
                        if(!dist || dist>out_pos || out_pos+l>dst_size){ stop=1; break; }
                        T[i]=TT_MAT; L[i]=l; A[i]=dist; D[i]=out_pos; out_pos+=l;
                    } else {
                        uint32_t lv=(c==0xFE)? rd_varint(len,np,len_sz) : (uint32_t)(c&0x3F);
                        uint32_t l=lv+6;
                        uint32_t dist=rd_varint(off,op,off_sz);
                        rep3=rep2; rep2=rep1; rep1=rep0; rep0=dist;
                        if(!dist || dist>out_pos || out_pos+l>dst_size){ stop=1; break; }
                        T[i]=TT_MAT; L[i]=l; A[i]=dist; D[i]=out_pos; out_pos+=l;
                    }
                }
                cp += i;
                sN[wid]=i;
            }
            __syncwarp();
            uint32_t n = sN[wid];
            if(n==0) break;

            // ---- all lanes: execute copy groups over tokens [0,n) ----
            uint32_t g=0;
            for(;;){
                uint32_t e=0, gb=0;
                if(lane==0){
                    while(g<n && T[g]==TT_SKIP) g++;
                    e=g;
                    if(g<n){
                        uint32_t base_dst=D[g]; uint32_t cnt=1; e=g+1; gb=L[g];
                        while(e<n && cnt<GROUP_CAP){
                            if(T[e]==TT_SKIP){ e++; continue; }
                            if(T[e]==TT_LIT){ gb+=L[e]; e++; cnt++; continue; }
                            uint32_t src=D[e]-A[e];
                            if(src+L[e]<=base_dst){ gb+=L[e]; e++; cnt++; }
                            else break;
                        }
                    }
                }
                g =__shfl_sync(0xffffffffu,g,0);
                e =__shfl_sync(0xffffffffu,e,0);
                gb=__shfl_sync(0xffffffffu,gb,0);
                if(g>=n) break;
                for(uint32_t i=lane; i<gb; i+=32){
                    uint32_t k=g, acc=0;
                    for(;;){
                        if(T[k]==TT_SKIP){ k++; continue; }
                        if(i < acc + L[k]) break;
                        acc += L[k]; k++;
                    }
                    uint32_t o=i-acc;
                    if(T[k]==TT_LIT) dst[D[k]+o] = lit[A[k]+o];
                    else {
                        uint32_t dd=A[k], s=D[k]-dd;
                        dst[D[k]+o] = dst[s + ((dd>=L[k])? o : (o % dd))];
                    }
                }
                __syncwarp();
                g=e;
            }
            __syncwarp();
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

// ---------------- host reference decode (debug aid on mismatch) --------------
static void host_decode(const AetHdr& hdr, const vector<BlockOffsets>& boffs,
                        const uint8_t* LIT,const uint8_t* OFF,const uint8_t* LEN,const uint8_t* CMD,
                        vector<uint8_t>& outb){
    outb.assign(hdr.orig_size,0);
    for(uint32_t b=0;b<hdr.num_blocks;b++){
        const BlockOffsets& bo=boffs[b];
        const uint8_t* lit=LIT+bo.lit_off; const uint8_t* off=OFF+bo.off_off;
        const uint8_t* len=LEN+bo.len_off; const uint8_t* cmd=CMD+bo.cmd_off;
        uint64_t base=(uint64_t)b*hdr.block_size;
        uint64_t rem=hdr.orig_size-base;
        uint32_t dstsz=(uint32_t)((rem<(uint64_t)hdr.block_size)?rem:(uint64_t)hdr.block_size);
        uint8_t* dst=outb.data()+base;
        uint32_t lp=0,op=0,np=0,cp=0,outp=0,rep[4]={1,2,4,8};
        uint32_t cs=(uint32_t)bo.cmd_sz, ls=(uint32_t)bo.lit_sz, os=(uint32_t)bo.off_sz, ns=(uint32_t)bo.len_sz;
        while(outp<dstsz && cp<cs){
            uint8_t c=cmd[cp++];
            if(c==0xFF){rep[0]=1;rep[1]=2;rep[2]=4;rep[3]=8;continue;}
            if(c<0x80){ uint32_t l=c+1; if(lp+l>ls||outp+l>dstsz)break;
                memcpy(dst+outp,lit+lp,l); outp+=l; lp+=l; }
            else if((c&0xC0)==0x80){
                uint32_t ri=(c>>4)&3,lv=c&0x0F;
                if(lv==0x0F) lv+=rd_varint(len,np,ns);
                uint32_t l=lv+6,dist=rep[ri];
                if(ri>0){ for(int i=(int)ri;i>0;i--)rep[i]=rep[i-1]; rep[0]=dist; }
                if(!dist||dist>outp||outp+l>dstsz)break;
                for(uint32_t i=0;i<l;i++) dst[outp+i]=dst[outp-dist+i];
                outp+=l;
            } else {
                uint32_t lv=(c==0xFE)?rd_varint(len,np,ns):(uint32_t)(c&0x3F);
                uint32_t l=lv+6,dist=rd_varint(off,op,os);
                rep[3]=rep[2];rep[2]=rep[1];rep[1]=rep[0];rep[0]=dist;
                if(!dist||dist>outp||outp+l>dstsz)break;
                for(uint32_t i=0;i<l;i++) dst[outp+i]=dst[outp-dist+i];
                outp+=l;
            }
        }
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

    const int TPB=128;
    int dev=0, nsm=0; CK(cudaGetDevice(&dev));
    CK(cudaDeviceGetAttribute(&nsm,cudaDevAttrMultiProcessorCount,dev));
    int maxblk=0; CK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&maxblk,k_decode_v5,TPB,0));
    uint32_t want=(uint32_t)(((uint64_t)nb*32+TPB-1)/TPB);
    uint32_t grid=(uint32_t)nsm*(uint32_t)maxblk; if(grid>want) grid=want; if(grid<1)grid=1;
    printf("persistent grid=%u CUDA-blocks (%d SM x %d), warps=%u for %u blocks\n",grid,nsm,maxblk,grid*(TPB/32),nb);

    cudaEvent_t t0,t1; CK(cudaEventCreate(&t0)); CK(cudaEventCreate(&t1));
    // warm-up
    CK(cudaMemset(dCTR,0,4));
    k_decode_v5<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,dOUT,dCTR);
    CK(cudaDeviceSynchronize()); CK(cudaGetLastError());
    CK(cudaMemset(dOUT,0,hdr.orig_size));
    // ===================== TIMED DEVICE-RESIDENT REGION ====================
    CK(cudaEventRecord(t0));
    CK(cudaMemsetAsync(dCTR,0,4));
    k_decode_v5<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,dOUT,dCTR);
    CK(cudaEventRecord(t1)); CK(cudaEventSynchronize(t1));
    // =======================================================================
    CK(cudaGetLastError());
    float ms=0; CK(cudaEventElapsedTime(&ms,t0,t1));
    printf("[timed] v5 batched-parse decode: %.2f ms -> %.1f GB/s\n", ms, hdr.orig_size/(ms*1e-3)/1e9);

    uint64_t *dH,h=0; CK(cudaMalloc(&dH,8));
    k_hash<<<1,1>>>(dOUT,hdr.orig_size,dH); CK(cudaMemcpy(&h,dH,8,cudaMemcpyDeviceToHost));
    printf("[5] out FNV=%016llx\n",(unsigned long long)h);
    if(argc>2){
        FILE* fo=fopen(argv[2],"rb");
        if(fo){ vector<uint8_t> orig(hdr.orig_size);
            if(fread(orig.data(),1,hdr.orig_size,fo)!=hdr.orig_size) fprintf(stderr,"short orig\n");
            fclose(fo);
            uint64_t ho=0xcbf29ce484222325ULL; for(uint8_t x:orig) ho=(ho^x)*0x100000001b3ULL;
            bool ok = (ho==h);
            printf("    orig FNV=%016llx  %s\n",(unsigned long long)ho, ok?"MATCHES OK":"DIFFERS X");
            if(!ok){
                printf("--- debug: comparing GPU output vs original ---\n");
                vector<uint8_t> gout(hdr.orig_size);
                CK(cudaMemcpy(gout.data(),dOUT,hdr.orig_size,cudaMemcpyDeviceToHost));
                size_t i=0; while(i<hdr.orig_size && gout[i]==orig[i]) i++;
                if(i<hdr.orig_size){
                    printf("first diff at byte %zu (block %zu, in-block off %zu) gpu=%02x orig=%02x\n",
                        i, i/hdr.block_size, i%hdr.block_size, gout[i], orig[i]);
                    vector<uint8_t> ref; host_decode(hdr,boffs,LIT.data(),OFF.data(),LEN.data(),CMD.data(),ref);
                    size_t j=0; while(j<hdr.orig_size && ref[j]==orig[j]) j++;
                    printf("host-reference first diff vs orig: %s (at %zu)\n", j==hdr.orig_size?"none (ref perfect, bug is in GPU kernel)":"REF ALSO DIFFERS", j);
                }
            }
        }
    }
    return 0;
}
