/**
    Copyright (C) powturbo 2013-2026
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
//   s TurboBench: main program
#define _CRT_SECURE_NO_WARNINGS
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE 1
  #if defined(__CYGWIN__) && !defined(_WIN32)
#define _WIN32
  #endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <float.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <ctype.h>
  #ifdef _MSC_VER
#include "vs/getopt.h"
  #else
#include <getopt.h>
#include <unistd.h>
  #endif
  #ifdef _WIN32
#include <io.h>
#include <fcntl.h>
  #else
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
  #endif
  #ifdef __APPLE__
#include <malloc/malloc.h>
#define malloc_usable_size malloc_size
  #else
#include <malloc.h>
  #endif

#include <time.h>
#include "conf.h"
#include "time_.h"
#include "cpu.h"
#include "plugin.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define RATIO(_clen_, _len_)           ((double)(_clen_)*100.0/(double)(_len_))
#define FACTOR(_clen_, _len_)          ((double)(_len_)/(double)(_clen_))
//#define SCORE(_clen_, _len_) (plug->tc + 10.0 * plug->td + (double)_clen_/1000000.0)
#define SCORE(_plug_, _len_)  _plug_->rank
static char _cpubrand[65];
static unsigned memout;

double weissman(double ratio, double bandwith, double bandwithlo, double bandwithhi ) {
  return ratio * log10( 1 + bandwith/(bandwithlo*ratio) ) - (bandwithhi > 0?ratio * log10( 1 + bandwith/(bandwithhi*ratio) ):0.0);
  //return ratio * log10( ( ratio + bandwith/bandwithlo ) / ( ratio + bandwith/bandwithhi ) )
}

int strpref(const char *const *str, int n, char sep1, char sep2) {
  int i, j=0;
  for(;;j++)
    for(i = 0; i < n; i++)
      if(!str[i][j] || str[i][j] != str[0][j]) {
    while(j > 0 && str[0][j-1] != sep1 && str[0][j-1] != sep2) j--;
    return j;
      }
  return 0;
}

int memcheck(unsigned char *in, unsigned n, unsigned char *cpy, int cmp, char *finame) {
  int i;
  if(cmp <= 1)
    return 0;
  for(i = 0; i < n; i++)
    if(in[i] != cpy[i]) {
      if(cmp > 3) abort(); // crash (AFL) fuzzing
      printf("ERROR at %d:%x, %x file=%s\n", i, in[i], cpy[i], finame);
      if(cmp > 2) exit(EXIT_FAILURE);
      return i+1;
    }
  return 0;
}
//------------------------------- malloc ------------------------------------------------
void vmemset(char *p, char c, size_t size) {
  volatile char _c = c;
  for(size_t i = 0; i < size; i++) ((char * volatile)p)[i] = _c;
}

//#define USE_MMAP
  #if __WORDSIZE == 64
#define MAP_BITS 30
  #else
#define MAP_BITS 28
  #endif

void *_valloc(size_t size, unsigned a) {
  if(!size) return NULL;
    #if defined(_WIN32)
  return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    #elif defined(USE_MMAP)
  void *ptr = mmap(NULL/*0(size_t)a<<MAP_BITS*/, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
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
#define mem_init()
#define memused() 0
  #elif defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
static volatile size_t g_baseline = 0;
static volatile size_t g_peak = 0;
static volatile LONG g_sampling = 0;
static HANDLE g_thread = NULL;
static HANDLE g_ready = NULL;
static inline size_t wmemused(void) {
  PROCESS_MEMORY_COUNTERS_EX pmc = {};
  pmc.cb = sizeof(pmc);
  if(!GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    return 0;
  return (size_t)pmc.PrivateUsage;
}
static inline void atomic_max(volatile size_t* dest, size_t value) {
  size_t prev = *dest;
  while(value > prev) {
    size_t old = InterlockedCompareExchange((volatile LONG_PTR*)dest, (LONG_PTR)value, (LONG_PTR)prev);
    if(old == prev) break;
    prev = old;
  }
}
static DWORD WINAPI sampler_thread(LPVOID) {
  SetEvent(g_ready);
  while(InterlockedCompareExchange(&g_sampling, 1, 1) == 1) {
    size_t cur = wmemused();
    atomic_max(&g_peak, cur);
    Sleep(0); // yield; use a small Sleep(1) if CPU matters
  }
  return 0;
}
static inline size_t mempeakinit(void) { if(!memout) return 0;
  g_baseline = wmemused();
  g_peak     = g_baseline;
  InterlockedExchange(&g_sampling, 1);
  g_ready = CreateEvent(NULL, TRUE, FALSE, NULL);
  if(!g_ready) return 0;
  g_thread = CreateThread(NULL, 0, sampler_thread, NULL, 0, NULL);
  if(!g_thread) {
    CloseHandle(g_ready);
    g_ready = NULL;
    InterlockedExchange(&g_sampling, 0);
    return 0;
  }
  WaitForSingleObject(g_ready, 100); // wait until sampler is actually running
  // SetThreadPriority(g_thread, THREAD_PRIORITY_HIGHEST); // optional
  return g_baseline;
}
static inline size_t mempeak(void) { if(!memout) return 0;
  InterlockedExchange(&g_sampling, 0);
  if(g_thread) {
    WaitForSingleObject(g_thread, 2000);
    CloseHandle(g_thread);
    g_thread = NULL;
  }
  if(g_ready) {
    CloseHandle(g_ready);
    g_ready = NULL;
  }
  size_t cur = wmemused();
  atomic_max(&g_peak, cur); 
  return (g_peak > g_baseline) ? (g_peak - g_baseline) : 0;
}
#else
static size_t mem_peak, mem_used;
size_t mempeak() { return mem_peak; }
size_t memused() { return mem_used; }

size_t mempeakinit() { mem_peak = mem_used = 0; return mem_peak; }

void mem_add(size_t size) { if((mem_used += size) > mem_peak) mem_peak = mem_used; }
void mem_sub(size_t size) {  //if(size > mem_used) { fflush(stdout); printf("\nfree: warning size=%zu > mem_used=%zu\n", size, mem_used); } 
  mem_used -= size > mem_used ? mem_used : size; 
}

#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE
#include <dlfcn.h>

static ALIGNED(char, mem_heap[1<<20],32);
static char *mem_heapp = mem_heap;

static void *(*mem_malloc)(size_t);
static void *(*mem_calloc)(size_t, size_t);
static void *(*mem_realloc)(void*, size_t);
static void  (*mem_free)(void *);
static void *(*mem_memalign)(size_t, size_t);
static int   (*mem_posix_memalign)(void**, size_t, size_t);
static void *(*mem_aligned_alloc)(size_t, size_t);

static __attribute__((constructor)) void mem_init(void) {
  mem_malloc   = dlsym(RTLD_NEXT, "malloc" );
  mem_realloc  = dlsym(RTLD_NEXT, "realloc");
  mem_free     = dlsym(RTLD_NEXT, "free"   );
  mem_calloc   = dlsym(RTLD_NEXT, "calloc" );   
    #if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200112L) || defined(_POSIX_ADVISORY_INFO) && (_POSIX_ADVISORY_INFO >= 200112L) || defined(__APPLE__)
  if(!(mem_posix_memalign = dlsym(RTLD_NEXT, "posix_memalign"))) die("mem_posix_memalign not found\n");
    #endif
    #ifndef __APPLE__
  mem_memalign = dlsym(RTLD_NEXT, "memalign");
    #endif   
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L 
  mem_aligned_alloc = dlsym(RTLD_NEXT, "aligned_alloc");
    #endif
  if(!mem_malloc || !mem_calloc || !mem_realloc || !mem_free)
    die("malloc not found. mem_malloc:%d mem_calloc:%d mem_realloc:%d mem_free:%d\n", mem_malloc?1:0, mem_calloc?1:0, mem_realloc?1:0, mem_free?1:0);
}

void *malloc(size_t size) {
  if(!mem_malloc) {
    void *p = mem_heapp;
    if((mem_heapp += size) >= mem_heap+sizeof(mem_heap))
      die("malloc:initial memory overflow\n");
    return p;
  }
  void *p = (*mem_malloc)(size);
  if(p) mem_add(malloc_usable_size(p));
  return p;
}

void *calloc(size_t alignment, size_t size) {
  size_t _size = alignment*size;
  if(!mem_calloc) {
    void *p = mem_heapp;
    if((mem_heapp += _size) >= mem_heap + sizeof(mem_heap))
      die("calloc:initial memory overflow\n");
    memset(p, 0, _size);
    return p;
  }
  void *p = (*mem_calloc)(alignment, size);
  if(p) mem_add(malloc_usable_size(p));
  return p;
}

void *memalign(size_t alignment, size_t size) {
  if(!memalign) die("memalign");
  void *p = (*mem_memalign)(alignment, size);
  if(p) mem_add(malloc_usable_size(p));
  return p;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
  size_t _size = alignment*size;
  if(!posix_memalign) {
    void *p = mem_heapp;
    if((mem_heapp += _size) >= mem_heap + sizeof(mem_heap))
      die("posix_memalign:initial memory overflow\n");
    memset(p, 0, _size);
    *memptr = p;
    return 0;
  }
  int rc = (*mem_posix_memalign)(memptr, alignment, size);
  if(*memptr) mem_add(malloc_usable_size(*memptr));
  return rc;
}

void *aligned_alloc(size_t alignment, size_t size) {
  if(!aligned_alloc) die("aligned_alloc");
  void *p = (*aligned_alloc)(alignment, size);
  if(p) mem_add(malloc_usable_size(p));
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

#define STACK_MAGIC  0x79a53fb6u

#ifdef _WIN32
  #ifndef STACK_PAINT_BYTES
#define STACK_PAINT_BYTES  (128 * 1024)
  #endif
#define STACK_PAINT_WORDS  (STACK_PAINT_BYTES / sizeof(unsigned))

typedef struct { unsigned *low, *high; } memstack_t;

NOINLINE memstack_t stackini(void) {
  volatile unsigned marker = 0;
  volatile unsigned *p     = (volatile unsigned *)(uintptr_t)&marker;
  unsigned          *high  = (unsigned *)(uintptr_t)p;

  for(size_t i = 0; i < STACK_PAINT_WORDS; i++) *--p = STACK_MAGIC;
  memstack_t r = { .low  = (unsigned *)(uintptr_t)p, .high = high };
  return r;
}

size_t stackpeak(memstack_t paint) {
  if(!paint.low || !paint.high || paint.high <= paint.low)
    return 0;

  volatile unsigned *p   = (volatile unsigned *)paint.low;
  volatile unsigned *end = (volatile unsigned *)paint.high;

  while(p < end && *p == STACK_MAGIC)
    ++p;
  return (size_t)((char *)end - (char *)p);
}

#elif defined(__linux__)
#include <pthread.h>
typedef unsigned* memstack_t;
static unsigned *g_stack_lo;   // lowest usable address                
static unsigned *g_stack_hi;   // one-past the highest usable address

NOINLINE unsigned *stackini(void) {
  pthread_attr_t attr;
  void           *addr;
  size_t         size;
  if(pthread_getattr_np(pthread_self(), &attr)) return NULL;
  if(pthread_attr_getstack(&attr, &addr, &size)) {
    pthread_attr_destroy(&attr);
    return NULL;
  }
  pthread_attr_destroy(&attr);
  g_stack_lo = (unsigned *)addr;
  g_stack_hi = (unsigned *)((unsigned char *)addr + size);
  volatile unsigned guard;  
  unsigned          *from = (unsigned *)&guard;
  if(from > g_stack_hi) from = g_stack_hi;
  volatile unsigned *sp = from;
  while(sp > g_stack_lo) *--sp = STACK_MAGIC;
  return g_stack_lo;
}

size_t stackpeak(unsigned *base) {
  unsigned *p = base;
  if(!base || !g_stack_hi) return 0;
  while(p < g_stack_hi && *p == STACK_MAGIC) p++;
  return (size_t)(g_stack_hi - p) * sizeof(unsigned);
}
#else
#define stackini() 0
#define stackpeak(base) 0
typedef unsigned *memstack_t;
#endif

//----------------------------------------------------------------------------------------------------------------
#include <sys/stat.h>
#include <dirent.h>

#ifndef _WIN32
#include <limits.h>
#include <unistd.h>
#else
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif

#define RDIR_DEPTH 128

typedef struct {
  DIR *dirs[RDIR_DEPTH];
  DIR **pos;
  char prefix[PATH_MAX];
} rdirstack_t;

typedef struct {
  char        **list;
  rdirstack_t *stack;
} rdir_t;

rdir_t *rdiropen(char **paths) {
  if(!paths) return NULL;
  rdir_t *rdir = (rdir_t *)calloc(1, sizeof(rdir_t));       if(!rdir) return NULL;
  rdir->list = paths;
  rdir->stack = (rdirstack_t *)malloc(sizeof(rdirstack_t)); if(!rdir->stack) { free(rdir);  return NULL;  }
  return rdir;
}

void rdirclose(rdir_t *rdir) {
  if(!rdir) return;
  if(rdir->stack) {
    if(rdir->stack->pos) { /* Close any open directories left in the stack */
      while(rdir->stack->pos >= rdir->stack->dirs) {
        if(*(rdir->stack->pos))
          closedir(*(rdir->stack->pos));
        if(rdir->stack->pos == rdir->stack->dirs) break;
        rdir->stack->pos--;
      }
    }
    free(rdir->stack);
  }
  free(rdir);
}

// rdir_next: Retrieve the next file name in the sequence. Returns 0 on success, -1 when exhausted.
int rdirnext(rdir_t *rdir, char *name, struct stat *st_arg) {
  if(!rdir || !rdir->stack || !name || !st_arg) return -1;
  rdirstack_t *stk = rdir->stack;
  while(1) {
    if(stk->pos) {
      DIR *dircur = *(stk->pos);
      if(!dircur) {
        if(stk->pos == stk->dirs) stk->pos = NULL;
        else stk->pos--;
        continue;
      }
      struct dirent *ent = readdir(dircur);
      if(!ent) {
        closedir(dircur); // Reached end of current directory: close and pop
        if(stk->pos == stk->dirs) stk->pos = NULL;
        else stk->pos--;

        char *last_slash = strrchr(stk->prefix, '/'); // Truncate the prefix to the parent directory
          #ifdef _WIN32
        char *last_backslash = strrchr(stk->prefix, '\\');
        if(last_backslash && (!last_slash || last_backslash > last_slash)) last_slash = last_backslash;
          #endif
        if(last_slash && last_slash != stk->prefix) *last_slash = '\0';       // Normal truncation to parent
        else if(last_slash == stk->prefix)          *(last_slash + 1) = '\0'; // Keep root '/'
        else                                        stk->prefix[0] = '\0';    // Clear completely for relative roots
        continue;
      }

      if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
      size_t plen = strlen(stk->prefix);
      if(plen > 0 && (stk->prefix[plen - 1] == '/'
        #ifdef _WIN32
                || stk->prefix[plen - 1] == '\\'
        #endif
      )) {
        snprintf(name, PATH_MAX, "%s%s", stk->prefix, ent->d_name);
      } else if(plen > 0) snprintf(name, PATH_MAX, "%s/%s", stk->prefix, ent->d_name);
      else strncpy(name, ent->d_name, PATH_MAX);
      name[PATH_MAX - 1] = '\0';

      if(stat(name, st_arg) == 0) {
        if(S_ISDIR(st_arg->st_mode) && (stk->pos - stk->dirs) < (RDIR_DEPTH - 1)) {
          DIR *new_dir = opendir(name);
          if(new_dir) {
            stk->pos++;
            *(stk->pos) = new_dir;
            strncpy(stk->prefix, name, PATH_MAX - 1);
            stk->prefix[PATH_MAX - 1] = '\0';
          }
        }
        return 0; /* Successfully yielded an entry */
      }
    } else {
      if(!rdir->list || !*(rdir->list)) return -1; // finished
      char *current_path = *(rdir->list);
      rdir->list++;
      strncpy(name, current_path, PATH_MAX);
      name[PATH_MAX - 1] = '\0';
      if(stat(name, st_arg) == 0) {
        if(S_ISDIR(st_arg->st_mode)) {
          DIR *dir = opendir(name);
          if(dir) {
            stk->pos = stk->dirs;
            *(stk->pos) = dir;
            strncpy(stk->prefix, name, PATH_MAX - 1);
            stk->prefix[PATH_MAX - 1] = '\0';
          }
        }
        return 0;
      }
    }
  }
  return -1;
}

//--------------------------------------- TurboBench ------------------------------------------------------------------
enum {
  FMT_TEXT=1,
  FMT_HTML,
  FMT_HTMLT,
  FMT_MARKDOWN,
  FMT_VBULLETIN,  // ex. post to encode.ru
  FMT_VBULLETIN2,
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

struct plugg plugg[] = {
  { "TURBO",     "lzturbo,10,11,12,19/lz4,1,5,9,12/zxc,3,4,5,6/misa77,0,1,2,3,4/lzav,1,2/memcpy", "Fastest de-/compression. HDD/SSD/RAM speed" },
  { "FAST",      "lzturbo,10,10a,11,12/lz4,1,5,9/lzav,1,2/misa77,0,1,2,3,4/brotli,0,1,4,5/zlib,1,6,9/libdeflate,1,6,9/zlib_ng,1,6/igzip,1,2,3/zstd,1,5,9/zxc,3,4,5,6,7/memcpy", "lz4,lzturbo,zlib class" },
  { "EFFICIENT", "lzturbo,12/brotli,4,5/zlib,6/zstd,5,9/libdeflate,6/zlib-ng,6/igzip,3/memcpy", "Compression speed > 'zlib 6' class" },
  { "MAX",       "lzturbo,19/lzma,9/lzham,4/brotli,11/lz4,12/lizard,19,29,39,49/lzlib,9/libdeflate,12/zstd,22/zxc,6,7/misa77,4/zlib,9/zlib-ng,9/memcpy","Best compression (slow)" },
  { "OPTIMAL"    "lzturbo,19/lzma,9/lzham,4/brotli,11/lz4,12/libdeflate,12/lizard,49/lzlib,9/zstd,22/zopfli/memcpy", "Optimal compression (slow)" },
  { "BWT"        "bsc_st,4,5/bsc,2/bcm/bzip2/bzip3/turborc,20e8,20e9/memcpy", "ST & BWT" },
  { "ECODER"     "turbohf/turboanx/turborc/turborc_o1/turboac_byte/arith_static/rans_static16/rans_static16o1/subotin/fasthf/fastac/zlibh/fse/fsehuf/pivco,0,1/memcpy",  "Entropy coder" },
  { "MEMCPY"     "imemcpy/memcpy", "memcpy" },
  { "WEB"        "zlib,1,6,9/libdeflate,1,6,9,12/zlib_ng,1,6,9/igzip,0,1,2,3/zopfli/brotli,1,2,4,5,11/slz,1,6,9/zstd,1,9,15,22", "web/http compression"}
};

#define PLUGGSIZE (sizeof(plugg)/sizeof(plugg[0]))
#define INVLEV -9999

void plugsprt(void) {
  plugs_t *gs;

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
  plugs_t *gs;
  char         *pv = "";

  switch(fmt) {
    case FMT_HTMLT:
    case FMT_HTML:
      printf("%s\n", "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><title>TurboBench</title></head><body><pre><ul>");
      break;
    case FMT_VBULLETIN:
    case FMT_VBULLETIN2:
      fprintf(f,"[list]\n");
      break;
  }

  for(gs = plugs; gs->id >= 0; gs++)
    if(gs->codec && strcmp(gs->name,pv)) {
      pv = gs->name;
      char name[65],ver[33]; ver[0] = 0;
      sprintf(name, "%s %s", gs->name, codver(gs->id, ""/*gs->ver*/, ver));
      switch(fmt) {
         case FMT_VBULLETIN:
         case FMT_VBULLETIN2:
          fprintf(f, "[*]%s\n", name );
          break;
        case FMT_HTML     :
          fprintf(f, "<li>%s\n", name );
          break;
        case FMT_MARKDOWN :
          fprintf(f, " - %s\n", name );
          break;
        default:
          fprintf(f, "%-24s\n", name);
      }
    }

  switch(fmt) {
    case FMT_VBULLETIN:
    case FMT_VBULLETIN2:
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
typedef struct {
  int       id,err,lev;
  unsigned  blksize, rank;
  char      *s,prm[PRM_SIZE+1],tms[TMS_SIZE+1];
  unsigned long long len,memc,memd,stkc,stkd;
  double    tc,td,tck,tdk;
} plug_t;

//****************************************************** Rank Aggregation ************************************************************
#define MEDAL_KRONE  "\xF0\x9F\x91\x91"   /* 👑 U+1F451 */
#define MEDAL_CUP    "\xF0\x9F\x8F\x86"   /* 🏆 U+1F3C6 */
#define MEDAL_GOLD   "\xF0\x9F\xA5\x87"   /* 🥇 */
#define MEDAL_SILVER "\xF0\x9F\xA5\x88"   /* 🥈 */
#define MEDAL_BRONZE "\xF0\x9F\xA5\x89"   /* 🥉 */

static char *medal[] = { "", MEDAL_KRONE , MEDAL_CUP, MEDAL_GOLD, MEDAL_SILVER, MEDAL_BRONZE };
#define MEDALMAX     (sizeof(medal)/sizeof(medal[0]))
#define RANK_RATIO   0x01
#define RANK_COMP    0x02
#define RANK_DECOMP  0x04
static unsigned rankmode = 0xf;

/* Pairwise duel between candidates a and b, decided by majority vote over the 3 criteria (ratio: lower better, tc/td: higher better).
 * Returns +1 if a wins, -1 if b wins, 0 on an exact 1-1-1 style tie (only possible when two criteria disagree and are compensated). */
static int pairwise_winner(plug_t *plug, int a, int b) {
  int a_wins = 0, b_wins = 0;
  if(rankmode & RANK_RATIO)  { if(plug[a].len < plug[b].len) a_wins++; else if(plug[a].len > plug[b].len) b_wins++; }
  if(rankmode & RANK_COMP)   { if(plug[a].tc  < plug[b].tc ) a_wins++; else if(plug[a].tc  > plug[b].tc ) b_wins++; }
  if(rankmode & RANK_DECOMP) { if(plug[a].td  < plug[b].td ) a_wins++; else if(plug[a].td  > plug[b].td ) b_wins++; }
  if(a_wins > b_wins) return  1;
  if(b_wins > a_wins) return -1;
  return 0;
}

/* Borda count per criterion: the best candidate gets (k-1) points, the worst gets 0; candidates tied on a criterion split the points they
 * would occupy evenly. Scores from the 3 criteria are summed and used. ONLY to break Copeland ties. */
static void borda_scores(plug_t *plug, int k, double *borda) {
  int i, j;
  for(i = 0; i < k; i++) borda[i] = 0.0;
  if(rankmode & RANK_RATIO) for(i = 0; i < k; i++) {
    int better = 0, equal = 0;
    for(j = 0; j < k; j++) {
      if(j == i) continue;
      if(plug[j].len < plug[i].len) better++; else if(plug[j].len == plug[i].len) equal++;
    }
    borda[i] += (double)(k - 1 - better) - 0.5 * equal;
  }
  if(rankmode & RANK_COMP) for(i = 0; i < k; i++) {
    int better = 0, equal = 0;
    for(j = 0; j < k; j++) {
      if(j == i) continue;
      if(plug[j].tc < plug[i].tc) better++; else if(plug[j].tc == plug[i].tc) equal++;
    }
    borda[i] += (double)(k - 1 - better) - 0.5 * equal;
  }
  if(rankmode & RANK_DECOMP) for(i = 0; i < k; i++) {
    int better = 0, equal = 0;
    for(j = 0; j < k; j++) {
      if(j == i) continue;
      if(plug[j].td < plug[i].td) better++; else if(plug[j].td == plug[i].td) equal++;
    }
    borda[i] += (double)(k - 1 - better) - 0.5 * equal;
  }
}

// qsort comparator context (single-threaded use, mirrors typical small-utility C style)
static plug_t *g_plug;
static double *g_copeland;
static double *g_borda;

static int final_cmp(const void *pa, const void *pb) {
  int a = *(const int *)pa;
  int b = *(const int *)pb;
  if(g_copeland[a] != g_copeland[b]) return (g_copeland[a] < g_copeland[b]) ? 1 : -1;      /* desc */
  if(g_borda[a]    != g_borda[b])    return (g_borda[a]    < g_borda[b])    ? 1 : -1;            /* desc */
  if(rankmode & RANK_RATIO)  { if(g_plug[a].len != g_plug[b].len) return (g_plug[a].len > g_plug[b].len) ? 1 : -1; }      /* asc  */
  if(rankmode & RANK_COMP)   { if(g_plug[a].tc != g_plug[b].tc)   return (g_plug[a].tc  > g_plug[b].tc)  ? 1 : -1; }       /* asc  */
  if(rankmode & RANK_DECOMP) { if (g_plug[a].td != g_plug[b].td)  return (g_plug[a].td  > g_plug[b].td)  ? 1 : -1; }        /* asc  */
  return (g_plug[a].id > g_plug[b].id) ? 1 : -1;            /* deterministic */
}

// Aggregates len / tc / td into ONE global ranking using the Copeland pairwise method (primary) with a Borda-count tie-break   
// (secondary). Writes 1..k into plug[i].rank, 1 = best compressor. Equal candidates on every criterion share the same rank value.      *
void plugrank(plug_t *plug, int k) {
  int i, j;

  if(k <= 0) return;
  double *copeland = malloc((size_t)k * sizeof(double));
  double *borda    = malloc((size_t)k * sizeof(double));
  int    *order    = malloc((size_t)k * sizeof(int));
  if(!copeland || !borda || !order) { free(copeland); free(borda); free(order); return; }
  for(i = 0; i < k; i++) copeland[i] = 0.0;
  // --- all-pairs duels: O(k^2), fine for small fleets like IDNUM=30 ---
  for(i = 0; i < k; i++) {
    for(j = i + 1; j < k; j++) {
      int w = pairwise_winner(plug, i, j);
      if(w > 0)      { copeland[i] += 1.0; copeland[j] -= 1.0; }
      else if(w < 0) { copeland[j] += 1.0; copeland[i] -= 1.0; }
    }
  }
  borda_scores(plug, k, borda);
  for(i = 0; i < k; i++) { order[i] = i;     if(plug[i].id > 1) copeland[i] = 1e15; }
  g_plug = plug; g_copeland = copeland; g_borda = borda;
  qsort(order, (size_t)k, sizeof(int), final_cmp);
  plug[order[0]].rank = 1;
  for(i = 1; i < k; i++) {
    int prev = order[i - 1], cur = order[i];
    int identical = copeland[cur]     == copeland[prev] &&
                       borda[cur]     == borda[prev]    &&
                        plug[cur].len == plug[prev].len && plug[cur].tc  == plug[prev].tc && plug[cur].td  == plug[prev].td;
    plug[cur].rank = identical ? plug[prev].rank : (unsigned)(i + 1);
  }
  free(copeland);
  free(borda);
  free(order);
}

#define PLUGN 256
plug_t plug[PLUGN+1], plugt[PLUGN+1];
int         seg_ans = 32*1024, seg_huf = 32*1024; //, seg_anx = 12*1024, seg_hufx=11*1024;
static int  cmp = 2,trans;
int         verbose=1;
double      fac = 1.3;

int plugins(plug_t *plug, plugs_t *gs, int *pk, unsigned bsize, unsigned bsizex, int lev, char *prm) {
  int i,k = *pk;
  for(i = 0; i < k; i++)
    if(plug[i].id == gs->id && plug[i].lev == lev && !strcmp(plug[i].prm,prm))
      return -1;
  if(k >= PLUGN)
    die("Too many codecs specified\n");
  memset(&plug[k], 0, sizeof(plug_t));
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

int plugreg(plug_t *plug, char *cmd, int k, unsigned bsize, unsigned bsizex) {
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
      plugs_t *gs,*gfs=NULL;
      if(!*name)
        break;
      for(gs = plugs; gs->id >= 0; gs++)
        if(gs->codec && !strcasecmp(gs->s, name) ) {
          char s[33],*q;
          sprintf(s,"%d", lev);
          found++;
          if(lev == INVLEV && gs->lev && !gs->lev[0] || gs->lev && (q=strstr(gs->lev, s)) && (q==gs->lev || *(q-1) == ',')) {
            found++;
            plugins(plug, gs, &k, bsize, bsizex, lev, prm);
          }
          break;
        }
      if(found < 2 && !ignore) {
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
//***********************************************************************************************************************************************************
#define SVG_PLUGMAX 30
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
typedef enum { M_RATIO = 0, M_COMP = 1, M_DECOMP = 2 } metric_t;
#define CHART_W        990 //900
#define BAR_H          26
#define BAR_GAP        12
#define LEFT_MARGIN    150 //170
#define RIGHT_MARGIN   130 //150
#define TOP_MARGIN     70
#define BOTTOM_MARGIN  40

static void xml_escape(const char *in, char *out, size_t outsz) {
  size_t o = 0;
  for(; *in && o + 6 < outsz; in++) {
    const char *rep = NULL;
    switch (*in) {
      case '&': rep = "&amp;"; break;
      case '<': rep = "&lt;";  break;
      case '>': rep = "&gt;";  break;
       default:  out[o++] = *in; continue;
    }
    size_t l = strlen(rep);
    memcpy(out + o, rep, l);
    o += l;
  }
  out[o] = '\0';
}
static double max_metric(plug_t *a, int n, metric_t m, size_t len) {
  double mx = 0; int i;
  for(i = 0; i < n; i++) {
    double v = (m == M_RATIO) ? a[i].len : (m == M_COMP) ? a[i].tc : a[i].td;
    if(v > mx) mx = v;
  }
  if(m == M_RATIO) mx = RATIO(mx, len);
  return mx <= 0 ? 1 : mx;
}
static int cmp_ratio(const void *p1, const void *p2) { const plug_t *a = p1, *b = p2; return (b->len > a->len) - (b->len < a->len); }
static int cmp_tc(   const void *p1, const void *p2) { const plug_t *a = p1, *b = p2; return (b->tc > a->tc) - (b->tc < a->tc); }
static int cmp_td(   const void *p1, const void *p2) { const plug_t *a = p1, *b = p2; return (b->td > a->td) - (b->td < a->td); }

static void svg_open(FILE *f, int w, int h, const char *title) {
  char esc[256];
  xml_escape(title, esc, sizeof(esc));
  fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(f, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\" font-family=\"Arial,Helvetica,sans-serif\">\n", w, h, w, h);
  fprintf(f, "<rect x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"#ffffff\"/>\n", w, h);
  fprintf(f, "<text x=\"%d\" y=\"30\" font-size=\"20\" font-weight=\"bold\" text-anchor=\"middle\" fill=\"#222\">%s</text>\n", w / 2, esc);
}
static void svg_close(FILE *f) { fprintf(f, "</svg>\n"); }

static void svg_text(FILE *f, double x, double y, const char *anchor, int size, const char *color, const char *txt) {
  char esc[256];
  xml_escape(txt, esc, sizeof(esc));
  fprintf(f, "<text x=\"%.2f\" y=\"%.2f\" text-anchor=\"%s\" font-size=\"%d\" fill=\"%s\">%s</text>\n", x, y, anchor, size, color, esc);
}
static void svg_rect(FILE *f, double x, double y, double w, double h, const char *fill, double rx) {
  fprintf(f, "<rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" rx=\"%.1f\" fill=\"%s\"/>\n", x, y, w, h, rx, fill);
}
static void svg_line(FILE *f, double x1, double y1, double x2, double y2, const char *color, double sw) {
  fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" stroke=\"%s\" stroke-width=\"%.2f\"/>\n", x1, y1, x2, y2, color, sw);
}
static void svg_circle(FILE *f, double cx, double cy, double r, const char *fill, const char *stroke) {
  fprintf(f, "<circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" fill=\"%s\" stroke=\"%s\" stroke-width=\"1.5\"/>\n", cx, cy, r, fill, stroke);
}
typedef enum { SHAPE_CIRCLE=0, SHAPE_SQUARE, SHAPE_TRIANGLE_UP, SHAPE_TRIANGLE_DOWN, SHAPE_DIAMOND, SHAPE_CROSS, SHAPE_XMARK, SHAPE_STAR, SHAPE_PENTAGON, SHAPE_HEXAGON, SHAPE_COUNT } shape_t;
static void svg_poly_begin(FILE *f) { fprintf(f, "<polygon points=\""); }
static void svg_poly_pt(FILE *f, double x, double y) { fprintf(f, "%.2f,%.2f ", x, y); }
static void svg_poly_end(FILE *f, const char *fill, const char *stroke) {
  fprintf(f, "\" fill=\"%s\" stroke=\"%s\" stroke-width=\"1.5\"/>\n", fill, stroke);
}

// 1) HORIZONTAL BAR CHART - single metric (ratio | compression | decomp)
void chart_bar(const char *fname, char *name, plug_t *a, int n, metric_t metric, size_t len) {
  char s[256];
  if(n > SVG_PLUGMAX) n = SVG_PLUGMAX;
  sprintf(s, "%s_%s", fname, name);
  plug_t tmp[SVG_PLUGMAX];
  memcpy(tmp, a, n * sizeof(plug_t));

  int (*cmp)(const void *, const void *) =
  metric == M_RATIO ? cmp_ratio : metric == M_COMP ? cmp_tc : cmp_td;
  qsort(tmp, n, sizeof(plug_t), cmp);

  double mx = max_metric(tmp, n, metric, len);
  int h = TOP_MARGIN + n * (BAR_H + BAR_GAP) + BOTTOM_MARGIN;

  FILE *f = fopen(s, "w");
  if(!f) { perror(s); return; }

  char color[16];
  if(metric == M_RATIO) {
    snprintf(s, 80, "TurboBench: Ratio '%s'", fname);
    strcpy(color, "#4C72B0");
  } else if(metric == M_COMP) {
    snprintf(s, 80, "TurboBench: C Speed MB/s '%s'", fname);
    strcpy(color, "#2E86AB");
  } else {
    snprintf(s, 80, "TurboBench: D Speed MB/s '%s'", fname);
    strcpy(color, "#E67E22");
  }
  svg_open(f, CHART_W, h, s);

  double plot_w = CHART_W - LEFT_MARGIN - RIGHT_MARGIN;
  svg_line(f, LEFT_MARGIN, TOP_MARGIN - 10, LEFT_MARGIN, h - BOTTOM_MARGIN + 5, "#333", 1.5);

  int i;
  for(i = 0; i < n; i++) {
    double v = metric == M_RATIO ? RATIO(tmp[i].len,len): metric == M_COMP ? tmp[i].tc : tmp[i].td;
    double y = TOP_MARGIN + i * (BAR_H + BAR_GAP);
    double bw = (v / mx) * plot_w;
    if(tmp[i].lev==INVLEV) sprintf(s, "%s", tmp[i].s); else sprintf(s, "%s,%d", tmp[i].s, tmp[i].lev);
    svg_text(f, LEFT_MARGIN - 10, y + BAR_H * 0.7, "end", 13, "#222", s);
    svg_rect(f, LEFT_MARGIN, y, bw, BAR_H, color, 4);
    char lbl[64];
    if(metric == M_RATIO)      snprintf(lbl, sizeof(lbl), "%.1f%%", v);
    else if(metric == M_COMP)  snprintf(lbl, sizeof(lbl), "%.1f", v);
    else                        snprintf(lbl, sizeof(lbl), "%.1f", v);
    svg_text(f, LEFT_MARGIN + bw + 6, y + BAR_H * 0.7, "start", 12, "#111", lbl);
  }
  svg_close(f);
  fclose(f);
}

// *** 2) GROUPED HORIZONTAL BAR CHART - compression + decompression speed
void chart_grouped(const char *fname, char *name, plug_t *a, int n, size_t len) {
  char s[256];
  if(n > SVG_PLUGMAX) n = SVG_PLUGMAX;
  sprintf(s, "%s_%s", fname, name);
  plug_t tmp[SVG_PLUGMAX];
  memcpy(tmp, a, n * sizeof(plug_t));
  qsort(tmp, n, sizeof(plug_t), cmp_tc);

  double mx_c = max_metric(tmp, n, M_COMP, len);
  double mx_d = max_metric(tmp, n, M_DECOMP, len);
  double mx = mx_c > mx_d ? mx_c : mx_d;
  double sub_h = BAR_H * 0.42;
  double group_h = sub_h * 2 + 6;
  int h = TOP_MARGIN + (int)(n * (group_h + BAR_GAP)) + BOTTOM_MARGIN;

  FILE *f = fopen(s, "w");
  if(!f) { perror(s); return; }
  snprintf(s, 80, "TurboBench: C/D Speed '%s'", fname);
  svg_open(f, CHART_W, h, s);
  double plot_w = CHART_W - LEFT_MARGIN - RIGHT_MARGIN;
  svg_line(f, LEFT_MARGIN, TOP_MARGIN - 10, LEFT_MARGIN, h - BOTTOM_MARGIN + 5, "#333", 1.5);

  /* legend */
  svg_rect(f, LEFT_MARGIN, 40, 14, 14, "#2E86AB", 2);
  svg_text(f, LEFT_MARGIN + 20, 51, "start", 12, "#111", "MB/s");
  svg_rect(f, LEFT_MARGIN + 240, 40, 14, 14, "#E67E22", 2);
  svg_text(f, LEFT_MARGIN + 260, 51, "start", 12, "#111", "MB/s");

  int i;
  for(i = 0; i < n; i++) {
    double y = TOP_MARGIN + i * (group_h + BAR_GAP);
    if(tmp[i].lev==INVLEV) sprintf(s, "%s", tmp[i].s); else sprintf(s, "%s,%d", tmp[i].s, tmp[i].lev);
    svg_text(f, LEFT_MARGIN - 10, y + group_h * 0.65, "end", 13, "#222", s);

    double bw_c = (tmp[i].tc / mx) * plot_w;
    svg_rect(f, LEFT_MARGIN, y, bw_c, sub_h, "#2E86AB", 3);
    char lbl_c[64];
    snprintf(lbl_c, sizeof(lbl_c), "%.1f", tmp[i].tc);
    svg_text(f, LEFT_MARGIN + bw_c + 6, y + sub_h * 0.75, "start", 11, "#111", lbl_c);

    double bw_d = (tmp[i].td / mx) * plot_w;
    svg_rect(f, LEFT_MARGIN, y + sub_h + 4, bw_d, sub_h, "#E67E22", 3);
    char lbl_d[64];
    snprintf(lbl_d, sizeof(lbl_d), "%.1f", tmp[i].td);
    svg_text(f, LEFT_MARGIN + bw_d + 6, y + sub_h + 4 + sub_h * 0.75, "start", 11, "#111", lbl_d);
  }
  svg_close(f);
  fclose(f);
}

static void svg_marker(FILE *f, int shape, double cx, double cy, double r, const char *fill, const char *stroke) {
  int k;
  switch (shape % SHAPE_COUNT) {
    case SHAPE_CIRCLE       : svg_circle(f, cx, cy, r, fill, stroke); break;
    case SHAPE_SQUARE       : fprintf(f, "<rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" fill=\"%s\" stroke=\"%s\" stroke-width=\"1.5\"/>\n", cx - r, cy - r, r * 2, r * 2, fill, stroke); break;
    case SHAPE_TRIANGLE_UP  : svg_poly_begin(f); svg_poly_pt(f, cx, cy - r); svg_poly_pt(f, cx - r, cy + r); svg_poly_pt(f, cx + r, cy + r); svg_poly_end(f, fill, stroke);  break;
    case SHAPE_TRIANGLE_DOWN: svg_poly_begin(f); svg_poly_pt(f, cx, cy + r); svg_poly_pt(f, cx - r, cy - r); svg_poly_pt(f, cx + r, cy - r); svg_poly_end(f, fill, stroke);  break;
    case SHAPE_DIAMOND      : svg_poly_begin(f); svg_poly_pt(f, cx, cy - r); svg_poly_pt(f, cx + r, cy);     svg_poly_pt(f, cx, cy + r);     svg_poly_pt(f, cx - r, cy); svg_poly_end(f, fill, stroke);  break;
    case SHAPE_CROSS        : { double t = r * 0.4;  svg_poly_begin(f);
      svg_poly_pt(f, cx - t, cy - r); svg_poly_pt(f, cx + t, cy - r); svg_poly_pt(f, cx + t, cy - t); svg_poly_pt(f, cx + r, cy - t);  svg_poly_pt(f, cx + r, cy + t); svg_poly_pt(f, cx + t, cy + t);
      svg_poly_pt(f, cx + t, cy + r); svg_poly_pt(f, cx - t, cy + r); svg_poly_pt(f, cx - t, cy + t); svg_poly_pt(f, cx - r, cy + t);  svg_poly_pt(f, cx - r, cy - t); svg_poly_pt(f, cx - t, cy - t);
      svg_poly_end(f, fill, stroke);
      break;
    }
    case SHAPE_XMARK        : {  double t = r * 0.35;  svg_poly_begin(f);
      svg_poly_pt(f, cx - r, cy - r + t); svg_poly_pt(f, cx - r + t, cy - r); svg_poly_pt(f, cx, cy - t); svg_poly_pt(f, cx + r - t, cy - r); svg_poly_pt(f, cx + r, cy - r + t); svg_poly_pt(f, cx + t, cy);
      svg_poly_pt(f, cx + r, cy + r - t); svg_poly_pt(f, cx + r - t, cy + r); svg_poly_pt(f, cx, cy + t); svg_poly_pt(f, cx - r + t, cy + r); svg_poly_pt(f, cx - r, cy + r - t); svg_poly_pt(f, cx - t, cy);
      svg_poly_end(f, fill, stroke);
      break;
    }
    case SHAPE_STAR         :  svg_poly_begin(f);
      for(k = 0; k < 10; k++) {
        double ang = -M_PI / 2 + k * M_PI / 5.0;
        double rad = (k % 2 == 0) ? r : r * 0.45;
        svg_poly_pt(f, cx + rad * cos(ang), cy + rad * sin(ang));
      }
      svg_poly_end(f, fill, stroke);
      break;
    case SHAPE_PENTAGON:  svg_poly_begin(f);
      for(k = 0; k < 5; k++) {  double ang = -M_PI / 2 + k * 2 * M_PI / 5.0;  svg_poly_pt(f, cx + r * cos(ang), cy + r * sin(ang));  }
      svg_poly_end(f, fill, stroke);
      break;
    case SHAPE_HEXAGON:   svg_poly_begin(f);
      for(k = 0; k < 6; k++) {
        double ang = k * M_PI / 3.0;
        svg_poly_pt(f, cx + r * cos(ang), cy + r * sin(ang));
      }
      svg_poly_end(f, fill, stroke);
      break;
    default:
      svg_circle(f, cx, cy, r, fill, stroke);
  }
}

#define LEGEND_W        180
#define LEGEND_SWATCH   10
#define LEGEND_LINEH    16
static const char *palette[] = { "#2E86AB", "#E67E22", "#27AE60", "#C0392B", "#8E44AD", "#16A085", "#D35400", "#2C3E50", "#F1C40F", "#7F8C8D", "#3498DB", "#E74C3C", "#1ABC9C", "#9B59B6", "#F39C12", "#34495E" };
#define PALETTE_N (int)(sizeof(palette)/sizeof(palette[0]))

#define SCATTER_W      900 // 620 //
#define SCATTER_H      680 // 756 //
#define SCATTER_MARGIN  50 //  //
void chart_scatter(const char *fname, char *name, plug_t *a, int n, metric_t xmetric, size_t len) {
  char s[256];
  if(n > SVG_PLUGMAX) n = SVG_PLUGMAX;
  sprintf(s, "%s_%s", fname, name);
  FILE *f = fopen(s, "w");
  if(!f) { perror(s); return; }
  int svg_w = SCATTER_W + LEGEND_W;
  strncpy(s, fname, 30); s[30] = 0;
  if(xmetric == M_COMP)
    snprintf(s, 80, "TurboBench: C Speed/Ratio %s", fname);
  else
    snprintf(s, 80, "TurboBench: D Speed/Ratio %s", fname);
  svg_open(f, svg_w, SCATTER_H, s);
  double mx_x = max_metric(a, n, xmetric, len) * 1.15;
  double mx_y = 100.0;
  double px0 = SCATTER_MARGIN, py0 = SCATTER_H - SCATTER_MARGIN;
  double pw = SCATTER_W - 2 * SCATTER_MARGIN;
  double ph = SCATTER_H - SCATTER_MARGIN - 50;
  svg_line(f, px0, py0, px0 + pw, py0, "#333", 1.5);
  svg_line(f, px0, py0, px0, py0 - ph, "#333", 1.5);
  int gi;
  for(gi = 0; gi <= 5; gi++) {
    double gx = px0 + pw * gi / 5.0;
    double vx = mx_x * gi / 5.0;
    svg_line(f, gx, py0, gx, py0 - ph, "#eee", 1);
    char t[32];
    snprintf(t, sizeof(t), "%.0f", vx);
    svg_text(f, gx, py0 + 18, "middle", 11, "#555", t);
    double gy = py0 - ph * gi / 5.0;
    double vy = mx_y * gi / 5.0;
    svg_line(f, px0, gy, px0 + pw, gy, "#eee", 1);
    char ty[32];
    snprintf(ty, sizeof(ty), "%.0f%%", vy);
    svg_text(f, px0 - 10, gy + 4, "end", 11, "#555", ty);
  }
  svg_text(f, px0 + pw / 2, SCATTER_H - 15, "middle", 13, "#222", xmetric == M_COMP ? "C MB/s" : "D MB/s");
  fprintf(f, "<text x=\"20\" y=\"%.2f\" font-size=\"13\" fill=\"#222\" transform=\"rotate(-90 20 %.2f)\" text-anchor=\"middle\">Ratio%%</text>\n", py0 - ph / 2, py0 - ph / 2);
  double legend_x = SCATTER_W + 20, legend_y = SCATTER_MARGIN;
  svg_text(f, legend_x, legend_y - 12, "start", 13, "#222", "Legend");
  for(int i = 0; i < n; i++) {
    double      v = xmetric == M_COMP ? a[i].tc : a[i].td,
               cx = px0 + (v / mx_x) * pw,
               cy = py0 - (RATIO(a[i].len, len) / mx_y) * ph;
    const char *color = palette[i % PALETTE_N];
    int        shape = i % SHAPE_COUNT;
    svg_marker(f, shape, cx, cy, 6, color, "#333");
    if(a[i].lev == INVLEV) sprintf(s, "%s", a[i].s);
    else                    sprintf(s, "%s,%d", a[i].s, a[i].lev);
    double ly = legend_y + i * LEGEND_LINEH;
    svg_marker(f, shape, legend_x + LEGEND_SWATCH / 2, ly - 4, LEGEND_SWATCH / 2, color, "#333");
    svg_text(f, legend_x + LEGEND_SWATCH + 8, ly, "start", 11, "#111", s);
  }
  svg_close(f);
  fclose(f);
}

void chart(plug_t *a, int n, char *fname, size_t len) {
  chart_bar(fname, "ratio.svg",              a, n, M_RATIO,  len);
  chart_bar(fname,  "comp.svg",              a, n, M_COMP,   len);
  chart_bar(fname,"decomp.svg",              a, n, M_DECOMP, len);
  chart_grouped(fname,"grouped.svg",         a, n, len);
  chart_scatter(fname, "scatter_comp.svg",   a, n, M_COMP,   len);
  chart_scatter(fname, "scatter_decomp.svg", a, n, M_DECOMP, len);
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

typedef struct {
  unsigned long long bw;
  unsigned           rtt;
  char               *s;
} bandwidth_t;

bandwidth_t bw[] = {
  {    7*KB, 500, "GPRS 56"  },//56kbps
  {   57*KB, 150, "2G 456"   },
  {  125*KB,  40, "3G 1M"    },
  {  250*KB,   5, "DSL 2M"   },//DSL 2000
  {  500*KB,  20, "4G 4M"    },
  { 3750*KB,   5, "WIFI 30M" },
  {12500*KB,   5, "CAB 100M" },
  {37500*KB,  10, "5G 300M"  },//5G sub-6GHz avg ~300Mbps

  {   40*MB,   0, "USB2 40MB"},
  {  125*MB,   0, "ETH 1000" },
  {  200*MB,   0, "HDD 200MB"},
  {  550*MB,   0, "SSD 550MB"},
  {   1u*GB,   0, "SSD 1GB"  },
  {   2u*GB,   0, "SSD 2GB"  },
  { 4ull*GB,   0, "4GB/s"    },
  { 8ull*GB,   0, "8GB/s"    },
  {14ull*GB,   0, "SSD 14GB/s"}//PCIe 5.0 NVMe (e.g. Crucial T705)
};
#define BWSIZE (sizeof(bw)/sizeof(bandwidth_t))

char *tm2yyyymmdd(time_t t, char *buf, size_t bufsize) {
  struct tm *tm = localtime(&t);   // use gmtime(&t) for UTC
  if(!tm) { snprintf(buf, bufsize, "????.??.?? ??:??"); return buf; }
  strftime(buf, bufsize, "%Y.%m.%d %H:%M", tm);
  return buf;
}

void plugprth(FILE *f, int fmt, char *t) {
  char *plot  = "<script src=https://cdn.plot.ly/plotly-latest.min.js></script>";
  char *jquery = "<script src=\"http://ajax.googleapis.com/ajax/libs/jquery/1/jquery.min.js\"></script>";
  char *tstyle = "<link rel=\"stylesheet\" href=\"http://tablesorter.com/themes/blue/style.css\" type=\"text/css\" media=\"print, projection, screen\" />";
  char *table  = "<script type=\"text/javascript\" src=\"http://tablesorter.com/__jquery.tablesorter.min.js\"></script>";
  char *code   = "<script type=\"text/javascript\">$(function() {       $(\"#myTable\").tablesorter({sortList:[[0,0],[2,1]], widgets: ['zebra']});      $(\"#options\").tablesorter({sortList: [[0,0]], headers: { 3:{sorter: false}, 4:{sorter: false}}}); }); </script><script type=\"text/javascript\" src=\"http://tablesorter.com/__jquery.tablesorter.min.js\"></script><script type=\"text/javascript\">$(function() {       $(\"#myTable2\").tablesorter({sortList:[[0,0],[2,1]], widgets: ['zebra']});     $(\"#options\").tablesorter({sortList: [[0,0]], headers: { 3:{sorter: false}, 4:{sorter: false}}}); }); </script>";
  char s[128+1], ts[64];  
  time_t tm;
  time(&tm); strcpy(ts,asctime(localtime(&tm))); ts[strlen(ts)-1] = 0;
  snprintf(s, 128, "TurboBench:%s %s - %s", t, tm2yyyymmdd(tm, ts, 64), _cpubrand);

  switch(fmt) {
    case FMT_TEXT:
      fprintf(f,"%s\n", s );
      break;
    case FMT_VBULLETIN:
    case FMT_VBULLETIN2:
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
      if(memout)
        fprintf(f,"      C Size  ratio%%      C MB/s    D MB/s Rank        C MEM     D MEM   C STACK   D STACK Name            File\n");
      else
        fprintf(f,"      C Size  ratio%%      C MB/s    D MB/s Rank    Name            File\n");
      break;
    case FMT_VBULLETIN:
      fprintf(f,"[table]C Size|ratio%|C MB/s|D MB/s|Name|File (MB=1.000.0000)\n");
      break;
    case FMT_VBULLETIN2:
      fprintf(f,"[CODE][B]%s[/B] MB=1.000.0000\n", head);
      break;
    case FMT_HTMLT:
      fprintf(f,"<pre><b>%s</b> MB=1.000.0000\n", head);
      break;
    case FMT_HTML:
      fprintf(f,"<h3>TurboBench: Compressor Benchmark</h3><table id='myTable' class='tablesorter' style=\"width:35%%\"><thead><tr><th>C Size</th><th>ratio%%</th><th>C MB/s</th><th>D MB/s</th><th>Name</th><th>C Mem</th><th>D Mem</th><th>File</th></tr></thead><tbody>\n");
      break;
    case FMT_MARKDOWN:
      if(memout)
        fprintf(f,"|C Size|ratio%|C MB/s|D MB/s|Rank|C Mem|D Mem|C Stack|D Stack|Name|File|\n|--------:|-----:|--------:|--------:|--------:|--------:|--------:|--------:|----------------|----------------|\n");
      else  
        fprintf(f,"|C Size|ratio%|C MB/s|D MB/s|Rank|Name|File|\n|--------:|-----:|--------:|--------:|----------------|----------------|\n");
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
      fprintf(f,"[/table]\n");
      break;
    case FMT_VBULLETIN2:
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

double tc_smin, td_smin; // show only if greater than

void plugprt(plug_t *plug, unsigned long long totinlen, char *finame, int fmt, double *ptc, double *ptd, FILE *f) {
  double ratio  = RATIO(plug->len,totinlen),           //ratio  = FACTOR(plug->len,totinlen),
         tc     = TMBS(totinlen,plug->tc), td = TMBS(totinlen,plug->td);
  unsigned score = SCORE(plug,totinlen);
  char   name[256], sratio[16];
  strratio(ratio, sratio);
  if(tc < tc_smin) return;  if(td < td_smin) return;
  if(plug->lev != INVLEV)
    sprintf(name, "%s%s %d%s", plug->err?"?":"", plug->s, plug->lev, plug->prm);
  else
    sprintf(name, "%s%s%s",    plug->err?"?":"", plug->s,            plug->prm);

  int c = 0, d = 0, n = 0;
  if(!plug->err && tc > *ptc) { c++; n++; *ptc = tc; }
  if(!plug->err && td > *ptd) { d++; n++; *ptd = td; }
  switch(fmt) {
    case FMT_TEXT:
      if(f == stdout) {
          #ifdef _WIN32
        static int once = 0;
        if(!once) { SetConsoleOutputCP(65001); once = 1; }
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        fprintf(f, "%12"PRId64" %s", plug->len, sratio);
        #define BBOLD 2 //2=green, 15=white
        if(c) SetConsoleTextAttribute(h, BBOLD);
        fprintf(f, "%9.2f ", tc);
        if(c) SetConsoleTextAttribute(h, 7);

        if(d) SetConsoleTextAttribute(h, BBOLD);
        fprintf(f, "%9.2f ", td);
        if(d) SetConsoleTextAttribute(h, 7);

        if(n) SetConsoleTextAttribute(h, BBOLD);
        fprintf(f, "%3d %s", score, score<MEDALMAX?medal[score]:"  ");
        if(n) SetConsoleTextAttribute(h, 7);
        if(memout) fprintf(f, "%9llu %9llu %9llu %9llu ", plug->memc, plug->memd, plug->stkc, plug->stkd);   

        if(n) SetConsoleTextAttribute(h, BBOLD);
        fprintf(f, "%-16s", name);
        if(n) SetConsoleTextAttribute(h, 7);
        fprintf(f, "%s\n", finame);
        #undef BBOLD
          #else
        fprintf(f, "%12"PRId64" %s%s%9.2f%s %s%9.2f%s %s%3d%s%s   ",
          plug->len, sratio, c?BOLDB:"", tc, c?BOLDE:"",  d?BOLDB:"", td, d?BOLDE:"", n?BOLDB:"", score, score<MEDALMAX?medal[score]:"  ", n?BOLDE:"", n?BOLDB:"", name, n?BOLDE:"");
        if(memout) fprintf(f, "%9d %9d %9d %9d ", plug->memc, plug->memd, plug->stkc, plug->stkd);   
        fprintf(f, "%s%-16s%s%s\n", n?BOLDB:"", name, n?BOLDE:"", finame);
          #endif
      }
      else
        fprintf(f,"%12"PRId64" %s   %9.2f   %9.2f %3d   %-32s %s\n", plug->len, sratio, tc, td, score, name, finame);
      break;
    case FMT_VBULLETIN:
      fprintf(f, "%12"PRId64"|%s|%s%9.2f%s|%s%9.2f%s|%s%-16s%s|%s\n",
        plug->len, sratio, c?"[B]":"", tc, c?"[/B]":"",  d?"[B]":"", td, d?"[/B]":"", n?"[B]":"", name, n?"[/B]":"", finame);
      break;
    case FMT_VBULLETIN2:
      fprintf(f, "%12"PRId64" %s   %s%9.2f%s   %s%9.2f%s %s%-16s%s%s\n",
        plug->len, sratio, c?"[B]":"", tc, c?"[/B]":"",  d?"[B]":"", td, d?"[/B]":"", n?"[B]":"", name, n?"[/B]":"", finame);
      break;
    case FMT_HTMLT:
      fprintf(f, "%12"PRId64" %s   %s%9.2f%s   %s%9.2f%s %s%-16s%s%s\n",
        plug->len, sratio, c?"<b>":"", tc, c?"</b>":"",  d?"<b>":"", td, d?"</b>":"", n?"<b>":"", name, n?"</b>":"", finame);
      break;
    case FMT_HTML:
      fprintf(f, "<tr><td align=\"right\">%11"PRId64"</td><td align=\"right\">%s</td><td align=\"right\">%s%9.2f%s</td><td align=\"right\">%s%9.2f%s</td><td>%s%-16s%s</td><td align=\"right\">%"PRId64"</td><td align=\"right\">%"PRId64"</td><td>%s</td></tr>\n",
        plug->len, sratio, c?"<b>":"", tc, c?"</b>":"",  d?"<b>":"", td, d?"</b>":"", n?"<b>":"", name, n?"</b>":"",
//        SIZE_ROUNDUP(plug->memc, Kb)/Kb, SIZE_ROUNDUP(plug->memd,Kb)/Kb,
        plug->memc, plug->memd,
        finame);
      break;
    case FMT_MARKDOWN:
      if(memout)
        fprintf(f, "|%"PRId64"|%s|%s%.2f%s|%s%.2f%s|%d%s|%u|%u|%u|%u|%s%s%s|%s|\n", plug->len, sratio, c?"**":"",  tc, c?"**":"",    d?"**":"",  td, d?"**":"", score, score<MEDALMAX?medal[score]:" ", 
                      plug->memc, plug->memd, plug->stkc, plug->stkd, n?"**":"",  name, n?"**":"",   finame);
      else        
        fprintf(f, "|%"PRId64"|%s|%s%.2f%s|%s%.2f%s|%d%s|%s%s%s|%s|\n", plug->len, sratio, c?"**":"",  tc, c?"**":"",    d?"**":"",  td, d?"**":"", score, score<MEDALMAX?medal[score]:" ",  n?"**":"",  name, n?"**":"",   finame);  break;
    case FMT_CSV:    fprintf(f,"%12"PRId64",%11"PRId64",%s,%9.2f,%9.2f,%-16s,%s\n",       totinlen, plug->len, sratio, tc, td, name, finame);  break;
    case FMT_TSV:    fprintf(f,"%12"PRId64"\t%11"PRId64"\t%s\t%9.2f\t%9.2f\t%-16s\t%s\n", totinlen, plug->len, sratio, tc, td, name, finame);  break;
      break;
  }
}

static unsigned blknum, speedup;
enum { SP_SPEEDUPC=1, SP_SPEEDUPD, SP_TRANSFERC, SP_TRANSFERD, SP_SPEEDUP, SP_TRANSFER };
#define SP_ISFACTOR(x) (x==SP_SPEEDUPC || x == SP_SPEEDUPD || x == SP_SPEEDUP)
#define SP_TYPE(x)     (x<SP_SPEEDUP)?(x&1):2        // 0:Decomp 1:Comp 2:Comp+Decomp

char *plugspeedup(int speedup, int trans) { return (speedup==SP_SPEEDUP || speedup==SP_TRANSFER)?(trans?"Compression+Transfer+Decompression":"Compression+Decompression"):((speedup&1)?"Compression":"Decompression"); }

void plugprtph(FILE *f, int fmt) {
  int i; char *s = plugspeedup(speedup,0);

  switch(fmt) {
    case FMT_HTML:
      fprintf(f,"<p><h3>TurboBench: Speedup %s sheet</h3><table id='myTable2' class='tablesorter' style=\"width:80%%\"><thead><tr><th>Name</th>", s);
      for(i = 0; i < BWSIZE; i++)
        fprintf(f, "<th>%s</th>", bw[i].s);
      fprintf(f, "<td>File");
      if(blknum)
        fprintf(f, " blknum=%d ", blknum);
      fprintf(f, "</td></tr></thead><tbody>\n");
      break;
    case FMT_MARKDOWN:
      fprintf(f,"#### TurboBench: Speedup %s sheet\n\n", s);
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
      fprintf(f,"TurboBench: Speedup %s sheet\n\n", s);
      fprintf(f,"[table][B]\n");
      break;
    case FMT_VBULLETIN2:
      fprintf(f,"TurboBench: Speedup %s sheet\n\n", s);
      fprintf(f,"[CODE][B]\n");
    default:
      fprintf(f,"Name           ");
      for(i = 0; i < BWSIZE; i++)
        fprintf(f, "%10s", bw[i].s);
      if(blknum)
        fprintf(f, " blknum=%d ", blknum);
      fprintf(f, "\n");
    if(fmt == FMT_VBULLETIN || fmt == FMT_VBULLETIN2)
      fprintf(f,"[/B]\n");
  }
}

static inline double spmbs(double td, unsigned long long len, int i, unsigned long long totinlen) {
  double t = td + len/(double)bw[i].bw + blknum*(bw[i].rtt*1000.0);
  return TMBS(totinlen,t);
}

//static inline double spdup(double td, long long len, int i, long long totinlen) { double t = td + len*TM_T/(double)bw[i].bw + blknum*(bw[i].rtt*1000.0); return ((double)totinlen*TM_T*100.0/t)/(double)bw[i].bw;}
static inline double spdup(double td, long long len, int i, long long totinlen) {
  return (double)totinlen*100.0 / ((double)len + ((td+blknum*bw[i].rtt*1000.0))*(double)bw[i].bw );
}

void plugprtp(plug_t *plug, long long totinlen, char *finame, int fmt, int speedup, FILE *f) {
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
      case SP_TRANSFERD:  fprintf(f,"%9.3f ",     spmbs(plug->td,          plug->len, i, totinlen));       break;
      case SP_SPEEDUPD:   fprintf(f,"%9d ", (int)(spdup(plug->td,          plug->len, i, totinlen)+0.5));  break;
      case SP_TRANSFERC:  fprintf(f,"%9.3f ",     spmbs(plug->tc,          plug->len, i, totinlen));       break;
      case SP_SPEEDUPC:   fprintf(f,"%9d ", (int)(spdup(plug->tc,          plug->len, i, totinlen)+0.5));  break;
      case SP_SPEEDUP:    fprintf(f,"%9d ", (int)(spdup(plug->tc+plug->td, plug->len, i, totinlen)+0.5));  break;
      case SP_TRANSFER:   fprintf(f,"%9.3f ",     spmbs(plug->tc+plug->td, plug->len, i, totinlen));       break;
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
      if(memout) fprintf(f, "%.10d %.10d %.10d %.10d ", plug->memc, plug->memd, plug->stkc, plug->stkd); 
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

void plugplot(plug_t *plug, unsigned long long totinlen, int fmt, int speedup, char *s, FILE *f) {
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
      case SP_TRANSFERD: fprintf(f,"%9.3f%s",     spmbs(plug->td,          plug->len, i, totinlen),      i+1 < BWSIZE?",":""); break;
      case SP_SPEEDUPD:  fprintf(f,"%9d%s", (int)(spdup(plug->td,          plug->len, i, totinlen)+0.5), i+1 < BWSIZE?",":""); break;
      case SP_TRANSFERC: fprintf(f,"%9.3f%s",     spmbs(plug->tc,          plug->len, i, totinlen),      i+1 < BWSIZE?",":""); break;
      case SP_SPEEDUPC:  fprintf(f,"%9d%s", (int)(spdup(plug->tc,          plug->len, i, totinlen)+0.5), i+1 < BWSIZE?",":""); break;
      case SP_SPEEDUP:   fprintf(f,"%9d%s", (int)(spdup(plug->tc+plug->td, plug->len, i, totinlen)+0.5), i+1 < BWSIZE?",":""); break;
      case SP_TRANSFER:  fprintf(f,"%9.3f%s",     spmbs(plug->tc+plug->td, plug->len, i, totinlen),      i+1 < BWSIZE?",":""); break;
    }
  fprintf(f, "],\ntype: 'scatter',\nmode: 'lines+markers',\nline: {shape: 'spline'},\nname: '%s'\n};\n", name);
}

void plugplote(FILE *f, int fmt, char *s) {
  fprintf(f, "var data = [%s];\nvar layout = {\ntitle:'TurboBench Speedup: %s Speed',\nxaxis: {\ntitle: '%s Transfer Speed (M=MB/s B=GB/s)',\n%s    autorange: true\n  }, \n  yaxis: {\n\ntitle: 'Speedup %%',\n%sautorange: true\n  }\n};\nPlotly.plot('myDiv1', data, layout);</script>\n",
    s, plugspeedup(speedup,1), xlog?"log":"", xlog?"type: 'log',\n":"", ylog?"type: 'log',\n":"");
}

int libcmp(const plug_t *e1, const plug_t *e2) {
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

int libcmpn(const plug_t *e1, const plug_t *e2) {
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
void plugplotc(plug_t *plug, int k, long long totinlen, int fmt, int speedup, char *s, FILE *f) {
  int  i, n = 0;
  char name[65],txt[256];
  qsort(plug, k, sizeof(plug_t), (int(*)(const void*,const void*))libcmpn);

  plug_t *g,*gs=plug,*p;
  for(txt[0] = name[0] = 0, g = plug; g < plug+k; g++)
  if(g->id <= P_MCPY && !plotmcpy)
    continue;
  else {
    if(strcmp(g->s, name)) {
      if(name[0]) {
        fprintf(f, "],\ny: [");
        for(p = gs; p < g; p++)
          fprintf(f, "%.2f%s", SP_ISFACTOR(speedup)?FACTOR(p->len,totinlen):RATIO(p->len,totinlen), p+1<g?",":"");
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
    unsigned sptype = SP_TYPE(speedup);
    double t = sptype==1?g->tc:(sptype?g->tc+g->td:g->td);  //0:Decomp 1:Comp 2:Comp+Decomp
    fprintf(f, "%.2f", TMBS(totinlen,t));
  }
  fprintf(f, "],\ny: [");
  for(p = gs; p < g; p++)
    fprintf(f, "%.2f%s", SP_ISFACTOR(speedup)?FACTOR(p->len,totinlen):RATIO(p->len,totinlen), p+1<g?",":"");
  fprintf(f, "],\nmode: 'markers+text',\ntype: 'scatter',\nname: '%s',\ntextposition: 'top center', textfont: { family:  'Raleway, sans-serif' }, marker: { size: 12 }\n", name, txt);
  if(txt[0])
    fprintf(f, "\n,text:[%s]\n", txt);
  fprintf(f, "};\n");
}

void plugplotce(FILE *f, int fmt, char *s) {
  fprintf(f, "var data = [%s];\nvar layout = {\ntitle:'TurboBench: %s',\nxaxis: {\ntitle: '%s speed MB/s',\n%s    autorange: true\n  }, \n  yaxis: {\n\ntitle: 'Ratio (factor)',\n%sautorange: true\n  }\n};\nPlotly.plot('myDiv2', data, layout);</script>\n",
    s, plugspeedup(speedup,0), xlog2?"log":"", xlog2?"type: 'log',\n":"", ylog2?"type: 'log',\n":"");
}

int plugprts(plug_t *plug, int k, char *finame, int xstdout, unsigned long long totlen, int fmt, char *t) {
  double ptc = 0.0, ptd = 0.0;
  plug_t *g;
  if(!totlen) return 0;                                                                             if(verbose>1) printf("plugprts:'%s' k=%d\n", finame, k);

  qsort(plugt, k, sizeof(plug_t), (int(*)(const void*,const void*))libcmp);
  for(g = plugt; g < plugt+k; g++) g->rank = 0;
  plugrank(plugt, k);  //for(g = plugt; g < plugt+k; g++) printf("(%d)", g->rank);
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
        fprintf(fo,"[/table]\n");
        break;
      case FMT_VBULLETIN2:
        fprintf(fo,"[/CODE]\n");
        break;
      case FMT_MARKDOWN :
        fprintf(fo, "\n");
        break;
    }
    if(fmt == FMT_HTML) {
      char s[1025];
      s[0] = 0;                                                             if(verbose>1) printf("generate speedup plot\n");
      plugplotb(fo, fmt, 1);
      for(g = plugt; g < plugt+k; g++)
        if(g->id > P_MCPY || plotmcpy)
          plugplot(g, totlen, fmt, speedup, s, fo);
      plugplote(fo, fmt, s);

      s[0] = 0;                                                             if(verbose>1) printf("generate speed/ratio plot\n");
      plugplotb(fo, fmt, 2);
      plugplotc(plug, k, totlen, fmt, speedup, s, fo);
      plugplotce(fo, fmt, s);

    }
  }
  plugprtf(fo, fmt);
  fclose(fo);
}

int plugread(plug_t *plug, char *finame, unsigned long long *totinlen) {
  char   s[LSIZE+1], name[33];
  plug_t *p = plug;
  FILE   *fi = fopen(finame, "r");
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
    p->memc   = strtoull(  t, &t, 10);
    p->memd   = strtoull(++t, &t, 10);
    p->stkc   = strtoull(++t, &t, 10);
    p->stkd   = strtoull(++t, &t, 10);   

    for(q = ++t; *q && *q != '\t'; q++); *q++ = 0; strncpy(p->tms, t, TMS_SIZE); p->tms[TMS_SIZE]=0; t = q;
    if(p->prm[0]=='?')
      p->prm[0]=0;
    int i;
    for(i = 0; plugs[i].id >=0; i++) {
      if(!strcmp(name, plugs[i].s)) {
        p->s  = plugs[i].s;
        p->id = plugs[i].id;                                            if(verbose>1) { fprintf(stdout, "$%s\t%"PRId64"\t%"PRId64"\t%.6f\t%.6f\t%s\t%d%s\t%s\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%"PRId64"\n",
                                                                                         s, *totinlen, p->len, p->td, p->tc, p->s, p->lev, p->prm, p->tms, p->memc, p->memd, p->stkc, p->stkd); fflush(stdout); }
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

static int mcpy = 0, mode, tincx, fuzz;

unsigned becomp(unsigned char *_in, size_t _inlen, unsigned char *_out, size_t outsize, unsigned bsize, int id, int lev, char *prm, char *name, char *fname) {
  unsigned char *op,*oe = _out + outsize;
  codstart(bsize, id, lev, prm, 0);
  TMBEG(tm_Rep);//    mempeakinit();
    unsigned char *in,*ip;
    for(op = _out, in = _in; in < _in+_inlen; ) {
      unsigned inlen, bs;
      if(mode) {                                                         blknum++;
        inlen      = ctou32(in); in += 4;
        ctou32(op) = inlen; op += 4; //vbput32(op, inlen);
        if(in+inlen>_in+_inlen) inlen = (_in+_inlen)-in;
      } else inlen = _inlen;

      for(ip = in, in += inlen; ip < in; ) {                                                
        size_t iplen = in - ip; iplen = min(iplen, bsize);
        bs = (mode || bsize < inlen)?((min(bsize, iplen) < (1<<16))?2:4):0;
        size_t oplen = codcomp(ip, iplen, op+bs, oe-(op+bs), id, lev,prm);
        if(oplen <= 0 || oplen >= iplen && mcpy) {
          if(mcpy) { memcpy(op+bs, ip, iplen); oplen = iplen; }
          else if(oplen <= 0) { op = _out; goto end; }
        }
        if(bs == 2 && oplen >= (1<<16)) die("Output larger than input! Use option '-P'\n");
        if(mode || bsize < inlen) { bs==2?(ctou16(op) = oplen):(ctou32(op) = oplen); } op += oplen+bs; ip += iplen;
        if(op > _out+outsize)
          die("Overflow error %llu, %u in lib=%d '%s %d'\n", outsize, (int)(op - _out), id, plug[id].s, plug[id].lev);
      }
    }
    size_t olen = op - _out;
  TMENDC(_inlen, olen, name, fname, 0);
  end: codend(_inlen, id, lev, prm, 0);
  return op - _out;
}

int bedecomp(unsigned char *_in, unsigned _inlen, unsigned char *_out, unsigned _outlen, unsigned bsize, int id, int lev, char *prm) {
  unsigned char *ip;
  codstart(_inlen, id, lev, prm, 1);
  TMBEG(tm_Rep2);     //mempeakinit();
  unsigned char *out,*op;
  for(ip = _in, out = _out; out < _out+_outlen;) {
    unsigned outlen,bs;
    if(mode) { outlen = ctou32(ip); ip += 4;
      ctou32(out) = outlen; out += 4;
      if(out+outlen>_out+_outlen)
        outlen = (_out+_outlen)-out;
    } else outlen = _outlen;
    for(op = out, out += outlen; op < out; ) {
      unsigned oplen = out - op; oplen = min(oplen, bsize);
      bs = (mode || bsize < outlen)?((min(bsize, oplen)<(1<<16))?2:4):0;
      int l, iplen = (mode || bsize < outlen)?(bs==2?ctou16(ip):ctou32(ip)):_inlen; ip += bs;
      if(mcpy && iplen==oplen)
        memcpy(op, ip, oplen);
      else l = coddecomp(ip, iplen, op, oplen, id, lev,prm);
      ip += iplen; op += oplen;
    }
  }
  TMEND(_outlen);
  codend(_inlen, id, lev, prm, 1);
  return ip - _in;
}

plug_t plugr[32]; int tid;
#define BEPRE
#define BEINI
#define BEPOST
#define BEOPT
#define BEUSAGE
#define BEFILE
#define BENCHSTA

int delim;
#define PATH_LENMAX 1024
#define FLENMAX Gb

void bebuild(char **files, int argc, int recurse, char *foname, unsigned long long filenmax, int lim) {
  FILE               *fo = fopen(foname, "wb"); if(!fo) { perror(foname); die("creat error '%s'", foname); }
  unsigned           st_fnum = 0, fno, insize = 100*MB, inlen;
  char               *in = malloc(insize),*finame; vmemset(in, 0, insize);
  unsigned long long st_flen = 0, st_blklen = 0;

  if(!filenmax) filenmax = FLENMAX;                                     fprintf(stdout,"number of files=%d. Max. file length=%llu\n", argc, filenmax); fflush(stdout);
  if(recurse) {
    struct recur *recur = rdiropen(files);
    if(recur) {
      char finame[PATH_LENMAX+1]; finame[0] = 0; struct stat st;
      while(!rdirnext(recur, finame, &st)) {
        unsigned char *p = finame;
        if(strlen(p) > 3 && (!strncasecmp(&p[strlen(p)-3], ".7z", 3) || !strncasecmp(&p[strlen(p)-3], ".gz", 3) || !strncasecmp(&p[strlen(p)-4], ".zip", 4) )  ) continue;
          #ifndef _WIN32
        if(S_ISLNK(st.st_mode)) fprintf(stderr,"warning link '%s' broken\n", finame);else
          #endif
        if(S_ISREG(st.st_mode)  /*st.st_mode & S_IFREG*/) {
          FILE *fi = fopen(finame, "rb"); if(!fi) { perror(finame); die("open error '%s'", finame); }                 fprintf(stdout,"'%s'\n", finame); fflush(stdout);
          if((inlen = fread(in, 1, insize, fi)) > 0/*16*1024 && inlen < 128*1024*/) {
            if(st_flen + 4 + inlen > filenmax) inlen = filenmax - (st_flen+4);
            fwrite(&inlen, 1, 4,    fo);                                                                  st_fnum++; st_flen += inlen+4;
            fwrite(in,     1, inlen,fo);                                                                  if(st_flen >= filenmax) break;
          }
          fclose(fi);
        }
      }
      rdirclose(recur);
    }
  } else for(fno = 0; fno < argc; fno++) {
    char *finame = files[fno];
    if(finame[0]=='-') continue;
    FILE *fi = fopen(finame, "rb");
    if(!fi) { perror(finame); die("open error '%s'", finame); } if(verbose>1) fprintf(stdout,"'%s'\n", finame); fflush(stdout);
    if((inlen = fread(in, 1, insize, fi))) {
      if(ftell(fo) + 4 + inlen > filenmax)
        inlen = filenmax - (st_flen+4);
      if(delim > 0) {
        char *p;
        for(p = in; p < in+inlen;) {
          char *q = strchr(p, delim);
          unsigned l = q - p;
          fwrite(&l, 1, 4,fo);                                              st_fnum++; st_blklen += l; st_flen += l+4;
          fwrite(p,  1, l,fo);                                              if(st_flen >= filenmax) break;
          if(!q) break;
          p = q+1;
        }
        exit(0);
      } else {
        fwrite(&inlen, 1, 4, fo);                                           st_fnum++; st_blklen += inlen; st_flen += inlen+4;
        fwrite(in,1, inlen,fo);                                             if(ftell(fo) > filenmax) break;
      }
    }
    fclose(fi);
  }                                                                         printf("Number of files=%d, Number of files processed=%d, avglen=%d\n", argc, st_fnum, (int)(st_blklen/st_fnum));
  fclose(fo);
  free(in);
}

#define INOVD 4*1024

  #if defined(_WIN32) && !defined(__MINGW__)
int getpagesize() {
  static int pagesize = 0;
  if(pagesize == 0) {
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    pagesize = max(system_info.dwPageSize, system_info.dwAllocationGranularity);
  }
  return pagesize;
}
  #endif

size_t mininlen;

unsigned long long plugfile(plug_t *plug, char *finame, unsigned long long filenmax, size_t bsize, plug_t *plugr, int tid, int krep) {
  size_t outsize;
  FILE   *fi = strcmp(finame,"stdin")?fopen(finame, "rb"):stdin; if(!fi) { perror(finame); return 0; /*die("open error '%s'\n", finame);*/ }
  char   *p;
  if((p = strrchr(finame, '\\')) || (p = strrchr(finame, '/'))) finame = p+1;              if(verbose>1) printf("'%s'\n", finame);
  p = finame;

  char name[65];
  if(plug->lev != INVLEV)
    sprintf(name, "%s %d%s", plug->s, plug->lev, plug->prm);
  else
    sprintf(name, "%s%s",    plug->s,            plug->prm);

  long long filen;
  if(finame) {
    fseeko(fi, 0, SEEK_END); filen = ftello(fi); fseeko(fi , 0 , SEEK_SET); if(filenmax && filen > filenmax) filen = filenmax;
  } else
    filen = filenmax?filenmax:Gb;
                                                                                              
  size_t insize   = filen>bsize?bsize:filen;                                                   if(filen < mininlen) insize = mininlen;
  size_t pagesize = getpagesize();
  size_t insizem  = (fuzz&3)?SIZE_ROUNDUP(insize, pagesize):(insize+INOVD);

  outsize = insize*fac + 10*Mb;
  unsigned char *_in = NULL, *_cpy = _in, *out;
  if(insizem && !(_in = _valloc(insizem,1)))                     die("malloc error in size=%u\n", insizem);
  vmemset(_in, 0, insize);
  if(!(out = (unsigned char*)_valloc(outsize,2)))                die("malloc error out size=%u\n", outsize);
  vmemset(out, 0, outsize);
  if((cmp || tid) && insizem && !(_cpy = _valloc(insizem*3,3)))  die("malloc error cpy size=%u\n", insizem);
  if(_cpy) vmemset(_cpy, 0, insizem*3);

  codini(insize, plug->id, plug->lev, plug->prm);
  size_t    inlen;
  long long totinlen = 0;
  double    ptc = DBL_MAX, ptd = DBL_MAX;
  bsize     = plug->blksize?plug->blksize:bsize;
  plug->len = plug->tc = plug->td = 0;
                                                                                blknum = 0;

  while((inlen = fread(_in, 1, insize, fi)) > 0) {                              
    double        tc = 0.0, td = 0.0;
    unsigned char *in = _in;
    size_t        len = inlen;
    totinlen += inlen;
    if(fuzz & 1) { in = (_in+insizem)-inlen; memmove(in, _in, inlen);           /*printf("SEGFAULT Check");fflush(stdout); in[inlen-1] = in[inlen]; printf("SEGFAULT TEST FAILED"); fflush(stdout);*/  }
    //BEPRE;
                                                                                memrcpy(out, in, len);
    unsigned nb = 1;
    if(len < mininlen) {
      bsize = len;
      unsigned char *p;
      for(p = in+len; ; p+=len) {
        if(p+len > in+insize) break;
        nb++;
        memcpy(p, in, len);
      }
    }
    size_t     peak   = mempeakinit();
    memstack_t _stack = stackini();
    size_t outlen = becomp(in, len*nb, out, outsize, bsize, plug->id,plug->lev,plug->prm, name, finame)/nb;
    tc         = tm_tmin(nb);
    plug->len += outlen;
    plug->tc  += tc;
    plug->memc = mempeak() - peak;                                              //if(memused()>2048) printf("warning: memory in compression not freed by compressor '%s %s'.%zu\n", plug->s, plug->prm, memused());
    plug->stkc = stackpeak(_stack);
    
    if(cmp) {
      unsigned char *cpz = _cpy;
      if(fuzz & 2) { cpz = (_cpy+insizem) - len;                                    /*printf("SEGFAULT Check");fflush(stdout); cpz[len-1] = cpz[len]; printf("SEGFAULT TEST FAILED"); fflush(stdout);*/  }
      if(_cpy != _in) memrcpy(cpz, in, len);
      
      size_t     peak   = mempeakinit();
      memstack_t _stack = stackini();
      unsigned cpylen  = bedecomp(out, outlen, cpz, len*nb, bsize, plug->id,plug->lev,plug->prm)/nb;
      td         = tm_tmin(nb);
      plug->td  += td;
      plug->memd = mempeak() - peak;                                            //if(tm_verbose && totinlen == filen) printf("%9.2f   %-16s %s\n", TMBS(totinlen,plug->td), name, finame); //for(int i=0; i < strlen(name)+strlen(finame)+55;i++) printf("\b");}
      plug->stkd = stackpeak(_stack);                                           //if(memused()>2048) printf("warning: memory in decompression not freed by compressor '%s %s'.%zu\n", plug->s, plug->prm, memused());
      
      int e = memcheck(in, len, cpz, fuzz?3:cmp, finame);
      plug->err = plug->err?plug->err:e;
      BEPOST;
    } //else                                                                      if(tm_verbose && totinlen == filen) { printf("%9.2f   %-16s %s\n", 0.0, name, finame); }
    if(totinlen >= filen) break;
  }                                                                          //printf("ILEN=%llu Olen=%llu c=%f d=%f\n", totinlen, plug->len, plug->tc, plug->td);
  _vfree(out, outsize);
  _vfree(_in, insizem);
  if(_cpy && _cpy != _in)
    _vfree(_cpy, insizem);
  codexit(plug->id);
  fclose(fi);
  if(tm_verbose) { printf("\n"); fflush(stdout); }
  //if(verbose && filen > insize) plugprt(plug, totinlen, finame, FMT_TEXT, &ptc, &ptd,stdout);
  //if(memused()) printf("Mem allocated not freed null\n");
  return totinlen;
}

void usage(char *pgm, int bsize) {
  char s[50] = "Unkown Compiler";
    #if defined(__clang__)
  sprintf(s, "Clang %d.%d.%d",  __clang_major__, __clang_minor__, __clang_patchlevel__);
    #elif defined(__GNUC__)
  sprintf(s, "GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    #elif defined(_MS_VER)
  int build = 0;
      #ifdef _MSC_FULL_VER
  build = _MSC_FULL_VER % 100000;
      #endif
  snprintf(buffer, size, "%d.%02d.%05d", _MSC_VER / 100, minor = _MSC_VER % 100, build);
    #endif
  fprintf(stderr, "\nTurboBench Copyright (c) 2013-2026 Powturbo %s [%s] | %s\n", __DATE__, s, _cpubrand);
  fprintf(stderr, "Usage: %s [options] [file]\n", pgm);
  fprintf(stderr, " -eS      S = compressors/groups separated by '/' levels can be specified after ','. Ex. -ezlib,1/FAST\n");
  fprintf(stderr, " -b#s     # = blocksize {%d}\n", bsize);
  fprintf(stderr, " -d#      # = log2 dictionary size: 15-30 {blocksize}.Only brotli,lzham,lzlib,lzma,zstd\n");
  fprintf(stderr, " -B#s     # = max. filesize to benchmark {1GB} ex. -B4G\n");
  fprintf(stderr, " -s#s     # = min. buffer size to duplicate & test small files (ex. -s50)\n");
  fprintf(stderr, "          s = modifier s:K,M,G=(1000, 1.000.000, 1.000.000.000) s:k,m,h=(1024,1Mb,1Gb). {m} ex. 64k or 64K\n");
  fprintf(stderr, " -r       process directories recursively\n");
  fprintf(stderr, " -a#      add suffix # to tbb filename. file.tbb->file.#.tbb\n");
  fprintf(stderr, "Benchmark:\n");
  fprintf(stderr, " -iX,Y    Decompression/Compression iterations {3,3}. e.g. -i15,15\n");
  fprintf(stderr, " -t#      # = min. time in seconds per iterations.{1}\n");
  fprintf(stderr, " -S#      Sleep # sec. after 2 min. processing mimizing CPU throttling {20}\n");
  fprintf(stderr, " -K#t     Max. time limit for all benchmarks {24h}\n");
  fprintf(stderr, "          t = M:millisecond s:second m:minute h:hour. ex. 3h\n");
  fprintf(stderr, " -D       No process real-time priority setting\n");
  fprintf(stderr, "Check:\n");
  fprintf(stderr, " -C#      #=0 compress only, #1=No check #=2 ignore errors, #=3 exit on error, #=4 abort on error\n");
  fprintf(stderr, " -f#      check reading/writing outside bounds: #=1 compress, #=2 decompress, #3:both\n");
  fprintf(stderr, "Output:\n");
  fprintf(stderr, " -v#      # = verbosity 0..3 {1}\n");
  fprintf(stderr, " -RX,Y    Show/Reveal only when compression/decompression speed > X/Y MB/s\n");
  fprintf(stderr, " -Kstr    Rank Aggregation: str = combination of R/C/D = Ratio/Comp./Decomp. {RCD}\n");
  fprintf(stderr, " -kstr    str = Remark/Comment string\n");
  fprintf(stderr, " -U       print memory/stack usage\n");
  fprintf(stderr, " -l#      # = 1 : print all groups/plugins, # = 2 : print all codecs\n");
  fprintf(stderr, " -S#      Plot transfer speed: #=1 Comp        speedup #=2 Decomp speedup #=3 Comp        'MB/s' #=4 Decomp 'MB/s'\n");
  fprintf(stderr, "                               #=4 Comp+Decomp speedup                    #=5 Comp+Decomp 'MB/s'\n");
  fprintf(stderr, " -p#      #='print format' 1=text 2=html 3=htm 4=markdown 5/6:vBulletin 7:csv(comma) 8=tsv(tab)\n");
  fprintf(stderr, " -Q#      # Plot window 0:1920x1080, 1:1600x900, 2:1280x720, 3:800x600 {1}\n");
  fprintf(stderr, " -g       -g:no merge w/ old result 'file.tbb', -gg:process w/o output (use for fuzzing)\n");
  fprintf(stderr, " -o       print on standard output\n");
  fprintf(stderr, " -G       plot memcpy\n");
  fprintf(stderr, " -w       Plot Speedup linear x-axis {log}\n");
  fprintf(stderr, " -z       Plot Ratio/Speed logarithmic x-axis {linear}\n");
  fprintf(stderr, "Multiblock/Join\n");
  fprintf(stderr, " -Moutput Step 1: Concatenate all input files into a single output file organized into multiple blocks\n");
  fprintf(stderr, " -m       Step 2: Process each block independently from the merged file created in Step 1\n");
  fprintf(stderr, " -N       text files character delimiter (ex. -N9 for newline, 1 block/line)\n");
  BEUSAGE;
  fprintf(stderr, "ex. ./turbobench enwik9 -eFAST/bzip2/lzma,5,9\n");
  fprintf(stderr, "ex. ./turbobench enwik9 -eFAST/OPTIMAL/bsc,0:e2 -i0\n");
  fprintf(stderr, "ex. ./turbobench eECODER -k\"entropy coder test\"\n");
  fprintf(stderr, "ex. ./turbobench enwik9 -elzma,9:fb273:lc2:lp2:t2\n");
  fprintf(stderr, "\n");
  exit(0);
}

void printfile(char *finame, int xstdout, int fmt, char *rem) {
  unsigned long long totinlen;
  int       k = plugread(plugt, finame, &totinlen);
  char      *p, s[256];
  if(k < 0)
    die("file open error for '%s'\n", finame);

  if(!k) return;
  strncpy(s, finame, 255);
  s[255]=0;
  if((p = strrchr(s,'.')) && !strcmp(p, ".tbb"))
    *p = 0;
  if(p = strrchr(s,'@')) *p = 0;
  plugprts(plugt, k, s, xstdout, totinlen, fmt, rem);
}

  #ifdef __MINGW32__
extern int _CRT_glob = 1;
  #endif

int main(int argc, char* argv[]) {
  int xstdout=-1,xstdin=-1;
  int                recurse  = 0, xplug = 0,tm_Repk=1,plot=-1,fmt=0,fno,merge=0,rprio=1;
  unsigned           bsize    = 1u<<30, bsizex=0;
  unsigned long long filenmax = 0;
  char               *scmd = NULL, *xcmd = NULL, *trans=NULL,*beb=NULL,*rem="",s[2049], fsuffix[17]="";
  char               *_argvx[1], **argvx=_argvx;                                          if(verbose > 5) printf("START1\n");fflush(stdout);
  
  cpubrand(_cpubrand, 64); 
  
  int c, digit_optind = 0;                                              
  for(;;) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
      { "help",     0, 0, 'h'},
      { 0,          0, 0, 0}
    };
    if((c = getopt_long(argc, argv, "0:1:2:3:4:5:6:7:8:9:a:b:B:C:d:De:E:F:f:gGi:I:j:J:k:K:l:L:mM:N:oO:Pp:Q:r:Rs:S:t:T:Uv:V:W:w:X:x:Y:y:Z:z:", long_options, &option_index)) == -1) break;
    switch(c) {
      case 0:
        printf("Option %s", long_options[option_index].name);
        if(optarg) printf (" with arg %s", optarg);  printf ("\n");
        break;
      case 'a': snprintf(fsuffix, 16, "@%s", optarg); fsuffix[16] = 0; break;
      case 'b': bsize      = argtoi(optarg,Mb); bsizex++; break;
      case 'B': filenmax   = argtol(optarg, 'G');     break;
      case 'C': cmp        = atoi(optarg);            break;
      case 'd': coddicsize(argtoi(optarg,0));         break;
      //case 'D': dict     = optarg;                 break;
      case 'D': rprio      = 0;                       break;
      case 'e': scmd       = optarg;                  break;
//    case 'E': xcmd       = optarg;                   break;
      case 'f': fuzz       = atoi(optarg);            break;
      case 'F': fac        = strtod(optarg, NULL);    break;
      case 'g': merge++;                              break;
      case 'G': plotmcpy++;                           break;

      case 'i':
      case 'I': { char *q = strchr(optarg,','); if((tm_Rep  = atoi(optarg))<=0) tm_rep=tm_Rep=1; if(q && (tm_Rep2 = atoi(q+1))<=0) tm_rep=tm_Rep2=1;}  break;
      case 'J': if((tm_Rep2 = atoi(optarg))<=0) tm_rep=tm_Rep2=1; break;
      case 'K': { char *q = optarg; rankmode = (strchr(q,'R')?RANK_RATIO:0) | (strchr(q,'C')?RANK_COMP:0) | (strchr(q,'D')?RANK_DECOMP:0); } break;
      case 'k': rem       = optarg;                   break;
      case 'L': tm_slp    = atoi(optarg);             break;

      case 'l': xplug     = atoi(optarg);             break;
      case 'M': beb       = optarg;                   break;
      case 'm': mode++;                               break;
      case 'N': delim     = atoi(optarg);             break;
      case 'o': xstdout++;                            break;
      case 'p': fmt       = atoi(optarg);             break;
      case 'P': mcpy++;                               break;
      case 'Q': divxy     = atoi(optarg);
                if(divxy>3) divxy=3;                  break;
      case 'R' :{ char *q = strchr(optarg,','); if((tc_smin = atoi(optarg)) <=0) tc_smin=1000; if(q && (td_smin = atoi(q+1))<=0) td_smin = 100; printf("minc=%.1f,mind=%.1f\n", tc_smin,td_smin); }
      case 'r': recurse++;                            break;
      case 's': mininlen  = argtoi(optarg,1);         break;
      case 'S': speedup   = atoi(optarg); if(speedup < 0 || speedup > SP_TRANSFER) speedup=SP_TRANSFER; break;
      case 't': tm_tx     = atoi(optarg);             break;
      case 'T': tm_TX     = atoi(optarg);             break;
      case 'U': memout++;                             break;
      case 'v': verbose   = atoi(optarg);             break;
      case 'V': tm_verbose= atoi(optarg);             break;
      case 'Y': seg_ans   = argtoi(optarg,1);         break;
      case 'Z': seg_huf   = argtoi(optarg,1);         break;
      case 'w': xlog      =  xlog?0:1;                break;
      case 'x': ylog      =  ylog?0:1;                break;
      case 'y': xlog2     = xlog2?0:1;                break;
      case 'z': ylog2     = ylog2?0:1;                break;
      BEOPT;
      case 'h':
      default:
        usage(argv[0], bsize);
    }
  }                                                                                 if(verbose > 5) printf("START2\n");fflush(stdout);
  if(xplug) {
    xplug==1?plugsprt():plugsprtv(stdout, fmt);
    exit(0);
  }
  tm_init(0, tm_verbose);
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
                                                                                    if(verbose > 5) printf("START3\n");fflush(stdout);
  if(fmt) {
    if(argc <= optind) die("no input file specified");
    for(fno = optind; fno < argc; fno++)
      printfile(argvx[fno], xstdout, fmt, rem);
    exit(0);
  }
  tm_Repk = 1;                                                                      if(verbose > 5) printf("START4\n");fflush(stdout);
  if(rprio) {
      #ifdef _WIN32
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
      #else
    setpriority(PRIO_PROCESS, 0, -19);
      #endif
  }
  if(!scmd) scmd = "FAST";                                                          if(verbose > 5) printf("%s\n", scmd);fflush(stdout);
  for(s[0] = 0;;) {
    char *q;
    int  i = 0;
    if(!*scmd) break;
    if(q = strchr(scmd,'/')) *q = '\0';
    FILE *fi = fopen("turbobench.ini", "r");    //if(!fi) { char s[1024];  strcpy(s, rootdir); strcat(s, "turbobench.ini");  fi = fopen(s, "r");                                                           if(verbose > 5) printf("ini=%s %s\n", s, fi?"found":"not found");    }
    if(fi) {
      char ss[LSIZE+1];
      while(fgets(ss, LSIZE, fi)) {
        char *t = ss,*u;
        while(isspace(*t)) t++; u = t; while(isalnum(*u) || ispunct(*u)) u++; *u = 0;  //printf("search:%s\n ", t);
        if(!strcmp(scmd, t)) {
          for(t = ++u; isspace(*t); t++);
          u = t; while(isalnum(*u) || ispunct(*u)) u++; *u = 0;
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
  }                                                                 if(verbose > 5) printf("plugreg\n");fflush(stdout);

  unsigned k = plugreg(plug, s, 0, bsize, bsizex);
  if(k > 1 && argc == 1 && !strcmp(argvx[0],"stdin")) die("multiple codecs not allowed when reading from stdin");

  if(beb) { bebuild(&argvx[optind], argc-optind, recurse, beb, filenmax, delim); exit(0); }
  BEINI;
                                                                                                    if(verbose > 5) printf("Process files\n");fflush(stdout);
  unsigned long long totinlen = 0;
  int       krep;
  plug_t   *p;
  char     *finame = "";
  tm_t      tmk0 = tminit();
  for(p = plugt; p < plugt+k; p++) p->tc = p->td = DBL_MAX;
  for(krep = 0; krep < tm_Repk; krep++) {
    if(tm_Repk > 1)
      printf("Benchmark: %d from %d\n", krep+1, tm_Repk);
    for(p = plug; p < plug+k; p++) {
      plug_t *g = &plugt[p - plug];
      totinlen = 0; g->len = g->tck = g->tdk = g->memc = g->memd = g->stkc = g->stkd = g->rank = 0;
      BEFILE;
      for(fno = optind; fno < argc; fno++) {
        finame    = argvx[fno];                                                                            if(verbose > 1) printf("%s,%u\n", finame, filenmax);fflush(stdout);
        p->len    = p->tc = p->td = p->memc = p->memd = p->stkc = p->stkd = 0;
        totinlen += plugfile(p, finame, filenmax, bsize, plugr, tid, krep);
        g->len   += p->len;
        g->tck   += p->tc;
        g->tdk   += p->td;
        g->err    = g->err?g->err:p->err;
        if(p->memc > g->memc) g->memc = p->memc;
        if(p->memd > g->memd) g->memd = p->memd;
        if(p->stkc > g->stkc) g->stkc = p->stkc;
        if(p->stkd > g->stkd) g->stkd = p->stkd;
      }
      g->id  = p->id;
      g->s   = p->s;
      g->lev = p->lev;
      strcpy(g->prm, p->prm);
      if(g->tck < g->tc) g->tc = g->tck;
      if(g->tdk < g->td) g->td = g->tdk;
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
  if(!totinlen) exit(0);
  sprintf(s, "%s%s.tbb", finame, fsuffix);
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
    char tms[30];
    time_t tm;
    time(&tm);
    struct tm *ltm = localtime(&tm);
    sprintf(tms, "%.4d-%.2d-%.2d.%.2d:%.2d:%.2d", 1900 + ltm->tm_year, ltm->tm_mon+1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);

    plug_t *g;
    fprintf(fo, "dataset\tsize\tcsize\tdtime\tctime\tcodec\tlevel\tparam\tcmem\tdmem\tcstack\tdstack\ttime\n");
    for(p = plugt; p < plugt+k; p++) {
      for(g = plug; g < plug+gk; g++)
        if(g->id >= 0 && !strcmp(g->s, p->s) && g->lev == p->lev && !strcmp(g->prm, p->prm)) {
          int u = 0;                                                             
          if(g->len == p->len) {
            if(g->tc < p->tc) { p->tc = g->tc; u++; }
            if(g->td < p->td) { p->td = g->td; u++; }

            if(g->memc > p->memc) { p->memc = g->memc; u++; }
            if(g->memd > p->memd) { p->memd = g->memd; u++; }
            if(g->stkc > p->stkc) { p->stkc = g->stkc; u++; }
            if(g->stkd > p->stkd) { p->stkd = g->stkd; u++; }
            strcpy(p->tms, u?tms:g->tms);
          }                                                                      
          g->id = -1;
          break;
        }
      fprintf(fo,   "%s\t%"PRId64"\t%"PRId64"\t%.6f\t%.6f\t%s\t%d\t%s\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%s\n", 
                 finame, totinlen, p->len,    p->td,p->tc,p->s,p->lev,p->prm[0]?p->prm:"?", p->memc, p->memd, p->stkc, p->stkd, p->tms[0]?p->tms:tms);
    }
    for(g = plug; g < plug+gk; g++)
      if(g->id >= 0 /*&& g->tc > 1e-10 && g->td > 1e-10 && !plug->err*/) fprintf(fo, "%s\t%"PRId64"\t%"PRId64"\t%.6f\t%.6f\t%s\t%d\t%s\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%"PRId64"\t%s\n", 
                                 finame, totinlen, g->len, g->td, g->tc, g->s, g->lev, g->prm[0]?g->prm:"?", g->memc, g->memd, g->stkc, g->stkd, g->tms[0]?g->tms:tms);
    fclose(fo);
    printfile(s, 0, FMT_TEXT, rem);
    plug_t plugv[SVG_PLUGMAX],*vp=plugv; int x = 0;
    for(g = plug; g < plug+gk; g++)
      if(g->id >= 0) { *vp = *g; vp->tc = TMBS(totinlen, vp->tc); vp->td = TMBS(totinlen, vp->td);  vp++; if(vp-plugv >= SVG_PLUGMAX) break; }
    chart(plugv, vp - plugv, finame, totinlen);
  }

    #ifdef _WIN32          // Finish!
  Beep( 440, 100 );
    #else
  putchar('\a');
    #endif
}
