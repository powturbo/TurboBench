// dense2_lane_emu.cpp - CPU mirror of k_r2 (aceapex_gpu.cu, dense-open v2 measurement): the header parse of
// lane 0, the lookup structures as the 32 lanes fill them (V 0 byte slot table, 1 nibble slot table, 2 cum rows
// as u16 pairs compared per halfword, 3 binary search for chunks with K > 16), the prefix of the substream
// lengths and every lane's decode loop with its 32-bit output words, run over an AR2L file
// (components/rans1_seg.c) and compared byte for byte with the literal stream it encodes. As on the GPU,
// chunks with K > 16 always take V 3. No GPU needed.
// Build: g++ -O2 -o dense2_lane_emu scripts/dense2_lane_emu.cpp
// Usage: dense2_lane_emu <file.ar2l> <literal stream>   -> exit 0 when all three variants match
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
static std::vector<uint8_t> slurp(const char* p){ FILE* f=fopen(p,"rb"); if(!f){ perror(p); exit(1); } fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    std::vector<uint8_t> v(n+16,0); if(fread(v.data(),1,n,f)!=(size_t)n) exit(1); fclose(f); v.resize(n); return v; }
static uint32_t rd32(const uint8_t* p){ return p[0]|p[1]<<8|p[2]<<16|(uint32_t)p[3]<<24; }

static uint32_t vcmpleu2(uint32_t a, uint32_t b){ uint32_t r=0;
    if((a&0xFFFF)<=(b&0xFFFF)) r|=0xFFFFu; if((a>>16)<=(b>>16)) r|=0xFFFF0000u; return r; }
// one chunk, variant V (0 byte, 1 nibble, 2 cum compare; K > 16 -> 3 binary search); returns framing errors
static int chunk(const uint8_t* p, uint32_t n, uint8_t* o, int V){
    if(n==0 || p[0]==1){ memcpy(o,p+1,n); return 0; }
    static uint8_t slot[16*4096]; uint16_t cum[64][65]; uint8_t sym[64]; uint32_t cw[16][9];
    const int K=p[1]; if(K>64) return 1; if(K>16) V=3;
    const uint8_t* q=p+2;
    for(int i=0;i<K;i++) sym[i]=q[i];
    q+=K;
    uint64_t acc=0; int nb=0;
    auto get=[&](int bits)->uint32_t{ while(nb<bits){ acc|=(uint64_t)(*q++)<<nb; nb+=8; } uint32_t v=(uint32_t)(acc&((1u<<bits)-1)); acc>>=bits; nb-=bits; return v; };
    uint64_t ru=0; for(int i=0;i<K;i++) ru|=(uint64_t)get(1)<<i;
    for(int i=0;i<K;i++){ cum[i][0]=0;
        if(!((ru>>i)&1)){ for(int j=0;j<K;j++) cum[i][j+1]=0; continue; }
        uint64_t cu=0; for(int j=0;j<K;j++) cu|=(uint64_t)get(1)<<j;
        uint32_t cc=0; for(int j=0;j<K;j++){ if((cu>>j)&1) cc+=get(12)+1; cum[i][j+1]=(uint16_t)cc; } }
    if(*q++!=32) return 1;
    const uint8_t* lens=q;
    if(V<=1) for(int ln=0;ln<32;ln++) for(int i=0;i<K;i++){ if(cum[i][K]==0) continue;
        uint32_t m0=ln*128; int j=0; while(cum[i][j+1]<=m0) j++;
        if(V==1){ uint8_t* w=slot+i*2048+ln*64;
            for(int k=0;k<16;k++){ uint32_t v=0; for(int b=0;b<8;b++){ uint32_t m=m0+k*8+b; while(cum[i][j+1]<=m) j++; v|=(uint32_t)j<<(4*b); } memcpy(w+4*k,&v,4); } }
        else { uint8_t* w=slot+i*4096+ln*128;
            for(int k=0;k<32;k++){ uint32_t v=0; for(int b=0;b<4;b++){ uint32_t m=m0+k*4+b; while(cum[i][j+1]<=m) j++; v|=(uint32_t)j<<(8*b); } memcpy(w+4*k,&v,4); } } }
    if(V==2) for(int e=0;e<K*8;e++){ const int i=e>>3, w=e&7, j0=2*w+1, j1=2*w+2;
        const uint32_t lo = j0<=K ? cum[i][j0] : 0xFFFFu, hi = j1<=K ? cum[i][j1] : 0xFFFFu; cw[i][w]=lo|hi<<16; }
    int err=0; uint32_t incl=0;
    for(int ln=0;ln<32;ln++){
        const uint32_t L=lens[2*ln]|(uint32_t)lens[2*ln+1]<<8; incl+=L;
        const uint8_t* b=lens+64+(incl-L);
        const uint32_t qq=(n/32)&~3u, lo=ln*qq, len= ln==31 ? n-31*qq : qq;
        uint32_t x=rd32(b); const uint8_t* pk=b+4; uint32_t ctx=0, w=0;
        for(uint32_t t=0;t<len;t++){
            const uint32_t m=x&4095u; uint32_t j;
            if(V==0) j=slot[ctx*4096+m];
            else if(V==1) j=(slot[ctx*2048+(m>>1)]>>((m&1)*4))&15u;
            else if(V==2){ uint32_t mm=m|m<<16, cnt=0; for(int k=0;k<8;k++) cnt+=__builtin_popcount(vcmpleu2(cw[ctx][k],mm)); j=cnt>>4; }
            else { int a=0, h=K; while(a<h){ const int mid=(a+h)>>1; if(cum[ctx][mid+1]<=m) a=mid+1; else h=mid; } j=(uint32_t)a; }
            const uint32_t st=cum[ctx][j], f=cum[ctx][j+1]-st;
            x=f*(x>>12)+m-st; ctx=j;
            w|=(uint32_t)sym[j]<<(8*(t&3));
            if((t&3)==3){ memcpy(o+lo+4*(t>>2),&w,4); w=0; }
            if(x<(1u<<15)){ x=(x<<16)|pk[0]|((uint32_t)pk[1]<<8); pk+=2; }
        }
        for(uint32_t t=len&~3u;t<len;t++) o[lo+t]=(uint8_t)(w>>(8*(t&3)));
        if(pk!=b+L) err++;
    }
    return err;
}
int main(int argc, char** argv){
    if(argc<3){ fprintf(stderr,"usage: %s <file.ar2l> <literal stream>\n",argv[0]); return 1; }
    std::vector<uint8_t> r=slurp(argv[1]), lit=slurp(argv[2]);
    if(r.size()<8 || memcmp(r.data(),"AR2L",4) || r[5]!=32){ fprintf(stderr,"not AR2L NS=32\n"); return 1; }
    int rc=0;
    static const char* NM[3]={"byte table","nibble table","cum compare"};
    for(int V=0;V<3;V++){
        std::vector<uint8_t> out(lit.size()+64,0); size_t pos=8, dst=0, nc=0, err=0, wide=0;
        while(pos+8<=r.size()){ uint32_t cn=rd32(&r[pos]), cs=rd32(&r[pos+4]); pos+=8; if(dst+cn>lit.size()) return 2;
            wide+= cs>1 && r[pos]!=1 && r[pos+1]>16; err+=chunk(&r[pos],cn,&out[dst],V); pos+=cs; dst+=cn; nc++; }
        size_t bad=0; for(size_t i=0;i<lit.size();i++) bad+= out[i]!=lit[i];
        bad+= dst!=lit.size();
        printf("%s: %zu chunks (%zu with K > 16 by binary search), %zu bytes, %zu differ, %zu framing errors -> %s\n",NM[V],nc,wide,dst,bad,err,bad||err?"DIFFERS":"MATCHES");
        if(bad||err) rc=2;
    }
    return rc;
}
