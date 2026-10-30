// v3gpu.cu - P2 of docs/ARCH_V3.md on the GPU: the same matches (one ACEPX2 archive's parse), decoded by
//   base  the production v7-RA match kernel k_decode_g<G> (src/aceapex_gpu_kernels.cuh): per token, the
//         group leader parses the next command (varint, rep codes) serially, then the group copies
//   v3    fixed-width tokens (litlen u16 | mlen u16 | dist u32, one per "literals + match"), one warp per
//         block, 32 tokens per step: positions by warp scans (no serial parse); all literal bytes of the
//         step copied by the whole warp at once (the step's literals are contiguous in the literal stream;
//         each lane finds its token by a binary search over the scanned run starts); then the matches in
//         token order, each copied by the warp (period copy when dist < length, as in v7-RA)
// Timed on the device (cudaEvent, median), match phase only (literal / token entropy outside, same for both);
// output checked byte for byte against the original. Input: streams.bin (ACEAPEX_DUMP=1 aceapex d).
// Build: nvcc -O3 -std=c++17 -arch=sm_XX -I../../src -o v3gpu v3gpu.cu
// Usage: v3gpu streams.bin original [reps=9]
#include "aceapex_gpu_kernels.cuh"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cuda_runtime.h>

#define CK(x) do{cudaError_t e_=(x); if(e_!=cudaSuccess){fprintf(stderr,"CUDA %s @%d: %s\n",#x,__LINE__,cudaGetErrorString(e_)); exit(2);} }while(0)

static std::vector<uint8_t> slurp(const char* p){ FILE* f=fopen(p,"rb"); if(!f){perror(p); exit(1);} fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    std::vector<uint8_t> v(n); if(fread(v.data(),1,n,f)!=(size_t)n) exit(1); fclose(f); return v; }
static inline uint64_t rd64(const uint8_t* p){ uint64_t v; memcpy(&v,p,8); return v; }
static inline uint32_t rd32(const uint8_t* p){ uint32_t v; memcpy(&v,p,4); return v; }
static inline uint32_t varint(const uint8_t* b,size_t& p,size_t lim){ uint32_t v=0,sh=0; while(p<lim&&sh<=28){ uint8_t c=b[p++]; v|=(uint32_t)(c&0x7F)<<sh; if(!(c&0x80)) return v; sh+=7; } return v; }

// ACEPX2 commands of one block -> v3 tokens (same as v3proto.cpp)
static void to_v3(const uint8_t* off,size_t osz,const uint8_t* len,size_t nsz,const uint8_t* cmd,size_t csz,
                  std::vector<uint16_t>& LL,std::vector<uint16_t>& ML,std::vector<uint32_t>& D){
    size_t op=0,np=0,cp=0; uint32_t rep[4]={1,2,4,8}, pend=0;
    auto emit=[&](uint32_t ll,uint32_t ml,uint32_t d){
        while(ll>65535){ LL.push_back(65535); ML.push_back(0); D.push_back(0); ll-=65535; }
        uint32_t first=ml>65535?65535:ml; LL.push_back((uint16_t)ll); ML.push_back((uint16_t)first); D.push_back(ml?d:0); ml-=first;
        while(ml){ uint32_t k=ml>65535?65535:ml; LL.push_back(0); ML.push_back((uint16_t)k); D.push_back(d); ml-=k; } };
    while(cp<csz){ uint8_t c=cmd[cp++];
        if(c<0x80){ pend+=c+1; continue; }
        if(c==0xFF){ rep[0]=1;rep[1]=2;rep[2]=4;rep[3]=8; continue; }
        uint32_t l,dist;
        if((c&0xC0)==0x80){ uint32_t ri=(c>>4)&3,lv=c&0x0F; if(lv==0x0F) lv+=varint(len,np,nsz); l=lv+6; dist=rep[ri]; if(ri>0){ for(int i=ri;i>0;i--) rep[i]=rep[i-1]; rep[0]=dist; } }
        else { uint32_t lv=(c==0xFE)?varint(len,np,nsz):(uint32_t)(c&0x3F); l=lv+6; dist=varint(off,op,osz); rep[3]=rep[2];rep[2]=rep[1];rep[1]=rep[0];rep[0]=dist; }
        emit(pend,l,dist); pend=0; }
    if(pend) emit(pend,0,0);
}

__device__ static inline uint32_t w_incl(uint32_t v,uint32_t lane){
    #pragma unroll
    for(int o=1;o<32;o<<=1){ uint32_t t=__shfl_up_sync(0xffffffffu,v,o); if(lane>=(uint32_t)o) v+=t; }
    return v;
}
__global__ void k_v3(const uint8_t* __restrict__ LIT,const BlockOffsets* __restrict__ boffs,
                     const uint16_t* __restrict__ LL,const uint16_t* __restrict__ ML,const uint32_t* __restrict__ D,
                     const uint32_t* __restrict__ T0,uint64_t orig,uint32_t bs,uint8_t* __restrict__ out,
                     uint32_t* __restrict__ ctr,uint32_t nb){
    const uint32_t FULL=0xffffffffu, lane=threadIdx.x&31;
    for(;;){
        uint32_t b=0; if(lane==0) b=atomicAdd(ctr,1u); b=__shfl_sync(FULL,b,0); if(b>=nb) return;
        const uint8_t* lit=LIT+boffs[b].lit_off; uint8_t* dst=out+(uint64_t)b*bs;
        const uint32_t t0=T0[b], t1=T0[b+1];
        uint32_t lbase=0, obase=0;
        for(uint32_t s=t0; s<t1; s+=32){
            const uint32_t i=s+lane; const bool live=i<t1;
            const uint32_t ll=live?LL[i]:0, ml=live?ML[i]:0, d=live?D[i]:0;
            const uint32_t lin=w_incl(ll,lane), oin=w_incl(ll+ml,lane);
            const uint32_t lex=lin-ll, oex=oin-ll-ml, ltot=__shfl_sync(FULL,lin,31), otot=__shfl_sync(FULL,oin,31);
            // short literal runs (< 64 B): all their bytes of the step copied by the warp at once; byte k of the
            // short-run concatenation belongs to the token j with sex_j <= k < sex_j + sl_j (binary search over lanes)
            const uint32_t sl=ll<64?ll:0, sin=w_incl(sl,lane), sex=sin-sl, stot=__shfl_sync(FULL,sin,31);
            for(uint32_t kb=0;kb<stot;kb+=32){                   // uniform trip count: every lane runs the shuffles
                const uint32_t k=kb+lane;
                uint32_t lo=0,hi=31;                               // last j with sex_j <= k
                #pragma unroll
                for(int it=0;it<5;it++){ uint32_t mid=(lo+hi+1)>>1; uint32_t sm=__shfl_sync(FULL,sex,mid); if(sm<=k) lo=mid; else hi=mid-1; }
                const uint32_t sj=__shfl_sync(FULL,sex,lo), lj=__shfl_sync(FULL,lex,lo), oj=__shfl_sync(FULL,oex,lo);
                if(k<stot) dst[obase+oj+(k-sj)]=lit[lbase+lj+(k-sj)];
            }
            // long literal runs: one at a time, the warp copies 16 bytes per lane (as v7-RA)
            uint32_t lm=__ballot_sync(FULL,ll>=64);
            while(lm){ const int j=__ffs(lm)-1; lm&=lm-1;
                const uint32_t n=__shfl_sync(FULL,ll,j), lj=__shfl_sync(FULL,lex,j), oj=__shfl_sync(FULL,oex,j);
                axv_copy16(dst+obase+oj,lit+lbase+lj,n,lane,32); }
            __syncwarp();
            // matches. pos_j = step start + oex_j + ll_j; a match whose source [src, src + min(dist, len)) ends at or
            // before the first match destination of the step reads only final bytes (literals of the step are
            // written): such matches run in parallel; the others in token order, each by the whole warp
            const uint32_t mpos=obase+oex+ll, msrc=mpos-d;
            uint32_t fm=ml?mpos:0xffffffffu;
            #pragma unroll
            for(int o=16;o>0;o>>=1) fm=min(fm,__shfl_xor_sync(FULL,fm,o));
            const bool indep=ml>0 && msrc+min(d,ml)<=fm;
            const uint32_t ms=(indep&&ml<64)?ml:0, mi=w_incl(ms,lane), mex=mi-ms, mtot=__shfl_sync(FULL,mi,31);
            for(uint32_t kb=0;kb<mtot;kb+=32){                   // short independent matches: all bytes at once
                const uint32_t k=kb+lane;
                uint32_t lo=0,hi=31;
                #pragma unroll
                for(int it=0;it<5;it++){ uint32_t mid=(lo+hi+1)>>1; uint32_t xm=__shfl_sync(FULL,mex,mid); if(xm<=k) lo=mid; else hi=mid-1; }
                const uint32_t xj=__shfl_sync(FULL,mex,lo), pj=__shfl_sync(FULL,mpos,lo), dj=__shfl_sync(FULL,d,lo);
                if(k<mtot){ const uint32_t q=k-xj; dst[pj+q]=dst[pj-dj+(q%dj)]; }
            }
            uint32_t mm=__ballot_sync(FULL,indep&&ml>=64);         // long independent matches
            while(mm){ const int j=__ffs(mm)-1; mm&=mm-1;
                const uint32_t pos=__shfl_sync(FULL,mpos,j), len=__shfl_sync(FULL,ml,j), dist=__shfl_sync(FULL,d,j), src=pos-dist;
                if(dist>=len) axv_copy16(dst+pos,dst+src,len,lane,32); else for(uint32_t q=lane;q<len;q+=32) dst[pos+q]=dst[src+(q%dist)]; }
            __syncwarp();
            mm=__ballot_sync(FULL,ml>0&&!indep);                   // dependent matches, in token order
            while(mm){ const int j=__ffs(mm)-1; mm&=mm-1;
                const uint32_t pos=__shfl_sync(FULL,mpos,j), len=__shfl_sync(FULL,ml,j), dist=__shfl_sync(FULL,d,j), src=pos-dist;
                if(dist>=len){ if(len>=64) axv_copy16(dst+pos,dst+src,len,lane,32); else for(uint32_t q=lane;q<len;q+=32) dst[pos+q]=dst[src+q]; }
                else { for(uint32_t q=lane;q<len;q+=32) dst[pos+q]=dst[src+(q%dist)]; }
                __syncwarp(); }
            lbase+=ltot; obase+=otot;
        }
    }
}
__global__ void k_fnv(const uint8_t* buf,size_t n,uint64_t* h){ if(blockIdx.x||threadIdx.x) return; uint64_t x=0xcbf29ce484222325ULL; for(size_t i=0;i<n;i++) x=(x^buf[i])*0x100000001b3ULL; *h=x; }

int main(int argc,char** argv){
    if(argc<3){ fprintf(stderr,"usage: %s streams.bin original [reps=9]\n",argv[0]); return 1; }
    const int R=argc>3?atoi(argv[3]):9;
    std::vector<uint8_t> s=slurp(argv[1]), orig=slurp(argv[2]);
    const uint64_t osz=rd64(&s[12]); const uint32_t bs=rd32(&s[20]), nb=rd32(&s[24]);
    if(osz!=orig.size()){ fprintf(stderr,"size mismatch\n"); return 1; }
    std::vector<BlockOffsets> bo(nb); memcpy(bo.data(),&s[68],64ull*nb);
    uint64_t L=0,O=0,N=0,C=0; for(auto& b:bo){ L=std::max(L,b.lit_off+b.lit_sz); O=std::max(O,b.off_off+b.off_sz); N=std::max(N,b.len_off+b.len_sz); C=std::max(C,b.cmd_off+b.cmd_sz); }
    const uint8_t* lit=&s[68+64ull*nb]; const uint8_t* off=lit+L; const uint8_t* len=off+O; const uint8_t* cmd=len+N;
    std::vector<uint16_t> LLv,MLv; std::vector<uint32_t> Dv,T0(nb+1);
    for(uint32_t b=0;b<nb;b++){ T0[b]=(uint32_t)LLv.size(); to_v3(off+bo[b].off_off,bo[b].off_sz,len+bo[b].len_off,bo[b].len_sz,cmd+bo[b].cmd_off,bo[b].cmd_sz,LLv,MLv,Dv); }
    T0[nb]=(uint32_t)LLv.size(); const size_t NT=LLv.size();
    uint8_t *dL,*dO,*dN,*dC,*dOut; BlockOffsets* dB; uint16_t *dLL,*dML; uint32_t *dD,*dT0,*dCtr; uint64_t* dH;
    CK(cudaMalloc(&dL,L+256)); CK(cudaMalloc(&dO,O+256)); CK(cudaMalloc(&dN,N+256)); CK(cudaMalloc(&dC,C+256)); CK(cudaMalloc(&dOut,osz+256));
    CK(cudaMalloc(&dB,64ull*nb)); CK(cudaMalloc(&dLL,NT*2+64)); CK(cudaMalloc(&dML,NT*2+64)); CK(cudaMalloc(&dD,NT*4+64)); CK(cudaMalloc(&dT0,(nb+1)*4)); CK(cudaMalloc(&dCtr,4)); CK(cudaMalloc(&dH,8));
    CK(cudaMemcpy(dL,lit,L,cudaMemcpyHostToDevice)); CK(cudaMemcpy(dO,off,O,cudaMemcpyHostToDevice)); CK(cudaMemcpy(dN,len,N,cudaMemcpyHostToDevice)); CK(cudaMemcpy(dC,cmd,C,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dB,bo.data(),64ull*nb,cudaMemcpyHostToDevice)); CK(cudaMemcpy(dLL,LLv.data(),NT*2,cudaMemcpyHostToDevice)); CK(cudaMemcpy(dML,MLv.data(),NT*2,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dD,Dv.data(),NT*4,cudaMemcpyHostToDevice)); CK(cudaMemcpy(dT0,T0.data(),(nb+1)*4,cudaMemcpyHostToDevice));
    int dev=0,nsm=0; CK(cudaGetDevice(&dev)); CK(cudaDeviceGetAttribute(&nsm,cudaDevAttrMultiProcessorCount,dev)); cudaDeviceProp pr; CK(cudaGetDeviceProperties(&pr,dev));
    uint64_t hO=0xcbf29ce484222325ULL; for(uint8_t c:orig) hO=(hO^c)*0x100000001b3ULL;
    cudaEvent_t e0,e1; CK(cudaEventCreate(&e0)); CK(cudaEventCreate(&e1));
    auto timed=[&](auto launch){ std::vector<float> t; bool ok=true;
        for(int r=0;r<R;r++){ CK(cudaMemset(dOut,0,osz)); CK(cudaMemset(dCtr,0,4)); CK(cudaEventRecord(e0)); launch(); CK(cudaEventRecord(e1)); CK(cudaEventSynchronize(e1)); CK(cudaGetLastError());
            float ms; CK(cudaEventElapsedTime(&ms,e0,e1)); t.push_back(ms); }
        uint64_t h; k_fnv<<<1,1>>>(dOut,osz,dH); CK(cudaMemcpy(&h,dH,8,cudaMemcpyDeviceToHost)); ok=(h==hO);
        std::sort(t.begin(),t.end()); return std::make_pair(t[t.size()/2],ok); };
    const int TPB=128;
    printf("%s on %s (%d SMs): %llu B, %u blocks of %u, %zu v3 tokens, literals %llu B\n",argv[2],pr.name,nsm,(unsigned long long)osz,nb,bs,NT,(unsigned long long)L);
    float best=1e30f; int bestG=0;
    for(int G:{8,16,32}){ kern_t k=G==8?k_decode_g<8>:G==16?k_decode_g<16>:k_decode_g<32>; int mb=0; CK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&mb,k,TPB,0));
        uint32_t want=(uint32_t)(((uint64_t)nb*G+TPB-1)/TPB), grid=std::min<uint32_t>(want,(uint32_t)nsm*mb); if(!grid) grid=1;
        auto r=timed([&]{ k<<<grid,TPB>>>(dL,dO,dN,dC,dB,osz,bs,dOut,dCtr,nb,nullptr); });
        printf("base v7-RA G=%2d: %.3f ms = %.1f GB/s  %s\n",G,r.first,osz/r.first/1e6,r.second?"bit-perfect":"MISMATCH");
        if(r.second && r.first<best){ best=r.first; bestG=G; } }
    { int mb=0; CK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&mb,k_v3,TPB,0));
      uint32_t want=(uint32_t)(((uint64_t)nb*32+TPB-1)/TPB), grid=std::min<uint32_t>(want,(uint32_t)nsm*mb); if(!grid) grid=1;
      auto r=timed([&]{ k_v3<<<grid,TPB>>>(dL,dB,dLL,dML,dD,dT0,osz,bs,dOut,dCtr,nb); });
      printf("v3 warp-scan   : %.3f ms = %.1f GB/s  %s  (x%.2f vs best base G=%d)\n",r.first,osz/r.first/1e6,r.second?"bit-perfect":"MISMATCH",best/r.first,bestG); }
    return 0;
}
