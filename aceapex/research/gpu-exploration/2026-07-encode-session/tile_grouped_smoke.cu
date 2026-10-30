// tile_grouped_smoke.cu — проверяет, что группировка (цикл Stride-вызовов по
// группам <=6000 тайлов) даёт bit-perfect на ОБЩЕМ числе тайлов ВЫШЕ потолка
// одиночного вызова (6800+). Изолированно, до полной интеграции.
//
// Механизм: один непрерывный буфер под ВЕСЬ поток; encode/decode идут группами
// по GROUP тайлов, каждая группа — Stride-вызов со смещением в общий буфер.
// Если это bit-perfect на 10000 тайлов (> потолка 6800), путь к e2e_pipe_tile
// открыт.
#include <cstdio>
#include <cstdint>
#include <vector>
#include <random>
#include <cstring>
#include <cuda_runtime.h>
#include "dietgpu/ans/GpuANSCodec.h"
#include "dietgpu/utils/StackDeviceMemory.h"
using namespace dietgpu; using namespace std;
#define CK(x) do{cudaError_t e=(x); if(e){printf("CUDA err %s:%d %s\n",__FILE__,__LINE__,cudaGetErrorString(e));return false;}}while(0)

static const uint32_t TILE=65536;
static const uint32_t GROUP=6000;   // безопасно ниже потолка 6800

static bool grouped_roundtrip(uint32_t numTiles, StackDeviceMemory& res, cudaStream_t s){
    uint64_t padded=(uint64_t)numTiles*TILE;
    vector<uint8_t> h(padded); mt19937 rng(7);
    for(auto&b:h) b=(uint8_t)(rng()&0xFF);

    uint8_t* dIn; CK(cudaMalloc(&dIn,padded)); CK(cudaMemcpy(dIn,h.data(),padded,cudaMemcpyHostToDevice));
    uint32_t mc=getMaxCompressedSize(TILE);
    // общий compressed-буфер на ВСЕ тайлы (один cudaMalloc)
    uint8_t* dC; CK(cudaMalloc(&dC,(size_t)numTiles*mc));
    uint32_t* dS; CK(cudaMalloc(&dS,numTiles*4));
    uint8_t* dO; CK(cudaMalloc(&dO,padded)); CK(cudaMemset(dO,0,padded));
    uint8_t* dSucc; CK(cudaMalloc(&dSucc,numTiles)); uint32_t* dDsz; CK(cudaMalloc(&dDsz,numTiles*4));
    ANSCodecConfig cfg(10,false);

    // ENCODE группами: каждая группа пишет в свой сегмент общего буфера
    for(uint32_t g0=0; g0<numTiles; g0+=GROUP){
        uint32_t gn=(g0+GROUP<numTiles)?GROUP:(numTiles-g0);
        const void* inSeg = dIn + (uint64_t)g0*TILE;
        void* outSeg = dC + (uint64_t)g0*mc;
        ansEncodeBatchStride(res,cfg,gn,inSeg,TILE,TILE,nullptr,outSeg,mc,dS+g0,s);
    }
    CK(cudaStreamSynchronize(s));

    // DECODE группами: каждая группа читает свой сегмент, пишет в свой выход
    for(uint32_t g0=0; g0<numTiles; g0+=GROUP){
        uint32_t gn=(g0+GROUP<numTiles)?GROUP:(numTiles-g0);
        const void* inSeg = dC + (uint64_t)g0*mc;
        void* outSeg = dO + (uint64_t)g0*TILE;
        ansDecodeBatchStride(res,cfg,gn,inSeg,mc,outSeg,TILE,TILE,dSucc+g0,dDsz+g0,s);
    }
    CK(cudaStreamSynchronize(s));

    vector<uint8_t> ho(padded); CK(cudaMemcpy(ho.data(),dO,padded,cudaMemcpyDeviceToHost));
    bool ok=(memcmp(ho.data(),h.data(),padded)==0);
    cudaFree(dIn);cudaFree(dC);cudaFree(dS);cudaFree(dO);cudaFree(dSucc);cudaFree(dDsz);
    return ok;
}

int main(){
    printf("=== grouped tile-ANS smoke (GROUP=%u, TILE=%u) ===\n", GROUP, TILE);
    printf("Проверка: bit-perfect на числе тайлов ВЫШЕ потолка одиночного вызова (6800)?\n\n");
    auto res=dietgpu::makeStackMemory((size_t)2*1024*1024*1024);
    cudaStream_t s; cudaStreamCreate(&s);
    uint32_t sweep[]={7000,8000,10000,12000};  // все > потолка 6800
    bool all=true;
    for(uint32_t n:sweep){
        bool ok=grouped_roundtrip(n,res,s);
        printf("numTiles=%u (%.0f MB) : %s\n", n, (double)n*TILE/1e6, ok?"bit-perfect OK":"MISMATCH");
        if(!ok){ all=false; break; }
    }
    cudaStreamDestroy(s);
    printf("\n=== VERDICT ===\n");
    if(all) printf("GREEN: группировка работает выше потолка. Путь к e2e_pipe_tile открыт.\n");
    else    printf("RED: группировка не спасает — нужен другой подход.\n");
    return all?0:1;
}
