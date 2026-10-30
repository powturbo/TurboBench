#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "aceapex.h"
static double ms(struct timespec a,struct timespec b){
    return (b.tv_sec-a.tv_sec)*1e3+(b.tv_nsec-a.tv_nsec)/1e6; }
int main(int argc,char**argv){
    FILE* f=fopen(argv[1],"rb"); fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    void* arc=malloc(n); fread(arc,1,n,f); fclose(f);
    int N=atoi(argv[2]); int LEN=16384;
    unsigned long long L=248956422, seed=20260812;
    struct timespec a,b;

    void* out=malloc(LEN*2+64);
    clock_gettime(CLOCK_MONOTONIC,&a);
    for(int i=0;i<N;i++){
        seed=seed*6364136223846793005ULL+1442695040888963407ULL;
        unsigned long long pos=1+(seed>>33)%(L-LEN);
        unsigned long long b0=6+(pos-1)/50*51+(pos-1)%50;
        unsigned long long b1=6+(pos+LEN-2)/50*51+(pos+LEN-2)%50;
        aceapex_decompress_region(arc,n,out,b1-b0+1,b0,b1-b0+1);
    }
    clock_gettime(CLOCK_MONOTONIC,&b);
    double t_seek=ms(a,b);

    void* whole=malloc(253935557+64);
    clock_gettime(CLOCK_MONOTONIC,&a);
    aceapex_decompress(arc,n,whole,253935557+64);
    clock_gettime(CLOCK_MONOTONIC,&b);
    double t_full=ms(a,b);

    printf("  N=%6d: %d region reads %8.1f ms | one full decode %6.1f ms | %s\n",
           N,N,t_seek,t_full, t_seek<t_full?"seek wins":"full decode wins");
    return 0;
}
