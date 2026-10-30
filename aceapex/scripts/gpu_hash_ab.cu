// gpu_hash_ab.cu - #N1: A/B of output hashing on the GPU, one archive, one card, one binary, three modes:
//   a  decode only                                    (aceapex_gpu_decompress_async, flags 0)
//   b  decode + XXH3 of the whole output              (ACEAPEX_GPU_VERIFY_XXH3: kg_xxh_blocks + kg_xxh_chain)
//   c  decode + nvCOMP nvcompBatchedCRC32Async per block (CRC-32/ISO-HDLC, zlib's) + D2H of the block CRCs
//      + crc32_combine on the host -> one CRC-32 of the file
// Timed with cudaEvent on one stream (c: device part by events, host combine by the wall clock, both reported),
// N runs per mode after a warm-up, median and p90. The file CRC of mode c is printed for the check against
// zlib.crc32 of the CPU decode (done by scripts/colab_hash_ab.sh).
// Build: nvcc -O3 -std=c++17 -arch=sm_XX -Isrc -I<nvcomp>/include -DACEAPEX_GPU_NVCOMP? (not needed for open
//        archives) -o gpu_hash_ab scripts/gpu_hash_ab.cu src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp
//        -L<nvcomp>/lib -l:libnvcomp.so.6 -lz
// Usage: gpu_hash_ab archive.aet [runs=10]
#include "aceapex_gpu.h"
#include <nvcomp/crc32.h>
#include <zlib.h>
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <chrono>

#define CK(x) do{cudaError_t e_=(x); if(e_!=cudaSuccess){fprintf(stderr,"CUDA %s @%d: %s\n",#x,__LINE__,cudaGetErrorString(e_)); exit(2);} }while(0)
static double wall(){ return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static void stats(std::vector<double> v, double& med, double& p90){ std::sort(v.begin(),v.end()); med=v[v.size()/2]; p90=v[std::min(v.size()-1,(size_t)(0.9*(v.size()-1)+0.5))]; }

int main(int argc, char** argv){
    if(argc<2){ fprintf(stderr,"usage: %s archive.aet [runs=10]\n",argv[0]); return 1; }
    const int R=argc>2?atoi(argv[2]):10;
    FILE* f=fopen(argv[1],"rb"); if(!f){ perror(argv[1]); return 1; } fseek(f,0,SEEK_END); std::vector<char> a(ftell(f)); fseek(f,0,SEEK_SET);
    if(fread(a.data(),1,a.size(),f)!=a.size()) return 1; fclose(f);
    uint32_t bs; memcpy(&bs,a.data()+20,4);
    aceapex_gpu_plan* plan=aceapex_gpu_plan_create(a.data(),a.size(),0); if(!plan){ fprintf(stderr,"plan: error %d\n",aceapex_gpu_last_error()); return 1; }
    const size_t n=aceapex_gpu_output_bytes(plan), nb=(n+bs-1)/bs;
    void *d_in,*d_out,*d_temp; int* d_status; cudaStream_t s; CK(cudaStreamCreate(&s));
    CK(cudaMalloc(&d_in,a.size())); CK(cudaMalloc(&d_out,n+256)); CK(cudaMalloc(&d_temp,aceapex_gpu_temp_bytes(plan))); CK(cudaMalloc(&d_status,sizeof(int)));
    CK(cudaMemcpy(d_in,a.data(),a.size(),cudaMemcpyHostToDevice));
    // CRC inputs: one chunk per archive block of the output
    std::vector<const void*> hp(nb); std::vector<size_t> hn(nb);
    for(size_t b=0;b<nb;b++){ hp[b]=(const char*)d_out+b*(size_t)bs; hn[b]=std::min<size_t>(bs,n-b*(size_t)bs); }
    const void** dp; size_t* dn; uint32_t* dcrc; nvcompStatus_t* dst;
    CK(cudaMalloc(&dp,nb*sizeof(void*))); CK(cudaMalloc(&dn,nb*sizeof(size_t))); CK(cudaMalloc(&dcrc,nb*4)); CK(cudaMalloc(&dst,nb*sizeof(nvcompStatus_t)));
    CK(cudaMemcpy(dp,hp.data(),nb*sizeof(void*),cudaMemcpyHostToDevice)); CK(cudaMemcpy(dn,hn.data(),nb*sizeof(size_t),cudaMemcpyHostToDevice));
    nvcompBatchedCRC32Opts_t opts; memset(&opts,0,sizeof(opts)); opts.spec=nvcompCRC32;
    // kernel configuration: nvCOMP's heuristic (by max chunk size, then deduced from the sizes); if both refuse,
    // the zeroed configuration (warp kernel) - which one was used is printed
    const char* conf="heuristic(max chunk)";
    nvcompStatus_t hs=nvcompBatchedCRC32GetHeuristicConf(nvcompCRC32IgnoredInputChunkBytes,nb,&opts.kernel_conf,bs,s);
    if(hs!=nvcompSuccess){ conf="heuristic(deduced)"; memset(&opts.kernel_conf,0,sizeof(opts.kernel_conf)); hs=nvcompBatchedCRC32GetHeuristicConf(dn,nb,&opts.kernel_conf,nvcompCRC32DeducedMaxInputChunkBytes,s); }
    if(hs!=nvcompSuccess){ fprintf(stderr,"nvcomp heuristic conf: status %d, using the zeroed (warp kernel) configuration\n",(int)hs); conf="zeroed (warp kernel)"; memset(&opts.kernel_conf,0,sizeof(opts.kernel_conf)); }
    CK(cudaStreamSynchronize(s));
    std::vector<uint32_t> hcrc(nb);
    cudaEvent_t e0,e1; CK(cudaEventCreate(&e0)); CK(cudaEventCreate(&e1));
    int status=-1; uint32_t file_crc=0;
    auto run=[&](char mode, double& dev_ms, double& host_ms){
        CK(cudaEventRecord(e0,s));
        int r=aceapex_gpu_decompress_async(plan,d_in,d_out,d_temp,d_status,mode=='b'?ACEAPEX_GPU_VERIFY_XXH3:0u,s);
        if(r){ fprintf(stderr,"decode returned %d\n",r); exit(4); }
        if(mode=='c'){
            if(nvcompBatchedCRC32Async(dp,dn,nb,dcrc,opts,nvcompCRC32OnlySegment,dst,s)!=nvcompSuccess){ fprintf(stderr,"nvcompBatchedCRC32Async failed\n"); exit(5); }
            CK(cudaMemcpyAsync(hcrc.data(),dcrc,nb*4,cudaMemcpyDeviceToHost,s));
        }
        CK(cudaMemcpyAsync(&status,d_status,sizeof(int),cudaMemcpyDeviceToHost,s));
        CK(cudaEventRecord(e1,s)); CK(cudaEventSynchronize(e1));
        float ms; CK(cudaEventElapsedTime(&ms,e0,e1)); dev_ms=ms; host_ms=0;
        if(mode=='c'){ double t0=wall(); uint32_t c=hcrc[0]; for(size_t b=1;b<nb;b++) c=(uint32_t)crc32_combine(c,hcrc[b],(z_off_t)hn[b]); file_crc=c; host_ms=(wall()-t0)*1e3; }
        if(status){ fprintf(stderr,"mode %c: device status %d\n",mode,status); exit(6); }
    };
    cudaDeviceProp pr; int dev; CK(cudaGetDevice(&dev)); CK(cudaGetDeviceProperties(&pr,dev));
    printf("archive %s: %zu B -> %zu B, %zu blocks of %u, %s, runs %d per mode; nvCOMP CRC32 conf: %s\n",argv[1],a.size(),n,nb,bs,pr.name,R,conf);
    double dm,hm; for(char m:{'a','b','c'}) run(m,dm,hm);            // warm-up
    double base=0;
    printf("mode | device ms med / p90 | host combine ms med | total ms med | GB/s (total) | x decode\n");
    for(char m:{'a','b','c'}){
        std::vector<double> d,h,t; for(int r=0;r<R;r++){ run(m,dm,hm); d.push_back(dm); h.push_back(hm); t.push_back(dm+hm); }
        double dmed,dp90,hmed,hp90,tmed,tp90; stats(d,dmed,dp90); stats(h,hmed,hp90); stats(t,tmed,tp90);
        if(m=='a') base=tmed;
        printf("%c    | %.3f / %.3f | %.3f | %.3f (p90 %.3f) | %.1f | x%.2f\n",m,dmed,dp90,hmed,tmed,tp90,n/tmed/1e6,tmed/base);
    }
    printf("file CRC-32 (mode c, crc32_combine of %zu block CRCs): %08x\n",nb,file_crc);
    printf("XXH3 check (mode b): status 0 = match\n");
    aceapex_gpu_plan_destroy(plan); return 0;
}
