
enum {
  SCALElog = 14,
  SCALE    = 1<<SCALElog,
//  eSCALE = 16*SCALE,
  hSCALE = SCALE/2,
  mSCALE = SCALE-1
};

//template < int ProcMode >
struct Rangecoder_SH1m {

  enum {
    rcN   = 8,
    NUM   = 3,
    sTOP  = 0x01000000U,
    hTOP  = 0x01000000U>>SCALElog,
    gTOP  = 0x00010000U>>SCALElog,
    tTOP  = 0x00010000U,
    Thres = 0x00FF0000U,
    Threg = 0x0000FF00U
  };

//  int   ProcMode; // 0=encode, 1=decode;
  uint  range[rcN];
  uint  low[rcN];
  uint  Cache[rcN];
  uint  FFNum[rcN];
  uint  freq[rcN];
  uint  bitt[rcN];
  uint  ymid[rcN];

  byte* restrict qtr[rcN];
  byte* restrict ptr;

  uint  skew;

  byte get( const int i ) { return *ptr++; }
  void put( const int i, const uint c ) { *qtr[i]++ = c; }
  void queue( const int i ) { *ptr++ = i; }

  template< int DECODE >
  void Renorm( int i ) {
    if( DECODE ) {
      if( range[i]<hTOP ) 
        if( range[i]<gTOP ) 
          range[i]<<=16, low[i]=low[i]*65536+get(i)*256+get(i); 
        else range[i]<<=8, low[i]=low[i]*256+get(i);
    } else {
      if( range[i]<hTOP ) 
        if( range[i]<gTOP ) 
          range[i]<<=16, ShiftLow2(i), queue(i), queue(i); 
        else range[i]<<=8, ShiftLow(i), queue(i);
    }
  }

  NOINLINE
  void BatchRenorm0( void ) {
    int x;
    uint* restrict i = ymid;
    uint* restrict y = bitt;
    uint* restrict l = low;
    uint* restrict r = range;
    uint* restrict f = freq;
    #pragma vector aligned
    for( x=0; x<rcN; x++ ) {
      i[x] = (r[x]*f[x])>>SCALElog;
//      r[x] = r[x]-i[x] + ((i[x]*2-r[x])&(-y[x]));
//      l[x] = l[x] + (i[x] & (~(-y[x])));
      r[x] = i[x] + ((r[x]-i[x]*2) & (~(-y[x])));
      l[x] = l[x] + i[x] - (i[x] & (-y[x]));
    }
    for( x=0; x<rcN; x++ ) Renorm<0>(x);

    skew = (skew+1)&(rcN-1);
  }

//  NOINLINE
  void BatchRenorm1( void ) {
    int x;
    uint* restrict i = ymid;
    uint* restrict y = bitt;
    uint* restrict l = low;
    uint* restrict r = range;
//    uint* restrict f = freq;
    #pragma vector aligned
    for( x=0; x<rcN; x++ ) {
//      r[x] = r[x]-i[x] + ((i[x]*2-r[x]) & (-y[x]));
      r[x] = i[x] + ((r[x]-i[x]*2) & (~(-y[x])));
      l[x] = l[x] - (i[x] & (~(-y[x])));
    }
    for( x=0; x<rcN; x++ ) Renorm<1>(x);

    skew = (skew+1)&(rcN-1);
  }

  uint process_bit( const uint y, const uint prob, const uint i ) {
    if( ProcMode ) {
      uint rnew = (range[i]*prob)>>SCALElog;
      uint y = low[i]<rnew;
      bitt[i] = y;
      ymid[i] = rnew;
      return y;
    } else {
      freq[i] = prob;
      bitt[i] = y;
      return 0;
    }
  }

//  NOINLINE
  void storeFF( int i ) {
    int N = FFNum[i];
    FFNum[i] = 0;
    while( --N>=0 ) put( i, 0xFF );
  }

//  NOINLINE
  void store00( int i ) {
    int N = FFNum[i];
    FFNum[i] = 0;
    while( --N>=0 ) put( i, 0x00 );
  }

  void ShiftLow( int i ) {
    if( uint(low[i]-Thres)>=tTOP ) {
      if( low[i]>=sTOP ) {
        put( i, Cache[i]+1 );
        if( FFNum[i] ) store00(i);
      } else {
        put( i, Cache[i] );
        if( FFNum[i] ) storeFF(i);
      }
      Cache[i] = low[i]>>16;
      FFNum[i] = -1;
    }
    FFNum[i]++;
    low[i] = word(low[i]);
    low[i] <<= 8;
  }

  void ShiftLow2( int i ) {
    if( uint(low[i]-Thres)>=tTOP ) {
      if( low[i]>=sTOP ) {
        put( i, Cache[i]+1 );
        if( FFNum[i] ) store00(i);
      } else {
        put( i, Cache[i] );
        if( FFNum[i] ) storeFF(i);
      }
      Cache[i] = low[i]>>16;
      FFNum[i] = -1;
    }
    FFNum[i]++;
    low[i] = word(low[i]);
    if( low[i]<Threg ) {
      put( i, Cache[i] );
      if( FFNum[i] ) storeFF(i);
      Cache[i] = low[i]>>8;
      FFNum[i] = -1;
    }
    FFNum[i]++;
    low[i] = byte(low[i]);
    low[i]<<=16;
  }

  void rcInit( int i ) { 
    range[i] = 0xFFFFFFFFU>>SCALElog;
    low[i]   = 0;
    FFNum[i] = 0;
    Cache[i] = 0;
  }
  
  void Init( int i ) {
    rcInit(i);
    if( ProcMode==1 ) {
      for(int _=0; _<NUM+1; _++) (low[i]<<=8)+=get(i); 
    } else {
      for(int _=0; _<NUM+1; _++) queue(i); 
    }
  }

  void Quit( int i ) {
    if( ProcMode==0 ) {
      for(int _=0; _<NUM+1; _++) ShiftLow(i); //ptr-=NUM+1; // no reserve
    }
  }

};

