
static const int SCALElog = 15;
static const int SCALE    = 1<<SCALElog;
static const int eSCALE   = 16*SCALE;
static const int hSCALE   = SCALE/2;
static const int mSCALE   = SCALE-1;

template< int f_DEC >
struct Rangecoder : Coroutine {

  enum {
    NUM   = 4,
    sTOP  = 0x01000000U,
    gTOP  = 0x00010000U,
    Thres = 0xFF000000U,
    Threg = 0x00FF0000U
  };

//  int   f_DEC; // 0=encode, 1=decode;
  union {
    struct {
      uint  low;  
      uint  Carry;
    };
    qword lowc;
    uint  code; 
  };
  uint  FFNum;
  uint  Cache;
  uint  range;

  void rc_Process( uint cumFreq, uint freq, uint totFreq ) {
    uint tmp = cumFreq*range;
    if( f_DEC ) code-=tmp; else lowc+=tmp;
    range *= freq;
    Renorm();
  }


  void rc_Arrange( uint totFreq ) {
    range /= totFreq;
  }

  uint rc_GetFreq( uint totFreq ) {
    return code/range;
  }

  void Renorm( void ) {
    if( f_DEC ) {
      while( range<sTOP ) range<<=8, (code<<=8)+=get();
//      if_e1( range<gTOP ) range<<=16, (code<<=16)+=(get()<<8)+get(); else if_e0( range<sTOP ) range<<=8, (code<<=8)+=get();
    } else {
      while( range<sTOP ) range<<=8, ShiftLow();
//      if_e1( range<gTOP ) range<<=16, ShiftLow2(); else if_e0( range<sTOP ) range<<=8, ShiftLow();
    }
  }

//  NOINLINE
  uint rc_BProcess( uint freq, uint bit ) { 

    Renorm();

    freq = ptrim(freq);

    uint r1 = range>>SCALElog;
    uint rnew = r1*freq;

    if( f_DEC ) {

      if( code<rnew ) {
        range = rnew;
        return 0;
      } else {
        range -= rnew;
        if( f_DEC ) code -= rnew; else lowc += rnew;
        return 1;
      }

    } else {

      if( bit==0 ) {
        range = rnew;
        return 0;
      } else {
        range -= rnew;
        if( f_DEC ) code -= rnew; else lowc += rnew;
        return 1;
      }

    }

  }

  void ShiftLow( void ) {
    if( low<Thres || Carry ) {
      put( Cache+Carry );
      for (;FFNum != 0;FFNum--) put( Carry-1 ); // (Carry-1)&255;
      Cache = low>>24;
      Carry = 0;
    } else FFNum++;
    low<<=8;
  }

  void ShiftLow2( void ) {
    if( low<Thres || Carry ) {
      put( Cache+Carry );
      for (;FFNum != 0;FFNum--) put( Carry-1 ); // (Carry-1)&255;
      Cache = low>>24;
      Carry = 0;
    } else FFNum++;
    low &= sTOP-1;
    if( low<Threg ) {
      put( Cache );
      for(; FFNum!=0; FFNum-- ) put( 0xFF ); // (Carry-1)&255;
      Cache = low>>16;
    } else FFNum++;
    low<<=16;
  }

  void rcInit( void ) { 
    range = 0xFFFFFFFF;
    low   = 0;
    FFNum = 0;
    Carry = 0;    
    Cache = 0;
  }
  
  void rc_Init( void ) {
    rcInit();
    if( f_DEC==1 ) {
      for(int _=0; _<NUM+1; _++) (code<<=8)+=get(); 
    }
  }

  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      for(int _=0; _<NUM+1; _++) ShiftLow(); 
    }
  }

};

