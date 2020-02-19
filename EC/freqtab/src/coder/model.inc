
static const uint CNUM = 256; // alphabet size
static const uint BLKSIZE = (1<<14); // block size (freqtable is encoded per block)
static const uint max_quants_log = 256; // unused atm

#include "sh_v1x.inc"

uint ptrim( uint p ) {
  if_e0( p<1 ) p=1;
  if_e0( p>mSCALE ) p=mSCALE;
  return p;
}

//#include "sh_mapping.inc"
#include "./sh_model_h.inc"

#include "sh_qsort.inc"

template< class Base >
struct TEST {
  // Returns neg if 1<2, 0 if 1=2, pos if 1>2.
  static int  c( Base* __restrict A, int x, int y ) {
    uint a = A->freq[x];
    uint b = A->freq[y];
    return (a==b)?0:(a<b)?1:-1;
//    return wcscmp( a->name, b->name );
  }
  static void s( Base* __restrict A, int x, int y ) {
    SWAP( A->freq[x], A->freq[y] );
    SWAP( A->revmap[x], A->revmap[y] );
  }
};


template< uint f_DEC >
struct Model: Rangecoder<f_DEC> {

  typedef word freq_t;

  ALIGN(64) freq_t freq[CNUM];
  ALIGN(64) freq_t NL[256];
  ALIGN(64) freq_t NH[16];
  ALIGN(64) freq_t IH[16];
  ALIGN(64) freq_t II[16];

  byte chrmap[CNUM];
  byte revmap[CNUM];

  uint total;

#include "counter.inc"
#include "model_freq.inc"

  void do_process( void ) {

    uint i,j,c,code,low,nh,nhm;

    for( i=0; i<16; i++ ) II[i]=i;

    freq_proc_Init();
    rc_Init();

    if( f_DEC==0 ) {
      #include "model_enc.inc"
    } else {
      #include "model_dec.inc"
    }

    rc_Quit();

    yield(this,0);
  }

};
