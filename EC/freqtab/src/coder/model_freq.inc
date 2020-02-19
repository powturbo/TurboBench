
template <int __len__, int _rlen>
struct NumberBits {
  enum{ rlen=_rlen };
  Counter P[1<<rlen];
  void Init( void ) {
    uint i;
    for( i=0; i<DIM(P); i++ ) P[i].P=hSCALE;
  }
};

template< class NumBits >
INLINE
uint numbits_proc( NumBits& X, uint n, uint clen, int lo, int hi, const int wr, const int mw ) {
  int i,j,k,z=0;
  const uint lim = 1<<X.rlen;
  for( i=clen-1; i>=0; i-- ) {
    j = ((1<<clen)+z)>>(i+1);
    k = (n>>i)&1;
    if( z+(1<<i)-1<=lo ) k=1; else
    if( z+(1<<i)  >=hi ) k=0; else {
      k = rc_BProcess( (j<lim?X.P[j].P:hSCALE), k );
      if( j<lim ) X.P[j].Update( k, wr, mw );
    }
    z |= (k<<i);
  }
  return z; // n = z;
}

template< class NumBits >
INLINE
uint xnumbits_proc( NumBits& X, uint n, int lo, int hi, const int wr, const int mw ) {
  uint l = log2i(hi-1);
  uint a = numbits_proc(X,n, l+1,lo,hi,wr,mw);
  return a;
}

#if 0
uint number_proc( uint x, uint max ) {
  uint i;
  for( i=0; i<sizeof(x); i++ ) ((byte*)&x)[i]=procbyte(((byte*)&x)[i]);
  return x;
}
#endif

NumberBits<max_quants_log,5> nsum;
Counter summin, summax;

void freq_proc_Init( void ) {
  summin.P = ptrim(M_smP0);
  summax.P = ptrim(M_sxP0);
  nsum.Init();
}

// uint freq[A..B-1] to encode
template< typename T >
uint freq_proc( T* __restrict freq, uint A, uint B, uint total, uint sum=0 ) { 
  if( B-A==0 ) return 0;

  uint i,flag;

  if( f_DEC==0 ) for( i=A,sum=0; i<B; i++ ) sum+=freq[i];

//if( total<sum ) printf( "A=%i B=%i total=%i sum=%i level=%i\n", A,B,total,sum,level );

  if( total>=sum ) 
  do {

    flag = rc_BProcess( summax.P, sum==total ); // 0=EOF
    summax.Update( flag, M_sxwr, M_sxmw );
    if( flag ) { sum=total; break; }

    flag = rc_BProcess( summin.P, sum==0 ); // all zeroes
    summin.Update( flag, M_smwr, M_smmw );
    if( flag ) { sum=0; break; } 

    sum = xnumbits_proc( nsum,sum, 0,total, M_n0wr, M_n0mw );

  } while(0);

  if( B-A<=1 ) {

    freq[A+0] = sum;

  } else {

    if( sum==0 ) {

      if( f_DEC==1 ) for( i=A; i<B; i++ ) freq[i]=0;

    } else {

      uint h = (B-A)>>1;
      uint sum0 = 
      freq_proc( freq,A+0,A+h,   sum,0        );
      freq_proc( freq,A+h,B+0,   0,  sum-sum0 );

    } // if sum=0

  }

  return sum;
}



