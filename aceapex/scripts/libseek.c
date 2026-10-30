#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "aceapex.h"

static double ms(struct timespec a, struct timespec b){
    return (b.tv_sec-a.tv_sec)*1e3 + (b.tv_nsec-a.tv_nsec)/1e6;
}
static int cmpd(const void*x,const void*y){
    double a=*(const double*)x,b=*(const double*)y; return a<b?-1:a>b;
}
int main(int argc,char**argv){
    FILE* f=fopen(argv[1],"rb");
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    void* arc=malloc(n); fread(arc,1,n,f); fclose(f);       // архив в памяти ОДИН раз

    int N=200, LEN=16384;
    double* t=malloc(N*sizeof(double));
    void* out=malloc(LEN*2+64);      // байтовый диапазон включает переводы строк
    unsigned long long L=248956422, seed=20260812;

    for(int i=-1;i<N;i++){                                   // -1 = прогрев
        seed = seed*6364136223846793005ULL + 1442695040888963407ULL;
        unsigned long long pos = 1 + (seed>>33) % (L-LEN);
        // координата -> байты, та же арифметика что в fai
        unsigned long long b0 = 6 + (pos-1)/50*51 + (pos-1)%50;
        unsigned long long b1 = 6 + (pos+LEN-2)/50*51 + (pos+LEN-2)%50;
        struct timespec a,b;
        clock_gettime(CLOCK_MONOTONIC,&a);
        long long r = aceapex_decompress_region(arc,n,out,b1-b0+1,b0,b1-b0+1);
        clock_gettime(CLOCK_MONOTONIC,&b);
        if(r<0){ fprintf(stderr,"error %lld at %llu\n",r,pos); return 1; }
        if(i>=0) t[i]=ms(a,b);
    }
    qsort(t,N,sizeof(double),cmpd);
    printf("  ACEAPEX library    %8.3fms %8.3fms  (%d requests, archive resident)\n",
           t[N/2], t[(int)(N*0.99)], N);
    return 0;
}
