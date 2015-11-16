/**
    Copyright (C) powturbo 2013-2015
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

    - homepage : https://sites.google.com/site/powturbo/
    - github   : https://github.com/powturbo
    - twitter  : https://twitter.com/powturbo
    - email    : powturbo [_AT_] gmail [_DOT_] com
**/
//	    TurboBench: main program
#define _CRT_SECURE_NO_WARNINGS
#define _GNU_SOURCE              
#define _LARGEFILE64_SOURCE 1 
#include <stdio.h>  
#include <string.h>  
#include <stdlib.h> 
#include <float.h> 
#include <errno.h>
#include <malloc.h>			
#include <getopt.h>
#include <unistd.h> 
#include <sys/types.h>
  #ifndef _WIN32
#include <sys/resource.h>
  #endif 
//--------------------------------------- Time ---------------------------------------------------------------------  
typedef unsigned long long tm_t;
#define TM_T 1000000.0
#define TM_MAX (1ull<<63)
  #ifdef _WIN32
#include <windows.h>
static LARGE_INTEGER tps;
static tm_t tmtime(void) { LARGE_INTEGER tm; QueryPerformanceCounter(&tm); return (tm_t)((double)tm.QuadPart*1000000.0/tps.QuadPart); }
static tm_t tminit() { QueryPerformanceFrequency(&tps); tm_t t0=tmtime(),ts; while((ts = tmtime())==t0); return ts; } 
  #else
#include <time.h>
static   tm_t tmtime(void)    { struct timespec tm; clock_gettime(CLOCK_MONOTONIC, &tm); return (tm_t)tm.tv_sec*1000000ull + tm.tv_nsec/1000; }
static   tm_t tminit()        { tm_t t0=tmtime(),ts; while((ts = tmtime())==t0); return ts; }
  #endif
//---------------------------------------- bench -------------------------------------------------------------------
#define TM_MAX (1ull<<63)
#define TMPRINT(__x) { printf("%7.2f MB/s\t%s", (double)(tm_tm>=0.000001?(((double)n*tm_rm/MBS)/(((double)tm_tm/1)/TM_T)):0.0), __x); fflush(stdout); }
#define TMDEF unsigned tm_r,tm_R; tm_t _t0,_tc,_ts;
#define TMSLEEP do { tm_T = tmtime(); if(!tm_0) tm_0 = tm_T; else if(tm_T - tm_0 > tm_TX) { printf("S\b");fflush(stdout); sleep(tm_slp); tm_0=tmtime();} } while(0)
#define TMBEG(__c,__tm_reps,__tm_Reps) \
  for(tm_tm = TM_MAX,tm_R=0,_ts=tmtime(); tm_R < __tm_Reps; tm_R++) { if(__tm_reps>1) TMSLEEP; printf("%c%d\b\b",__c,tm_R+1);fflush(stdout);\
    for(_t0 = tminit(), tm_r=0; tm_r < __tm_reps;) {

#define TMEND tm_r++; tm_T = tmtime(); if((_tc = (tm_T - _t0)) > tm_tx) break; } if(_tc < tm_tm) tm_tm = _tc,tm_rm=tm_r; if(tm_T-_ts > tm_TX) break; }
#define MBS   1000000.0

static unsigned tm_repc = 1<<30, tm_Repc = 3, tm_repd = 1<<30, tm_Repd = 4, tm_rm, tm_slp = 60;
static tm_t     tm_tm, tm_tx = 2*TM_T, tm_TX = 120*TM_T, tm_0, tm_T;

#define Kb (1u<<10)
#define Mb (1u<<20)
#define Gb (1u<<30)
#define KB 1000
#define MB 1000000
#define GB 1000000000

unsigned argtoi(char *s) {
  char *p; 
  unsigned n = strtol(s, &p, 10),f=1; 
  switch(*p) {
    case 'K': f = KB; break;
    case 'M': f = MB; break;
    case 'G': f = GB; break;
    case 'k': f = Kb; break;
    case 'm': f = Mb; break;
    case 'g': f = Gb; break;
	default:  f = Mb;
  }
  return n*f;
}

unsigned long long argtol(char *s) {
  char *p;
  unsigned long long n = strtol(s, &p, 10),f=1; 
  switch(*p) {
    case 'K': f = KB; break;
    case 'M': f = MB; break;
    case 'G': f = GB; break;
    case 'k': f = Kb; break;
    case 'm': f = Mb; break;
    case 'g': f = Gb; break;
	default:  f = MB;	
  }
  return n*f;
}

void memrcpy(unsigned char *out, unsigned char *in, unsigned n) { int i; for(i = 0; i < n; i++) out[i] = ~in[i]; }

int strpref(const char *const *str, int n, char sep1, char sep2) {
  int i, j=0;
  for(;;j++)
    for(i = 0; i < n; i++)
 	  if(!str[i][j] || str[i][j] != str[0][j]) {
	    while (j > 0 && str[0][j-1] != sep1 && str[0][j-1] != sep2) j--;
	    return j;
	  }
  return 0;
}

//--------------------------------------- TurboBench ----------------------------------------------------------------------
#include "conf.h"   
#include "plugins.h"
#include "plugreg.c"
static int prts=1,quiet,speedup;

void plugsprt(void) {
  struct plugs *gs;
    #if defined(_COMPRESS1) || defined(_COMPRESS2)
  printf("Compressor group:\n'FASTEST' HDD/SSD/RAM speed\n'FAST' lz4,lzturbo,zlib class\n'EFFICIENT' efficient compression,Rolz,Lzp class\n'OPTIMAL': optimal parsing (slow)\n'ECODER' entropy coders\n");
    #endif
    #if defined(_ECODEC)
  printf("\nEntropy Coder group: 'ECODER' entropy coders\n");
    #endif
  printf("\nPlugins:\n");
  for(gs = plugs; gs->id >= 0; gs++) 
    printf("%s %s\n", gs->s, gs->lev?gs->lev:""); 
}

void plugsprtv(FILE *f) {
  struct plugs *gs;
  char *pv="";
  if(prts> 4) printf("%s\n", "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><title>TurboBench</title></head><body><pre>");
  if(prts>=4) fprintf(f, "%s", prts>4?"<ul>":"[list]");
  for(gs = plugs; gs->id >= 0; gs++) 
    if(/*gs->v[0]>' ' &&*/ strcmp(gs->v,pv)) {
      pv = gs->v;
	  char name[65],ver[33],s[255],*p,*url="",*pn="",*lic="",*mir=""; ver[0]=0;
	  strcpy(s, gs->v);
      if(p = strtok(s,"\t")) codver( gs->id, p, ver); 
      if(p = strtok(NULL,"\t")) pn  = p;
      if(p = strtok(NULL,"\t")) lic = p;
      if(p = strtok(NULL,"\t")) url = p;
      if(p = strtok(NULL,"\t")) mir = p;
      sprintf(name, "%s v%s", pn, ver);
      switch(prts) {  
        case 4 : fprintf(f, "[*][URL=\"%s\"]%s[/URL] %s\n", url, name, lic ); break;
        case 5 : fprintf(f, "<li><a href=\"%s\">%s</a> %s\n", url, name, lic ); break;
        case 6 : fprintf(f, " - [%s](%s) %s\n", name, url, lic ); break;
        default: fprintf(f, "%-24s\t%s\t%s\t%s\n", name, lic, url, mir );
      }
    }  
  if(prts>=4) fprintf(f, "%s\n", prts>4?"</ul>":"[/list]");
  if(prts> 4) printf("%s\n", "</pre></body></html>");
}

int memcheck(unsigned char *in, unsigned n, unsigned char *cpy, int cmp) { 
  int i;
  if(cmp <= 1) 
    return 0;
  for(i = 0; i < n; i++)
    if(in[i] != cpy[i]) {
      printf("ERROR at %d:%x, %x\n", i, in[i], cpy[i]); 
      if(cmp > 2) exit(0);
	  return i+1; 
	}
  return 0;
}

static int mcpy, mode, tincx, blknum;

int becomp(unsigned char *__restrict _in, unsigned _inlen, unsigned char *__restrict _out, unsigned outsize, unsigned bsize, int id, int lev, char *prm) { 
  unsigned char *op,*oe = _out + outsize;
  TMDEF;
  TMBEG('C',tm_repc,tm_Repc);                                              
  unsigned char *in,*ip;																							
  for(op = _out, in = _in; in < _in+_inlen; ) { 
    unsigned inlen,bs; 
    if(mode) { 														blknum++;
      inlen = ctou32(in); in += 4; 
      ctou32(op) = inlen; op += 4; //vbput32(op, inlen); 
      if(in+inlen>_in+_inlen) inlen = (_in+_inlen)-in;
    } else inlen = _inlen;

    for(ip = in, in += inlen; ip < in; ) { 
      unsigned iplen = in - ip; iplen = min(iplen, bsize);       
      bs = (min(bsize, iplen) < (1<<16))?2:4;
      int oplen = codcomp(ip, iplen, op+bs, oe-(op+bs), id, lev,prm);
      if(oplen <= 0 || oplen >= iplen && mcpy) {
	    if(mcpy) { memcpy(op+bs, ip, iplen); oplen = iplen; }
	    else if(oplen <= 0) return 0;
	  }
      bs==2?(ctou16(op) = oplen):(ctou32(op) = oplen); op += oplen+bs; ip += iplen; 
    }                                                             
    if(op > _out+outsize) 
	  die("Overflow error %llu, %u in lib=%d\n", outsize, (int)(ptrdiff_t)(op - _out), id);      
  }
  TMEND;	
  return op - _out;
}

int bedecomp(unsigned char *_in, int _inlen, unsigned char *_out, unsigned _outlen, unsigned bsize, int id, int lev) { 
  unsigned char *ip;
  TMDEF; 
  TMBEG('D',tm_repd,tm_Repd);   
  unsigned char *out,*op;
  for(ip = _in, out = _out; out < _out+_outlen;) {
    unsigned outlen,bs; 
    if(mode) { outlen = /*vbget32(ip);*/ ctou32(ip); ip += 4; 
      ctou32(out) = outlen; out += 4;
      if(out+outlen>_out+_outlen) 
        outlen = (_out+_outlen)-out; 
    } else outlen = _outlen;
    for(op = out, out += outlen; op < out; ) { 
      unsigned oplen = out - op; 
      oplen = min(oplen, bsize); 
      bs = (min(bsize,oplen)<(1<<16))?2:4;
      int l, iplen = bs==2?ctou16(ip):ctou32(ip); ip += bs;
      if(mcpy && iplen==oplen) 
        memcpy(op, ip, oplen); 
	  else l = coddecomp(ip, iplen, op, oplen, id, lev);
      ip += iplen; op += oplen;
    }
  }
  TMEND;
  return ip - _in;
}

//---------------------------------------------------------------------------------------------------------------------
struct plug { 
  int       id,err,blksize,lev;
  char      *s,*v,*prm; 
  long long len; 
  double    tc,td,tck,tdk;
};

struct plug plug[255],plugt[255];
int         seg_ans = 32*1024, seg_huf = 32*1024, seg_anx = 12*1024, seg_hufx=11*1024, verbose;
static int  cmp = 2,trans;
double      fac = 1.3;

int plugins(struct plug *plug, struct plugs *gs, int *pk, unsigned bsize, int bsizex, int lev, char *prm) { 
  int i,k = *pk;
  for(i = 0; i < k; i++) 
    if(plug[i].id == gs->id && plug[i].lev == lev && !strcmp(plug[i].prm,prm))
      return -1;

  memset(&plug[k], 0, sizeof(struct plug)); 
  plug[k].id  = gs->id; 
  plug[k].err = 0; 
  plug[k].s   = gs->s; 
  plug[k].v   = gs->v; 
  plug[k].lev = lev; 
  plug[k].prm = prm?prm:""; 
  if(gs->flag & E_FLAG) {
    if(gs->flag & E_ANS)  plug[k].blksize = seg_ans;
    if(gs->flag & E_HUF)  plug[k].blksize = seg_huf;
  } else plug[k].blksize = gs->blksize && !bsizex?gs->blksize:bsize;
  *pk = ++k;
  return 0;
}

int plugreg(struct plug *plug, char *cmd, int k, int bsize, int bsizex) {
  static char *cempty=""; 
  int ignore = 0;
  while(*cmd) { 
    while(isspace(*cmd)) cmd++; 
    char *name = cmd; 
    while(isalnum(*cmd) || *cmd == '_' || *cmd == '-') cmd++; 
    if(*cmd) *cmd++ = 0;
         if(!strcmp(name, "ON" )) { ignore=1; continue; }
    else if(!strcmp(name, "OFF")) { ignore=0; continue; }
    for(;;) {																			
      while(isspace(*cmd) || *cmd == ',') cmd++;
      char *prm = cmd; 									
      int lev = strtol(cmd, &cmd, 10); 
      if(prm == cmd) { lev = -1; prm = cempty; }
      else if(isalnum(*cmd)) {
        prm = cmd;
        while(isalnum(*cmd) || *cmd == '_' || *cmd == '-') cmd++; 
        if(*cmd) *cmd++=0; 
      } else prm = cempty;
      int found = 0;
      struct plugs *gs,*gfs=NULL;  
      if(!*name) break;
      for(gs = plugs; gs->id >= 0; gs++)
        if(!strcasecmp(gs->s, name) ) { 
          char s[33]; sprintf(s,"%d", lev); 
          found++; 
          if(lev<0 && gs->lev && !gs->lev[0] || strstr(gs->lev, s)) { 						//printf("[%s,'%s',%s]", name, s, prm);fflush(stdout);
            found++; 
            plugins(plug, gs, &k, bsize, bsizex, lev, prm); 
          }
          break; 
        }
      if(found<2 && !ignore) {
        if(!found) fprintf(stderr, "codec '%s' not found\n", name);
        else if(lev<0) fprintf(stderr, "level [%s] not specified for codec '%s'\n", gs->lev, name );
        else if(gs->lev && gs->lev[0]) fprintf(stderr, "level '%d' for codec '%s' not in range [%s]\n", lev, name, gs->lev);
        else fprintf(stderr, "codec '%s' has no levels\n", name);
        exit(0); 
      }
      while(isspace(*cmd)) cmd++;	//printf("(%s)\n", cmd);fflush(stdout);						
      if(*cmd != ',' && (*cmd < '0' || *cmd > '9')) break;
    }
  } 
  a:plug[k].id = -1;  
  return k;
}

struct bandw {
  unsigned long long bw;
  unsigned           rtt; 
  char               *s;
};

static struct bandw bw[] = {
  {    7*KB, 500, "GPRS 56"  },//56kbps
  {   32*KB, 300, "2G 256"   },//Regular 
  {   57*KB, 150, "2G 456"   },//Good
  {   94*KB, 100, "3G 752"   },//Regular
  {  125*KB,  40, "3G 1M"    },//Good
  {  250*KB,   5, "DSL 2M"   },//DSL 2000
  {  500*KB,  20, "4G 4M"    },
  { 3750*KB,   5, "WIFI 30M" },
  {12500*KB,   5, "CAB 100M" },
  {  125*MB,   0, "ETH 1000" },
  {  150*MB,   0, "HDD 150MB"},
  {  550*MB,   0, "SSD 550MB"},
  {   1u*GB,   0, "SSD 1GB"  },
  {   2u*GB,   0, "SSD 2GB"  },
  { 4ull*GB,   0, "4GB/s"    },
  { 8ull*GB,   0, "8GB/s"    }
};

#define BWSIZE (sizeof(bw)/sizeof(struct bandw))

void plugprth(FILE *f) {
  int i; 
  switch(prts) {
    case 3:  
      fprintf(f,"size\tcsize\tratio\tctime\tdtime\tname\tfile");
      if(speedup)
        for(i = 0; i < BWSIZE; i++) { 
          char s[33],*p; 
          strcpy(s, bw[i].s); 
          if(p=strchr(s,' ')) 
            *p='_'; 
          printf("\t%s", s); 
        }
        break;
    default: 
      fprintf(f,"     Size           C Size  ratio%%     C MB/s     D MB/s   Name           "); 
      if(speedup) {
        for(i = 0; i < BWSIZE; i++) 
          printf("%10s", bw[i].s);
      fprintf(f,"  File"); 
        if(blknum) 
          printf(" blknum=%d ", blknum);
      }
  }
}

#define TMBS(__l,__t) ((__t)>=0.000001?((double)(__l)/MBS)/((__t)/TM_T):0.0)

void plugprt(struct plug *plug, long long totinlen, FILE *f, char *finame, double *ptc, double *ptd, int speedup) {
  double ratio = (double)plug->len*100.0/totinlen,
         tc    = TMBS(totinlen,plug->tc), td = TMBS(totinlen,plug->td);
  char   name[65]; 
  if(plug->lev>=0) 
    sprintf(name, "%s%s %d%s", plug->err?"?":"", plug->s, plug->lev, plug->prm);
  else
    sprintf(name, "%s%s%s", plug->err?"?":"", plug->s, plug->prm);
 
  switch(prts) {
    case 3:   fprintf(f,"%12lld\t%11lld\t%5.1f\t%8.2f\t%8.2f\t%-16s\t%s",          totinlen, plug->len, ratio, tc, td, name, finame); break;
    default:  fprintf(f,"%12lld   %11lld   %5.1f", totinlen, plug->len, ratio); 
      int c = 0;
      if(prts>=4 && tc > *ptc) { c++; printf("%s", prts==4?"[B]":(prts==5?"<b>":"**")); *ptc = tc; } 
      fprintf(f,"   %8.2f", tc); 
      if(c) printf("%s", prts==4?"[/B]":"</b>");
      int d = 0;
      if(prts>=4 && td > *ptd) { d++; printf("%s", prts==4?"[B]":(prts==5?"<b>":"**")); *ptd = td; } 
      fprintf(f,"   %8.2f", td); 
      if(d) printf("%s",prts==4?"[/B]":"</b>");
      if(c || d) printf("%s", prts==4?"[B]":(prts==5?"<b>":"**"));
      fprintf(f,"   %-16s", name); 
      if(c || d) printf("%s",prts==4?"[/B]":(prts==5?"</b>":"**"));
      switch(speedup) {
        case 1: 
          for(c=0;c<BWSIZE; c++) { 
            double t = plug->td + plug->len*TM_T/(double)bw[c].bw + blknum*(bw[c].rtt*1000.0); 
            printf("%9.3f ", TMBS(totinlen,t) ); 
          } break;
        case 2: for(c=0;c<BWSIZE; c++) 
                  printf("%9d ", (int)((double)totinlen*100.0 / ((double)plug->len + ((plug->td+blknum*bw[c].rtt*1000.0)/TM_T)*(double)bw[c].bw ) ));           
      }
      fprintf(f," %s", finame); 
  }
  if(plug->err) 
	fprintf(f, "\tERROR[%d]", plug->err);
}

int libcmp(const struct plug *e1, const struct plug *e2) {
  if(e1->len < e2->len)
    return -1;
  else if(e1->len > e2->len)
    return 1;
  else if(e1->td < e2->td)
    return -1;
  else if(e1->td > e2->td)
    return 1;
  return 0;
}

int plugprts(struct plug *plug, int k, char *finame, unsigned long long totlen) { 
  if(!totlen) return 0; 														  if(verbose) printf("'%s'\n", finame); 

  double ptc = 0.0, ptd = 0.0;
  qsort(plugt, k, sizeof(struct plug), libcmp);  
  printf("\n");
  if(prts >= 4) printf("%s\n", prts==4?"[CODE][B]":"<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><title>TurboBench</title></head><body><pre><b>");
  plugprth(stdout);
  if(prts >= 4) printf("   (bold = pareto)  MB%s=1.000.0000 ", prts==4?"[/B]":"</b>");
  printf("\n");
  struct plug *g; 
  double td=0.0,tc=0.0;
  for(g = plugt; g < plugt+k; g++) { plugprt(g, totlen, stdout, finame, &ptc, &ptd, speedup); printf("\n"); }
  if(prts >= 4) printf("%s\n", prts==4?"[/CODE]":"</pre></body></html>");
} 

  #ifdef _PLUGIN2
#include "../bebench.h"
  #else
unsigned trid[32],tid;
#define BEPRE
#define BEINI
#define BEPOST
#define BEOPT
#define BEUSAGE
  #endif

  #ifdef __WORDSIZE == 64
#define BENCHSIZE (1u<<30)
  #else
#define BENCHSIZE (1u<<28)
  #endif

unsigned long long plugfile(struct plug *plug, char *finame, unsigned long long filenmax, unsigned bsize, unsigned *trid, int tid, int krep) {
  size_t outsize;   
  FILE *fi = fopen(finame, "rb"); if(!fi) { perror(finame); die("open error '%s'\n", finame); }
  unsigned char *p; 
  if((p = strrchr(finame, '\\')) || (p = strrchr(finame, '/'))) finame = p+1; 	if(verbose) printf("'%s'\n", finame);
  p = finame; 

  fseeko(fi, 0, SEEK_END); unsigned long long filen = ftello(fi); fseeko(fi , 0 , SEEK_SET); if(filen > filenmax) filen = filenmax;  				

  size_t insize = min(filen,BENCHSIZE);
  outsize = insize*fac+10*Mb; 
  unsigned char *in  = (unsigned char*)malloc(insize+4096),*cpy = in;
  unsigned char *out = (unsigned char*)malloc(outsize);
 
  if(cmp || tid) cpy = (unsigned char*)malloc(insize+4096);
  if(!in || !out || !cpy) die("malloc err=%u\n", insize+outsize+cpy?insize:0);

  codini(insize);																
  int       inlen;																	
  long long totinlen = 0;
  double ptc = DBL_MAX, ptd = DBL_MAX;
  bsize = plug->blksize;
  plug->len = plug->tc = plug->td = 0; 											blknum=0;	
  while((inlen = fread(in, 1, insize, fi)) > 0) {
    double   tc = 0.0, td = 0.0;         
    unsigned l = inlen,outlen;
	totinlen += inlen;															//if(!quiet) printf("%12d ", inlen); fflush(stdout);	
    BEPRE;																		memrcpy(out, in, inlen);
	outlen = becomp(in, l, out, outsize, bsize, plug->id, plug->lev, plug->prm);
																				plug->len += outlen; plug->tc += tc += (double)tm_tm/(double)tm_rm;
    if(tm_Repc>1) TMSLEEP;
																				//if(!quiet) { double ratio = (double)outlen*100.0/inlen; printf("%11d\t%5.1f\t%7.2f\t", outlen, ratio, tc>=0.000001?((double)inlen/MBS)/(tc/TM_T):0.0); fflush(stdout); printf("\t");fflush(stdout);}
    if(cmp) {
	  if(cpy != in) memrcpy(cpy, in, inlen);
	  unsigned cpylen = bedecomp(out, outlen, cpy, l, bsize, plug->id,plug->lev); 		td = (double)tm_tm/(double)tm_rm;
      plug->err = memcheck(in, l, cpy, cmp);  
      BEPOST;																	plug->td += td;
	}																			if(!quiet && inlen == filen) { plugprt(plug, totinlen, stdout, finame, &ptc, &ptd,0);printf("\n");}
	if(totinlen >= filen) break;
  }	  
  if(!quiet && filen > insize) { plugprt(plug, totinlen, stdout, finame, &ptc, &ptd,0); printf("\n"); }
  codexit();
  fclose(fi); 
  free(in); free(out); if(cpy && cpy != in) free(cpy);
  return totinlen;
}

void usage(char *pgm) {
  fprintf(stderr, "\nTurboBench Copyright (c) 2013-2015 Powturbo v15.11 %s\n", __DATE__);
  fprintf(stderr, "Usage: %s [options] [file]\n", pgm);
  fprintf(stderr, " -eS      S = compressors/groups separated by '/' Parameter can be specified after ','\n");
  fprintf(stderr, " -b#s     # = blocksize (default filesize,). max=1GB\n");
  fprintf(stderr, " -B#s     # = max. benchmark filesize (default 1GB) ex. -B4G\n");
  fprintf(stderr, " -v#      # = verbosity 1..3\n");
  fprintf(stderr, " -t#      # = min. time in seconds per run.(default=2sec)\n");
  fprintf(stderr, " -S#      Sleep 1 min. after 2 min. processing\n");
  fprintf(stderr, " -i#/-j#  # = Minimum  de/compression iterations per run (default=auto). 0=1 iter. + 1 run\n");
  fprintf(stderr, " -I#/-J#  # = Number of de/compression runs (default=3)\n");
  fprintf(stderr, " -K#      Repeat all benchmarks N (default=3) number\n");
  fprintf(stderr, " -B#s     # = max. file size to read\n");
  fprintf(stderr, "          s = modifier s:K,M,G=(1000,1 million,1 billion) s:k,m,h=(1024,1MB,1GB) ex. 64k or 64K\n");
  fprintf(stderr, " -g#      # = 1 : print all groups/plugins\n");
  fprintf(stderr, " -g#      # = 2 : print all codecs\n");
  fprintf(stderr, " -Rstr    str=Remark/Comment string\n");
  fprintf(stderr, " -Moutput concatenate all input files to multiple blocks file output\n");
  fprintf(stderr, " -m       process multiple blocks per file\n");
  fprintf(stderr, " -H#      #=0 compress only, #=1 no compare, #=2 exit on error\n");
  fprintf(stderr, " -a#      #='print format' 2:raw 3:csv 4=encode.ru 5:html 6:markdown\n");
  fprintf(stderr, " -S#      print speed sheet: #=1 'MB/s' #=2 speedup\n");
  BEUSAGE;
  fprintf(stderr, "ex. ./turbobench enwik9 -eFAST/bzip2/lzma,5,9\n");
  fprintf(stderr, "ex. ./turbobench enwik9 -eFAST/OPTIMAL/bsc,2 -i0\n");
  fprintf(stderr, "ex. ./turbobench eECODER -R\"entropy coder test\"\n");
  exit(0);
} 

struct plugg { 
  char id[17],*s; 
};

struct plugg plugg[] = {
  { "FASTEST",   "lzturbo,10,11,12,19,20,21,22,29/lz4,0,1,9/lz5,0,1,9/chameleon,1,2/density,1,3/memcpy/" },
  { "FAST",      "lzturbo,10,10a,11,12,20,20a,21,22,30,30a,31,32/zlib,1,6,9/brotli,1,5,6/lz4,1/zstd,1,5,9/lz5,1,9/memcpy/" },
  { "EFFICIENT", "lzturbo,22,31,32/brotli,5/zlib,5/zstd,5,9/zling,4/memcpy" },
  { "OPTIMAL",   "lzturbo,19,29,39,49/lzma,9/lzham,4/brotli,11/lzlib,9/zstd,20/memcpy" },
  { "BWT",       "bsc-st,4,5/bsc,2/bcm/bzip2" },
  { "ECODER",    "turbohf/turboanx/turborc/turborc-o1/turboac-byte/arith_static/rans_static4c/subotin/fasthf/fastac/zlibh/fse/fsehuf/memcpy/" },
};
#define PLUGGSIZE (sizeof(plugg)/sizeof(plugg[0]))

extern int _CRT_glob = 1;
unsigned dbglcnt;
int main(int argc, char* argv[]) { 
  int                recurse  = 0, xplug = 0,tm_Repk=3;
  unsigned           bsize    = 1u<<30, bsizex=0;
  unsigned long long filenmax = 0;
  char               *scmd = NULL,*trans=NULL,*beb=NULL,*rem,cmd[2049];
   
  int c, digit_optind = 0;
  for(;;) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
      { "help", 	0, 0, 'h'},
      { 0, 		    0, 0, 0}
    };

    if((c = getopt_long(argc, argv, "a:b:B:C:e:E:F:g:H:i:I:j:J:K:L:mM:PqrR:S:t:T:Uv:V:W:xX:Y:Z:", long_options, &option_index)) == -1) break;
    switch(c) { 
      case 0:
        printf("Option %s", long_options[option_index].name);
        if(optarg) printf (" with arg %s", optarg);  printf ("\n");
        break;
      case 'a': prts     = atoi(optarg);             break;
      case 'b': bsize    = argtoi(optarg); bsizex++; break;
      case 'B': filenmax = argtol(optarg);    		 break;
      case 'e': scmd     = optarg;            		 break;
      case 'F': fac      = strtod(optarg, NULL); 	 break;
      case 'g': xplug    = atoi(optarg);             break;
      case 'H': cmp      = atoi(optarg);      		 break;

      case 'i': if((tm_repc  = atoi(optarg))<=0) 
		          tm_repc=tm_Repc=tm_Repk=1;         break;
      case 'I': tm_Repc  = atoi(optarg);       		 break;
      case 'j': if((tm_repd  = atoi(optarg))<=0) 
		          tm_repd=tm_Repd=tm_Repk=1;         break;
      case 'J': tm_Repd  = atoi(optarg);      		 break;
      case 'K': tm_Repk  = atoi(optarg);      		 break;
      case 'L': tm_slp   = atoi(optarg);      		 break;
 	  case 't': tm_tx    = atoi(optarg)*TM_T; 		 break;
 	  case 'T': tm_TX    = atoi(optarg)*TM_T; 		 break;
      case 'R': rem      = optarg;		      		 break;
      case 'S': speedup  = atoi(optarg);       		 break;

      case 'q': quiet++; 		 			 		 break;
      case 'm': mode++; 		 			 		 break;
      case 'P': mcpy++;       		 			     break;	  
      case 'v': verbose  = atoi(optarg);       		 break;
      case 'Y': seg_ans  = argtoi(optarg);           break;
      case 'Z': seg_huf  = argtoi(optarg);           break;
      BEOPT;
	  case 'h':
      default: usage(argv[0]); exit(1); 
    }
  }
  if(xplug) { xplug==1?plugsprt():plugsprtv(stdout); exit(0); }
  if(argc <= optind) die("file not specified. type turbobench -h for help\n");
  if((tm_repc|tm_Repc|tm_repd|tm_Repd) ==1) tm_Repk=1;
    #ifdef _WIN32
  SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    #else
  setpriority(PRIO_PROCESS, 0, -19);
	#endif

  if(!scmd) scmd = "FAST";
  for(cmd[0] = 0;;) {
    char *q; int i;
    if(!*scmd) break;
    if(q = strchr(scmd,'/')) *q = '\0';
    for(i = 0; i < PLUGGSIZE; i++)
      if(!strcmp(scmd, plugg[i].id)) { strcat(cmd, "ON/"); strcat(cmd, plugg[i].s); strcat(cmd, "OFF/"); break; }
    if(i >= PLUGGSIZE) { strcat(cmd,scmd); strcat(cmd,"/"); }
    scmd = q?(q+1):"";
  }

  unsigned k = plugreg(plug, cmd, 0, bsize, bsizex);
  BEINI;
  if(!filenmax) filenmax = Gb; 
  long long totinlen = 0;  
  int       fno,krep;
  struct    plug *p;
  char     *finame = "";
  for(p = plugt; p < plugt+k; p++) p->tc = p->td = DBL_MAX; 
  for(krep = 0; krep < tm_Repk; krep++) { 
    if(tm_Repk>1) printf("Benchmark: %d from %d\n", krep+1, tm_Repk);
    for(p = plug; p < plug+k; p++) {
      struct plug *g = &plugt[p-plug];
	  totinlen = 0;  
      g->len = g->tck = g->tdk = 0;
      for(fno = optind; fno < argc; fno++) {
	    finame = argv[fno];
        totinlen += plugfile(p, finame, filenmax, bsize, trid, tid,krep);
	    g->len += p->len; 
	    g->tck += p->tc;  
	    g->tdk += p->td;
	  }
      g->v   = p->v;
      g->s   = p->s;
      g->lev = p->lev;
      g->prm = p->prm;
	  g->id  = p->id;
	  g->err = g->err?g->err:p->err;
      if(g->tck < g->tc) g->tc = g->tck;
      if(g->tdk < g->td) g->td = g->tdk;
    } 
  }
  char s[256];
  if(argc - optind > 1) {
    unsigned clen = strpref(&argv[optind], argc-optind, '\\', '/');
    strncpy(s, argv[optind], clen);
    if(clen && (s[clen-1] == '/' || s[clen-1] == '\\')) clen--;
    s[clen] = 0; 
    finame = s;
  } else {
    unsigned char *p;  
    if((p = strrchr(finame, '\\')) || (p = strrchr(finame, '/'))) finame = p+1;
  }
  plugprts(plugt, k, finame, totinlen);	
}
