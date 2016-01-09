
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "narans.h"
// Timer

 #if 0
// Just some platform utilities.
#ifndef PLATFORM_H_INCLUDED
#define PLATFORM_H_INCLUDED

// x86 intrinsics (__rdtsc etc.)

#if defined(_MSC_VER)

#define _CRT_SECURE_NO_DEPRECATE
#include <intrin.h>
#define ALIGNSPEC(type,name,alignment) __declspec(align(alignment)) type name

#elif defined(__GNUC__)

#include <x86intrin.h>
#define ALIGNSPEC(type,name,alignment) type name __attribute__((aligned(alignment)))

#else
#error Unknown compiler!
#endif

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#define PRIu64 "llu"

double timer()
{
    LARGE_INTEGER ctr, freq;
    QueryPerformanceCounter(&ctr);
    QueryPerformanceFrequency(&freq);
    return 1.0 * ctr.QuadPart / freq.QuadPart;
}

#elif defined(__linux__)

#define __STDC_FORMAT_MACROS
#include <time.h>
#include <inttypes.h>
#include <assert.h>

static inline double timer()
{
    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    int status = clock_gettime(CLOCK_MONOTONIC, &ts);
    assert(status == 0);
    return double(ts.tv_sec) + 1.0e-9 * double(ts.tv_nsec);
}

#else

#error Unknown platform!

#endif

#endif // PLATFORM_H_INCLUDED
 #endif

// Simple byte-aligned rANS encoder/decoder - public domain - Fabian 'ryg' Giesen 2014
//
// Not intended to be "industrial strength"; just meant to illustrate the general
// idea.


#include <stdint.h>



static unsigned short cnt [0x10000];
static unsigned short cum [0x10000];

#define RANS_BYTE_L (1u << 23)  // lower bound of our normalization interval

// State for a rANS encoder. Yep, that's all there is to it.
typedef unsigned long  RansState;


static inline void RansEncPut(RansState &x, unsigned char** pptr, unsigned long  start, unsigned long  freq, unsigned long  scale_bits)
{
 unsigned long  x_max = ((RANS_BYTE_L >> scale_bits) << 8) * freq; // this turns into a shift.

    unsigned char* ptr = *pptr;
    if (x >= x_max)
    {
    *--ptr = (unsigned char) (x & 0xff);x >>= 8;
                                        if (x >= x_max){*--ptr = (unsigned char) (x & 0xff);x >>= 8;}
    }
    *pptr = ptr;
    // x = C(s,x)
    x = ((x / freq) << scale_bits) + (x % freq) + start;
}

static inline void RansEncFlush(RansState* r, unsigned char** pptr)
{
    unsigned long  x = *r;
    unsigned char* ptr = *pptr;

    ptr -= 4;
    ptr[0] = (unsigned char) (x >> 0);
    ptr[1] = (unsigned char) (x >> 8);
    ptr[2] = (unsigned char) (x >> 16);
    ptr[3] = (unsigned char) (x >> 24);

    *pptr = ptr;
}

// Initializes a rANS decoder.
// Unlike the encoder, the decoder works forwards as you'd expect.
static inline void RansDecInit(RansState &x, unsigned char** pptr)
{

    unsigned char* ptr = *pptr;

    x  = ptr[0] << 0;
    x |= ptr[1] << 8;
    x |= ptr[2] << 16;
    x |= ptr[3] << 24;
    ptr += 4;

    *pptr = ptr;
}


// Advances in the bit stream by "popping" a single symbol with range start
// "start" and frequency "freq". All frequencies are assumed to sum to "1 << scale_bits",
// and the resulting bytes get written to ptr (which is updated).
static inline void RansDecAdvance(RansState &x, unsigned char** pptr, unsigned long  start, unsigned long  freq, unsigned long  scale_bits)
{
    // s, x = D(x)
    x = freq * (x >> 13) + (x & 8191) - start;
    // renormalize
    unsigned char* ptr = *pptr;

    if (x < RANS_BYTE_L)
    {
    x = (x << 8) | *ptr++;
                           if (x < RANS_BYTE_L){x = (x << 8) | *ptr++;}
    }

    *pptr = ptr;

}

// --------------------------------------------------------------------------



// This is just the sample program. All the meat is in rans_byte.h.



template <class T> void alloc(T*&p, int n) {
  p=(T*)calloc(n, sizeof(T));
  if (!p) printf("No memory \n"),exit(1);
}



struct Rangecounter
{
private:

public:
  unsigned int total,ntotal,NUM;
  unsigned short cnt [256];
  unsigned short ncnt[256];
  unsigned short cum [256];
  unsigned char  ftochar[(1<<13)+16];

   int Rescale()
  {
   int cumu=0;ntotal=0;
   for (int i = 0; i < NUM; i++)
   {
   cnt[i]=ncnt[i]; for (int u = cumu; u< cumu+cnt[i]; u++)ftochar[u]=i;
   cum[i]=cumu;cumu+=cnt[i];
   ncnt[i]-= (ncnt[i]>>1);
   ntotal+=ncnt[i];
   }

  }


  void Init(unsigned int n)
  {
    NUM=n;
    for (int i = 0; i < NUM; i++) ncnt[i] = 32;
    Rescale();

  }

  void Add(int s)
  {
  (ntotal> 8190)?Rescale():ntotal++,ncnt[s]++;
  }


};

unsigned nansenc(unsigned char *in, unsigned inlen, unsigned char *out, unsigned char *oend) {
  static const unsigned long  prob_bits = 13;
  Rangecounter counter;
  RansState rans;
  counter.Init(256);

  unsigned char *ip;
  for(int i = 0; i < inlen; i++) {
    unsigned char s = in[i];
    cum[i] = counter.cum[s];
    cnt[i] = counter.cnt[s];
    counter.Add(s);
  }
  unsigned char *op = oend;
  rans = RANS_BYTE_L ;
  for(int i=inlen-1; i >= 0; i--)
    RansEncPut(rans, &op, cum[i], cnt[i], prob_bits);
  
  RansEncFlush(&rans, &op);
  memmove(out, op, oend-op);
  return oend-op;
}

unsigned nansdec(unsigned char *in, unsigned inlen, unsigned char *out, unsigned int outlen) { 
  Rangecounter counter;
  RansState rans;

  static const unsigned long  prob_bits = 13;
  static const unsigned long  prob_scale = 1 << prob_bits;
  static const unsigned long  mask=prob_scale-1;

  counter.Init(256);
  unsigned char *op = out,*ip;
  RansDecInit(rans, &op);
  for(ip = in; ip < in+inlen; ip++) {
    unsigned char s = counter.ftochar[rans&8191];
    *out++ = s;
    RansDecAdvance(rans, &ip, counter.cum[s], counter.cnt[s], mask);
    counter.Add(s);
  }
  return inlen;
}

