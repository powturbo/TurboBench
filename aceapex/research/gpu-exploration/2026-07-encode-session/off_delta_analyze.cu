// off_delta_analyze.cu — измеряет РЕАЛЬНЫЙ выигрыш delta-кодирования OFF-потока
// на настоящих offset'ах из streams.bin (не синтетика). Плюс LEN и CMD для полноты.
//
// Механизм: OFF хранит АБСОЛЮТНЫЕ позиции (varint). Гипотеза: соседние matches
// в блоке указывают на близкие абсолютные позиции -> дельты малы -> ниже энтропия.
// Меряем: энтропию raw varint-байт OFF vs delta-varint-байт, на реальных данных.
//
// Build: nvcc -O3 -o off_delta_analyze off_delta_analyze.cu  (только CPU-логика,
//        но .cu чтобы переиспользовать структуры; можно и g++)
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>
#include <map>
using namespace std;

#pragma pack(push,1)
struct AetHdr { char magic[8]; uint32_t version; uint64_t orig_size;
    uint32_t block_size; uint32_t num_blocks; uint8_t xxhash[8];
    uint64_t zlit_sz, zoff_sz, zlen_sz, zcmd_sz; };
struct BlockOffsets { uint64_t lit_off,off_off,len_off,cmd_off;
    uint64_t lit_sz,off_sz,len_sz,cmd_sz; };
#pragma pack(pop)

static inline uint32_t rd_varint(const uint8_t* b, uint32_t& p, uint32_t lim){
    uint32_t v=0,s=0; while(p<lim){uint8_t c=b[p++]; v|=(uint32_t)(c&0x7F)<<s; if(!(c&0x80))return v; s+=7;} return v;
}
// сколько varint-байт занимает значение
static inline int vlen(uint64_t v){ int n=1; while(v>=0x80){v>>=7;n++;} return n; }

// энтропия набора байт -> ожидаемый размер в байтах при идеальном энтропийном кодере
static double entropy_size(const vector<uint8_t>& bytes){
    if(bytes.empty()) return 0;
    map<uint8_t,uint64_t> cnt; for(uint8_t b:bytes) cnt[b]++;
    double H=0; uint64_t n=bytes.size();
    for(auto&kv:cnt){ double pr=(double)kv.second/n; H-=pr*log2(pr); }
    return H*n/8.0;
}

int main(int argc,char**argv){
    const char* path = argc>1?argv[1]:"streams.bin";
    FILE* f=fopen(path,"rb"); if(!f){perror("open");return 1;}
    AetHdr hdr; fread(&hdr,sizeof(hdr),1,f);
    uint32_t nb=hdr.num_blocks;
    vector<BlockOffsets> bo(nb); fread(bo.data(),sizeof(BlockOffsets),nb,f);
    uint64_t totL=0,totO=0,totN=0,totC=0;
    for(auto&b:bo){totL+=b.lit_sz;totO+=b.off_sz;totN+=b.len_sz;totC+=b.cmd_sz;}
    vector<uint8_t> LIT(totL),OFF(totO),LEN(totN),CMD(totC);
    fread(LIT.data(),1,totL,f); fread(OFF.data(),1,totO,f);
    fread(LEN.data(),1,totN,f); fread(CMD.data(),1,totC,f);
    fclose(f);
    printf("blocks=%u block_size=%u  OFF=%.1fMB LEN=%.1fMB CMD=%.1fMB\n",
        nb,hdr.block_size,totO/1e6,totN/1e6,totC/1e6);

    // --- OFF: декодируем абсолютные offset'ы поблочно, строим raw и delta varint ---
    vector<uint8_t> off_raw, off_delta;
    uint64_t noff=0;
    for(uint32_t b=0;b<nb;b++){
        const uint8_t* base=OFF.data()+bo[b].off_off;
        uint32_t sz=(uint32_t)bo[b].off_sz, p=0;
        int64_t prev=0;
        while(p<sz){
            uint32_t v=rd_varint(base,p,sz); noff++;
            // raw: как есть
            uint64_t x=v; while(x>=0x80){off_raw.push_back((x&0x7F)|0x80);x>>=7;} off_raw.push_back(x);
            // delta (zigzag для знака)
            int64_t d=(int64_t)v-prev; prev=v;
            uint64_t zz=(d<<1)^(d>>63);
            while(zz>=0x80){off_delta.push_back((zz&0x7F)|0x80);zz>>=7;} off_delta.push_back(zz);
        }
    }
    double raw_bytes=off_raw.size(), del_bytes=off_delta.size();
    double raw_H=entropy_size(off_raw), del_H=entropy_size(off_delta);
    printf("\n=== OFF (%llu offsets, реальные абсолютные позиции) ===\n",(unsigned long long)noff);
    printf("  raw varint:   %.1f KB на диске, %.1f KB энтропийно\n", raw_bytes/1e3, raw_H/1e3);
    printf("  delta varint: %.1f KB на диске, %.1f KB энтропийно\n", del_bytes/1e3, del_H/1e3);
    printf("  ВЫИГРЫШ delta (varint-размер):    %.3fx\n", raw_bytes/del_bytes);
    printf("  ВЫИГРЫШ delta (после энтропии):   %.3fx  <- реальный для OFF-потока\n", raw_H/del_H);

    // --- то же для LEN (тоже может иметь структуру) ---
    vector<uint8_t> len_raw,len_delta;
    for(uint32_t b=0;b<nb;b++){
        const uint8_t* base=LEN.data()+bo[b].len_off;
        uint32_t sz=(uint32_t)bo[b].len_sz,p=0; int64_t prev=0;
        while(p<sz){ uint32_t v=rd_varint(base,p,sz);
            uint64_t x=v; while(x>=0x80){len_raw.push_back((x&0x7F)|0x80);x>>=7;} len_raw.push_back(x);
            int64_t d=(int64_t)v-prev;prev=v; uint64_t zz=(d<<1)^(d>>63);
            while(zz>=0x80){len_delta.push_back((zz&0x7F)|0x80);zz>>=7;} len_delta.push_back(zz);
        }
    }
    if(!len_raw.empty()){
        double lr=entropy_size(len_raw), ld=entropy_size(len_delta);
        printf("\n=== LEN ===\n  delta после энтропии: %.3fx\n", lr/ld);
    }
    printf("\nЧЕСТНО: это выигрыш ТОЛЬКО OFF/LEN потоков, не всего файла. OFF ~%.0f%% сжатых\n",
        100.0*totO/(totL+totO+totN+totC));
    printf("данных. Общий ratio-выигрыш = этот x доля OFF в сжатом размере.\n");
    return 0;
}
