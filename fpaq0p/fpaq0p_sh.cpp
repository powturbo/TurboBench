
//#include <stdio.h>
#include "fpaq0p_sh.h"

#define _putc(__ch, __out) { *(__out)++ = __ch; }
#define _getc(in) *(unsigned char *)in++		

typedef unsigned int   uint;
typedef unsigned long long int qword;

enum { SCALElog=15, SCALE=1<<SCALElog, mSCALE=SCALE-1, hSCALE=SCALE/2 };

template< int DECODE >
struct Rangecoder {

  enum { NUM=4, sTOP=0x01000000U, Thres=0xFF000000U };

  uint  range;
  uint  code; 
  qword lowc;
  uint  FFNum;
  uint  Cache;
  FILET *f;

  FILET *F() const { return f; }
  void Init( FILET* _f ) {
    f = _f; range = 0xFFFFFFFF; lowc = 0; FFNum = 0; Cache = 0;
    if( DECODE==1 ) for( uint _=0; _<NUM+1; _++ ) (code<<=8) += _getc(f); 
  }

  void Quit( void ) {
    if( DECODE==0 ) for( uint _=0; _<NUM+1; _++ ) ShiftLow(); 
  }

  uint Process( uint freq, uint bit ) { 
//    uint rnew = (qword(range)*(freq<<(32-SCALElog)))>>32;
    uint rnew = (range>>SCALElog)*freq;
    if( DECODE ) bit = (code>=rnew);
    bit ? range-=rnew, (DECODE ? code-=rnew : lowc+=rnew) : range=rnew;
    while( range<sTOP ) range<<=8, (DECODE ? (code<<=8)+=_getc(f) : ShiftLow());
    return bit;
  }

  uint ShiftLow( void ) {
    uint Carry = uint(lowc>>32), low = uint(lowc);
    if( low<Thres || Carry ) {
      _putc( Cache+Carry, f );
      for (;FFNum != 0;FFNum--) _putc( Carry-1, f );
      Cache = low>>24;
    } else FFNum++;
    return lowc = (low<<8);
  }

};

struct Predictor {
  short cxt; 
  short p[256-1]; 

  Predictor() { for( unsigned i=0; i<sizeof(p)/sizeof(p[0]); i++ ) p[i]=hSCALE; byte(); }
  int P() const { return p[cxt-1]; }
  unsigned byte( void ) { unsigned c=cxt; cxt=1; return c; }

  void update( unsigned y ) {
    if( y ) p[cxt-1] -= (         p[cxt-1]  >> 5), cxt+=cxt+1;
    else    p[cxt-1] += ((SCALE - p[cxt-1]) >> 5), cxt+=cxt+0;
  }
};
 
template< class Predictor, class Rangecoder > 
void proc( Predictor& p, Rangecoder& rc, uint y=0 ) { p.update( rc.Process( p.P(), y ) ); }


int rcshc(unsigned char *f,  int f_len, FILET *g) { unsigned i,c;
  Predictor p; Rangecoder<0> rc; rc.Init(g);
  for( i=0; i<f_len; i++ ) {
    c = f[i]; /*getc(f);*/ p.byte();
    proc(p,rc,c&0x80); proc(p,rc,c&0x40); proc(p,rc,c&0x20); proc(p,rc,c&0x10);
    proc(p,rc,c&0x08); proc(p,rc,c&0x04); proc(p,rc,c&0x02); proc(p,rc,c&0x01);
  } rc.Quit(); return rc.F() - g;
}

int rcshd(FILET *f, unsigned char *g, int f_len) { unsigned i;
  Predictor p; Rangecoder<1> rc; rc.Init(f);
  for( i=0; i<f_len; i++ ) {
    proc(p,rc); proc(p,rc); proc(p,rc); proc(p,rc); proc(p,rc); proc(p,rc); proc(p,rc); proc(p,rc);
    g[i] = p.byte(); //putc( p.byte(), g );
  } return f_len;
}

#if 0
unsigned flen( FILE* f ) {
  fseek( f, 0, SEEK_END );
  unsigned len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}

int main( int argc, char** argv ) {
  unsigned i,c,f_len = 0;
  if( argc<4 ) return 1;
  FILE* f = fopen( argv[2], "rb" ); if( f==0 ) return 2;
  FILE* g = fopen( argv[3], "wb" ); if( g==0 ) return 3;
  Predictor p;
  if( argv[1][0]=='c' ) {
    f_len = flen( f );
    fwrite( &f_len, 1,4, g );
    Rangecoder<0> rc; rc.Init(g);
    for( i=0; i<f_len; i++ ) {
      c = getc(f); p.byte();
      proc(p,rc,c&0x80); proc(p,rc,c&0x40); proc(p,rc,c&0x20); proc(p,rc,c&0x10);
      proc(p,rc,c&0x08); proc(p,rc,c&0x04); proc(p,rc,c&0x02); proc(p,rc,c&0x01);
    } rc.Quit();
  } else {
    fread( &f_len, 1,4, f );
    Rangecoder<1> rc; rc.Init(f);
    for( i=0; i<f_len; i++ ) {
      proc(p,rc); proc(p,rc); proc(p,rc); proc(p,rc); proc(p,rc); proc(p,rc); proc(p,rc); proc(p,rc);
      putc( p.byte(), g );
    }
  }
  fclose( f );
  fclose( g );
  return 0;
}
#endif
