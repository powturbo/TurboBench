
//#include "coder/common.inc"

#include <stdio.h>
#include <stdlib.h>

typedef unsigned int   uint;
typedef unsigned char  byte;

#include "coder/coder.h"

#include "valloc.inc"

struct FileProc {

  enum { inpbufsize=1<<16, outbufsize=1<<16 };

  byte inpbuf[inpbufsize];
  byte outbuf[outbufsize];

  void processfile( void* M, FILE* f, FILE* g, uint f_DEC ) {
    uint l,r;

    C_init(M);
    C_addout(M, outbuf, outbufsize );

    while(1) {
      r = C_process(M,f_DEC); 
      if( r==1 ) {
        l = fread( inpbuf, 1,inpbufsize, f );
        C_addinp( M, inpbuf, l ); 
      } else { // r0=quit, r3=error
        l = C_getoutsize(M);
        if( l>0 ) fwrite( outbuf, 1,l, g ); 
        if( r!=2 ) break;
        C_addout( M, outbuf, outbufsize );
      } // if
    } // while
  } // func

};

int main( int argc, char** argv ) {

  if( argc<4 ) return 1;

  void* M = VAlloc(0,C_get_object_size());
  FileProc* C = (FileProc*)VAlloc(0,sizeof(FileProc));
  if( (M==0) || (C==0) ) return 1;

  uint f_DEC = (argv[1][0]=='d');
  FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2;
  FILE* g = fopen(argv[3],"wb"); if( g==0 ) return 3;

  C->processfile( M, f, g, f_DEC );

  fclose(g);
  fclose(f);

  return 0;
}
