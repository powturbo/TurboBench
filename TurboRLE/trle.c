/**
    Copyright (C) powturbo 2015-2016
    GPL v2 License

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    - email    : powturbo [AT] gmail.com
    - github   : https://github.com/powturbo
    - homepage : https://sites.google.com/site/powturbo/
    - twitter  : https://twitter.com/powturbo

    TurboRLE - "Most efficient and fastest Run Length Encoding"
**/      
#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
  #ifdef _MSC_VER
#include "vs/getopt.h"
  #else
#include <getopt.h> 
#endif

#include "trle.h"
   
//-------------------------------------- Time ---------------------------------------------------------------------  
typedef unsigned long long tm_t;
#define TM_T 1000000.0
#define TM_MAX (1ull<<63)
  #ifdef _WIN32
#include <windows.h>
static LARGE_INTEGER tps;
static tm_t tmtime(void) { LARGE_INTEGER tm; QueryPerformanceCounter(&tm); return (tm_t)(tm.QuadPart*1000000/tps.QuadPart); }
static tm_t tminit() { QueryPerformanceFrequency(&tps); tm_t t0=tmtime(),ts; while((ts = tmtime())==t0); return ts; } 
  #else
#include <time.h>
static   tm_t tmtime(void)    { struct timespec tm; clock_gettime(CLOCK_MONOTONIC, &tm); return (tm_t)tm.tv_sec*1000000ull + tm.tv_nsec/1000; }
static   tm_t tminit()        { tm_t t0=tmtime(),ts; while((ts = tmtime())==t0) {}; return ts; }
  #endif
//---------------------------------------- bench ---------------------------------------------------------------------
#define TMPRINT(__x) { printf("%7.2f MB/s\t%s", (double)(tm>=0.000001?(((double)n*rm/MBS)/(((double)tm/1)/TM_T)):0.0), __x); fflush(stdout); }
#define TMDEF unsigned r,t,rm; tm_t tx = 2000000,t0,tc,tm
#define TMBEG for(tm = TM_MAX,t = 0; t < trips; t++) {  for(t0 = tminit(), r=0; r < reps;) { 
#define TMEND                                            r++; if((tc = tmtime() - t0) > tx) break; } if(tc < tm) tm = tc,rm=r; }
#define MBS 1000000.0 //MiBS 1048576.0

unsigned argtoi(char *s) {
  char *p; unsigned n = strtol(s, &p, 10),f=1; 
  switch(*p) {
     case 'k': f = 1000;       break;
     case 'm': f = 1000000;    break;
     case 'g': f = 1000000000; break;
     case 'K': f = 1<<10; 	   break;
     case 'M': f = 1<<20; 	   break;
     case 'G': f = 1<<30; 	   break;
  }
  return n*f;
}
//-------------------------------------------------------------------------------------------------------------------------------------
#include "ext/mrle.c"

void check(const unsigned char *in, unsigned char *cpy, unsigned n) { int i;
  for(i = 0; i < n; i++) 
    if(in[i] != cpy[i]) { printf("ERROR at %d ", i); break; }
  memset(cpy,0xff,n); 
}

int main(int argc, char *argv[]) {
  unsigned reps = 1<<30, trips = 3,cmp=0, b = 1 << 30;
  int c, digit_optind = 0, this_option_optind = optind ? optind : 1, option_index = 0;
  static struct option long_options[] = { {"blocsize", 	0, 0, 'b'}, {0,0, 0, 0}  };
  for(;;) {
    if((c = getopt_long(argc, argv, "cb:r:R:", long_options, &option_index)) == -1) break;
    switch(c) {
      case  0 : printf("Option %s", long_options[option_index].name); if(optarg) printf (" with arg %s", optarg);  printf ("\n"); break;								
      case 'r': reps  = atoi(optarg); break;
      case 'R': trips = atoi(optarg); break;
      case 'b': b = argtoi(optarg);   break;
      case 'c': cmp++; 				  break;
    }
  }
  if(argc - optind < 1) { fprintf(stderr, "File not specified\n"); exit(-1); }

  unsigned char *in,*out,*cpy;
  char *inname = argv[optind];  
  FILE *fi = fopen(inname, "rb"); if(!fi ) perror(inname), exit(1);  							
  fseek(fi, 0, SEEK_END); long long flen = ftell(fi); fseek(fi, 0, SEEK_SET);
  if(flen > b) flen = b;
  int n = flen; 
  if(!(in  =        (unsigned char*)malloc(n+1024)))        { fprintf(stderr, "malloc error\n"); exit(-1); } cpy = in;
  if(!(out =        (unsigned char*)malloc(flen*4/3+1024))) { fprintf(stderr, "malloc error\n"); exit(-1); } 
  if(cmp && !(cpy = (unsigned char*)malloc(n+1024)))        { fprintf(stderr, "malloc error\n"); exit(-1); }
  n = fread(in, 1, n, fi);
  fclose(fi);
  if(n <= 0) exit(0); 																printf("'%s' %u\n", inname, n);
    
  unsigned l;
  TMDEF; memcpy(out, in,  n); memcpy(out,cpy,n);
  TMBEG l = trlec(in, n, out); 				  TMEND; printf("%10u ", l); TMPRINT(""); TMBEG trled(out, l, cpy, n); 				TMEND; if(cmp) check(in,cpy,n); TMPRINT("TurboRLE\n");
  TMBEG l = srlec(in, n, out); 				  TMEND; printf("%10u ", l); TMPRINT(""); TMBEG srled(out, l, cpy, n); 				TMEND; if(cmp) check(in,cpy,n); TMPRINT("TurboRLE esc\n");
  TMBEG l = mrlec(in, n, out); 				  TMEND; printf("%10u ", l); TMPRINT(""); TMBEG mrled(out, cpy, n); 				TMEND; if(cmp) check(in,cpy,n); TMPRINT("Mespotine RLE\n");
  
    #if 1
  TMBEG l = srlec8( in, n, out, 0xda);  	  TMEND; printf("%10u ", l); TMPRINT(""); TMBEG srled8( out, l, cpy, n, 0xda);  		TMEND; if(cmp) check(in,cpy,n); TMPRINT("TurboRLE esc  8\n");
  TMBEG l = srlec16(in, n, out, 0xdada); 	  TMEND; printf("%10u ", l); TMPRINT(""); TMBEG srled16(out, l, cpy, n, 0xdada); 		TMEND; if(cmp) check(in,cpy,n); TMPRINT("TurboRLE esc 16\n");
  TMBEG l = srlec32(in, n, out, 0xdadadadau); TMEND; printf("%10u ", l); TMPRINT(""); TMBEG srled32(out, l, cpy, n, 0xdadadadau);	TMEND; if(cmp) check(in,cpy,n); TMPRINT("TurboRLE esc 32\n");
  unsigned long long esc = 0xdadadadau; esc=esc<<32|esc;
  TMBEG l = srlec64(in, n, out, esc); 		  TMEND; printf("%10u ", l); TMPRINT(""); TMBEG srled64(out, l, cpy, n, esc);TMEND; if(cmp) check(in,cpy,n); TMPRINT("TurboRLE esc 64\n");
	#endif
  TMBEG memcpy(out, in,  n);   				  TMEND; printf("%10u ", n); TMPRINT(""); TMBEG memcpy(cpy,out, n); 				TMEND; if(cmp) check(in,cpy,n); TMPRINT("memcpy\n"); 
}
