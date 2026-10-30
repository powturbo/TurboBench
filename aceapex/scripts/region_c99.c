/* region_c99.c - one region per call through the C99 decoder (c/aceapex_decode.c), one thread, archive in memory:
 * the one-shot call aceapex_decompress_region (tables parsed per call) and the persistent handle aceapex_dec_region
 * (aceapex_dec_open once; what the Python reader uses). Regions: a samtools-style list (name:start-end, 1-based;
 * `gpu_h100_tests ra` writes <archive>.regions.txt, seed 20261002), byte offsets from the FASTA's .fai, every result
 * compared with the FASTA. p50 / p99 / mean in microseconds, file order, 200 warm-up calls per row.
 * Usage: region_c99 <archive.aet> <fasta> <regions.txt>      Last line: RCROW <tab> n one-shot p50/p99 handle p50/p99 ok|FAILED
 * Build: cc -O3 -march=native -std=c99 -Ic -o region_c99 scripts/region_c99.c c/aceapex_decode.c -lzstd */
#define _POSIX_C_SOURCE 199309L
#include "aceapex_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static uint8_t* slurp(const char* p, size_t* n){ FILE* f=fopen(p,"rb"); if(!f){ perror(p); exit(1); }
    fseek(f,0,SEEK_END); long s=ftell(f); fseek(f,0,SEEK_SET); uint8_t* b=(uint8_t*)malloc((size_t)s+1);
    if(!b||fread(b,1,(size_t)s,f)!=(size_t)s){ fprintf(stderr,"read %s\n",p); exit(1); } fclose(f); *n=(size_t)s; return b; }
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static int cmpd(const void* a, const void* b){ double x=*(const double*)a, y=*(const double*)b; return x<y?-1:x>y; }
typedef struct { char name[256]; uint64_t len, off, lb, lw; } Fai;
typedef struct { uint64_t lo, n; } Q;
static double pct(double* v, int n, double q){ qsort(v,(size_t)n,sizeof(double),cmpd); int i=(int)(q*n); if(i>=n) i=n-1; return v[i]; }
int main(int argc, char** argv){
    if(argc<4){ fprintf(stderr,"usage: %s <archive.aet> <fasta> <regions.txt>\n",argv[0]); return 1; }
    size_t an, fn; uint8_t* a=slurp(argv[1],&an); uint8_t* f=slurp(argv[2],&fn);
    Fai fx[4096]; int nf=0; { char p[4096]; snprintf(p,sizeof p,"%s.fai",argv[2]); FILE* x=fopen(p,"r"); if(!x){ perror(p); return 1; }
        unsigned long long l,o,b,w; while(nf<4096 && fscanf(x,"%255s %llu %llu %llu %llu",fx[nf].name,&l,&o,&b,&w)==5){ fx[nf].len=l; fx[nf].off=o; fx[nf].lb=b; fx[nf].lw=w; nf++; } fclose(x); }
    Q* q=(Q*)malloc(sizeof(Q)*200000); int N=0; uint64_t maxn=0;
    { FILE* x=fopen(argv[3],"r"); if(!x){ perror(argv[3]); return 1; } char ln[4096];
      while(N<200000 && fgets(ln,sizeof ln,x)){ char* c=strrchr(ln,':'); if(!c) continue; *c=0; unsigned long long s=0,e=0; if(sscanf(c+1,"%llu-%llu",&s,&e)!=2) continue;
        int k=0; while(k<nf && strcmp(fx[k].name,ln)) k++; if(k==nf){ fprintf(stderr,"unknown %s\n",ln); return 1; }
        uint64_t b0=s-1, b1=e-1, lo=fx[k].off+b0/fx[k].lb*fx[k].lw+b0%fx[k].lb, hi=fx[k].off+b1/fx[k].lb*fx[k].lw+b1%fx[k].lb+1;
        q[N].lo=lo; q[N].n=hi-lo; if(hi-lo>maxn) maxn=hi-lo; N++; } fclose(x); }
    uint8_t* buf=(uint8_t*)malloc(maxn+64); double* t1=(double*)malloc(sizeof(double)*N); double* t2=(double*)malloc(sizeof(double)*N);
    int bad1=0, bad2=0; double m1=0, m2=0;
    for(int i=0;i<N&&i<200;i++) aceapex_decompress_region(a,an,buf,maxn+64,q[i].lo,q[i].n);
    for(int i=0;i<N;i++){ double t0=now(); int64_t r=aceapex_decompress_region(a,an,buf,maxn+64,q[i].lo,q[i].n); t1[i]=now()-t0; m1+=t1[i];
        if(r!=(int64_t)q[i].n || memcmp(buf,f+q[i].lo,q[i].n)) bad1++; }
    aceapex_dec_t* h=aceapex_dec_open(a,an); if(!h){ fprintf(stderr,"dec_open failed\n"); return 2; }
    for(int i=0;i<N&&i<200;i++) aceapex_dec_region(h,buf,maxn+64,q[i].lo,q[i].n);
    for(int i=0;i<N;i++){ double t0=now(); int64_t r=aceapex_dec_region(h,buf,maxn+64,q[i].lo,q[i].n); t2[i]=now()-t0; m2+=t2[i];
        if(r!=(int64_t)q[i].n || memcmp(buf,f+q[i].lo,q[i].n)) bad2++; }
    aceapex_dec_close(h);
    printf("[rc] %d regions, archive %zu B in memory\n",N,an);
    printf("[rc] C99 aceapex_decompress_region (one-shot)  p50 %7.1f us  p99 %7.1f us  mean %7.1f us  %s\n",pct(t1,N,.5)*1e6,pct(t1,N,.99)*1e6,m1/N*1e6,bad1?"DIFFERS":"== FASTA");
    printf("[rc] C99 aceapex_dec_region (handle)          p50 %7.1f us  p99 %7.1f us  mean %7.1f us  %s\n",pct(t2,N,.5)*1e6,pct(t2,N,.99)*1e6,m2/N*1e6,bad2?"DIFFERS":"== FASTA");
    printf("RCROW\t%d\t%.1f/%.1f\t%.1f/%.1f\t%s\n",N,pct(t1,N,.5)*1e6,pct(t1,N,.99)*1e6,pct(t2,N,.5)*1e6,pct(t2,N,.99)*1e6,bad1||bad2?"FAILED":"ok");
    return bad1||bad2?5:0;
}
