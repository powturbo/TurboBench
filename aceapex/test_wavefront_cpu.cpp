#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>
using namespace std;

int main(int argc, char**argv){
    if(argc<3){printf("usage: %s rawfile tokfile\n",argv[0]);return 1;}

    // Читаем оригинал для проверки
    FILE*fr=fopen(argv[1],"rb");fseek(fr,0,SEEK_END);size_t n=ftell(fr);fseek(fr,0,SEEK_SET);
    uint8_t*data=(uint8_t*)malloc(n);fread(data,1,n,fr);fclose(fr);

    // Читаем lit[] буфер
    FILE*flit=fopen("lit_positions.bin","rb");
    uint8_t*lit_buf=(uint8_t*)malloc(n);
    fread(lit_buf,1,n,flit);fclose(flit);
    printf("File: %zu bytes\n",n);

    // Читаем токены
    FILE*ft=fopen(argv[2],"rb");size_t ntok;fread(&ntok,8,1,ft);
    vector<uint32_t> tp(ntok),ts(ntok),tl(ntok),tlit(ntok);
    fread(tp.data(),4,ntok,ft);fread(ts.data(),4,ntok,ft);
    fread(tl.data(),4,ntok,ft);fread(tlit.data(),4,ntok,ft);fclose(ft);

    // Читаем уровни
    FILE*fl=fopen("levels.bin","rb");
    vector<int32_t> lev(ntok);fread(lev.data(),4,ntok,fl);fclose(fl);
    int ml=*max_element(lev.begin(),lev.end());
    printf("Tokens: %zu MaxLevel: %d\n",ntok,ml);

    // CPU wavefront симуляция
    uint8_t*out=(uint8_t*)malloc(n);
    memcpy(out,lit_buf,n); // инициализируем из lit[] — НЕ из оригинала

    // Сортируем по уровням
    vector<uint32_t> cnt_l(ml+1,0);
    for(auto v:lev)cnt_l[v]++;
    vector<uint32_t> off(ml+2,0);
    for(int L=0;L<=ml;L++)off[L+1]=off[L]+cnt_l[L];
    vector<uint32_t> order(ntok),cur(off.begin(),off.end());
    for(uint32_t ti=0;ti<ntok;ti++)order[cur[lev[ti]]++]=ti;

    // Применяем матчи по уровням
    for(int L=0;L<=ml;L++){
        for(uint32_t i=off[L];i<off[L+1];i++){
            uint32_t ti=order[i];
            uint32_t dst=tp[ti],src=ts[ti],len=tl[ti];
            for(uint32_t k=0;k<len;k++) out[dst+k]=out[src+k];
        }
    }

    // Проверка
    int ok=(memcmp(out,data,n)==0);
    printf("BIT-PERFECT (from lit[] stream): %s\n",ok?"YES":"NO");
    if(!ok){
        for(size_t i=0;i<n;i++) if(out[i]!=data[i]){
            printf("First mismatch at byte %zu: got %02x expected %02x\n",i,out[i],data[i]);
            break;
        }
    }
    free(data);free(lit_buf);free(out);return 0;
}
