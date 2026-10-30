#include <unistd.h>
// acepx4.cpp — isolated ACEPX4 flat-match analysis
// Does NOT modify any encoder/decoder code
// Reads existing .aet files and tests flat-match decoding
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <zstd.h>

// ── read helpers ──────────────────────────────────────────────────────────────
static uint32_t rv(const uint8_t* b, size_t& p, size_t lim) {
    uint32_t v=0; int s=0;
    while (p<lim){ uint8_t c=b[p++]; v|=(uint32_t)(c&0x7f)<<s; s+=7; if(!(c&0x80))break; }
    return v;
}
static uint8_t* decomp(const uint8_t* src, size_t csz, size_t& osz) {
    uint64_t h=*(const uint64_t*)src; osz=h&~(uint64_t(1)<<62);
    uint8_t* out=(uint8_t*)malloc(osz); if(!out)return nullptr;
    const size_t CHUNK=512*1024;
    if(!(h&(uint64_t(1)<<62))){ // single zstd
        ZSTD_decompress(out,osz,src+8,csz-8); return out;
    }
    const int NW=4; const uint64_t* zsz=(const uint64_t*)(src+8);
    size_t csz2=(osz+NW-1)/NW;
    const uint8_t* p0=src+8+NW*8; const uint8_t* p=p0;
    for(int t=0;t<NW;t++){
        size_t off=(size_t)t*csz2, raw=(t<NW-1)?csz2:osz-off;
        ZSTD_decompress(out+off,raw,p,(size_t)zsz[t]); p+=(size_t)zsz[t];
    }
    return out;
}
static uint8_t* decomp_fse(const uint8_t* src, size_t& osz) {
    uint64_t h=*(const uint64_t*)src; osz=h&~(uint64_t(1)<<63);
    uint8_t* out=(uint8_t*)malloc(osz); if(!out)return nullptr;
    const size_t CHUNK=512*1024;
    const uint64_t* cs=(const uint64_t*)(src+8);
    size_t nc=(osz+CHUNK-1)/CHUNK;
    const uint8_t* p=src+8+nc*8; size_t off=0;
    for(size_t i=0;i<nc;i++){
        size_t raw=std::min<size_t>(CHUNK,osz-off);
        if(cs[i]>>63){memcpy(out+off,p,raw);p+=raw;}
        else{ZSTD_decompress(out+off,raw,p,cs[i]);p+=cs[i];}
        off+=raw;
    }
    return out;
}

// ── structures ────────────────────────────────────────────────────────────────
#pragma pack(push,1)
struct AetHdr {
    char     magic[8];
    uint32_t version;
    uint64_t orig_size;
    uint32_t block_size;
    uint32_t num_blocks;
    uint8_t  xxhash[8];
    uint64_t zlit_sz,zoff_sz,zlen_sz,zcmd_sz;
};
struct BlockOffsets {
    uint64_t lit_off,off_off,len_off,cmd_off;
    uint64_t lit_sz, off_sz, len_sz, cmd_sz;
};
#pragma pack(pop)

static void copy_match(uint8_t* dst, size_t out, uint32_t dist, uint32_t len) {
    uint8_t* d=dst+out; const uint8_t* s=dst+out-dist;
    if(dist>=len){memcpy(d,s,len);return;}
    size_t done=0;
    while(done+dist<=len){memcpy(d+done,s,dist);done+=dist;}
    if(done<len)memcpy(d+done,s,len-done);
}

int main(int argc, char** argv) {
    if(argc<2){fprintf(stderr,"Usage: acepx4_test <file_or_aet>\n");return 1;}

    // ── compress with aceapex first if not .aet ──
    char aet_path[512]; bool tmp_aet=false;
    const char* inp=argv[1];
    if(strstr(inp,".aet")){
        snprintf(aet_path,sizeof(aet_path),"%s",inp);
    } else {
        snprintf(aet_path,sizeof(aet_path),"/tmp/acepx4_test_%d.aet",(int)getpid());
        tmp_aet=true;
        char cmd[1024]; snprintf(cmd,sizeof(cmd),
            "%s/aceapex c --in '%s' --out '%s' --threads 1 2>/dev/null",
            ".", inp, aet_path);
        // find aceapex binary
        snprintf(cmd,sizeof(cmd),
            "$(dirname %s)/aceapex c --in '%s' --out '%s' --threads 1 2>/dev/null",
            argv[0], inp, aet_path);
        // Use direct path
        char exe[512]; snprintf(exe,sizeof(exe),"%s","./aceapex");
        char cmd2[1024]; snprintf(cmd2,sizeof(cmd2),
            "%s c --in '%s' --out '%s' --threads 1 2>/dev/null", exe, inp, aet_path);
        if(system(cmd2)!=0){fprintf(stderr,"compress failed\n");return 1;}
    }

    // ── load .aet ──
    FILE* f=fopen(aet_path,"rb"); if(!f){perror(aet_path);return 1;}
    fseek(f,0,SEEK_END); size_t fsz=ftell(f); rewind(f);
    uint8_t* raw=(uint8_t*)malloc(fsz); fread(raw,1,fsz,f); fclose(f);

    AetHdr hdr; memcpy(&hdr,raw,sizeof(hdr));
    uint32_t nb=hdr.num_blocks;
    size_t bsz=hdr.block_size;

    fprintf(stderr,"[acepx4] file=%s blocks=%u block_size=%zu\n",inp,nb,bsz);

    // ── decompress streams ──
    const uint8_t* p=raw+sizeof(hdr)+nb*sizeof(BlockOffsets);
    BlockOffsets* boffs=(BlockOffsets*)(raw+sizeof(hdr));

    size_t lit_osz,off_osz,len_osz,cmd_osz;
    uint8_t* lit=decomp(p,hdr.zlit_sz,lit_osz); p+=hdr.zlit_sz;
    uint8_t* off_=decomp_fse(p,off_osz); p+=hdr.zoff_sz;
    uint8_t* len_=decomp_fse(p,len_osz); p+=hdr.zlen_sz;
    uint8_t* cmd_=decomp_fse(p,cmd_osz); p+=hdr.zcmd_sz;

    // ── allocate output buffers ──
    uint8_t* dstA=(uint8_t*)calloc(1,hdr.orig_size); // recon A: standard
    uint8_t* dstB=(uint8_t*)calloc(1,hdr.orig_size); // recon B: flat-single-pass

    // ── per-block stats ──
    size_t total_matches=0, flat_matches=0, rep_matches=0, non_flat=0;

    // ── origin table (local to each block) ──
    static uint32_t origin[1048576];

    for(uint32_t b=0;b<nb;b++){
        BlockOffsets& bo=boffs[b];
        size_t bstart=b*bsz;
        size_t bsize=std::min<size_t>(bsz,hdr.orig_size-bstart);

        const uint8_t* litB=lit+bo.lit_off;
        const uint8_t* offB=off_+bo.off_off;
        const uint8_t* lenB=len_+bo.len_off;
        const uint8_t* cmdB=cmd_+bo.cmd_off;

        // init origin
        for(size_t i=0;i<bsize;i++) origin[i]=0xFFFFFFFFu;
        uint32_t lit_i=0;

        // ── Recon A: standard sequential decode ──
        {
            size_t lp=0,op=0,np=0,cp=0,out=0;
            uint32_t rep[4]={1,2,4,8};
            while(out<bsize && cp<bo.cmd_sz){
                uint8_t c=cmdB[cp++];
                if(c==0xFF){rep[0]=1;rep[1]=2;rep[2]=4;rep[3]=8;continue;}
                if(c<0x80){
                    uint32_t l=c+1;
                    if(lp+l>bo.lit_sz||out+l>bsize)break;
                    memcpy(dstA+bstart+out,litB+lp,l);
                    // set origin
                    for(uint32_t fi=0;fi<l;fi++) origin[out+fi]=lit_i+fi;
                    lit_i+=l; out+=l; lp+=l;
                } else if((c&0xC0)==0x80){
                    uint32_t ri=(c>>4)&3,lv=c&0x0F;
                    if(lv==0x0F)lv+=rv(lenB,np,bo.len_sz);
                    uint32_t l=lv+6,dist=rep[ri];
                    if(ri>0){for(int i=ri;i>0;i--)rep[i]=rep[i-1];rep[0]=dist;}
                    if(!dist||out+l>bsize)break;
                    copy_match(dstA+bstart,out,dist,l);
                    for(uint32_t fi=0;fi<l;fi++) origin[out+fi]=origin[out-dist+fi];
                    rep_matches++; total_matches++;
                    out+=l;
                } else {
                    uint32_t lv=(c==0xFE)?rv(lenB,np,bo.len_sz):(uint32_t)(c&0x3F);
                    uint32_t l=lv+6,dist=rv(offB,op,bo.off_sz);
                    rep[3]=rep[2];rep[2]=rep[1];rep[1]=rep[0];rep[0]=dist;
                    if(!dist||out+l>bsize)break;
                    copy_match(dstA+bstart,out,dist,l);
                    // check flat
                    uint32_t lit_base=origin[out-dist];
                    bool flat=(lit_base!=0xFFFFFFFFu);
                    for(uint32_t fi=0;fi<l&&flat;fi++){
                        uint32_t o=origin[out-dist+fi];
                        if(o==0xFFFFFFFFu||o!=lit_base+fi)flat=false;
                    }
                    for(uint32_t fi=0;fi<l;fi++) origin[out+fi]=origin[out-dist+fi];
                    if(flat)flat_matches++; else non_flat++;
                    total_matches++;
                    out+=l;
                }
            }
        }

        // reset origin for recon B
        for(size_t i=0;i<bsize;i++) origin[i]=0xFFFFFFFFu;
        lit_i=0;

        // ── Recon B: flat single-pass ──
        {
            size_t lp=0,op=0,np=0,cp=0,out=0;
            uint32_t rep[4]={1,2,4,8};
            while(out<bsize && cp<bo.cmd_sz){
                uint8_t c=cmdB[cp++];
                if(c==0xFF){rep[0]=1;rep[1]=2;rep[2]=4;rep[3]=8;continue;}
                if(c<0x80){
                    uint32_t l=c+1;
                    if(lp+l>bo.lit_sz||out+l>bsize)break;
                    memcpy(dstB+bstart+out,litB+lp,l);
                    for(uint32_t fi=0;fi<l;fi++) origin[out+fi]=lit_i+fi;
                    lit_i+=l; out+=l; lp+=l;
                } else if((c&0xC0)==0x80){
                    uint32_t ri=(c>>4)&3,lv=c&0x0F;
                    if(lv==0x0F)lv+=rv(lenB,np,bo.len_sz);
                    uint32_t l=lv+6,dist=rep[ri];
                    if(ri>0){for(int i=ri;i>0;i--)rep[i]=rep[i-1];rep[0]=dist;}
                    if(!dist||out+l>bsize)break;
                    // rep match: always back-ref (no flat for reps yet)
                    copy_match(dstB+bstart,out,dist,l);
                    for(uint32_t fi=0;fi<l;fi++) origin[out+fi]=origin[out-dist+fi];
                    out+=l;
                } else {
                    uint32_t lv=(c==0xFE)?rv(lenB,np,bo.len_sz):(uint32_t)(c&0x3F);
                    uint32_t l=lv+6,dist=rv(offB,op,bo.off_sz);
                    rep[3]=rep[2];rep[2]=rep[1];rep[1]=rep[0];rep[0]=dist;
                    if(!dist||out+l>bsize)break;
                    // check flat
                    uint32_t lit_base=origin[out-dist];
                    bool flat=(lit_base!=0xFFFFFFFFu);
                    for(uint32_t fi=0;fi<l&&flat;fi++){
                        uint32_t o=origin[out-dist+fi];
                        if(o==0xFFFFFFFFu||o!=lit_base+fi)flat=false;
                    }
                    if(flat && lit_base+l<=bo.lit_sz){
                        memcpy(dstB+bstart+out,litB+lit_base,l);
                    } else {
                        copy_match(dstB+bstart,out,dist,l);
                    }
                    for(uint32_t fi=0;fi<l;fi++) origin[out+fi]=origin[out-dist+fi];
                    out+=l;
                }
            }
        }
    }

    // ── compare ──
    // load original
    uint8_t* orig_data=(uint8_t*)malloc(hdr.orig_size);
    FILE* fo=fopen(inp,"rb");
    size_t orig_read=fo?fread(orig_data,1,hdr.orig_size,fo):0;
    if(fo)fclose(fo);

    size_t diffA=0,diffB=0;
    for(size_t i=0;i<hdr.orig_size;i++){
        if(orig_data[i]!=dstA[i])diffA++;
        if(orig_data[i]!=dstB[i])diffB++;
    }

    printf("=== ACEPX4 Analysis: %s ===\n",inp);
    printf("Recon A (standard):  %s  (%zu diffs)\n",diffA==0?"BIT-PERFECT":"MISMATCH",diffA);
    printf("Recon B (flat):      %s  (%zu diffs)\n",diffB==0?"BIT-PERFECT":"MISMATCH",diffB);
    printf("Total matches:  %zu\n",total_matches);
    printf("  flat normal:  %zu (%.1f%%)\n",flat_matches,100.0*flat_matches/std::max<size_t>(1,total_matches));
    printf("  non-flat:     %zu (%.1f%%)\n",non_flat,100.0*non_flat/std::max<size_t>(1,total_matches));
    printf("  rep matches:  %zu (%.1f%%)\n",rep_matches,100.0*rep_matches/std::max<size_t>(1,total_matches));

    free(raw); free(lit); free(off_); free(len_); free(cmd_);
    free(dstA); free(dstB); free(orig_data);
    if(tmp_aet) remove(aet_path);
    return (diffA==0&&diffB==0)?0:1;
}
