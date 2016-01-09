
#define PSCALE (1<<14)

struct Model {

  struct Counter {

    word p;
    void Update( uint bit ) {
      if( bit ) {
        p += uint(PSCALE-p)>>5;
      } else {
        p -= uint(p)>>5;
      }
    }

  };
  byte dum[1<<14];

  #include "sh_v1m.h"

  Rangecoder_SH1m rc;

  #include "model_byte.h"

  enum {
    inpbufsize = 1<<22,
    inppadsize = 1<<12,
    outbufsize = 1<<13
  };

  ALIGN(4096) Counter o0[256];
  ALIGN(4096) byte inpbuf[inpbufsize];
  ALIGN(4096) byte outbuf[outbufsize*2];
  ALIGN(4096) byte altbuf[8][outbufsize+256];

  NOINLINE
  unsigned char *rc_out_flush( unsigned char *out, uint size ) {
    int i;
    // rc output pointers for interleaving
    byte* restrict xtr[rc.rcN];
    for( i=0; i<rc.rcN; i++ ) xtr[i]=&altbuf[i][0];
    // interleave the outputs
    for( i=0; i<size; i++ ) {
      uint x = outbuf[i];
      byte* p = xtr[x];
      outbuf[i] = p[0];
      xtr[x] = p+1;
    }
    // shift the remaining tails
    for( i=0; i<rc.rcN; i++ ) {
      memcpy( altbuf[i], xtr[i], rc.qtr[i]-xtr[i] );
      rc.qtr[i] = &altbuf[i][rc.qtr[i]-xtr[i]];
    }
    // store
    //l = fwrite( outbuf, 1,size, g ); 
    memcpy(out, outbuf, size); out+=size;
    // shift the rc index buffer
    memcpy( outbuf, &outbuf[size], rc.ptr-(&outbuf[size]) );
    rc.ptr -= size;
    return out;
  }

  NOINLINE
  unsigned char *rc_inp_flush( unsigned char *in, uint size ) {
    int l = inpbufsize - (rc.ptr-inpbuf);
    if( l<=inppadsize ) {
      memcpy( inpbuf, rc.ptr, l );
    } else l=0;
    //int r = fread( &inpbuf[l], 1,inpbufsize-l, f );
    memcpy( &inpbuf[l], in, inpbufsize-l ); in += inpbufsize-l;
    rc.ptr = inpbuf;
    return in;
  }

//  void process( FILE* f, FILE* g, int n) 
  unsigned char *process( unsigned char *in, int n, unsigned char *out) 
  {
    int x,z;

    Init_Model();

    rc.ptr = ProcMode ? inpbuf+inpbufsize : outbuf;
    if( ProcMode==1 ) in = rc_inp_flush( in, inpbufsize );
    for( x=0; x<rc.rcN; x++ ) { rc.Init(x); rc.qtr[x]=&altbuf[x][0]; }
    rc.skew=0;
    while( n ) {
      int j = ProcMode ? outbufsize : inpbufsize; if( j>n ) j=n;
      n -= j;
      if( ProcMode==0 ) {
        //fread( inpbuf, j, 1, f );
        memcpy(inpbuf, in, j); in += j;  byte * restrict bi = inpbuf;      //byte * restrict bi = in; in += j;
        while( --j>=0 ) {
          process_byte( *bi++ ); 
          rc.BatchRenorm0();
          if(rc.ptr-outbuf>=outbufsize+256) out = rc_out_flush(out,outbufsize);
        }
      } else {
        byte * restrict bi = outbuf;     //byte * restrict bi = out;
        while( --j>=0 ) {
          *bi++ = process_byte( 0 );
          rc.BatchRenorm1();
          if( rc.ptr-inpbuf>=inpbufsize-inppadsize ) in = rc_inp_flush( in, inpbufsize );
        }       
        memcpy(out, outbuf, bi-outbuf); out += bi-outbuf;        //out = bi;	//fwrite( outbuf, bi-outbuf, 1, f );
      }
    }
    for( x=0; x<rc.rcN; x++ ) rc.Quit(x);
    if( ProcMode==0 ) out = rc_out_flush(out,rc.ptr-outbuf);
    return ProcMode ? in:out;
  }
};

unsigned char * PROC( unsigned char *in, int n, unsigned char *out) {
  Model M;// = new Model;
  /*byte* M0 = 0;
  byte* _M1 = new byte[sizeof(Model)+4096-1];
  byte* _M = (byte*)( ((_M1-M0)|4095)+1 );

  Model* M = (Model*)_M;*/

  unsigned char *p = M.process( in, n, out );
  //delete[] M;
  return p;
}
