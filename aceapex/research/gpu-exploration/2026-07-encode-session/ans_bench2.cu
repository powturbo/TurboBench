// ans_bench2.cu — DietGPU ANS pointer-batch (no stride issues)
#include <cuda.h>
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <chrono>
#include "dietgpu/ans/GpuANSCodec.h"
#include "dietgpu/utils/StackDeviceMemory.h"
using namespace dietgpu;
#define CK(x) do{ cudaError_t e=(x); if(e){printf("CUDA err @ %d: %s\n",__LINE__,cudaGetErrorString(e));exit(1);} }while(0)

static std::vector<uint8_t> readFile(const char* p){
  FILE* f=fopen(p,"rb"); if(!f){printf("open fail %s\n",p);exit(1);}
  fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
  std::vector<uint8_t> b(n); size_t r=fread(b.data(),1,n,f); (void)r; fclose(f); return b;
}

int main(int argc, char** argv){
  if(argc<2){printf("usage: %s <file> [blockKB=64] [probBits=10]\n",argv[0]);return 1;}
  uint32_t blockBytes=(argc>2?atoi(argv[2]):64)*1024;
  int probBits=(argc>3?atoi(argv[3]):10);
  auto host=readFile(argv[1]);
  size_t total=host.size();
  uint32_t numBatch=total/blockBytes;
  if(!numBatch){printf("file too small\n");return 1;}
  size_t used=(size_t)numBatch*blockBytes;
  printf("file=%s  total=%.1fMB  block=%uKB  numBatch=%u  probBits=%d\n",
         argv[1],total/1e6,blockBytes/1024,numBatch,probBits);

  auto res=makeStackMemory((size_t)2*1024*1024*1024);
  cudaStream_t stream; CK(cudaStreamCreate(&stream));

  // per-block device input pointers
  uint8_t* d_in; CK(cudaMalloc(&d_in,used));
  CK(cudaMemcpy(d_in,host.data(),used,cudaMemcpyHostToDevice));

  // per-block compressed buffers
  uint32_t maxComp=getMaxCompressedSize(blockBytes);
  std::vector<uint8_t*> d_comp_bufs(numBatch);
  for(uint32_t i=0;i<numBatch;i++) CK(cudaMalloc(&d_comp_bufs[i],maxComp));

  // host arrays of pointers
  std::vector<const void*> in_ptrs(numBatch);
  std::vector<void*> out_ptrs(numBatch);
  for(uint32_t i=0;i<numBatch;i++){
    in_ptrs[i]=d_in+(size_t)i*blockBytes;
    out_ptrs[i]=d_comp_bufs[i];
  }
  std::vector<uint32_t> inSizes(numBatch,blockBytes);
  uint32_t* d_compSize; CK(cudaMalloc(&d_compSize,numBatch*sizeof(uint32_t)));

  ANSCodecConfig cfg(probBits,false);

  // warmup
  ansEncodeBatchPointer(res,cfg,numBatch,in_ptrs.data(),inSizes.data(),
                        nullptr,out_ptrs.data(),d_compSize,stream);
  CK(cudaStreamSynchronize(stream));

  // timed encode
  const int IT=10; double best=1e30;
  for(int i=0;i<IT;i++){
    auto t0=std::chrono::high_resolution_clock::now();
    ansEncodeBatchPointer(res,cfg,numBatch,in_ptrs.data(),inSizes.data(),
                          nullptr,out_ptrs.data(),d_compSize,stream);
    CK(cudaStreamSynchronize(stream));
    double ms=std::chrono::duration<double,std::milli>(
      std::chrono::high_resolution_clock::now()-t0).count();
    if(ms<best) best=ms;
  }
  double encGBs=used/1e9/(best/1e3);

  // compressed sizes
  std::vector<uint32_t> compSizes(numBatch);
  CK(cudaMemcpy(compSizes.data(),d_compSize,numBatch*sizeof(uint32_t),cudaMemcpyDeviceToHost));
  size_t totalComp=0; for(auto s:compSizes) totalComp+=s;
  double ratio=(double)used/totalComp;

  // decode buffers
  uint8_t* d_dec; CK(cudaMalloc(&d_dec,used));
  std::vector<void*> dec_ptrs(numBatch);
  for(uint32_t i=0;i<numBatch;i++) dec_ptrs[i]=d_dec+(size_t)i*blockBytes;
  std::vector<uint32_t> caps(numBatch,blockBytes);
  uint8_t* d_succ; CK(cudaMalloc(&d_succ,numBatch));
  uint32_t* d_decSize; CK(cudaMalloc(&d_decSize,numBatch*sizeof(uint32_t)));

  // warmup decode
  ansDecodeBatchPointer(res,cfg,numBatch,
                        (const void**)out_ptrs.data(),dec_ptrs.data(),
                        caps.data(),d_succ,d_decSize,stream);
  CK(cudaStreamSynchronize(stream));

  double bestD=1e30;
  for(int i=0;i<IT;i++){
    auto t0=std::chrono::high_resolution_clock::now();
    ansDecodeBatchPointer(res,cfg,numBatch,
                          (const void**)out_ptrs.data(),dec_ptrs.data(),
                          caps.data(),d_succ,d_decSize,stream);
    CK(cudaStreamSynchronize(stream));
    double ms=std::chrono::duration<double,std::milli>(
      std::chrono::high_resolution_clock::now()-t0).count();
    if(ms<bestD) bestD=ms;
  }
  double decGBs=used/1e9/(bestD/1e3);

  // bit-perfect
  std::vector<uint8_t> back(used);
  CK(cudaMemcpy(back.data(),d_dec,used,cudaMemcpyDeviceToHost));
  bool ok=(memcmp(back.data(),host.data(),used)==0);

  printf("ENCODE %.1f GB/s | DECODE %.1f GB/s | ratio %.3f | comp %.1fMB | bit-perfect %s\n",
         encGBs,decGBs,ratio,totalComp/1e6,ok?"YES":"NO");

  for(uint32_t i=0;i<numBatch;i++) cudaFree(d_comp_bufs[i]);
  cudaFree(d_in); cudaFree(d_dec); cudaFree(d_compSize); cudaFree(d_succ); cudaFree(d_decSize);
  return 0;
}
