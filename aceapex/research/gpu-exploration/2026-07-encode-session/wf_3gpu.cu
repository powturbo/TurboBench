#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cuda_runtime.h>
using namespace std;
#define CK(x) do{cudaError_t e=(x);if(e){printf("err %s\n",cudaGetErrorString(e));exit(1);}}while(0)

__global__ void k_match(uint8_t* out,const uint32_t* tp,const uint32_t* ts,
    const uint32_t* tl,const uint32_t* ord,uint32_t base,uint32_t cnt){
    uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;
    if(i>=cnt)return;
    uint32_t ti=ord[base+i];
    uint32_t dst=tp[ti],src=ts[ti],len=tl[ti];
    for(uint32_t k=0;k<len;k++)out[dst+k]=out[src+k];
}

int main(int argc,char**argv){
    int NGPU=atoi(argv[1]); // 1, 2 или 3
    // Загружаем данные ОДИН раз (вне таймера)
    FILE*ft=fopen("tokens.bin","rb");size_t ntok;fread(&ntok,8,1,ft);
    vector<uint32_t>tp(ntok),ts(ntok),tl(ntok),tlit(ntok);
    fread(tp.data(),4,ntok,ft);fread(ts.data(),4,ntok,ft);
    fread(tl.data(),4,ntok,ft);fread(tlit.data(),4,ntok,ft);fclose(ft);
    FILE*flp=fopen("lit_positions.bin","rb");fseek(flp,0,SEEK_END);
    size_t n=ftell(flp);fseek(flp,0,SEEK_SET);
    uint8_t*lit=(uint8_t*)malloc(n);fread(lit,1,n,flp);fclose(flp);
    FILE*fv=fopen("levels.bin","rb");
    vector<int32_t>lev(ntok);fread(lev.data(),4,ntok,fv);fclose(fv);
    int ml=*max_element(lev.begin(),lev.end());
    vector<uint32_t>cnt_l(ml+1,0);for(auto v:lev)cnt_l[v]++;
    vector<uint32_t>off(ml+2,0);for(int L=0;L<=ml;L++)off[L+1]=off[L]+cnt_l[L];
    vector<uint32_t>order(ntok),cur(off.begin(),off.end());
    for(uint32_t ti=0;ti<ntok;ti++)order[cur[lev[ti]]++]=ti;

    // Готовим буферы и graph на КАЖДОМ GPU (вне таймера)
    struct G{uint8_t*d_out;uint32_t*d_tp,*d_ts,*d_tl,*d_ord;cudaStream_t s;cudaGraphExec_t ex;};
    vector<G> g(NGPU);
    for(int d=0;d<NGPU;d++){
        CK(cudaSetDevice(d));
        G&x=g[d];
        CK(cudaMalloc(&x.d_out,n));CK(cudaMalloc(&x.d_tp,ntok*4));
        CK(cudaMalloc(&x.d_ts,ntok*4));CK(cudaMalloc(&x.d_tl,ntok*4));
        CK(cudaMalloc(&x.d_ord,ntok*4));
        CK(cudaMemcpy(x.d_tp,tp.data(),ntok*4,cudaMemcpyHostToDevice));
        CK(cudaMemcpy(x.d_ts,ts.data(),ntok*4,cudaMemcpyHostToDevice));
        CK(cudaMemcpy(x.d_tl,tl.data(),ntok*4,cudaMemcpyHostToDevice));
        CK(cudaMemcpy(x.d_ord,order.data(),ntok*4,cudaMemcpyHostToDevice));
        CK(cudaMemcpy(x.d_out,lit,n,cudaMemcpyHostToDevice));
        CK(cudaStreamCreate(&x.s));
        CK(cudaStreamBeginCapture(x.s,cudaStreamCaptureModeGlobal));
        for(int L=0;L<=ml;L++)if(cnt_l[L])
            k_match<<<(cnt_l[L]+255)/256,256,0,x.s>>>(x.d_out,x.d_tp,x.d_ts,x.d_tl,x.d_ord,off[L],cnt_l[L]);
        cudaGraph_t gr;CK(cudaStreamEndCapture(x.s,&gr));
        CK(cudaGraphInstantiate(&x.ex,gr,0));
    }
    // Warmup
    for(int d=0;d<NGPU;d++){CK(cudaSetDevice(d));CK(cudaGraphLaunch(g[d].ex,g[d].s));}
    for(int d=0;d<NGPU;d++){CK(cudaSetDevice(d));CK(cudaStreamSynchronize(g[d].s));}

    // ТАЙМЕР: только параллельный decode на всех GPU, 20 итераций
    cudaEvent_t t0,t1;cudaSetDevice(0);
    CK(cudaEventCreate(&t0));CK(cudaEventCreate(&t1));
    // reinit d_out перед каждой итерацией
    auto reinit=[&](){for(int d=0;d<NGPU;d++){CK(cudaSetDevice(d));
        CK(cudaMemcpy(g[d].d_out,lit,n,cudaMemcpyHostToDevice));}};
    double total_ms=0;
    for(int it=0;it<20;it++){
        reinit();
        for(int d=0;d<NGPU;d++){CK(cudaSetDevice(d));CK(cudaDeviceSynchronize());}
        cudaSetDevice(0);cudaEventRecord(t0);
        // launch все GPU
        for(int d=0;d<NGPU;d++){CK(cudaSetDevice(d));CK(cudaGraphLaunch(g[d].ex,g[d].s));}
        // ждём все
        for(int d=0;d<NGPU;d++){CK(cudaSetDevice(d));CK(cudaStreamSynchronize(g[d].s));}
        cudaSetDevice(0);cudaEventRecord(t1);cudaEventSynchronize(t1);
        float ms;cudaEventElapsedTime(&ms,t0,t1);total_ms+=ms;
    }
    double ms=total_ms/20;
    double agg=(double)n*NGPU/1e9/(ms/1000);
    printf("%d GPU: decode %.3f ms | aggregate %.1f GB/s (%.1f per-GPU)\n",
        NGPU,ms,agg,agg/NGPU);
    return 0;
}
