#include "lzoma-0.2.c"
#include "lzoma.h"

int lzomapack(unsigned char *in, int inlen, unsigned char *out, int lev) { 
  char C[2],*c=C; C[0]=lev+48; C[1]=0;
 
		int A[]={15,2,999},i=0,a=0,l,n=0,bs;
		for(l=sizeof(A)/sizeof(A[0]);*++c&&i<l;)
		  if(*c<58&&*c>47){if(a<9)n=n*10+*c-48,a++;}
		  else{ if(a) A[i]=n;i++;a=n=0;}
		if(a&&i<l)A[i]=n;

		bs=(A[0]&511)+1<<20; if(bs>inlen)bs=inlen;
        //int bs = inlen+(1<<24);
        if(bs<65536)bs=65536;
		l = sizeof(int);
		T=malloc(bs*(l*5+1));     if(!T) return 0;
		same=malloc(bs*l*9+l*2);  if(!same) return free(T),0;

		O=T+bs, SA=(int*)O, ISA=SA+bs, LCP=ISA+bs, Prev=LCP+bs, Next=Prev+bs;
		same3=same+bs, samelen=same3+bs, useOlz=samelen+bs, useOlz2=useOlz+bs, olzLen=useOlz2+bs, olzLen2=olzLen+bs, RLE=olzLen2+bs, cacheRLE=RLE+bs+1;
  memcpy(T, in, inlen);
  unsigned outlen = pack(inlen); 
  memcpy(out, O, outlen);
  free(T);
  free(same);
  return outlen;
}

int lzomaunpack( unsigned char *in, int inlen, unsigned char *out, int outlen) {
  if(!(T=malloc(outlen*2+9))) return 0;
  O=T+outlen+9;
  memcpy(T,in,inlen);
  unpack(T, O, outlen);
  memcpy(out,O, outlen);
  free(T);
  return outlen;
}

