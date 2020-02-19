
while(1) {

  chkinp(); total=Min<uint>(BLKSIZE,getinplen());
  byte* __restrict p = this->inpptr;

//printf( "!total=%i p=%I64X w.l=%i!\n", total, p, outptr-outbeg );

  for( i=0; i<CNUM; i++ ) freq[i]=0;
  for( i=0; i<total; i++ ) freq[p[i]]++;

  total = freq_proc( freq, 0,CNUM, BLKSIZE );
  if_e0( total==0 ) break;

  for( i=0; i<CNUM; i++ ) revmap[i]=i;
  sh_qsort< TEST<Model> >( this, 0,CNUM-1 );
  for( i=0; i<CNUM; i++ ) chrmap[revmap[i]]=i;

  while( total>0 ) {

    rc_Arrange(total);

    c = get0(); c=chrmap[c];

    {
      freq_t _low;
//printf( (char*)12345678 );

#undef min
#undef max
      #pragma loop_count min(0),max(CNUM),avg(8)
      for( i=0,_low=0; i<c; i++ ) _low += freq[i];

//      for( i=0,_low=0; i<CNUM; i++ ) _low += freq[i] & -freq_t( freq_t(i) < freq_t(c) );

//printf( (char*)12345678 );
      low = _low;
    }

    rc_Process(low,freq[c],total);

    freq[c]--; total--;
  }

}
