#include "dietgpu/ans/GpuANSCodec.h"
#include "dietgpu/utils/StackDeviceMemory.h"
#include <cuda_runtime.h>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace dietgpu;

int main(int argc, char** argv){
  if(argc<2){printf("usage: %s file\n",argv[0]);return 1;}
  FILE* f=fopen(argv[1],"rb"); fseek(f,0,SEEK_END); size_t n=ftell(f); fseek(f,0,SEEK_SET);
  std::vector<uint8_t> hbuf(n);
  if(fread(hbuf.data(),1,n,f)!=n){printf("read fail\n");return 1;} fclose(f);
  printf("input: %.1f MB\n", n/1e6);

  // большой стек — 4GB
  auto res = makeStackMemory((size_t)4*1024*1024*1024);
  cudaStream_t stream; cudaStreamCreate(&stream);

  uint32_t bs = 65536;
  uint32_t numBatch = (n + bs - 1)/bs;

  // input на device одним куском
  uint8_t* d_in; cudaMalloc(&d_in,n); cudaMemcpy(d_in,hbuf.data(),n,cudaMemcpyHostToDevice);

  // HOST-массивы указателей и размеров (API берёт host)
  std::vector<const void*> inPtrs(numBatch);
  std::vector<uint32_t> batchSizes(numBatch);
  for(uint32_t i=0;i<numBatch;i++){
    inPtrs[i]=d_in+(size_t)i*bs;
    batchSizes[i]=(i==numBatch-1)?(uint32_t)(n-(size_t)i*bs):bs;
  }

  uint32_t maxC = getMaxCompressedSize(bs);
  auto enc_dev = res.alloc<uint8_t>(stream,(size_t)numBatch*maxC);
  std::vector<void*> encPtrs(numBatch);
  for(uint32_t i=0;i<numBatch;i++) encPtrs[i]=(uint8_t*)enc_dev.data()+(size_t)i*maxC;
  auto encSize_dev = res.alloc<uint32_t>(stream,numBatch);

  ansEncodeBatchPointer(res,ANSCodecConfig(10,true),numBatch,
      inPtrs.data(),batchSizes.data(),nullptr,
      encPtrs.data(),encSize_dev.data(),stream);
  cudaStreamSynchronize(stream);

  auto encSize = encSize_dev.copyToHost(stream);
  size_t totalEnc=0; for(auto v:encSize) totalEnc+=v;
  printf("compressed: %.1f MB  ratio %.2fx\n", totalEnc/1e6,(double)n/totalEnc);

  // decode
  uint8_t* d_dec; cudaMalloc(&d_dec,n);
  std::vector<void*> decPtrs(numBatch);
  for(uint32_t i=0;i<numBatch;i++) decPtrs[i]=d_dec+(size_t)i*bs;
  auto success_dev = res.alloc<uint8_t>(stream,numBatch);
  auto outSize_dev = res.alloc<uint32_t>(stream,numBatch);

  for(int w=0;w<3;w++)
    ansDecodeBatchPointer(res,ANSCodecConfig(10,true),numBatch,
        (const void**)encPtrs.data(),decPtrs.data(),batchSizes.data(),
        success_dev.data(),outSize_dev.data(),stream);
  cudaStreamSynchronize(stream);

  cudaEvent_t t0,t1; cudaEventCreate(&t0); cudaEventCreate(&t1);
  int IT=30; cudaEventRecord(t0,stream);
  for(int i=0;i<IT;i++)
    ansDecodeBatchPointer(res,ANSCodecConfig(10,true),numBatch,
        (const void**)encPtrs.data(),decPtrs.data(),batchSizes.data(),
        success_dev.data(),outSize_dev.data(),stream);
  cudaEventRecord(t1,stream); cudaStreamSynchronize(stream);
  float ms; cudaEventElapsedTime(&ms,t0,t1); ms/=IT;
  printf("DietGPU ANS decode: %.3f ms  %.1f GB/s\n", ms, n/1e9/(ms/1000));

  std::vector<uint8_t> out(n); cudaMemcpy(out.data(),d_dec,n,cudaMemcpyDeviceToHost);
  printf("bit-perfect: %s\n", memcmp(out.data(),hbuf.data(),n)==0?"YES":"NO");
  return 0;
}
