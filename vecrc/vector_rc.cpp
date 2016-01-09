
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#pragma pack(1)

#ifndef __INTEL_COMPILER
#define restrict __restrict
#endif

#ifdef __GNUC__
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
#else
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;
typedef signed long long sqword;
typedef signed short sint16;
typedef signed char* pchar;
typedef word* pword;
typedef short* pshort;
typedef uint* puint;
typedef byte* pbyte;

/*uint flen( FILE* f ) {
  fseek( f, 0, SEEK_END );
  uint len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}*/

#define ProcMode 0
#define Model Model_5_0
#define PROC Model_5_0_process
#include "model.h"
#undef PROC
#undef ProcMode
#undef Model

#define ProcMode 1
#define Model Model_5_1
#define PROC Model_5_1_process
#include "model.h"
#undef PROC
#undef ProcMode
#undef Model

#include "vector_rc.h"
int vecrcenc(unsigned char *ip, int ilen, unsigned char *op) {
  return Model_5_0_process( ip, ilen, op)-op;
}

int vecrcdec(unsigned char *ip, int olen, unsigned char *op) {
  return Model_5_1_process( ip, olen, op)-ip;
}

#if 0 
int main( int argc, char** argv ) {

  test:

  int mode = argc<2 ? 1 : argv[1][0]!='d';
  FILE* f = fopen( argc<3 ? mode ? "book1"     : "book1.ari" : argv[2], "rb" );
  FILE* g = fopen( argc<4 ? mode ? "book1.ari" : "book1.unp" : argv[3], "wb" );
  if( (f==0) || (g==0) ) return 1;

  if( mode ) {
    uint f_len = flen( f );
    fwrite( &f_len, 1,4, g );
    Model_5_0_process( f, g, f_len );
  } else {
    uint f_len;
    fread( &f_len, 1,4, f );
    Model_5_1_process( g, f, f_len );
  }
 
  fclose(f);
  fclose(g);

  if( (argv[1][0]=='t') && (argc==4) ) { 
    argv[2]=argv[3]; argv[3]=&argv[1][1];
    argv[1][0] = 'd';
    goto test;
  }

  return 0;
}
#endif
