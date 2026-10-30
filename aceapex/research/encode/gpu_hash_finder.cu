// gpu_hash_finder.cu — проверка Q3 второго инженера ФАКТОМ.
// Его заявление: stage1 (cub::DeviceRadixSort ключей) + stage2 (parallel match-find)
// на H100 даёт 500MB/s-1GB/s. Измеряем реально.
//
// Stage1: key[i] = load64(T+i) (первые 8 байт), sort (key,pos) через cub radix
// Stage2: одна нить на позицию, скан D соседей с pos'<i, extend LCP -> (best_src,best_len)
// Stage3: greedy parse на host (его признание: serial, но дёшев)
// bit-perfect check.
//
// Build: nvcc -O3 -arch=sm_90 -o gpu_hash_finder gpu_hash_finder.cu
// Run:   ./gpu_hash_finder <file> [max_bytes] [k_bytes] [chain_D]
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cuda_runtime.h>
#include <cub/cub.cuh>

#define CK(x) do{cudaError_t e=(x); if(e){printf("CUDA err %s:%d %s\n",__FILE__,__LINE__,cudaGetErrorString(e));exit(1);} }while(0)

// key = первые min(k,8) байт как uint64 (big-endian для правильного порядка)
__global__ void k_makekeys(const uint8_t* T, uint32_t n, int k, uint64_t* keys, uint32_t* poss){
    uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;
    if(i>=n) return;
    uint64_t v=0;
    #pragma unroll
    for(int j=0;j<8;j++){ v=(v<<8)|((i+j<n && j<k)?T[i+j]:0); }
    keys[i]=v; poss[i]=i;
}

// stage2: для каждой позиции (в sorted-порядке по ключу) смотрим D левых соседей
// в отсортированном массиве, у кого pos<i, extend LCP. best -> per original position.
__global__ void k_find(const uint8_t* T, uint32_t n, int D,
                       const uint32_t* sorted_pos, uint32_t* best_src, uint32_t* best_len){
    uint32_t s=blockIdx.x*blockDim.x+threadIdx.x;  // индекс в отсортированном массиве
    if(s>=n) return;
    uint32_t i=sorted_pos[s];  // оригинальная позиция
    uint32_t bl=0, bs=0xFFFFFFFF;
    // соседи по сортировке = похожие префиксы. смотрим D влево и вправо.
    for(int d=1; d<=D; d++){
        // влево
        if(s>=(uint32_t)d){
            uint32_t src=sorted_pos[s-d];
            if(src<i){ uint32_t l=0,lim=n-i; while(l<lim && T[i+l]==T[src+l]) l++; if(l>bl){bl=l;bs=src;} }
        }
        // вправо
        if(s+d<n){
            uint32_t src=sorted_pos[s+d];
            if(src<i){ uint32_t l=0,lim=n-i; while(l<lim && T[i+l]==T[src+l]) l++; if(l>bl){bl=l;bs=src;} }
        }
    }
    best_len[i]=bl; best_src[i]=bs;
}

int main(int argc,char**argv){
    if(argc<2){ printf("Usage: %s <file> [max_bytes] [k] [D]\n",argv[0]); return 1; }
    FILE* f=fopen(argv[1],"rb"); if(!f){perror("open");return 1;}
    fseek(f,0,SEEK_END); long fsz=ftell(f); fseek(f,0,SEEK_SET);
    uint32_t n=(argc>2)?(uint32_t)atoi(argv[2]):(uint32_t)fsz; if(n>(uint32_t)fsz)n=fsz;
    int k=(argc>3)?atoi(argv[3]):4; int D=(argc>4)?atoi(argv[4]):64;
    std::vector<uint8_t> h(n);
    if(fread(h.data(),1,n,f)!=n){printf("short read\n");return 1;} fclose(f);
    printf("=== GPU hash-finder (Q3 проверка) n=%u k=%d D=%d ===\n",n,k,D);

    uint8_t* dT; CK(cudaMalloc(&dT,n)); CK(cudaMemcpy(dT,h.data(),n,cudaMemcpyHostToDevice));
    uint64_t *dKeys,*dKeysOut; uint32_t *dPos,*dPosOut,*dBsrc,*dBlen;
    CK(cudaMalloc(&dKeys,n*8)); CK(cudaMalloc(&dKeysOut,n*8));
    CK(cudaMalloc(&dPos,n*4));  CK(cudaMalloc(&dPosOut,n*4));
    CK(cudaMalloc(&dBsrc,n*4)); CK(cudaMalloc(&dBlen,n*4));

    cudaEvent_t t0,t1,t2,t3; for(auto p:{&t0,&t1,&t2,&t3})CK(cudaEventCreate(p));
    CK(cudaEventRecord(t0));
    // stage1a: make keys
    k_makekeys<<<(n+255)/256,256>>>(dT,n,k,dKeys,dPos);
    CK(cudaDeviceSynchronize()); CK(cudaEventRecord(t1));
    // stage1b: cub radix sort (key,pos)
    void* dTemp=nullptr; size_t tempBytes=0;
    cub::DeviceRadixSort::SortPairs(dTemp,tempBytes,dKeys,dKeysOut,dPos,dPosOut,n);
    CK(cudaMalloc(&dTemp,tempBytes));
    cub::DeviceRadixSort::SortPairs(dTemp,tempBytes,dKeys,dKeysOut,dPos,dPosOut,n);
    CK(cudaDeviceSynchronize()); CK(cudaEventRecord(t2));
    // stage2: find
    k_find<<<(n+255)/256,256>>>(dT,n,D,dPosOut,dBsrc,dBlen);
    CK(cudaDeviceSynchronize()); CK(cudaEventRecord(t3));
    CK(cudaEventSynchronize(t3));
    float ms_key,ms_sort,ms_find;
    CK(cudaEventElapsedTime(&ms_key,t0,t1));
    CK(cudaEventElapsedTime(&ms_sort,t1,t2));
    CK(cudaEventElapsedTime(&ms_find,t2,t3));
    float ms_total=ms_key+ms_sort+ms_find;

    // stage3 host: greedy parse + bit-perfect
    std::vector<uint32_t> bsrc(n),blen(n);
    CK(cudaMemcpy(bsrc.data(),dBsrc,n*4,cudaMemcpyDeviceToHost));
    CK(cudaMemcpy(blen.data(),dBlen,n*4,cudaMemcpyDeviceToHost));
    std::vector<uint8_t> recon; recon.reserve(n);
    uint32_t matches=0,lits=0; uint64_t cov=0,i=0;
    while(i<n){ uint32_t L=blen[i],src=bsrc[i];
        if(L>=4 && src<i){ for(uint32_t z=0;z<L;z++)recon.push_back(h[src+z]); cov+=L;matches++;i+=L; }
        else{ recon.push_back(h[i]); lits++; i++; } }
    bool bp=(recon.size()==n)&&memcmp(recon.data(),h.data(),n)==0;

    printf("\n--- GPU stages (H100) ---\n");
    printf("makekeys: %.3f ms | radix-sort: %.3f ms | find: %.3f ms | TOTAL(1-2): %.3f ms\n",
        ms_key,ms_sort,ms_find,ms_total);
    printf("GPU throughput (stage1-2, index+find): %.1f MB/s\n", n/(ms_total*1e-3)/1e6);
    printf("  из них radix-sort: %.1f MB/s (это cub, argued быстрый)\n", n/(ms_sort*1e-3)/1e6);
    printf("coverage=%.1f%% matches=%u lits=%u\n", 100.0*cov/n,matches,lits);
    printf("BIT-PERFECT: %s\n", bp?"YES OK":"NO FAIL");
    printf("\nЧЕСТНО: parse (stage3) на host serial, не в throughput. PCIe не считан.\n");
    printf("Это проверка argued 500MB/s-1GB/s второго инженера ФАКТОМ на H100.\n");
    return 0;
}
