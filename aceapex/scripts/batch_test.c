// Батч против цикла одиночных вызовов: корректность и пропускная способность.
// Промышленный набор: uniform random, sorted, clustered (как BED-файл),
// hot-set (как геномный браузер).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "aceapex.h"

static double ms(struct timespec a,struct timespec b){
    return (b.tv_sec-a.tv_sec)*1e3+(b.tv_nsec-a.tv_nsec)/1e6; }

static unsigned long long rnd(unsigned long long* s){
    *s=*s*6364136223846793005ULL+1442695040888963407ULL; return *s>>33; }

typedef struct { unsigned long long off, len; } Q;

// четыре промышленных профиля обращения
static void gen(Q* q,int n,int mode,unsigned long long L,int LEN,unsigned long long* s){
    for(int i=0;i<n;i++){
        unsigned long long pos;
        switch(mode){
        case 0: pos=1+rnd(s)%(L-LEN); break;                       // uniform
        case 1: pos=1+(unsigned long long)((double)i/n*(L-LEN)); break; // sorted scan
        case 2: { unsigned long long c=1+rnd(s)%(L-LEN-1000000);    // clustered BED
                  pos=c+rnd(s)%1000000; if(pos>L-LEN) pos=L-LEN; } break;
        default:{ unsigned long long hot[64];                        // hot set
                  for(int k=0;k<64;k++) hot[k]=1+((rnd(s))%(L-LEN));
                  pos=hot[rnd(s)%64]; } break;
        }
        unsigned long long b0=6+(pos-1)/50*51+(pos-1)%50;
        unsigned long long b1=6+(pos+LEN-2)/50*51+(pos+LEN-2)%50;
        q[i].off=b0; q[i].len=b1-b0+1;
    }
}

int main(int argc,char**argv){
    FILE* f=fopen(argv[1],"rb"); fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    void* arc=malloc(n); if(fread(arc,1,n,f)!=(size_t)n) return 1; fclose(f);
    int LEN=16384; unsigned long long L=248956422;
    const char* names[4]={"uniform","sorted","clustered","hot-set"};
    int sizes[4]={100,1000,10000,50000};

    printf("%-11s %7s | %11s %11s | %8s | %10s %10s\n",
           "профиль","N","цикл ms","батч ms","ускор","цикл/с","батч/с");
    for(int mode=0;mode<4;mode++){
      for(int si=0;si<4;si++){
        int N=sizes[si];
        unsigned long long seed=20260813+mode*7919+si;
        Q* q=malloc(N*sizeof(Q));
        gen(q,N,mode,L,LEN,&seed);

        char** bufA=malloc(N*sizeof(char*)); char** bufB=malloc(N*sizeof(char*));
        for(int i=0;i<N;i++){ bufA[i]=malloc(q[i].len); bufB[i]=malloc(q[i].len); }
        struct timespec a,b;

        // A: цикл одиночных вызовов
        clock_gettime(CLOCK_MONOTONIC,&a);
        for(int i=0;i<N;i++)
            aceapex_decompress_region(arc,n,bufA[i],q[i].len,q[i].off,q[i].len);
        clock_gettime(CLOCK_MONOTONIC,&b);
        double tA=ms(a,b);

        // B: батч
        aceapex_range_t* rs=malloc(N*sizeof(aceapex_range_t));
        for(int i=0;i<N;i++){ rs[i].offset=q[i].off; rs[i].length=q[i].len;
                              rs[i].dst=bufB[i]; rs[i].written=0; }
        clock_gettime(CLOCK_MONOTONIC,&a);
        int64_t ok=aceapex_decompress_ranges(arc,n,rs,N,0);
        clock_gettime(CLOCK_MONOTONIC,&b);
        double tB=ms(a,b);

        // судья: побайтное совпадение всех ответов
        int bad=0;
        for(int i=0;i<N;i++){
            if(rs[i].written!=(int64_t)q[i].len){ bad++; continue; }
            if(memcmp(bufA[i],bufB[i],q[i].len)) bad++;
        }
        printf("%-11s %7d | %11.1f %11.1f | %7.2fx | %10.0f %10.0f%s\n",
               names[mode],N,tA,tB,tA/tB,N/(tA/1000.0),N/(tB/1000.0),
               bad? "  РАСХОЖДЕНИЯ!":"");
        if(bad) printf("        несовпадений: %d из %d, ok=%lld\n",bad,N,(long long)ok);

        for(int i=0;i<N;i++){ free(bufA[i]); free(bufB[i]); }
        free(bufA); free(bufB); free(rs); free(q);
      }
      printf("\n");
    }
    return 0;
}
