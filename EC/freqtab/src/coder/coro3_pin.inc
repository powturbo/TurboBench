
#include "coro3_pin_0.inc"

struct coro3_pin: coro3_pin_0 {
  typedef Coroutine wrap;

  void pin_init( wrap* that, uint _r_code ) {
    ptr=beg=end=0; f_EOF=0;
    base_offs = ((char*)this) - ((char*)that);
    r_code = _r_code;
  }

  void yield_r( void ) {
    wrap& W = *(wrap*)(((char*)this) - base_offs );
    yield( (void*)&W, r_code );
  }

  inline uint f_quit( void );

#define coro3_pin_DEFINE_f_quit                     \
  inline uint coro3_pin::f_quit( void ) {           \
    wrap& W = *(wrap*)(((char*)this) - base_offs ); \
    return f_EOF | W.f_quit;                        \
  }                                                 

  void chkinp( void ) { if_e0( ptr>=end ) yield_r(); }

  void chkout( uint d=0 ) { if_e0( ptr>=end-d ) yield_r(); }

  byte get0( void ) { return *ptr++; }
  void put0( uint c ) { *ptr++ = c; }

  uint get( void ) { 
    m0:
    if_e0( ptr>=end ) {
      if_e0( f_quit() ) return uint(-1);
      yield_r();
      goto m0;
    }
    return *ptr++; 
  }

  void put( uint c ) { 
    *ptr++ = c; chkout(); 
  }

};
