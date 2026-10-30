// aceapex_gpu_lib.cu - the GPU decoder behind src/aceapex_gpu.h (C ABI). Host plan: src/aceapex_gpu_plan.h;
// device kernels: src/aceapex_gpu_kernels.cuh (the ones the measurement tool aceapex_gpu.cu uses).
// A decode call: reset the error words, copy the plan's descriptor templates into d_temp adding the d_temp /
// d_in base addresses, stored token chunks d_in -> streams, zstd frames (nvCOMP, optional), rANS pieces,
// DNA unpack (zstd pack mode 1, open pack mode 2), the match kernel, then the status word. Everything on the
// caller's stream; no allocation, no host copy, no synchronization.
// Build: nvcc -O3 -arch=sm_XX [-DACEAPEX_GPU_NVCOMP -I<nvcomp>/include] -c src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp
//        (shared library: make gpu-lib [NVCOMP=<dir>] -> libaceapex_gpu.so.1)
//        (link -l:libnvcomp.so.5 -lzstd with nvCOMP)
#include "aceapex_gpu.h"
#ifdef ACEAPEX_GPU_NVCOMP
#define AGP_WITH_ZSTD       // ACEAPEX_GPU_VALIDATE_ZSTD: libzstd on the host (link -lzstd)
#endif
#include "aceapex_gpu_plan.h"
#include "ax_env.h"         // ax_getenv: AX_GPU_TILE only with ACEAPEX_ENV_TUNING (gpu_api_test, gpu_run.sh)
#include "aceapex_gpu_kernels.cuh"
#include "ax_xxh3.h"
#include <cuda_runtime.h>
#ifdef ACEAPEX_GPU_NVCOMP
#include <nvcomp/zstd.h>
#endif
#include <new>

static_assert(sizeof(agp::Rans) == sizeof(RansDesc), "Rans layout");
static_assert(sizeof(agp::Open) == sizeof(OpenDesc), "Open layout");
static_assert(sizeof(agp::Dna) == sizeof(DnaDesc), "Dna layout");

struct aceapex_gpu_plan {
    agp::Plan P;
    uint8_t* dmem = nullptr;                      // device: block table + descriptor templates
    size_t o_bo = 0, o_rans = 0, o_open = 0, o_dna = 0, o_nv = 0, o_raw = 0, o_cmap = 0, o_rkey = 0, o_okey = 0, o_wkey = 0;   // keys: windows batch
    unsigned grid = 1;                            // match kernel: resident blocks of 128 threads
    bool tile = false; unsigned grid_t = 1;       // AX_GPU_TILE (env at plan_create, plan tile_ok): k_decode_t, its resident blocks
};
// plan_create, last_error and version are in src/aceapex_gpu_abi.cpp (host C++, judged without CUDA:
// claim head_gpu_abi); it checks the arguments and calls this
aceapex_gpu_plan* agpu_plan_build(const void* h_archive, size_t in_bytes, uint64_t flags, int* err, uint32_t b0, uint32_t b1);
// test hook (not in the C ABI; scripts/gpu_api_test.cu): called on the host after each phase of a decode is
// enqueued, so a test can wait for the stream and name the phase a hang is in. nullptr (default): no call
typedef void (*aceapex_gpu_phase_fn)(const char* phase, cudaStream_t s);
static aceapex_gpu_phase_fn g_phase = nullptr;
extern "C" void aceapex_gpu_debug_phase_hook(aceapex_gpu_phase_fn f){ g_phase=f; }
static inline void phase(const char* p, cudaStream_t s){ if(g_phase) g_phase(p,s); }
static const unsigned TPB = 128, G = 32;

// ---- small kernels of the library
__global__ void kg_init(uint32_t* e){ if(threadIdx.x==0){ e[0]=0; e[1]=~0u; e[2]=0; e[3]=~0u; e[4]=0; e[5]=0; e[6]=0; e[7]=0; e[8]=0; } }
__global__ void kg_fix_rans(const agp::Rans* t, RansDesc* d, uint32_t n, uint8_t* base){
    for(uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;i<n;i+=gridDim.x*blockDim.x){ agp::Rans r=t[i];
        RansDesc o; o.src=r.src; o.dst=base+r.dst; o.n=r.n; o.csz=r.csz; o.mode=r.mode; o.cls=r.pad; o.res=0; d[i]=o; } }
__global__ void kg_fix_open(const agp::Open* t, OpenDesc* d, uint32_t n, uint8_t* base){
    for(uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;i<n;i+=gridDim.x*blockDim.x){ agp::Open r=t[i]; OpenDesc o;
        o.seq=base+r.seq; o.cse=base+r.cse; o.gap=base+r.gap; o.val=base+r.val; o.dst=base+r.dst;
        o.ends=(uint32_t*)(base+r.ends); o.nrun=(uint32_t*)(base+r.nrun); o.epos=(uint32_t*)(base+r.epos); o.raw=r.raw; o.ncse=r.ncse; o.ngap=r.ngap; o.nexc=r.nexc; o.res=0; d[i]=o; } }
__global__ void kg_fix_dna(const agp::Dna* t, DnaDesc* d, uint32_t n, uint8_t* base){
    for(uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;i<n;i+=gridDim.x*blockDim.x){ agp::Dna r=t[i]; DnaDesc o;
        o.seq=base+r.seq; o.cse=base+r.cse; o.gap= r.gap==agp::NUL?nullptr:base+r.gap; o.val= r.val==agp::NUL?nullptr:base+r.val;
        o.dst=base+r.dst; o.raw=r.raw; o.nexc=r.nexc; o.res=0; d[i]=o; } }
__global__ void kg_fix_nv(const agp::Nv* t, uint32_t n, const uint8_t* in, uint8_t* base, const void** cp, void** op, size_t* cs, size_t* os){
    for(uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;i<n;i+=gridDim.x*blockDim.x){ agp::Nv j=t[i];
        cp[i]=in+j.in_off; op[i]=base+j.out_off; cs[i]=(size_t)j.csz; os[i]=(size_t)j.osz; } }
__global__ void kg_raw(const agp::Raw* t, const uint8_t* in, uint8_t* base){       // one block per stored chunk
    const agp::Raw r=t[blockIdx.x]; for(uint64_t i=threadIdx.x;i<r.n;i+=blockDim.x) base[r.dst+i]=in[r.src+i]; }
__global__ void kg_nvcheck(const int* st, const size_t* act, const size_t* os, uint32_t n, uint32_t* e){   // nvcompStatus_t is an int enum, success = 0
    for(uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;i<n;i+=gridDim.x*blockDim.x) if(st[i]!=0 || act[i]!=os[i]) atomicOr(e+5,1u); }
__global__ void kg_status(const uint32_t* e, int* st){
    if(threadIdx.x==0) *st=(e[0]?ACEAPEX_GPU_STATUS_PIECE:0)|(e[2]?ACEAPEX_GPU_STATUS_OPEN:0)|(e[5]?ACEAPEX_GPU_STATUS_ZSTD:0)|(e[4]?ACEAPEX_GPU_STATUS_MATCH:0)|(e[6]?ACEAPEX_GPU_STATUS_HASH:0)|(e[7]?ACEAPEX_GPU_STATUS_LIMIT:0)|(e[8]?ACEAPEX_GPU_STATUS_RANGE:0); }
// XXH3_64bits of the output (src/ax_xxh3.h): block terms in parallel (8 threads per 1 KiB block, aligned 64-bit
// words, key table), then the scramble chain - one step per KiB on each of the 8 independent accumulator lanes, one
// thread per lane, the lane's key in a register and the block terms prefetched 32 steps ahead - then tail, merge, compare
#define AXH_PF 32
__global__ void kg_xxh_blocks(const uint8_t* __restrict__ in, uint64_t nb, uint64_t* __restrict__ S){
    const bool al=((uintptr_t)in&7)==0;
    for(uint64_t t=blockIdx.x*(uint64_t)blockDim.x+threadIdx.x;t<nb*8;t+=(uint64_t)gridDim.x*blockDim.x){
        const unsigned lane=(unsigned)(t&7); const uint8_t* blk=in+(t>>3)*AXH_BLOCK;
        S[t]= al ? axh_block_lane_w((const uint64_t*)blk,axh_k64,lane) : axh_block_lane(blk,axh_secret,lane); } }
__global__ void __launch_bounds__(32) kg_xxh_chain(const uint8_t* __restrict__ in, uint64_t len, const uint64_t* __restrict__ S, uint64_t want, uint32_t* e){
    const unsigned lane=threadIdx.x&7;
    if(len<=240){ if(threadIdx.x==0 && axh_short(in,len,axh_secret)!=want) e[6]=1; return; }
    const uint64_t nb=(len-1)/AXH_BLOCK, key=axh_k64[16+lane]; const uint64_t* Sl=S+lane; uint64_t acc=axh_init(lane), buf[AXH_PF];
    #pragma unroll
    for(int i=0;i<AXH_PF;i++) buf[i] = (uint64_t)i<nb ? __ldg(Sl+8ull*i) : 0;
    uint64_t b=0;
    for(; b+AXH_PF<=nb; b+=AXH_PF){
        #pragma unroll
        for(int i=0;i<AXH_PF;i++){ const uint64_t v=buf[i], nx=b+AXH_PF+i; buf[i] = nx<nb ? __ldg(Sl+8ull*nx) : 0; acc=axh_step_k(acc,v,key); } }
    #pragma unroll
    for(int i=0;i<AXH_PF;i++) if(b+i<nb) acc=axh_step_k(acc,buf[i],key);
    acc=axh_tail_lane(acc,in,len,axh_secret,lane);
    uint64_t a[8];
    #pragma unroll
    for(int i=0;i<8;i++) a[i]=__shfl_sync(0xffu,acc,i);
    if(threadIdx.x==0 && axh_merge(a,len,axh_secret)!=want) e[6]=1; }
__global__ void kg_set(uint32_t* p, uint32_t v){ *p=v; }
// ---- windows batch: the selection on the device (aceapex_gpu_decompress_windows_async)
struct WinArgs { uint64_t cb[4], ch[4]; };
__global__ void kw_mark(const uint64_t* off, uint32_t n, uint32_t W, uint32_t bs, uint64_t orig, uint8_t* need, uint32_t* err){
    for(uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;i<n;i+=gridDim.x*blockDim.x){ const uint64_t o=off[i];
        if(o>orig || W>orig-o){ atomicOr(err+8,1u); continue; }                     // a window past the end: STATUS_RANGE
        for(uint64_t b=o/bs; b<=(o+W-1)/bs; b++) need[b]=1; } }
__global__ void kw_chunks(const BlockOffsets* bo, uint32_t nb, const uint8_t* need, WinArgs a, uint8_t* cneed){
    for(uint32_t b=blockIdx.x*blockDim.x+threadIdx.x;b<nb;b+=gridDim.x*blockDim.x){ if(!need[b]) continue; const BlockOffsets e=bo[b];
        const uint64_t o[4]={e.lit_off,e.off_off,e.len_off,e.cmd_off}, z[4]={e.lit_sz,e.off_sz,e.len_sz,e.cmd_sz};
        for(int s=0;s<4;s++){ if(!z[s] || !a.ch[s]) continue; for(uint64_t k=o[s]/a.ch[s]; k<=(o[s]+z[s]-1)/a.ch[s]; k++) cneed[a.cb[s]+k]=1; } } }
__global__ void __launch_bounds__(1024) kw_scan(const uint8_t* need, uint32_t nb, uint32_t* slot, uint32_t* list, uint32_t* cnt){   // one block
    __shared__ uint32_t sh[1024]; uint32_t carry=0; const uint32_t t=threadIdx.x;
    for(uint32_t base=0; base<nb; base+=1024){ const uint32_t b=base+t, v= b<nb ? need[b] : 0; sh[t]=v; __syncthreads();
        for(uint32_t o=1;o<1024;o<<=1){ const uint32_t x= t>=o ? sh[t-o] : 0; __syncthreads(); sh[t]+=x; __syncthreads(); }
        if(v){ const uint32_t q=carry+sh[t]-1; slot[b]=q; list[q]=b; } carry+=sh[1023]; __syncthreads(); }
    if(t==0) cnt[3]=carry; }
// jobs whose chunk is needed, copied to a compact list (any order; descriptors are 8-byte multiples)
__global__ void kw_pick(const uint8_t* all, uint32_t sz, const uint64_t* key, uint32_t n, const uint8_t* cneed, WinArgs a, uint8_t* out, uint32_t* cnt){
    for(uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;i<n;i+=gridDim.x*blockDim.x){ const uint64_t k=key[i], st=k>>48, c=k&((1ull<<48)-1);
        if(st<4 && cneed[a.cb[st]+c]){ const uint32_t q=atomicAdd(cnt,1u); for(uint32_t w=0;w<sz/8;w++) ((uint64_t*)(out+(uint64_t)q*sz))[w]=((const uint64_t*)(all+(uint64_t)i*sz))[w]; } } }
__global__ void kw_raw_n(const agp::Raw* t, const uint32_t* dn, const uint8_t* in, uint8_t* base){
    const uint32_t n=*dn; for(uint32_t j=blockIdx.x;j<n;j+=gridDim.x){ const agp::Raw r=t[j]; for(uint64_t i=threadIdx.x;i<r.n;i+=blockDim.x) base[r.dst+i]=in[r.src+i]; } }
__global__ void kw_gather(const uint64_t* off, uint32_t n, uint32_t W, uint32_t bs, uint64_t orig, const uint32_t* slot, const uint8_t* sbuf, uint8_t* out){
    for(uint32_t i=blockIdx.x;i<n;i+=gridDim.x){ const uint64_t o=off[i]; if(o>orig || W>orig-o) continue; const uint64_t b=o/bs;
        const uint8_t* src=sbuf+(uint64_t)slot[b]*bs+(o%bs); uint8_t* dst=out+(uint64_t)i*W;
        for(uint32_t j=threadIdx.x;j<W;j+=blockDim.x) dst[j]=src[j]; } }
__global__ void kg_copy(const uint8_t* s, uint8_t* d, uint64_t n){
    for(uint64_t i=blockIdx.x*(uint64_t)blockDim.x+threadIdx.x;i<n;i+=(uint64_t)gridDim.x*blockDim.x) d[i]=s[i]; }

static unsigned blocks_for(uint64_t n, unsigned tpb=256){ uint64_t b=(n+tpb-1)/tpb; return (unsigned)(b<1?1:(b>65535?65535:b)); }

#ifdef ACEAPEX_GPU_NVCOMP
static uint64_t nv_temp(size_t n, size_t maxo, size_t tot){ size_t t=0;
    if(nvcompBatchedZstdDecompressGetTempSizeAsync(n,maxo,nvcompBatchedZstdDecompressDefaultOpts,&t,tot)!=nvcompSuccess) return ~0ull; return t; }
#endif

aceapex_gpu_plan* agpu_plan_build(const void* h_archive, size_t in_bytes, uint64_t flags, int* err, uint32_t b0, uint32_t b1){
    int& g_last=*err; g_last=ACEAPEX_GPU_OK;
    aceapex_gpu_plan* pl=new(std::nothrow) aceapex_gpu_plan; if(!pl){ g_last=ACEAPEX_GPU_E_CUDA; return nullptr; }
#ifdef ACEAPEX_GPU_NVCOMP
    int e=agp::build((const uint8_t*)h_archive,in_bytes,pl->P,nv_temp,b0,b1);
    if(!e && pl->P.nv_temp==~0ull) e=agp::E_NVCOMP;
    if(!e && (flags & ACEAPEX_GPU_VALIDATE_ZSTD) && agp::validate_zstd((const uint8_t*)h_archive+pl->P.in_lo,pl->P)) e=agp::E_STREAM;
#else
    int e=agp::build((const uint8_t*)h_archive,in_bytes,pl->P,nullptr,b0,b1);
#endif
    if(e){ g_last= e==agp::E_NVCOMP?ACEAPEX_GPU_E_NVCOMP:ACEAPEX_GPU_E_ARCHIVE; delete pl; return nullptr; }
    agp::Plan& P=pl->P; size_t o=0;
    auto put=[&](size_t n){ size_t r=o; o+=agp::al(n+8); return r; };
    pl->o_bo=put(P.bo.size()); pl->o_rans=put(P.rans.size()*sizeof(agp::Rans)); pl->o_open=put(P.open.size()*sizeof(agp::Open));
    pl->o_dna=put(P.dna.size()*sizeof(agp::Dna)); pl->o_nv=put(P.nv.size()*sizeof(agp::Nv)); pl->o_raw=put(P.raw.size()*sizeof(agp::Raw)); pl->o_cmap=put(P.cmap.size()*4);
    pl->o_rkey=put(P.rans_key.size()*8); pl->o_okey=put(P.open_key.size()*8); pl->o_wkey=put(P.raw_key.size()*8);
    bool ok = cudaMalloc(&pl->dmem,o)==cudaSuccess;
    auto up=[&](size_t off, const void* src, size_t n){ if(ok && n) ok = cudaMemcpy(pl->dmem+off,src,n,cudaMemcpyHostToDevice)==cudaSuccess; };
    up(pl->o_bo,P.bo.data(),P.bo.size()); up(pl->o_rans,P.rans.data(),P.rans.size()*sizeof(agp::Rans));
    up(pl->o_open,P.open.data(),P.open.size()*sizeof(agp::Open)); up(pl->o_dna,P.dna.data(),P.dna.size()*sizeof(agp::Dna));
    up(pl->o_nv,P.nv.data(),P.nv.size()*sizeof(agp::Nv)); up(pl->o_raw,P.raw.data(),P.raw.size()*sizeof(agp::Raw)); up(pl->o_cmap,P.cmap.data(),P.cmap.size()*4);
    up(pl->o_rkey,P.rans_key.data(),P.rans_key.size()*8); up(pl->o_okey,P.open_key.data(),P.open_key.size()*8); up(pl->o_wkey,P.raw_key.data(),P.raw_key.size()*8);
    int dev=0,nsm=1,maxblk=1;
    if(ok) ok = cudaGetDevice(&dev)==cudaSuccess && cudaDeviceGetAttribute(&nsm,cudaDevAttrMultiProcessorCount,dev)==cudaSuccess
             && cudaOccupancyMaxActiveBlocksPerMultiprocessor(&maxblk,k_decode_g<G>,TPB,0)==cudaSuccess;
    int maxblk_t=1;
    if(ok) ok = cudaOccupancyMaxActiveBlocksPerMultiprocessor(&maxblk_t,k_decode_t,32*AXT_SLOTS,0)==cudaSuccess;
    if(!ok){ if(pl->dmem) cudaFree(pl->dmem); delete pl; g_last=ACEAPEX_GPU_E_CUDA; return nullptr; }
    pl->grid=(unsigned)std::max(1,nsm*maxblk); pl->grid_t=(unsigned)std::max(1,nsm*maxblk_t);
    { const char* e=ax_getenv("AX_GPU_TILE"); pl->tile = e && atoi(e) && P.tile_ok; }
    return pl;
}
extern "C" size_t aceapex_gpu_temp_bytes(const aceapex_gpu_plan* p){ return p ? (size_t)p->P.temp_bytes : 0; }
extern "C" size_t aceapex_gpu_range_temp_bytes(const aceapex_gpu_plan* p, uint64_t len){ return p ? (size_t)(p->P.temp_bytes+agp::window_bytes(p->P,len)) : 0; }
extern "C" size_t aceapex_gpu_output_bytes(const aceapex_gpu_plan* p){ return p ? (size_t)p->P.orig : 0; }
extern "C" int    aceapex_gpu_plan_input_window(const aceapex_gpu_plan* p, uint64_t* lo, uint64_t* hi){ if(!p||!lo||!hi) return ACEAPEX_GPU_E_ARGS; *lo=p->P.in_lo; *hi=p->P.in_hi; return ACEAPEX_GPU_OK; }
extern "C" size_t aceapex_gpu_windows_temp_bytes(const aceapex_gpu_plan* p, uint64_t max_windows, uint32_t window_len){
    return (p && window_len) ? (size_t)agp::win_layout(p->P, max_windows, window_len).total : 0; }
extern "C" int aceapex_gpu_decompress_windows_async(const aceapex_gpu_plan* pl, const void* d_in, const uint64_t* d_offsets, uint32_t n, uint32_t W,
                                                    void* d_out, void* d_temp, int* d_status, cudaStream_t s){
    if(!pl || !d_in || !d_offsets || !d_out || !d_temp || !d_status || !n || !W || ((uintptr_t)d_temp & 255)) return ACEAPEX_GPU_E_ARGS;
    const agp::Plan& P=pl->P; if(P.partial || !P.nv.empty() || !P.dna.empty() || pl->tile || W>P.orig || !P.contiguous) return ACEAPEX_GPU_E_ARGS;   // open profile, whole plan
    const agp::WinLayout L=agp::win_layout(P,n,W); uint8_t* T=(uint8_t*)d_temp; const uint8_t* M=pl->dmem; const uint8_t* in=(const uint8_t*)d_in;
    uint32_t* err=(uint32_t*)(T+P.o_err); uint32_t* cnt=(uint32_t*)(T+L.cnt); const uint32_t NR=(uint32_t)P.rans.size(), NO=(uint32_t)P.open.size(), NW=(uint32_t)P.raw.size();
    RansDesc* dR=(RansDesc*)(T+P.o_rans); OpenDesc* dO=(OpenDesc*)(T+P.o_open);
    WinArgs a; for(int q=0;q<4;q++){ a.cb[q]=L.cb[q]; a.ch[q]=P.chunk[q]; }
    WinArgs a0=a; for(int q=1;q<4;q++) a0.cb[q]=a.cb[0];                       // open keys: the literal chunk index alone (stream 0)
    kg_init<<<1,32,0,s>>>(err);
    cudaMemsetAsync(T+L.need,0,(size_t)P.nb,s); cudaMemsetAsync(T+L.cneed,0,(size_t)(L.cb[3]+L.nc[3]),s); cudaMemsetAsync(cnt,0,64,s);
    if(NR) kg_fix_rans<<<blocks_for(NR),256,0,s>>>((const agp::Rans*)(M+pl->o_rans),dR,NR,T);
    if(NO) kg_fix_open<<<blocks_for(NO),256,0,s>>>((const agp::Open*)(M+pl->o_open),dO,NO,T);
    kw_mark<<<std::max(1u,std::min(blocks_for(n),4096u)),256,0,s>>>(d_offsets,n,W,(uint32_t)P.bs,P.orig,T+L.need,err);
    kw_chunks<<<std::max(1u,std::min(blocks_for(P.nb),4096u)),256,0,s>>>((const BlockOffsets*)(M+pl->o_bo),P.nb,T+L.need,a,T+L.cneed);
    kw_scan<<<1,1024,0,s>>>(T+L.need,P.nb,(uint32_t*)(T+L.slot),(uint32_t*)(T+L.list),cnt);
    if(NR) kw_pick<<<std::min(blocks_for(NR),4096u),256,0,s>>>((const uint8_t*)dR,(uint32_t)sizeof(RansDesc),(const uint64_t*)(M+pl->o_rkey),NR,T+L.cneed,a,T+L.rj,cnt+0);
    if(NO) kw_pick<<<std::min(blocks_for(NO),4096u),256,0,s>>>((const uint8_t*)dO,(uint32_t)sizeof(OpenDesc),(const uint64_t*)(M+pl->o_okey),NO,T+L.cneed,a0,T+L.oj,cnt+1);
    if(NW) kw_pick<<<std::min(blocks_for(NW),4096u),256,0,s>>>((const uint8_t*)(M+pl->o_raw),(uint32_t)sizeof(agp::Raw),(const uint64_t*)(M+pl->o_wkey),NW,T+L.cneed,a,T+L.wj,cnt+2);
    if(NW) kw_raw_n<<<std::min(NW,4096u),256,0,s>>>((const agp::Raw*)(T+L.wj),cnt+2,in,T);
    const unsigned gw=std::max(1u,pl->grid*4), gyo=(unsigned)((P.chunk[0]/16+255)/256);
    if(NR) k_rans_n<1><<<std::min(gw,(NR+AXW_WARPS-1)/AXW_WARPS),32*AXW_WARPS,0,s>>>(in,(const RansDesc*)(T+L.rj),cnt+0,err);
    if(NO){ k_open_cg_n<<<std::min(gw,NO),AXO_NT,0,s>>>((const OpenDesc*)(T+L.oj),cnt+1,err+2);
            k_open_bases_s_n<<<dim3(std::min(gw,NO),gyo),256,0,s>>>((const OpenDesc*)(T+L.oj),cnt+1); }
    uint32_t* ctr=(uint32_t*)(T+P.o_ctr); kg_set<<<1,1,0,s>>>(ctr,0);
    k_decode_list<G><<<std::max(1u,pl->grid),TPB,0,s>>>(T+P.o_s[0],T+P.o_s[1],T+P.o_s[2],T+P.o_s[3],(const BlockOffsets*)(M+pl->o_bo),P.orig,P.bs,T+L.sbuf,
        (const uint32_t*)(T+L.list),cnt+3,ctr,err+4);
    kw_gather<<<std::max(1u,std::min(n,65535u)),256,0,s>>>(d_offsets,n,W,(uint32_t)P.bs,P.orig,(const uint32_t*)(T+L.slot),T+L.sbuf,(uint8_t*)d_out);
    kg_status<<<1,32,0,s>>>(err,d_status);
    return cudaGetLastError()==cudaSuccess ? ACEAPEX_GPU_OK : ACEAPEX_GPU_E_CUDA;
}
extern "C" void   aceapex_gpu_plan_destroy(aceapex_gpu_plan* p){ if(p){ if(p->dmem) cudaFree(p->dmem); delete p; } }

// the whole schedule; S == nullptr: every job and every block into d_out; S: the range selection into the window
static int run(const aceapex_gpu_plan* pl, const agp::Sel* S, const uint8_t* in, uint8_t* out, uint8_t* T, int* d_status, cudaStream_t s){
    const agp::Plan& P=pl->P; const uint8_t* M=pl->dmem;
    uint32_t* err=(uint32_t*)(T+P.o_err);
    RansDesc* dR=(RansDesc*)(T+P.o_rans); OpenDesc* dO=(OpenDesc*)(T+P.o_open); DnaDesc* dD=(DnaDesc*)(T+P.o_dna);
    const uint32_t NR=(uint32_t)P.rans.size(), NO=(uint32_t)P.open.size(), ND=(uint32_t)P.dna.size(), NN=(uint32_t)P.nv.size(), NW=(uint32_t)P.raw.size();
    kg_init<<<1,32,0,s>>>(err);
    if(NR) kg_fix_rans<<<blocks_for(NR),256,0,s>>>((const agp::Rans*)(M+pl->o_rans),dR,NR,T);
    if(NO) kg_fix_open<<<blocks_for(NO),256,0,s>>>((const agp::Open*)(M+pl->o_open),dO,NO,T);
    if(ND) kg_fix_dna<<<blocks_for(ND),256,0,s>>>((const agp::Dna*)(M+pl->o_dna),dD,ND,T);
    const void** cp=(const void**)(T+P.o_nvcp); void** op=(void**)(T+P.o_nvop); size_t* cs=(size_t*)(T+P.o_nvcs); size_t* os=(size_t*)(T+P.o_nvos);
    size_t* act=(size_t*)(T+P.o_nvact); int* nst=(int*)(T+P.o_nvst);
    if(NN) kg_fix_nv<<<blocks_for(NN),256,0,s>>>((const agp::Nv*)(M+pl->o_nv),NN,in,T,cp,op,cs,os);
    auto raws=[&](agp::Seg g){ if(g.hi>g.lo) kg_raw<<<g.hi-g.lo,256,0,s>>>((const agp::Raw*)(M+pl->o_raw)+g.lo,in,T); };
    auto zstd=[&](agp::Seg g)->bool{ if(g.hi<=g.lo) return true;
#ifdef ACEAPEX_GPU_NVCOMP
        const size_t i0=g.lo, n=g.hi-g.lo;
        if(nvcompBatchedZstdDecompressAsync(cp+i0,cs+i0,os+i0,act+i0,n,T+P.o_nvtmp,(size_t)P.nv_temp,op+i0,nvcompBatchedZstdDecompressDefaultOpts,(nvcompStatus_t*)(nst+i0),s)!=nvcompSuccess) return false;
        kg_nvcheck<<<blocks_for(n),256,0,s>>>(nst+i0,act+i0,os+i0,(uint32_t)n,err);
        return true;
#else
        return false;
#endif
    };
    // rANS pieces with the windowed refill (k_rans<1>, AX_OPEN_SEQ in the tool: Blackwell chr1 seq 0.309 -> 0.238 ms)
    auto pieces=[&](agp::Seg g){ if(g.hi>g.lo){ const uint32_t n=g.hi-g.lo; k_rans<1><<<(n+AXW_WARPS-1)/AXW_WARPS,32*AXW_WARPS,0,s>>>(in,dR+g.lo,n,err); } };
    const unsigned gy1=(unsigned)((P.chunk[0]/AXU_PER+255)/256), gyo=(unsigned)((P.chunk[0]/16+255)/256);
    if((ND && gy1>65535) || (NO && gyo>65535)) return ACEAPEX_GPU_E_ARCHIVE;
    auto dna=[&](agp::Seg g){ if(g.hi>g.lo){ const uint32_t n=g.hi-g.lo; k_unpack<<<dim3(n,gy1),256,0,s>>>(dD+g.lo); k_exc<<<n,256,0,s>>>(dD+g.lo); } };
    auto open=[&](agp::Seg g){ if(g.hi>g.lo){ const uint32_t n=g.hi-g.lo;
        // case runs + exception positions, then bases with case and exceptions in one store (AX_OPEN_EXC), the block's
        // run ends and exception positions read from shared memory (AX_OPEN_SHB: Blackwell T2T unpack 8.95 -> 7.46 ms);
        // AX_GPU_TILE: no bases kernel, the match kernel builds each block's literals in shared memory
        k_open_cg<<<n,AXO_NT,0,s>>>(dO+g.lo,err+2); if(!pl->tile) k_open_bases_s<<<dim3(n,gyo),256,0,s>>>(dO+g.lo); } };
    const agp::Seg all_r{0,NR}, all_o{0,NO}, all_d{0,ND}, all_w{0,NW};
    uint32_t b0=0, b1=P.nb; uint8_t* mout=out;
    phase("init + fixups",s);
    if(!S){ raws(all_w); phase("stored chunks",s);
        if(!zstd(agp::Seg{0,(uint32_t)P.NT})) return ACEAPEX_GPU_E_NVCOMP; phase("nvCOMP zstd token frames",s);
        if(!zstd(agp::Seg{(uint32_t)P.NT,NN})) return ACEAPEX_GPU_E_NVCOMP; phase("nvCOMP zstd literal frames",s);
        pieces(all_r); phase("rANS pieces",s); dna(all_d); phase("DNA unpack",s); open(all_o); phase("open DNA pack",s); }
    else {
        for(int st=1;st<4;st++){ raws(S->raw[st]); if(!zstd(S->nv_tok[st])) return ACEAPEX_GPU_E_NVCOMP; pieces(S->rans_tok[st]); }
        if(!zstd(S->nv_lit)) return ACEAPEX_GPU_E_NVCOMP;
        for(int q=agp::C_SEQ;q<agp::C_N;q++) pieces(S->rans_cls[q]);
        dna(S->dna); open(S->open);
        b0=S->b0; b1=S->b1; mout=T+P.temp_bytes-(uint64_t)b0*P.bs;          // window after the temp layout
        phase("range jobs",s);
    }
    uint32_t* ctr=(uint32_t*)(T+P.o_ctr);
    kg_set<<<1,1,0,s>>>(ctr,b0);
    const uint64_t lanes=(uint64_t)(b1-b0)*G; const unsigned want=(unsigned)std::min<uint64_t>((lanes+TPB-1)/TPB,0x7fffffffull);
    if(pl->tile){ const unsigned want_t=(unsigned)std::min<uint64_t>((uint64_t)(b1-b0)/AXT_SLOTS+1,0x7fffffffull);
        k_decode_t<<<std::max(1u,std::min(pl->grid_t,want_t)),32*AXT_SLOTS,0,s>>>(T+P.o_s[1],T+P.o_s[2],T+P.o_s[3],(const BlockOffsets*)(M+pl->o_bo),
            P.orig,P.bs,mout,ctr,b1,err+4,dO,(const uint32_t*)(M+pl->o_cmap),T+P.o_s[0],(uint32_t)P.cmap.size(),P.chunk[0],P.ssz[0]); }
    else
    k_decode_g<G><<<std::max(1u,std::min(pl->grid,want)),TPB,0,s>>>(T+P.o_s[0],T+P.o_s[1],T+P.o_s[2],T+P.o_s[3],(const BlockOffsets*)(M+pl->o_bo),
        P.orig,P.bs,mout,ctr,b1,err+4);           // err[4] bad blocks, err[7] step limit
    phase("match",s);
    return ACEAPEX_GPU_OK;
}

extern "C" int aceapex_gpu_decompress_async(const aceapex_gpu_plan* pl, const void* d_in, void* d_out, void* d_temp, int* d_status, unsigned flags, cudaStream_t s){
    if(!pl || !d_in || !d_out || !d_temp || !d_status || ((uintptr_t)d_temp & 255) || (flags & ~ACEAPEX_GPU_DECODE_FLAGS)) return ACEAPEX_GPU_E_ARGS;
    if((flags & ACEAPEX_GPU_VERIFY_XXH3) && pl->P.partial) return ACEAPEX_GPU_E_ARGS;   // the hash covers the whole original, not a block window
    int r=run(pl,nullptr,(const uint8_t*)d_in,(uint8_t*)d_out,(uint8_t*)d_temp,d_status,s);
    if(r) return r;
    if(flags & ACEAPEX_GPU_VERIFY_XXH3){
        const agp::Plan& P=pl->P; uint8_t* T=(uint8_t*)d_temp; const uint64_t nb = P.orig>240 ? (P.orig-1)/AXH_BLOCK : 0;
        if(nb) kg_xxh_blocks<<<blocks_for(nb*8),256,0,s>>>((const uint8_t*)d_out,nb,(uint64_t*)(T+P.o_hash));
        phase("XXH3 block terms",s);
        kg_xxh_chain<<<1,8,0,s>>>((const uint8_t*)d_out,P.orig,(const uint64_t*)(T+P.o_hash),P.xxh,(uint32_t*)(T+P.o_err));
        phase("XXH3 chain",s);
    }
    kg_status<<<1,32,0,s>>>((const uint32_t*)((uint8_t*)d_temp+pl->P.o_err),d_status);
    return cudaGetLastError()==cudaSuccess ? ACEAPEX_GPU_OK : ACEAPEX_GPU_E_CUDA;
}
extern "C" int aceapex_gpu_decompress_range_async(const aceapex_gpu_plan* pl, const void* d_in, uint64_t offset, uint64_t length,
                                                  void* d_out, void* d_temp, int* d_status, cudaStream_t s){
    if(!pl || !d_in || !d_out || !d_temp || !d_status || ((uintptr_t)d_temp & 255)) return ACEAPEX_GPU_E_ARGS;
    agp::Sel S; int e=agp::select(pl->P,offset,length,S);
    if(e==-2) return ACEAPEX_GPU_E_RANGE;
    if(e) return ACEAPEX_GPU_E_ARGS;
    int r=run(pl,&S,(const uint8_t*)d_in,(uint8_t*)d_out,(uint8_t*)d_temp,d_status,s);
    if(r) return r;
    const uint8_t* win=(const uint8_t*)d_temp+pl->P.temp_bytes+S.win_off;
    kg_copy<<<blocks_for(length),256,0,s>>>(win,(uint8_t*)d_out,length);
    kg_status<<<1,32,0,s>>>((const uint32_t*)((uint8_t*)d_temp+pl->P.o_err),d_status);
    return cudaGetLastError()==cudaSuccess ? ACEAPEX_GPU_OK : ACEAPEX_GPU_E_CUDA;
}
