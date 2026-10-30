// nvcomp_frame_repro.cu - one zstd frame through nvCOMP's batched decoder alone (batch of 1), against libzstd on
// the CPU: does nvCOMP finish, which status, which size, the same bytes as libzstd. A watchdog polls the stream
// (AX_WATCHDOG seconds, default 60): past it the line NVCOMP HANG and exit 4 (a hung kernel cannot be stopped).
// Inputs: verify/repro/*.zst (verify/repro/README.md). Colab: scripts/colab_gpu_open.sh, before gpu_api_test.
// Build: nvcc -O3 -arch=sm_XX -I<nvcomp>/include scripts/nvcomp_frame_repro.cu -l:libnvcomp.so.5 -lzstd
// Usage: nvcomp_frame_repro <frame.zst> <decoded size> [<frame.zst> <decoded size> ...]
#include <cuda_runtime.h>
#include <nvcomp/zstd.h>
#include <zstd.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <vector>
#define CK(x) do{ cudaError_t e_=(x); if(e_!=cudaSuccess){ printf("CUDA %s @%d: %s\n",#x,__LINE__,cudaGetErrorString(e_)); exit(2);} }while(0)
static bool wait(cudaStream_t s, double lim){
    const auto t0=std::chrono::steady_clock::now();
    for(;;){ cudaError_t e=cudaStreamQuery(s); if(e==cudaSuccess) return true;
        if(e!=cudaErrorNotReady){ printf("CUDA error: %s\n",cudaGetErrorString(e)); exit(2); }
        if(std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count()>lim) return false;
        std::this_thread::sleep_for(std::chrono::microseconds(200)); } }
int main(int argc, char** argv){
    setvbuf(stdout,nullptr,_IONBF,0);
    if(argc<3 || argc%2==0){ fprintf(stderr,"usage: %s <frame.zst> <decoded size> ...\n",argv[0]); return 1; }
    const double lim=getenv("AX_WATCHDOG")?atof(getenv("AX_WATCHDOG")):60;
    cudaStream_t s; CK(cudaStreamCreate(&s));
    for(int a=1;a+1<argc;a+=2){
        FILE* f=fopen(argv[a],"rb"); if(!f){ perror(argv[a]); return 1; }
        std::vector<uint8_t> z; fseek(f,0,SEEK_END); z.resize(ftell(f)); fseek(f,0,SEEK_SET); if(fread(z.data(),1,z.size(),f)!=z.size()) return 1; fclose(f);
        const size_t osz=strtoull(argv[a+1],0,10);
        std::vector<uint8_t> c(osz+64); const size_t r=ZSTD_decompress(c.data(),osz,z.data(),z.size());
        printf("%s: %zu B -> %zu B; libzstd: %s\n",argv[a],z.size(),osz,ZSTD_isError(r)?ZSTD_getErrorName(r):(r==osz?"ok":"other size"));
        uint8_t *dz,*dout; void *dtmp; const void** dcp; void** dop; size_t *dcs,*dos,*dact; nvcompStatus_t* dst;
        size_t tmp=0; if(nvcompBatchedZstdDecompressGetTempSizeAsync(1,osz,nvcompBatchedZstdDecompressDefaultOpts,&tmp,osz)!=nvcompSuccess){ printf("  nvCOMP temp size failed\n"); return 3; }
        CK(cudaMalloc(&dz,z.size()+256)); CK(cudaMalloc(&dout,osz+256)); CK(cudaMalloc(&dtmp,tmp+256)); CK(cudaMalloc(&dcp,8)); CK(cudaMalloc(&dop,8));
        CK(cudaMalloc(&dcs,8)); CK(cudaMalloc(&dos,8)); CK(cudaMalloc(&dact,8)); CK(cudaMalloc(&dst,sizeof(nvcompStatus_t)));
        CK(cudaMemcpy(dz,z.data(),z.size(),cudaMemcpyHostToDevice)); CK(cudaMemset(dout,0,osz));
        const void* hcp=dz; void* hop=dout; const size_t hcs=z.size(), hos=osz;
        CK(cudaMemcpy(dcp,&hcp,8,cudaMemcpyHostToDevice)); CK(cudaMemcpy(dop,&hop,8,cudaMemcpyHostToDevice));
        CK(cudaMemcpy(dcs,&hcs,8,cudaMemcpyHostToDevice)); CK(cudaMemcpy(dos,&hos,8,cudaMemcpyHostToDevice));
        const auto t0=std::chrono::steady_clock::now();
        const nvcompStatus_t q=nvcompBatchedZstdDecompressAsync(dcp,dcs,dos,dact,1,dtmp,tmp,dop,nvcompBatchedZstdDecompressDefaultOpts,dst,s);
        if(q!=nvcompSuccess){ printf("  nvCOMP launch: status %d\n",(int)q); continue; }
        if(!wait(s,lim)){ printf("NVCOMP HANG on %s: not finished in %.0f s (nvCOMP alone on this frame; known for nvCOMP 5.3.0.16, verify/repro/README.md)\n",argv[a],lim); fflush(stdout); _exit(4); }
        nvcompStatus_t hs; size_t act=0; std::vector<uint8_t> g(osz);
        CK(cudaMemcpy(&hs,dst,sizeof hs,cudaMemcpyDeviceToHost)); CK(cudaMemcpy(&act,dact,8,cudaMemcpyDeviceToHost)); CK(cudaMemcpy(g.data(),dout,osz,cudaMemcpyDeviceToHost));
        printf("  nvCOMP: finished in %.1f ms, status %d, decoded %zu B%s\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count(),(int)hs,act,
            ZSTD_isError(r)?"":(!memcmp(g.data(),c.data(),osz)?", == libzstd":", != libzstd"));
        cudaFree(dz); cudaFree(dout); cudaFree(dtmp); cudaFree(dcp); cudaFree(dop); cudaFree(dcs); cudaFree(dos); cudaFree(dact); cudaFree(dst);
    }
    return 0;
}
