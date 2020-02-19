
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <memory.h>
#undef EOF

#pragma pack(1)

typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;
typedef signed long long sqword;


template <class T> void bzero( T &_p ) { int i; byte* p = (byte*)&_p; for( i=0; i<sizeof(_p); i++ ) p[i]=0; }
//template <class T, int N> void bzero( T (&p)[N] ) { int i; for( i=0; i<N; i++ ) p[i]=0; }

template <class T, int N> void bzero( T (&p)[N] ) { 
  byte* q = (byte*)&p; uint _N=N*sizeof(T);
  uint i; for( i=0; i<_N; i++ ) q[i]=0; 
}

template <class T> void bzero( T* p, int N ) { int i; for( i=0; i<N; i++ ) p[i]=0; }

template <class T, int N, int M> void bzero( T (&p)[N][M] ) { int i; for( i=0; i<N*M; i++ ) p[0][i]=0; }

template <class T> T Min( T x, T y ) { return (x<y) ? x : y; }
template <class T> T Max( T x, T y ) { return (x>y) ? x : y; }

#define macro_Min( x, y ) (((x)<(y)) ? (x) : (y))
#define macro_Max( x, y ) (((x)>(y)) ? (x) : (y))

template <class T,int N> constexpr int DIM( T (&wr)[N] ) { return sizeof(wr)/sizeof(wr[0]); };
#define AlignUp(x,r) ((x)+((r)-1))/(r)*(r)
template<byte a,byte b,byte c,byte d> struct wc { 
  static const unsigned int n=(d<<24)+(c<<16)+(b<<8)+a; 
  static const unsigned int x=(a<<24)+(b<<16)+(c<<8)+d;
};

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline)) 
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
// #define __assume_aligned(x,y) x=(byte*)__builtin_assume_aligned((void*)x,y)
 #define __assume_aligned(x,y) (x=decltype(x)(__builtin_assume_aligned((void*)x,y)))
 #define restrict __restrict
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

#define if_e0(x) if(__builtin_expect((x),0))
#define if_e1(x) if(__builtin_expect((x),1))
#define for_e0(x,y,z) for( (x); __builtin_expect((y),0); (z) )
#define for_e1(x,y,z) for( (x); __builtin_expect((y),1); (z) )

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
 #define __builtin_expect(x,y) (x)
// #define __assume_aligned(x,y) 
 #define __assume_aligned(x,y) __assume( (((byte*)x)-((byte*)0))%(y)==0 )
 #define restrict __restrict
 //#include "intrin.h"
 #ifndef COMMON_SKIP_BSF
 extern "C" {
 byte __cdecl _BitScanForward( uint* _Index, uint _Mask);
 byte __cdecl _BitScanForward64( uint* _Index, qword _Mask );
 }
 #endif
#endif

#if !defined(_MSC_VER) && !defined(__INTEL_COMPILER)
 #define __assume(x) (x)
#endif

#ifdef INC_FLEN
static uint flen( FILE* f ) {
  fseek( f, 0, SEEK_END );
  uint len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}
#endif

#ifdef INC_LOG2I
static uint log2i( uint x ) {
#if ((defined __GNUC__) || (defined __INTEL_COMPILER))
 #ifdef __GNUC__
  return 31-__builtin_clz(x);
 #else
  return _bit_scan_reverse(x);
 #endif
#else
  uint i; 
  for( i=0; i<32; i++,x>>=1 ) if( x==0 ) break;
  return i-1;
#endif
}
#endif

#if defined(__x86_64) || defined(_M_X64)
 #define X64
 #define X64flag 1
#else
 #undef X64
 #define X64flag 0
#endif
