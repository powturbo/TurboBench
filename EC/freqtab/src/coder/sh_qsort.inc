
enum {
  CUTOFF=8,
  STKSIZ=(8*sizeof(void*)-2)
};

template <class T>
  void SWAP( T& t1, T& t2 ) { T tmp=t1; t1=t2; t2=tmp; }

template <class CMP, class Element>
void sh_qsort( Element* array, uint lo, uint hi ) {
  // Note: the number of stack entries required is no more than
  //   1 + log2(num), so 30 is sufficient for any array
  uint lostk[STKSIZ];
  uint histk[STKSIZ];
  uint mid;                  /* points to middle of subarray */
  uint loguy;
  uint higuy;        /* traveling pointers for partition step */
  int  size;                /* size of the sub-array */
  int  stkptr;                 /* stack for saving sub-array to be processed */

  if( hi-lo+1<2 ) return;

  stkptr = 0;

recurse:

  size = hi - lo + 1;   

  if( size<=CUTOFF ) {
    uint p, max;
    while( hi>lo ) {
      max = lo;
      for( p=lo+1; p<=hi; p++ ) if( CMP::c(array,p,max)>0 ) max = p;
      CMP::s(array, max, hi );
      hi--;
    }
  } else {
    mid = lo + (size>>1);
    if( CMP::c(array,lo, mid)>0 ) CMP::s(array,lo,mid);
    if( CMP::c(array,lo, hi)>0  ) CMP::s(array,lo,hi);
    if( CMP::c(array,mid, hi)>0 ) CMP::s(array,mid,hi);
    loguy = lo; higuy = hi;
    while(1) {
      if( mid>loguy )  do loguy++; while( loguy<mid && CMP::c(array,loguy,mid)<=0 );
      if( mid<=loguy ) do loguy++; while( loguy<=hi && CMP::c(array,loguy,mid)<=0 );
      do higuy--; while( higuy>mid && CMP::c(array,higuy,mid)>0 );
      if( higuy<loguy ) break;
      CMP::s(array,loguy,higuy);
      if( mid==higuy ) mid = loguy;
    }

    higuy++;
    if( mid<higuy )  do higuy--; while( higuy>mid && CMP::c(array,higuy,mid)==0 );
    if( mid>=higuy ) do higuy--; while( higuy>lo  && CMP::c(array,higuy,mid)==0 );

    if( higuy-lo >= hi-loguy ) {
      if( lo < higuy ) { lostk[stkptr]=lo; histk[stkptr]=higuy; ++stkptr; }
      if( loguy < hi ) { lo = loguy; goto recurse; }
    } else {
      if( loguy < hi ) { lostk[stkptr]=loguy; histk[stkptr]=hi; ++stkptr; }
      if( lo < higuy ) { hi = higuy; goto recurse; }
    }
  }

  if( --stkptr>=0 ) { lo = lostk[stkptr]; hi = histk[stkptr]; goto recurse; }
}

/*
struct TEST {
  // Returns neg if 1<2, 0 if 1=2, pos if 1>2.
  static int  c( pdirdata* restrict A, int x, int y ) {
    pdirdata a = A[x];
    pdirdata b = A[y];
//    return (a==b)?0:(a<b)?-1:1;
    return wcscmp( a->name, b->name );
  }
  static void s( pdirdata* restrict A, int x, int y ) {
    SWAP( A[x], A[y] );
  }
};
*/

