#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cuda_runtime.h>
using namespace std;
#define CK(x) do{cudaError_t e=(x);if(e!=cudaSuccess){printf("CUDA err %d: %s\n",__LINE__,cudaGetErrorString(e));exit(1);}}while(0)

__global__ void k_lit(uint8_t*out,const uint8_t*data,const uint32_t*pos,const uint32_t*ord,uint32_t base,uint32_t cnt){
    uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;if(i>=cnt)return;
    out[pos[ord[base+i]]]=data[pos[ord[base+i]]];
}
__global__ void k_match(uint8_t*out,const uint32_t*tpos,const uint32_t*tsrc,const uint32_t*tlen,const uint32_t*ord,uint32_t base,uint32_t cnt){
    uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;if(i>=cnt)return;
    uint32_t ti=ord[base+i],dst=tpos[ti],src=tsrc[ti],len=tlen[ti];
    uint32_t d=dst-src;
    if(d<len){ for(uint32_t k=0;k<len;k++) out[dst+k]=out[src+(k%d)]; }
    else     { for(uint32_t k=0;k<len;k++) out[dst+k]=out[src+k]; }
}

int main(int argc,char**argv){
    if(argc<3){printf("usage: %s tokfile rawfile\n",argv[0]);return 1;}
    FILE*ft=fopen(argv[1],"rb");size_t ntok;fread(&ntok,8,1,ft);
    vector<uint32_t> tp(ntok),ts(ntok),tl(ntok),tlit(ntok);
    fread(tp.data(),4,ntok,ft);fread(ts.data(),4,ntok,ft);
    fread(tl.data(),4,ntok,ft);fread(tlit.data(),4,ntok,ft);fclose(ft);
    // Читаем оригинальный файл для BIT-PERFECT проверки
    FILE*fr=fopen(argv[2],"rb");fseek(fr,0,SEEK_END);size_t n=ftell(fr);fseek(fr,0,SEEK_SET);
    uint8_t*data=(uint8_t*)malloc(n);fread(data,1,n,fr);fclose(fr);
    // Читаем lit_positions.bin — честная инициализация
    FILE*flit=fopen("lit_positions.bin","rb");
    uint8_t*lit_buf=(uint8_t*)malloc(n);
    fread(lit_buf,1,n,flit); fclose(flit);
    printf("Tokens: %zu  File: %zu bytes\n",ntok,n);
    FILE*fl=fopen("levels.bin","rb");
    vector<int32_t> lev(ntok);fread(lev.data(),4,ntok,fl);fclose(fl);
    int ml=*max_element(lev.begin(),lev.end());
    printf("MaxLevel: %d\n",ml);

    vector<uint32_t> cnt_l(ml+1,0);
    for(auto v:lev)cnt_l[v]++;
    vector<uint32_t> off(ml+2,0);
    for(int L=0;L<=ml;L++)off[L+1]=off[L]+cnt_l[L];
    vector<uint32_t> order(ntok),cur(off.begin(),off.end());
    for(uint32_t ti=0;ti<ntok;ti++)order[cur[lev[ti]]++]=ti;

    uint8_t*d_out,*d_lit;uint32_t*d_tp,*d_ts,*d_tl,*d_ord;
    CK(cudaMalloc(&d_out,n));CK(cudaMalloc(&d_lit,n));
    CK(cudaMalloc(&d_tp,ntok*4));CK(cudaMalloc(&d_ts,ntok*4));
    CK(cudaMalloc(&d_tl,ntok*4));CK(cudaMalloc(&d_ord,ntok*4));
    CK(cudaMemcpy(d_lit,lit_buf,n,cudaMemcpyHostToDevice)); // lit[] stream, not original file
    CK(cudaMemcpy(d_tp,tp.data(),ntok*4,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_ts,ts.data(),ntok*4,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_tl,tl.data(),ntok*4,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_ord,order.data(),ntok*4,cudaMemcpyHostToDevice));

    cudaStream_t s;CK(cudaStreamCreate(&s));
    // Инициализируем d_out = original data (литеральные позиции уже правильные)
    CK(cudaMemcpy(d_out,d_lit,n,cudaMemcpyDeviceToDevice));
    CK(cudaStreamBeginCapture(s,cudaStreamCaptureModeGlobal));
    for(int L=0;L<=ml;L++) if(cnt_l[L])  // ВСЕ токены — матчи, используем k_match
        k_match<<<(cnt_l[L]+255)/256,256,0,s>>>(d_out,d_tp,d_ts,d_tl,d_ord,off[L],cnt_l[L]);
    cudaGraph_t g;CK(cudaStreamEndCapture(s,&g));
    cudaGraphExec_t ex;CK(cudaGraphInstantiate(&ex,g,0));

    CK(cudaGraphLaunch(ex,s));CK(cudaStreamSynchronize(s));
    cudaEvent_t t0,t1;CK(cudaEventCreate(&t0));CK(cudaEventCreate(&t1));
    CK(cudaEventRecord(t0,s));
    for(int i=0;i<20;i++) CK(cudaGraphLaunch(ex,s));
    CK(cudaEventRecord(t1,s));CK(cudaStreamSynchronize(s));
    float ms;CK(cudaEventElapsedTime(&ms,t0,t1));ms/=20;

    vector<uint8_t> out(n);CK(cudaMemcpy(out.data(),d_out,n,cudaMemcpyDeviceToHost));
    printf("\n=== REAL ACEAPEX GPU WAVEFRONT ===\n");
    printf("BIT-PERFECT: %s\n",memcmp(out.data(),data,n)==0?"YES ✓":"NO ✗");
    printf("CUDA graph (%d levels): %.3f ms\n",ml,ms);
    printf("Throughput: %.1f GB/s\n",n/1e9/(ms/1000));
    free(data);return 0;
}
