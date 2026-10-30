#define ACEAPEX_NO_MAIN
#include "aceapex_main.cpp"

static const size_t HISTORY_SIZE = 1 * 1024 * 1024;
static const size_t CHUNK_SIZE   = 4 * 1024 * 1024;

int encode_streaming_file(const char* in_path, const char* out_path, int level) {
    FILE* fin = fopen(in_path, "rb");
    if (!fin) { fprintf(stderr,"Cannot open: %s\n",in_path); return 1; }
    fseek(fin,0,SEEK_END); size_t total_size=ftell(fin); fseek(fin,0,SEEK_SET);
    fprintf(stderr,"[*] Streaming: %s (%.2f MB)\n",in_path,total_size/1e6);
    size_t num_chunks = (total_size + CHUNK_SIZE - 1) / CHUNK_SIZE;

    FILE* fout = fopen(out_path,"wb");
    AetHeader hdr; memset(&hdr,0,sizeof(hdr));
    memcpy(hdr.magic,"ACEPX2\0\0",8);
    hdr.version=2; hdr.orig_size=total_size;
    hdr.block_size=(uint32_t)CHUNK_SIZE; hdr.num_blocks=(uint32_t)num_chunks;
    fwrite(&hdr,sizeof(hdr),1,fout);
    std::vector<BlockOffsets> boffs(num_chunks);
    memset(boffs.data(),0,num_chunks*sizeof(BlockOffsets));
    fwrite(boffs.data(),sizeof(BlockOffsets),num_chunks,fout);

    uint32_t hash_mask=(1u<<17)-1, chain_mask=(1u<<20)-1;
    ThreadHashTable* ht=(ThreadHashTable*)calloc(1,sizeof(ThreadHashTable));
    ht->pos=(int32_t*)calloc(hash_mask+1,sizeof(int32_t));
    ht->epoch=(uint32_t*)calloc(hash_mask+1,sizeof(uint32_t));
    ht->chain=(int32_t*)malloc(((size_t)chain_mask+1)*sizeof(int32_t));
    memset(ht->chain,-1,((size_t)chain_mask+1)*sizeof(int32_t));
    ht->hash_mask=hash_mask; ht->chain_mask=chain_mask;
    ht->max_attempts=(level>=2)?32:4;

    uint8_t* window=(uint8_t*)malloc(HISTORY_SIZE+CHUNK_SIZE);
    size_t history_len=0, total_read=0;
    size_t co=0,oo=0,no=0,mo=0;

    for(size_t ci=0; ci<num_chunks; ci++) {
        size_t to_read=std::min(CHUNK_SIZE,total_size-total_read);
        fread(window+history_len,1,to_read,fin);
        size_t wlen=history_len+to_read; total_read+=to_read;

        size_t buf_sz=to_read*2+65536;
        BlockResult res;
        res.lit_buf=(uint8_t*)malloc(buf_sz); res.off_buf=(uint8_t*)malloc(buf_sz);
        res.len_buf=(uint8_t*)malloc(buf_sz); res.cmd_buf=(uint8_t*)malloc(buf_sz);
        res.lit_size=res.off_size=res.len_size=res.cmd_size=res.overflow=0;
        compress_block(window,wlen,history_len,wlen,ht,&res);

        size_t zls,zos,zns,zcs;
        uint8_t* zl=lit_compress(res.lit_buf,res.lit_size,zls);
        uint8_t *zo,*zn,*zc;
        entropy_encode(res.lit_buf,res.lit_size,res.off_buf,res.off_size,
                       res.len_buf,res.len_size,res.cmd_buf,res.cmd_size,
                       zl,zls,zo,zos,zn,zns,zc,zcs);

        boffs[ci].lit_off=co; boffs[ci].lit_sz=zls;
        boffs[ci].off_off=co+zls; boffs[ci].off_sz=zos;
        boffs[ci].len_off=co+zls+zos; boffs[ci].len_sz=zns;
        boffs[ci].cmd_off=co+zls+zos+zns; boffs[ci].cmd_sz=zcs;
        co+=zls+zos+zns+zcs;
        fwrite(zl,1,zls,fout); fwrite(zo,1,zos,fout);
        fwrite(zn,1,zns,fout); fwrite(zc,1,zcs,fout);
        free(zl);free(zo);free(zn);free(zc);
        free(res.lit_buf);free(res.off_buf);free(res.len_buf);free(res.cmd_buf);

        size_t keep=std::min(HISTORY_SIZE,wlen);
        memmove(window,window+wlen-keep,keep);
        history_len=keep; ht->cur_epoch++;
        fprintf(stderr,"  Chunk %zu: %.1f MB\n",ci+1,total_read/1e6);
    }
    fclose(fin);

    hdr.zlit_sz=co; hdr.zoff_sz=oo; hdr.zlen_sz=no; hdr.zcmd_sz=mo;
    fseek(fout,0,SEEK_SET);
    fwrite(&hdr,sizeof(hdr),1,fout);
    fwrite(boffs.data(),sizeof(BlockOffsets),num_chunks,fout);
    fclose(fout);

    fprintf(stderr,"  Ratio: %.5fx\n",(double)total_size/(co+oo+no+mo));
    free(window);free(ht->pos);free(ht->epoch);free(ht->chain);free(ht);
    return 0;
}

int decode_streaming_file(const char*, const char*);

int main(int argc, char** argv) {
    const char* cmd=argc>1?argv[1]:"c";
    const char* in=nullptr; const char* out=nullptr; int level=2;
    for(int i=2;i<argc;i++) {
        if (!strcmp(argv[i],"--in")&&i+1<argc) in=argv[++i];
        else if (!strcmp(argv[i],"--out")&&i+1<argc) out=argv[++i];
    }
    if (!in||!out) { fprintf(stderr,"Usage: %s c|d --in <in> --out <out>\n",argv[0]); return 1; }
    if (!strcmp(cmd,"d")) return decode_streaming_file(in,out);
    return encode_streaming_file(in,out,level);
}

int decode_streaming_file(const char* in_path, const char* out_path) {
    FILE* fin=fopen(in_path,"rb");
    if (!fin) { fprintf(stderr,"Cannot open\n"); return 1; }
    AetHeader hdr; fread(&hdr,sizeof(hdr),1,fin);
    if (memcmp(hdr.magic,"ACEPX2\0\0",8)!=0) { fprintf(stderr,"Bad magic\n"); return 1; }
    uint32_t nb=hdr.num_blocks;
    std::vector<BlockOffsets> boffs(nb);
    fread(boffs.data(),sizeof(BlockOffsets),nb,fin);

    // Read all interleaved chunk data
    size_t data_start=ftell(fin);
    fseek(fin,0,SEEK_END); size_t data_end=ftell(fin); fseek(fin,data_start,SEEK_SET);
    size_t data_sz=data_end-data_start;
    uint8_t* data=(uint8_t*)malloc(data_sz);
    fread(data,1,data_sz,fin); fclose(fin);

    FILE* fout=fopen(out_path,"wb");
    uint8_t* dst=(uint8_t*)malloc(hdr.block_size+65536);
    size_t out_pos=0;
    size_t data_off=0;

    for(uint32_t i=0;i<nb;i++) {
        size_t zls=boffs[i].lit_sz, zos=boffs[i].off_sz;
        size_t zns=boffs[i].len_sz, zcs=boffs[i].cmd_sz;
        uint8_t* zl=data+boffs[i].lit_off;
        uint8_t* zo=data+boffs[i].off_off;
        uint8_t* zn=data+boffs[i].len_off;
        uint8_t* zc=data+boffs[i].cmd_off;

        size_t os=*(uint64_t*)zo, ns=*(uint64_t*)zn, cs=*(uint64_t*)zc;
        size_t ls=0; uint8_t* l=lit_decompress(zl,zls,ls);
        uint8_t* o=(uint8_t*)malloc(os); fse_chunked_decomp(zo,os,o);
        uint8_t* n=(uint8_t*)malloc(ns); fse_chunked_decomp(zn,ns,n);
        uint8_t* c=(uint8_t*)malloc(cs); fse_chunked_decomp(zc,cs,c);

        size_t bsize=std::min((size_t)hdr.block_size, hdr.orig_size-out_pos);
        decompress_streams(dst,bsize,l,ls,o,os,n,ns,c,cs);
        fwrite(dst,1,bsize,fout);
        out_pos+=bsize;
        free(l);free(o);free(n);free(c);
    }
    free(dst);free(data);fclose(fout);
    fprintf(stderr,"  Decoded: %zu bytes\n",out_pos);
    return 0;
}
