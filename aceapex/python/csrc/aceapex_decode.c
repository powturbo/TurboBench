/* aceapex_decode.c - standalone C99 decoder for ACEPX2 archives. See aceapex_decode.h.
 *
 * Header version 3 (the FAST profile, 2.4.0) is read through ax_fast.h: header, u64 block table, independent
 * token blocks, no entropy streams; docs/FORMAT_FAST.md. Below: header version 2, the four-stream layout.
 *
 * Archive layout (all little-endian, packed):
 *   Empty input = one AetHeader: num_blocks 0, orig_size 0, stream sizes 0, block_size nonzero.
 *   AetHeader 68 B: magic "ACEPX2\0\0", u32 version, u64 orig_size, u32 block_size,
 *                   u32 num_blocks, u8 xxh3[8], u64 zlit_sz, zoff_sz, zlen_sz, zcmd_sz
 *   BlockOffsets 64 B x num_blocks: lit_off, off_off, len_off, cmd_off, then the 4 sizes,
 *                   each a slice of the decoded lit / off / len / cmd stream
 *   zlit, zoff, zlen, zcmd: the four entropy-coded streams
 * off/len/cmd ("FSE" streams): u64 word = size (bits 0..47) | chunk/4096 (bits 48..62);
 *   u64 cs[nc] (bit 63 set = chunk stored raw); then the chunks as standard zstd frames.
 *   chunk field 0 = archive older than the field: chunk comes from FSE_CHUNK, else 512 KiB.
 * lit stream: u64 word = size | bit62 (zstd lanes) | bit61 (fixed chunks) | bit60 (tagged);
 *   bit62 clear = same layout as an FSE stream. Chunked: u64 CH, u64 zsz[NW], chunks.
 *   Legacy (bit61 clear): NW = 4 equal shares, u64 zsz[4] right after the word.
 *   Tagged chunk: byte 0 = 1 -> DNA pack (u32 nexc,h1..h4; zstd frames seq,cse,gap,val;
 *   2-bit bases MSB-first, case-mask bit -> |0x20, exceptions at cumulative gaps),
 *   any other byte -> zstd frame follows the tag.
 * Block tokens (cmd stream): 0xFF reset reps; c<0x80 literal run of c+1 bytes;
 *   0x80..0xBF rep match: rep index (c>>4)&3, len (c&15)+6, 15 -> +varint from len stream;
 *   0xC0..0xFE new match: len (c&63)+6 or 0xFE -> varint from len, distance varint from
 *   off, reps shift. Distances never leave the block (that is the whole point).
 */
#include "aceapex_decode.h"
#include "ax_env.h"      /* ax_getenv: FSE_CHUNK of legacy archives only with ACEAPEX_ENV_TUNING */
#include "ax_rans.h"   /* rANS token chunks (entry bit 62); identical copy of src/ax_rans.h */
#include "ax_lit_open.h" /* zstd-free literal chunks, modes 2/3; identical copy of src/ax_lit_open.h */
#include "ax_fast.h"     /* header version 3, the FAST profile (2.4.0): axf_open, axf_dec_block_of; identical copy of src/ax_fast.h */
#include <stdlib.h>
#include <string.h>
#include <zstd.h>

typedef struct { uint64_t lit_off, off_off, len_off, cmd_off, lit_sz, off_sz, len_sz, cmd_sz; } BO;
typedef struct {
    uint64_t orig; uint32_t bs, nb; const uint8_t* bo;   /* table bytes, 4-aligned: read via memcpy */
    const uint8_t *zl, *zo, *zn, *zc; uint64_t zls, zos, zns, zcs;
    int fast; axf_arc_t fa;                               /* header version 3: the FAST profile, no streams */
} Arc;
static uint64_t rd64(const uint8_t* p){ uint64_t v; memcpy(&v,p,8); return v; }
static uint32_t rd32(const uint8_t* p){ uint32_t v; memcpy(&v,p,4); return v; }
static BO bo_at(const uint8_t* t, size_t b){ BO o; memcpy(&o,t+64*b,64); return o; }
static int zdec(ZSTD_DCtx* x, void* d, size_t raw, const void* s, size_t n){
    size_t r = x ? ZSTD_decompressDCtx(x,d,raw,s,n) : ZSTD_decompress(d,raw,s,n); return !ZSTD_isError(r) && r==raw; }

/* ---- archive header / bounds ------------------------------------------------------- */
static int open_arc(const void* src, size_t n, Arc* a){
    const uint8_t* p=(const uint8_t*)src;
    if(!src || n<68 || memcmp(p,"ACEPX2\0\0",8)) return ACEAPEX_ERR_DATA;
    a->fast=0;
    if(rd32(p+8)==AXF_VERSION){ /* FAST profile: header + block table + blocks; validated by axf_open */
        if(axf_open(p,n,&a->fa)) return ACEAPEX_ERR_DATA;
        a->fast=1; a->orig=a->fa.orig; a->bs=a->fa.bs; a->nb=a->fa.nb; a->bo=0;
        a->zl=a->zo=a->zn=a->zc=0; a->zls=a->zos=a->zns=a->zcs=0; return ACEAPEX_OK; }
    if(rd32(p+8)!=2) return ACEAPEX_ERR_DATA;             /* version 2: the four-stream layout below */
    a->orig=rd64(p+12); a->bs=rd32(p+20); a->nb=rd32(p+24);
    a->zls=rd64(p+36); a->zos=rd64(p+44); a->zns=rd64(p+52); a->zcs=rd64(p+60);
    if(!a->bs) return ACEAPEX_ERR_DATA;
    if(!a->nb){ /* empty archive: one header, orig 0, no streams */
        if(a->orig || a->zls || a->zos || a->zns || a->zcs) return ACEAPEX_ERR_DATA; }
    else if((uint64_t)a->nb*a->bs < a->orig) return ACEAPEX_ERR_DATA;
    uint64_t need=68+(uint64_t)a->nb*64+a->zls+a->zos+a->zns+a->zcs;
    if(need>n) return ACEAPEX_ERR_DATA;
    a->bo=p+68; a->zl=p+68+(size_t)a->nb*64;
    a->zo=a->zl+a->zls; a->zn=a->zo+a->zos; a->zc=a->zn+a->zns;
    return ACEAPEX_OK;
}

/* ---- a stream cursor: unpacks the chunks covering [from,to) and caches the window --- */
typedef struct {
    const uint8_t* z; size_t zsz;      /* the compressed stream */
    uint64_t size, chunk; size_t nc;   /* decoded size, chunk size, chunk count */
    int kind;                          /* 0 = FSE layout, 1 = lit chunked, 2 = lit legacy */
    int tagged; const uint8_t* tab; const uint8_t* data;   /* size table, first chunk */
    size_t* coff;                      /* compressed offset of each chunk (prefix sums) */
    uint8_t* win; size_t wcap;         /* assembly buffer for slices that span chunks */
    struct { size_t idx; uint8_t* buf; size_t raw; unsigned age; } cc[4];   /* last decoded chunks */
    unsigned tick;
    ZSTD_DCtx* dctx;                   /* reused across chunks: one workspace per cursor */
} Cur;
static uint64_t fse_default_chunk(void){
    const char* e=ax_getenv("FSE_CHUNK"); uint64_t v=e?strtoull(e,0,10):0;
    if(v>=4096){ v&=~(uint64_t)4095; if(v>((uint64_t)16<<20)) v=(uint64_t)16<<20; return v; }
    return 512*1024;
}
/* compressed size of chunk i (the table was checked by cur_open) */
static size_t cur_csize(const Cur* c, size_t i){
    uint64_t e=rd64(c->tab+8*i);
    if(c->kind){ return (size_t)(e&~((uint64_t)1<<63)); }
    if(e>>63){ uint64_t o=(uint64_t)i*c->chunk, raw=c->chunk; if(o>=c->size) raw=0; else if(o+raw>c->size) raw=c->size-o; return (size_t)raw; }
    return (size_t)(e&(((uint64_t)1<<48)-1));
}
/* compressed offset of chunk i: from the index when there is one, else a straight sum of the sizes before it */
static size_t cur_coff(const Cur* c, size_t i){
    if(c->coff) return c->coff[i];
    uint64_t p=0;
    if(c->kind){ for(size_t k=0;k<i;k++) p+=rd64(c->tab+8*k)&~((uint64_t)1<<63); return (size_t)p; }   /* vectorised */
    /* FSE layout: chunks before i are full (raw size = chunk) */
    for(size_t k=0;k<i;k++){ uint64_t e=rd64(c->tab+8*k); p+=(e>>63)?c->chunk:(e&(((uint64_t)1<<48)-1)); }
    return (size_t)p;
}
/* index = 1: prefix sums of the chunk offsets (a decoder that serves many chunks: full decode, a handle, a range
 * batch); 0: none, a chunk's offset is summed when it is needed (one region per call reads 1-3 chunks of a table that
 * has 12 800 entries on T2T - building the index was a fifth of such a call). The table check is the same either way. */
static int cur_open(Cur* c, const uint8_t* z, size_t zsz, int is_lit, int index){
    memset(c,0,sizeof *c); c->z=z; c->zsz=zsz; for(int k=0;k<4;k++) c->cc[k].idx=(size_t)-1;
    if(zsz<8){ c->size=0; c->nc=0; return ACEAPEX_OK; }   /* empty stream (tiny inputs) */
    uint64_t h=rd64(z);
    if(is_lit && (h&((uint64_t)1<<62))){
        int chunked=(h>>61)&1; c->tagged=(h>>60)&1; c->kind=chunked?1:2;
        c->size=h&~(((uint64_t)1<<62)|((uint64_t)1<<61)|((uint64_t)1<<60));
        if(chunked){ if(zsz<16) return ACEAPEX_ERR_DATA; c->chunk=rd64(z+8); c->tab=z+16; }
        else { c->chunk=(c->size+3)/4; c->tab=z+8; }
        if(!c->chunk) return ACEAPEX_ERR_DATA;
        c->nc=chunked?(size_t)((c->size+c->chunk-1)/c->chunk):4;
    } else {
        c->kind=0; c->size=h&(((uint64_t)1<<48)-1); uint64_t f=(h>>48)&0x7FFF;
        c->chunk=f?f<<12:fse_default_chunk(); c->tab=z+8;
        c->nc=(size_t)((c->size+c->chunk-1)/c->chunk);
    }
    if(c->nc>(zsz-(size_t)(c->tab-z))/8) return ACEAPEX_ERR_DATA;
    c->data=c->tab+c->nc*8;
    const size_t room=zsz-(size_t)(c->data-z);             /* bytes the chunk bodies may use */
    /* check: every entry well-formed and the sum of the sizes inside the stream. Sizes are checked <= room and
     * summed in runs of 4096 (4096 x 2^48 < 2^64), each run added to a checked total; a stream of 2^48 bytes or
     * more takes the entry-by-entry path. Same verdict as summing one by one with a bound after each entry. */
    if(zsz>=((size_t)1<<48)){
        size_t p=0;
        for(size_t i=0;i<c->nc;i++){ uint64_t e=rd64(c->tab+8*i);
            if(!c->kind && (((e>>48)&0x3FFF) || ((e>>63) && ((e>>62)&1)))) return ACEAPEX_ERR_DATA;
            size_t n=cur_csize(c,i); if(n>room-p) return ACEAPEX_ERR_DATA; p+=n; }
    } else {
        uint64_t sum=0;
        for(size_t t0=0;t0<c->nc;t0+=4096){ const size_t t1=c->nc-t0<4096?c->nc:t0+4096; uint64_t mx=0, s=0, bad=0;
            if(c->kind) for(size_t i=t0;i<t1;i++){ uint64_t n=rd64(c->tab+8*i)&~((uint64_t)1<<63); mx=n>mx?n:mx; s+=n; }
            else for(size_t i=t0;i<t1;i++){ uint64_t e=rd64(c->tab+8*i);
                /* bits 48..61 reserved; raw (63) and rANS (62) exclusive */
                bad|=((e>>48)&0x3FFF)|((e>>63)&(e>>62)&1);
                uint64_t o=(uint64_t)i*c->chunk, raw=o>=c->size?0:(o+c->chunk>c->size?c->size-o:c->chunk);
                uint64_t n=(e>>63)?raw:(e&(((uint64_t)1<<48)-1)); mx=n>mx?n:mx; s+=n; }
            if(bad || mx>room || s>room-sum) return ACEAPEX_ERR_DATA;
            sum+=s; }
    }
    if(index && c->nc){
        c->coff=(size_t*)malloc((c->nc+1)*sizeof(size_t)); if(!c->coff) return ACEAPEX_ERR_MEMORY;
        size_t p=0; for(size_t i=0;i<c->nc;i++){ c->coff[i]=p; p+=cur_csize(c,i); } c->coff[c->nc]=p;
    }
    return ACEAPEX_OK;
}
static void cur_close(Cur* c){ free(c->coff); free(c->win); c->coff=0; c->win=0;
    for(int k=0;k<4;k++){ free(c->cc[k].buf); c->cc[k].buf=0; }
    if(c->dctx){ ZSTD_freeDCtx(c->dctx); c->dctx=0; } }

static int dna_unpack(ZSTD_DCtx* x, const uint8_t* s, size_t n, uint8_t* dst, size_t raw){
    if(n<20) return 0;
    uint32_t nexc=rd32(s),h1=rd32(s+4),h2=rd32(s+8),h3=rd32(s+12),h4=rd32(s+16);
    if(20+(size_t)h1+h2+h3+h4>n) return 0;
    size_t np=(raw+3)/4, nq=(raw+7)/8; const uint8_t* p=s+20; int ok=1;
    uint8_t* seq=(uint8_t*)malloc(np+1); uint8_t* cse=(uint8_t*)malloc(nq+1);
    uint32_t* gap=(uint32_t*)malloc((size_t)nexc*4+4); uint8_t* val=(uint8_t*)malloc((size_t)nexc+1);
    if(!seq||!cse||!gap||!val){ ok=0; goto out; }
    ok = zdec(x,seq,np,p,h1); p+=h1;
    ok = ok && zdec(x,cse,nq,p,h2); p+=h2;
    if(h3){ ok = ok && zdec(x,gap,(size_t)nexc*4,p,h3); } else memset(gap,0,(size_t)nexc*4); p+=h3;
    if(h4){ ok = ok && zdec(x,val,nexc,p,h4); } else memset(val,0,nexc);
    if(!ok) goto out;
    { /* bases: one packed byte -> four ASCII bases through a 256-entry u32 table, one store each */
      static uint32_t T4[256]; static int ready=0;
      if(!ready){ for(int v=0;v<256;v++){ uint8_t q[4]={(uint8_t)"ACGT"[(v>>6)&3],(uint8_t)"ACGT"[(v>>4)&3],(uint8_t)"ACGT"[(v>>2)&3],(uint8_t)"ACGT"[v&3]}; uint32_t w; memcpy(&w,q,4); T4[v]=w; } ready=1; }
      size_t full=raw>>2; uint32_t* d32=(uint32_t*)dst;                 /* dst comes from malloc: aligned */
      for(size_t k=0;k<full;k++) d32[k]=T4[seq[k]];
      for(size_t i=full<<2;i<raw;i++) dst[i]=(uint8_t)"ACGT"[(seq[i>>2]>>(6-2*(i&3)))&3]; }
    { /* case: one mask byte -> eight bytes of 0x20/0x00 through a 256-entry u64 table, one OR each */
      static uint64_t M8[256]; static int mready=0;
      if(!mready){ for(int v=0;v<256;v++){ uint8_t q[8]; for(int j=0;j<8;j++) q[j]=(v&(0x80>>j))?0x20:0; uint64_t w; memcpy(&w,q,8); M8[v]=w; } mready=1; }
      size_t fullq=raw>>3; uint64_t* d64=(uint64_t*)dst;
      for(size_t k=0;k<fullq;k++){ uint8_t m=cse[k]; if(m) d64[k]|=M8[m]; }
      if(fullq<nq){ uint8_t m=cse[fullq]; size_t b=fullq<<3; for(size_t j=0;b+j<raw;j++) if(m&(0x80>>j)) dst[b+j]|=0x20; } }
    { uint64_t pos=0; for(uint32_t k=0;k<nexc;k++){ pos+=gap[k]; if(pos<raw) dst[pos]=val[k]; } }
out:
    free(seq); free(cse); free(gap); free(val); return ok;
}
/* decode chunk i into dst (raw bytes) */
static int cur_chunk(Cur* c, size_t i, uint8_t* dst, size_t raw){
    if(!c->dctx) c->dctx=ZSTD_createDCtx();   /* NULL -> fall back to one-shot decompress */
    uint64_t e=rd64(c->tab+8*i); const uint8_t* p=c->data+cur_coff(c,i); size_t n=cur_csize(c,i);
    if(c->kind==0){ if(e>>63){ memcpy(dst,p,raw); return 1; }
        if((e>>62)&1) return axr_decode(p,n,dst,raw)==0;
        return zdec(c->dctx,dst,raw,p,n); }
    if(!n) return raw==0;
    if(!c->tagged) return zdec(c->dctx,dst,raw,p,n);
    if(p[0]==1) return dna_unpack(c->dctx,p+1,n-1,dst,raw);
    if(p[0]==2) return axo_dna_decode(p+1,n-1,dst,raw)==0;      /* open DNA pack (ADR-019) */
    if(p[0]==3) return axo_piece_decode(p+1,n-1,dst,raw)==0;    /* open plain */
    return zdec(c->dctx,dst,raw,p+1,n-1);
}
/* decoded chunk i, from the 4-entry cache or freshly decoded into the least recently used slot */
static const uint8_t* cur_chunk_get(Cur* c, size_t i, size_t* raw_out, int* err){
    uint64_t o=(uint64_t)i*c->chunk; size_t raw=(size_t)((o+c->chunk<=c->size)?c->chunk:c->size-o); *raw_out=raw;
    int slot=-1; unsigned oldest=~0u; int victim=0;
    for(int k=0;k<4;k++){ if(c->cc[k].idx==i){ slot=k; break; } if(c->cc[k].age<oldest){ oldest=c->cc[k].age; victim=k; } }
    if(slot<0){ slot=victim; uint8_t* b=(uint8_t*)realloc(c->cc[slot].buf,raw+64); if(!b){ *err=ACEAPEX_ERR_MEMORY; return 0; }
        c->cc[slot].buf=b; c->cc[slot].idx=(size_t)-1;
        if(!cur_chunk(c,i,b,raw)){ *err=ACEAPEX_ERR_DATA; return 0; }
        c->cc[slot].idx=i; c->cc[slot].raw=raw; }
    c->cc[slot].age=++c->tick; return c->cc[slot].buf;
}
/* make [from,to) of the decoded stream available; returns pointer to byte 'from'. A slice inside one
 * chunk is served from that chunk's buffer; a slice spanning chunks is assembled by copying. */
static const uint8_t* cur_get(Cur* c, uint64_t from, uint64_t to, int* err){
    if(to<=from) return c->z;                       /* empty slice: any valid pointer */
    if(to>c->size){ *err=ACEAPEX_ERR_DATA; return 0; }
    size_t c0=(size_t)(from/c->chunk), c1=(size_t)((to-1)/c->chunk), raw;
    if(c0==c1){ const uint8_t* b=cur_chunk_get(c,c0,&raw,err); return b ? b+(from-(uint64_t)c0*c->chunk) : 0; }
    size_t need=(size_t)(to-from); if(need+64>c->wcap){ uint8_t* w=(uint8_t*)realloc(c->win,need+64); if(!w){ *err=ACEAPEX_ERR_MEMORY; return 0; } c->win=w; c->wcap=need+64; }
    size_t done=0;
    for(size_t i=c0;i<=c1;i++){ const uint8_t* b=cur_chunk_get(c,i,&raw,err); if(!b) return 0;
        uint64_t o=(uint64_t)i*c->chunk; uint64_t lo = from>o ? from : o; uint64_t hi = to<o+raw ? to : o+raw;
        if(hi>lo){ memcpy(c->win+done,b+(lo-o),(size_t)(hi-lo)); done+=(size_t)(hi-lo); } }
    return c->win;
}

/* ---- one block: literals + matches ----------------------------------------------- */
static uint32_t varint(const uint8_t* b, size_t* p, size_t lim){
    uint32_t v=0, sh=0;
    while(*p<lim){ uint8_t x=b[(*p)++]; v|=(uint32_t)(x&0x7F)<<sh; if(!(x&0x80)) break; sh+=7; if(sh>28) break; }
    return v;
}
static void copy_match(uint8_t* d, size_t out, uint32_t dist, uint32_t len){
    uint8_t* t=d+out; const uint8_t* s=t-dist;
    if(dist>=len){ memcpy(t,s,len); return; }
    if(dist==1){ memset(t,s[0],len); return; }
    uint32_t done=0; while(done+dist<=len){ memcpy(t+done,s,dist); done+=dist; }
    if(done<len) memcpy(t+done,s,len-done);
}
static int decode_block(uint8_t* dst, size_t dsz, const uint8_t* lit, size_t ls,
                        const uint8_t* off, size_t os, const uint8_t* len, size_t ns,
                        const uint8_t* cmd, size_t cs){
    size_t lp=0, op=0, np=0, cp=0, out=0; uint32_t rep[4]={1,2,4,8};
    while(out<dsz && cp<cs){
        uint8_t c=cmd[cp++];
        if(c==0xFF){ rep[0]=1; rep[1]=2; rep[2]=4; rep[3]=8; continue; }
        if(c<0x80){ uint32_t l=c+1; if(lp+l>ls||out+l>dsz) return 0; memcpy(dst+out,lit+lp,l); out+=l; lp+=l; }
        else if((c&0xC0)==0x80){
            uint32_t ri=(c>>4)&3, lv=c&15; if(lv==15) lv+=varint(len,&np,ns);
            uint32_t l=lv+6, dist=rep[ri];
            if(ri){ for(int i=ri;i>0;i--) rep[i]=rep[i-1]; rep[0]=dist; }
            if(!dist||dist>out||out+l>dsz) return 0;
            copy_match(dst,out,dist,l); out+=l;
        } else {
            uint32_t lv=(c==0xFE)?varint(len,&np,ns):(uint32_t)(c&0x3F);
            uint32_t l=lv+6, dist=varint(off,&op,os);
            rep[3]=rep[2]; rep[2]=rep[1]; rep[1]=rep[0]; rep[0]=dist;
            if(!dist||dist>out||out+l>dsz) return 0;
            copy_match(dst,out,dist,l); out+=l;
        }
    }
    return out==dsz;
}

/* ---- a decoder over the four cursors -------------------------------------------- */
typedef struct { Arc a; Cur l,o,n,c; } Dec;
static int dec_open(Dec* d, const void* src, size_t n, int index){
    int r=open_arc(src,n,&d->a); if(r) return r;
    memset(&d->l,0,sizeof(Cur)); memset(&d->o,0,sizeof(Cur)); memset(&d->n,0,sizeof(Cur)); memset(&d->c,0,sizeof(Cur));
    if(d->a.fast) return ACEAPEX_OK;                      /* no stream cursors in the FAST profile */
    if((r=cur_open(&d->l,d->a.zl,(size_t)d->a.zls,1,index))) return r;
    if((r=cur_open(&d->o,d->a.zo,(size_t)d->a.zos,0,index))) return r;
    if((r=cur_open(&d->n,d->a.zn,(size_t)d->a.zns,0,index))) return r;
    if((r=cur_open(&d->c,d->a.zc,(size_t)d->a.zcs,0,index))) return r;
    return ACEAPEX_OK;
}
static void dec_close(Dec* d){ cur_close(&d->l); cur_close(&d->o); cur_close(&d->n); cur_close(&d->c); }
static size_t block_size_of(const Dec* d, size_t b){
    uint64_t st=(uint64_t)b*d->a.bs; return (size_t)(st>=d->a.orig?0:(d->a.orig-st<d->a.bs?d->a.orig-st:d->a.bs)); }
static int dec_block(Dec* d, size_t b, uint8_t* dst){
    if(b>=d->a.nb) return ACEAPEX_ERR_DATA;
    if(d->a.fast) return axf_dec_block_of(&d->a.fa,(uint32_t)b,dst) ? ACEAPEX_OK : ACEAPEX_ERR_DATA;
    BO bov=bo_at(d->a.bo,b); const BO* bo=&bov; size_t sz=block_size_of(d,b); int err=0;
    const uint8_t* L=cur_get(&d->l,bo->lit_off,bo->lit_off+bo->lit_sz,&err); if(err) return err;
    const uint8_t* O=cur_get(&d->o,bo->off_off,bo->off_off+bo->off_sz,&err); if(err) return err;
    const uint8_t* N=cur_get(&d->n,bo->len_off,bo->len_off+bo->len_sz,&err); if(err) return err;
    const uint8_t* C=cur_get(&d->c,bo->cmd_off,bo->cmd_off+bo->cmd_sz,&err); if(err) return err;
    return decode_block(dst,sz,L,(size_t)bo->lit_sz,O,(size_t)bo->off_sz,N,(size_t)bo->len_sz,C,(size_t)bo->cmd_sz)
           ? ACEAPEX_OK : ACEAPEX_ERR_DATA;
}

/* ---- public API ------------------------------------------------------------------- */
int64_t aceapex_decoded_size(const void* src, size_t n){ Arc a; int r=open_arc(src,n,&a); return r?r:(int64_t)a.orig; }

int64_t aceapex_decompress(const void* src, size_t n, void* dst, size_t cap){
    Dec d; int r=dec_open(&d,src,n,1); if(r){ dec_close(&d); return r; }
    if(cap<d.a.orig){ dec_close(&d); return ACEAPEX_ERR_BUFFER; }
    for(size_t b=0;b<d.a.nb && !r;b++) r=dec_block(&d,b,(uint8_t*)dst+(size_t)b*d.a.bs);
    dec_close(&d); return r?r:(int64_t)d.a.orig;
}

/* ranges are served from a block cache: each distinct block decoded once */
typedef struct { size_t b; uint8_t* buf; } Cached;
static int64_t serve(Dec* d, aceapex_range_t* rg, size_t count){
    uint8_t* blk=(uint8_t*)malloc(d->a.bs+64); if(!blk) return ACEAPEX_ERR_MEMORY;
    size_t cur=(size_t)-1; int64_t okn=0;
    /* order of service: by first block, so a block is decoded once for consecutive users */
    size_t* order=(size_t*)malloc(count*sizeof(size_t)); if(!order){ free(blk); return ACEAPEX_ERR_MEMORY; }
    for(size_t i=0;i<count;i++) order[i]=i;
    /* insertion sort by offset is O(n^2) for adversarial input; a simple merge sort instead */
    { size_t* tmp=(size_t*)malloc(count*sizeof(size_t)); if(!tmp){ free(order); free(blk); return ACEAPEX_ERR_MEMORY; }
      for(size_t w=1;w<count;w*=2) for(size_t lo=0;lo<count;lo+=2*w){
        size_t mid=lo+w<count?lo+w:count, hi=lo+2*w<count?lo+2*w:count, i=lo,j=mid,k=lo;
        while(i<mid&&j<hi) tmp[k++]= rg[order[i]].offset<=rg[order[j]].offset ? order[i++] : order[j++];
        while(i<mid) tmp[k++]=order[i++];
        while(j<hi) tmp[k++]=order[j++];
        memcpy(order+lo,tmp+lo,(hi-lo)*sizeof(size_t)); }
      free(tmp); }
    for(size_t oi=0;oi<count;oi++){
        aceapex_range_t* r=&rg[order[oi]];
        if(r->length==0){ r->written=0; okn++; continue; }
        if(!r->dst || r->offset>d->a.orig || r->length>d->a.orig-r->offset){ r->written=ACEAPEX_ERR_DATA; continue; }
        uint64_t pos=r->offset, end=r->offset+r->length; uint8_t* out=(uint8_t*)r->dst; int64_t st=ACEAPEX_OK;
        while(pos<end){
            size_t b=(size_t)(pos/d->a.bs);
            if(b!=cur){ int e=dec_block(d,b,blk); if(e){ st=e; break; } cur=b; }
            uint64_t bst=(uint64_t)b*d->a.bs, take=bst+d->a.bs-pos; if(take>end-pos) take=end-pos;
            memcpy(out,blk+(pos-bst),(size_t)take); out+=take; pos+=take;
        }
        r->written= st? st : (int64_t)r->length; if(!st) okn++;
    }
    free(order); free(blk); return okn;
}
int64_t aceapex_decompress_region(const void* src, size_t n, void* dst, size_t cap, uint64_t off, uint64_t len){
    if(len==0) return 0;
    if(cap<len) return ACEAPEX_ERR_BUFFER;
    Dec d; int r=dec_open(&d,src,n,0); if(r){ dec_close(&d); return r; }   /* one region: no chunk index */
    aceapex_range_t rg={off,len,dst,0}; int64_t k=serve(&d,&rg,1); dec_close(&d);
    if(k<0) return k;
    return rg.written;
}
int64_t aceapex_decompress_ranges(const void* src, size_t n, aceapex_range_t* rg, size_t count, int threads){
    (void)threads;
    Dec d; int r=dec_open(&d,src,n,count>1); if(r){ dec_close(&d); return r; }
    int64_t k=serve(&d,rg,count); dec_close(&d); return k;
}

/* ---- persistent handle ------------------------------------------------------------ */
struct aceapex_dec { Dec d; uint8_t* blk; size_t cur; };
aceapex_dec_t* aceapex_dec_open(const void* src, size_t n){
    aceapex_dec_t* h=(aceapex_dec_t*)calloc(1,sizeof *h); if(!h) return 0;
    if(dec_open(&h->d,src,n,1)){ dec_close(&h->d); free(h); return 0; }
    h->blk=(uint8_t*)malloc((size_t)h->d.a.bs+64); h->cur=(size_t)-1;
    if(!h->blk){ dec_close(&h->d); free(h); return 0; }
    return h;
}
int64_t aceapex_dec_size(const aceapex_dec_t* h){ return h ? (int64_t)h->d.a.orig : ACEAPEX_ERR_DATA; }
void aceapex_dec_close(aceapex_dec_t* h){ if(!h) return; dec_close(&h->d); free(h->blk); free(h); }
/* serve from the handle's block cache: a range that stays in the last decoded block costs no decode */
static int64_t serve_h(aceapex_dec_t* h, aceapex_range_t* rg, size_t count){
    Dec* d=&h->d; int64_t okn=0;
    for(size_t i=0;i<count;i++){
        aceapex_range_t* r=&rg[i];
        if(r->length==0){ r->written=0; okn++; continue; }
        if(!r->dst || r->offset>d->a.orig || r->length>d->a.orig-r->offset){ r->written=ACEAPEX_ERR_DATA; continue; }
        uint64_t pos=r->offset, end=r->offset+r->length; uint8_t* out=(uint8_t*)r->dst; int64_t st=ACEAPEX_OK;
        while(pos<end){
            size_t b=(size_t)(pos/d->a.bs);
            if(b!=h->cur){ h->cur=(size_t)-1; int e=dec_block(d,b,h->blk); if(e){ st=e; break; } h->cur=b; }
            uint64_t bst=(uint64_t)b*d->a.bs, take=bst+d->a.bs-pos; if(take>end-pos) take=end-pos;
            memcpy(out,h->blk+(pos-bst),(size_t)take); out+=take; pos+=take;
        }
        r->written= st? st : (int64_t)r->length; if(!st) okn++;
    }
    return okn;
}
int64_t aceapex_dec_region(aceapex_dec_t* h, void* dst, size_t cap, uint64_t off, uint64_t len){
    if(!h) return ACEAPEX_ERR_DATA;
    if(len==0) return 0;
    if(cap<len) return ACEAPEX_ERR_BUFFER;
    aceapex_range_t rg={off,len,dst,0}; int64_t k=serve_h(h,&rg,1); if(k<0) return k;
    return rg.written;
}
int64_t aceapex_dec_ranges(aceapex_dec_t* h, aceapex_range_t* rg, size_t count){
    if(!h||(!rg&&count)) return ACEAPEX_ERR_DATA;
    return serve_h(h,rg,count);
}
