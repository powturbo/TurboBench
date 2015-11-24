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

//	    conf.h - config & common 
#ifndef CONF_H
#define CONF_H
//------------------------- Compiler ------------------------------------------
  #if defined(__GNUC__)
#define ALIGNED(t,v,n)  t v __attribute__ ((aligned (n))) 
#define ALWAYS_INLINE   inline __attribute__((always_inline))
#define NOINLINE        __attribute__((noinline))
#define _PACKED 		__attribute__ ((packed))
#define likely(x)     	__builtin_expect((x),1)
#define unlikely(x)   	__builtin_expect((x),0)

#define popcnt32(__x) 	__builtin_popcount(__x)
#define popcnt64(__x) 	__builtin_popcountll(__x)

    #if defined(__i386__) || defined(__x86_64__)
static inline int __bsr32(int x) {             asm("bsr  %1,%0" : "=r" (x) : "rm" (x) ); return x; }
static inline int bsr32(  int x) { int b = -1; asm("bsrl %1,%0" : "+r" (b) : "rm" (x) ); return b + 1; }
static inline int bsr64(unsigned long long x) { return x?64 - __builtin_clzll(x):0; }
#define bsr16(__x) bsr32(__x)
    #else
static inline int bsr32(int x               ) { return x?32 - __builtin_clz(  x):0; }
static inline int bsr64(unsigned long long x) { return x?64 - __builtin_clzll(x):0;
}
    #endif
#define ctzll(__x) __builtin_ctzll(__x)
  #elif _MSC_VER
#define ALIGNED(x) __declspec(align(x))
#define ALWAYS_INLINE __forceinline
#define NOINLINE __declspec(noinline)
#define THREADLOCAL   __declspec(thread)
#define likely(x)     	__builtin_expect((x),1)
#define unlikely(x)   	__builtin_expect((x),0)
#define __builtin_prefetch(x) /* _mm_prefetch(x, _MM_HINT_NTA) */
  #endif 

//--------------- Unaligned memory access -------------------------------------
  #if defined(__i386__) || defined(__x86_64__) || defined(__powerpc__) ||\
    defined(__ARM_FEATURE_UNALIGNED) || defined(__aarch64__) || defined(__arm__) ||\
    defined(__ARM_ARCH_4__) || defined(__ARM_ARCH_4T__) || \
    defined(__ARM_ARCH_5__) || defined(__ARM_ARCH_5T__) || defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__) || \
    defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__)  || defined(__ARM_ARCH_6T2__) || defined(__ARM_ARCH_6Z__)   || defined(__ARM_ARCH_6ZK__)
#define ctou16(__cp) (*(unsigned short *)(__cp))
#define ctou32(__cp) (*(unsigned       *)(__cp))

    #if defined(__i386__) || defined(__x86_64__) || defined(__powerpc__)
#define ctou64(__cp)       (*(unsigned long long *)(__cp))
#define ctou(__cp_t, __cp) (*(__cp_t *)(__cp))
    #endif

  #elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7S__)
struct _PACKED shortu    { unsigned short     s; };
struct _PACKED unsignedu { unsigned           u; };
struct _PACKED longu     { unsigned long long l; };

#define ctou16(__cp) ((struct shortu    *)(__cp))->s
#define ctou32(__cp) ((struct unsignedu *)(__cp))->u
#define ctou64(__cp) ((struct longu     *)(__cp))->l
  #endif

  #ifdef ctou16
#define utoc16(__x,__cp) ctou16(__cp) = __x
  #else
static inline unsigned short     ctou16(const void *cp) { unsigned short     x; memcpy(&x, cp, sizeof(x); return x; }
static inline               void utoc16(unsigned short     x, void *cp ) { memcpy(cp, &x, sizeof(x)); }
  #endif

  #ifdef ctou32
#define utoc32(__x,__cp) ctou32(__cp) = __x
  #else
static inline unsigned           ctou32(const void *cp) { unsigned           x; memcpy(&x, cp, sizeof(x); return x; }
static inline               void utoc32(unsigned           x, void *cp ) { memcpy(cp, &x, sizeof(x)); }
  #endif

  #ifdef ctou64
#define utoc64(__x,__cp) ctou64(__cp) = __x
  #else
static inline unsigned long long ctou64(const void *cp) { unsigned long long x; memcpy(&x, cp, sizeof(x); return x; }
static inline               void utoc64(unsigned long long x, void *cp ) { memcpy(cp, &x, sizeof(x)); }
  #endif

#define ctou24(__cp) (ctou32(__cp) & 0xffffff)
#define ctou48(__cp) (ctou64(__cp) & 0xffffffffffffull)

//--------------------- wordsize ----------------------------------------------
  #if defined(__64BIT__) || defined(_LP64) || defined(__LP64__) || defined(_WIN64) ||\
    defined(__x86_64__) || defined(_M_X64) ||\
    defined(__ia64) || defined(_M_IA64) ||\
    defined(__aarch64__) ||\
    defined(__mips64) ||\
    defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) ||\
    defined(__s390x__)
#define __WORDSIZE 64
  #else
#define __WORDSIZE 32 
  #endif
#endif

//---------------------misc ---------------------------------------------------   
#define SIZE_ROUNDUP(__n, __a) (((size_t)(__n) + (size_t)((__a) - 1)) & ~(size_t)((__a) - 1))
  #ifndef min
#define min(x,y) (((x)<(y)) ? (x) : (y))
#define max(x,y) (((x)>(y)) ? (x) : (y))
  #endif
  
#define TEMPLATE2_(__x, __y) __x##__y
#define TEMPLATE2(__x, __y) TEMPLATE2_(__x,__y)

#define TEMPLATE3_(__x,__y,__z) __x##__y##__z
#define TEMPLATE3(__x,__y,__z) TEMPLATE3_(__x, __y, __z)

//--- NDEBUG -------
  #ifdef NDEBUG
#define AS(expr, fmt,args...)
#define AC(expr, fmt,args...) if(!(expr)) { fprintf(stderr, fmt, ## args ); fflush(stderr); abort(); }
#define die(fmt,args...) do { fprintf(stderr, fmt, ## args ); fflush(stderr); exit(-1); } while(0)
  #else
#include <stdio.h>
#define AS(expr, fmt,args...) if(!(expr)) { fflush(stdout);fprintf(stderr, "%s:%s:%d:", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, fmt, ## args ); fflush(stderr); abort(); }
#define AC(expr, fmt,args...) if(!(expr)) { fflush(stdout);fprintf(stderr, "%s:%s:%d:", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, fmt, ## args ); fflush(stderr); abort(); }
#define die(fmt,args...) do { fprintf(stderr, "%s:%s:%d:", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, fmt, ## args ); fflush(stderr); exit(-1); } while(0)
  #endif

