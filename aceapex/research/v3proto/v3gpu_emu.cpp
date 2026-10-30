// v3gpu_emu.cpp - k_v3 (v3gpu.cu) on the CPU: the same step algorithm (32 tokens per step, scans,
// literal bytes by binary search over the scanned run starts, matches in token order with the period
// copy), lane by lane, checked byte for byte against the original. Catches logic errors before a GPU run.
// Build: g++ -O2 -std=c++17 -o v3gpu_emu v3gpu_emu.cpp      Usage: v3gpu_emu streams.bin original
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <algorithm>
static std::vector<uint8_t> slurp(const char* p){ FILE* f=fopen(p,"rb"); if(!f){perror(p); exit(1);} fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    std::vector<uint8_t> v(n); if(fread(v.data(),1,n,f)!=(size_t)n) exit(1); fclose(f); return v; }
static inline uint64_t rd64(const uint8_t* p){ uint64_t v; memcpy(&v,p,8); return v; }
static inline uint32_t rd32(const uint8_t* p){ uint32_t v; memcpy(&v,p,4); return v; }
static inline uint32_t varint(const uint8_t* b,size_t& p,size_t lim){ uint32_t v=0,sh=0; while(p<lim&&sh<=28){ uint8_t c=b[p++]; v|=(uint32_t)(c&0x7F)<<sh; if(!(c&0x80)) return v; sh+=7; } return v; }
struct BO { uint64_t lit_off, off_off, len_off, cmd_off, lit_sz, off_sz, len_sz, cmd_sz; };
static void to_v3(const uint8_t* off,size_t osz,const uint8_t* len,size_t nsz,const uint8_t* cmd,size_t csz,
                  std::vector<uint16_t>& LL,std::vector<uint16_t>& ML,std::vector<uint32_t>& D){
    size_t op=0,np=0,cp=0; uint32_t rep[4]={1,2,4,8}, pend=0;
    auto emit=[&](uint32_t ll,uint32_t ml,uint32_t d){
        while(ll>65535){ LL.push_back(65535); ML.push_back(0); D.push_back(0); ll-=65535; }
        uint32_t first=ml>65535?65535:ml; LL.push_back((uint16_t)ll); ML.push_back((uint16_t)first); D.push_back(ml?d:0); ml-=first;
        while(ml){ uint32_t k=ml>65535?65535:ml; LL.push_back(0); ML.push_back((uint16_t)k); D.push_back(d); ml-=k; } };
    while(cp<csz){ uint8_t c=cmd[cp++];
        if(c<0x80){ pend+=c+1; continue; }
        if(c==0xFF){ rep[0]=1;rep[1]=2;rep[2]=4;rep[3]=8; continue; }
        uint32_t l,dist;
        if((c&0xC0)==0x80){ uint32_t ri=(c>>4)&3,lv=c&0x0F; if(lv==0x0F) lv+=varint(len,np,nsz); l=lv+6; dist=rep[ri]; if(ri>0){ for(int i=ri;i>0;i--) rep[i]=rep[i-1]; rep[0]=dist; } }
        else { uint32_t lv=(c==0xFE)?varint(len,np,nsz):(uint32_t)(c&0x3F); l=lv+6; dist=varint(off,op,osz); rep[3]=rep[2];rep[2]=rep[1];rep[1]=rep[0];rep[0]=dist; }
        emit(pend,l,dist); pend=0; }
    if(pend) emit(pend,0,0);
}
int main(int argc,char** argv){
    if(argc<3){ fprintf(stderr,"usage: %s streams.bin original\n",argv[0]); return 1; }
    std::vector<uint8_t> s=slurp(argv[1]), orig=slurp(argv[2]);
    const uint64_t osz=rd64(&s[12]); const uint32_t bs=rd32(&s[20]), nb=rd32(&s[24]);
    std::vector<BO> bo(nb); memcpy(bo.data(),&s[68],64ull*nb);
    uint64_t L=0,O=0,N=0; for(auto& b:bo){ L=std::max(L,b.lit_off+b.lit_sz); O=std::max(O,b.off_off+b.off_sz); N=std::max(N,b.len_off+b.len_sz); }
    const uint8_t* lit=&s[68+64ull*nb]; const uint8_t* off=lit+L; const uint8_t* len=off+O; const uint8_t* cmd=len+N;
    std::vector<uint8_t> out(osz,0);
    for(uint32_t b=0;b<nb;b++){
        std::vector<uint16_t> LL,ML; std::vector<uint32_t> D;
        to_v3(off+bo[b].off_off,bo[b].off_sz,len+bo[b].len_off,bo[b].len_sz,cmd+bo[b].cmd_off,bo[b].cmd_sz,LL,ML,D);
        const uint8_t* bl=lit+bo[b].lit_off; uint8_t* dst=out.data()+(uint64_t)b*bs; const size_t nt=LL.size();
        uint32_t lbase=0,obase=0;
        for(size_t st=0;st<nt;st+=32){
            uint32_t ll[32],ml[32],d[32],lex[32],oex[32],ltot=0,otot=0;
            for(int l=0;l<32;l++){ size_t i=st+l; bool live=i<nt; ll[l]=live?LL[i]:0; ml[l]=live?ML[i]:0; d[l]=live?D[i]:0; lex[l]=ltot; oex[l]=otot; ltot+=ll[l]; otot+=ll[l]+ml[l]; }
            for(uint32_t kb=0;kb<ltot;kb+=32) for(int l=0;l<32;l++){ uint32_t k=kb+l, lo=0, hi=31;
                for(int it=0;it<5;it++){ uint32_t mid=(lo+hi+1)>>1; if(lex[mid]<=k) lo=mid; else hi=mid-1; }
                if(k<ltot) dst[obase+oex[lo]+(k-lex[lo])]=bl[lbase+k]; }
            uint32_t mpos[32], fm=0xffffffffu; bool indep[32];
            for(int j=0;j<32;j++){ mpos[j]=obase+oex[j]+ll[j]; if(ml[j]) fm=std::min(fm,mpos[j]); }
            for(int j=0;j<32;j++) indep[j]=ml[j]>0 && (mpos[j]-d[j])+std::min(d[j],ml[j])<=fm;
            for(int j=0;j<32;j++) if(indep[j]){ uint32_t pos=mpos[j],n=ml[j],dist=d[j],src=pos-dist; for(uint32_t q=0;q<n;q++) dst[pos+q]=dst[src+(q%dist)]; }
            for(int j=0;j<32;j++){ if(!ml[j]||indep[j]) continue; uint32_t pos=mpos[j], n=ml[j], dist=d[j], src=pos-dist;
                if(dist>=n){ for(uint32_t q=0;q<n;q++) dst[pos+q]=dst[src+q]; } else { for(uint32_t q=0;q<n;q++) dst[pos+q]=dst[src+(q%dist)]; } }
            lbase+=ltot; obase+=otot;
        }
    }
    bool ok=memcmp(out.data(),orig.data(),osz)==0;
    printf("k_v3 emulation: %s, %llu B, %u blocks\n",ok?"bit-perfect":"MISMATCH",(unsigned long long)osz,nb);
    return ok?0:1;
}
