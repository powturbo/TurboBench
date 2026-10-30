#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cuda_runtime.h>
using namespace std;
#define CK(x) do{cudaError_t e=(x);if(e){printf("err %s\n",cudaGetErrorString(e));exit(1);}}while(0)

__global__ void k_hash(const uint8_t* buf, size_t n, uint64_t* out){
    if(blockIdx.x==0&&threadIdx.x==0){
        uint64_t h=0xcbf29ce484222325ULL;
        for(size_t i=0;i<n;i++) h=(h^buf[i])*0x100000001b3ULL;
        *out=h;
    }
}
__global__ void k_match(uint8_t* out,const uint32_t* tp,const uint32_t* ts,
    const uint32_t* tl,const uint32_t* ord,uint32_t base,uint32_t cnt){
    uint32_t i=blockIdx.x*blockDim.x+threadIdx.x;
    if(i>=cnt)return;
    uint32_t ti=ord[base+i];
    uint32_t dst=tp[ti],src=ts[ti],len=tl[ti];
    for(uint32_t k=0;k<len;k++)out[dst+k]=out[src+k];
}
int main(int argc,char**argv){
    if(argc<3){printf("usage: %s tokens.bin origfile\n",argv[0]);return 1;}
    FILE*ft=fopen(argv[1],"rb");size_t ntok;fread(&ntok,8,1,ft);
    vector<uint32_t>tp(ntok),ts(ntok),tl(ntok),tlit(ntok);
    fread(tp.data(),4,ntok,ft);fread(ts.data(),4,ntok,ft);
    fread(tl.data(),4,ntok,ft);fread(tlit.data(),4,ntok,ft);fclose(ft);
    FILE*fr=fopen(argv[2],"rb");fseek(fr,0,SEEK_END);size_t n=ftell(fr);
    fseek(fr,0,SEEK_SET);
    uint8_t*data=(uint8_t*)malloc(n);fread(data,1,n,fr);fclose(fr);
    FILE*fl=fopen("lit_positions.bin","rb");
    uint8_t*lit=(uint8_t*)malloc(n);fread(lit,1,n,fl);fclose(fl);
    FILE*fv=fopen("levels.bin","rb");
    vector<int32_t>lev(ntok);fread(lev.data(),4,ntok,fv);fclose(fv);
    int ml=*max_element(lev.begin(),lev.end());
    vector<uint32_t>cnt_l(ml+1,0);for(auto v:lev)cnt_l[v]++;
    vector<uint32_t>off(ml+2,0);for(int L=0;L<=ml;L++)off[L+1]=off[L]+cnt_l[L];
    vector<uint32_t>order(ntok),cur(off.begin(),off.end());
    for(uint32_t ti=0;ti<ntok;ti++)order[cur[lev[ti]]++]=ti;
    uint8_t*d_out;uint32_t*d_tp,*d_ts,*d_tl,*d_ord;uint64_t*d_h;
    CK(cudaMalloc(&d_out,n));CK(cudaMalloc(&d_tp,ntok*4));
    CK(cudaMalloc(&d_ts,ntok*4));CK(cudaMalloc(&d_tl,ntok*4));
    CK(cudaMalloc(&d_ord,ntok*4));CK(cudaMalloc(&d_h,8));
    CK(cudaMemcpy(d_tp,tp.data(),ntok*4,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_ts,ts.data(),ntok*4,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_tl,tl.data(),ntok*4,cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_ord,order.data(),ntok*4,cudaMemcpyHostToDevice));
    // Инициализируем из lit_positions — НЕ из оригинала
    CK(cudaMemcpy(d_out,lit,n,cudaMemcpyHostToDevice));
    // ХЭШ ДО матчей
    k_hash<<<1,1>>>(d_out,n,d_h);CK(cudaDeviceSynchronize());
    uint64_t h_before;CK(cudaMemcpy(&h_before,d_h,8,cudaMemcpyDeviceToHost));
    // Хэш оригинала на CPU
    uint64_t h_orig=0xcbf29ce484222325ULL;
    for(size_t i=0;i<n;i++)h_orig=(h_orig^data[i])*0x100000001b3ULL;
    printf("\n=== HASH DUMP ===\n");
    printf("hash(оригинал):        %016llx\n",(unsigned long long)h_orig);
    printf("hash(d_out ДО матчей): %016llx\n",(unsigned long long)h_before);
    printf("Совпадают ДО: %s\n",h_before==h_orig?"ДА — ПРОБЛЕМА!":"НЕТ — хорошо ✓");
    // Применяем матчи
    cudaStream_t s;CK(cudaStreamCreate(&s));
    CK(cudaStreamBeginCapture(s,cudaStreamCaptureModeGlobal));
    for(int L=0;L<=ml;L++)if(cnt_l[L])
        k_match<<<(cnt_l[L]+255)/256,256,0,s>>>(d_out,d_tp,d_ts,d_tl,d_ord,off[L],cnt_l[L]);
    cudaGraph_t g;CK(cudaStreamEndCapture(s,&g));
    cudaGraphExec_t ex;CK(cudaGraphInstantiate(&ex,g,0));
    CK(cudaGraphLaunch(ex,s));CK(cudaStreamSynchronize(s));
    // Хэш ПОСЛЕ матчей
    k_hash<<<1,1>>>(d_out,n,d_h);CK(cudaDeviceSynchronize());
    uint64_t h_after;CK(cudaMemcpy(&h_after,d_h,8,cudaMemcpyDeviceToHost));
    printf("hash(d_out ПОСЛЕ матч):%016llx\n",(unsigned long long)h_after);
    printf("Совпадают ПОСЛЕ: %s\n",h_after==h_orig?"ДА — wavefront работает! ✓":"НЕТ — ПРОБЛЕМА!");
    printf("\nВЕРДИКТ: %s\n",
        (h_before!=h_orig && h_after==h_orig)?
        "ЧЕСТНЫЙ DECODE ДОКАЗАН":"ТРЕБУЕТ РАССЛЕДОВАНИЯ");
    return 0;
}
