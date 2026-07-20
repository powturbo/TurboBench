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
//      time_.h : parameter free high precision time/benchmark functions
#define _POSIX_C_SOURCE 200809L // clocktime
#include <stdio.h>
#include <time.h>
#include <float.h>

  #ifdef _WIN32
#include <windows.h>
    #ifndef sleep
#define sleep(n) Sleep((n) * 1000)
    #endif
#define uint64_t unsigned __int64
  #else
#include <stdint.h>
#include <unistd.h>
#define Sleep(ms) usleep((ms) * 1000)
#ifdef __riscv_v
#define sleep(x)  
#endif
  #endif

#define TM_FPRE   2 // decimals
#define TM_FWIDTH 9  

#if defined (__i386__) || defined( __x86_64__ ) // ------------------ rdtsc --------------------------
  #ifdef _MSC_VER
#include <intrin.h> // __rdtsc
  #else
#include <x86intrin.h>
  #endif

  #ifdef __corei7__
#define RDTSC_INI(_c_) do { unsigned _cl, _ch;              \
  __asm volatile ("cpuid\n\t"                               \
                "rdtsc\n\t"                                 \
                "mov %%edx, %0\n"                           \
                "mov %%eax, %1\n": "=r" (_ch), "=r" (_cl):: \
                "%rax", "%rbx", "%rcx", "%rdx");            \
  _c_ = (uint64_t)_ch << 32 | _cl;              \
} while(0)

#define RDTSC(_c_) do { unsigned _cl, _ch;                  \
  __asm volatile("rdtscp\n"                                 \
               "mov %%edx, %0\n"                            \
               "mov %%eax, %1\n"                            \
               "cpuid\n\t": "=r" (_ch), "=r" (_cl):: "%rax",\
               "%rbx", "%rcx", "%rdx");\
  _c_ = (uint64_t)_ch << 32 | _cl;\
} while(0)
  #else
/*#define RDTSC(_c_) do { unsigned _cl, _ch;\
  __asm volatile ("cpuid \n"\
                "rdtsc"\
                : "=a"(_cl), "=d"(_ch)\
                : "a"(0)\
                : "%ebx", "%ecx");\
  _c_ = (uint64_t)_ch << 32 | _cl;\
} while(0)*/
#define RDTSC(_c_) do { unsigned _cl, _ch;\
   __asm volatile("rdtsc" : "=a"(_cl), "=d"(_ch) );\
  _c_ = (uint64_t)_ch << 32 | _cl;\
} while(0)
  #endif

#define RDTSC_INI(_c_) RDTSC(_c_)
#else                                          // ------------------ time --------------------------
#define RDTSC_INI(_c_)
#define RDTSC(_c_) __rdtsc()       
#endif

#ifndef TM_F
#define TM_F 1.0  // TM_F=4 -> MI/s
#endif

#ifdef _RDTSC //---------------------- rdtsc --------------------------------
#define TM_M   (CLOCKS_PER_SEC*1000000ull)
#define TM_FPRE 4
#define TM_MBS "cycle/byte"
static double TMBS(size_t l, double t) { return t/l; }

typedef uint64_t tm_t;
static tm_t   tmtime()                      { uint64_t c; RDTSC(c); return c; }
static tm_t   tminit()                      { uint64_t c; __asm volatile("" ::: "memory"); RDTSC_INI(c); return c; }
static double tmdiff(tm_t start, tm_t stop) { return (double)(stop - start); }
static int    tmiszero(tm_t t)              { return !t; }

#else          //---------------------- time -----------------------------------
#define TM_M   1
#define TM_MBS "MB/s"
static double TMBS(size_t l, double t) { return (l/t)/1000000.0; }

  #ifdef _WIN32 //-------- windows
static LARGE_INTEGER tps;

typedef unsigned __int64 tm_t;
static tm_t   tmtime()                      { LARGE_INTEGER tm; tm_t t; QueryPerformanceCounter(&tm); return tm.QuadPart; }
static tm_t   tminit()                      { tm_t t0,ts; QueryPerformanceFrequency(&tps); t0 = tmtime(); while((ts = tmtime())==t0) {}; return ts; }
static double tmdiff(tm_t start, tm_t stop) { return (double)(stop - start)/tps.QuadPart; }
static int    tmiszero(tm_t t)              { return !t; }
  #else        // Linux & compatible / MacOS
    #ifdef __APPLE__  //TODO: #include <mach/mach_time.h> uint64_t t = mach_absolute_time();
#include <AvailabilityMacros.h>
      #ifndef MAC_OS_X_VERSION_10_12
#define MAC_OS_X_VERSION_10_12 101200
      #endif
#define CIVETWEB_APPLE_HAVE_CLOCK_GETTIME (defined(__APPLE__) && defined(MAC_OS_X_VERSION_MIN_REQUIRED) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12)
      #if !(CIVETWEB_APPLE_HAVE_CLOCK_GETTIME)
#include <sys/time.h>
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 0
int clock_gettime(int /*clk_id*/, struct timespec* t) {
  struct timeval now;
  int rv = gettimeofday(&now, NULL);
  if (rv) return rv;
  t->tv_sec  = now.tv_sec;
  t->tv_nsec = now.tv_usec * 1000;
  return 0;
}
      #endif
    #endif

    #ifdef CLOCK_MONOTONIC
typedef struct timespec tm_t;
static tm_t tmtime() { struct timespec tm; clock_gettime(CLOCK_MONOTONIC, &tm); return tm; }
static int    tmiszero(tm_t t)              { return !(t.tv_sec|t.tv_nsec); }
static double tmdiff(tm_t start, tm_t stop) { return (stop.tv_sec - start.tv_sec) + (double)(stop.tv_nsec - start.tv_nsec)/1e9f; }
static tm_t   tminit()                      { tm_t t0 = tmtime(),t; while(!tmdiff(t = tmtime(),t0)) {}; return t; }
    #else
typedef unsigned long long tm_t;
static tm_t tps;

static tm_t   tmtime()                      { return __builtin_readcyclecounter(); }
static int    tmiszero(tm_t t)              { return !t; }
static double tmdiff(tm_t start, tm_t stop) { return (double)(stop - start); }
static tm_t   tminit()                      { tm_t t0,ts; tps = __builtin_readcyclecounter(); t0 = tmtime(); while((ts = tmtime()) == t0) {}; return ts; }
    #endif	
  #endif
#endif
//**************************************************************************************************************************************************************
#ifdef _WIN32
#include <powerbase.h> 
#pragma comment(lib, "powrprof.lib")
#define ProcessorInformation 11

typedef struct _PROCESSOR_POWER_INFORMATION {
  ULONG Number;
  ULONG MaxMhz;
  ULONG CurrentMhz;
  ULONG MhzLimit;
  ULONG MaxIdleState;
  ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION;

int throttling() {
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  int numCPUs = sysInfo.dwNumberOfProcessors;
  int throttled = 0;
    
  PROCESSOR_POWER_INFORMATION* ppi = (PROCESSOR_POWER_INFORMATION*)malloc(  sizeof(PROCESSOR_POWER_INFORMATION) * numCPUs);      
  NTSTATUS sts = CallNtPowerInformation(ProcessorInformation, NULL, 0, ppi, sizeof(PROCESSOR_POWER_INFORMATION) * numCPUs );
  if(!sts)
    for(int i = 0; i < numCPUs; ++i) {
      if(ppi[i].CurrentMhz < (ppi[i].MaxMhz * 0.9)) {
        throttled = 1;
        break;
      }
    }
  free(ppi);
  return throttled;
}

#elif __APPLE__
#include <notify.h>
#include <stdint.h>

int throttling() {
  int      token = 0;
  uint32_t sts = notify_register_check("com.apple.system.thermalpressurelevel", &token); if(sts) return 0;   
  uint64_t state = 0;
  sts = notify_get_state(token, &state);
  notify_cancel(token);  
  if(sts && state >= 1) return 1;
  return 0;
}

#elif __linux__
long freq_cur() { FILE *f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r"); if (!f) return -1; long freq = 0; fscanf(f, "%ld", &freq); fclose(f); return freq; }
long freq_max() { FILE *f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r"); if (!f) return -1; long freq = 0; fscanf(f, "%ld", &freq); fclose(f); return freq; }
int throttling() { long cur = freq_cur(), max = freq_max(); if(cur == -1 || max == -1) return 0; return cur < (max * 0.9); }
#else
void throttling() { return 1; }
#endif

void sleep_cool(int sec) { int s = 4; while(throttling() && s < sec) { sleep(8); s += 8; } }

//---------------------------------------- bench ----------------------------------------------------------------------
#define TM_BACK(_n_) { for(int i=0; i < (_n_); i++) printf("\b"); fflush(stdout); }
#define tm_tmin(nb) ((double)tm_min/((double)tm_rm*(nb)));

/* - 1st iteration: break the loop after tm_tx=1 sec, calculate a new repeats 'tm_rm' to avoid calling time() after each function call.\
     set min time, recalculate repeats tm_rm based on tm_tx, recalculate number of runs based on tm_TX
   - thereafter: break the loop only after 'tm_rm' repeats 
   break the loop, if there is no improvement after 16 runs*/
#define TMBEG(_tm_Reps_) { unsigned _tm_r,_tm_c = 0,_tm_R,_tm_Rx = _tm_Reps_,_tm_Rn = _tm_Reps_, _tm_i = 0; double _tm_d;  \
  for(tm_rm = tm_rep, tm_min = DBL_MAX, _tm_R = 0; _tm_R < _tm_Rn; _tm_R++) { tm_t _tm_t0 = tminit();\
    for(_tm_r = 0; _tm_r < tm_rm;) {

#define TMEND(_size_) ;\
      _tm_r++; if(tm_min == DBL_MAX && (_tm_d = tmdiff(_tm_t0, tmtime())) > tm_tx) { tm_rm = _tm_r; _tm_Rn = tm_TX/_tm_d; _tm_Rn = _tm_Rn<_tm_Rx?_tm_Rn:_tm_Rx; }\
    }\
    if((_tm_d = tmdiff(_tm_t0, tmtime())) < tm_min) { tm_min = _tm_d; _tm_c++; _tm_i = 0; } /*else if(_tm_d > tm_min*1.30) sleep_cool(8);*/\
    if(tm_verbose>1)    { printf("%*.*f %2d_%.2d\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", TM_FWIDTH,TM_FPRE, TMBS(_size_, tm_min/tm_rm),_tm_R+1,_tm_c); fflush(stdout); }\
    else if(tm_verbose) { printf("%*.*f         ",                               TM_FWIDTH,TM_FPRE, TMBS(_size_, tm_min/tm_rm));               fflush(stdout); }\
    if(++_tm_i >= 16 || _tm_d >= 60) break; if((_tm_R & 7)==7) sleep_cool(tm_slp);\
  }\
}

#define TM(_name_, _efunc_, _size_, _len_, _dfunc_) do {\
  TMBEG(tm_Rep) _efunc_; if(_size_ && !(_tm_R|_tm_r) && tm_verbose) pr(_len_,_size_); TMEND(_size_);\
  double dm = tm_min, dr = tm_rm; \
  if(tm_verbose>1)    { printf("%*.*f      \b\b\b\b\b", TM_FWIDTH, TM_FPRE, TMBS(_size_, dm/dr) ); fflush(stdout); }\
  else if(tm_verbose) { printf("%*.*f      ",           TM_FWIDTH, TM_FPRE, TMBS(_size_, dm/dr) ); fflush(stdout); }\
  \
  TMBEG(tm_Rep2) _dfunc_; TMEND(_size_);\
  dm = tm_min; dr = tm_rm; \
  if(tm_verbose>1)    { printf("%*.*f      \b\b\b\b\b", TM_FWIDTH, TM_FPRE, TMBS(_size_, dm/dr) ); fflush(stdout); }\
  else if(tm_verbose) { printf("%*.*f      ",           TM_FWIDTH, TM_FPRE, TMBS(_size_, dm/dr) ); fflush(stdout); }\
  if(tm_verbose)    { printf("%s ", _name_?_name_:#_efunc_); fflush(stdout); }\
} while(0)

#define TM0(_name_, _efunc_, _size_, _len_) do {\
  TMBEG(tm_Rep) _efunc_; if(_size_ && !(_tm_R|_tm_r) && tm_verbose) pr(_len_,_size_); TMEND(_size_);\
  double dm = tm_min, dr = tm_rm; \
  if(tm_verbose>1)    { printf("%*.*f      \b\b\b\b\b", TM_FWIDTH, TM_FPRE, TMBS(_size_, dm/dr) ); fflush(stdout); }\
  else if(tm_verbose) { printf("%*.*f      ", TM_FWIDTH, TM_FPRE, TMBS(_size_, dm/dr) );           fflush(stdout); }\
  \
  if(tm_verbose) { printf("%s ", _name_?_name_:#_efunc_); fflush(stdout); }\
} while(0)

static void pr(size_t l, size_t n) {
  double r = (double)l*100.0/n;
  if(r>0.1)       printf("%11llu %6.2f%%   ", l, r);
  else if(r>0.01) printf("%11llu %7.3f%%  ",  l, r);
  else            printf("%11llu %8.4f%% ",   l, r); fflush(stdout);
}

static unsigned tm_rep = 1u<<30, tm_Rep = 3, tm_Rep2 = 3, tm_rm, tm_RepMin = 1, tm_slp = 24, tm_verbose = 2, itempr;
static tm_t tm_0, tm_T;
static double tm_min, tm_tx = 1.0*TM_M, tm_TX = 60.0*TM_M;

static void tm_init(int _tm_Rep, int _tm_verbose) { tm_verbose = _tm_verbose; if(_tm_Rep) tm_Rep = _tm_Rep; /*itemper = temperature();*/ }

//----------------------------------------------------------------------------------------------------------------------------------
#define Kb (1u<<10)
#define Mb (1u<<20)
#define Gb (1u<<30)
#define KB 1000
#define MB 1000000
#define GB 1000000000

static unsigned argtoi(char *s, unsigned def) {
  char *p;
  unsigned n = strtol(s, &p, 10),f = 1;
  switch(*p) {
    case 'K': f = KB; break;
    case 'M': f = MB; break;
    case 'G': f = GB; break;
    case 'k': f = Kb; break;
    case 'm': f = Mb; break;
    case 'g': f = Gb; break;
    case 'B': return n; break;
    case 'b': def = 0;
    default: if(!def) return n>=32?0xffffffffu:(1u << n); f = def;
  }
  return n*f;
}
static uint64_t argtol(char *s, uint64_t def) {
  char *p;
  uint64_t n = strtol(s, &p, 10),f=1;
  switch(*p) {
    case 'K': f = KB; break;
    case 'M': f = MB; break;
    case 'G': f = GB; break;
    case 'k': f = Kb; break;
    case 'm': f = Mb; break;
    case 'g': f = Gb; break;
    case 'B': return n; break;
    case 'b': def = 0;
    default:  if(!def) return n>=64?0xffffffffffffffffu:(1ull << n); f = def;
  }
  return n*f;
}

static uint64_t argtot(char *s) {
  char *p;
  uint64_t n = strtol(s, &p, 10),f=1;
  switch(*p) {
    case 'h': f = 3600000; break;
    case 'm': f = 60000;   break;
    case 's': f = 1000;    break;
    case 'M': f = 1;       break;
    default:  f = 1000;
  }
  return n*f;
}

static void memrcpy(unsigned char *out, unsigned char *in, unsigned n) { int i; for(i = 0; i < n; i++) out[i] = ~in[i]; }
