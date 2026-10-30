/* region_bench.c - CPU region and full-decode timing through the public one-shot API
 * (aceapex_decompress_region / aceapex_decompress). The same source links against the C99
 * decoder (c/aceapex_decode.c) or the C++ library (src/aceapex_api.cpp): both export these
 * two calls with one signature. Every region is compared with the original.
 * Build (C99): cc -O2 -Ic -o region_bench_c scripts/region_bench.c c/aceapex_decode.c -lzstd
 * Build (C++): g++ -O3 -march=native -x c scripts/region_bench.c -x c++ src/aceapex_api.cpp -Isrc -lzstd -lpthread
 * Usage: region_bench <archive.aet> <original> [N=200] [len=16384] [full reps=3]
 * Prints p50/p90 microseconds per region (random offsets, fixed seed) and the best full decode. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
int64_t aceapex_decompress(const void* src, size_t src_size, void* dst, size_t dst_capacity);
int64_t aceapex_decompress_region(const void* src, size_t src_size, void* dst, size_t dst_capacity, uint64_t offset, uint64_t length);
static uint8_t* slurp(const char* p, size_t* n){ FILE* f=fopen(p,"rb"); if(!f){ perror(p); exit(1); }
    fseek(f,0,SEEK_END); long s=ftell(f); fseek(f,0,SEEK_SET); uint8_t* b=(uint8_t*)malloc((size_t)s+1);
    if(!b||fread(b,1,(size_t)s,f)!=(size_t)s){ fprintf(stderr,"read %s\n",p); exit(1); } fclose(f); *n=(size_t)s; return b; }
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static int cmpd(const void* a, const void* b){ double x=*(const double*)a, y=*(const double*)b; return x<y?-1:x>y; }
int main(int argc, char** argv){
    if(argc<3){ fprintf(stderr,"usage: %s <archive.aet> <original> [N=200] [len=16384] [full reps=3]\n",argv[0]); return 1; }
    size_t an, on; uint8_t* a=slurp(argv[1],&an); uint8_t* o=slurp(argv[2],&on);
    int N=argc>3?atoi(argv[3]):200; size_t L=argc>4?strtoull(argv[4],0,10):16384; int R=argc>5?atoi(argv[5]):3;
    uint8_t* buf=(uint8_t*)malloc(L); double* t=(double*)malloc(sizeof(double)*N); int bad=0;
    uint64_t s=20260929;
    for(int i=0;i<N;i++){
        s=s*6364136223846793005ull+1442695040888963407ull; uint64_t off=(s>>11)%(on-L);
        double t0=now(); int64_t r=aceapex_decompress_region(a,an,buf,L,off,L); t[i]=(now()-t0)*1e6;
        if(r!=(int64_t)L || memcmp(buf,o+off,L)) bad++;
    }
    qsort(t,N,sizeof(double),cmpd);
    uint8_t* full=(uint8_t*)malloc(on); double best=1e30;
    for(int k=0;k<R;k++){ double t0=now(); int64_t r=aceapex_decompress(a,an,full,on); double d=now()-t0; if(d<best) best=d;
        if(r!=(int64_t)on || memcmp(full,o,on)) bad++; }
    printf("regions %d x %zu B: p50 %.1f us, p90 %.1f us; full decode best of %d: %.3f s (%.0f MB/s); mismatches %d\n",
        N,L,t[N/2],t[N*9/10],R,best,on/best/1e6,bad);
    return bad?2:0;
}
