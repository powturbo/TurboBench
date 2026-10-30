// aceapex_gpu.cu - the whole GPU path on an .aet archive as the CPU encoder wrote it:
//   token streams: nvCOMP batched zstd frames and/or rANS chunks (ADR-018, k_rans: one warp
//   per chunk) -> literal chunks: nvCOMP batched zstd (modes 0, 1) and/or pieces of the open
//   profile (ADR-019, modes 2, 3: k_rans on raw/rANS pieces) -> DNA unpack kernels (mode 1:
//   k_unpack, k_exc; mode 2: k_open_cse, k_open_bases, k_open_exc) -> v7-RA match kernel
//   k_decode_g<G>. All frames and chunks land in the stream buffers. An archive of the open
//   profile (AX_PROFILE=open) is decoded without nvCOMP calls.
// G is chosen at run time by a short probe on this GPU (8/16/32) unless given.
// Two measurements: sequential (H2D, tok, lit, unpack, match, each median of N runs) and a
// pipeline where the H2D of batch k+1 overlaps the entropy decode of batch k.
// Output is hashed (FNV-1a) against the original file: bit-perfect or nothing. A rANS chunk
// that fails its checks (spec 3.1.1) sets a device flag and the run stops (exit 6).
// Build: nvcc -O3 -arch=sm_XX -I<nvcomp>/include -L<nvcomp>/lib64 -l:libnvcomp.so.5 -o aceapex_gpu aceapex_gpu.cu
// Usage: aceapex_gpu <archive.aet> <original> [G=auto|8|16|32] [repeats=7] [batches=4]
#include "src/ax_open_warp.h"   // + ax_rans_warp.h, ax_lit_open.h
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cuda_runtime.h>
#include <nvcomp/zstd.h>

#define CK(x) do{cudaError_t ck_e_=(x); if(ck_e_!=cudaSuccess){fprintf(stderr,"CUDA %s @%d: %s\n",#x,__LINE__,cudaGetErrorString(ck_e_)); exit(2);} }while(0)
#define NV(x) do{nvcompStatus_t nv_s_=(x); if(nv_s_!=nvcompSuccess){fprintf(stderr,"nvcomp %s @%d: status %d\n",#x,__LINE__,(int)nv_s_); exit(3);} }while(0)

#include "src/aceapex_gpu_kernels.cuh"   // block table, match, unpack, rANS pieces, open pack kernels

// ---------------------------------------------------------------- dense-open measurement (not in the format)
// The literal stream coded by components/rans1_v4.c (container "AR1L": per 64 KiB chunk [n u32][cs u32]
// then [flag][K][sym K][bit-packed order-1 table][4 line lengths][nck][checkpoints 9 B x 4 x nck][4 line
// substreams]). One block per chunk: thread 0 parses the table into shared cumulative frequencies, then
// lane = (line, checkpoint segment): 4 x (nck+1) lanes, each decodes <= 4096 symbols serially from the
// state saved at its checkpoint. Order-1 context = previous symbol (index into the chunk's alphabet).
struct R1Desc { uint64_t src, dst; uint32_t cs, n; };
__global__ void __launch_bounds__(32) k_r1(const uint8_t* __restrict__ C, const R1Desc* __restrict__ d, uint8_t* __restrict__ out, uint32_t* __restrict__ err){
    __shared__ uint16_t cum[64][65]; __shared__ uint8_t sym[64], rmap[256];
    __shared__ const uint8_t* base[4]; __shared__ const uint8_t* ckp; __shared__ int nck_s, K_s, raw_s;
    const R1Desc c=d[blockIdx.x]; const uint8_t* p=C+c.src; uint8_t* o=out+c.dst; const uint32_t n=c.n;
    if(threadIdx.x==0){
        raw_s = n==0 || p[0]==1;
        if(!raw_s && (p[1]==0 || p[1]>64)){ atomicAdd(err,1u); raw_s=2; }
        if(!raw_s){ const int K=p[1]; K_s=K; const uint8_t* q=p+2;
            for(int i=0;i<K;i++){ sym[i]=q[i]; rmap[q[i]]=(uint8_t)i; } q+=K;
            uint64_t acc=0; int nb=0;
            const uint8_t* qe=p+c.cs; bool qb=false;      // get(): never past the chunk (a read there sets err)
            auto get=[&](int bits)->uint32_t{ while(nb<bits){ if(q<qe) acc|=(uint64_t)(*q)<<nb; else qb=true; q++; nb+=8; } uint32_t v=(uint32_t)(acc&((1u<<bits)-1)); acc>>=bits; nb-=bits; return v; };
            uint64_t ru=0; for(int i=0;i<K;i++) ru|=(uint64_t)get(1)<<i;
            for(int i=0;i<K;i++){ cum[i][0]=0;
                if(!((ru>>i)&1)){ for(int j=0;j<K;j++) cum[i][j+1]=0; continue; }
                uint64_t cu=0; for(int j=0;j<K;j++) cu|=(uint64_t)get(1)<<j;
                uint32_t cc=0; for(int j=0;j<K;j++){ if((cu>>j)&1) cc+=get(12)+1; cum[i][j+1]=(uint16_t)cc; } }
            uint32_t sl[4]; for(int j=0;j<4;j++){ sl[j]=q[0]|q[1]<<8|q[2]<<16|(uint32_t)q[3]<<24; q+=4; }
            nck_s=*q++; ckp=q; q+=9*4*nck_s; for(int j=0;j<4;j++){ base[j]=q; q+=sl[j]; }
            if(qb || q>p+c.cs){ atomicAdd(err,1u); raw_s=2; } }
    }
    __syncthreads();
    if(raw_s==2) return;
    if(raw_s){ if(n) for(uint32_t i=threadIdx.x;i<n;i+=blockDim.x) o[i]=p[1+i]; return; }
    const int K=K_s, nck=nck_s; const uint32_t q4=n/4, CK=4096; const uint8_t* pe=p+c.cs;
    for(int lane=threadIdx.x; lane<4*(nck+1); lane+=blockDim.x){
        const int j=lane/(nck+1), sg=lane%(nck+1); const uint32_t len = j==3 ? n-3*q4 : q4;
        uint32_t t0=sg*CK, t1= sg<nck ? (sg+1)*CK : len; if(t0>=len) continue;
        uint32_t x, ctx; const uint8_t* pk;
        if(sg==0){ const uint8_t* b=base[j]; x=b[0]|b[1]<<8|b[2]<<16|(uint32_t)b[3]<<24; pk=b+4; ctx=0; }
        else { const uint8_t* e=ckp+(size_t)(j*nck+sg-1)*9; x=e[0]|e[1]<<8|e[2]<<16|(uint32_t)e[3]<<24;
               pk=base[j]+(e[4]|e[5]<<8|e[6]<<16|(uint32_t)e[7]<<24); ctx=rmap[e[8]]; }
        uint8_t* ol=o+(size_t)j*q4;
        for(uint32_t t=t0;t<t1;t++){
            const uint32_t m=x&4095u; const uint16_t* r=cum[ctx];
            int lo=0, hi=K;                                   // first s with r[s+1] > m
            while(lo<hi){ const int mid=(lo+hi)>>1; if(r[mid+1]<=m) lo=mid+1; else hi=mid; }
            const uint32_t st=r[lo], f=r[lo+1]-st;
            x=f*(x>>12)+m-st; ol[t]=sym[lo]; ctx=(uint32_t)lo;
            if(x<(1u<<15)){ if(pk+2>pe){ atomicAdd(err,1u); break; } x=(x<<16)|pk[0]|((uint32_t)pk[1]<<8); pk+=2; }
        }
    }
}

// ---------------------------------------------------------------- dense-open v2 (measurement, not in the format)
// The literal stream coded by components/rans1_seg.c (container "AR2L": per 64 KiB chunk [n u32][cs u32] then
// [flag][K][sym K][bit-packed order-1 table][NS=32 x u16 substream lengths][32 substreams]). One warp per chunk,
// lane = segment: segment s holds symbols [s*q, s*q+q) (q = (n/32)&~3, the last one runs to n) with its own state
// and substream; the context restarts at alphabet symbol 0 at the segment start. Symbol lookup by variant V:
//   0: slot->symbol table per context in shared memory, one byte per slot (K x 4096 B)
//   1: the same, two slots per byte (K x 2048 B)
//   2: no table: the context's cumulative row (16 x u16 in 8 words) against the slot, __vcmpleu2 + popc
//      (shared ~1 KB per block: every chunk of chr1 resident at once)
//   3: wide chunks (K > 16, e.g. T2T), binary search over a K x (K+1) row as k_r1 does
// Variants 0-2 take K <= 16; the host sends wider chunks to variant 3 in a second launch.
// Output in 32-bit words (segment starts are multiples of 4).
struct R2Desc { uint64_t src, dst; uint32_t cs, n; };
template<int V> __global__ void __launch_bounds__(32) k_r2(const uint8_t* __restrict__ C, const R2Desc* __restrict__ d, uint8_t* __restrict__ out, uint32_t* __restrict__ err){
    constexpr int KC = V==3 ? 64 : 16;
    extern __shared__ uint8_t slot[];                 // V 0/1: [K][4096 >> V]
    __shared__ uint16_t cum[KC][KC+1]; __shared__ uint8_t sym[KC]; __shared__ const uint8_t* lens; __shared__ int K_s, raw_s;
    __shared__ uint32_t cw[V==2 ? 16 : 1][9];         // V 2: cum[i][1..16] as 8 words, 0xFFFF past K, row stride 9
    const R2Desc c=d[blockIdx.x]; const uint8_t* p=C+c.src; uint8_t* o=out+c.dst; const uint32_t n=c.n; const int ln=threadIdx.x;
    if(ln==0){
        raw_s = n==0 || p[0]==1;
        if(!raw_s){ const int K=p[1]; K_s=K; const uint8_t* q=p+2;
            if(K>KC || K==0){ atomicAdd(err,1u); raw_s=2; }
            else {
            for(int i=0;i<K;i++) sym[i]=q[i];
            q+=K;
            uint64_t acc=0; int nb=0;
            const uint8_t* qe=p+c.cs; bool qb=false;      // get(): never past the chunk (a read there sets err)
            auto get=[&](int bits)->uint32_t{ while(nb<bits){ if(q<qe) acc|=(uint64_t)(*q)<<nb; else qb=true; q++; nb+=8; } uint32_t v=(uint32_t)(acc&((1u<<bits)-1)); acc>>=bits; nb-=bits; return v; };
            uint64_t ru=0; for(int i=0;i<K;i++) ru|=(uint64_t)get(1)<<i;
            for(int i=0;i<K;i++){ cum[i][0]=0;
                if(!((ru>>i)&1)){ for(int j=0;j<K;j++) cum[i][j+1]=0; continue; }
                uint64_t cu=0; for(int j=0;j<K;j++) cu|=(uint64_t)get(1)<<j;
                uint32_t cc=0; for(int j=0;j<K;j++){ if((cu>>j)&1) cc+=get(12)+1; cum[i][j+1]=(uint16_t)cc; } }
            for(int i=0;i<K;i++) if(((ru>>i)&1) && cum[i][K]!=4096) qb=true;   // a used row must sum to 4096 (the slot searches rely on it)
            if(qb || q>=qe || *q++!=32) { atomicAdd(err,1u); raw_s=2; }
            lens=q; }
        }
    }
    __syncthreads();
    if(raw_s==2) return;
    if(raw_s){ if(n) for(uint32_t i=ln;i<n;i+=32) o[i]=p[1+i]; return; }
    const int K=K_s;
    if(V<=1){                                         // slot table: lane ln fills slots [128 ln, 128 ln + 128) of every used row
        for(int i=0;i<K;i++){ if(cum[i][K]==0) continue;
            uint32_t m0=ln*128; int j=0; while(j<K-1 && cum[i][j+1]<=m0) j++;
            if(V==1){ uint32_t* w=(uint32_t*)(slot+i*2048+ln*64);
                for(int k=0;k<16;k++){ uint32_t v=0; for(int b=0;b<8;b++){ uint32_t m=m0+k*8+b; while(j<K-1 && cum[i][j+1]<=m) j++; v|=(uint32_t)j<<(4*b); } w[k]=v; } }
            else { uint32_t* w=(uint32_t*)(slot+i*4096+ln*128);
                for(int k=0;k<32;k++){ uint32_t v=0; for(int b=0;b<4;b++){ uint32_t m=m0+k*4+b; while(j<K-1 && cum[i][j+1]<=m) j++; v|=(uint32_t)j<<(8*b); } w[k]=v; } }
        }
    }
    if(V==2){                                         // lane ln < 16*8: row ln>>3, word ln&7 (and + 32 ...)
        for(int e=ln;e<K*8;e+=32){ const int i=e>>3, w=e&7; const int j0=2*w+1, j1=2*w+2;
            const uint32_t lo = j0<=K ? cum[i][j0] : 0xFFFFu, hi = j1<=K ? cum[i][j1] : 0xFFFFu;
            cw[i][w]=lo|hi<<16; }
    }
    // own substream: offset = exclusive prefix of the lengths
    const uint32_t L=lens[2*ln]|(uint32_t)lens[2*ln+1]<<8; uint32_t incl=L;
    for(int k=1;k<32;k<<=1){ uint32_t y=__shfl_up_sync(0xffffffffu,incl,k); if(ln>=k) incl+=y; }
    const uint8_t* b=lens+64+(incl-L);
    if(lens+64+__shfl_sync(0xffffffffu,incl,31)>p+c.cs){ if(ln==0) atomicAdd(err,1u); return; }   // substreams inside the chunk (uniform)
    __syncthreads();
    const uint32_t q=(n/32)&~3u, lo=ln*q, len= ln==31 ? n-31*q : q;
    if(len && L<4){ atomicAdd(err,1u); return; }        // no initial state (after the last block-wide barrier)
    uint32_t x= L>=4 ? b[0]|b[1]<<8|b[2]<<16|(uint32_t)b[3]<<24 : 0; const uint8_t* pk=b+4; uint32_t ctx=0, w=0;
    uint32_t* ow=(uint32_t*)(o+lo);
    for(uint32_t t=0;t<len;t++){
        const uint32_t m=x&4095u;
        uint32_t j;
        if(V==0) j=slot[ctx*4096+m];
        else if(V==1) j=(slot[ctx*2048+(m>>1)]>>((m&1)*4))&15u;
        else if(V==2){ const uint32_t mm=m|m<<16; const uint32_t* r=cw[ctx]; uint32_t cnt=0;
            #pragma unroll
            for(int k=0;k<8;k++) cnt+=__popc(__vcmpleu2(r[k],mm));
            j=cnt>>4; }
        else { const uint16_t* r=cum[ctx]; int a=0, h=K; while(a<h){ const int mid=(a+h)>>1; if(r[mid+1]<=m) a=mid+1; else h=mid; } j=(uint32_t)a; }
        const uint32_t st=cum[ctx][j], f=cum[ctx][j+1]-st;
        x=f*(x>>12)+m-st; ctx=j;
        w|=(uint32_t)sym[j]<<(8*(t&3));
        if((t&3)==3){ ow[t>>2]=w; w=0; }
        if(x<(1u<<15)){ if(pk+2>b+L){ atomicAdd(err,1u); break; } x=(x<<16)|pk[0]|((uint32_t)pk[1]<<8); pk+=2; }
    }
    for(uint32_t t=len&~3u;t<len;t++) o[lo+t]=(uint8_t)(w>>(8*(t&3)));
    if(pk!=b+L) atomicAdd(err,1u);
}
__global__ void k_cmp(const uint8_t* a, const uint8_t* b, size_t n, unsigned long long* bad){
    unsigned long long c=0; for(size_t i=blockIdx.x*(size_t)blockDim.x+threadIdx.x;i<n;i+=(size_t)gridDim.x*blockDim.x) c+= a[i]!=b[i];
    if(c) atomicAdd(bad,c);
}
__global__ void k_set(uint32_t* p, uint32_t v){ *p=v; }
// output check: FNV-1a of every 1 MiB chunk, one thread per chunk (one thread over the whole output took about a
// minute per check on T2T), then FNV-1a of the chunk hashes on the host; the original is hashed the same way
#define FNV_CH (1u<<20)
__global__ void k_fnv(const uint8_t* buf, size_t n, uint64_t* out){
    const size_t c=blockIdx.x*(size_t)blockDim.x+threadIdx.x, b=c*FNV_CH; if(b>=n) return;
    const size_t e=b+FNV_CH<n?b+FNV_CH:n; uint64_t h=0xcbf29ce484222325ULL; for(size_t i=b;i<e;i++) h=(h^buf[i])*0x100000001b3ULL; out[c]=h;
}
static uint64_t fnv_of_chunks(const std::vector<uint64_t>& v){ uint64_t h=0xcbf29ce484222325ULL;
    for(uint64_t x:v) for(int k=0;k<8;k++) h=(h^((x>>(8*k))&0xFF))*0x100000001b3ULL; return h; }

// ---------------------------------------------------------------- archive parsing
static std::vector<uint8_t> slurp(const char* p){ FILE* f=fopen(p,"rb"); if(!f){perror(p); exit(1);} fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET); std::vector<uint8_t> v(n); if(fread(v.data(),1,n,f)!=(size_t)n){fprintf(stderr,"short read %s\n",p); exit(1);} fclose(f); return v; }
static inline uint64_t rd64(const uint8_t* p){ uint64_t v; memcpy(&v,p,8); return v; }
static inline uint32_t rd32(const uint8_t* p){ uint32_t v; memcpy(&v,p,4); return v; }
enum { K_PLAIN=0, K_SEQ=1, K_CSE=2, K_GAP=3, K_VAL=4 };
// a zstd frame: bytes in the archive, decoded size, where the output goes (stream 0..3 or
// scratch for DNA sub-frames), block index it belongs to (for batching)
struct Job { const uint8_t* src; size_t csz; size_t osz; int stream; int kind; uint64_t dst_off; uint32_t chunk; };
struct RawCopy { const uint8_t* src; size_t n; int stream; uint64_t dst_off; };
// a piece for k_rans: class 0 = token chunk, 1..4 = seq/cse/gap/val of an open DNA pack, 5 = open
// plain chunk; stream -1 = the open scratch buffer
enum { P_TOK=0, P_SEQ=1, P_CSE=2, P_GAP=3, P_VAL=4, P_PLAIN=5, P_NCLS=6 };
struct RansJob { const uint8_t* src; size_t csz, n; int stream; uint64_t dst_off; uint32_t mode; int cls; uint64_t lo; };   // lo: literal-stream offset of an open piece's chunk
struct OpenChunk { uint32_t chunk, raw, ncse, ngap, nexc; uint64_t off[4]; };
struct DnaChunk { uint32_t chunk; size_t raw; uint32_t nexc; bool has_gap, has_val; size_t job[4]; };

static float median_ms(std::vector<float> v){ std::sort(v.begin(),v.end()); return v[v.size()/2]; }

int main(int argc, char** argv){
    const char* R1=nullptr;                          // --dense-lit=<file>: literal stream coded by rans1_v4 (measurement)
    const char* R2=nullptr;                          // --dense2-lit=<file>: literal stream coded by rans1_seg (dense-open v2, measurement)
    int PK=0;                                        // --pipeline[=K]: K block batches, H2D of batch k+1 under the decode of batch k
    { int j=1; for(int i=1;i<argc;i++){ if(!strncmp(argv[i],"--dense-lit=",12)) R1=argv[i]+12; else if(!strncmp(argv[i],"--dense2-lit=",13)) R2=argv[i]+13; else if(!strncmp(argv[i],"--pipeline",10)){ PK = argv[i][10]!='=' ? 8 : !strcmp(argv[i]+11,"auto") ? -1 : atoi(argv[i]+11); if(PK==0) PK=1; } else argv[j++]=argv[i]; } argc=j; }
    if(argc<3){ fprintf(stderr,"usage: %s <archive.aet> <original> [G=auto|8|16|32] [repeats=7] [batches=4] [--pipeline[=K|auto]]\n",argv[0]); return 1; }
    int Gwant = (argc>3 && strcmp(argv[3],"auto")!=0) ? atoi(argv[3]) : 0;
    int reps = argc>4 ? atoi(argv[4]) : 7; int NB = argc>5 ? atoi(argv[5]) : 4; if(NB<1) NB=1;
    std::vector<uint8_t> a=slurp(argv[1]);
    if(a.size()<68 || memcmp(a.data(),"ACEPX2\0\0",8)){ fprintf(stderr,"not an ACEPX2 archive\n"); return 1; }
    uint64_t orig=rd64(&a[12]); uint32_t bs=rd32(&a[20]), nb=rd32(&a[24]);
    uint64_t zl=rd64(&a[36]), zo=rd64(&a[44]), zn=rd64(&a[52]), zc=rd64(&a[60]);
    const uint8_t* zs[4]; uint64_t zsz[4]={zl,zo,zn,zc}; size_t p=68+64ull*nb; for(int i=0;i<4;i++){ zs[i]=&a[p]; p+=zsz[i]; }
    std::vector<BlockOffsets> bo(nb); memcpy(bo.data(),&a[68],64ull*nb);
    uint64_t ssz[4]={0,0,0,0}; uint64_t chunk[4]={0,0,0,0};
    std::vector<Job> jobs; std::vector<RawCopy> raws; std::vector<DnaChunk> dna; std::vector<RansJob> rans; std::vector<OpenChunk> opn; uint64_t oscr=0;
    for(int st=1; st<4; st++){                       // off/len/cmd
        const uint8_t* z=zs[st]; if(zsz[st]<8){ continue; } uint64_t w=rd64(z); uint64_t osz=w&((1ull<<48)-1);
        uint64_t ch=((w>>48)&0x7fff)*4096; if(!ch){ const char* e=getenv("FSE_CHUNK"); ch=e?strtoull(e,0,10):524288; }
        ssz[st]=osz; chunk[st]=ch; uint64_t nc=(osz+ch-1)/ch; size_t pos=8+8*nc;
        if(pos>zsz[st]){ fprintf(stderr,"chunk table of stream %d past the stream\n",st); return 1; }
        for(uint64_t i=0;i<nc;i++){ uint64_t cs=rd64(z+8+8*i); size_t raw=(size_t)std::min<uint64_t>(ch,osz-i*ch);
            if(((cs>>48)&0x3fff) || ((cs>>63)&&((cs>>62)&1))){ fprintf(stderr,"corrupt chunk entry, stream %d chunk %llu\n",st,(unsigned long long)i); return 1; }
            size_t csz=(cs>>63)?raw:(size_t)(cs&((1ull<<48)-1));
            if(csz>zsz[st]-pos){ fprintf(stderr,"chunk %llu of stream %d past the stream\n",(unsigned long long)i,st); return 1; }
            if(cs>>63) raws.push_back({z+pos,raw,st,i*ch});
            else if((cs>>62)&1) rans.push_back({z+pos,csz,raw,st,i*ch,1,P_TOK});   // rANS token chunk (ADR-018): k_rans on the device
            else jobs.push_back({z+pos,csz,raw,st,K_PLAIN,i*ch,(uint32_t)i});
            pos+=csz; }
    }
    const size_t NT=jobs.size();                     // jobs [0,NT) are token frames, [NT,N) literal frames
    { const uint8_t* z=zs[0]; uint64_t h=rd64(z); bool zl62=h&(1ull<<62); bool chunked=h&(1ull<<61); bool tagged=h&(1ull<<60);
      if(!zl62){ fprintf(stderr,"literal stream without bit62 (FSE layout) not handled here\n"); return 1; }
      uint64_t sz=h&~((1ull<<62)|(1ull<<61)|(1ull<<60)); uint64_t CH=chunked?rd64(z+8):(sz+3)/4; uint64_t NW=chunked?(sz+CH-1)/CH:4;
      size_t pos=(chunked?16:8)+8*NW; ssz[0]=sz; chunk[0]=CH;
      if(pos>zsz[0]){ fprintf(stderr,"literal chunk table past the stream\n"); return 1; }
      for(uint64_t t=0;t<NW;t++){ uint64_t o=t*CH; size_t raw=(size_t)(o>=sz?0:(o+CH<=sz?CH:sz-o)); size_t csz=rd64(z+(chunked?16:8)+8*t); const uint8_t* c=z+pos;
        if(csz>zsz[0]-pos){ fprintf(stderr,"literal chunk %llu past the stream\n",(unsigned long long)t); return 1; }
        pos+=csz;
        if(!raw) continue;
        if(tagged && csz && c[0]==2){                      // open DNA pack: framing checked here, content on the device
            AxoParts P; if(csz<2 || axo_parse(c+1,csz-1,(uint32_t)raw,&P)){ fprintf(stderr,"literal chunk %llu: bad open DNA pack framing\n",(unsigned long long)t); return 1; }
            OpenChunk oc; oc.chunk=(uint32_t)t; oc.raw=(uint32_t)raw; oc.ncse=P.ncse; oc.ngap=P.ngap; oc.nexc=P.nexc;
            for(int q=0;q<4;q++){ oc.off[q]=oscr; oscr+=((uint64_t)P.n[q]+64)&~63ull;
                if(P.n[q]) rans.push_back({c+1+P.off[q],P.h[q],P.n[q],-1,oc.off[q],P.mode[q],P_SEQ+q,o}); }
            opn.push_back(oc); continue; }
        if(tagged && csz && c[0]==3){                      // open plain: one piece into the literal stream
            if(csz<2 || c[1]>1 || (c[1]==0 && csz-2!=raw)){ fprintf(stderr,"literal chunk %llu: bad open piece\n",(unsigned long long)t); return 1; }
            rans.push_back({c+2,csz-2,raw,0,o,c[1],P_PLAIN}); continue; }
        if(tagged && c[0]==1){ uint32_t nexc=rd32(c+1),h1=rd32(c+5),h2=rd32(c+9),h3=rd32(c+13),h4=rd32(c+17); const uint8_t* f=c+21;
            DnaChunk d; d.chunk=(uint32_t)t; d.raw=raw; d.nexc=nexc; d.has_gap=h3!=0; d.has_val=h4!=0; for(int k=0;k<4;k++) d.job[k]=(size_t)-1;
            d.job[0]=jobs.size(); jobs.push_back({f,h1,(raw+3)/4,0,K_SEQ,0,(uint32_t)t}); f+=h1;
            d.job[1]=jobs.size(); jobs.push_back({f,h2,(raw+7)/8,0,K_CSE,0,(uint32_t)t}); f+=h2;
            if(h3){ d.job[2]=jobs.size(); jobs.push_back({f,h3,(size_t)nexc*4,0,K_GAP,0,(uint32_t)t}); } f+=h3;
            if(h4){ d.job[3]=jobs.size(); jobs.push_back({f,h4,(size_t)nexc,0,K_VAL,0,(uint32_t)t}); }
            dna.push_back(d); }
        else jobs.push_back({tagged?c+1:c, tagged?csz-1:csz, raw, 0, K_PLAIN, o, (uint32_t)t}); }
    }
    size_t N=jobs.size(), cbytes=0, scratch=0, maxo=0; std::vector<size_t> coff(N), soff(N);
    for(size_t i=0;i<N;i++){ coff[i]=cbytes; cbytes+=jobs[i].csz; maxo=std::max(maxo,jobs[i].osz);
        if(jobs[i].kind!=K_PLAIN){ soff[i]=scratch; scratch+=(jobs[i].osz+63)&~size_t(63); } }
    // pieces (rANS token chunks, open-profile pieces) grouped by class; their bytes follow the frames in dC
    std::stable_sort(rans.begin(),rans.end(),[](const RansJob& x,const RansJob& y){ return x.cls<y.cls; });
    const size_t zbytes=cbytes, NR=rans.size(); std::vector<RansDesc> hr(NR); size_t cls_off[P_NCLS+1]={0};
    for(size_t k=0;k<NR;k++){ hr[k]={cbytes,nullptr,(uint64_t)rans[k].n,(uint32_t)rans[k].csz,(uint32_t)rans[k].mode,(uint32_t)rans[k].cls,0u}; cbytes+=rans[k].csz; cls_off[rans[k].cls+1]++; }
    for(int q=0;q<P_NCLS;q++) cls_off[q+1]+=cls_off[q];
    const size_t NTOK=cls_off[P_TOK+1]-cls_off[P_TOK];
    printf("archive %s: orig=%llu block=%u nb=%u; %zu zstd frames (%zu token, %zu literal; %.1f MB), pieces %.1f MB: %zu token rANS chunks, open pack seq/cse/gap/val %zu/%zu/%zu/%zu, open plain %zu; %zu raw chunks, %zu DNA chunks (zstd), %zu open DNA chunks; streams lit/off/len/cmd = %.1f/%.1f/%.1f/%.1f MB\n",
        argv[1],(unsigned long long)orig,bs,nb,N,NT,N-NT,zbytes/1e6,(cbytes-zbytes)/1e6,NTOK,cls_off[2]-cls_off[1],cls_off[3]-cls_off[2],cls_off[4]-cls_off[3],cls_off[5]-cls_off[4],cls_off[6]-cls_off[5],
        raws.size(),dna.size(),opn.size(),ssz[0]/1e6,ssz[1]/1e6,ssz[2]/1e6,ssz[3]/1e6);

    // ---- device buffers: streams, scratch, compressed input, tables, output
    uint8_t *dS[4], *dScr, *dC, *dOUT; BlockOffsets* dBO; uint32_t* dCTR; DnaDesc* dDD; uint64_t* dH;
    for(int i=0;i<4;i++) CK(cudaMalloc(&dS[i],ssz[i]+256));
    CK(cudaMalloc(&dScr,scratch+256)); CK(cudaMalloc(&dC,cbytes+256)); CK(cudaMalloc(&dOUT,orig+256));
    CK(cudaMalloc(&dBO,64ull*nb)); CK(cudaMalloc(&dCTR,4)); CK(cudaMalloc(&dH,8)); CK(cudaMalloc(&dDD,(dna.size()+1)*sizeof(DnaDesc)));
    CK(cudaMemcpy(dBO,bo.data(),64ull*nb,cudaMemcpyHostToDevice));
    uint8_t* hc; CK(cudaHostAlloc(&hc,cbytes+1,cudaHostAllocDefault)); for(size_t i=0;i<N;i++) memcpy(hc+coff[i],jobs[i].src,jobs[i].csz);
    uint8_t* dOS; CK(cudaMalloc(&dOS,oscr+256));                      // open scratch: seq/cse/gap/val of every open chunk
    for(size_t k=0;k<NR;k++){ memcpy(hc+hr[k].src,rans[k].src,rans[k].csz); hr[k].dst=(rans[k].stream<0?dOS:dS[rans[k].stream])+rans[k].dst_off; }
    RansDesc* dRD; uint32_t* dErr; CK(cudaMalloc(&dRD,(NR+1)*sizeof(RansDesc))); CK(cudaMalloc(&dErr,16));
    if(NR) CK(cudaMemcpy(dRD,hr.data(),NR*sizeof(RansDesc),cudaMemcpyHostToDevice));
    { const uint32_t e0[4]={0u,0xffffffffu,0u,0xffffffffu}; CK(cudaMemcpy(dErr,e0,16,cudaMemcpyHostToDevice)); }
    const size_t NO=opn.size(); std::vector<OpenDesc> ho(NO); OpenDesc* dOD; CK(cudaMalloc(&dOD,(NO+1)*sizeof(OpenDesc)));
    uint64_t nends=0; for(const OpenChunk& q:opn) nends+=q.ncse;          // run ends: at most one per case-run byte
    uint32_t *dEnds, *dNrun; CK(cudaMalloc(&dEnds,(nends+1)*4)); CK(cudaMalloc(&dNrun,(NO+1)*4));
    // exception positions of every open chunk (k_open_cg -> k_open_bases_x), chunk k's from heo[k]
    std::vector<uint64_t> heo(NO+1,0); for(size_t k=0;k<NO;k++) heo[k+1]=heo[k]+opn[k].nexc;
    uint32_t* dEpos; CK(cudaMalloc(&dEpos,(heo[NO]+1)*4));
    for(size_t k=0;k<NO;k++){ const OpenChunk& q=opn[k]; ho[k]={dOS+q.off[0],dOS+q.off[1],dOS+q.off[2],dOS+q.off[3],dS[0]+(uint64_t)q.chunk*chunk[0],nullptr,dNrun+k,dEpos+heo[k],q.raw,q.ncse,q.ngap,q.nexc}; }
    { uint64_t e=0; for(size_t k=0;k<NO;k++){ ho[k].ends=dEnds+e; e+=opn[k].ncse; } }
    if(NO) CK(cudaMemcpy(dOD,ho.data(),NO*sizeof(OpenDesc),cudaMemcpyHostToDevice));
    for(auto& r:raws) CK(cudaMemcpy(dS[r.stream]+r.dst_off,r.src,r.n,cudaMemcpyHostToDevice));
    std::vector<const void*> hcp(N); std::vector<void*> hop(N); std::vector<size_t> hcs(N), hos(N);
    for(size_t i=0;i<N;i++){ const Job& j=jobs[i]; hcp[i]=dC+coff[i]; hcs[i]=j.csz; hos[i]=j.osz;
        hop[i]= j.kind==K_PLAIN ? (void*)(dS[j.stream]+j.dst_off) : (void*)(dScr+soff[i]); }
    const void** dcp; void** dop; size_t *dcs,*dos,*dact; nvcompStatus_t* dst;
    CK(cudaMalloc(&dcp,N*8)); CK(cudaMalloc(&dop,N*8)); CK(cudaMalloc(&dcs,N*8)); CK(cudaMalloc(&dos,N*8)); CK(cudaMalloc(&dact,N*8)); CK(cudaMalloc(&dst,N*sizeof(nvcompStatus_t)));
    CK(cudaMemcpy(dcp,hcp.data(),N*8,cudaMemcpyHostToDevice)); CK(cudaMemcpy(dop,hop.data(),N*8,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dcs,hcs.data(),N*8,cudaMemcpyHostToDevice)); CK(cudaMemcpy(dos,hos.data(),N*8,cudaMemcpyHostToDevice));
    std::vector<DnaDesc> hd(dna.size());
    for(size_t k=0;k<dna.size();k++){ const DnaChunk& d=dna[k]; auto P=[&](int q)->const uint8_t*{ return d.job[q]==(size_t)-1?nullptr:dScr+soff[d.job[q]]; };
        hd[k]={P(0),P(1),P(2),P(3), dS[0]+(uint64_t)d.chunk*chunk[0], (uint32_t)d.raw, d.nexc}; }
    if(!hd.empty()) CK(cudaMemcpy(dDD,hd.data(),hd.size()*sizeof(DnaDesc),cudaMemcpyHostToDevice));
    nvcompBatchedZstdDecompressOpts_t opts=nvcompBatchedZstdDecompressDefaultOpts;
    // temp for all frames in one call, and for the token / literal frames as two calls (stage timing)
    size_t temp=0, tmp2=0; const size_t tot_out=scratch+ssz[0]+ssz[1]+ssz[2]+ssz[3];
    if(N) NV(nvcompBatchedZstdDecompressGetTempSizeAsync(N,maxo,opts,&temp,tot_out));
    if(NT){ NV(nvcompBatchedZstdDecompressGetTempSizeAsync(NT,maxo,opts,&tmp2,tot_out)); temp=std::max(temp,tmp2); }
    if(N>NT){ NV(nvcompBatchedZstdDecompressGetTempSizeAsync(N-NT,maxo,opts,&tmp2,tot_out)); temp=std::max(temp,tmp2); }
    void* dtemp; CK(cudaMalloc(&dtemp,temp+256));
    cudaStream_t s0,s1; CK(cudaStreamCreate(&s0)); CK(cudaStreamCreate(&s1));
    cudaEvent_t e0,e1; CK(cudaEventCreate(&e0)); CK(cudaEventCreate(&e1));
    auto elapsed=[&](cudaStream_t s, auto fn){ CK(cudaEventRecord(e0,s)); fn(); CK(cudaEventRecord(e1,s)); CK(cudaEventSynchronize(e1)); float ms=0; CK(cudaEventElapsedTime(&ms,e0,e1)); return ms; };

    // ---- stage functions
    auto h2d=[&](cudaStream_t s){ CK(cudaMemcpyAsync(dC,hc,cbytes,cudaMemcpyHostToDevice,s)); };
    auto zstd_rng=[&](cudaStream_t s, size_t i0, size_t i1){ if(i1>i0) NV(nvcompBatchedZstdDecompressAsync(dcp+i0,dcs+i0,dos+i0,dact+i0,i1-i0,dtemp,temp,dop+i0,opts,dst+i0,s)); };
    auto zstd_all=[&](cudaStream_t s){ zstd_rng(s,0,N); };
    // the open path (same bytes either way; measured against each other below, "[open variants]"):
    // AX_OPEN_SEQ windowed refill in k_rans (default 1: Blackwell af2c70c chr1 seq 0.309 -> 0.238 ms; 0 = refill from
    // global); AX_OPEN_EXC exception positions in the case-run kernel and the bytes in the bases store (default 1; 0 =
    // k_open_cse, k_open_bases, k_open_exc)
    // AX_OPEN_SHB (with EXC): the bases kernel with the block's run ends / exception positions in shared memory
    // (k_open_bases_s; default 1 since a7d161d: Blackwell T2T unpack 8.948 -> 7.458 ms, chr1 0.605 -> 0.583)
    int VS=getenv("AX_OPEN_SEQ")?atoi(getenv("AX_OPEN_SEQ")):1, VE=getenv("AX_OPEN_EXC")?atoi(getenv("AX_OPEN_EXC")):1, VH=getenv("AX_OPEN_SHB")?atoi(getenv("AX_OPEN_SHB")):1;
    const int VS0=VS, VE0=VE, VH0=VH;
    auto pieces=[&](cudaStream_t s, int c0, int c1){ const size_t i0=cls_off[c0], n=cls_off[c1]-i0;
        if(n){ if(VS) k_rans<1><<<(unsigned)((n+AXW_WARPS-1)/AXW_WARPS),32*AXW_WARPS,0,s>>>(dC,dRD+i0,(uint32_t)n,dErr);
               else   k_rans<0><<<(unsigned)((n+AXW_WARPS-1)/AXW_WARPS),32*AXW_WARPS,0,s>>>(dC,dRD+i0,(uint32_t)n,dErr); } };
    auto rans_all=[&](cudaStream_t s){ pieces(s,0,P_NCLS); };
    auto tok=[&](cudaStream_t s){ zstd_rng(s,0,NT); pieces(s,P_TOK,P_TOK+1); };   // token streams: zstd frames and/or rANS chunks
    auto lit=[&](cudaStream_t s){ zstd_rng(s,NT,N); pieces(s,P_SEQ,P_NCLS); };    // literal zstd frames and open pieces
    // fail-closed: a piece or an open DNA chunk that failed its checks stops the run before any number is printed as valid
    auto rans_check=[&](const char* tag){ uint32_t e[4]; CK(cudaMemcpy(e,dErr,16,cudaMemcpyDeviceToHost));
        if(e[0]){ fprintf(stderr,"[%s] rANS/pieces: %u of %zu failed the spec 3.1.1/3.4 checks (first: piece %u) - archive rejected\n",tag,e[0],NR,e[1]); exit(6); }
        if(e[2]){ fprintf(stderr,"[%s] open DNA pack: %u of %zu chunks failed the spec 3.4 checks (first: %u) - archive rejected\n",tag,e[2],NO,e[3]); exit(6); } };
    dim3 g1((unsigned)std::max<size_t>(dna.size(),1), (unsigned)((chunk[0]/AXU_PER+255)/256));
    dim3 go((unsigned)std::max<size_t>(NO,1), (unsigned)((chunk[0]/16+255)/256));
    auto un_seq=[&](cudaStream_t s){ if(NO){ if(VE && VH) k_open_bases_s<<<go,256,0,s>>>(dOD); else if(VE) k_open_bases_x<<<go,256,0,s>>>(dOD); else k_open_bases<<<go,256,0,s>>>(dOD); } };   // bases + case (+ exceptions)
    auto un_cse=[&](cudaStream_t s){ if(NO){ if(VE) k_open_cg<<<(unsigned)NO,AXO_NT,0,s>>>(dOD,dErr+2);  // case runs -> run ends (+ exception positions)
                                             else k_open_cse<<<(unsigned)NO,AXO_NT,0,s>>>(dOD,dErr+2); } };
    auto un_exc=[&](cudaStream_t s){ if(NO && !VE) k_open_exc<<<(unsigned)NO,AXO_NT,0,s>>>(dOD,dErr+2); };
    // fused (literal chunks <= 64 KiB, every open chunk has its seq piece at cls_off[P_SEQ]+k): pieces without seq,
    // case runs, seq+bases in one kernel, exceptions - the same bytes as lit + unpack
    // closed 30.09 (Blackwell: fused chr1 2.18 vs 1.47 ms, T2T 19.36 vs 13.23 - one warp decodes the seq piece while
    // the block waits); kept for AX_GPU_FUSED=1 only
    const bool canF = getenv("AX_GPU_FUSED") && NO && chunk[0]<=65536 && cls_off[P_SEQ+1]-cls_off[P_SEQ]==NO && dna.empty();
    auto fused=[&](cudaStream_t s){ zstd_rng(s,NT,N); pieces(s,P_CSE,P_NCLS); un_cse(s);
        if(NO) k_open_seqb<<<(unsigned)NO,256,0,s>>>(dC,dRD+cls_off[P_SEQ],dOD,dErr); un_exc(s); };
    auto unpack=[&](cudaStream_t s){ if(!dna.empty()){ k_unpack<<<g1,256,0,s>>>(dDD); k_exc<<<(unsigned)dna.size(),256,0,s>>>(dDD); }
        un_cse(s); un_seq(s); un_exc(s); };
    const int TPB=128; int dev=0,nsm=0; CK(cudaGetDevice(&dev)); CK(cudaDeviceGetAttribute(&nsm,cudaDevAttrMultiProcessorCount,dev));
    cudaDeviceProp prop; CK(cudaGetDeviceProperties(&prop,dev));
    auto match=[&](int G, cudaStream_t s, uint32_t b0, uint32_t b1){
        kern_t k = G==8?k_decode_g<8>:G==16?k_decode_g<16>:k_decode_g<32>;
        int maxblk=0; CK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&maxblk,k,TPB,0));
        uint64_t lanes=(uint64_t)(b1-b0)*G; uint32_t want=(uint32_t)((lanes+TPB-1)/TPB), grid=(uint32_t)nsm*maxblk; if(grid>want) grid=want; if(grid<1) grid=1;
        CK(cudaMemcpyAsync(dCTR,&b0,4,cudaMemcpyHostToDevice,s));
        k<<<grid,TPB,0,s>>>(dS[0],dS[1],dS[2],dS[3],dBO,orig,bs,dOUT,dCTR,b1,nullptr); };

    // ---- warm-up: full sequential pass, then check bit-perfect against the original
    h2d(s0); zstd_all(s0); rans_all(s0); unpack(s0); match(Gwant?Gwant:16,s0,0,nb); CK(cudaStreamSynchronize(s0)); CK(cudaGetLastError());
    rans_check("warm-up");
    { std::vector<nvcompStatus_t> hst(N); CK(cudaMemcpy(hst.data(),dst,N*sizeof(nvcompStatus_t),cudaMemcpyDeviceToHost)); size_t bad=0; for(auto x:hst) if(x!=nvcompSuccess) bad++;
      printf("frames: %zu bad of %zu\n",bad,N); }
    const size_t nfc=((size_t)orig+FNV_CH-1)/FNV_CH; uint64_t* dFC; CK(cudaMalloc(&dFC,(nfc+1)*8));
    auto fnv_check=[&](const char* tag){ std::vector<uint64_t> hc(nfc);
        if(nfc){ k_fnv<<<(unsigned)((nfc+127)/128),128,0,s0>>>(dOUT,(size_t)orig,dFC); CK(cudaMemcpyAsync(hc.data(),dFC,nfc*8,cudaMemcpyDeviceToHost,s0)); }
        CK(cudaStreamSynchronize(s0)); const uint64_t h=fnv_of_chunks(hc);
        static uint64_t ho=0; static bool have=false;
        if(!have){ FILE* f=fopen(argv[2],"rb"); if(!f){perror(argv[2]); exit(1);} std::vector<uint8_t> buf(FNV_CH); std::vector<uint64_t> oc; size_t n;
            while((n=fread(buf.data(),1,buf.size(),f))>0){ uint64_t x=0xcbf29ce484222325ULL; for(size_t i=0;i<n;i++) x=(x^buf[i])*0x100000001b3ULL; oc.push_back(x); }
            fclose(f); ho=fnv_of_chunks(oc); have=true; }
        printf("[%s] FNV out=%016llx orig=%016llx %s\n",tag,(unsigned long long)h,(unsigned long long)ho,h==ho?"MATCHES OK":"DIFFERS X"); return h==ho; };
    bool ok=fnv_check("warm-up");

    // ---- G probe: first min(nb,2048) blocks, 3 runs each, pick the fastest
    int G=Gwant; uint32_t pb=std::min<uint32_t>(nb,2048);
    if(!G){ float best=1e30f; for(int g:{8,16,32}){ std::vector<float> t; for(int r=0;r<3;r++) t.push_back(elapsed(s0,[&]{ match(g,s0,0,pb); }));
            float m=median_ms(t); printf("probe G=%d: %.3f ms on %u blocks\n",g,m,pb); if(m<best){best=m;G=g;} }
        printf("chosen G=%d on %s (%d SMs)\n",G,prop.name,nsm); }
    // ---- sequential stages, median of reps; lit and unpack also split into their parts
    std::vector<float> tH,tT,tL,tU,tM, tLz,tLp[P_NCLS],tUd,tUs,tUc,tUe;
    for(int r=0;r<reps;r++){ tH.push_back(elapsed(s0,[&]{h2d(s0);})); tT.push_back(elapsed(s0,[&]{tok(s0);})); tL.push_back(elapsed(s0,[&]{lit(s0);}));
        tU.push_back(elapsed(s0,[&]{unpack(s0);})); tM.push_back(elapsed(s0,[&]{match(G,s0,0,nb);}));
        tLz.push_back(elapsed(s0,[&]{zstd_rng(s0,NT,N);}));
        for(int q=P_SEQ;q<P_NCLS;q++) tLp[q].push_back(elapsed(s0,[&]{pieces(s0,q,q+1);}));
        tUd.push_back(elapsed(s0,[&]{ if(!dna.empty()){ k_unpack<<<g1,256,0,s0>>>(dDD); k_exc<<<(unsigned)dna.size(),256,0,s0>>>(dDD); } }));
        tUc.push_back(elapsed(s0,[&]{un_cse(s0);})); tUs.push_back(elapsed(s0,[&]{un_seq(s0);})); tUe.push_back(elapsed(s0,[&]{un_exc(s0);})); }
    rans_check("sequential");
    float mH=median_ms(tH),mT=median_ms(tT),mL=median_ms(tL),mU=median_ms(tU),mM=median_ms(tM), mD=mT+mL+mU+mM, seq=mH+mD;
    float mLz=median_ms(tLz), mLp[P_NCLS]={0}; for(int q=P_SEQ;q<P_NCLS;q++) mLp[q]=median_ms(tLp[q]);
    float mUd=median_ms(tUd), mUs=median_ms(tUs), mUc=median_ms(tUc), mUe=median_ms(tUe);
    const size_t NLP=cls_off[P_NCLS]-cls_off[P_SEQ];
    const char* tmode = NTOK&&NT ? "zstd+rANS" : NTOK ? "rANS" : "zstd";
    const char* lmode = NLP&&N>NT ? "zstd+open" : NLP ? "open" : "zstd";
    printf("[sequential] H2D %.3f + tok(%s: %zu frames, %zu chunks) %.3f + lit(%s: %zu frames, %zu pieces) %.3f + unpack %.3f + match(G=%d) %.3f = %.3f ms -> %.1f GB/s delivered; on-device %.3f ms -> %.1f GB/s\n",
        mH,tmode,NT,NTOK,mT,lmode,N-NT,NLP,mL,mU,G,mM,seq,orig/seq/1e6,mD,orig/mD/1e6);
    printf("[lit parts] zstd frames %.3f | open pieces: seq %.3f, cse %.3f, gap %.3f, val %.3f, plain %.3f ms\n",mLz,mLp[P_SEQ],mLp[P_CSE],mLp[P_GAP],mLp[P_VAL],mLp[P_PLAIN]);
    printf("[unpack parts] zstd DNA pack %.3f | open: bases %.3f, case runs %.3f, exceptions %.3f ms\n",mUd,mUs,mUc,mUe);
    ok = fnv_check("sequential") && ok;
    // ---- fused seq piece + bases against lit + unpack; the literal stream is cleared first, then one full
    // pass through the fused path must reproduce the original
    float mF=-1, mFS=-1;
    if(canF){
        // a measurement row: a failure here is reported and does not fail the archive (nor stop the run)
        CK(cudaMemsetAsync(dS[0],0,ssz[0],s0)); tok(s0); fused(s0); match(G,s0,0,nb); CK(cudaStreamSynchronize(s0)); CK(cudaGetLastError());
        uint32_t fe[4]; CK(cudaMemcpy(fe,dErr,16,cudaMemcpyDeviceToHost));
        const bool fok = fnv_check("fused") && !fe[0] && !fe[2];
        if(!fok) printf("[fused] DIFFERS (pieces/chunks failed %u/%u) - fused row invalid\n",fe[0],fe[2]);
        { const uint32_t z0[4]={0u,0xffffffffu,0u,0xffffffffu}; CK(cudaMemcpy(dErr,z0,16,cudaMemcpyHostToDevice)); }
        lit(s0); unpack(s0); CK(cudaStreamSynchronize(s0));      // the literal stream as the regular path leaves it
        std::vector<float> tF,tFS; for(int r=0;r<reps;r++){ tF.push_back(elapsed(s0,[&]{fused(s0);}));
            tFS.push_back(elapsed(s0,[&]{ k_open_seqb<<<(unsigned)NO,256,0,s0>>>(dC,dRD+cls_off[P_SEQ],dOD,dErr); })); }
        mF=median_ms(tF); mFS=median_ms(tFS); if(!fok){ mF=-2; mFS=-2; }
        { const uint32_t z0[4]={0u,0xffffffffu,0u,0xffffffffu}; CK(cudaMemcpy(dErr,z0,16,cudaMemcpyHostToDevice)); lit(s0); unpack(s0); CK(cudaStreamSynchronize(s0)); }
        printf("[fused] lit + unpack %.3f ms (lit %.3f + unpack %.3f; seq piece %.3f + bases %.3f) -> fused %.3f ms (seq+bases kernel %.3f); on-device %.3f -> %.3f ms\n",
            mL+mU,mL,mU,mLp[P_SEQ],mUs,mF,mFS,mD,mT+mF+mM);
    }

    // ---- open variants (AX_OPEN_SEQ, AX_OPEN_EXC), each against the same archive in this run: the literal stream, the
    // open scratch and the output are cleared, lit + unpack + match run with the variant, the output is hashed
    // (bit-perfect or the run fails); then seq pieces and unpack timed (median of reps)
    if(NO){
        struct V { const char* name; int s,e,h; float seq,un,bases,cse,exc; bool ok; };
        V vv[4]={{"AX_OPEN_SEQ=0 AX_OPEN_EXC=0",0,0,0},{"AX_OPEN_SEQ=1 AX_OPEN_EXC=0",1,0,0},{"AX_OPEN_SEQ=1 AX_OPEN_EXC=1",1,1,0},{"AX_OPEN_SEQ=1 AX_OPEN_EXC=1 AX_OPEN_SHB=1",1,1,1}};
        for(V& v:vv){ VS=v.s; VE=v.e; VH=v.h;
            CK(cudaMemset(dS[0],0,ssz[0])); CK(cudaMemset(dOUT,0,orig)); CK(cudaMemset(dOS,0,oscr)); CK(cudaMemset(dEpos,0,(heo[NO]+1)*4));
            if(N>NT) zstd_rng(s0,NT,N);
            pieces(s0,P_SEQ,P_NCLS); unpack(s0); match(G,s0,0,nb); CK(cudaStreamSynchronize(s0)); CK(cudaGetLastError());
            rans_check(v.name); v.ok=fnv_check(v.name); ok=v.ok && ok;
            std::vector<float> a,b,c,d,e; for(int r=0;r<reps;r++){ a.push_back(elapsed(s0,[&]{pieces(s0,P_SEQ,P_SEQ+1);})); b.push_back(elapsed(s0,[&]{unpack(s0);}));
                c.push_back(elapsed(s0,[&]{un_seq(s0);})); d.push_back(elapsed(s0,[&]{un_cse(s0);})); e.push_back(elapsed(s0,[&]{un_exc(s0);})); }
            v.seq=median_ms(a); v.un=median_ms(b); v.bases=median_ms(c); v.cse=median_ms(d); v.exc=median_ms(e); }
        VS=VS0; VE=VE0; VH=VH0;
        printf("[open variants] seq pieces %.3f -> %.3f ms (AX_OPEN_SEQ 0 -> 1, windowed refill, %s); unpack %.3f ms (case %.3f + bases %.3f + exceptions %.3f) -> "
               "%.3f ms (AX_OPEN_EXC 0 -> 1: case+exception positions %.3f + bases with exceptions %.3f, %s) -> %.3f ms (AX_OPEN_SHB=1: bases from shared %.3f, %s)\n",
               vv[0].seq,vv[1].seq,vv[1].ok?"bit-perfect":"DIFFERS",vv[1].un,vv[1].cse,vv[1].bases,vv[1].exc,vv[2].un,vv[2].cse,vv[2].bases,vv[2].ok?"bit-perfect":"DIFFERS",
               vv[3].un,vv[3].bases,vv[3].ok?"bit-perfect":"DIFFERS");
        printf("OPENVAR\t%s\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%d\n",argv[1],vv[0].seq,vv[1].seq,vv[1].un,vv[2].un,vv[3].un,(int)(vv[0].ok&&vv[1].ok&&vv[2].ok&&vv[3].ok));
    }

    // ---- AX_GPU_TILE: the match kernel builds each block's literals in shared memory (k_decode_t; library: aceapex_gpu_lib.cu)
    // against the default path (case runs + exception positions, bases kernel into the literal stream, match kernel); both
    // from cleared buffers, each hashed (bit-perfect or the run fails); only when every literal chunk is an open DNA pack
    // or an open plain piece and blocks are <= 16 KiB
    {   const size_t NWc = chunk[0] ? (size_t)((ssz[0] + chunk[0] - 1) / chunk[0]) : 0;
        std::vector<uint32_t> hcm(NWc, ~0u); std::vector<uint8_t> have(NWc, 0); bool tok_ok = NWc && dna.empty() && N == NT && bs <= 16384 && chunk[0] % 16 == 0;
        for (size_t k = 0; tok_ok && k < NO; k++) { if (opn[k].chunk >= NWc) tok_ok = false; else { hcm[opn[k].chunk] = (uint32_t)k; have[opn[k].chunk] = 1; } }
        for (const RansJob& r : rans) if (tok_ok && r.cls == P_PLAIN) { const size_t t = r.dst_off / chunk[0]; if (t >= NWc) tok_ok = false; else have[t] = 1; }
        for (size_t t = 0; tok_ok && t < NWc; t++) if (!have[t]) tok_ok = false;
        if (tok_ok && NO) {
            uint32_t* dCM; CK(cudaMalloc(&dCM, NWc * 4 + 4)); CK(cudaMemcpy(dCM, hcm.data(), NWc * 4, cudaMemcpyHostToDevice));
            int mb = 1; CK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&mb, k_decode_t, 32 * AXT_SLOTS, 0));
            const unsigned gt = (unsigned)std::max(1, std::min<int>(nsm * mb, (int)(nb / AXT_SLOTS + 1)));
            auto tile_match = [&](cudaStream_t s) { const uint32_t z = 0; CK(cudaMemcpyAsync(dCTR, &z, 4, cudaMemcpyHostToDevice, s));
                k_decode_t<<<gt, 32 * AXT_SLOTS, 0, s>>>(dS[1], dS[2], dS[3], dBO, orig, bs, dOUT, dCTR, nb, dErr + 2, dOD, dCM, dS[0], (uint32_t)NWc, chunk[0], ssz[0]); };
            float tb, tt; bool okb, okt;
            {   CK(cudaMemset(dS[0], 0, ssz[0])); CK(cudaMemset(dOUT, 0, orig)); CK(cudaMemset(dOS, 0, oscr));
                pieces(s0, P_SEQ, P_NCLS); un_cse(s0); un_seq(s0); match(G, s0, 0, nb); CK(cudaStreamSynchronize(s0)); okb = fnv_check("tile base");
                std::vector<float> t; for (int r = 0; r < reps; r++) t.push_back(elapsed(s0, [&] { un_cse(s0); un_seq(s0); match(G, s0, 0, nb); })); tb = median_ms(t); }
            {   CK(cudaMemset(dS[0], 0, ssz[0])); CK(cudaMemset(dOUT, 0, orig)); CK(cudaMemset(dOS, 0, oscr));
                pieces(s0, P_SEQ, P_NCLS); un_cse(s0); tile_match(s0); CK(cudaStreamSynchronize(s0)); CK(cudaGetLastError()); rans_check("AX_GPU_TILE"); okt = fnv_check("AX_GPU_TILE=1");
                std::vector<float> t; for (int r = 0; r < reps; r++) t.push_back(elapsed(s0, [&] { un_cse(s0); tile_match(s0); })); tt = median_ms(t); }
            ok = okb && okt && ok;
            printf("[tile variants] case runs + bases + match %.3f ms -> case runs + tile match %.3f ms (AX_GPU_TILE=1: literals in shared memory, no literal stream, %s)\n", tb, tt, okt ? "bit-perfect" : "DIFFERS");
            printf("TILEVAR\t%s\t%.3f\t%.3f\t%d\n", argv[1], tb, tt, (int)(okb && okt));
            CK(cudaFree(dCM));
        } else printf("[tile variants] not applicable (literal chunks other than open DNA packs / open plain pieces, or blocks > 16 KiB)\n");
    }

    // ---- pipeline: NB batches of frames in order; H2D of batch k+1 on s1 overlaps zstd of batch k on s0
    // (the pieces follow the frames in dC and arrive with the last batch)
    std::vector<size_t> bcut(NB+1); for(int k=0;k<=NB;k++) bcut[k]=(size_t)((double)N*k/NB);
    auto byte_at=[&](size_t i)->size_t{ return i<N?coff[i]:zbytes; };
    std::vector<size_t> btemp(NB,0); size_t maxtemp=0;
    for(int k=0;k<NB;k++){ size_t n=bcut[k+1]-bcut[k], ob=0; for(size_t i=bcut[k];i<bcut[k+1];i++) ob+=jobs[i].osz;
        if(n) NV(nvcompBatchedZstdDecompressGetTempSizeAsync(n,maxo,opts,&btemp[k],ob)); maxtemp=std::max(maxtemp,btemp[k]); }
    void* dtemp2; CK(cudaMalloc(&dtemp2,maxtemp+256));
    std::vector<cudaEvent_t> ev(NB); for(auto& e:ev) CK(cudaEventCreate(&e));
    auto pipeline=[&](){
        for(int k=0;k<NB;k++){ size_t i0=bcut[k], n=bcut[k+1]-i0, b0=byte_at(i0), b1=k+1<NB?byte_at(bcut[k+1]):cbytes;
            if(b1>b0) CK(cudaMemcpyAsync(dC+b0,hc+b0,b1-b0,cudaMemcpyHostToDevice,s1)); CK(cudaEventRecord(ev[k],s1));
            CK(cudaStreamWaitEvent(s0,ev[k],0));
            if(n) NV(nvcompBatchedZstdDecompressAsync(dcp+i0,dcs+i0,dos+i0,dact+i0,n,dtemp2,maxtemp,dop+i0,opts,dst+i0,s0)); }
        rans_all(s0); unpack(s0); match(G,s0,0,nb); };
    std::vector<float> tP; for(int r=0;r<reps;r++) tP.push_back(elapsed(s0,[&]{ pipeline(); }));
    float mP=median_ms(tP);
    printf("[pipeline] %d batches, H2D overlapped with zstd: %.3f ms -> %.1f GB/s delivered (sequential %.3f ms, gain %.1f%%)\n",NB,mP,orig/mP/1e6,seq,100.0*(seq-mP)/seq);
    rans_check("pipeline");
    ok = fnv_check("pipeline") && ok;

    // ---- H2D of the archive bytes: pageable (a malloc'd copy) against pinned (hc, cudaHostAlloc)
    float gbPg=0, gbPn=0;
    { std::vector<uint8_t> pg(hc,hc+cbytes); std::vector<float> a1,a2;
      for(int r=0;r<reps;r++){ a1.push_back(elapsed(s0,[&]{ CK(cudaMemcpyAsync(dC,pg.data(),cbytes,cudaMemcpyHostToDevice,s0)); }));
                               a2.push_back(elapsed(s0,[&]{ h2d(s0); })); }
      gbPg=cbytes/median_ms(a1)/1e6; gbPn=cbytes/median_ms(a2)/1e6;
      printf("[h2d] %.1f MB: pageable %.3f ms (%.1f GB/s), pinned %.3f ms (%.1f GB/s)\n",cbytes/1e6,median_ms(a1),gbPg,median_ms(a2),gbPn); }

    // ---- stream pipeline (--pipeline=K): the blocks in K batches; every frame, piece and literal chunk
    // belongs to the batch of the first block that reads its output, and its compressed bytes are laid out
    // batch by batch in a second pinned buffer. Stream s1 copies batch k+1 while s0 decodes batch k
    // (frames, pieces, unpack, match of its blocks); s0 runs the batches in order, so a chunk decoded
    // for an earlier batch is in place when a later block reads it.
    // --pipeline=auto: the stream pipeline only when the copy is worth hiding - estimated H2D (archive bytes
    // at the measured pinned rate) >= half the on-device decode - and there are >= 2 batches of >= 64 MB;
    // otherwise the sequential path (a small archive loses more to per-batch launches than the copy costs)
    float mSP=0; int KR=PK;
    if(PK<0){ const double estH=cbytes/(gbPn*1e6); const int kb=(int)std::min<size_t>(8,cbytes/(64ull<<20));
        KR = (estH>=0.5*mD && kb>=2) ? kb : 0;
        printf("[auto] estimated H2D %.3f ms (%.1f MB at %.1f GB/s pinned), on-device %.3f ms, batches of >= 64 MB: %d -> %s\n",
            estH,cbytes/1e6,gbPn,mD,kb,KR?"stream pipeline":"sequential"); }
    if(KR){
        const int K=(int)std::min<uint32_t>((uint32_t)KR,nb);
        std::vector<uint32_t> bc(K+1); for(int k=0;k<=K;k++) bc[k]=(uint32_t)((uint64_t)nb*k/K);
        auto bend=[&](int st,uint32_t b)->uint64_t{ const BlockOffsets& x=bo[b]; return st==0?x.lit_off+x.lit_sz:st==1?x.off_off+x.off_sz:st==2?x.len_off+x.len_sz:x.cmd_off+x.cmd_sz; };
        for(int st=0;st<4;st++) for(uint32_t b=1;b<nb;b++) if(bend(st,b)<bend(st,b-1)){ fprintf(stderr,"[stream-pipeline] stream %d block ends not monotonic - not run\n",st); exit(7); }
        auto kof=[&](uint32_t b)->int{ return (int)(std::upper_bound(bc.begin(),bc.end(),b)-bc.begin())-1; };
        auto batch_of=[&](int st,uint64_t o)->int{ uint32_t lo=0,hi=nb; while(lo<hi){ uint32_t m=(lo+hi)/2; if(bend(st,m)>o) hi=m; else lo=m+1; } return kof(lo<nb?lo:nb-1); };
        std::vector<int> jb(N), rb(NR), ob(NO), db(dna.size());
        for(size_t i=0;i<N;i++){ const Job& j=jobs[i]; jb[i]= j.kind==K_PLAIN ? batch_of(j.stream,j.dst_off) : batch_of(0,(uint64_t)j.chunk*chunk[0]); }
        for(size_t k=0;k<NR;k++){ const RansJob& r=rans[k]; rb[k]= r.stream<0 ? batch_of(0,r.lo) : batch_of(r.stream,r.dst_off); }
        for(size_t k=0;k<NO;k++) ob[k]=batch_of(0,(uint64_t)opn[k].chunk*chunk[0]);
        for(size_t k=0;k<dna.size();k++) db[k]=batch_of(0,(uint64_t)dna[k].chunk*chunk[0]);
        for(size_t k=1;k<NO;k++) if(ob[k]<ob[k-1]){ fprintf(stderr,"[stream-pipeline] open chunks out of order - not run\n"); exit(7); }
        for(size_t k=1;k<dna.size();k++) if(db[k]<db[k-1]){ fprintf(stderr,"[stream-pipeline] DNA chunks out of order - not run\n"); exit(7); }
        std::vector<size_t> pj(N), pr(NR); for(size_t i=0;i<N;i++) pj[i]=i; for(size_t k=0;k<NR;k++) pr[k]=k;
        std::stable_sort(pj.begin(),pj.end(),[&](size_t x,size_t y){ return jb[x]<jb[y]; });
        std::stable_sort(pr.begin(),pr.end(),[&](size_t x,size_t y){ return rb[x]<rb[y]; });
        std::vector<size_t> jc(K+1,0), rc(K+1,0), oc(K+1,0), dc(K+1,0), cc(K+1,0);
        for(size_t i=0;i<N;i++) jc[jb[i]+1]++; for(size_t k=0;k<NR;k++) rc[rb[k]+1]++;
        for(size_t k=0;k<NO;k++) oc[ob[k]+1]++; for(size_t k=0;k<dna.size();k++) dc[db[k]+1]++;
        for(int k=0;k<K;k++){ jc[k+1]+=jc[k]; rc[k+1]+=rc[k]; oc[k+1]+=oc[k]; dc[k+1]+=dc[k]; }
        // batch-ordered bytes: batch k = its frames, then its pieces
        uint8_t* hc2; CK(cudaHostAlloc(&hc2,cbytes+1,cudaHostAllocDefault)); uint8_t* dC2; CK(cudaMalloc(&dC2,cbytes+256));
        std::vector<const void*> qcp(N); std::vector<void*> qop(N); std::vector<size_t> qcs(N), qos(N); std::vector<RansDesc> qr(NR);
        size_t w=0;
        for(int k=0;k<K;k++){ cc[k]=w;
            for(size_t i=jc[k];i<jc[k+1];i++){ const size_t x=pj[i]; memcpy(hc2+w,jobs[x].src,jobs[x].csz); qcp[i]=dC2+w; qop[i]=hop[x]; qcs[i]=hcs[x]; qos[i]=hos[x]; w+=jobs[x].csz; }
            for(size_t i=rc[k];i<rc[k+1];i++){ const size_t x=pr[i]; memcpy(hc2+w,rans[x].src,rans[x].csz); qr[i]=hr[x]; qr[i].src=w; w+=rans[x].csz; } }
        cc[K]=w; if(w!=cbytes){ fprintf(stderr,"[stream-pipeline] layout %zu != %zu bytes\n",w,cbytes); exit(7); }
        const void** qdcp; void** qdop; size_t *qdcs,*qdos,*qdact; nvcompStatus_t* qdst; RansDesc* qdRD;
        CK(cudaMalloc(&qdcp,N*8+8)); CK(cudaMalloc(&qdop,N*8+8)); CK(cudaMalloc(&qdcs,N*8+8)); CK(cudaMalloc(&qdos,N*8+8)); CK(cudaMalloc(&qdact,N*8+8));
        CK(cudaMalloc(&qdst,N*sizeof(nvcompStatus_t)+8)); CK(cudaMalloc(&qdRD,(NR+1)*sizeof(RansDesc)));
        if(N){ CK(cudaMemcpy(qdcp,qcp.data(),N*8,cudaMemcpyHostToDevice)); CK(cudaMemcpy(qdop,qop.data(),N*8,cudaMemcpyHostToDevice));
               CK(cudaMemcpy(qdcs,qcs.data(),N*8,cudaMemcpyHostToDevice)); CK(cudaMemcpy(qdos,qos.data(),N*8,cudaMemcpyHostToDevice)); }
        if(NR) CK(cudaMemcpy(qdRD,qr.data(),NR*sizeof(RansDesc),cudaMemcpyHostToDevice));
        size_t qtemp=0; for(int k=0;k<K;k++){ size_t n=jc[k+1]-jc[k], o=0, t=0; for(size_t i=jc[k];i<jc[k+1];i++) o+=qos[i];
            if(n){ NV(nvcompBatchedZstdDecompressGetTempSizeAsync(n,maxo,opts,&t,o)); qtemp=std::max(qtemp,t); } }
        void* qdtemp; CK(cudaMalloc(&qdtemp,qtemp+256));
        uint32_t* dCTR2; CK(cudaMalloc(&dCTR2,4*(K+1)));
        kern_t kG = G==8?k_decode_g<8>:G==16?k_decode_g<16>:k_decode_g<32>;
        int maxblk=0; CK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&maxblk,kG,TPB,0));
        std::vector<cudaEvent_t> cev(K); for(auto& e:cev) CK(cudaEventCreateWithFlags(&e,cudaEventDisableTiming));
        cudaEvent_t st0; CK(cudaEventCreateWithFlags(&st0,cudaEventDisableTiming));
        auto spipe=[&](){
            CK(cudaEventRecord(st0,s0)); CK(cudaStreamWaitEvent(s1,st0,0));
            for(int k=0;k<K;k++){ if(cc[k+1]>cc[k]) CK(cudaMemcpyAsync(dC2+cc[k],hc2+cc[k],cc[k+1]-cc[k],cudaMemcpyHostToDevice,s1)); CK(cudaEventRecord(cev[k],s1)); }
            for(int k=0;k<K;k++){ CK(cudaStreamWaitEvent(s0,cev[k],0));
                const size_t j0=jc[k], nj=jc[k+1]-j0, r0=rc[k], nr=rc[k+1]-r0, o0=oc[k], no=oc[k+1]-o0, d0=dc[k], nd=dc[k+1]-d0;
                if(nj) NV(nvcompBatchedZstdDecompressAsync(qdcp+j0,qdcs+j0,qdos+j0,qdact+j0,nj,qdtemp,qtemp,qdop+j0,opts,qdst+j0,s0));
                if(nr) k_rans<1><<<(unsigned)((nr+AXW_WARPS-1)/AXW_WARPS),32*AXW_WARPS,0,s0>>>(dC2,qdRD+r0,(uint32_t)nr,dErr);
                if(nd){ k_unpack<<<dim3((unsigned)nd,g1.y),256,0,s0>>>(dDD+d0); k_exc<<<(unsigned)nd,256,0,s0>>>(dDD+d0); }
                if(no){ k_open_cg<<<(unsigned)no,AXO_NT,0,s0>>>(dOD+o0,dErr+2); k_open_bases_x<<<dim3((unsigned)no,go.y),256,0,s0>>>(dOD+o0); }
                const uint32_t b0=bc[k], b1=bc[k+1]; if(b1>b0){
                    uint64_t lanes=(uint64_t)(b1-b0)*G; uint32_t want=(uint32_t)((lanes+TPB-1)/TPB), grid=(uint32_t)nsm*maxblk; if(grid>want) grid=want; if(grid<1) grid=1;
                    k_set<<<1,1,0,s0>>>(dCTR2+k,b0); kG<<<grid,TPB,0,s0>>>(dS[0],dS[1],dS[2],dS[3],dBO,orig,bs,dOUT,dCTR2+k,b1,nullptr); } } };
        // cold pass on cleared buffers (output, decoded streams, scratch; raw chunks copied back): the hash
        // below checks this path alone, a batch that ran before its bytes arrived cannot hide behind old data
        CK(cudaMemset(dOUT,0,orig)); for(int i=0;i<4;i++) CK(cudaMemset(dS[i],0,ssz[i]+256));
        CK(cudaMemset(dScr,0,scratch+256)); CK(cudaMemset(dOS,0,oscr+256));
        for(auto& r:raws) CK(cudaMemcpy(dS[r.stream]+r.dst_off,r.src,r.n,cudaMemcpyHostToDevice));
        spipe(); CK(cudaStreamSynchronize(s0)); CK(cudaGetLastError());
        rans_check("stream-pipeline");
        if(N){ std::vector<nvcompStatus_t> hst(N); CK(cudaMemcpy(hst.data(),qdst,N*sizeof(nvcompStatus_t),cudaMemcpyDeviceToHost)); size_t bad=0; for(auto x:hst) if(x!=nvcompSuccess) bad++;
            if(bad){ fprintf(stderr,"[stream-pipeline] %zu frames failed\n",bad); ok=false; } }
        ok = fnv_check("stream-pipeline cold") && ok;
        std::vector<float> tS; for(int r=0;r<reps;r++) tS.push_back(elapsed(s0,[&]{ spipe(); }));
        mSP=median_ms(tS); size_t mb=0; for(int k=0;k<K;k++) mb=std::max(mb,cc[k+1]-cc[k]);
        printf("[stream-pipeline] %d batches (largest %.1f MB), copy stream + decode stream: %.3f ms -> %.1f GB/s delivered; sequential %.3f (H2D %.3f + on-device %.3f), gain %.1f%%\n",
            K,mb/1e6,mSP,orig/mSP/1e6,seq,mH,mD,100.0*(seq-mSP)/seq);
        rans_check("stream-pipeline");
        ok = fnv_check("stream-pipeline") && ok;
    }
    // ---- dense-open (--dense-lit): order-1 literals decoded on the GPU, compared with the literal stream
    // this archive decoded to (dS[0] after the passes above)
    float mR1=0, mR1D=0; long long r1bad=-1;
    if(R1){
        std::vector<uint8_t> r=slurp(R1); if(r.size()<8 || memcmp(r.data(),"AR1L",4) || r[5]!=4 || r[6]!=12){ fprintf(stderr,"%s: not an AR1L N=4 TF=12 stream\n",R1); return 1; }
        std::vector<R1Desc> hd1; size_t pos=8; uint64_t dst=0; size_t nraw=0;
        while(pos+8<=r.size()){ uint32_t cn=rd32(&r[pos]), cs=rd32(&r[pos+4]); pos+=8; if(pos+cs>r.size()){ fprintf(stderr,"AR1L chunk past the end\n"); return 1; }
            if(cs&&r[pos]==1) nraw++; hd1.push_back({pos,dst,cs,cn}); pos+=cs; dst+=cn; }
        if(dst!=ssz[0]){ fprintf(stderr,"AR1L holds %llu bytes, literal stream %llu\n",(unsigned long long)dst,(unsigned long long)ssz[0]); return 1; }
        uint8_t *dR1,*dL1; R1Desc* dRD1; unsigned long long* dBad; uint32_t* dE1;
        CK(cudaMalloc(&dR1,r.size()+256)); CK(cudaMalloc(&dL1,ssz[0]+256)); CK(cudaMalloc(&dRD1,hd1.size()*sizeof(R1Desc)+8)); CK(cudaMalloc(&dBad,8)); CK(cudaMalloc(&dE1,4));
        CK(cudaMemcpy(dR1,r.data(),r.size(),cudaMemcpyHostToDevice)); CK(cudaMemcpy(dRD1,hd1.data(),hd1.size()*sizeof(R1Desc),cudaMemcpyHostToDevice));
        CK(cudaMemset(dL1,0,ssz[0]+256)); CK(cudaMemset(dBad,0,8)); CK(cudaMemset(dE1,0,4));
        auto r1=[&](cudaStream_t s){ if(!hd1.empty()) k_r1<<<(unsigned)hd1.size(),32,0,s>>>(dR1,dRD1,dL1,dE1); };
        r1(s0); CK(cudaStreamSynchronize(s0)); CK(cudaGetLastError());
        std::vector<float> t; for(int q=0;q<reps;q++) t.push_back(elapsed(s0,[&]{ r1(s0); })); mR1=median_ms(t);
        k_cmp<<<1024,256,0,s0>>>(dL1,dS[0],ssz[0],dBad); unsigned long long hb=0; uint32_t he=0;
        CK(cudaMemcpy(&hb,dBad,8,cudaMemcpyDeviceToHost)); CK(cudaMemcpy(&he,dE1,4,cudaMemcpyDeviceToHost)); r1bad=(long long)hb+he;
        mR1D=mT+mR1+mM;
        printf("[dense-lit] %s: %zu B, %zu chunks (%zu raw), order-1 rANS 4 lines x checkpoints: lit %.3f ms against this archive's lit %.3f + unpack %.3f ms; "
               "literal stream %s (%llu of %llu bytes differ, %u framing errors); on-device estimate tok %.3f + lit %.3f + match %.3f = %.3f ms -> %.1f GB/s\n",
            R1,r.size(),hd1.size(),nraw,mR1,mL,mU,r1bad==0?"MATCHES":"DIFFERS",hb,(unsigned long long)ssz[0],he,mT,mR1,mM,mR1D,orig/mR1D/1e6);
    }
    // ---- dense-open v2 (--dense2-lit): 32 segments per chunk; variants byte table / nibble table / cum compare
    // (k_r2<0..2>, chunks with K <= 16) plus k_r2<3> for wider chunks in a second launch, timed together
    float mR2[3]={-1,-1,-1}, mR2D=0; long long r2bad=-1;
    if(R2){
        std::vector<uint8_t> r=slurp(R2); if(r.size()<8 || memcmp(r.data(),"AR2L",4) || r[5]!=32 || r[6]!=12){ fprintf(stderr,"%s: not an AR2L NS=32 TF=12 stream\n",R2); return 1; }
        std::vector<R2Desc> hn, hw; size_t pos=8; uint64_t dst=0; size_t nraw=0; int Kn=1, Kw=0; bool bad=false;
        while(pos+8<=r.size()){ uint32_t cn=rd32(&r[pos]), cs=rd32(&r[pos+4]); pos+=8; if(pos+cs>r.size()){ bad=true; break; }
            int K = (cs>1 && r[pos]!=1) ? r[pos+1] : 0; if(cs&&r[pos]==1) nraw++;
            if(K>16){ hw.push_back({pos,dst,cs,cn}); Kw=std::max(Kw,K); } else { hn.push_back({pos,dst,cs,cn}); Kn=std::max(Kn,K); }
            pos+=cs; dst+=cn; }
        if(bad || dst!=ssz[0] || Kw>64){ printf("[dense2-lit] %s: unusable (chunk past the end, %llu of %llu bytes, or K %d > 64) - skipped\n",R2,(unsigned long long)dst,(unsigned long long)ssz[0],Kw); }
        else {
        uint8_t *dR2,*dL2; R2Desc *dN,*dW; unsigned long long* dBad; uint32_t* dE2;
        CK(cudaMalloc(&dR2,r.size()+256)); CK(cudaMalloc(&dL2,ssz[0]+256)); CK(cudaMalloc(&dN,hn.size()*sizeof(R2Desc)+8)); CK(cudaMalloc(&dW,hw.size()*sizeof(R2Desc)+8)); CK(cudaMalloc(&dBad,8)); CK(cudaMalloc(&dE2,4));
        CK(cudaMemcpy(dR2,r.data(),r.size(),cudaMemcpyHostToDevice));
        if(!hn.empty()) CK(cudaMemcpy(dN,hn.data(),hn.size()*sizeof(R2Desc),cudaMemcpyHostToDevice));
        if(!hw.empty()) CK(cudaMemcpy(dW,hw.data(),hw.size()*sizeof(R2Desc),cudaMemcpyHostToDevice));
        const size_t sh[3]={(size_t)Kn*4096,(size_t)Kn*2048,0};
        bool can[3]={true,true,true};
        for(int v=0;v<2;v++) if(sh[v]+2048>(size_t)prop.sharedMemPerBlockOptin) can[v]=false;
        if(can[0]) CK(cudaFuncSetAttribute(k_r2<0>,cudaFuncAttributeMaxDynamicSharedMemorySize,(int)sh[0]));
        if(can[1]) CK(cudaFuncSetAttribute(k_r2<1>,cudaFuncAttributeMaxDynamicSharedMemorySize,(int)sh[1]));
        unsigned long long hbs[3]={0,0,0}; uint32_t hes[3]={0,0,0}; r2bad=0;
        for(int v=0;v<3;v++){ if(!can[v]) continue;
            auto r2=[&](cudaStream_t s){
                if(!hn.empty()){ const unsigned g=(unsigned)hn.size();
                    if(v==0) k_r2<0><<<g,32,sh[0],s>>>(dR2,dN,dL2,dE2); else if(v==1) k_r2<1><<<g,32,sh[1],s>>>(dR2,dN,dL2,dE2); else k_r2<2><<<g,32,0,s>>>(dR2,dN,dL2,dE2); }
                if(!hw.empty()) k_r2<3><<<(unsigned)hw.size(),32,0,s>>>(dR2,dW,dL2,dE2); };
            CK(cudaMemset(dL2,0,ssz[0]+256)); CK(cudaMemset(dBad,0,8)); CK(cudaMemset(dE2,0,4));
            r2(s0); CK(cudaStreamSynchronize(s0)); CK(cudaGetLastError());
            k_cmp<<<1024,256,0,s0>>>(dL2,dS[0],ssz[0],dBad);
            CK(cudaMemcpy(&hbs[v],dBad,8,cudaMemcpyDeviceToHost)); CK(cudaMemcpy(&hes[v],dE2,4,cudaMemcpyDeviceToHost));
            r2bad+=(long long)(hbs[v]+hes[v]);
            std::vector<float> t; for(int q=0;q<reps;q++) t.push_back(elapsed(s0,[&]{ r2(s0); })); mR2[v]=median_ms(t);
        }
        float best=1e30f; for(int v=0;v<3;v++) if(mR2[v]>=0) best=std::min(best,mR2[v]);
        mR2D=mT+best+mM;
        printf("[dense2-lit] %s: %zu B, %zu chunks (%zu raw, %zu with K > 16 up to %d via binary search), K max %d in the rest; order-1 rANS 32 segments/chunk: "
               "lit byte table %.3f ms (%zu B shared), nibble table %.3f ms (%zu B), cum compare %.3f ms (no table) against this archive's lit %.3f + unpack %.3f = %.3f ms "
               "(-1 = does not fit this GPU's shared memory); literal stream %s (differing bytes %llu / %llu / %llu, framing errors %u / %u / %u); "
               "on-device estimate tok %.3f + lit %.3f + match %.3f = %.3f ms -> %.1f GB/s\n",
            R2,r.size(),hn.size()+hw.size(),nraw,hw.size(),Kw,Kn,mR2[0],sh[0],mR2[1],sh[1],mR2[2],mL,mU,mL+mU,r2bad==0?"MATCHES":"DIFFERS",
            hbs[0],hbs[1],hbs[2],hes[0],hes[1],hes[2],mT,best,mM,mR2D,orig/mR2D/1e6);
        }
    }
    const float mAuto = KR ? mSP : seq;               // the path this run would take
    printf("%s: %s, %d SMs, G=%d, %s\n", ok?"RESULT OK":"RESULT FAIL", prop.name, nsm, G, ok?"bit-perfect on all passes":"hash mismatch");
    // one tab-separated row for tables: archive, bytes, token coder, literal coder, tok, lit, unpack, match, on-device,
    // H2D+on-device (ms), GB/s, verdict, then the parts: lit zstd, seq, cse, gap, val, plain; unpack zstd-pack, bases, case, exceptions
    // then: chosen path ms (stream pipeline, or sequential when --pipeline=auto declines), H2D pageable GB/s,
    // H2D pinned GB/s, batches of the stream pipeline (0 = sequential); dense-lit ms, dense on-device estimate ms,
    // dense-lit differing bytes (-1 = not run); dense2-lit ms byte table, ms nibble table, ms cum compare, v2 on-device estimate ms,
    // dense2 differing bytes; fused lit+unpack ms, fused seq+bases kernel ms (-1 = not applicable, -2 = fused output differs)
    // dense2-lit differing bytes + framing errors (-1 = not run)
    printf("ROW\t%s\t%zu\t%s\t%s\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.2f\t%s\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.1f\t%.1f\t%d\t%.3f\t%.3f\t%lld\t%.3f\t%.3f\t%.3f\t%.3f\t%lld\t%.3f\t%.3f\n",
        argv[1],a.size(),tmode,lmode,mT,mL,mU,mM,mD,seq,orig/mD/1e6,ok?"bit-perfect":"MISMATCH",
        mLz,mLp[P_SEQ],mLp[P_CSE],mLp[P_GAP],mLp[P_VAL],mLp[P_PLAIN],mUd,mUs,mUc,mUe,mAuto,gbPg,gbPn,KR,mR1,mR1D,r1bad,mR2[0],mR2[1],mR2[2],mR2D,r2bad,mF,mFS);
    return ok?0:5;
}
