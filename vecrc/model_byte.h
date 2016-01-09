
void Init_Model( void ) {
  int i;
  for( i=0; i<256; i++ ) o0[i].p=PSCALE/2;
}

int process_byte( int c ) {

  int x; 
  uint freq,bit;
  uint cumbits = 1;

  for( x=7; x>=0; x-- ) {

    freq = o0[cumbits-1].p;

    if( ProcMode ) {
      bit = rc.process_bit( 0, freq, x^rc.skew );
    } else {
      bit = (c>>x)&1;
      rc.process_bit( bit, freq, x^rc.skew );
    }

    o0[cumbits-1].Update( bit );
    (cumbits<<=1) |= bit;
  }

  return (byte)cumbits;
}
