
while(1) {

  total = freq_proc( freq, 0,CNUM, BLKSIZE );
  if_e0( total==0 ) break;

  for( i=0; i<CNUM; i++ ) revmap[i]=i;
  sh_qsort< TEST<Model> >( this, 0,CNUM-1 );
  for( i=0; i<CNUM; i++ ) chrmap[revmap[i]]=i;

  while( total>0 ) {

    rc_Arrange(total);
    code = rc_GetFreq(total);

    {
      freq_t _low, _code=code;
      for( c=0,_low=0; _low+freq[c]<=_code; c++ ) _low+=freq[c];
      low = _low;
    }

    rc_Process(low,freq[c],total);

    put( revmap[c] );

    freq[c]--; total--;
  }

}
