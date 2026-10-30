// НЕРЕЗОННЫЙ ТЕСТ: decode без вычисления. Все факторы предвычислены как
// (src_abs, dst_abs, len) фиксированной ширины. Kernel = чистый parallel copy.
// Traces decode copy throughput against average match length, in isolation from parsing.
// CORRECTION 2026-07-12: an earlier version of this file compared the result against a
// hard-coded 221 GB/s "real kernel" figure and concluded that parsing was not the
// bottleneck. That figure came from a FASTQ sample with degenerate quality strings and
// the conclusion does not hold. On the real dataset (ENA ERR194147) the fused kernel runs
// at 143 GB/s while this harness reaches 212, and a direct parse-only ablation shows the
// serial per-token parse costs 66% of decode time. Parsing IS a bottleneck at short match
// lengths. This harness measures the copy phase only; draw no conclusion about parsing from it.
#include <cstdio>
#include <cstdint>
#include <cuda_runtime.h>
#include <cstdlib>
#define CK(x) do{cudaError_t e=(x);if(e){printf("ERR %s\n",cudaGetErrorString(e));return 1;}}while(0)

struct Tok { uint32_t src; uint32_t dst; uint32_t len; };

// чистый copy: каждый warp берёт токен, копирует len байт src->dst. НИКАКОГО парсинга.
__global__ void k_purecopy(const uint8_t* __restrict__ buf, uint8_t* __restrict__ out,
                           const Tok* __restrict__ toks, uint32_t ntok){
    uint32_t t = blockIdx.x*blockDim.x/32 + threadIdx.x/32; // один warp на токен
    uint32_t lane = threadIdx.x & 31;
    if(t>=ntok) return;
    Tok k = toks[t];
    // warp копирует len байт, coalesced по lane
    for(uint32_t i=lane; i<k.len; i+=32){
        out[k.dst+i] = buf[k.src+i];
    }
}

int main(int argc, char** argv){
    // симуляция: 1GB выход, средний матч len=32 (как реальный fastq ~ покрытие)
    uint64_t OUT=1073741824ULL;
    uint32_t avglen=(argc>1)?(uint32_t)atoi(argv[1]):32;
    uint32_t ntok=(uint32_t)(OUT/avglen);
    printf("=== PURE-COPY decode тест: OUT=%lu MB, ntok=%u, avglen=%u ===\n",
        (unsigned long)(OUT/1000000), ntok, avglen);

    uint8_t *dbuf,*dout; Tok* dtoks;
    CK(cudaMalloc(&dbuf, OUT));
    CK(cudaMalloc(&dout, OUT));
    CK(cudaMalloc(&dtoks, (uint64_t)ntok*sizeof(Tok)));
    // генерим токены на хосте: src случайно назад, dst последовательно
    Tok* htoks=(Tok*)malloc((uint64_t)ntok*sizeof(Tok));
    uint64_t pos=0;
    for(uint32_t i=0;i<ntok;i++){
        htoks[i].dst=(uint32_t)pos;
        htoks[i].src=(uint32_t)(pos>avglen? pos-avglen-(i%1000): 0); // назад
        htoks[i].len=avglen;
        pos+=avglen; if(pos>=OUT-avglen) pos=0;
    }
    CK(cudaMemcpy(dtoks,htoks,(uint64_t)ntok*sizeof(Tok),cudaMemcpyHostToDevice));

    int TPB=128; // 4 warpа/блок
    uint32_t warps_needed=ntok;
    uint32_t grid=(warps_needed*32 + TPB-1)/TPB;
    // ограничим grid как реальный kernel (nsm*maxblk)
    int nsm=0; cudaDeviceGetAttribute(&nsm,cudaDevAttrMultiProcessorCount,0);

    cudaEvent_t t0,t1; cudaEventCreate(&t0);cudaEventCreate(&t1);
    // warmup
    k_purecopy<<<grid,TPB>>>(dbuf,dout,dtoks,ntok); cudaDeviceSynchronize();
    cudaEventRecord(t0);
    k_purecopy<<<grid,TPB>>>(dbuf,dout,dtoks,ntok);
    cudaEventRecord(t1); cudaEventSynchronize(t1);
    float ms=0; cudaEventElapsedTime(&ms,t0,t1);
    double gbs = (double)OUT/(ms*1e-3)/1e9;
    printf("PURE-COPY throughput: %.1f GB/s (%.3f ms)\n", gbs, ms);
    printf("HBM3 theoretical peak, H100: ~3350 GB/s\n");
    printf("NOTE: this is the COPY phase in isolation. It says nothing about parse cost.\n");
    printf("      On real data the fused kernel is SLOWER than this harness, because the\n");
    printf("      serial per-token parse dominates at short match lengths.\n");
    free(htoks);
    return 0;
}
