/* axdec - tiny CLI over aceapex_decode.c, used by the judge and as an embedding example.
 *   axdec <archive.aet> <out>                 whole archive
 *   axdec <archive.aet> <out> <offset> <len>  one region
 *   axdec <archive.aet> <out> -r <file>       ranges: lines "offset length", concatenated
 * Exit 0 on success; error codes from aceapex_decode.h otherwise. */
#include "aceapex_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t* slurp(const char* p, size_t* n){
    FILE* f=fopen(p,"rb"); if(!f){ perror(p); exit(10); }
    fseek(f,0,SEEK_END); long L=ftell(f); fseek(f,0,SEEK_SET);
    uint8_t* b=(uint8_t*)malloc(L>0?(size_t)L:1); if(!b){ fputs("oom\n",stderr); exit(11); }
    if(L>0 && fread(b,1,(size_t)L,f)!=(size_t)L){ fputs("short read\n",stderr); exit(12); }
    fclose(f); *n=(size_t)L; return b;
}
int main(int argc, char** argv){
    if(argc<3){ fputs("usage: axdec <archive> <out> [offset len | -r ranges.txt]\n",stderr); return 1; }
    size_t n; uint8_t* a=slurp(argv[1],&n);
    FILE* o=fopen(argv[2],"wb"); if(!o){ perror(argv[2]); return 10; }
    int64_t rc;
    if(argc==3){
        int64_t sz=aceapex_decoded_size(a,n); if(sz<0){ fprintf(stderr,"bad archive (%lld)\n",(long long)sz); free(a); fclose(o); return 2; }
        uint8_t* d=(uint8_t*)malloc((size_t)sz+1); if(!d) return 11;
        rc=aceapex_decompress(a,n,d,(size_t)sz);
        if(rc>=0) fwrite(d,1,(size_t)rc,o);
        free(d);
    } else if(argc==5 && strcmp(argv[3],"-r")!=0){
        uint64_t off=strtoull(argv[3],0,10), len=strtoull(argv[4],0,10);
        uint8_t* d=(uint8_t*)malloc((size_t)len+1); if(!d) return 11;
        rc=aceapex_decompress_region(a,n,d,(size_t)len,off,len);
        if(rc>=0) fwrite(d,1,(size_t)rc,o);
        free(d);
    } else if(argc==5){
        FILE* rf=fopen(argv[4],"r"); if(!rf){ perror(argv[4]); return 10; }
        size_t cap=1024, cnt=0; aceapex_range_t* rg=(aceapex_range_t*)malloc(cap*sizeof *rg);
        unsigned long long off,len;
        while(fscanf(rf,"%llu %llu",&off,&len)==2){
            if(cnt==cap){ cap*=2; rg=(aceapex_range_t*)realloc(rg,cap*sizeof *rg); }
            rg[cnt].offset=off; rg[cnt].length=len; rg[cnt].dst=malloc((size_t)len+1); rg[cnt].written=0; cnt++;
        }
        fclose(rf);
        rc=aceapex_decompress_ranges(a,n,rg,cnt,0);
        for(size_t i=0;i<cnt;i++){ if(rg[i].written>=0) fwrite(rg[i].dst,1,(size_t)rg[i].written,o); else rc=rg[i].written; free(rg[i].dst); }
        if(rc>=0) rc=(int64_t)cnt;
        free(rg);
    } else { fputs("bad arguments\n",stderr); return 1; }
    fclose(o); free(a);
    if(rc<0){ fprintf(stderr,"decode error %lld\n",(long long)rc); return (int)(-rc)+1; }
    return 0;
}
