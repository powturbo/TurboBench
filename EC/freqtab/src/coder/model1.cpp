
#define INC_LOG2I
#include "common.inc"

//#include "coro3b.inc"

struct Coroutine {
  byte* __restrict inpptr;
  byte* __restrict inpbeg;
  byte* __restrict inpend;
  byte* __restrict outptr;
  byte* __restrict outbeg;
  byte* __restrict outend;
  uint f_quit;

  template <typename T> 
  uint coro_call( T* that ) {
    that->do_process();
    return 0;
  }

  void chkinp( void ) { }

  void chkout( uint d=0 ) {}

  uint get( void ) { return *inpptr++; }
  void put( uint c ) { *outptr++ = c; }
  byte get0( void ) { return *inpptr++; }
  void put0( uint c ) { *outptr++ = c; }

  void coro_init( void ) {
    inpptr=inpbeg=inpend = 0;
    outptr=outbeg=outend = 0;
    f_quit=0;
  }

  uint getinplen() { return inpend-inpptr; } 
  uint getoutlen() { return outend-outptr; } 

  uint getinpleft() { return inpend-inpptr; }
  uint getinpsize() { return inpptr-inpbeg; }
  uint getoutleft() { return outend-outptr; }
  uint getoutsize() { return outptr-outbeg; }

  void addinp( byte* inp,uint inplen ) {
    inpbeg = inpptr = inp;
    inpend = &inp[inplen];
  }

  void addout( byte* out,uint outlen ) {
    outbeg = outptr = out;
    outend = &out[outlen];
  }

  void yield( void* p, int value ) {}

};

#include "model.inc"

struct ModelWrap {

  static const uint model_size = macro_Max( sizeof(Model<0>), sizeof(Model<1>) );

  byte buf[ model_size ];

  void init( void ) {
    ((Coroutine*)buf)->coro_init();
  }

  uint get_object_size( void ) {
    return model_size;
  }

  uint process( uint f_DEC ) {
    if( f_DEC==0 ) {
      auto Q = ((Model<0>*)buf);
      return Q->coro_call(Q);
    } else {
      auto Q = ((Model<1>*)buf);
      return Q->coro_call(Q);
    }
  }

  uint getoutsize() { 
    Coroutine& Q = (*(Coroutine*)buf);
    return Q.outptr-Q.outbeg; 
  }

  void addinp( byte* __restrict inp,uint inplen ) { 
    Coroutine& Q = (*(Coroutine*)buf);
    Q.addinp(inp,inplen); 
    if_e0( inplen==0 ) Q.f_quit=1;
  }

  void addout( byte* __restrict out,uint outlen ) { 
    Coroutine& Q = (*(Coroutine*)buf);
    Q.addout(out,outlen); 
  }

#if 0
  void set_fquit( void ) {
    Coroutine& Q = (*(Coroutine*)buf);
    Q.f_quit=1;
  }
#endif

};

uint C_get_object_size( void ) { return ((ModelWrap*)0)->get_object_size(); }
void C_init( void* p ) { ((ModelWrap*)p)->init(); }
uint C_process( void* p, uint f_DEC ) { return ((ModelWrap*)p)->process(f_DEC); }
uint C_getoutsize( void* p ) { return ((ModelWrap*)p)->getoutsize(); }
void C_addinp( void* p, byte* inp,uint inplen ) { ((ModelWrap*)p)->addinp(inp,inplen); }
void C_addout( void* p, byte* out,uint outlen ) { ((ModelWrap*)p)->addout(out,outlen); }
