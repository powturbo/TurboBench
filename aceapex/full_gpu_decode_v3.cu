// =============================================================================
// full_gpu_decode_v3.cu — ACEAPEX device-resident decode, WARP-PER-BLOCK
//
// Architecture change vs v2: no tokens, no levels, no buckets. Blocks are
// independent => one warp decodes one block sequentially (token order, same
// state machine as CPU), 32 lanes cooperate on every copy:
//   literal run : lanes copy strided from LIT stream
//   match       : out[dst+i] = out[src + (i % dist)]  — pattern replication;
//                 all reads land BEFORE dst => fully parallel, no intra-copy sync
// __syncwarp() between tokens makes prior writes visible to the next token.
// Single kernel launch decodes the whole file.
//
// HONEST CAVEATS: entropy still from step0 streams.bin dump (raw lit/off/len/cmd
// after FSE), NOT nvcomp yet. Timed region = the one decode kernel (device-
// resident; H2D upload excluded as one-time staging, same policy as v2).
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

__device__ __forceinline__ uint32_t d_read_varint(const uint8_t* buf, uint32_t& p, uint32_t limit){
    uint32_t val=0, shift=0;
    while(p<limit){ uint8_t b=buf[p++]; val|=(uint32_t)(b&0x7F)<<shift; if(!(b&0x80)) return val; shift+=7; }
    return val;
}

// one warp = one block; lane 0 parses, all 32 lanes copy
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

    // parser state — lane 0 only
    uint32_t lp=0, op=0, np=0, cp=0;
    uint32_t rep[4]={1,2,4,8};
    uint32_t out_pos=0;
    uint32_t cmd_sz=(uint32_t)bo.cmd_sz, lit_sz=(uint32_t)bo.lit_sz;
    uint32_t off_sz=(uint32_t)bo.off_sz, len_sz=(uint32_t)bo.len_sz;

    while (out_pos < dst_size) {
        uint32_t type=2, l=0, aux=0;     // 0=lit(aux=lit start) 1=match(aux=dist) 2=end
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

        if (type==0) {                                   // literal run
            for (uint32_t i=lane; i<l; i+=32) dst[out_pos+i] = lit[aux+i];
        } else {                                         // match, dist=aux
            uint32_t src = out_pos - aux;
            if (aux >= l) {                              // non-overlapping: plain copy
                for (uint32_t i=lane; i<l; i+=32) dst[out_pos+i] = dst[src+i];
            } else {                                     // overlapping: pattern replication
                for (uint32_t i=lane; i<l; i+=32) dst[out_pos+i] = dst[src + (i % aux)];
            }
        }
        __syncwarp();                                    // writes visible to next token
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

    uint8_t *dLIT,*dOFF,*dLEN,*dCMD,*dOUT; BlockOffsets* dBO;
    CK(cudaMalloc(&dLIT,totL)); CK(cudaMalloc(&dOFF,totO));
    CK(cudaMalloc(&dLEN,totN)); CK(cudaMalloc(&dCMD,totC));
    CK(cudaMalloc(&dOUT,hdr.orig_size)); CK(cudaMalloc(&dBO,nb*sizeof(BlockOffsets)));
    CK(cudaMemcpy(dLIT,LIT.data(),totL,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dOFF,OFF.data(),totO,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dLEN,LEN.data(),totN,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dCMD,CMD.data(),totC,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dBO,boffs.data(),nb*sizeof(BlockOffsets),cudaMemcpyHostToDevice));

    cudaEvent_t t0,t1; CK(cudaEventCreate(&t0)); CK(cudaEventCreate(&t1));
    const int TPB=128;                                   // 4 warps per CUDA block
    uint64_t threads_needed=(uint64_t)nb*32;
    uint32_t grid=(uint32_t)((threads_needed+TPB-1)/TPB);

    // warm-up (excluded): page-in, instruction cache
    k_decode<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,dOUT);
    CK(cudaDeviceSynchronize());
    CK(cudaMemset(dOUT,0,hdr.orig_size));                // clear so timed run does all the work

    // ===================== TIMED DEVICE-RESIDENT REGION ====================
    CK(cudaEventRecord(t0));
    k_decode<<<grid,TPB>>>(dLIT,dOFF,dLEN,dCMD,dBO,nb,hdr.orig_size,hdr.block_size,dOUT);
    CK(cudaEventRecord(t1)); CK(cudaEventSynchronize(t1));
    // =======================================================================
    CK(cudaGetLastError());

    float ms=0; CK(cudaEventElapsedTime(&ms,t0,t1));
    printf("[timed] device-resident decode (1 kernel, warp/block): %.2f ms -> %.1f GB/s\n",
           ms, hdr.orig_size/(ms*1e-3)/1e9);

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
