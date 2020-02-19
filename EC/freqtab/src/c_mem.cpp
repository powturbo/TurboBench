
//#include "coder/common.inc"
#include <stdio.h>
#include <stdlib.h>

typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

#include "coder/coder.h"

#include "valloc.inc"

#if defined(_MSC_VER)
#define fseeko64 _fseeki64 
#define ftello64 _ftelli64
#endif

qword flen64( FILE* f ) { 
  qword p = ftello64(f);
  fseeko64( f, 0, SEEK_END );
  qword s = ftello64(f);
  fseeko64( f, p, SEEK_SET );
  return s; 
}

#if 1
#include "../freqtab.h"
int freqtabenc(unsigned char *in, int inlen, unsigned char *out, int outsize) {
  uint r;
  void* M = VAlloc(0,C_get_object_size());
  if( M==0 ) return 0;
  C_init(M);
  C_addinp(M, in,  inlen);
  C_addout(M, out, outsize);
  do r=C_process(M,0); while(r==1); 
  r = C_getoutsize(M);
  VFree(M);
  return r;
}

int freqtabdec(unsigned char *in, int inlen, unsigned char *out, int outlen) {
  uint r;
  void* M = VAlloc(0,C_get_object_size());
  if( M==0 ) return 0;
  C_init(M);
  C_addinp(M, in,  inlen );
  C_addout(M, out, outlen);
  do r=C_process(M,1); while(r==1); 
  r = C_getoutsize(M);
  VFree(M);
  return r;
}
#else
int main( int argc, char** argv ) {
  if( argc<4 ) return 1;

  void* M = VAlloc(0,C_get_object_size());
  if( M==0 ) return 1;

  uint r, f_DEC = (argv[1][0]=='d');
  FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2;
  FILE* g = fopen(argv[3],"wb"); if( g==0 ) return 3;

  qword f_len = flen64(f);
  if( f_len>=(1ULL<<32) ) f_len=0xFFFFFFFF;

  uint inpbufsize = f_len;
  byte* inpbuf = (byte*)VAlloc(0,inpbufsize); if( inpbuf==0 ) return 1;
  uint outbufsize = 4*f_len;
  byte* outbuf = (byte*)VAlloc(0,outbufsize); if( outbuf==0 ) return 1;

  inpbufsize = fread(inpbuf,1,f_len,f);

  C_init(M);
  C_addinp(M, inpbuf, inpbufsize );
  C_addout(M, outbuf, outbufsize );
#if 0
  // this is enough for "model1.cpp" memory-to-memory API
  C_process(M,f_DEC); 
#else
  // this is necessary for "model.cpp" API, it would ask for more input
  do r=C_process(M,f_DEC); while(r==1); 
#endif
  r = C_getoutsize(M);

  fwrite( outbuf, 1,r, g );

  fclose(g);
  fclose(f);

  return 0;
}
#endif
