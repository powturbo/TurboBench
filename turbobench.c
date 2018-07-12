/**
    Copyright (C) powturbo 2013-2017
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
#include <inttypes.h> 
#include <float.h> 
#include <errno.h>
#include <malloc.h>			
#include <sys/types.h>
#include <ctype.h>
  #ifndef _WIN32
#include <sys/resource.h>
  #endif 
  #ifdef _MSC_VER
#include "vs/getopt.h"
  #else
#include <getopt.h>
#include <unistd.h>   
  #endif
  #if !defined(_WIN32)  
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
  #else
#include <io.h> 
#include <fcntl.h>
  #endif

#include <time.h>
#include "conf.h"   
#include "time_.h"
#include "plugins.h" 

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

int memcheck(unsigned char *in, unsigned n, unsigned char *cpy, int cmp) { 
  int i;
  if(cmp <= 1) 
    return 0;
  for(i = 0; i < n; i++)
    if(in[i] != cpy[i]) {
      if(cmp > 3) abort(); // crash (AFL) fuzzing
      printf("ERROR at %d:%x, %x\n", i, in[i], cpy[i]);
      if(cmp > 2) exit(EXIT_FAILURE);      
	  return i+1; 
	}
  return 0;
}
//------------------------------- malloc ------------------------------------------------
#define USE_MMAP
  #ifdef __WORDSIZE == 64
#define MAP_BITS 30
  #else
#define MAP_BITS 28
  #endif

void *_valloc(size_t size, int a) {
  if(!size) return NULL;
    #if defined(_WIN32)
  return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    #elif defined(USE_MMAP)
  void *ptr = mmap((size_t)a<<MAP_BITS, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if(ptr == MAP_FAILED) return NULL;														
  return ptr;
    #else
  return malloc(size); 
    #endif
}

void _vfree(void *p, size_t size) {
  if(!p) return;
    #if defined(_WIN32)
  VirtualFree(p, 0, MEM_RELEASE);
    #elif defined(USE_MMAP)
  munmap(p, size);
    #else
  free(p);
    #endif
} 

  #if defined(NMEMSIZE) || defined(_WIN32) 
#define mempeakinit() 0
#define mempeak() 0
  #else
static size_t mem_peak, mem_used;
size_t mempeak() { return mem_peak; }

size_t mempeakinit() { mem_peak = mem_used = 0; return mem_peak; }

void mem_add(size_t size) { 
  if((mem_used += size) > mem_peak) 
  { mem_peak = mem_used; }
}

void mem_sub(size_t size) { 
  if(mem_used > size) 
    mem_used -= size; 
}

#include <dlfcn.h>
static ALIGNED(char, mem_heap[1<<20],32);
static char *mem_heapp = mem_heap;

static void *(*mem_malloc)(size_t);
static void *(*mem_calloc)(size_t, size_t);
static void *(*mem_realloc)(void*, size_t);
static void  (*mem_free)(void *);
static void *(*mem_memalign)(size_t, size_t);

static __attribute__((constructor)) void mem_init(void) {
  mem_malloc   = dlsym(RTLD_NEXT, "malloc" );
  mem_realloc  = dlsym(RTLD_NEXT, "realloc");
  mem_free     = dlsym(RTLD_NEXT, "free"   );
  mem_calloc   = dlsym(RTLD_NEXT, "calloc" );
  mem_memalign = dlsym(RTLD_NEXT, "memalign");
  if(!mem_malloc || !mem_calloc || !mem_realloc || !mem_free || !mem_memalign)
    die("malloc not found\n");
}

void *malloc(size_t size) {
  if(!mem_malloc) {
    void *p = mem_heapp;
    if((mem_heapp += size) >= mem_heap+sizeof(mem_heap)) 
      die("malloc:initial memory overflow\n");
    return p;       
  }
  void *p = (*mem_malloc)(size);
  if(p) 
    mem_add(malloc_usable_size(p)); 
  return p;
}

void *calloc(size_t nmemb, size_t size) { 
  size_t _size = nmemb*size;
  if(!mem_calloc) {
    void *p = mem_heapp;
    if((mem_heapp += _size) >= mem_heap+sizeof(mem_heap)) 
      die("calloc:initial memory overflow\n");
    memset(p,0,_size);
    return p;       
  }
  void *p = (*mem_calloc)(nmemb, size);
  if(p) 
    mem_add(malloc_usable_size(p)); 
  return p;
}

void *memalign(size_t nmemb, size_t size) { 
  size_t _size = nmemb*size;

  mem_add(_size);
  void *p = (*mem_memalign)(nmemb, size);      
  if(p) 
    mem_add(malloc_usable_size(p)); 
  return p;
}

void *realloc(void *p, size_t size) { 
  mem_sub(malloc_usable_size(p));
  if(p = (*mem_realloc)(p, size))
    mem_add(malloc_usable_size(p)); 
  return p;
}

void free(void *p) { 
   if(!p || p >= (void*)mem_heap && p < (void*)mem_heapp) 
     return; 
   mem_sub(malloc_usable_size(p));
  (*mem_free)(p); 
} 
  #endif
//--------------------------------------- TurboBench ------------------------------------------------------------------
enum { 
  FMT_TEXT=1, 
  FMT_HTML, 
  FMT_HTMLT, 
  FMT_MARKDOWN,    
  FMT_VBULLETIN,  // ex. post to encode.ru
  FMT_CSV,
  FMT_TSV,
  FMT_SQUASH 
};

char *fmtext[] = { "txt", "txt", "html", "htm", "md", "vbul", "csv", "tsv", "squash" };
#define LSIZE 512

//------------- plugin : usage ---------------------------------
struct plugg { 
  char id[17],*s,*desc; 
};

struct plugg plugg[] = 
{
  { "FASTEST",   "lzturbo,10,11,12,19,20,21,22,29/lz4,0,1/lizard,10/chameleon,1,2/density,1,3/memcpy", 						"Fastest de-/compression. HDD/SSD/RAM speed" },
  { "FAST",      "lzturbo,10,10a,11,12,20,20a,21,22,30,30a,31,32/zlib,1,6,9/brotli,0,1,4,5/lz4,1/zstd,1,5,9/memcpy","lz4,lzturbo,zlib class" },
  { "EFFICIENT", "lzturbo,21,22,30,30a,31,32/brotli,4,5/zlib,5,6/zstd,5,9/zling,4/memcpy",							"Compression speed > 'zlib 6' class" },
  { "MAX",       "lzturbo,19,29,39,49/lzma,9/lzham,4/brotli,11/lz4,9/lizard,19,29,39,49/lzlib,9/zstd,22/memcpy",						"Best compression (slow)" },
  { "OPTIMAL",   "lzturbo,19,29,39,49/lzma,9/lzham,4/brotli,11/lz4,9/lizard,49/lzlib,19,29,39,49/zstd,22/zopfli/memcpy", 				"Optimal compression (slow)" },
  { "BWT",       "bsc_st,4,5/bsc,2/bcm/bzip2/memcpy/", 																		"ST & BWT" },
  { "ECODER",    "turbohf/turboanx/turborc/turborc_o1/turboac_byte/arith_static/rans_static16/rans_static16o1/subotin/fasthf/fastac/zlibh/fse/fsehuf/memcpy/", "Entropy coder" },
};
#define PLUGGSIZE (sizeof(plugg)/sizeof(plugg[0]))
#define INVLEV -999

void plugsprt(void) {
  struct plugs *gs;

    #if defined(_COMPRESS1) || defined(_COMPRESS2)
  struct plugg *pg; 
  printf("Codec group:\n");
  for(pg = plugg; pg < plugg+PLUGGSIZE; pg++) 
    printf("%-16s %s %s\n", pg->id, pg->desc);
    #endif

    #if defined(_ECODEC)
  printf("\nEntropy Coder group: 'ECODER' entropy coders\n");
    #endif

  printf("\nPlugins:\n");
  for(gs = plugs; gs->id >= 0; gs++) 
    if(gs->codec)
      { printf("%s %s\n", gs->s, gs->lev?gs->lev:""); fflush(stdout);}
}

void plugsprtv(FILE *f, int fmt) {
  struct plugs *gs;
  char         *pv = "";

  switch(fmt) {
    case FMT_HTMLT: 
    case FMT_HTML: 
      printf("%s\n", "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><title>TurboBench</title></head><body><pre><ul>"); 
      break;
    case FMT_VBULLETIN:
      fprintf(f,"[list]\n"); 
      break;
  }

  for(gs = plugs; gs->id >= 0; gs++)
    if(gs->codec && strcmp(gs->name,pv)) {
      pv = gs->name;
	  char name[65],ver[33]; 
      ver[0] = 0;
	  codver(gs->id, gs->ver, ver); 
      sprintf(name, "%s v%s", gs->name, ver);
      switch(fmt) {  
        case FMT_VBULLETIN: 
          fprintf(f, "[*][URL=\"%s\"]%s[/URL] %s\n", gs->url, name, gs->lic ); 
          break;
        case FMT_HTML     : 
          fprintf(f, "<li><a href=\"%s\">%s</a> %s\n", gs->url, name, gs->lic ); 
          break;
        case FMT_MARKDOWN :
          fprintf(f, " - [%s](%s) %s\n", name, gs->url, gs->lic ); 
          break;
        default:
          fprintf(f, "%-24s\t%s\t%s\n", name, gs->lic, gs->url );
      }
    }

  switch(fmt) {
    case FMT_VBULLETIN:
      fprintf(f,"[/list]\n"); 
      break;
    case FMT_HTML:
      fprintf(f,"</ul></pre></body></html>"); 
      break;
  }
}

//------------------ plugin: process ----------------------------------
#define PRM_SIZE 64
#define TMS_SIZE 20
struct plug { 
  int       id,err,blksize,lev;
  char      *s,prm[PRM_SIZE+1],tms[TMS_SIZE+1]; 
  long long len,memc,memd;
  double    tc,td,tck,tdk;
};

struct plug plug[255],plugt[255];
int         seg_ans = 32*1024, seg_huf = 32*1024, seg_anx = 12*1024, seg_hufx=11*1024;
static int  cmp = 2,trans;
int verbose=1;
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
  plug[k].lev = lev; 
  strncpy(plug[k].prm, prm?prm:(char *)"", PRM_SIZE); plug[k].prm[PRM_SIZE] = 0;
  plug[k].tms[0]  = 0;
  if(gs->flag & E_ANS)  
    plug[k].blksize = seg_ans;
  else if(gs->flag & E_HUF) 
    plug[k].blksize = seg_huf;
  else plug[k].blksize = gs->blksize && !bsizex?gs->blksize:bsize;
  *pk = ++k;
  return 0;
}

int plugreg(struct plug *plug, char *cmd, int k, int bsize, int bsizex) {
  static char *cempty=""; 
  int ignore = 0;

  while(*cmd) { 
    while(isspace(*cmd)) 
      cmd++; 
    char *name = cmd; 
    while(isalnum(*cmd) || *cmd == '_' || *cmd == '-') 
      cmd++; 
    if(*cmd) *cmd++ = 0;

    if(!strcmp(name, "ON" )) { 
      ignore = 1; 
      continue; 
    }
    else if(!strcmp(name, "OFF")) { 
      ignore = 0; 
      continue; 
    }

    for(;;) {																			
      while(isspace(*cmd) || *cmd == ',') 
        cmd++;

      char *prm = cmd; 									
      int lev = strtol(cmd, &cmd, 10); 
      if(prm == cmd) { 
        lev = INVLEV; 
        prm = cempty; 
      }
      else if(isalnum(*cmd) || *cmd == ':') {
        prm = cmd;
        while(isalnum(*cmd) || *cmd == '_' || *cmd == '-'  || *cmd == ':' || *cmd == '=') 
          cmd++; 
        if(*cmd) 
          *cmd++ = 0; 
      } else 
        prm = cempty;

      int found = 0;
      struct plugs *gs,*gfs=NULL;  
      if(!*name) 
        break;
      for(gs = plugs; gs->id >= 0; gs++)
        if(gs->codec && !strcasecmp(gs->s, name) ) { 
          char s[33],*q; 
          sprintf(s,"%d", lev); 
          found++; 
          if(lev==INVLEV && gs->lev && !gs->lev[0] || gs->lev && (q=strstr(gs->lev, s)) && (q==gs->lev || *(q-1) == ',')) {				
            found++; 
            plugins(plug, gs, &k, bsize, bsizex, lev, prm); 
          }
          break; 
        }
      if(found<2 && !ignore) {
        if(!found) 
          fprintf(stderr, "codec '%s' not found\n", name);
        else if(lev == INVLEV) 
          fprintf(stderr, "level [%s] not specified for codec '%s'\n", gs->lev, name );
        else if(gs->lev && gs->lev[0]) 
          fprintf(stderr, "level '%d' for codec '%s' not in range [%s]\n", lev, name, gs->lev);
        else 
          fprintf(stderr, "codec '%s' has no levels\n", name);
        exit(0); 
      }
      while(isspace(*cmd)) 
        cmd++;						
      if(*cmd != ',' && (*cmd < '0' || *cmd > '9')) 
        break;
    }
  } 
  a:plug[k].id = -1;  
  return k;
}

//------------------ plugin: print/plot -----------------------------
  #ifndef _WIN32
#define CRED     "\x1b[31m"
#define CGREEN   "\x1b[32m"
#define CYELLOW  "\x1b[33m"
#define CBLUE    "\x1b[34m"
#define CMAGENTA "\x1b[35m"
#define CCYAN    "\x1b[36m"
#define CRESET   "\x1b[0m"

#define BOLDB  "\033[1;32m"
#define BOLDE  CRESET
  #endif

struct bandw {
  unsigned long long bw;
  unsigned           rtt; 
  char               *s;
};

static struct bandw bw[] = {
  {    7*KB, 500, "GPRS 56"  },//56kbps
  {   57*KB, 150, "2G 456"   },
  {  125*KB,  40, "3G 1M"    },
  {  250*KB,   5, "DSL 2M"   },//DSL 2000
  {  500*KB,  20, "4G 4M"    },
  { 3750*KB,   5, "WIFI 30M" },
  {12500*KB,   5, "CAB 100M" },
  {   40*MB,   0, "USB2 40MB"},
  {  125*MB,   0, "ETH 1000" },
  {  200*MB,   0, "HDD 200MB"},
  {  550*MB,   0, "SSD 550MB"},
  {   1u*GB,   0, "SSD 1GB"  },
  {   2u*GB,   0, "SSD 2GB"  },
  { 4ull*GB,   0, "4GB/s"    },
  { 8ull*GB,   0, "8GB/s"    }
};
#define BWSIZE (sizeof(bw)/sizeof(struct bandw))

void plugprth(FILE *f, int fmt, char *t) {
  char *plot  = "<script src=https://cdn.plot.ly/plotly-latest.min.js></script>";
  char *jquery = "<script src=\"http://ajax.googleapis.com/ajax/libs/jquery/1/jquery.min.js\"></script>";
  char *tstyle = "<link rel=\"stylesheet\" href=\"http://tablesorter.com/themes/blue/style.css\" type=\"text/css\" media=\"print, projection, screen\" />";
  char *table  = "<script type=\"text/javascript\" src=\"http://tablesorter.com/__jquery.tablesorter.min.js\"></script>";
  char *code   = "<script type=\"text/javascript\">$(function() {		$(\"#myTable\").tablesorter({sortList:[[0,0],[2,1]], widgets: ['zebra']});		$(\"#options\").tablesorter({sortList: [[0,0]], headers: { 3:{sorter: false}, 4:{sorter: false}}});	});	</script><script type=\"text/javascript\" src=\"http://tablesorter.com/__jquery.tablesorter.min.js\"></script><script type=\"text/javascript\">$(function() {		$(\"#myTable2\").tablesorter({sortList:[[0,0],[2,1]], widgets: ['zebra']});		$(\"#options\").tablesorter({sortList: [[0,0]], headers: { 3:{sorter: false}, 4:{sorter: false}}});	});	</script>";
  char s[128];
  time_t tm; 
  time(&tm);
  sprintf(s, "TurboBench: %s - %s", t, asctime(localtime(&tm)));

  switch(fmt) {
    case FMT_TEXT:     
      fprintf(f,"%s\n", s ); 
      break;
    case FMT_VBULLETIN:
      fprintf(f,"%s\n", s); 
      break;
    case FMT_HTMLT:  
      fprintf(f,"<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><title>TurboBench: %s - </title></head><body>\n", s); 
      break;
    case FMT_HTML:     
      fprintf(f,"<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><title>TurboBench: %s - </title>%s%s%s%s%s</head><body>\n", s, plot, jquery, tstyle, table, code); 
       break;
    case FMT_MARKDOWN: 
      fprintf(f,"#### %s (bold = pareto)  MB=1.000.000\n", s); 
      break;
  }
}

void plugprtf(FILE *f, int fmt) {
  switch(fmt) {
    case FMT_HTML:     
      fprintf(f,"</body></html>\n"); 
      break;
  }
}

void plugprtth(FILE *f, int fmt) {
  char *head =  "     C Size  ratio%     C MB/s     D MB/s   Name            File              (bold = pareto)"; 

  switch(fmt) {
    case FMT_TEXT:     
      fprintf(f,"      C Size  ratio%%     C MB/s     D MB/s   Name            File\n"); 
      break;
    case FMT_VBULLETIN:
      fprintf(f,"[CODE][B]%s[/B] MB=1.000.0000\n", head); 
      break;
    case FMT_HTMLT:    
      fprintf(f,"<pre><b>%s</b> MB=1.000.0000\n", head); 
      break;
    case FMT_HTML:     
      fprintf(f,"<h3>TurboBench: Compressor Benchmark</h3><table id='myTable' class='tablesorter' style=\"width:35%%\"><thead><tr><th>C Size</th><th>ratio%%</th><th>C MB/s</th><th>D MB/s</th><th>Name</th><th>C Mem</th><th>D Mem</th><th>File</th></tr></thead><tbody>\n"); 
      break;
    case FMT_MARKDOWN: 
      fprintf(f,"|C Size|ratio%|C MB/s|D MB/s|Name|File|\n|--------:|-----:|--------:|--------:|----------------|----------------|\n"); 
      break;
    case FMT_CSV:      
      fprintf(f,"size,csize,ratio,ctime,dtime,name,file\n"); 
      break;
    case FMT_TSV:      
      fprintf(f,"size\tcsize\tratio\tctime\tdtime\tname\tfile\n"); 
      break;
    case FMT_SQUASH:   
      fprintf(f,"dataset,plugin,codec,level,compressed_size,compress_cpu,compress_wall,decompress_cpu,decompress_wall\n"); 
      break;
  }
}

void plugprttf(FILE *f, int fmt) {
  switch(fmt) {
    case FMT_VBULLETIN:
      fprintf(f,"[/CODE]\n"); 
      break;
    case FMT_HTMLT:    
      fprintf(f,"</pre>\n"); 
      break;
    case FMT_HTML:
      fprintf(f,"</tbody></table>\n"); 
      break;
    case FMT_MARKDOWN: 
      fprintf(f,"\n\n"); 
      break;
  }
}

#define RATIO(_clen_, _len_)  ((double)_clen_*100.0/_len_)
#define FACTOR(_clen_, _len_) ((double)_len_/(double)_clen_)

void plugprt(struct plug *plug, long long totinlen, char *finame, int fmt, double *ptc, double *ptd, FILE *f) {
  double ratio  = RATIO(plug->len,totinlen),    
         //ratio  = FACTOR(plug->len,totinlen),
         tc     = TMBS(totinlen,plug->tc), td = TMBS(totinlen,plug->td);
  char   name[256]; 
  if(plug->lev != INVLEV) 
    sprintf(name, "%s%s %d%s", plug->err?"?":"", plug->s, plug->lev, plug->prm);
  else
    sprintf(name, "%s%s%s",    plug->err?"?":"", plug->s,            plug->prm);
  
  int c = 0, d = 0, n = 0;
  if(tc > *ptc) { c++; n++; *ptc = tc; } 
  if(td > *ptd) { d++; n++; *ptd = td; } 
  switch(fmt) {
    case FMT_TEXT:     
      if(f == stdout) {
          #ifdef _WIN32
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        fprintf(f, "%12"PRId64"   %5.1f   ", plug->len, ratio); 
        #define BBOLD 15
        if(c) SetConsoleTextAttribute(h, BBOLD);
        fprintf(f, "%8.2f   ", tc); 
        if(c) SetConsoleTextAttribute(h, 7);
        if(d) SetConsoleTextAttribute(h, BBOLD);
        fprintf(f, "%8.2f   ", td); 
        if(d) SetConsoleTextAttribute(h, 7);
        if(n) SetConsoleTextAttribute(h, BBOLD);
        fprintf(f, "%-16s", name); 
        if(n) SetConsoleTextAttribute(h, 7);
        fprintf(f, "%s\n", finame); 
		#undef BBOLD
          #else
        fprintf(f, "%12"PRId64"   %5.1f   %s%8.2f%s   %s%8.2f%s   %s%-16s%s%s\n", 
          plug->len, ratio, c?BOLDB:"", tc, c?BOLDE:"",  d?BOLDB:"", td, d?BOLDE:"", n?BOLDB:"", name, n?BOLDE:"", finame); 
          #endif
      } 
      else
        fprintf(f,"%12"PRId64"   %5.1f   %8.2f   %8.2f   %-32s %s\n", plug->len, ratio, tc, td, name, finame); 
      break;
    case FMT_VBULLETIN:
      fprintf(f, "%12"PRId64"   %5.1f   %s%8.2f%s   %s%8.2f%s   %s%-16s%s%s\n", 
        plug->len, ratio, c?"[B]":"", tc, c?"[/B]":"",  d?"[B]":"", td, d?"[/B]":"", n?"[B]":"", name, n?"[/B]":"", finame); 
      break;
    case FMT_HTMLT:    
      fprintf(f, "%12"PRId64"   %5.1f   %s%8.2f%s   %s%8.2f%s   %s%-16s%s%s\n", 
        plug->len, ratio, c?"<b>":"", tc, c?"</b>":"",  d?"<b>":"", td, d?"</b>":"", n?"<b>":"", name, n?"</b>":"", finame); 
      break;
    case FMT_HTML:     
      fprintf(f, "<tr><td align=\"right\">%11"PRId64"</td><td align=\"right\">%5.1f</td><td align=\"right\">%s%8.2f%s</td><td align=\"right\">%s%8.2f%s</td><td>%s%-16s%s</td><td align=\"right\">%"PRId64"</td><td align=\"right\">%"PRId64"</td><td>%s</td></tr>\n",
        plug->len, ratio, c?"<b>":"", tc, c?"</b>":"",  d?"<b>":"", td, d?"</b>":"", n?"<b>":"", name, n?"</b>":"", 
//        SIZE_ROUNDUP(plug->memc, Kb)/Kb, SIZE_ROUNDUP(plug->memd,Kb)/Kb, 
        plug->memc, plug->memd, 
        finame); 
      break;
    case FMT_MARKDOWN: 
      fprintf(f, "|%"PRId64"|%5.1f|%s%.2f%s|%s%.2f%s|%s%s%s|%s|\n", 
        plug->len, ratio, c?"**":"",  tc, c?"**":"",    d?"**":"",  td, d?"**":"",   n?"**":"",  name, n?"**":"",   finame); 
      break;
    case FMT_CSV:
      fprintf(f, "%12"PRId64",%11"PRId64",%5.1f,%8.2f,%8.2f,%-16s,%s\n",
        totinlen, plug->len, ratio, tc, td, name, finame); 
      break;
    case FMT_TSV:    
      fprintf(f,"%12"PRId64"\t%11"PRId64"\t%5.1f\t%8.2f\t%8.2f\t%-16s\t%s\n",
        totinlen, plug->len, ratio, tc, td, name, finame); 
      break;
    case FMT_SQUASH:
      fprintf(f,"%12"PRId64",%11"PRId64",%5.1f,%8.2f,%8.2f,%-16s,%s\n",
        finame, name, name, plug->len,        tc, tc, td, td); 
      break;
  }
}

static int blknum, speedup;
enum { SP_SPEEDUPC=1, SP_SPEEDUPD, SP_TRANSFERC, SP_TRANSFERD };

void plugprtph(FILE *f, int fmt) {
  int i;

  switch(fmt) {
    case FMT_HTML: 
      fprintf(f,"<p><h3>TurboBench: Speedup %s sheet</h3><table id='myTable2' class='tablesorter' style=\"width:80%%\"><thead><tr><th>Name</th>", (speedup&1)?"compression":"decompression");
      for(i = 0; i < BWSIZE; i++) 
        fprintf(f, "<th>%s</th>", bw[i].s);
      fprintf(f, "<td>File"); 
      if(blknum) 
        fprintf(f, " blknum=%d ", blknum);
      fprintf(f, "</td></tr></thead><tbody>\n"); 
      break;
    case FMT_MARKDOWN: 
      fprintf(f,"#### TurboBench: Speedup %s sheet\n\n", (speedup&1)?"compression":"decompression");
      fprintf(f, "|Name"); 
      for(i = 0; i < BWSIZE; i++) 
        fprintf(f, "|%s", bw[i].s);
      fprintf(f, "|File"); 
      if(blknum) 
        fprintf(f, " blknum=%d ", blknum);
      fprintf(f, "|\n"); 
      fprintf(f, "|-------------");
      for(i = 0; i < BWSIZE; i++) 
        fprintf(f, "|---------:");
      fprintf(f, "|-------------|\n"); 
      break;
    case FMT_VBULLETIN:
      fprintf(f,"TurboBench: Speedup %s sheet\n\n", (speedup&1)?"compression":"decompression");
      fprintf(f,"[CODE][B]\n"); 
    default: 
      fprintf(f,"Name           ");
      for(i = 0; i < BWSIZE; i++) 
        fprintf(f, "%10s", bw[i].s);
      if(blknum) 
        fprintf(f, " blknum=%d ", blknum);
      fprintf(f, "\n"); 
    if(fmt == FMT_VBULLETIN) 
      fprintf(f,"[/B]\n"); 
  }
}

static inline double spmbs(double td, long long len, int i, long long totinlen) { 
  double t = td + len*TM_T/(double)bw[i].bw + blknum*(bw[i].rtt*1000.0); 
  return TMBS(totinlen,t); 
}

//static inline double spdup(double td, long long len, int i, long long totinlen) { double t = td + len*TM_T/(double)bw[i].bw + blknum*(bw[i].rtt*1000.0); return ((double)totinlen*TM_T*100.0/t)/(double)bw[i].bw;}
static inline double spdup(double td, long long len, int i, long long totinlen) { 
  return (double)totinlen*100.0 / ((double)len + ((td+blknum*bw[i].rtt*1000.0)/TM_T)*(double)bw[i].bw ); 
}

void plugprtp(struct plug *plug, long long totinlen, char *finame, int fmt, int speedup, FILE *f) {
  int  i;
  char name[255]; 
  if(plug->lev != INVLEV) 
    sprintf(name, "%s%s%s%d%s", plug->err?"?":"", plug->s, fmt==FMT_MARKDOWN?"_":" ", plug->lev, plug->prm);
  else
    sprintf(name, "%s%s%s",    plug->err?"?":"", plug->s,            plug->prm);
  if(fmt == FMT_HTML) 
    fprintf(f, "<tr><td>%s</td>", name);
  else 
    fprintf(f, "%-32s", name);															 

  for(i = 0; i < BWSIZE; i++) {
    switch(fmt) {
      case FMT_HTMLT: 
      case FMT_HTML: 
        fprintf(f, "<td align=\"right\">"); 
        break;
      case FMT_MARKDOWN: 
        fprintf(f, "|"); 
        break;
    }
    switch(speedup) {
      case SP_TRANSFERD: 
        fprintf(f,"%9.3f ", spmbs(plug->td, plug->len, i, totinlen)); 
        break;
      case SP_SPEEDUPD:  
        fprintf(f,"%9d ", (int)(spdup(plug->td, plug->len, i, totinlen)+0.5)); 
        break;
      case SP_TRANSFERC: 
        fprintf(f,"%9.3f ", spmbs(plug->td, plug->len, i, totinlen)); 
        break;
      case SP_SPEEDUPC:  
        fprintf(f,"%9d ", (int)(spdup(plug->td, plug->len, i, totinlen)+0.5)); 
        break;
    }
    switch(fmt) {
      case FMT_HTMLT: 
      case FMT_HTML: 
        fprintf(f, "</td>");
        break;
      case FMT_MARKDOWN: 
        break;
    }
  }
  switch(fmt) {
    case FMT_HTMLT: 
    case FMT_HTML: 
      fprintf(f, "<td>%s</td></tr>\n", finame);
      break;
    case FMT_MARKDOWN: 
      fprintf(f, "|%s|\n", finame); 
      break;
    default: 
      fprintf(f, "%s\n", finame); 
      break;
  }
}

struct { unsigned x,y; } divplot[] = { 
  { 1920, 1080}, // 16:9
  { 1600,  900}, 
  { 1280,  720}, 
  {  800,  600} 
};

static unsigned divxy = 1, xlog = 1, xlog2, ylog, ylog2, plotmcpy;

void plugplotb(FILE *f, int fmt, int idiv) { 
  fprintf(f, "<div id='myDiv%d' style='width: %dpx; height: %dpx;'></div><script>", idiv, divplot[divxy].x, divplot[divxy].y); 
}

void plugplot(struct plug *plug, long long totinlen, int fmt, int speedup, char *s, FILE *f) {
  int  i;
  char name[65];
  if(plug->lev != INVLEV)
    sprintf(name, "%s%s_%d%s", plug->err?"?":"", plug->s, plug->lev, plug->prm);
  else
    sprintf(name, "%s%s%s",    plug->err?"?":"", plug->s,            plug->prm);
  strcat(s,name); strcat(s,",");

  fprintf(f, "var %s = { x: [", name);							
  for(i = 0; i < BWSIZE; i++) 
    fprintf(f,"%llu%s", bw[i].bw, i+1 < BWSIZE?",":""); 			
  fprintf(f, "],\ny: [");							

  for(i = 0; i < BWSIZE; i++)  				
    switch(speedup) {
      case SP_TRANSFERD: 
        fprintf(f,"%9.3f%s",    spmbs(plug->td, plug->len, i, totinlen), i+1 < BWSIZE?",":""); 
        break;
      case SP_SPEEDUPD:  
        fprintf(f,"%9d%s", (int)(spdup(plug->td, plug->len, i, totinlen)+0.5), i+1 < BWSIZE?",":""); 
        break;
      case SP_TRANSFERC: 
        fprintf(f,"%9.3f%s",    spmbs(plug->tc, plug->len, i, totinlen), i+1 < BWSIZE?",":""); 
        break;
      case SP_SPEEDUPC:  
        fprintf(f,"%9d%s", (int)(spdup(plug->tc, plug->len, i, totinlen)+0.5), i+1 < BWSIZE?",":""); 
        break;
    }															   
  fprintf(f, "],\ntype: 'scatter',\nmode: 'lines+markers',\nline: {shape: 'spline'},\nname: '%s'\n};\n", name);							 
}

void plugplote(FILE *f, int fmt, char *s) {
  fprintf(f, "var data = [%s];\nvar layout = {\ntitle:'TurboBench Speedup: Transfer+%s Speed',\nxaxis: {\ntitle: '%s Transfer Speed (M=MB/s B=GB/s)',\n%s    autorange: true\n  }, \n  yaxis: {\n\ntitle: 'Speedup %%',\n%sautorange: true\n  }\n};\nPlotly.plot('myDiv1', data, layout);</script>\n",
    s, (speedup&1)?"Compression":"Decompression", xlog?"log":"", xlog?"type: 'log',\n":"", ylog?"type: 'log',\n":"");
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

int libcmpn(const struct plug *e1, const struct plug *e2) {
  int c = strcmp(e1->s, e2->s);
  if(c < 0)
    return -1;
  else if(c > 0)
    return 1;
  else if(e1->lev < e2->lev)
    return -1;
  else if(e1->lev > e2->lev)
    return 1;
  return 0;
}

#define P_MCPY 1  // memcpy id
void plugplotc(struct plug *plug, int k, long long totinlen, int fmt, int speedup, char *s, FILE *f) {
  int  i, n = 0;
  char name[65],txt[256];  
  qsort(plug, k, sizeof(struct plug), (int(*)(const void*,const void*))libcmpn);
  
  struct plug *g,*gs=plug,*p;
  for(txt[0] = name[0] = 0, g = plug; g < plug+k; g++) 
  if(g->id <= P_MCPY && !plotmcpy) 
    continue;
  else { 					
    if(strcmp(g->s, name)) {
      if(name[0]) { 														
        fprintf(f, "],\ny: [");
        for(p = gs; p < g; p++) 
          fprintf(f, "%.2f%s", speedup<3?FACTOR(p->len,totinlen):RATIO(p->len,totinlen), p+1<g?",":"");        
        fprintf(f, "],\nmode: 'markers+text',\ntype: 'scatter',\nname: '%s',\ntextposition: 'top center', textfont: { family:  'Raleway, sans-serif' }, marker: { size: 12 }\n", name, txt);	
        if(txt[0]) 
          fprintf(f, "\n,text: [%s]\n", txt);
        fprintf(f, "};\n");
        strcat(s,",");
        txt[0] = 0;
      }
      gs = g;
      strcpy(name, g->s);													
      fprintf(f, "var %s = {\n x: [", g->s);
      strcat(s, g->s); 
    } else { 
      fprintf(f, ",");
      strcat(txt, ","); 
    }
    if(g->lev != INVLEV) { 
      char ts[33]; 
      sprintf(ts, "'%s%s%d%s'", divxy>=2?"":g->s, divxy>=2?"":",", g->lev, g->prm); 
      strcat(txt, ts); 
    }
    double t = (speedup&1)?g->tc:g->td;
    fprintf(f, "%.2f", TMBS(totinlen,t));
  }
  fprintf(f, "],\ny: [");
  for(p = gs; p < g; p++) 
    fprintf(f, "%.2f%s", speedup<3?FACTOR(p->len,totinlen):RATIO(p->len,totinlen), p+1<g?",":"");        
  fprintf(f, "],\nmode: 'markers+text',\ntype: 'scatter',\nname: '%s',\ntextposition: 'top center', textfont: { family:  'Raleway, sans-serif' }, marker: { size: 12 }\n", name, txt);	
  if(txt[0]) 
    fprintf(f, "\n,text:[%s]\n", txt);
  fprintf(f, "};\n");
}

void plugplotce(FILE *f, int fmt, char *s) {
  fprintf(f, "var data = [%s];\nvar layout = {\ntitle:'TurboBench: %s',\nxaxis: {\ntitle: '%s speed MB/s',\n%s    autorange: true\n  }, \n  yaxis: {\n\ntitle: 'Ratio (factor)',\n%sautorange: true\n  }\n};\nPlotly.plot('myDiv2', data, layout);</script>\n",
    s, (speedup&1)?"Compression":"Decompression", xlog2?"log":"", xlog2?"type: 'log',\n":"", ylog2?"type: 'log',\n":"");
}

int plugprts(struct plug *plug, int k, char *finame, int xstdout, unsigned long long totlen, int fmt, char *t) { 
  double ptc = 0.0, ptd = 0.0;
  struct plug *g; 
  if(!totlen) return 0; 														  					if(verbose>1) printf("'%s'\n", finame); 

  qsort(plugt, k, sizeof(struct plug), (int(*)(const void*,const void*))libcmp);
  char s[257];
  sprintf(s, "%s.%s", finame, fmtext[fmt]);
  FILE *fo = xstdout>=0?stdout:fopen(s, "w");
  if(!fo) 
    die("file create error for '%s'\n", finame); 
  plugprth( fo, fmt, t);
  plugprtth(fo, fmt);
  for(g = plugt; g < plugt+k; g++) 
    plugprt(g, totlen, finame, fmt, &ptc, &ptd, fo);
  plugprttf(fo, fmt);

  if(speedup) { 
    switch(fmt) {
      case FMT_TEXT : 
        fprintf(fo, "\n"); 
        break;
      case FMT_HTML : 
        break;
      case FMT_HTMLT: 
        fprintf(fo, "<pre>\n");
        break;
      case FMT_MARKDOWN :
        fprintf(fo, "\n"); 
        break;
    }
    plugprtph(fo, fmt); 
    for(g = plugt; g < plugt+k; g++) 
      plugprtp(g, totlen, finame, fmt, speedup, fo);  
    fprintf(fo, "\n"); 
    switch(fmt) {
      case FMT_TEXT : 
        fprintf(fo, "\n"); break;
      case FMT_HTML : 
        fprintf(fo, "</tbody></table>\n"); break;
      case FMT_HTMLT: 
        fprintf(fo, "</pre>\n"); 
        break;
      case FMT_VBULLETIN:
        fprintf(fo,"[/CODE]\n"); 
        break;
      case FMT_MARKDOWN :
        fprintf(fo, "\n"); 
        break;
    }
    if(fmt == FMT_HTML) {
      char s[1025];  
      s[0] = 0; 															if(verbose>1) printf("generate speedup plot\n");
      plugplotb(fo, fmt, 1); 
      for(g = plugt; g < plugt+k; g++) 
        if(g->id > P_MCPY || plotmcpy)
          plugplot(g, totlen, fmt, speedup, s, fo);  
      plugplote(fo, fmt, s);

      s[0] = 0; 															if(verbose>1) printf("generate speed/ratio plot\n");
      plugplotb(fo, fmt, 2); 
      plugplotc(plug, k, totlen, fmt, speedup, s, fo);
      plugplotce(fo, fmt, s);

    }
  }
  plugprtf(fo, fmt);
  fclose(fo);
} 

int plugread(struct plug *plug, char *finame, long long *totinlen) {
  char s[LSIZE+1],name[33];
  struct plug *p=plug;
  FILE *fi = fopen(finame, "r"); 
  if(!fi) return -1;

  fgets(s, LSIZE, fi);
  for(p = plug;;) {
    p->tms[0] = name[0] = 0;
	char ss[LSIZE+1],*t = ss,*q;
	if(!fgets(ss, LSIZE, fi)) break; ss[strlen(ss)-1]=0; if(!strlen(ss)) break; 

	for(q = t;  *q && *q != '\t'; q++);  *q++ = 0; strncpy(s, t, LSIZE); s[LSIZE]=0; t = q;
	*totinlen = strtoull(t, &t, 10);   
	p->len    = strtoull(++t, &t, 10);
	p->td     = strtod(  ++t, &t);
	p->tc     = strtod(  ++t, &t);
	for(q = ++t; *q && *q != '\t'; q++); *q++ = 0; strncpy(name,t,32); name[32]=0; t=q;
	p->lev    = strtol(t, &t, 10); 
	while(*t && isspace(*t)) t++;
	for(q = t; *q && *q != '\t'; q++);   *q++ = 0; strncpy(p->prm, t, PRM_SIZE); p->prm[PRM_SIZE]=0; t = q;
	p->memc   = strtoull(t, &t, 10);
    p->memd   = strtoull(++t, &t, 10);
    for(q = ++t; *q && *q != '\t'; q++); *q++ = 0; strncpy(p->tms, t, TMS_SIZE); p->tms[TMS_SIZE]=0; t = q;
    if(p->prm[0]=='?') 
      p->prm[0]=0;
    int i;  							
    for(i = 0; plugs[i].id >=0; i++) {  
      if(!strcmp(name, plugs[i].s)) { 
        p->s  = plugs[i].s; 
        p->id = plugs[i].id; 											if(verbose>1) { fprintf(stdout, "$%s\t%"PRId64"\t%"PRId64"\t%.6f\t%.6f\t%s\t%d%s\t%s\t%"PRId64"\t%"PRId64"\n", s, *totinlen, p->len, p->td, p->tc, p->s, p->lev, p->prm, p->tms, p->memc, p->memd); fflush(stdout); }
        p++;
        break; 
      } 
    }	  
  }
  fclose(fi);
  return p - plug;
}

//----------------------------------- Benchmark -----------------------------------------------------------------------------
  #ifndef min
#define min(x,y) (((x)<(y)) ? (x) : (y))
#define max(x,y) (((x)>(y)) ? (x) : (y))
  #endif

static int mcpy=0, mode, tincx, fuzz;

int becomp(unsigned char *_in, unsigned _inlen, unsigned char *_out, unsigned outsize, unsigned bsize, int id, int lev, char *prm) { 
  unsigned char *op,*oe = _out + outsize;
  TMBEG(tm_rep,tm_Rep);     mempeakinit();                                           
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
      if(bs == 2 && oplen >= (1<<16)) { printf("Output larger than input! Use option '-P'\n"); exit(-1); }
      bs==2?(ctou16(op) = oplen):(ctou32(op) = oplen); op += oplen+bs; ip += iplen; 
      if(op > _out+outsize) 
	    die("Overflow error %llu, %u in lib=%d\n", outsize, (int)(ptrdiff_t)(op - _out), id);                                                      
    }
  }
  TMEND(_inlen);	
  return op - _out;;
}

int bedecomp(unsigned char *_in, int _inlen, unsigned char *_out, unsigned _outlen, unsigned bsize, int id, int lev, char *prm) { 
  unsigned char *ip;
  TMBEG(tm_rep2,tm_Rep2);     mempeakinit();
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
	  else l = coddecomp(ip, iplen, op, oplen, id, lev,prm);
      ip += iplen; op += oplen;
    }
  }
  TMEND(_outlen);
  return ip - _in;
}

  #ifdef LZTURBO
#include "../dev/x/bebench.h"
  #else
struct plug plugr[32]; int tid;
#define BEPRE
#define BEINI
#define BEPOST
#define BEOPT
#define BEUSAGE
#define BEFILE
#define BENCHSTA
#endif

#define INOVD 4*1024

  #if defined(_WIN32) && !defined(__MINGW__)
int getpagesize() {
  static int pagesize = 0;
  if (pagesize == 0) {
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    pagesize = max(system_info.dwPageSize, system_info.dwAllocationGranularity);
  }
  return pagesize;
} 
  #endif

unsigned mininlen;

unsigned long long plugfile(struct plug *plug, char *finame, unsigned long long filenmax, unsigned bsize, struct plug *plugr, int tid, int krep) {
  size_t outsize;   
  FILE *fi = strcmp(finame,"stdin")?fopen(finame, "rb"):stdin; if(!fi) { perror(finame); return 0; /*die("open error '%s'\n", finame);*/ }
  char *p; 
  if((p = strrchr(finame, '\\')) || (p = strrchr(finame, '/'))) finame = p+1; 	if(verbose>1) printf("'%s'\n", finame);
  p = finame; 

  char name[65]; 
  if(plug->lev != INVLEV) 
    sprintf(name, "%s %d%s", plug->s, plug->lev, plug->prm);
  else
    sprintf(name, "%s%s",    plug->s,            plug->prm);
 
  unsigned long long filen;
  if(finame) {
    fseeko(fi, 0, SEEK_END); filen = ftello(fi); fseeko(fi , 0 , SEEK_SET); if(filen > filenmax) filen = filenmax;
  } else 
    filen = filenmax;
  
  size_t insize   = min(filen,(1u<<MAP_BITS)); 											if(filen < mininlen) insize = mininlen;
  int    pagesize = getpagesize();
  size_t insizem  = (fuzz&3)?SIZE_ROUNDUP(insize, pagesize):(insize+INOVD);

  outsize = insize*fac + 10*Mb; 
  unsigned char *_in = NULL;
  if(insizem && !(_in = _valloc(insizem,1)))
    die("malloc error in size=%u\n", insizem);
  
  unsigned char *_cpy = _in, *out = (unsigned char*)_valloc(outsize,2);  		if(!out) die("malloc error out size=%u\n", outsize);

  if((cmp || tid) && insizem && !(_cpy = _valloc(insizem,3)))
    die("malloc error cpy size=%u\n", insizem);
 
  codini(insize, plug->id);	
  int       inlen;																	
  long long totinlen = 0;
  double    ptc = DBL_MAX, ptd = DBL_MAX;
  bsize     = plug->blksize;
  plug->len = plug->tc = plug->td = 0; 											blknum = 0;	

  while((inlen = fread(_in, 1, insize, fi)) > 0) {    
    unsigned char *in = _in; 
    if(fuzz & 1) { in = (_in+insizem)-inlen; memmove(in, _in, inlen); /*in[inlen-1] = in[inlen];*/ }
    double   tc = 0.0, td = 0.0;         
    unsigned l = inlen,outlen;
	totinlen += inlen;																
    BEPRE;		
																				memrcpy(out, in, inlen);
    int nb = 1;
    if(l < mininlen) {
      bsize = l;
      unsigned char *p;
      for(p = in+l; ; p+=l) {
        if(p+l > in+insize) break;
        nb++;
        memcpy(p, in, l);
      }
    }
    size_t peak = mempeakinit();
	outlen = becomp(in, l*nb, out, outsize, bsize, plug->id, plug->lev, plug->prm)/nb;
	plug->len += outlen; 
    plug->tc  += (tc += (double)tm_tm/((double)tm_rm*nb)); 
	plug->memc = mempeak() - peak;
    if(tm_Rep > 1) 
      TMSLEEP;
																		if(verbose && inlen == filen) { double ratio = (double)outlen*100.0/inlen; printf("%12u   %5.1f   %8.2f   ", outlen, ratio, TMBS(inlen,tc)); fflush(stdout); }
    if(cmp) {
      unsigned char *cpz = _cpy; 
      if(fuzz & 2) cpz = (_cpy+insizem) - l;
	  if(_cpy != _in) memrcpy(cpz, in, l);
      peak = mempeakinit();
	  unsigned cpylen = bedecomp(out, outlen, cpz, l*nb, bsize, plug->id,plug->lev, plug->prm)/nb; 
	  td = (double)tm_tm/((double)tm_rm*nb);		
      plug->memd = mempeak() - peak;                                    if(verbose && inlen == filen) { printf("%8.2f   %-16s %s\n", TMBS(inlen,td), name, finame); }
      int e = memcheck(in, l, cpz, fuzz?3:cmp);  
      plug->err = plug->err?plug->err:e;
      BEPOST;																	
 	  plug->td += td; 
	} else 																if(verbose && inlen == filen) { printf("%8.2f   %-16s %s\n", 0.0, name, finame); }
	if(totinlen >= filen) 
      break;
  }	  
  _vfree(out, outsize); 
  _vfree(_in, insizem);
  if(_cpy && _cpy != _in) 
    _vfree(_cpy, insizem); 
  codexit(plug->id);
  fclose(fi); 
  if(verbose && filen > insize) 
    plugprt(plug, totinlen, finame, FMT_TEXT, &ptc, &ptd,stdout);
  return totinlen;
}

void usage(char *pgm) {
  fprintf(stderr, "\nTurboBench Copyright (c) 2013-2017 Powturbo %s\n", __DATE__);
  fprintf(stderr, "Usage: %s [options] [file]\n", pgm);
  fprintf(stderr, " -eS      S = compressors/groups separated by '/' Parameter can be specified after ','\n");
  fprintf(stderr, " -b#s     # = blocksize (default filesize). max=1GB\n");
  fprintf(stderr, " -d#      # = log2 dictionary size: 15-30 (default blocksize).Only brotli,lzham,lzlib,lzma,zstd\n");
  fprintf(stderr, " -B#s     # = max. benchmark filesize (default 1GB) ex. -B4G\n");
  fprintf(stderr, " -s#s     # = min. buffer size to duplicate & test small files (ex. -s50)\n");
  fprintf(stderr, "          s = modifier s:K,M,G=(1000, 1.000.000, 1.000.000.000) s:k,m,h=(1024,1Mb,1Gb). (default m) ex. 64k or 64K\n");
  fprintf(stderr, "Benchmark:\n");
  fprintf(stderr, " -i#/-j#  # = Minimum  de/compression iterations per run (default=auto)\n");
  fprintf(stderr, " -I#/-J#  # = Number of de/compression runs (default=3)\n");
  fprintf(stderr, " -t#      # = min. time in seconds per run.(default=2sec)\n");
  fprintf(stderr, " -S#      Sleep # min. after 2 min. processing mimizing CPU trottling\n");
  fprintf(stderr, " -k#      Repeat all benchmarks # times (default=3). -k0 = test mode\n");
  fprintf(stderr, " -K#t     Max. time limit for all benchmarks (default 24h)\n");
  fprintf(stderr, "          t = M:millisecond s:second m:minute h:hour. ex. 3h\n");
  fprintf(stderr, " -D       No process real-time priority setting\n");
  fprintf(stderr, "Check:\n");
  fprintf(stderr, " -C#      #=0 compress only, #1=No check #=2 ignore errors, #=3 exit on error, #=4 crash on error\n");
  fprintf(stderr, " -f#      check reading/writing outside bounds: #=1 compress, #=2 decompress, #3:both\n");
  fprintf(stderr, "Output:\n");
  fprintf(stderr, " -v#      # = verbosity 0..3 (default 1)\n");
  fprintf(stderr, " -rstr    str = Remark/Comment string\n");
  fprintf(stderr, " -l#      # = 1 : print all groups/plugins, # = 2 : print all codecs\n");
  fprintf(stderr, " -S#      Plot transfer speed: #=1 Comp speedup #=2 Decomp speedup #=3 Comp 'MB/s' #=4 Decomp 'MB/s'\n");
  fprintf(stderr, " -p#      #='print format' 1=text 2=html 3=htm 4=markdown 5:vBulletin 6:csv(comma) 7=tsv(tab)\n");
  fprintf(stderr, " -Q#      # Plot window 0:1920x1080, 1:1600x900, 2:1280x720, 3:800x600 (default=1)\n");
  fprintf(stderr, " -g       -g:no merge w/ old result 'file.tbb', -gg:process w/o output (use for fuzzing)\n");
  fprintf(stderr, " -o       print on standard output\n");
  fprintf(stderr, " -G       plot memcpy\n");
  fprintf(stderr, " -w       Plot Speedup linear x-axis (default log)\n");
  fprintf(stderr, " -z       Plot Ratio/Speed logarithmic x-axis (default linear)\n");
  fprintf(stderr, "Multiblock:\n");
  fprintf(stderr, " -Moutput concatenate all input files to multiple blocks file output\n");\
  fprintf(stderr, " -m       process multiple blocks per file.\n");
  BEUSAGE;
  fprintf(stderr, "ex. ./turbobench enwik9 -eFAST/bzip2/lzma,5,9\n");
  fprintf(stderr, "ex. ./turbobench enwik9 -eFAST/OPTIMAL/bsc,0:e2 -i0\n");
  fprintf(stderr, "ex. ./turbobench eECODER -R\"entropy coder test\"\n");
  fprintf(stderr, "ex. ./turbobench enwik9 -elzma,9:fb273:lc2:lp2\n");
  exit(0);
} 

void printfile(char *finame, int xstdout, int fmt, char *rem) {
  long long totinlen;
  int       k = plugread(plugt, finame, &totinlen); 
  char      *p, s[256];
  if(k < 0)
    die(stderr, "file open error for '%s'\n", finame);

  if(!k) return;
  strncpy(s, finame, 255); 
  s[255]=0;
  if((p = strrchr(s,'.')) && !strcmp(p, ".tbb"))
    *p=0;
  plugprts(plugt, k, s, xstdout, totinlen, fmt, rem);	
} 

  #ifdef __MINGW32__
extern int _CRT_glob = 1;
  #endif
unsigned xbits[256] = {0};
#define XSTPRT { int i; for(i = 0; i < 256; i++) if(xbits[i]) printf("%d:%d,", i, xbits[i]); }
#define XSTPRT if(xbits[1]) printf("%d:%d", xbits[0], xbits[0]/xbits[1])
//#define XSTDEF 
//#define XSTINC(_x_) 
//#define XSTPRT 

//XSTDEF;

void ansprt(void) { XSTPRT; }

int main(int argc, char* argv[]) { 
  int xstdout=-1,xstdin=-1;
  int                recurse  = 0, xplug = 0,tm_Repk=1,plot=-1,fmt=0,fno,merge=0,rprio=1;
  unsigned           bsize    = 1u<<30, bsizex=0;
  unsigned long long filenmax = 0;
  char               *scmd = NULL,*trans=NULL,*beb=NULL,*rem="",s[2049];
  char               *_argvx[1], **argvx=_argvx;

  int c, digit_optind = 0;
  for(;;) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
      { "help", 	0, 0, 'h'},
      { 0, 		    0, 0, 0}
    }; 
    if((c = getopt_long(argc, argv, "0:1:2:3:4:5:6:7:8:9:A:b:B:C:d:e:E:F:f:gGi:I:j:J:k:K:l:L:mM:N:oOPp:Q:rRs:S:t:T:Uv:V:W:w:X:x:Y:y:Z:z:", long_options, &option_index)) == -1) break;
    switch(c) { 
      case 0:
        printf("Option %s", long_options[option_index].name);
        if(optarg) printf (" with arg %s", optarg);  printf ("\n");
        break;
      case 'b': bsize    = argtoi(optarg,Mb); bsizex++; break;
      case 'B': filenmax = argtol(optarg);    		 break;
      case 'd': coddicsize(argtoi(optarg,0));        break; 
      case 'C': cmp      = atoi(optarg);      		 break;
      case 'e': scmd     = optarg;            		 break;
      case 'F': fac      = strtod(optarg, NULL); 	 break;
      case 'f': fuzz     = atoi(optarg);       		 break;
      case 'g': merge++;		 			 		 break;
      case 'G': plotmcpy++;	 			 		 	 break;

      case 'i': if((tm_rep  = atoi(optarg))<=0) tm_rep =tm_Rep=1; break;
      case 'I': if((tm_Rep  = atoi(optarg))<=0) tm_rep =tm_Rep=1; break;
      case 'j': if((tm_rep2 = atoi(optarg))<=0) tm_rep2=tm_Rep2=1; break;
      case 'J': if((tm_Rep2 = atoi(optarg))<=0) tm_rep2=tm_Rep2=1; break;
      case 'k': if((tm_Repk = atoi(optarg))<=0) tm_rep=tm_Rep=tm_rep2=tm_Rep2=tm_Repk=1; break;
      //case 'K': tm_RepkT = argtot(optarg);     		 break;
      case 'L': tm_slp   = atoi(optarg);      		 break;
 	  case 't': tm_tx    = atoi(optarg)*TM_T; 		 break;
 	  case 'T': tm_TX    = atoi(optarg)*TM_T; 		 break;
      case 'r': rem      = optarg;		      		 break;
      case 'S': speedup  = atoi(optarg);       		 break;

      case 'l': xplug    = atoi(optarg);             break;
      case 'm': mode++; 		 			 		 break;
      case 'o': xstdout++; 							 break;
      case 'p': fmt      = atoi(optarg);             break;
      case 'P': mcpy++;       		 			     break;	  
      case 'Q': divxy    = atoi(optarg); 
                if(divxy>3) divxy=3;                 break;
      case 'D': rprio=0;		 			 		 break;
      case 's': mininlen = argtoi(optarg,1);    	 break;
      case 'v': verbose  = atoi(optarg);       		 break;
      case 'Y': seg_ans  = argtoi(optarg,1);         break;
      case 'Z': seg_huf  = argtoi(optarg,1);         break;  
      case 'w': xlog     =  xlog?0:1; 				 break;
      case 'x': ylog     =  ylog?0:1;                break;
      case 'y': xlog2    = xlog2?0:1;                break;
      case 'z': ylog2    = ylog2?0:1;                break;
        #ifdef LZTURBO
      case 'M': beb      = optarg; 		 			 break; 
        #else
      case 'M': fprintf(stderr, "Option M: only in binary package available"); exit(0);
        #endif
      BEOPT;
	  case 'h':
      default: 
        usage(argv[0]);
        exit(0); 
    }
  }
  if(xplug) { 
    xplug==1?plugsprt():plugsprtv(stdout, fmt); 
    exit(0); 
  }

  if(argc <= optind) {
      #ifdef _WIN32
    setmode( fileno(stdin), O_BINARY ); 
      #endif
    argvx[0] = "stdin";
    optind   = 0;
    argc     = 1;   
    recurse  = 0;
  } else 
    argvx = argv;

  if(fmt) {
    if(argc <= optind) { printf("no input file specified"); exit(0); }
    for(fno = optind; fno < argc; fno++)
      printfile(argvx[fno], xstdout, fmt, rem);
    exit(0);
  }
  if((tm_rep|tm_Rep|tm_rep2|tm_Rep2) ==1) 
    tm_Repk = 1;
  if(rprio) { 
      #ifdef _WIN32
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
      #else
    setpriority(PRIO_PROCESS, 0, -19);
	  #endif
  }
  if(!scmd) scmd = "FAST";
  for(s[0] = 0;;) {
    char *q; 
	int i=0;
    if(!*scmd) break;
    if(q = strchr(scmd,'/')) *q = '\0';
	FILE *fi = fopen("turbobench.ini", "r");
	if(fi) {
	  char ss[LSIZE+1];
	  while(fgets(ss, LSIZE, fi)) {
        char *t = ss,*u;  																	
        while(isspace(*t)) t++; u = t; while(isalnum(*u) || ispunct(*u)) u++; *u = 0; 
        if(!strcmp(scmd, t)) {  
		  for(t=++u;isspace(*t);t++); u = t; while(isalnum(*u) || ispunct(*u)) u++; *u = 0;
          strcat(s, "/ON/"); 
          strcat(s, t);
          strcat(s, "/OFF/"); 
		  i = 1;
          break;
        }
	  }
	  fclose(fi);
	}
	if(!i)
      for(i = 0; i < PLUGGSIZE; i++) 
        if(!strcmp(scmd, plugg[i].id)) { 
          strcat(s, "/ON/"); 
          strcat(s, plugg[i].s); 
          strcat(s, "/OFF/"); 
          break;
        }		
    if(i >= PLUGGSIZE) { 
      strcat(s,scmd); 
      strcat(s,"/"); 
    }
    scmd = q?(q+1):(char*)"";
  }
  
  unsigned k = plugreg(plug, s, 0, bsize, bsizex);
  if(k > 1 && argc == 1 && !strcmp(argvx[0],"stdin")) { printf("multiple codecs not allowed when reading from stdin"); exit(0); }

  BEINI;
  if(!filenmax) filenmax = Gb; 
  long long totinlen = 0;  
  int       krep;
  struct    plug *p;
  char     *finame = "";
  tm_t      tmk0 = tminit();      
  for(p = plugt; p < plugt+k; p++) p->tc = p->td = DBL_MAX; 
  for(krep = 0; krep < tm_Repk; krep++) { 
    if(tm_Repk > 1)
      printf("Benchmark: %d from %d\n", krep+1, tm_Repk);
    for(p = plug; p < plug+k; p++) {
      struct plug *g = &plugt[p-plug];
	  totinlen = 0;  
      g->len = g->tck = g->tdk = g->memc = g->memd = 0;
      BEFILE;
      for(fno = optind; fno < argc; fno++) {
	    finame = argvx[fno];																			if(verbose > 1) printf("%s\n", finame);	
        totinlen += plugfile(p, finame, filenmax, bsize, plugr, tid, krep);
	    g->len += p->len;
	    g->tck += p->tc;
	    g->tdk += p->td;
        g->err  = g->err?g->err:p->err;  								
        if(p->memc > g->memc) g->memc = p->memc;
        if(p->memd > g->memd) g->memd = p->memd;						
	  }
	  g->id  = p->id;
      g->s   = p->s;
      g->lev = p->lev;
      strcpy(g->prm, p->prm);
      if(g->tck < g->tc) g->tc = g->tck;
      if(g->tdk < g->td) g->td = g->tdk;      //if(tmtime() - tmk0 > tm_RepkT) break;
    } 
  }
    BENCHSTA;

  if(argc - optind > 1) {
    unsigned clen = strpref(&argvx[optind], argc-optind, '\\', '/');
    strncpy(s, argvx[optind], clen);
    if(clen && (s[clen-1] == '/' || s[clen-1] == '\\')) 
      clen--;
    s[clen] = 0; 
    finame = strrchr(s,'/'); 
    if(!finame) 
      finame = strrchr(s, '\\'); 
    if(!finame) 
      finame = s; 
    else finame++;
  } else {
    char *p;  
    if((p = strrchr(finame, '\\')) || (p = strrchr(finame, '/'))) 
      finame = p+1;
  }
  ansprt();

  sprintf(s, "%s.tbb", finame);
  if(merge /*|| tm_rep <= 1 && tm_rep2 <= 1*/) {
    if(merge == 1) 
      plugprts(plugt, k, s, 1, totinlen, FMT_TEXT, rem);	
    exit(0);
  }

  long long _totinlen;
  int       gk = plugread(plug, s, &_totinlen);  
  if(_totinlen != totinlen) 
    gk = 0;                  
  FILE *fo = fopen(s, "w");
  if(fo) {
    char tms[20];
    time_t tm; 
    time(&tm);    
	struct tm *ltm = localtime(&tm); 
	sprintf(tms, "%.4d-%.2d-%.2d.%.2d:%.2d:%.2d", 1900 + ltm->tm_year, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
	
    struct plug *g;
    fprintf(fo, "dataset\tsize\tcsize\tdtime\tctime\tcodec\tlevel\tparam\tcmem\tdmem\ttime\n");
    for(p = plugt; p < plugt+k; p++) { 																
      for(g = plug; g < plug+gk; g++) { 
        if(g->id >= 0 && !strcmp(g->s, p->s) && g->lev == p->lev && !strcmp(g->prm, p->prm)) {
          if(g->len == p->len) {
            int u = 0;
            if(g->td < p->td || p->td < 0.01) 
			  p->td = g->td,u++;
            if(g->tc < p->tc || p->tc < 0.01) 
			  p->tc = g->tc,u++;

            if(g->memd != p->memd) u++;
            if(g->memc != p->memc) u++;
            strcpy(p->tms, u?tms:g->tms);
          } 
          g->id = -1;
          break; 
        }
      } 
      fprintf(fo,   "%s\t%"PRId64"\t%"PRId64"\t%.6f\t%.6f\t%s\t%d\t%s\t%"PRId64"\t%"PRId64"\t%s\n", finame, totinlen, p->len, p->td, p->tc, p->s, p->lev, p->prm[0]?p->prm:"?", p->memc, p->memd, p->tms[0]?p->tms:tms);
    }
    for(g = plug; g < plug+gk; g++) 
      if(g->id >= 0) {
        fprintf(fo, "%s\t%"PRId64"\t%"PRId64"\t%.6f\t%.6f\t%s\t%d\t%s\t%"PRId64"\t%"PRId64"\t%s\n", finame, totinlen, g->len, g->td, g->tc, g->s, g->lev, g->prm[0]?g->prm:"?", g->memc, g->memd, g->tms[0]?g->tms:tms);
      }
	fclose(fo);
    printfile(s, 0, FMT_TEXT, rem);
  }
}
