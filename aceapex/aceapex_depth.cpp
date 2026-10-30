#include "aceapex.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif
#endif
#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#endif
#ifndef MAP_HUGE_2MB
#define MAP_HUGE_2MB (21 << MAP_HUGE_SHIFT)
#endif
#include <pthread.h>
#include <atomic>
#include <vector>
#include <algorithm>
#include <zstd.h>
#define XXH_STATIC_LINKING_ONLY
#ifndef ACEAPEX_NO_XXH
#define XXH_IMPLEMENTATION
#endif
#include "xxhash.h"
#define OUR_CHECKSUM(buf,sz) XXH3_64bits(buf,sz)
#include "lit_fse.cpp"

#include <vector>
struct DepTok{uint64_t pos,src; uint32_t len;};
static std::vector<DepTok> g_tokens;
static bool g_record=false;
static size_t g_bstart=0;
static uint64_t g_match_calls=0;

 
#define HASH_SIZE    0xFFFF
#define MAX_DIST     (128 * 1024 * 1024)
#define BLOCK_SIZE   (1 * 1024 * 1024)
static size_t g_block_size = BLOCK_SIZE; // runtime adaptive, set in encode_file
static uint8_t* g_forced=nullptr; static size_t g_forced_n=0;  // PHASE2B forced-literal
#define MAX_THREADS  16
#define BLOCK_MARKER 0xFF
#define ZSTD_LEVEL   22
 
struct BlockResult {
    uint8_t* lit_buf; uint8_t* off_buf;
    uint8_t* len_buf; uint8_t* cmd_buf;
    size_t   lit_size, off_size, len_size, cmd_size;
    int      overflow;
};
 
struct PoolState {
    const uint8_t* src; size_t src_size;
    size_t num_blocks; BlockResult* results;
    std::atomic<size_t> next_block;
};
 
struct WorkerArgs {
    int thread_id;
    struct ThreadHashTable* htab;
    PoolState* pool;
};
 
// 4 последних позиции на ключ; поколение вместо memset
struct Direct8 { uint32_t pos[65536][4]; uint16_t gen[65536]; uint8_t cnt[65536]; uint16_t cur; };
struct ThreadHashTable {
    Direct8* d8;
    int64_t*  pos;
    uint32_t* epoch;
    int64_t*  chain;
    uint32_t  cur_epoch;
    uint32_t  hash_mask;
    uint32_t  chain_mask;
    int       max_attempts;
};
 
struct BlockOffsets {
    uint64_t lit_off, off_off, len_off, cmd_off;
    uint64_t lit_sz,  off_sz,  len_sz,  cmd_sz;
};
 
static inline void wv(uint8_t* buf, size_t& ptr, uint32_t val,
                      size_t limit, int& ov, int sid) {
    while (val >= 0x80) {
        if (ptr >= limit) { ov=sid; return; }
        buf[ptr++] = (uint8_t)((val & 0x7F) | 0x80); val >>= 7;
    }
    if (ptr >= limit) { ov=sid; return; }
    buf[ptr++] = (uint8_t)val;
}
 
static int g_mm = 0;
// DIRECT8: прямая таблица по 8 основаниям (16 бит ключа) вместо хэша с цепью.
// Только для алфавита ACGT; иные байты дают код 255 и позиция не индексируется.
// Гистограмма 09.09: при K=8 98.9% корзин пусты или <=2, тяжёлых 1.6 на блок.
static int g_direct8 = 0;
static uint8_t g_nt[256];
static void init_nt(void){ memset(g_nt,255,256); g_nt['A']=g_nt['a']=0; g_nt['C']=g_nt['c']=1;
                           g_nt['G']=g_nt['g']=2; g_nt['T']=g_nt['t']=3; }
static inline int key8(const uint8_t* p, uint32_t* out){
    uint32_t k=0; for(int j=0;j<8;j++){ uint8_t c=g_nt[p[j]]; if(c==255) return 0; k=(k<<2)|c; }
    *out=k; return 1; }
static inline uint32_t ext8(const uint8_t* a,const uint8_t* b,uint32_t maxl){
    uint32_t l=0;
    while(l+8<=maxl){ uint64_t x=*(const uint64_t*)(a+l)^*(const uint64_t*)(b+l);
        if(x) return l+(__builtin_ctzll(x)>>3); l+=8; }
    while(l<maxl&&a[l]==b[l]) l++; return l; }      // MIN_MATCH=N -> нижний порог длины матча (0 = штатные 6/8/10/12)

static inline uint32_t min_match_len(uint32_t dist) {
    uint32_t base;
    if (dist < 128)          base = 6;
    else if (dist < 16384)   base = 8;
    else if (dist < 2097152) base = 10;
    else                     base = 12;
    return (g_mm > (int)base) ? (uint32_t)g_mm : base;
}
 

struct Match { uint32_t len, off; int rep; };
static int g_norep = 0;   // NO_REP=1 -> не эмитить rep-матчи (scan-friendly parse)

static inline int find_matches(const uint8_t* src, size_t pos, size_t bstart, size_t bend,
                                ThreadHashTable* ht, uint32_t* rep, Match* out, int maxout) {
    int max_attempts = ht->max_attempts;
    int n = 0;
    uint32_t maxl = (uint32_t)(bend - pos);
    for (int i = 0; i < 4 && n < maxout; i++) {
        uint32_t d = rep[i]; if (pos < bstart+d) continue;
        if (*(uint32_t*)(src+pos)!=*(uint32_t*)(src+pos-d)) continue;
        uint32_t l=4; while(l<maxl&&src[pos+l]==src[pos-d+l]&&l<65535) l++;
        if (l>=6) out[n++]={l,d,(g_norep?-1:i)};
    }
    if(g_direct8 && ht->d8){
        uint32_t k;
        if(pos+8<=bend && key8(src+pos,&k)){
            Direct8* D=ht->d8;
            if(D->gen[k]==D->cur){
                for(int c=0;c<D->cnt[k]&&n<maxout;c++){
                    uint32_t cur=D->pos[k][c];
                    if(cur<bstart) continue;
                    uint32_t dist=(uint32_t)(pos-cur); if(dist>=MAX_DIST) continue;
                    bool is_rep=false; for(int r=0;r<4;r++) if(dist==rep[r]){is_rep=true;break;}
                    if(is_rep) continue;
                    uint32_t l=ext8(src+pos,src+cur,maxl>65535?65535:maxl);
                    if(l>=min_match_len(dist)) out[n++]={l,dist,-1};
                }
            } else { D->gen[k]=D->cur; D->cnt[k]=0; }
            // вставка: сдвиг, новейшая в [0]
            uint8_t c=D->cnt[k]; if(c<4) D->cnt[k]=c+1;
            for(int j=(c<4?c:3);j>0;j--) D->pos[k][j]=D->pos[k][j-1];
            D->pos[k][0]=(uint32_t)pos;
        }
        // Близкие матчи (dist<128, порог 6 байт) прямая таблица не видит: ей нужно
        // 8 совпавших оснований. Пройти хэш-цепью, но не дальше 128 назад.
        {
            uint32_t h=((*(uint32_t*)(src+pos)*0x9E3779B1u)>>10)&ht->hash_mask;
            int64_t head=(ht->epoch[h]==ht->cur_epoch)?ht->pos[h]:-1;
            ht->pos[h]=(int64_t)pos; ht->epoch[h]=ht->cur_epoch;
            if(head>=0) ht->chain[pos & ht->chain_mask]=head;
            int64_t cur=head; int att=8;
            while(cur>=(int64_t)bstart && att-->0 && n<maxout && pos-cur<128){
                uint32_t dist=(uint32_t)(pos-cur);
                bool is_rep=false; for(int r=0;r<4;r++) if(dist==rep[r]){is_rep=true;break;}
                if(!is_rep && *(uint32_t*)(src+pos)==*(uint32_t*)(src+cur)){
                    uint32_t l=ext8(src+pos,src+cur,maxl>65535?65535:maxl);
                    if(l>=6 && l<8) out[n++]={l,dist,-1};
                }
                int64_t nxt=ht->chain[cur & ht->chain_mask];
                if(nxt<0||nxt>=cur) break; cur=nxt;
            }
        }
        return n;
    }
    uint32_t h=((*(uint32_t*)(src+pos)*0x9E3779B1u)>>10)&ht->hash_mask;
    int64_t head=(ht->epoch[h]==ht->cur_epoch)?ht->pos[h]:-1;
    ht->pos[h]=(int64_t)pos; ht->epoch[h]=ht->cur_epoch;
    if (head>=0) ht->chain[pos & ht->chain_mask]=head;
    int64_t cur=head; int attempts=max_attempts;
    while(cur>=(int64_t)bstart && attempts-->0 && n<maxout) {
        uint32_t dist=(uint32_t)(pos-cur); if(dist>=MAX_DIST) break;
        bool is_rep=false; for(int r=0;r<4;r++) if(dist==rep[r]){is_rep=true;break;}
        if(!is_rep){
            uint32_t mlen=min_match_len(dist);
            if(pos+8<=bend&&*(uint64_t*)(src+pos)==*(uint64_t*)(src+cur)){
                uint32_t l=8; while(l<maxl&&src[pos+l]==src[cur+l]&&l<65535) l++;
                if(l>=mlen) out[n++]={l,dist,-1};
            } else if(*(uint32_t*)(src+pos)==*(uint32_t*)(src+cur)){
                uint32_t l=4; while(l<maxl&&src[pos+l]==src[cur+l]&&l<65535) l++;
                if(l>=mlen) out[n++]={l,dist,-1};
            }
        }
        int64_t nxt=ht->chain[cur & ht->chain_mask];
        if(nxt<0||nxt>=cur) break; cur=nxt;
    }
    return n;
}
static void compress_block(const uint8_t* src, size_t src_size,
                            size_t bstart, size_t bend,
                            ThreadHashTable* ht, BlockResult* res) {
    size_t bsz = bend - bstart;
    size_t cap = bsz * 2 + 1024;
    res->lit_buf = (uint8_t*)malloc(cap);
    res->off_buf = (uint8_t*)malloc(cap * 6);
    res->len_buf = (uint8_t*)malloc(cap * 6);
    res->cmd_buf = (uint8_t*)malloc(cap + cap/4 + 4);
    res->overflow = 0;
    if (!res->lit_buf || !res->off_buf || !res->len_buf || !res->cmd_buf) {
        res->overflow = 99; return;
    }
    size_t lit_cap=cap, off_cap=cap*6, len_cap=cap*6, cmd_cap=cap+cap/4+4;
 
    ht->cur_epoch++;
 
    if(ht->d8) ht->d8->cur++;
    if (ht->cur_epoch == 0) {
        memset(ht->epoch, 0, (ht->hash_mask+1)*sizeof(uint32_t)); ht->cur_epoch = 1;
    }
 
    size_t lit_i=0, off_i=0, len_i=0, cmd_i=0, pos=bstart;
    uint32_t rep[4]={1,2,4,8}, lit_run=0, miss=0;
    int ov=0;
    res->cmd_buf[cmd_i++] = BLOCK_MARKER;

    // ULTRA: Chain flattening origin table
    // origin[local_pos] = original literal source position (local)
    // Chain flattening работает только на первом мегабайте блока: origin —
    // таблица фиксированного размера. Чтение за её границей давало SIGSEGV при
    // ACEAPEX_BS больше 1 MB; теперь крупные блоки просто не флэттенятся.
    #define ORIGIN_CAP (1u<<20)
    static thread_local uint32_t origin[ORIGIN_CAP];
    size_t flat_pos = 0; // track which positions are initialized
    auto init_origin = [&](size_t from, size_t to) {
        for (size_t i = from; i < to && i < ORIGIN_CAP; i++) origin[i] = (uint32_t)i;
    };
    init_origin(0, bsz); // init all as self-referential (literal)
 
    auto flush_lit = [&]() {
        while (lit_run > 0 && !ov) {
            uint32_t chunk = (lit_run > 128) ? 128 : lit_run;
            if (cmd_i >= cmd_cap) { ov=4; return; }
            res->cmd_buf[cmd_i++] = (uint8_t)(chunk-1);
            lit_run -= chunk;
        }
    };
 
    while (pos + 12 < bend && !ov) {
        uint32_t c_len=0, c_off=0; int c_rep=-1;
        Match matches[36]; int nm=find_matches(src,pos,bstart,bend,ht,rep,matches,36);
        for(int mi=0;mi<nm;mi++) if(matches[mi].len>c_len){c_len=matches[mi].len;c_off=matches[mi].off;c_rep=matches[mi].rep;}
        if (c_len >= 6 && c_len < 64 && pos+13 < bend) {
            uint32_t h1=((*(uint32_t*)(src+pos+1)*0x9E3779B1u)>>10)&ht->hash_mask;
            int64_t mp1=(ht->epoch[h1]==ht->cur_epoch)?ht->pos[h1]:-1;
            if (mp1>=0 && (size_t)mp1>=bstart && (size_t)mp1<pos+1) {
                uint32_t dist1=(uint32_t)(pos+1-mp1);
                if (dist1<MAX_DIST && dist1!=rep[0]) {
                    uint32_t mlen1=min_match_len(dist1);
                    uint32_t maxl1=(uint32_t)(bend-pos-1);
                    if (pos+9<=bend && *(uint64_t*)(src+pos+1)==*(uint64_t*)(src+mp1)) {
                        uint32_t l1=8;
                        while (l1<maxl1 && src[pos+1+l1]==src[mp1+l1] && l1<65535) l1++;
                        if (l1 >= mlen1 && l1 > c_len + 1) {
                            if (lit_i < lit_cap) {
                                res->lit_buf[lit_i++]=src[pos]; lit_run++; miss++;
                                pos++;
                                c_len=l1; c_off=dist1; c_rep=-1;
                            }
                        }
                    }
                }
            }
            // Lazy check pos+2
            if (c_len >= 6 && c_len < 64 && pos+14 < bend) {
                uint32_t h2=((*(uint32_t*)(src+pos+2)*0x9E3779B1u)>>10)&ht->hash_mask;
                int64_t mp2=(ht->epoch[h2]==ht->cur_epoch)?ht->pos[h2]:-1;
                if (mp2>=0 && (size_t)mp2>=bstart && (size_t)mp2<pos+2) {
                    uint32_t dist2=(uint32_t)(pos+2-mp2);
                    if (dist2<MAX_DIST && dist2!=rep[0]) {
                        uint32_t maxl2=(uint32_t)(bend-pos-2);
                        if (pos+10<=bend && *(uint64_t*)(src+pos+2)==*(uint64_t*)(src+mp2)) {
                            uint32_t l2=8;
                            while (l2<maxl2 && src[pos+2+l2]==src[mp2+l2] && l2<65535) l2++;
                            if (l2 >= 6 && l2 > c_len + 2 && lit_i+1 < lit_cap) {
                                res->lit_buf[lit_i++]=src[pos]; lit_run++; miss++;
                                res->lit_buf[lit_i++]=src[pos+1]; lit_run++;
                                pos+=2;
                                c_len=l2; c_off=dist2; c_rep=-1;
                            }
                        }
                    }
                }
            }
        }
        if (c_len >= 6 && g_forced) {  // PHASE2B: reject if match covers forced-literal
            size_t gp=pos;  // pos is global output coord in this encoder
            for (uint32_t k=0;k<c_len;k++) if(gp+k<g_forced_n && g_forced[gp+k]){c_len=0;break;}
        }
        if (c_len >= 6) {
            flush_lit(); if (ov) break; miss=0;
            uint32_t lv=c_len-6;
            if (c_rep != -1) {
                if (cmd_i>=cmd_cap) { ov=4; break; }
                if (lv<15) { res->cmd_buf[cmd_i++]=(uint8_t)(0x80|(c_rep<<4)|lv); }
                else { res->cmd_buf[cmd_i++]=(uint8_t)(0x80|(c_rep<<4)|0x0F);
                       wv(res->len_buf,len_i,lv-15,len_cap,ov,3); if(ov) break; }
                uint32_t rd=rep[c_rep];
                for (int i=c_rep;i>0;i--) rep[i]=rep[i-1]; rep[0]=rd;
            } else {
                if (cmd_i>=cmd_cap) { ov=4; break; }
                if (lv<62) { res->cmd_buf[cmd_i++]=(uint8_t)(0xC0|lv); }
                else { res->cmd_buf[cmd_i++]=0xFE;
                       wv(res->len_buf,len_i,lv,len_cap,ov,3); if(ov) break; }
                // ULTRA: Chain flattening with validation
                size_t local_pos = pos - bstart;
                uint32_t flat_off = c_off;
                if (c_off <= local_pos && local_pos < ORIGIN_CAP) {
                    size_t src_local = local_pos - c_off;
                    uint32_t orig_src = origin[src_local];
                    if (orig_src != src_local) {
                        uint32_t candidate = (uint32_t)(local_pos - orig_src);
                        // Validate ALL bytes
                        bool valid = (candidate <= local_pos && candidate < 8388608u);
                        for (size_t fi = 0; fi < c_len && valid; fi++) {
                            if (src[bstart + local_pos - candidate + fi] !=
                                src[bstart + local_pos - c_off + fi])
                                valid = false;
                        }
                        if (valid) flat_off = candidate;
                    }
                    for (size_t fi = 0; fi < c_len && local_pos+fi < ORIGIN_CAP && src_local+fi < ORIGIN_CAP; fi++)
                        origin[local_pos + fi] = origin[src_local + fi];
                }
                wv(res->off_buf,off_i,flat_off,off_cap,ov,2); if(ov) break;
                rep[3]=rep[2]; rep[2]=rep[1]; rep[1]=rep[0]; rep[0]=flat_off;
            }
            // Insert intermediate positions for short matches only
            // Insert intermediate positions for short matches only
            if (c_len < 32) {
              uint32_t step=1+(c_len>>3);
              for(size_t ii=1;ii<c_len&&pos+ii+4<bend;ii+=step){
                uint32_t hh=((*(uint32_t*)(src+pos+ii)*0x9E3779B1u)>>10)&ht->hash_mask;
                ht->chain[(pos+ii)&ht->chain_mask]=ht->pos[hh]; ht->pos[hh]=(int64_t)(pos+ii); ht->epoch[hh]=ht->cur_epoch;
              }
            }
            pos+=c_len; continue;
        }
        if (lit_i>=lit_cap) { ov=1; break; }
        res->lit_buf[lit_i++]=src[pos++]; lit_run++; miss++;
        if (miss>=1 && pos+12<bend) {
            uint32_t hh=((*(uint32_t*)(src+pos)*0x9E3779B1u)>>10)&ht->hash_mask;
            if(hh<=ht->hash_mask) { ht->pos[hh]=(int64_t)pos; ht->epoch[hh]=ht->cur_epoch; }
            if (lit_i>=lit_cap) { ov=1; break; }
            res->lit_buf[lit_i++]=src[pos++]; lit_run++;
        }
    }
    if (!ov) {
        while (pos<bend) {
            if (lit_i>=lit_cap) { ov=1; break; }
            res->lit_buf[lit_i++]=src[pos++]; lit_run++;
        }
        flush_lit();
    }
    res->lit_size=lit_i; res->off_size=off_i;
    res->len_size=len_i; res->cmd_size=cmd_i; res->overflow=ov;
}
 
static void* worker_func(void* arg) {
    WorkerArgs* wa=(WorkerArgs*)arg;
    PoolState*  ps=wa->pool;
    while (true) {
        size_t bid=ps->next_block.fetch_add(1);
        if (bid>=ps->num_blocks) break;
        size_t bstart=bid*g_block_size, bend=bstart+g_block_size;
        if (bend>ps->src_size) bend=ps->src_size;
        compress_block(ps->src,ps->src_size,bstart,bend,wa->htab,&ps->results[bid]);
    }
    return nullptr;
}
 
// WILD COPY: dst выделяется с DST_PAD запасом, поэтому можно писать за len.
// Один 16-байтовый store вместо вызова memcpy на матч в 7-8 байт.
#define DST_PAD 64

static inline void wild16(uint8_t* d, const uint8_t* s, uint32_t len) {
    uint32_t i = 0;
    do {
        __builtin_memcpy(d + i, s + i, 16);   // компилятор разворачивает в один store
        i += 16;
    } while (i < len);
}

static inline void copy_match(uint8_t* dst, size_t out_ptr, uint32_t dist, uint32_t len,
                              size_t safe_end) {
    uint8_t* d = dst + out_ptr;
    const uint8_t* s = dst + out_ptr - dist;

    // wild copy пишет округляя вверх до 16; блоки декодируются параллельно в общий
    // буфер, поэтому выходить за границу СВОЕГО блока нельзя
    const bool room = (out_ptr + ((size_t)len + 15u & ~(size_t)15u)) <= safe_end;

    if (__builtin_expect(dist >= 16 && room, 1)) {
        wild16(d, s, len);
        return;
    }
    if (dist >= len) { memcpy(d, s, len); return; }
    if (dist == 1) { memset(d, s[0], len); return; }

    // dist < 16: сначала размножаем паттерн до >=16 байт побайтно (максимум 15 шагов),
    // дальше копируем широко — вместо len/dist вызовов memcpy по 3 байта
    // копируем первые dist байт, затем удваиваем уже готовый префикс
    for (uint32_t k = 0; k < dist && k < len; k++) d[k] = s[k];
    uint32_t have = dist;
    while (have < len) {
        uint32_t take = have < (len - have) ? have : (len - have);
        memcpy(d + have, d, take);
        have += take;
    }
}
 
static inline uint32_t read_varint(const uint8_t* buf, size_t& ptr, size_t limit) {
    if (__builtin_expect(ptr < limit, 1)) {
        uint8_t b0 = buf[ptr];
        if (__builtin_expect(!(b0 & 0x80), 1)) { ptr++; return b0; }
        if (__builtin_expect(ptr + 1 < limit, 1)) {
            uint8_t b1 = buf[ptr+1];
            if (__builtin_expect(!(b1 & 0x80), 1)) {
                ptr += 2;
                return (uint32_t)(b0 & 0x7F) | ((uint32_t)b1 << 7);
            }
        }
    }
    uint32_t val=0, shift=0;
    while (ptr<limit) {
        uint8_t b=buf[ptr++]; val|=(uint32_t)(b&0x7F)<<shift;
        if (!(b&0x80)) return val; shift+=7;
    }
    return val;
}
 

// ULTRA: D=1 depth analyzer
// Checks if ALL match sources are in literal-only zones (never in match-written zones)
// If true: two-phase decode is provably correct
static void analyze_depth(
    size_t dst_size,
    const uint8_t* off, size_t off_sz,
    const uint8_t* len, size_t len_sz,
    const uint8_t* cmd, size_t cmd_sz, size_t block_id)
{
    // First pass: collect all ops with positions
    struct Op { uint32_t dst; uint32_t src; uint32_t len; uint8_t is_lit; };
    static Op ops[131072];
    size_t n=0;
    size_t op=0,np=0,cp=0,out=0;
    uint32_t rep[4]={1,2,4,8};
    while(out<dst_size && cp<cmd_sz && n<131072) {
        uint8_t c=cmd[cp++];
        if(c==0xFF){rep[0]=1;rep[1]=2;rep[2]=4;rep[3]=8;continue;}
        if(c<0x80) {
            uint32_t l=c+1;
            ops[n++]={(uint32_t)out,(uint32_t)0,l,1};
            out+=l;
        } else if((c&0xC0)==0x80) {
            uint32_t ri=(c>>4)&3,lv=c&0x0F;
            if(lv==0x0F)lv+=read_varint(len,np,len_sz);
            uint32_t l=lv+6,dist=rep[ri];
            if(ri>0){for(int i=ri;i>0;i--)rep[i]=rep[i-1];rep[0]=dist;}
            if(!dist||out+l>dst_size)break;
            ops[n++]={(uint32_t)out,(uint32_t)(out-dist),l,0};
            out+=l;
        } else {
            uint32_t lv=(c==0xFE)?read_varint(len,np,len_sz):(uint32_t)(c&0x3F);
            uint32_t l=lv+6,dist=read_varint(off,op,off_sz);
            rep[3]=rep[2];rep[2]=rep[1];rep[1]=rep[0];rep[0]=dist;
            if(!dist||out+l>dst_size)break;
            ops[n++]={(uint32_t)out,(uint32_t)(out-dist),l,0};
            out+=l;
        }
    }

    // Second pass: for each match, check if src overlaps any MATCH dst
    // Build match dst ranges first
    size_t d1_safe=0, d2_dep=0;
    for(size_t i=0;i<n;i++) {
        if(ops[i].is_lit) continue;
        uint32_t src_start=ops[i].src;
        uint32_t src_end=ops[i].src+ops[i].len;
        bool has_match_dep=false;
        // Check all previous match destinations
        for(size_t j=0;j<i;j++) {
            if(ops[j].is_lit) continue; // skip literals
            uint32_t mdst_start=ops[j].dst;
            uint32_t mdst_end=ops[j].dst+ops[j].len;
            // Does our src overlap this match's dst?
            if(src_start < mdst_end && src_end > mdst_start) {
                has_match_dep=true;
                break;
            }
        }
        if(has_match_dep) d2_dep++;
        else d1_safe++;
    }
    fprintf(stderr, "block %zu: matches=%zu D1_safe=%.1f%% D2_dep=%.1f%%\n",
        block_id, d1_safe+d2_dep,
        (d1_safe+d2_dep)?100.0*d1_safe/(d1_safe+d2_dep):0,
        (d1_safe+d2_dep)?100.0*d2_dep/(d1_safe+d2_dep):0);
}

static void decompress_streams(
    uint8_t* dst, size_t dst_size,
    const uint8_t* lit, size_t lit_sz,
    const uint8_t* off, size_t off_sz,
    const uint8_t* len, size_t len_sz,
    const uint8_t* cmd, size_t cmd_sz)
{
    size_t lp=0, op=0, np=0, cp=0, out=0;
    uint32_t rep[4]={1,2,4,8};
    while (out<dst_size && cp<cmd_sz) {
        uint8_t c=cmd[cp++];
        if (c==0xFF) { rep[0]=1;rep[1]=2;rep[2]=4;rep[3]=8; continue; }
        if (c<0x80) {
            uint32_t l=c+1;
            if (lp+l>lit_sz||out+l>dst_size) break;
            memcpy(dst+out,lit+lp,l); out+=l; lp+=l;
        } else if ((c&0xC0)==0x80) {
            uint32_t ri=(c>>4)&3, lv=c&0x0F;
            if (lv==0x0F) lv+=read_varint(len,np,len_sz);
            uint32_t l=lv+6, dist=rep[ri];
            if (ri>0) { for(int i=ri;i>0;i--) rep[i]=rep[i-1]; rep[0]=dist; }
            if (!dist||out+l>dst_size) break;
            copy_match(dst,out,dist,l,dst_size);
                if(g_record){g_match_calls++;int64_t src64=(int64_t)g_bstart+(int64_t)out-(int64_t)dist;
                    if(src64>=0) g_tokens.push_back({(uint64_t)(g_bstart+out),(uint64_t)src64,(uint32_t)l});
                } out+=l;
        } else {
            uint32_t lv=(c==0xFE)?read_varint(len,np,len_sz):(uint32_t)(c&0x3F);
            uint32_t l=lv+6, dist=read_varint(off,op,off_sz);
            rep[3]=rep[2];rep[2]=rep[1];rep[1]=rep[0];rep[0]=dist;
            if (!dist||out+l>dst_size) break;
            copy_match(dst,out,dist,l,dst_size);
                if(g_record){
                    int64_t src64=(int64_t)g_bstart+(int64_t)out-(int64_t)dist;
                    if(src64>=0) g_tokens.push_back({(uint64_t)(g_bstart+out),(uint64_t)src64,(uint32_t)l});
                } out+=l;
        }
    }
}
 

// ULTRA: Adaptive parallel decoder
// Checks true source-readiness (not just self-overlap)
static void decompress_adaptive(
    uint8_t* dst, size_t dst_size,
    const uint8_t* lit, size_t lit_sz,
    const uint8_t* off, size_t off_sz,
    const uint8_t* len, size_t len_sz,
    const uint8_t* cmd, size_t cmd_sz)
{
    // Pass 1: decode all literals first into dst
    // This makes all literal bytes "source-ready"
    size_t lp=0, op=0, np=0, cp=0, out=0;
    uint32_t rep[4]={1,2,4,8};
    
    // First pass: literals only
    size_t cp1=0, lp1=0, out1=0;
    uint32_t rep1[4]={1,2,4,8};
    size_t np1=0, op1=0;
    while (out1<dst_size && cp1<cmd_sz) {
        uint8_t c=cmd[cp1++];
        if (c==0xFF){rep1[0]=1;rep1[1]=2;rep1[2]=4;rep1[3]=8;continue;}
        if (c<0x80) {
            uint32_t l=c+1;
            if (lp1+l>lit_sz||out1+l>dst_size) break;
            memcpy(dst+out1, lit+lp1, l);
            out1+=l; lp1+=l;
        } else if ((c&0xC0)==0x80) {
            uint32_t ri=(c>>4)&3,lv=c&0x0F;
            if(lv==0x0F) lv+=read_varint(len,np1,len_sz);
            uint32_t l=lv+6,dist=rep1[ri];
            if(ri>0){for(int i=ri;i>0;i--)rep1[i]=rep1[i-1];rep1[0]=dist;}
            out1+=l; // skip match for now
        } else {
            uint32_t lv=(c==0xFE)?read_varint(len,np1,len_sz):(uint32_t)(c&0x3F);
            uint32_t l=lv+6; read_varint(off,op1,off_sz);
            rep1[3]=rep1[2];rep1[2]=rep1[1];rep1[1]=rep1[0];
            out1+=l; // skip match for now
        }
    }
    size_t lit_ready_end = out1; // all literal positions are ready

    // Second pass: matches only, using lit_ready_end as source-readiness threshold
    size_t cp2=0, lp2=0, out2=0, op2=0, np2=0;
    uint32_t rep2[4]={1,2,4,8};
    while (out2<dst_size && cp2<cmd_sz) {
        uint8_t c=cmd[cp2++];
        if (c==0xFF){rep2[0]=1;rep2[1]=2;rep2[2]=4;rep2[3]=8;continue;}
        if (c<0x80) {
            uint32_t l=c+1; out2+=l; lp2+=l; // already done
        } else if ((c&0xC0)==0x80) {
            uint32_t ri=(c>>4)&3,lv=c&0x0F;
            if(lv==0x0F) lv+=read_varint(len,np2,len_sz);
            uint32_t l=lv+6,dist=rep2[ri];
            if(ri>0){for(int i=ri;i>0;i--)rep2[i]=rep2[i-1];rep2[0]=dist;}
            if(!dist||out2+l>dst_size) break;
            // Source-ready check: src fully before lit_ready_end
            if (out2-dist+l <= lit_ready_end) {
                memcpy(dst+out2, dst+out2-dist, l); // safe parallel copy
            } else {
                copy_match(dst,out2,dist,l,dst_size); // fallback
            }
            out2+=l;
        } else {
            uint32_t lv=(c==0xFE)?read_varint(len,np2,len_sz):(uint32_t)(c&0x3F);
            uint32_t l=lv+6,dist=read_varint(off,op2,off_sz);
            rep2[3]=rep2[2];rep2[2]=rep2[1];rep2[1]=rep2[0];rep2[0]=dist;
            if(!dist||out2+l>dst_size) break;
            if (out2-dist+l <= lit_ready_end) {
                memcpy(dst+out2, dst+out2-dist, l);
            } else {
                copy_match(dst,out2,dist,l,dst_size);
            }
            out2+=l;
        }
    }
}


struct DecOp {
    uint32_t src_off;
    uint32_t dst_off;
    uint32_t len;
    uint8_t  is_lit;
};

// ULTRA: True parallel decoder using Bernstein conditions
// Step 1: Sequential literals (creates ready zone)
// Step 2: Parallel matches where src is in ready zone
static void decompress_parallel(
    uint8_t* dst, size_t dst_size,
    const uint8_t* lit, size_t lit_sz,
    const uint8_t* off, size_t off_sz,
    const uint8_t* len, size_t len_sz,
    const uint8_t* cmd, size_t cmd_sz)
{
    // Build ops list first
    static thread_local DecOp ops_buf[131072];
    size_t ops_cnt = 0;
    size_t lp=0, op=0, np=0, cp=0, out=0;
    uint32_t rep[4]={1,2,4,8};
    while (out<dst_size && cp<cmd_sz) {
        uint8_t c=cmd[cp++];
        if (c==0xFF){rep[0]=1;rep[1]=2;rep[2]=4;rep[3]=8;continue;}
        if (c<0x80) {
            uint32_t l=c+1;
            if (lp+l>lit_sz||out+l>dst_size) break;
            ops_buf[ops_cnt++]={(uint32_t)lp,(uint32_t)out,l,1};
            out+=l; lp+=l;
        } else if ((c&0xC0)==0x80) {
            uint32_t ri=(c>>4)&3,lv=c&0x0F;
            if(lv==0x0F) lv+=read_varint(len,np,len_sz);
            uint32_t l=lv+6,dist=rep[ri];
            if(ri>0){for(int i=ri;i>0;i--)rep[i]=rep[i-1];rep[0]=dist;}
            if(!dist||out+l>dst_size) break;
            ops_buf[ops_cnt++]={(uint32_t)(out-dist),(uint32_t)out,l,0};
            out+=l;
        } else {
            uint32_t lv=(c==0xFE)?read_varint(len,np,len_sz):(uint32_t)(c&0x3F);
            uint32_t l=lv+6,dist=read_varint(off,op,off_sz);
            rep[3]=rep[2];rep[2]=rep[1];rep[1]=rep[0];rep[0]=dist;
            if(!dist||out+l>dst_size) break;
            ops_buf[ops_cnt++]={(uint32_t)(out-dist),(uint32_t)out,l,0};
            out+=l;
        }
    }

    // Step 1: Sequential literals — creates ready zone
    size_t ready_end = 0;
    for (size_t i = 0; i < ops_cnt; i++) {
        if (ops_buf[i].is_lit) {
            memcpy(dst+ops_buf[i].dst_off, lit+ops_buf[i].src_off, ops_buf[i].len);
            size_t end = ops_buf[i].dst_off + ops_buf[i].len;
            if (end > ready_end) ready_end = end;
        }
    }

    // Step 2: Parallel matches where src+len <= ready_end (Bernstein safe)
    // Split into parallel and sequential
    static thread_local size_t par_idx[131072];
    static thread_local size_t seq_idx[131072];
    size_t par_cnt=0, seq_cnt=0;
    for (size_t i = 0; i < ops_cnt; i++) {
        if (!ops_buf[i].is_lit) {
            const DecOp& o = ops_buf[i];
            if (o.src_off + o.len <= ready_end && o.len <= (o.dst_off - o.src_off)) {
                par_idx[par_cnt++] = i;
            } else {
                seq_idx[seq_cnt++] = i;
            }
        }
    }

    // Parallel copies — Bernstein conditions verified
    #pragma omp parallel for schedule(static) num_threads(2)
    for (size_t i = 0; i < par_cnt; i++) {
        const DecOp& o = ops_buf[par_idx[i]];
        memcpy(dst+o.dst_off, dst+o.src_off, o.len);
    }

    // Sequential fallback for dependent copies
    for (size_t i = 0; i < seq_cnt; i++) {
        const DecOp& o = ops_buf[seq_idx[i]];
        copy_match(dst, o.dst_off, o.dst_off-o.src_off, o.len, 0);  // точный путь
    }
}

static const uint32_t K256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};
 
static void sha256(const uint8_t* data, size_t len, uint8_t out[32]) {
    uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                   0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    auto ror=[](uint32_t x,int n){ return (x>>n)|(x<<(32-n)); };
    size_t total=(len+9+63)&~63ULL;
    uint8_t* buf=(uint8_t*)calloc(total,1);
    if(!buf) return;
    memcpy(buf,data,len); buf[len]=0x80;
    uint64_t bits=(uint64_t)len*8;
    for(int i=0;i<8;i++) buf[total-1-i]=(uint8_t)(bits>>(i*8));
    for(size_t off=0;off<total;off+=64) {
        uint32_t w[64];
        for(int i=0;i<16;i++)
            w[i]=((uint32_t)buf[off+i*4]<<24)|((uint32_t)buf[off+i*4+1]<<16)|
                 ((uint32_t)buf[off+i*4+2]<<8)|(uint32_t)buf[off+i*4+3];
        for(int i=16;i<64;i++) {
            uint32_t s0=ror(w[i-15],7)^ror(w[i-15],18)^(w[i-15]>>3);
            uint32_t s1=ror(w[i-2],17)^ror(w[i-2],19)^(w[i-2]>>10);
            w[i]=w[i-16]+s0+w[i-7]+s1;
        }
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for(int i=0;i<64;i++) {
            uint32_t S1=ror(e,6)^ror(e,11)^ror(e,25);
            uint32_t ch=(e&f)^(~e&g);
            uint32_t t1=hh+S1+ch+K256[i]+w[i];
            uint32_t S0=ror(a,2)^ror(a,13)^ror(a,22);
            uint32_t maj=(a&b)^(a&c)^(b&c);
            uint32_t t2=S0+maj;
            hh=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
        }
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;
    }
    free(buf);
    for(int i=0;i<8;i++) {
        out[i*4+0]=(uint8_t)(h[i]>>24); out[i*4+1]=(uint8_t)(h[i]>>16);
        out[i*4+2]=(uint8_t)(h[i]>>8);  out[i*4+3]=(uint8_t)(h[i]);
    }
}
 
static void sha256_hex(const uint8_t* data, size_t len, char out[65]) {
    uint8_t d[32]; sha256(data,len,d);
    for(int i=0;i<32;i++) sprintf(out+i*2,"%02x",d[i]); out[64]=0;
}
 
static uint8_t* zstd_comp(const uint8_t* src, size_t sz, size_t& out_sz, int lv) {
    size_t b=ZSTD_compressBound(sz); uint8_t* buf=(uint8_t*)malloc(b);
    if(!buf){out_sz=0;return nullptr;}
    out_sz=ZSTD_compress(buf,b,src,sz,lv);
    if (ZSTD_isError(out_sz)) { free(buf); out_sz=0; return nullptr; }
    return buf;
}
 
#pragma pack(push,1)
struct AetHeader {
    char     magic[8];
    uint32_t version;
    uint64_t orig_size;
    uint32_t block_size;
    uint32_t num_blocks;
    uint8_t  xxhash[8];  // XXH3_64bits
    uint64_t zlit_sz, zoff_sz, zlen_sz, zcmd_sz;
};
#pragma pack(pop)
 
static double now_sec() {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return t.tv_sec + t.tv_nsec*1e-9;
}
 
struct DecArgs {
    const uint8_t* lit; const uint8_t* off;
    const uint8_t* len; const uint8_t* cmd;
    const BlockOffsets* boffs;
    uint8_t* dst; size_t dst_size;
    size_t bid_start; size_t bid_end;
    size_t block_size;
};
 
static void* dec_worker(void* arg) {
    DecArgs* a = (DecArgs*)arg;
    for (size_t b = a->bid_start; b < a->bid_end; b++) {
        const BlockOffsets& bo = a->boffs[b];
        size_t bstart = b * a->block_size;
        size_t bsize  = a->dst_size > bstart ?
                        std::min<size_t>((size_t)a->block_size, a->dst_size - bstart) : 0;
        if (bsize > 0) {
#ifdef ANALYZE_DEPS
            analyze_depth(bsize,
                a->off + bo.off_off, bo.off_sz,
                a->len + bo.len_off, bo.len_sz,
                a->cmd + bo.cmd_off, bo.cmd_sz, b);
#endif
            g_bstart = bstart;
            decompress_streams(
                a->dst + bstart, bsize,
                a->lit + bo.lit_off, bo.lit_sz,
                a->off + bo.off_off, bo.off_sz,
                a->len + bo.len_off, bo.len_sz,
                a->cmd + bo.cmd_off, bo.cmd_sz);
        }
    }
    return nullptr;
}
 
static size_t compute_block_size(size_t src_size, int threads) {
    const char* bsenv=getenv("ACEAPEX_BS");
    if(bsenv){ size_t v=(size_t)strtoull(bsenv,0,10); if(v>=4096){ fprintf(stderr,"[ACEAPEX_BS override] block_size=%zu\n",v); return v; } }
    const size_t MIN_BS = 256*1024, MAX_BS = 1*1024*1024;
    if (threads < 1) threads = 1;
    size_t want_blocks = (size_t)threads * 4;
    size_t blocks_at_max = (src_size + MAX_BS - 1) / MAX_BS;
    if (blocks_at_max >= want_blocks) return MAX_BS; // 1MB enough -> best ratio, exact baseline
    size_t bs = (src_size + want_blocks - 1) / want_blocks;
    if (bs > MAX_BS) bs = MAX_BS;
    bs &= ~((size_t)65536 - 1); // round to 64KB
    if (bs < MIN_BS) bs = MIN_BS;
    return bs;
}

// Объявлены до encode_file: определения ниже (dna_worth ~1001, флаг ~1136).
static bool dna_worth(const uint8_t* s, size_t n);
extern int g_input_is_dna;

static bool encode_file(const uint8_t* src, size_t src_size, int threads, int level,
    std::vector<BlockOffsets>& boffs,
    uint8_t*& raw_lit, size_t& total_lit,
    uint8_t*& raw_off, size_t& total_off,
    uint8_t*& raw_len, size_t& total_len,
    uint8_t*& raw_cmd, size_t& total_cmd,
    size_t& num_blocks)
{
    g_block_size = compute_block_size(src_size, threads);
    // Подсказка для lit_chunk_size(): проверяем сам вход, не литералы.
    if(!getenv("LIT_CHUNK"))
        g_input_is_dna = dna_worth(src, src_size < (1u<<22) ? src_size : (1u<<22)) ? 1 : -1;
    num_blocks = (src_size + g_block_size - 1) / g_block_size;
    boffs.resize(num_blocks);
 
    // Adaptive hash size
    uint32_t hash_log = (src_size < 16*1024*1024) ? 13 :
                        (src_size < 128*1024*1024) ? 15 : 17;
    uint32_t hash_mask = (1u << hash_log) - 1;
    size_t ht_sz = (hash_mask+1);
    uint32_t chain_mask = (1u<<20)-1;
    ThreadHashTable** htabs=(ThreadHashTable**)calloc(threads,sizeof(ThreadHashTable*));
    if(!htabs){return false;}
    for(int i=0;i<threads;i++) {
        htabs[i]=(ThreadHashTable*)calloc(1,sizeof(ThreadHashTable));
        if(!htabs[i]){return false;}
        htabs[i]->pos  =(int64_t*) calloc(ht_sz,sizeof(int64_t));
        htabs[i]->epoch=(uint32_t*)calloc(ht_sz,sizeof(uint32_t));
        htabs[i]->chain=(int64_t*) malloc(((size_t)chain_mask+1)*sizeof(int64_t));
        if(!htabs[i]->pos||!htabs[i]->epoch||!htabs[i]->chain){return false;}
        memset(htabs[i]->chain,-1,((size_t)chain_mask+1)*sizeof(int64_t));
        htabs[i]->cur_epoch=0;
        htabs[i]->hash_mask=hash_mask;
        htabs[i]->chain_mask=chain_mask;
        htabs[i]->max_attempts=(level>=2)?32:4;
        htabs[i]->d8 = g_direct8 ? (Direct8*)calloc(1,sizeof(Direct8)) : nullptr;
    }
    BlockResult* results=(BlockResult*)calloc(num_blocks,sizeof(BlockResult));
    if(!results){return false;}
    PoolState pool;
    pool.src=src; pool.src_size=src_size;
    pool.num_blocks=num_blocks; pool.results=results;
    pool.next_block.store(0);
    WorkerArgs* wargs=(WorkerArgs*)calloc(threads,sizeof(WorkerArgs));
    pthread_t* pts=(pthread_t*)calloc(threads,sizeof(pthread_t));
    if(!wargs||!pts){free(results);return false;}
    for(int i=0;i<threads;i++) {
        wargs[i].thread_id=i; wargs[i].htab=htabs[i]; wargs[i].pool=&pool;
        pthread_create(&pts[i],nullptr,worker_func,&wargs[i]);
    }
    for(int i=0;i<threads;i++) pthread_join(pts[i],nullptr);

    total_lit=0; total_off=0; total_len=0; total_cmd=0;
    for(size_t b=0;b<num_blocks;b++) {
        if(results[b].overflow==99) { free(results); return false; }
        boffs[b].lit_off=total_lit; boffs[b].lit_sz=results[b].lit_size;
        boffs[b].off_off=total_off; boffs[b].off_sz=results[b].off_size;
        boffs[b].len_off=total_len; boffs[b].len_sz=results[b].len_size;
        boffs[b].cmd_off=total_cmd; boffs[b].cmd_sz=results[b].cmd_size;
        total_lit+=results[b].lit_size; total_off+=results[b].off_size;
        total_len+=results[b].len_size; total_cmd+=results[b].cmd_size;
    }

    raw_lit=(uint8_t*)malloc(total_lit);
    raw_off=(uint8_t*)malloc(total_off);
    raw_len=(uint8_t*)malloc(total_len);
    raw_cmd=(uint8_t*)malloc(total_cmd);
    if(!raw_lit||!raw_off||!raw_len||!raw_cmd){free(results);return false;}

    size_t li=0,oi=0,ni=0,ci=0;
    for(size_t b=0;b<num_blocks;b++) {
        memcpy(raw_lit+li,results[b].lit_buf,results[b].lit_size); li+=results[b].lit_size;
        memcpy(raw_off+oi,results[b].off_buf,results[b].off_size); oi+=results[b].off_size;
        memcpy(raw_len+ni,results[b].len_buf,results[b].len_size); ni+=results[b].len_size;
        memcpy(raw_cmd+ci,results[b].cmd_buf,results[b].cmd_size); ci+=results[b].cmd_size;
        free(results[b].lit_buf); free(results[b].off_buf);
        free(results[b].len_buf); free(results[b].cmd_buf);
    }
    for(int i=0;i<threads;i++) {
        free(htabs[i]->pos); free(htabs[i]->epoch); free(htabs[i]->chain);
        free(htabs[i]);
    }
    free(htabs); free(results); free(wargs); free(pts);
    return true;
}
 
static void parallel_decode(
    const uint8_t* lit, const uint8_t* off,
    const uint8_t* len, const uint8_t* cmd,
    const BlockOffsets* boffs, size_t num_blocks,
    uint8_t* dst, size_t dst_size, size_t block_size,
    int nthreads = 0)
{
    if (nthreads <= 0) nthreads = 8;
    size_t nt = std::min<size_t>((size_t)nthreads, num_blocks);
    std::vector<DecArgs> dargs(nt);
    size_t blocks_per_thread = (num_blocks + nt - 1) / nt;
    for(size_t t=0;t<nt;t++) {
        size_t bstart = t * blocks_per_thread;
        size_t bend   = std::min<size_t>(bstart + blocks_per_thread, num_blocks);
        dargs[t]={lit,off,len,cmd,boffs,dst,dst_size,bstart,bend,block_size};
    }
    std::vector<pthread_t> dpts(nt);
    for(size_t t=0;t<nt;t++) pthread_create(&dpts[t],nullptr,dec_worker,&dargs[t]);
    for(size_t t=0;t<nt;t++) pthread_join(dpts[t],nullptr);
}
 
// Helper: chunked FSE decompress a stream
// Format: [8:orig_sz][nc*8:csizes][chunks...]
// Размер FSE-чанка был зашит на 512 KB. Три потока off/len/cmd требуют по чанку
// при ЛЮБОМ размере чанка литералов — это пол латентности региона, около 1 MB.
static size_t fse_chunk_size(){
    const char* e=getenv("FSE_CHUNK");
    if(e){ size_t v=strtoull(e,0,10); if(v>=4096) return v; }
    return 512*1024;
}

static void fse_chunked_decomp(const uint8_t* src, size_t orig_sz, uint8_t* dst) {
    const size_t CHUNK=fse_chunk_size();
    const uint64_t* cs = (const uint64_t*)(src + 8);
    size_t nc = (orig_sz + CHUNK - 1) / CHUNK;
    // Build chunk offsets for parallel decompress
    std::vector<size_t> src_off(nc), dst_off(nc), raw_sz(nc);
    size_t p_off = (size_t)(src + 8 + nc * 8 - src);
    for (size_t i = 0; i < nc; i++) {
        dst_off[i] = i * CHUNK;
        raw_sz[i] = std::min<size_t>(CHUNK, orig_sz - dst_off[i]);
        src_off[i] = p_off;
        p_off += (cs[i] >> 63) ? raw_sz[i] : (size_t)(cs[i] & ~(uint64_t(1)<<63));
    }
    // Parallel decompress chunks
    #pragma omp parallel for schedule(dynamic,1)
    for (size_t i = 0; i < nc; i++) {
        const uint8_t* p = src + src_off[i];
        if (cs[i] >> 63) memcpy(dst + dst_off[i], p, raw_sz[i]);
        else ZSTD_decompress(dst + dst_off[i], raw_sz[i], p, cs[i] & ~(uint64_t(1)<<63));
    }
}
 
// Parallel entropy encode — 4 streams simultaneously
static int lit_level(){
    const char* e=getenv("LIT_LEVEL");
    if(e){ int v=atoi(e); if(v>=-5 && v<=19 && v!=0) return v; }
    return 3;
}

// --- ДОМЕННЫЙ ТРАНСФОРМ ЛИТЕРАЛОВ (FASTA) ----------------------------------
// Литералы генома это ACGT в двух регистрах плюс редкие N и переводы строк.
// Два бита на основание + битовая маска регистра + разности позиций исключений
// дают на chr1 -18% к литералам. Включается по доле не-ACGT в чанке (порог 10%);
// на тексте, архивах и ридах доля 74-92%, режим не включается.
// Формат чанка: [4]n_exc [4]sz_seq [4]sz_case [4]sz_gap [4]sz_val, затем данные.
static bool dna_worth(const uint8_t* s, size_t n){
    if(n < 4096) return false;
    size_t step = n>65536 ? n/65536 : 1, bad=0, cnt=0;
    for(size_t i=0;i<n;i+=step){ uint8_t u=s[i]&0xDF;
        if(u!='A'&&u!='C'&&u!='G'&&u!='T') bad++;
        cnt++; }
    return cnt && (double)bad/(double)cnt < 0.10;
}

static uint8_t* dna_compress(const uint8_t* s, size_t n, size_t& out_sz){
    size_t np=(n+3)/4, nc=(n+7)/8, nexc=0;
    for(size_t i=0;i<n;i++){ uint8_t u=s[i]&0xDF;
        if(u!='A'&&u!='C'&&u!='G'&&u!='T') nexc++; }
    uint8_t* seq=(uint8_t*)calloc(np?np:1,1);
    uint8_t* cse=(uint8_t*)calloc(nc?nc:1,1);
    uint32_t* gap=(uint32_t*)malloc((nexc?nexc:1)*4);
    uint8_t*  val=(uint8_t*)malloc(nexc?nexc:1);
    if(!seq||!cse||!gap||!val){free(seq);free(cse);free(gap);free(val);out_sz=0;return nullptr;}
    size_t e=0, prev=0;
    for(size_t i=0;i<n;i++){
        uint8_t b=s[i], u=b&0xDF, code=0;
        if(u=='A') code=0; else if(u=='C') code=1;
        else if(u=='G') code=2; else if(u=='T') code=3;
        else { gap[e]=(uint32_t)(i-prev); prev=i; val[e]=b; e++; }
        seq[i>>2] |= (uint8_t)(code << (6-2*(i&3)));
        if(b>=0x61 && b<=0x7a) cse[i>>3] |= (uint8_t)(0x80>>(i&7));
    }
    size_t c1=ZSTD_compressBound(np)+8, c2=ZSTD_compressBound(nc)+8;
    size_t c3=ZSTD_compressBound(nexc*4)+8, c4=ZSTD_compressBound(nexc)+8;
    uint8_t* buf=(uint8_t*)malloc(20+c1+c2+c3+c4);
    if(!buf){free(seq);free(cse);free(gap);free(val);out_sz=0;return nullptr;}
    uint8_t* p=buf+20;
    int L=lit_level();
    size_t s1=ZSTD_compress(p,c1,seq,np,L); p+=s1;
    size_t s2=ZSTD_compress(p,c2,cse,nc,L); p+=s2;
    size_t s3=nexc?ZSTD_compress(p,c3,gap,nexc*4,L):0; p+=s3;
    size_t s4=nexc?ZSTD_compress(p,c4,val,nexc,L):0; p+=s4;
    uint32_t h[5]={(uint32_t)nexc,(uint32_t)s1,(uint32_t)s2,(uint32_t)s3,(uint32_t)s4};
    memcpy(buf,h,20);
    free(seq);free(cse);free(gap);free(val);
    out_sz=20+s1+s2+s3+s4;
    return buf;
}

static void dna_decompress(const uint8_t* src, uint8_t* dst, size_t n){
    uint32_t h[5]; memcpy(h,src,20);
    size_t nexc=h[0], np=(n+3)/4, nc=(n+7)/8;
    const uint8_t* p=src+20;
    uint8_t* seq=(uint8_t*)malloc(np?np:1);
    uint8_t* cse=(uint8_t*)malloc(nc?nc:1);
    uint32_t* gap=(uint32_t*)malloc((nexc?nexc:1)*4);
    uint8_t*  val=(uint8_t*)malloc(nexc?nexc:1);
    ZSTD_decompress(seq,np,p,h[1]); p+=h[1];
    ZSTD_decompress(cse,nc,p,h[2]); p+=h[2];
    if(h[3]) ZSTD_decompress(gap,nexc*4,p,h[3]); p+=h[3];
    if(h[4]) ZSTD_decompress(val,nexc,p,h[4]);
    // Таблица на 256 входов: один упакованный байт разворачивается в четыре
    // основания одним 32-битным store вместо четырёх сдвигов с условием.
    static uint32_t T4[256]; static bool T4_ready=false;
    if(!T4_ready){
        static const uint8_t B[4]={'A','C','G','T'};
        for(int v=0;v<256;v++){
            uint8_t q[4]={B[(v>>6)&3],B[(v>>4)&3],B[(v>>2)&3],B[v&3]};
            memcpy(&T4[v],q,4);
        }
        T4_ready=true;
    }
    size_t full=n>>2;
    for(size_t k=0;k<full;k++) memcpy(dst+4*k,&T4[seq[k]],4);
    for(size_t i=full<<2;i<n;i++){
        static const uint8_t B2[4]={'A','C','G','T'};
        dst[i]=B2[(seq[i>>2] >> (6-2*(i&3))) & 3];
    }
    // Регистр: байты, где бит маски выставлен, получают 0x20. Байт маски покрывает
    // восемь позиций, поэтому нулевой байт пропускается целиком.
    size_t nb8=(n+7)>>3;
    for(size_t k=0;k<nb8;k++){
        uint8_t m=cse[k];
        if(!m) continue;
        size_t base=k<<3, lim=(base+8<=n)?8:(n-base);
        for(size_t j=0;j<lim;j++) if(m & (0x80>>j)) dst[base+j]|=0x20;
    }
    size_t pos=0;
    for(size_t k=0;k<nexc;k++){ pos+=gap[k]; if(pos<n) dst[pos]=val[k]; }
    free(seq);free(cse);free(gap);free(val);
}

// Старая схема: NW равных долей. Остаётся для входов, где чанкование неприменимо
// (очень маленький файл, или чанков было бы больше 65535).
static uint8_t* lit_compress_legacy(const uint8_t* src, size_t sz, size_t& out_sz) {
    const int NW=4; size_t csz=(sz+NW-1)/NW;
    struct ZW{const uint8_t*in;size_t isz;uint8_t*out;size_t osz;size_t cap;};
    ZW zws[NW];
    for(int t=0;t<NW;t++){
        size_t off=(size_t)t*csz; if(off>sz) off=sz;
        size_t isz=(t<NW-1)?((off+csz<=sz)?csz:(sz-off)):(sz-off);
        zws[t]={src+off,isz,nullptr,0,ZSTD_compressBound(isz)+8};
        zws[t].out=(uint8_t*)malloc(zws[t].cap);
        if(!zws[t].out){out_sz=0;return nullptr;}}
    auto zfn=[](void*a)->void*{ZW*z=(ZW*)a;
        ZSTD_CCtx*ctx=ZSTD_createCCtx();
        if(!ctx){z->osz=0; return nullptr;}
        ZSTD_CCtx_setParameter(ctx,ZSTD_c_compressionLevel,lit_level());
        z->osz=ZSTD_compress2(ctx,z->out,z->cap,z->in,z->isz);
        ZSTD_freeCCtx(ctx); return nullptr;};
    pthread_t pts[NW];
    for(int t=0;t<NW;t++) pthread_create(&pts[t],nullptr,zfn,&zws[t]);
    for(int t=0;t<NW;t++) pthread_join(pts[t],nullptr);
    size_t hdrsz=8+NW*8,totalsz=hdrsz;
    for(int t=0;t<NW;t++) totalsz+=zws[t].osz;
    uint8_t* res=(uint8_t*)malloc(totalsz);
    if(!res){out_sz=0;return nullptr;}
    *(uint64_t*)res=sz|(uint64_t(1)<<62);
    uint64_t* zsz=(uint64_t*)(res+8); uint8_t* p=res+hdrsz;
    for(int t=0;t<NW;t++){zsz[t]=zws[t].osz;memcpy(p,zws[t].out,zws[t].osz);p+=zws[t].osz;free(zws[t].out);}
    out_sz=totalsz; return res;
}

// LIT_CHUNK: литералы режутся на куски фиксированного размера, а не на NW равных
// долей. Это делает распаковку ЧАСТИЧНОЙ: для региона нужен один кусок, а не четверть
// файла. Цена на chr1 = +0.051% общего размера при 1 MB (измерено).
// Бит 61 в заголовке потока = новая схема; число кусков идёт следующим полем.
// Схема литералов по умолчанию — прежняя (NW равных долей), чтобы опубликованные
// числа воспроизводились без оговорок. Чанки включаются явно: LIT_CHUNK=1048576.
// Они дают частичную распаковку, без которой region-декод пришлось бы делать на
// четверти файла; цена и выигрыш зависят от данных, поэтому это опция, не дефолт.
// Рабочих в пуле — по числу доступных ядер, а не фикс. 8: на 16-поточной машине
// половина простаивала. Переопределяется LIT_LANES для замеров.
static int lit_lanes(){
    const char* e=getenv("LIT_LANES");
    if(e){ int v=atoi(e); if(v>0) return v; }
    long n=sysconf(_SC_NPROCESSORS_ONLN);
    return n>0 ? (int)n : 8;
}

// Выставляется один раз в encode_file по выборке из входа.
// 0 = не проверяли, 1 = вход проходит dna_worth, -1 = нет.
int g_input_is_dna = 0;

static size_t lit_chunk_size(){
    const char* e=getenv("LIT_CHUNK");
    if(e){ size_t v=strtoull(e,0,10); return v<(1u<<16) ? 0 : v; }
    // Дефолт по данным (11.09): чанкование открывает DNA-трансформ, который на
    // чистой ДНК окупает цену таблиц zstd на кусок. chr1 3.18065 -> 3.72329
    // (+17.1%); enwik8 и silesia не трогаем — там трансформ не срабатывает,
    // а чанкование стоило бы 6-8%. FASTQ тоже нет: половина байтов — качества.
    // Явный LIT_CHUNK и --profile перекрывают эту ветку.
    return g_input_is_dna == 1 ? 65536 : 0;
}
#define LIT_CHUNK lit_chunk_size()

static uint8_t* lit_compress(const uint8_t* src, size_t sz, size_t& out_sz) {
    const size_t CH = LIT_CHUNK;
    if (CH == 0) return lit_compress_legacy(src, sz, out_sz);
    const int NW = (int)((sz + CH - 1) / CH);
    if (NW < 1 || NW > 65535) return lit_compress_legacy(src, sz, out_sz);
    size_t csz = CH;
    struct ZW{const uint8_t*in;size_t isz;uint8_t*out;size_t osz;size_t cap;};
    std::vector<ZW> zws(NW);
    for(int t=0;t<NW;t++){
        // Unsigned-underflow guard: the last worker's offset 3*ceil(sz/4) exceeds
        // sz for sz in {1,2,5,...}, so sz-off wrapped to ~2^64 -> ZSTD_compressBound
        // huge -> malloc failed -> the literal stream was SILENTLY DROPPED and decode
        // produced garbage. (lzbench issue #2, reported by inikep.)
        size_t off=(size_t)t*csz; if(off>sz) off=sz;
        size_t isz=(t<NW-1)?((off+csz<=sz)?csz:(sz-off)):(sz-off);
        zws[t]={src+off,isz,nullptr,0,ZSTD_compressBound(isz)+8};
        zws[t].out=(uint8_t*)malloc(zws[t].cap);
        if(!zws[t].out){out_sz=0;return nullptr;}}
    auto zfn=[](void*a)->void*{ZW*z=(ZW*)a;
        ZSTD_CCtx*ctx=ZSTD_createCCtx();
        if(!ctx){z->osz=0; return nullptr;}
        ZSTD_CCtx_setParameter(ctx,ZSTD_c_compressionLevel,lit_level());
        z->osz=ZSTD_compress2(ctx,z->out,z->cap,z->in,z->isz);
        ZSTD_freeCCtx(ctx); return nullptr;};
    struct CPool{ ZW* w; int n; std::atomic<int> next; };
    CPool cpool{zws.data(),NW,{0}};
    auto zfn_pool=[](void*a)->void*{
        CPool* p=(CPool*)a;
        ZSTD_CCtx* ctx=ZSTD_createCCtx();
        if(!ctx) return nullptr;
        ZSTD_CCtx_setParameter(ctx,ZSTD_c_compressionLevel,lit_level());
        for(;;){ int i=p->next.fetch_add(1); if(i>=p->n) break;
            ZW& z=p->w[i];
            // Байт режима перед данными: 0 = обычный zstd, 1 = DNA-трансформ.
            // Считаем оба и берём меньший, поэтому режим не может ухудшить размер.
            size_t zs=ZSTD_compress2(ctx,z.out+1,z.cap-1,z.in,z.isz);
            uint8_t* db=nullptr; size_t dsz=0;
            if(dna_worth(z.in,z.isz)) db=dna_compress(z.in,z.isz,dsz);
            if(db && dsz && dsz<zs){
                z.out[0]=1; memcpy(z.out+1,db,dsz); z.osz=dsz+1;
            } else {
                z.out[0]=0; z.osz=zs+1;
            }
            free(db);
        }
        ZSTD_freeCCtx(ctx); return nullptr;};
    const int LANES=std::min(lit_lanes(),NW);
    std::vector<pthread_t> pts(LANES);
    for(int t=0;t<LANES;t++) pthread_create(&pts[t],nullptr,zfn_pool,&cpool);
    for(int t=0;t<LANES;t++) pthread_join(pts[t],nullptr);
    size_t hdrsz=8+8+(size_t)NW*8,totalsz=hdrsz;   // h + nchunks + таблица
    for(int t=0;t<NW;t++) totalsz+=zws[t].osz;
    uint8_t* res=(uint8_t*)malloc(totalsz);
    if(!res){out_sz=0;return nullptr;}
    *(uint64_t*)res=sz|(uint64_t(1)<<62)|(uint64_t(1)<<61)|(uint64_t(1)<<60);
    *(uint64_t*)(res+8)=(uint64_t)CH;   // размер чанка, а не их число
    uint64_t* zsz=(uint64_t*)(res+16); uint8_t* p=res+hdrsz;
    for(int t=0;t<NW;t++){zsz[t]=zws[t].osz;memcpy(p,zws[t].out,zws[t].osz);p+=zws[t].osz;free(zws[t].out);}
    out_sz=totalsz; return res;
}
static uint8_t* lit_decompress(const uint8_t* src, size_t src_sz, size_t& orig_sz) {
    // An empty or truncated literal stream must not be read as an 8-byte header.
    // Tiny inputs give zlit_sz==0; the out-of-bounds read corrupted heap metadata
    // and surfaced as a double-free thousands of calls later. (lzbench issue #2.)
    if (!src || src_sz < 8) { orig_sz = 0; return (uint8_t*)malloc(1); }
    uint64_t h; memcpy(&h, src, 8);   // alignment-safe (no AX_read64 in this tree)
    const int NW_LEGACY=4;
    const bool chunked = (h & (uint64_t(1)<<61)) != 0;
    const bool tagged = (h & (uint64_t(1)<<60)) != 0;
    orig_sz = h & ~((uint64_t(1)<<62)|(uint64_t(1)<<61)|(uint64_t(1)<<60));
    uint8_t* out=(uint8_t*)malloc(orig_sz?orig_sz:1);
    if(!out) return nullptr;
    if(!(h & (uint64_t(1)<<62))){fse_chunked_decomp(src,orig_sz,out);return out;}
    // Размер чанка читается ИЗ ФАЙЛА: архив не должен зависеть от окружения читателя.
    size_t csz = chunked ? (size_t)(*(const uint64_t*)(src+8)) : (orig_sz+NW_LEGACY-1)/NW_LEGACY;
    const int NW = chunked ? (int)((orig_sz + csz - 1)/csz) : NW_LEGACY;
    const uint64_t* zsz=(const uint64_t*)(src+(chunked?16:8));
    const uint8_t* p0=src+(chunked?16:8)+(size_t)NW*8;
    struct DW{uint8_t*out;size_t raw;const uint8_t*in;size_t isz;bool tg;};
    std::vector<DW> dws(NW); const uint8_t* p=p0;
    for(int t=0;t<NW;t++){
        // Same underflow guard as in lit_compress (decoder side).
        size_t off=(size_t)t*csz; if(off>orig_sz) off=orig_sz;
        // Правило совпадает с кодером: кусок равен csz, кроме последнего.
        // Прежняя формула с (t<NW-1) была под legacy и на чанках расходилась.
        size_t raw=(off+csz<=orig_sz)?csz:(orig_sz-off);
        dws[t]={out+off,raw,p,(size_t)zsz[t],tagged}; p+=(size_t)zsz[t];}
    // Пул фиксированного размера: восемь рабочих разбирают очередь чанков через
    // атомарный счётчик. Создание потока на каждый чанк давало x3 к времени фазы.
    struct Pool{ DW* w; int n; std::atomic<int> next; };
    Pool pool{dws.data(),NW,{0}};
    auto dfn=[](void*a)->void*{
        Pool* p=(Pool*)a;
        for(;;){ int i=p->next.fetch_add(1); if(i>=p->n) break;
            DW& d=p->w[i];
            if(!d.isz) continue;
            if(!d.tg){ ZSTD_decompress(d.out,d.raw,d.in,d.isz); continue; }
            if(d.in[0]==1) dna_decompress(d.in+1,d.out,d.raw);
            else           ZSTD_decompress(d.out,d.raw,d.in+1,d.isz-1);
        }
        return nullptr;};
    const int LANES=std::min(lit_lanes(),NW);
    std::vector<pthread_t> pts(LANES);
    for(int t=0;t<LANES;t++) pthread_create(&pts[t],nullptr,dfn,&pool);
    for(int t=0;t<LANES;t++) pthread_join(pts[t],nullptr);
    return out;
}
static void entropy_encode(
    const uint8_t* raw_lit, size_t total_lit,
    const uint8_t* raw_off, size_t total_off,
    const uint8_t* raw_len, size_t total_len,
    const uint8_t* raw_cmd, size_t total_cmd,
    uint8_t*& zlit, size_t& zlit_sz,
    uint8_t*& zoff, size_t& zoff_sz,
    uint8_t*& zlen, size_t& zlen_sz,
    uint8_t*& zcmd, size_t& zcmd_sz)
{
    struct EA{const uint8_t*in;size_t isz;uint8_t**out;size_t*osz;};
    auto ew=[](void*a)->void*{
        EA*e=(EA*)a;
        const size_t CHUNK=fse_chunk_size();
        size_t nc=(e->isz+CHUNK-1)/CHUNK;
        size_t hdrsz=8+nc*8;
        size_t cap=hdrsz+e->isz+nc*64;
        *e->out=(uint8_t*)malloc(cap);
        if(!*e->out) return nullptr;
        *(uint64_t*)*e->out=e->isz;
        uint64_t* csizes=(uint64_t*)(*e->out+8);
        uint8_t* p=*e->out+hdrsz;
        size_t total=hdrsz;
        for(size_t i=0;i<nc;i++){
            size_t off=i*CHUNK;
            size_t isz=std::min<size_t>(CHUNK,e->isz-off);
            size_t b=ZSTD_compressBound(isz)+4;
            size_t r=ZSTD_compress(p,b,e->in+off,isz,1);
            if(!r||ZSTD_isError(r)){memcpy(p,e->in+off,isz);csizes[i]=isz|(uint64_t(1)<<63);total+=isz;p+=isz;}
            else{csizes[i]=r;total+=r;p+=r;}
        }
        *e->osz=total;
        return nullptr;
    };
    EA ea[3]={
        {raw_off,total_off,&zoff,&zoff_sz},
        {raw_len,total_len,&zlen,&zlen_sz},
        {raw_cmd,total_cmd,&zcmd,&zcmd_sz}
    };
    pthread_t epts[3];
    for(int i=0;i<3;i++) pthread_create(&epts[i],nullptr,ew,&ea[i]);
    for(int i=0;i<3;i++) pthread_join(epts[i],nullptr);
}
 
static void load_forced(){  // PHASE2B
    { const char* nr=getenv("NO_REP"); g_norep = (nr && *nr=='1') ? 1 : 0; }
    { const char* mm=getenv("MIN_MATCH"); g_mm = mm ? atoi(mm) : 0; }
    { const char* d8=getenv("DIRECT8"); g_direct8 = (d8 && *d8=='1') ? 1 : 0; init_nt(); }
    const char* fp=getenv("FORCED_BIN"); if(!fp) return;
    FILE* ff=fopen(fp,"rb"); if(!ff) return;
    fseek(ff,0,SEEK_END); long fs=ftell(ff); fseek(ff,0,SEEK_SET);
    g_forced=(uint8_t*)malloc(fs); g_forced_n=fread(g_forced,1,fs,ff); fclose(ff);
    fprintf(stderr,"PHASE2B: forced %zu positions\n",g_forced_n);
}
static int do_compress(const char* in_path, const char* out_path, int threads, int level=2) {
    load_forced();
    double t_fread=now_sec();
#ifdef _WIN32
    FILE* fin_w=fopen(in_path,"rb");
    if(!fin_w){fprintf(stderr,"Cannot open: %s\n",in_path);return 1;}
    fseek(fin_w,0,SEEK_END); size_t src_size=(size_t)ftell(fin_w); fseek(fin_w,0,SEEK_SET);
    uint8_t* src=(uint8_t*)malloc(src_size);
    if(!src){fprintf(stderr,"malloc failed\n");fclose(fin_w);return 1;}
    fread(src,1,src_size,fin_w); fclose(fin_w);
    bool src_is_mmap=false;
#else
    int fin_fd=open(in_path,O_RDONLY);
    if (fin_fd<0) { fprintf(stderr,"Cannot open: %s\n",in_path); return 1; }
    struct stat fin_st; fstat(fin_fd,&fin_st);
    size_t src_size=(size_t)fin_st.st_size;
    uint8_t* src=(uint8_t*)mmap(nullptr,src_size,PROT_READ,MAP_SHARED|MAP_POPULATE,fin_fd,0);
    close(fin_fd);
    if (src==MAP_FAILED) { fprintf(stderr,"mmap failed\n"); return 1; }
    bool src_is_mmap=true;
#endif
    t_fread=now_sec()-t_fread;
    // SHA256 в фоне снят 09.09: результат нигде не читался (в заголовок идёт XXH3),
    // а на одном ядре поток съедал 12% времени кодирования (perf record).
    // Апрельский фикс заменил хэш заголовка, но поток остался.

    fprintf(stderr,"[*] Compress: %s (%.2f MB) threads=%d\n",in_path,src_size/1e6,threads);
    double t_total_c=now_sec();
 
    std::vector<BlockOffsets> boffs;
    uint8_t *raw_lit,*raw_off,*raw_len,*raw_cmd;
    size_t total_lit,total_off,total_len,total_cmd,num_blocks;
    double t0=now_sec();
    encode_file(src,src_size,threads,level,boffs,
                raw_lit,total_lit,raw_off,total_off,
                raw_len,total_len,raw_cmd,total_cmd,
                num_blocks);
    double enc_time=now_sec()-t0;
 
    size_t zlit_sz,zoff_sz,zlen_sz,zcmd_sz;
    uint8_t *zlit,*zoff,*zlen,*zcmd;
    double t_lz=enc_time;
    double t1=now_sec();
    zlit=lit_compress(raw_lit,total_lit,zlit_sz);
    double t_lit=now_sec()-t1;
    double t2=now_sec();
    entropy_encode(raw_lit,total_lit,raw_off,total_off,raw_len,total_len,raw_cmd,total_cmd,
                   zlit,zlit_sz,zoff,zoff_sz,zlen,zlen_sz,zcmd,zcmd_sz);
    double t_fse=now_sec()-t2;
 
    size_t total_z=zlit_sz+zoff_sz+zlen_sz+zcmd_sz;
 
    AetHeader hdr;
    memcpy(hdr.magic,"ACEPX2\0\0",8);
    hdr.version=2; hdr.orig_size=(uint64_t)src_size;
    hdr.block_size=(uint32_t)g_block_size; hdr.num_blocks=(uint32_t)num_blocks;
    double t_sha256=now_sec();
    uint64_t hv=OUR_CHECKSUM(src,src_size);
    memcpy(hdr.xxhash,&hv,8);
    t_sha256=now_sec()-t_sha256;
    char sha_hex[17];
    uint64_t hv2; memcpy(&hv2,hdr.xxhash,8);
    sprintf(sha_hex,"%016llx",(unsigned long long)hv2);
    hdr.zlit_sz=zlit_sz; hdr.zoff_sz=zoff_sz;
    hdr.zlen_sz=zlen_sz; hdr.zcmd_sz=zcmd_sz;
 
    FILE* fout=fopen(out_path,"wb");
    fwrite(&hdr,sizeof(hdr),1,fout);
    fwrite(boffs.data(),sizeof(BlockOffsets),num_blocks,fout);
    fwrite(zlit,1,zlit_sz,fout); fwrite(zoff,1,zoff_sz,fout);
    fwrite(zlen,1,zlen_sz,fout); fwrite(zcmd,1,zcmd_sz,fout);
    fclose(fout);
    fprintf(stderr,"  Original:   %14zu bytes\n",src_size);
    fprintf(stderr,"  Compressed: %14zu bytes\n",total_z);
    fprintf(stderr,"  Ratio:  %.5fx\n",(double)src_size/total_z);
    double t3=now_sec();
    (void)(t3-t_total_c-t_lz-t_lit-t_fse);
    double real_enc=now_sec()-t_total_c;
    fprintf(stderr,"  Phase LZ77:    %.3fs\n",t_lz);
    fprintf(stderr,"  Phase lit/zstd:%.3fs\n",t_lit);
    fprintf(stderr,"  Phase FSE:     %.3fs\n",t_fse);
    fprintf(stderr,"  Phase fread:   %.3fs\n",t_fread);
    fprintf(stderr,"  Phase sha256:  %.3fs\n",t_sha256);
    fprintf(stderr,"  Phase other:   %.3fs\n",real_enc-t_lz-t_lit-t_fse-t_fread-t_sha256);
    fprintf(stderr,"  Encode: %.2f MB/s  (%.3fs)\n",src_size/real_enc/1e6,real_enc);
    fprintf(stderr,"  XXH3:   %s\n",sha_hex);
 
    #ifdef _WIN32
    free((void*)src);
#else
    if(src_is_mmap) munmap((void*)src,src_size); else free((void*)src);
#endif
    free(raw_lit); free(raw_off); free(raw_len); free(raw_cmd);
    free(zlit); free(zoff); free(zlen); free(zcmd);
    return 0;
}
 
static double tl_lit=0, tl_fse=0;
static int do_decompress(const char* in_path, const char* out_path, int threads=8) {
    double t_wall=now_sec();
    FILE* fin=fopen(in_path,"rb");
    if (!fin) { fprintf(stderr,"Cannot open: %s\n",in_path); return 1; }
    AetHeader hdr;
    fread(&hdr,sizeof(hdr),1,fin);
    if (memcmp(hdr.magic,"ACEPX2\0\0",8)!=0) { fprintf(stderr,"Bad magic\n"); return 1; }
    // Версия писалась с первого дня и не проверялась ни разу. Архив, созданный
    // более новым кодером, старый декодер читал как валидный и выдавал мусор:
    // при интерпретации новых битов orig_size получалось 3.4 эксабайта.
    if (hdr.version != 2) {
        fprintf(stderr,"Unsupported format version %u (this build reads 2)\n", hdr.version);
        return 1;
    }
    fprintf(stderr,"[*] Decompress: %s -> %s\n",in_path,out_path);
 
    uint32_t nb=hdr.num_blocks;
    std::vector<BlockOffsets> boffs(nb);
    fread(boffs.data(),sizeof(BlockOffsets),nb,fin);
 
    uint8_t* zlit=(uint8_t*)malloc(hdr.zlit_sz);
    uint8_t* zoff=(uint8_t*)malloc(hdr.zoff_sz);
    uint8_t* zlen=(uint8_t*)malloc(hdr.zlen_sz);
    uint8_t* zcmd=(uint8_t*)malloc(hdr.zcmd_sz);
    if(!zlit||!zoff||!zlen||!zcmd){free(zlit);free(zoff);free(zlen);free(zcmd);fclose(fin);return 1;}
    fread(zlit,1,hdr.zlit_sz,fin); fread(zoff,1,hdr.zoff_sz,fin);
    fread(zlen,1,hdr.zlen_sz,fin); fread(zcmd,1,hdr.zcmd_sz,fin);
    fclose(fin);
    fprintf(stderr,"  Phase read: %.3fs\n",now_sec()-t_wall);
 
    size_t off_sz=*(uint64_t*)zoff;
    size_t len_sz=*(uint64_t*)zlen;
    size_t cmd_sz=*(uint64_t*)zcmd;
 
    double dec_time=now_sec();
    double t_lit=now_sec();
    // Run lit + fse decompress in parallel
    size_t lit_sz=0; uint8_t* lit=nullptr;
    uint8_t* off=(uint8_t*)malloc(off_sz);
    uint8_t* len=(uint8_t*)malloc(len_sz);
    uint8_t* cmd=(uint8_t*)malloc(cmd_sz);
    if(!off||!len||!cmd){free(off);free(len);free(cmd);free(zlit);free(zoff);free(zlen);free(zcmd);return 1;}
    struct LitArg{const uint8_t*s;size_t sz;uint8_t**out;size_t*osz;};
    LitArg larg={zlit,(size_t)hdr.zlit_sz,&lit,&lit_sz};
    auto litfn=[](void*a)->void*{LitArg*l=(LitArg*)a;
        double t0=now_sec();
        *l->out=lit_decompress(l->s,l->sz,*l->osz);
        tl_lit=now_sec()-t0; return nullptr;};
    struct FD{const uint8_t*s;size_t sz;uint8_t*d;};
    FD fds[3]={{zoff,off_sz,off},{zlen,len_sz,len},{zcmd,cmd_sz,cmd}};
    auto fdfn=[](void*a)->void*{FD*f=(FD*)a;
        double t0=now_sec();
        size_t orig=*(const uint64_t*)f->s&~(uint64_t(1)<<63);
        fse_chunked_decomp(f->s,orig,f->d);
        double d=now_sec()-t0; if(d>tl_fse) tl_fse=d; return nullptr;};
    pthread_t fpts[4];
    pthread_create(&fpts[0],nullptr,litfn,&larg);
    for(int i=0;i<3;i++) pthread_create(&fpts[i+1],nullptr,fdfn,&fds[i]);
    for(int i=0;i<4;i++) pthread_join(fpts[i],nullptr);
    if(!lit){free(off);free(len);free(cmd);free(zlit);free(zoff);free(zlen);free(zcmd);return 1;}
    double t_fse=now_sec()-t_lit;
    fprintf(stderr,"  [t] lit_only %.3fs  fse_only %.3fs\n", tl_lit, tl_fse);
    t_lit=t_fse;
    free(zlit); free(zoff); free(zlen); free(zcmd);
    uint8_t* dst=(uint8_t*)malloc(hdr.orig_size + DST_PAD);
    if(!dst){free(lit);free(off);free(len);free(cmd);return 1;}
    // Записываем lit[] буфер для честного GPU decode
    if(getenv("ACEAPEX_DUMP"))
    { FILE*fl=fopen("lits.bin","wb");
      fwrite(&lit_sz,8,1,fl);
      fwrite(lit,1,lit_sz,fl);
      fclose(fl); }
    // step0: dump raw decoded streams + BlockOffsets for full_gpu_decode.cu
    if(getenv("ACEAPEX_DUMP"))
    { FILE* fs=fopen("streams.bin","wb");
      fwrite(&hdr,sizeof(hdr),1,fs);
      fwrite(boffs.data(),sizeof(BlockOffsets),nb,fs);
      fwrite(lit,1,lit_sz,fs); fwrite(off,1,off_sz,fs);
      fwrite(len,1,len_sz,fs); fwrite(cmd,1,cmd_sz,fs);
      fclose(fs);
      fprintf(stderr,"Dumped streams.bin (%u blocks)\n",nb); }
    g_record=false; g_tokens.clear(); double t_lz=now_sec(); parallel_decode(lit,off,len,cmd,boffs.data(),nb,dst,hdr.orig_size,hdr.block_size,threads); t_lz=now_sec()-t_lz;
    dec_time=now_sec()-dec_time;
    fprintf(stderr,"  Phase lit:  %.3fs\n  Phase fse:  %.3fs\n  Phase lz77: %.3fs\n",t_lit,t_fse,t_lz);
 
    double t_hash=now_sec();
    uint64_t dv=OUR_CHECKSUM(dst,hdr.orig_size);
    fprintf(stderr,"  Phase hash: %.3fs\n",now_sec()-t_hash);
    uint64_t hv3; memcpy(&hv3,hdr.xxhash,8);
    bool ok=(dv==hv3);
    FILE* fout=fopen(out_path,"wb");
    if (fout) { fwrite(dst,1,hdr.orig_size,fout); fclose(fout); }
    double wall=now_sec()-t_wall;
    fprintf(stderr,"  Decode: %.2f MB/s  (%.3fs, algorithmic)\n",hdr.orig_size/dec_time/1e6,dec_time);
    fprintf(stderr,"  Decode wall: %.2f MB/s  (%.3fs, wall clock)\n",hdr.orig_size/wall/1e6,wall);
    if(!ok) fprintf(stderr,"  Status: ❌ HASH MISMATCH\n");
 
    
    fprintf(stderr,"g_match_calls=%llu g_tokens.size=%zu\n",(unsigned long long)g_match_calls,g_tokens.size());
    if(!g_tokens.empty()){
        size_t N=hdr.orig_size;
        std::vector<uint32_t> tok_of(N,0xFFFFFFFFu);
        for(uint32_t ti=0;ti<g_tokens.size();ti++)
            for(uint32_t i=0;i<g_tokens[ti].len&&g_tokens[ti].pos+i<N;i++)
                tok_of[g_tokens[ti].pos+i]=ti;
        std::vector<int32_t> lev(g_tokens.size(),0); int ml=0;
        for(uint32_t ti=0;ti<g_tokens.size();ti++){
            int mx=0; uint32_t s=g_tokens[ti].src,e=s+g_tokens[ti].len,pp=s;
            while(pp<e&&pp<N){uint32_t st=tok_of[pp];
                    if(st!=0xFFFFFFFFu){if(lev[st]+1>mx)mx=lev[st]+1;
                        uint32_t nx=g_tokens[st].pos+g_tokens[st].len;pp=(nx>pp)?nx:pp+1;}
                    else{pp++;}}
            lev[ti]=mx; if(mx>ml)ml=mx;
        }
        double avg=0; for(auto v:lev)avg+=v; avg/=lev.size();
        fprintf(stderr,"\n=== REAL ACEAPEX DEPTH ===\n");
        fprintf(stderr,"Match calls: %llu\n",(unsigned long long)g_match_calls);
        fprintf(stderr,"Tokens: %zu  MaxLevel: %d  AvgLevel: %.1f\n",g_tokens.size(),ml,avg);
        { std::vector<uint64_t> hist(3244,0);
          for(auto v:lev) hist[std::min(v,(int32_t)3243)]++;
          fprintf(stderr,"=== DEPTH DISTRIBUTION ===\n");
          uint64_t total=lev.size(),cum=0;
          int limits[]={1,2,3,5,10,20,50,100,200,500,1000,2000,3243};
          for(int li=0;li<13;li++){int d=limits[li];
            for(int i=(li?limits[li-1]+1:0);i<=d;i++)cum+=hist[i];
            fprintf(stderr,"depth<=%4d: %6.2f%% cumulative\n",d,100.0*cum/total);
            if(cum>=total)break;}}
        FILE*ft=fopen("tokens.bin","wb");
        size_t ntok=g_tokens.size();fwrite(&ntok,8,1,ft);
        std::vector<uint32_t> tp(ntok),ts(ntok),tl(ntok),tlit(ntok);
        for(uint32_t i=0;i<ntok;i++){tp[i]=g_tokens[i].pos;ts[i]=g_tokens[i].src;tl[i]=g_tokens[i].len;}
        fwrite(tp.data(),4,ntok,ft);fwrite(ts.data(),4,ntok,ft);
        fwrite(tl.data(),4,ntok,ft);fwrite(tlit.data(),4,ntok,ft);fclose(ft);
        FILE*fl=fopen("levels.bin","wb");fwrite(lev.data(),4,ntok,fl);fclose(fl);
        fprintf(stderr,"Dumped: tokens.bin + levels.bin\n");
        // Создаём буфер: литеральные позиции=правильные, матч-позиции=0
        { std::vector<uint8_t> lit_pos(dst,dst+N);
          for(uint32_t ti=0;ti<g_tokens.size();ti++)
              for(uint32_t k=0;k<g_tokens[ti].len&&g_tokens[ti].pos+k<N;k++)
                  lit_pos[g_tokens[ti].pos+k]=0; // зануляем матч-позиции
          FILE*fp=fopen("lit_positions.bin","wb");
          fwrite(lit_pos.data(),1,N,fp); fclose(fp);
          fprintf(stderr,"Dumped: lit_positions.bin\n"); }
    }
free(lit); free(off); free(len); free(cmd); free(dst);
    return ok?0:1;
}
 
static int do_test(const char* in_path, int threads, int level=2) {
    load_forced();
    FILE* fin=fopen(in_path,"rb");
    if (!fin) { fprintf(stderr,"Cannot open: %s\n",in_path); return 1; }
    fseek(fin,0,SEEK_END); size_t src_size=(size_t)ftell(fin); fseek(fin,0,SEEK_SET);
    uint8_t* src=(uint8_t*)malloc(src_size);
    if(!src){fclose(fin);return 1;}
    fread(src,1,src_size,fin); fclose(fin);
    fprintf(stderr,"[*] Test: %s (%.2f MB) threads=%d\n",in_path,src_size/1e6,threads);
    double t_total_t=now_sec();
 
    std::vector<BlockOffsets> boffs;
    uint8_t *raw_lit,*raw_off,*raw_len,*raw_cmd;
    size_t total_lit,total_off,total_len,total_cmd,num_blocks;
    encode_file(src,src_size,threads,level,boffs,
                raw_lit,total_lit,raw_off,total_off,
                raw_len,total_len,raw_cmd,total_cmd,
                num_blocks);
 
    size_t zlit_sz,zoff_sz,zlen_sz,zcmd_sz;
    uint8_t *zlit,*zoff,*zlen,*zcmd;
    zlit=lit_compress(raw_lit,total_lit,zlit_sz);
    entropy_encode(raw_lit,total_lit,raw_off,total_off,raw_len,total_len,raw_cmd,total_cmd,
                   zlit,zlit_sz,zoff,zoff_sz,zlen,zlen_sz,zcmd,zcmd_sz);
 
    size_t total_z=zlit_sz+zoff_sz+zlen_sz+zcmd_sz;
 
    size_t off_sz=*(uint64_t*)zoff;
    size_t len_sz=*(uint64_t*)zlen;
    size_t cmd_sz=*(uint64_t*)zcmd;
 
    size_t lit_sz=0; uint8_t* lit=lit_decompress(zlit,zlit_sz,lit_sz);
    if(!lit) return 1;
    uint8_t* off=(uint8_t*)malloc(off_sz);
    uint8_t* len=(uint8_t*)malloc(len_sz);
    uint8_t* cmd=(uint8_t*)malloc(cmd_sz);
    uint8_t* dst=(uint8_t*)malloc(src_size + DST_PAD);
    if(!off||!len||!cmd||!dst){free(lit);free(off);free(len);free(cmd);free(dst);return ACEAPEX_ERR_MEMORY;}
    fse_chunked_decomp(zoff,off_sz,off);
    fse_chunked_decomp(zlen,len_sz,len);
    fse_chunked_decomp(zcmd,cmd_sz,cmd);
    parallel_decode(lit,off,len,cmd,boffs.data(),num_blocks,
                    dst,src_size,g_block_size);
 
    uint8_t digest_orig[32], digest_dec[32];
    sha256(src,src_size,digest_orig); sha256(dst,src_size,digest_dec);
    bool ok=(memcmp(digest_orig,digest_dec,32)==0);
    char sha_hex[65]; sha256_hex(src,src_size,sha_hex);
 
    fprintf(stderr,"\n  ====================================================\n");
    fprintf(stderr,"  ACEAPEX v3 FSE TEST REPORT\n");
    fprintf(stderr,"  ====================================================\n");
    fprintf(stderr,"  Original:   %14zu bytes\n",src_size);
    fprintf(stderr,"  Compressed: %14zu bytes\n",total_z);
    fprintf(stderr,"  Ratio:  %.5fx   BPB: %.4f\n",(double)src_size/total_z,total_z*8.0/src_size);
    double real_enc_t=now_sec()-t_total_t;
    fprintf(stderr,"  Encode: %.2f MB/s  (%.3fs)\n",src_size/real_enc_t/1e6,real_enc_t);
    fprintf(stderr,"  Decode: n/a (timing removed from library)\n");
    fprintf(stderr,"  SHA256: %.16s...\n",sha_hex);
    fprintf(stderr,"  Status: %s\n",ok?"✅ BIT-PERFECT":"❌ HASH MISMATCH");
    fprintf(stderr,"  ====================================================\n");
 
    free(src); free(dst);
    free(raw_lit); free(raw_off); free(raw_len); free(raw_cmd);
    free(zlit); free(zoff); free(zlen); free(zcmd);
    free(lit); free(off); free(len); free(cmd);
    return ok?0:1;
}
 
#ifndef ACEAPEX_NO_MAIN
// ---------------------------------------------------------------------------
// REGION DECODE. Блоки самодостаточны, а таблица BlockOffsets даёт байтовые
// диапазоны четырёх потоков для каждого блока. Осталось распаковать не потоки
// целиком, а только куски, покрывающие эти диапазоны.
// ---------------------------------------------------------------------------

// FSE-поток: чанки по 512 KB, таблица размеров в заголовке.
// Распаковываем только чанки, пересекающие [from,to). Возвращаем буфер размера
// orig_sz, где валиден лишь запрошенный диапазон.
// Буфер выделяется НА ПОКРЫВАЕМЫЙ ДИАПАЗОН, а не на весь поток. Полный calloc сам по
// себе бесплатен (0.002 ms на 210 MB), но КАЖДОЕ первое касание страницы — page fault:
// 2 MB данных стоили 0.5 ms, то есть всю оставшуюся латентность региона.
// base_off — смещение начала буфера в координатах потока.
static uint8_t* fse_range(const uint8_t* src, size_t orig_sz, size_t from, size_t to,
                          size_t* base_off=nullptr) {
    const size_t CHUNK=fse_chunk_size();
    const uint64_t* cs=(const uint64_t*)(src+8);
    size_t nc=(orig_sz+CHUNK-1)/CHUNK;
    // Пустой диапазон: поток len не нужен 29% регионов (7478 блоков chr1 из 15 499
    // имеют len_sz=0, медиана 1 байт при таблице 16 байт на блок). Вызов ради нуля
    // байт стоит ~12 us по закону амплификации.
    if(to<=from){ if(base_off) *base_off=0; return (uint8_t*)calloc(1,1); }
    size_t c0=from/CHUNK, c1=(to?to-1:0)/CHUNK;
    if(c1>=nc) c1=nc?nc-1:0;
    size_t win_lo=c0*CHUNK, win_hi=std::min(orig_sz,(c1+1)*CHUNK);
    if(base_off) *base_off=win_lo;
    // malloc, не calloc: распакованные чанки перезаписывают окно целиком, а вне
    // запрошенного диапазона декодер блоков не читает. Зануление 130 KB на вызов
    // было самой дорогой операцией профиля (memset_avx512, 9.75%).
    uint8_t* out=(uint8_t*)malloc(win_hi>win_lo?win_hi-win_lo:1);
    if(!out) return nullptr;
    size_t p_off=8+nc*8;
    for(size_t i=0;i<nc;i++){
        size_t d_off=i*CHUNK;
        size_t raw=std::min<size_t>(CHUNK,orig_sz-d_off);
        size_t csz=(cs[i]>>63)?raw:(size_t)(cs[i]&~(uint64_t(1)<<63));
        if(d_off<to && d_off+raw>from){
            uint8_t* dst=out+(d_off-win_lo);
            if(cs[i]>>63) memcpy(dst,src+p_off,raw);
            else ZSTD_decompress(dst,raw,src+p_off,csz);
        }
        p_off+=csz;
    }
    return out;
}

// Литеральный поток: новая схема — чанки LIT_CHUNK, старая — NW равных долей.
static uint8_t* lit_range(const uint8_t* src, size_t src_sz, size_t& orig_sz,
                          size_t from, size_t to, size_t* base_off=nullptr) {
    if(!src||src_sz<8){orig_sz=0;return (uint8_t*)malloc(1);}
    uint64_t h; memcpy(&h,src,8);
    const bool chunked=(h&(uint64_t(1)<<61))!=0;
    const bool tagged=(h&(uint64_t(1)<<60))!=0;
    orig_sz=h&~((uint64_t(1)<<62)|(uint64_t(1)<<61)|(uint64_t(1)<<60));
    if(!(h&(uint64_t(1)<<62))) return fse_range(src,orig_sz,from,to);
    size_t csz=chunked?(size_t)(*(const uint64_t*)(src+8)):(orig_sz+3)/4;
    const int NW=chunked?(int)((orig_sz+csz-1)/csz):4;
    const uint64_t* zsz=(const uint64_t*)(src+(chunked?16:8));
    const uint8_t* p=src+(chunked?16:8)+(size_t)NW*8;
    size_t win_lo=(from/csz)*csz;
    size_t win_hi=std::min(orig_sz,((to?to-1:0)/csz+1)*csz);
    if(win_hi<win_lo) win_hi=win_lo;
    if(base_off) *base_off=win_lo;
    // malloc, не calloc: распакованные чанки перезаписывают окно целиком, а вне
    // запрошенного диапазона декодер блоков не читает. Зануление 130 KB на вызов
    // было самой дорогой операцией профиля (memset_avx512, 9.75%).
    uint8_t* out=(uint8_t*)malloc(win_hi>win_lo?win_hi-win_lo:1);
    if(!out) return nullptr;
    for(int t=0;t<NW;t++){
        size_t off=(size_t)t*csz; if(off>orig_sz) off=orig_sz;
        // Правило то же, что в кодере и в lit_decompress: кусок равен csz,
        // кроме последнего. Старая формула с (t<NW-1) на чанках расходилась.
        size_t raw=(off+csz<=orig_sz)?csz:(orig_sz-off);
        if(off<to && off+raw>from){
            uint8_t* d2=out+(off-win_lo);
            if(!tagged)          ZSTD_decompress(d2,raw,p,(size_t)zsz[t]);
            else if(p[0]==1)     dna_decompress(p+1,d2,raw);
            else                 ZSTD_decompress(d2,raw,p+1,(size_t)zsz[t]-1);
        }
        p+=(size_t)zsz[t];
    }
    return out;
}

// Три представления одного и того же среза:
//   source   — исходные байты файла, включая переводы строк (было раньше)
//   sequence — ровно TO-FROM+1 оснований без переводов
//   fasta    — заголовок ">name:from-to" плюс sequence, как отдаёт samtools faidx
// Основания у всех трёх одинаковы, различается только обёртка.
static int do_region(const char* in_path,const char* out_path,
                     uint64_t req_off,uint64_t req_len,
                     const char* view=nullptr,const char* rname=nullptr,
                     uint64_t rfrom=0,uint64_t rto=0,uint64_t lbases=0) {
    double t0=now_sec();
    FILE* fin=fopen(in_path,"rb");
    if(!fin){fprintf(stderr,"Cannot open: %s\n",in_path);return 1;}
    AetHeader hdr;
    if(fread(&hdr,sizeof(hdr),1,fin)!=1){fclose(fin);return 1;}
    if(memcmp(hdr.magic,"ACEPX2\0\0",8)!=0){fprintf(stderr,"Bad magic\n");fclose(fin);return 1;}
    if(req_off+req_len>hdr.orig_size){fprintf(stderr,"region beyond end\n");fclose(fin);return 1;}

    uint32_t nb=hdr.num_blocks;
    std::vector<BlockOffsets> boffs(nb);
    if(fread(boffs.data(),sizeof(BlockOffsets),nb,fin)!=nb){fclose(fin);return 1;}

    size_t b0=req_off/hdr.block_size;
    size_t b1=(req_off+req_len-1)/hdr.block_size;

    // байтовые диапазоны четырёх потоков, покрывающие блоки [b0,b1]
    size_t lf=boffs[b0].lit_off, lt=boffs[b1].lit_off+boffs[b1].lit_sz;
    size_t of=boffs[b0].off_off, ot=boffs[b1].off_off+boffs[b1].off_sz;
    size_t nf=boffs[b0].len_off, nt2=boffs[b1].len_off+boffs[b1].len_sz;
    size_t cf=boffs[b0].cmd_off, ct=boffs[b1].cmd_off+boffs[b1].cmd_sz;

    // Не читаем архив — отображаем. Ядро подкачает ТОЛЬКО те страницы, которых
    // коснутся fse_range/lit_range: таблицы размеров в началах потоков и один-два
    // чанка. Остальные 78 MB никогда не попадут в память.
    long base=ftell(fin);
    int fd=fileno(fin);
    struct stat st; fstat(fd,&st);
    uint8_t* map=(uint8_t*)mmap(nullptr,(size_t)st.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    if(map==MAP_FAILED){fclose(fin);fprintf(stderr,"mmap failed\n");return 1;}
    madvise(map,(size_t)st.st_size,MADV_RANDOM);
    const uint8_t* zlit=map+base;
    const uint8_t* zoff=zlit+hdr.zlit_sz;
    const uint8_t* zlen=zoff+hdr.zoff_sz;
    const uint8_t* zcmd=zlen+hdr.zlen_sz;
    fclose(fin);
    double t_read=now_sec()-t0;

    double t1=now_sec();
    size_t lit_sz=0, wl=0,wo=0,wn=0,wc=0;
    uint8_t* lit=lit_range(zlit,hdr.zlit_sz,lit_sz,lf,lt,&wl);
    uint8_t* off=fse_range(zoff,*(uint64_t*)zoff&~(uint64_t(1)<<63),of,ot,&wo);
    uint8_t* len=fse_range(zlen,*(uint64_t*)zlen&~(uint64_t(1)<<63),nf,nt2,&wn);
    uint8_t* cmd=fse_range(zcmd,*(uint64_t*)zcmd&~(uint64_t(1)<<63),cf,ct,&wc);
    if(!lit||!off||!len||!cmd){munmap(map,(size_t)st.st_size);return 1;}
    double t_ent=now_sec()-t1;

    double t2=now_sec();
    size_t span_start=b0*(size_t)hdr.block_size;
    size_t span_end=std::min<size_t>((b1+1)*(size_t)hdr.block_size,hdr.orig_size);
    uint8_t* dst=(uint8_t*)malloc(span_end-span_start+DST_PAD);
    if(!dst) return 1;
    for(size_t b=b0;b<=b1;b++){
        const BlockOffsets& bo=boffs[b];
        size_t bstart=b*(size_t)hdr.block_size;
        size_t bsize=std::min<size_t>((size_t)hdr.block_size,hdr.orig_size-bstart);
        decompress_streams(dst+(bstart-span_start),bsize,
            lit+(bo.lit_off-wl),bo.lit_sz, off+(bo.off_off-wo),bo.off_sz,
            len+(bo.len_off-wn),bo.len_sz, cmd+(bo.cmd_off-wc),bo.cmd_sz);
    }
    double t_lz=now_sec()-t2;

    FILE* fo=fopen(out_path,"wb");
    if(fo){
        const uint8_t* p0=dst+(req_off-span_start);
        bool want_seq = view && (!strcmp(view,"sequence")||!strcmp(view,"fasta"));
        if(!want_seq){
            fwrite(p0,1,req_len,fo);
        } else {
            if(!strcmp(view,"fasta") && rname)
                fprintf(fo,">%s:%llu-%llu\n",rname,
                        (unsigned long long)rfrom,(unsigned long long)rto);
            // снимаем переводы строк; для fasta заворачиваем по 60, как htslib
            size_t wrap = (!strcmp(view,"fasta")) ? 60 : 0, col=0, wrote=0;
            for(uint64_t i=0;i<req_len;i++){
                uint8_t c=p0[i];
                if(c=='\n'||c=='\r') continue;
                fputc(c,fo); wrote++;
                if(wrap && ++col==wrap){ fputc('\n',fo); col=0; }
            }
            if(wrap && col) fputc('\n',fo);
            uint64_t want = (rto>=rfrom) ? (rto-rfrom+1) : 0;
            if(want && wrote!=want)
                fprintf(stderr,"  WARNING: wrote %llu bases, expected %llu\n",
                        (unsigned long long)wrote,(unsigned long long)want);
        }
        fclose(fo);
    }
    fprintf(stderr,"  blocks %zu..%zu  read %.3fs  entropy %.3fs  lz77 %.3fs  total %.3fs\n",
            b0,b1,t_read,t_ent,t_lz,now_sec()-t0);
    free(lit);free(off);free(len);free(cmd);free(dst);
    munmap(map,(size_t)st.st_size);
    return 0;
}

// ---------------------------------------------------------------------------
// FASTA-ИНДЕКС. Регион до сих пор адресовался БАЙТАМИ файла, а биологу нужна
// координата: chr1:5000000-5016000. Для FASTA перевод арифметический — заголовок,
// фиксированная длина строки, переводы строк, — и совпадает с .fai у samtools:
// name, length, offset первого основания, оснований в строке, байт в строке.
// Мы читаем ту же таблицу, но дальше идём в aceapex_decompress_region.
// ---------------------------------------------------------------------------
struct FaiRec { char name[256]; uint64_t len, off, lbases, lbytes; };

static bool fai_load(const char* path, std::vector<FaiRec>& out, uint64_t& src_hash){
    src_hash=0;
    FILE* f=fopen(path,"r"); if(!f) return false;
    char line[1024]; unsigned long long a,b,c,d; char nm[256];
    while(fgets(line,sizeof line,f)){
        if(line[0]=='#'){
            unsigned long long h;
            if(sscanf(line,"#aceapex-src-xxh3 %llx",&h)==1) src_hash=h;
            continue;
        }
        if(sscanf(line,"%255s %llu %llu %llu %llu",nm,&a,&b,&c,&d)!=5) continue;
        FaiRec r; snprintf(r.name,sizeof r.name,"%s",nm);
        r.len=a; r.off=b; r.lbases=c; r.lbytes=d;
        out.push_back(r);
    }
    fclose(f); return !out.empty();
}

// Индекс строится ОДНИМ проходом по несжатому входу — при кодировании он уже в
// памяти, поэтому стоит около нуля.
// Индекс несёт XXH3-64 файла, для которого построен. Архив несёт свой в заголовке.
// Не совпало — отказ: чужой или устаревший индекс даёт НЕ ТОТ участок генома молча,
// и это опаснее любой ошибки формата. Строка начинается с '#', samtools её игнорирует.
static bool fai_build(const uint8_t* src, size_t n, const char* out_path){
    FILE* fo=fopen(out_path,"w"); if(!fo) return false;
    fprintf(fo,"#aceapex-src-xxh3\t%016llx\n",
            (unsigned long long)OUR_CHECKSUM(src,n));
    size_t i=0;
    while(i<n){
        if(src[i]!='>'){ i++; continue; }
        size_t hs=i+1, he=hs;
        while(he<n && src[he]!='\n' && src[he]!=' ' && src[he]!='\t') he++;
        char name[256];
        size_t nl2 = (he-hs < sizeof(name)-1) ? (he-hs) : (sizeof(name)-1);
        memcpy(name, src+hs, nl2); name[nl2]=0;
        size_t eol=he; while(eol<n && src[eol]!='\n') eol++;
        size_t seq=eol+1;
        // Геометрию задаёт ПЕРВАЯ строка, но остальные обязаны ей соответствовать:
        // htslib отвергает рваные записи, и мы обязаны тоже. Иначе смещения врут молча,
        // а это худший класс ошибки — пользователь получает не тот участок генома.
        // CR перед LF не считается основанием (файлы с CRLF).
        size_t l1=seq; while(l1<n && src[l1]!='\n') l1++;
        size_t l1b=l1; if(l1b>seq && src[l1b-1]=='\r') l1b--;
        uint64_t lb=l1b-seq, lby=l1-seq+1;      // оснований и БАЙТ в строке
        if(lb==0){ fprintf(stderr,"faidx: empty first line in %s\n",name);
                   fclose(fo); remove(out_path); return false; }
        uint64_t total=0; size_t p=seq; bool last=false;
        while(p<n && src[p]!='>'){
            size_t e=p; while(e<n && src[e]!='\n') e++;
            size_t eb=e; if(eb>p && src[eb-1]=='\r') eb--;
            uint64_t cur=eb-p;
            if(last && cur>0){
                fprintf(stderr,"faidx: ragged lines in %s near offset %llu\n",
                        name,(unsigned long long)p);
                fclose(fo); remove(out_path); return false;
            }
            if(cur<lb) last=true;               // короче нормы — допустима только последняя
            else if(cur>lb){
                fprintf(stderr,"faidx: line longer than the first in %s near offset %llu\n",
                        name,(unsigned long long)p);
                fclose(fo); remove(out_path); return false;
            }
            total += cur; p = (e<n) ? e+1 : e;
        }
        fprintf(fo,"%s\t%llu\t%llu\t%llu\t%llu\n", name,
                (unsigned long long)total,(unsigned long long)seq,
                (unsigned long long)lb,(unsigned long long)lby);
        i=p;
    }
    fclose(fo); return true;
}

// Координата (1-based, включительно) -> байтовый диапазон в несжатом файле.
static bool fai_range(const FaiRec& r, uint64_t from, uint64_t to,
                      uint64_t& b0, uint64_t& b1){
    if(from<1 || to<from || to>r.len) return false;
    b0 = r.off + (from-1)/r.lbases * r.lbytes + (from-1)%r.lbases;
    b1 = r.off + (to-1)  /r.lbases * r.lbytes + (to-1)  %r.lbases;
    return true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr,"ACEAPEX v3 FSE — Global FSE + Parallel decode\n\n"
            "Usage:\n  %s c --in <f> --out <f.aet> [--threads N]\n"
            "  %s d --in <f.aet> --out <f>\n  %s t --in <f> [--threads N]\n"
            "  %s r --in <f.aet> --out <f> --region OFFSET LENGTH\n",
            argv[0],argv[0],argv[0],argv[0]);
        return 1;
    }
    const char* cmd=argv[1]; const char* in=nullptr; const char* out=nullptr; int thr=8; int level=2;
    const char* profile=nullptr;
    uint64_t reg_off=0, reg_len=0;
    const char* fai_path=nullptr; const char* range_spec=nullptr;
    const char* view_mode="source";   // source | sequence | fasta
    const char* fai_out=nullptr;
    for(int i=2;i<argc;i++) {
        if (!strcmp(argv[i],"--in")&&i+1<argc) in=argv[++i];
        else if (!strcmp(argv[i],"--out")&&i+1<argc) out=argv[++i];
        else if (!strcmp(argv[i],"--threads")&&i+1<argc) thr=atoi(argv[++i]);
        else if (!strcmp(argv[i],"--level")&&i+1<argc) level=atoi(argv[++i]);
        else if (!strcmp(argv[i],"--fast")) level=1;
        else if (!strcmp(argv[i],"--fai")&&i+1<argc) fai_path=argv[++i];
        else if (!strcmp(argv[i],"--view")&&i+1<argc) view_mode=argv[++i];
        else if (!strcmp(argv[i],"--fai-out")&&i+1<argc) fai_out=argv[++i];
        else if (!strcmp(argv[i],"--range")&&i+1<argc) range_spec=argv[++i];
        else if (!strcmp(argv[i],"--region")&&i+2<argc){
            reg_off=strtoull(argv[++i],0,10); reg_len=strtoull(argv[++i],0,10); }
        else if (!strcmp(argv[i],"--profile")&&i+1<argc) profile=argv[++i];
    }
    // Профили — точки Парето-фронта из sweep.jsonl (324 точки, 09.09, все bit-perfect).
    // Ни один не назначен вручную. Явная переменная окружения побеждает профиль.
    if(profile){
        struct { const char* n; const char* bs; const char* lit; const char* fse; const char* why; } P[]={
            {"interactive","16384", "65536",  "4096", "seek 0.082 ms, ratio 3.655 (chr1)"},
            {"seekable",   "262144","262144", "32768","seek 0.49 ms, ratio 3.769"},
            {"dense",      "262144","1048576","32768","ratio 3.778, seek 0.85 ms"},
            {"fast-decode","262144","1048576","4096", "decode 743 MB/s, ratio 3.761"},
        };
        int ok=0;
        for(auto& e:P) if(!strcmp(profile,e.n)){
            setenv("ACEAPEX_BS",e.bs,0); setenv("LIT_CHUNK",e.lit,0); setenv("FSE_CHUNK",e.fse,0);
            fprintf(stderr,"  profile %s: %s\n",e.n,e.why); ok=1; break; }
        if(!ok){ fprintf(stderr,"unknown profile '%s' (interactive|seekable|dense|fast-decode)\n",profile); return 1; }
    }
    if (!in) { fprintf(stderr,"--in required\n"); return 1; }
    if (!strcmp(cmd,"c")) {
        if (!out) { fprintf(stderr,"--out required\n"); return 1; }
        int rc = do_compress(in,out,thr,level);
        // Индекс строится в момент сжатия — только так он гарантированно про ТОТ ЖЕ файл.
        if (rc==0 && fai_out) {
            double t0=now_sec();
            FILE* fi=fopen(in,"rb");
            if(fi){
                fseek(fi,0,SEEK_END); long fsz=ftell(fi); fseek(fi,0,SEEK_SET);
                uint8_t* b=(uint8_t*)malloc(fsz);
                if(b && fread(b,1,fsz,fi)==(size_t)fsz)
                    fprintf(stderr,"  fai: %s (%.3fs)\n",
                            fai_build(b,(size_t)fsz,fai_out)?"written":"FAILED", now_sec()-t0);
                free(b); fclose(fi);
            }
        }
        return rc;
    }
    if (!strcmp(cmd,"d")) { if (!out) { fprintf(stderr,"--out required\n"); return 1; } return do_decompress(in,out,thr); }
    if (!strcmp(cmd,"t")) return do_test(in,thr,level);
    if (!strcmp(cmd,"faidx")) {
        FILE* fi=fopen(in,"rb"); if(!fi){fprintf(stderr,"cannot open\n");return 1;}
        fseek(fi,0,SEEK_END); long fsz=ftell(fi); fseek(fi,0,SEEK_SET);
        uint8_t* buf=(uint8_t*)malloc(fsz);
        if(fread(buf,1,fsz,fi)!=(size_t)fsz){fclose(fi);return 1;}
        fclose(fi);
        char op[1024];
        if(out) snprintf(op,sizeof op,"%s",out);
        else    snprintf(op,sizeof op,"%s.fai",in);
        bool ok=fai_build(buf,(size_t)fsz,op);
        free(buf);
        fprintf(stderr,"%s %s\n", ok?"wrote":"failed", op);
        return ok?0:1;
    }
    if (!strcmp(cmd,"r")) {
        if(!out){fprintf(stderr,"--out required\n");return 1;}
        // Координата вместо байтов: --fai chr1.fai --range chr1:5000000-5016000
        char rng_name[256]={0}; uint64_t rng_from=0,rng_to=0,rng_lb=0;
        if(range_spec){
            if(!fai_path){fprintf(stderr,"--range requires --fai\n");return 1;}
            std::vector<FaiRec> recs; uint64_t fai_hash=0;
            if(!fai_load(fai_path,recs,fai_hash)){fprintf(stderr,"cannot read %s\n",fai_path);return 1;}
            {   // индекс обязан относиться к ЭТОМУ архиву
                FILE* fh=fopen(in,"rb"); AetHeader hh;
                if(fh && fread(&hh,sizeof hh,1,fh)==1){
                    uint64_t ah; memcpy(&ah,hh.xxhash,8);
                    if(fai_hash && fai_hash!=ah){
                        fprintf(stderr,"index/archive mismatch: fai %016llx, archive %016llx\n",
                                (unsigned long long)fai_hash,(unsigned long long)ah);
                        fclose(fh); return 1;
                    }
                    if(!fai_hash)
                        fprintf(stderr,"  note: index has no source hash, cannot verify it matches\n");
                }
                if(fh) fclose(fh);
            }
            char nm[256]; unsigned long long a=0,b=0;
            if(sscanf(range_spec,"%255[^:]:%llu-%llu",nm,&a,&b)!=3){
                fprintf(stderr,"bad --range, expected NAME:FROM-TO\n"); return 1; }
            const FaiRec* r=nullptr;
            for(auto& x:recs) if(!strcmp(x.name,nm)) { r=&x; break; }
            if(!r){fprintf(stderr,"%s not in index\n",nm);return 1;}
            uint64_t b0,b1;
            if(!fai_range(*r,a,b,b0,b1)){fprintf(stderr,"range outside %s (len %llu)\n",
                nm,(unsigned long long)r->len);return 1;}
            reg_off=b0; reg_len=b1-b0+1;
            snprintf(rng_name,sizeof rng_name,"%s",nm);
            rng_from=a; rng_to=b; rng_lb=r->lbases;
            fprintf(stderr,"  %s:%llu-%llu -> bytes %llu..%llu (%llu)\n",
                nm,a,b,(unsigned long long)b0,(unsigned long long)b1,
                (unsigned long long)reg_len);
        }
        if(reg_len==0){fprintf(stderr,"--region OFFSET LENGTH required\n");return 1;}
        return do_region(in,out,reg_off,reg_len,view_mode,rng_name,rng_from,rng_to,rng_lb);
    }
    return 1;
}
#endif // ACEAPEX_NO_MAIN
