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

 
#define HASH_SIZE    0xFFFF
#define MAX_DIST     (128 * 1024 * 1024)
#define BLOCK_SIZE   (1 * 1024 * 1024)
static size_t g_block_size = BLOCK_SIZE; // runtime adaptive, set in encode_file
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
 
struct ThreadHashTable {
    int32_t*  pos;
    uint32_t* epoch;
    int32_t*  chain;
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
 
static inline uint32_t min_match_len(uint32_t dist) {
    if (dist < 128)     return 6;
    if (dist < 16384)   return 8;
    if (dist < 2097152) return 10;
    return 12;
}
 

struct Match { uint32_t len, off; int rep; };
static inline int find_matches(const uint8_t* src, size_t pos, size_t bstart, size_t bend,
                                ThreadHashTable* ht, uint32_t* rep, Match* out, int maxout) {
    int max_attempts = ht->max_attempts;
    int n = 0;
    uint32_t maxl = (uint32_t)(bend - pos);
    for (int i = 0; i < 4 && n < maxout; i++) {
        uint32_t d = rep[i]; if (pos < bstart+d) continue;
        if (*(uint32_t*)(src+pos)!=*(uint32_t*)(src+pos-d)) continue;
        uint32_t l=4; while(l<maxl&&src[pos+l]==src[pos-d+l]&&l<65535) l++;
        if (l>=6) out[n++]={l,d,i};
    }
    uint32_t h=((*(uint32_t*)(src+pos)*0x9E3779B1u)>>10)&ht->hash_mask;
    int32_t head=(ht->epoch[h]==ht->cur_epoch)?ht->pos[h]:-1;
    ht->pos[h]=(int32_t)pos; ht->epoch[h]=ht->cur_epoch;
    if (head>=0) ht->chain[pos & ht->chain_mask]=head;
    int32_t cur=head; int attempts=max_attempts;
    while(cur>=(int32_t)bstart && attempts-->0 && n<maxout) {
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
        int32_t nxt=ht->chain[cur & ht->chain_mask];
        if(nxt<0||nxt>=cur) break; cur=nxt;
    }
    return n;
}
static int g_depth_limit = 2147483647;

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
    if (ht->cur_epoch == 0) {
        memset(ht->epoch, 0, (ht->hash_mask+1)*sizeof(uint32_t)); ht->cur_epoch = 1;
    }
 
    size_t lit_i=0, off_i=0, len_i=0, cmd_i=0, pos=bstart;
    uint32_t rep[4]={1,2,4,8}, lit_run=0, miss=0;
    int ov=0;
    res->cmd_buf[cmd_i++] = BLOCK_MARKER;

    // ULTRA: Chain flattening origin table
    // origin[local_pos] = original literal source position (local)
    static thread_local uint32_t origin[1048576];
    static thread_local uint32_t enc_depth[1048576];
    size_t flat_pos = 0; // track which positions are initialized
    auto init_origin = [&](size_t from, size_t to) {
        for (size_t i = from; i < to && i < 1048576; i++) origin[i] = (uint32_t)i;
    };
    init_origin(0, bsz);
    memset(enc_depth, 0, bsz * sizeof(uint32_t)); // init all as self-referential (literal)
 
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
            int32_t mp1=(ht->epoch[h1]==ht->cur_epoch)?ht->pos[h1]:-1;
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
                int32_t mp2=(ht->epoch[h2]==ht->cur_epoch)?ht->pos[h2]:-1;
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
        // DEPTH LIMIT: check before emitting match
        if (c_len >= 6 && g_depth_limit < 2147483647) {
            size_t lp = pos - bstart;
            uint32_t dist = (c_rep >= 0) ? rep[c_rep] : c_off;
            uint32_t sd = 0;
            if (dist <= lp) { size_t sl=lp-dist;
                for(size_t k=0;k<(size_t)c_len&&sl+k<bsz;k++) sd=std::max(sd,enc_depth[sl+k]); }
            if (sd+1 > (uint32_t)g_depth_limit) c_len = 0; // force literal
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
                if (c_off <= local_pos) {
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
                    for (size_t fi = 0; fi < c_len && local_pos+fi < 1048576 && src_local+fi < 1048576; fi++)
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
                ht->chain[(pos+ii)&ht->chain_mask]=ht->pos[hh]; ht->pos[hh]=(int32_t)(pos+ii); ht->epoch[hh]=ht->cur_epoch;
              }
            }
            // Update enc_depth for emitted match
            if (g_depth_limit < 2147483647) {
                size_t lp=pos-bstart;
                uint32_t dist2=(c_rep>=0)?rep[0]:c_off; // rep[0] updated already for REP
                uint32_t sd2=0;
                if(dist2<=lp){size_t sl2=lp-dist2;
                    for(size_t k=0;k<(size_t)c_len&&sl2+k<bsz;k++) sd2=std::max(sd2,enc_depth[sl2+k]);}
                uint32_t nd=sd2+1;
                for(size_t k=0;k<(size_t)c_len&&lp+k<bsz;k++) enc_depth[lp+k]=nd;
            }
            pos+=c_len; continue;
        }
        if (lit_i>=lit_cap) { ov=1; break; }
        res->lit_buf[lit_i++]=src[pos++]; lit_run++; miss++;
        if (miss>=1 && pos+12<bend) {
            uint32_t hh=((*(uint32_t*)(src+pos)*0x9E3779B1u)>>10)&ht->hash_mask;
            if(hh<=ht->hash_mask) { ht->pos[hh]=(int32_t)pos; ht->epoch[hh]=ht->cur_epoch; }
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
 
static inline void copy_match(uint8_t* dst, size_t out_ptr, uint32_t dist, uint32_t len) {
    uint8_t* d = dst + out_ptr;
    const uint8_t* s = dst + out_ptr - dist;
    if (__builtin_expect(dist >= len, 1)) { memcpy(d, s, len); return; }
    if (dist == 1) { memset(d, s[0], len); return; }
    uint32_t done = 0;
    while (done + dist <= len) { memcpy(d + done, s, dist); done += dist; }
    if (done < len) memcpy(d + done, s, len - done);
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
            copy_match(dst,out,dist,l); out+=l;
        } else {
            uint32_t lv=(c==0xFE)?read_varint(len,np,len_sz):(uint32_t)(c&0x3F);
            uint32_t l=lv+6, dist=read_varint(off,op,off_sz);
            rep[3]=rep[2];rep[2]=rep[1];rep[1]=rep[0];rep[0]=dist;
            if (!dist||out+l>dst_size) break;
            copy_match(dst,out,dist,l); out+=l;
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
                copy_match(dst,out2,dist,l); // fallback
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
                copy_match(dst,out2,dist,l);
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
        copy_match(dst, o.dst_off, o.dst_off-o.src_off, o.len);
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

static bool encode_file(const uint8_t* src, size_t src_size, int threads, int level,
    std::vector<BlockOffsets>& boffs,
    uint8_t*& raw_lit, size_t& total_lit,
    uint8_t*& raw_off, size_t& total_off,
    uint8_t*& raw_len, size_t& total_len,
    uint8_t*& raw_cmd, size_t& total_cmd,
    size_t& num_blocks)
{
    g_block_size = compute_block_size(src_size, threads);
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
        htabs[i]->pos  =(int32_t*) calloc(ht_sz,sizeof(int32_t));
        htabs[i]->epoch=(uint32_t*)calloc(ht_sz,sizeof(uint32_t));
        htabs[i]->chain=(int32_t*) malloc(((size_t)chain_mask+1)*sizeof(int32_t));
        if(!htabs[i]->pos||!htabs[i]->epoch||!htabs[i]->chain){return false;}
        memset(htabs[i]->chain,-1,((size_t)chain_mask+1)*sizeof(int32_t));
        htabs[i]->cur_epoch=0;
        htabs[i]->hash_mask=hash_mask;
        htabs[i]->chain_mask=chain_mask;
        htabs[i]->max_attempts=(level>=2)?32:4;
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
static void fse_chunked_decomp(const uint8_t* src, size_t orig_sz, uint8_t* dst) {
    const size_t CHUNK=512*1024;
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
static uint8_t* lit_compress(const uint8_t* src, size_t sz, size_t& out_sz) {
    const int NW=4; size_t csz=(sz+NW-1)/NW;
    struct ZW{const uint8_t*in;size_t isz;uint8_t*out;size_t osz;size_t cap;};
    ZW zws[NW];
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
        ZSTD_CCtx_setParameter(ctx,ZSTD_c_compressionLevel,3);
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
static uint8_t* lit_decompress(const uint8_t* src, size_t src_sz, size_t& orig_sz) {
    // An empty or truncated literal stream must not be read as an 8-byte header.
    // Tiny inputs give zlit_sz==0; the out-of-bounds read corrupted heap metadata
    // and surfaced as a double-free thousands of calls later. (lzbench issue #2.)
    if (!src || src_sz < 8) { orig_sz = 0; return (uint8_t*)malloc(1); }
    uint64_t h; memcpy(&h, src, 8);   // alignment-safe (no AX_read64 in this tree)
    orig_sz=h & ~(uint64_t(1)<<62);
    uint8_t* out=(uint8_t*)malloc(orig_sz);
    if(!out) return nullptr;
    if(!(h & (uint64_t(1)<<62))){fse_chunked_decomp(src,orig_sz,out);return out;}
    const int NW=4; const uint64_t* zsz=(const uint64_t*)(src+8);
    const uint8_t* p0=src+8+NW*8;
    size_t csz=(orig_sz+NW-1)/NW;
    struct DW{uint8_t*out;size_t raw;const uint8_t*in;size_t isz;};
    DW dws[NW]; const uint8_t* p=p0;
    for(int t=0;t<NW;t++){
        // Same underflow guard as in lit_compress (decoder side).
        size_t off=(size_t)t*csz; if(off>orig_sz) off=orig_sz;
        size_t raw=(t<NW-1)?((off+csz<=orig_sz)?csz:(orig_sz-off)):(orig_sz-off);
        dws[t]={out+off,raw,p,(size_t)zsz[t]}; p+=(size_t)zsz[t];}
    auto dfn=[](void*a)->void*{DW*d=(DW*)a;
        ZSTD_decompress(d->out,d->raw,d->in,d->isz); return nullptr;};
    pthread_t pts[NW];
    for(int t=0;t<NW;t++) pthread_create(&pts[t],nullptr,dfn,&dws[t]);
    for(int t=0;t<NW;t++) pthread_join(pts[t],nullptr);
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
        const size_t CHUNK=512*1024;
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
 
static int do_compress(const char* in_path, const char* out_path, int threads, int level=2) {
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
    // Запускаем SHA256 параллельно с encode
    struct ShaArg { const uint8_t* d; size_t n; uint8_t out[32]; };
    ShaArg sha_arg={src,src_size,{}};
    pthread_t sha_thr;
    pthread_create(&sha_thr,nullptr,[](void*a)->void*{
        ShaArg*s=(ShaArg*)a; sha256(s->d,s->n,s->out); return nullptr;
    },&sha_arg);

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
 
static int do_decompress(const char* in_path, const char* out_path, int threads=8) {
    double t_wall=now_sec();
    FILE* fin=fopen(in_path,"rb");
    if (!fin) { fprintf(stderr,"Cannot open: %s\n",in_path); return 1; }
    AetHeader hdr;
    fread(&hdr,sizeof(hdr),1,fin);
    if (memcmp(hdr.magic,"ACEPX2\0\0",8)!=0) { fprintf(stderr,"Bad magic\n"); return 1; }
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
        *l->out=lit_decompress(l->s,l->sz,*l->osz); return nullptr;};
    struct FD{const uint8_t*s;size_t sz;uint8_t*d;};
    FD fds[3]={{zoff,off_sz,off},{zlen,len_sz,len},{zcmd,cmd_sz,cmd}};
    auto fdfn=[](void*a)->void*{FD*f=(FD*)a;
        size_t orig=*(const uint64_t*)f->s&~(uint64_t(1)<<63);
        fse_chunked_decomp(f->s,orig,f->d); return nullptr;};
    pthread_t fpts[4];
    pthread_create(&fpts[0],nullptr,litfn,&larg);
    for(int i=0;i<3;i++) pthread_create(&fpts[i+1],nullptr,fdfn,&fds[i]);
    for(int i=0;i<4;i++) pthread_join(fpts[i],nullptr);
    if(!lit){free(off);free(len);free(cmd);free(zlit);free(zoff);free(zlen);free(zcmd);return 1;}
    double t_fse=now_sec()-t_lit;
    t_lit=t_fse;
    free(zlit); free(zoff); free(zlen); free(zcmd);
    uint8_t* dst=(uint8_t*)malloc(hdr.orig_size);
    if(!dst){free(lit);free(off);free(len);free(cmd);return 1;}
    double t_lz=now_sec(); parallel_decode(lit,off,len,cmd,boffs.data(),nb,dst,hdr.orig_size,hdr.block_size,threads); t_lz=now_sec()-t_lz;
    dec_time=now_sec()-dec_time;
    fprintf(stderr,"  Phase lit:  %.3fs\n  Phase fse:  %.3fs\n  Phase lz77: %.3fs\n",t_lit,t_fse,t_lz);
 
    uint64_t dv=OUR_CHECKSUM(dst,hdr.orig_size);
    uint64_t hv3; memcpy(&hv3,hdr.xxhash,8);
    bool ok=(dv==hv3);
    FILE* fout=fopen(out_path,"wb");
    if (fout) { fwrite(dst,1,hdr.orig_size,fout); fclose(fout); }
    double wall=now_sec()-t_wall;
    fprintf(stderr,"  Decode: %.2f MB/s  (%.3fs, algorithmic)\n",hdr.orig_size/dec_time/1e6,dec_time);
    fprintf(stderr,"  Decode wall: %.2f MB/s  (%.3fs, wall clock)\n",hdr.orig_size/wall/1e6,wall);
    if(!ok) fprintf(stderr,"  Status: ❌ HASH MISMATCH\n");
 
    free(lit); free(off); free(len); free(cmd); free(dst);
    return ok?0:1;
}
 
static int do_test(const char* in_path, int threads, int level=2) {
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
    uint8_t* dst=(uint8_t*)malloc(src_size);
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
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr,"ACEAPEX v3 FSE — Global FSE + Parallel decode\n\n"
            "Usage:\n  %s c --in <f> --out <f.aet> [--threads N]\n"
            "  %s d --in <f.aet> --out <f>\n  %s t --in <f> [--threads N]\n",
            argv[0],argv[0],argv[0]);
        return 1;
    }
    const char* cmd=argv[1]; const char* in=nullptr; const char* out=nullptr; int thr=8; int level=2;
    for(int i=2;i<argc;i++) {
        if (!strcmp(argv[i],"--in")&&i+1<argc) in=argv[++i];
        else if (!strcmp(argv[i],"--out")&&i+1<argc) out=argv[++i];
        else if (!strcmp(argv[i],"--threads")&&i+1<argc) thr=atoi(argv[++i]);
        else if (!strcmp(argv[i],"--level")&&i+1<argc) level=atoi(argv[++i]);
        else if (!strcmp(argv[i],"--fast")) level=1;
            else if (!strcmp(argv[i],"--depth-limit")&&i+1<argc) g_depth_limit=atoi(argv[++i]);
    }
    if (!in) { fprintf(stderr,"--in required\n"); return 1; }
    if (!strcmp(cmd,"c")) { if (!out) { fprintf(stderr,"--out required\n"); return 1; } return do_compress(in,out,thr,level); }
    if (!strcmp(cmd,"d")) { if (!out) { fprintf(stderr,"--out required\n"); return 1; } return do_decompress(in,out,thr); }
    if (!strcmp(cmd,"t")) return do_test(in,thr,level);
    return 1;
}
#endif // ACEAPEX_NO_MAIN
