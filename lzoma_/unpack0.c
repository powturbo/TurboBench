// test file decompression using LZOMA algoritm
// (c) Alexandr Efimov, 2015
// License: GPL v2 or later

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <x86intrin.h>

#define O_BINARY 0
#define byte unsigned char
#include "../lzoma/lzoma.h"									//TurboBench
#include "lzoma.h"

byte in_buf[MAX_SIZE]; /* text to be encoded */
byte out_buf[MAX_SIZE];

//#define getbit (((bits=bits&0x7f? bits+bits :  (((unsigned)(*src++))<<1)+1)>>8)&1)
#define getbit ((bits=bits&0x7fffffff? (resbits=bits,bits+bits) :  (src+=4,resbits=*((uint32_t *)(src-4)),(resbits<<1)+1)),resbits>>31)
#define loadbit

#define getcode(bits, src, ptotal) {\
  int total = (ptotal);\
  ofs=0;\
  long int res=0;\
  int x=1;\
  int top=0;\
\
  /*if (total > 256) {*/\
     top=lzlow(total);\
     x=256;\
     res=*src++;\
  /*}*/\
\
  while (1) {\
    x+=x;\
    if (x>=total+top) break;\
    if (x & lzmagic)\
      top=lzshift(top);\
    /*if (x>=512) {*/\
      if (res<top) {  goto getcode_doneit;}\
      ofs-=top;\
      total+=top;\
      top+=top;\
    /*}*/\
    loadbit;\
    res+=res+getbit;\
  }\
  x-=total;\
  if (res>=x) { \
    loadbit; res+=res+getbit;\
    res-=x;\
  }\
getcode_doneit: \
  ofs+=res;\
  /*fprintf(stderr,"ofs=%d total=%d\n",ofs,ptotal);*/\
}

#define getlen(bits, src) {\
  long int res=0;\
  int x=1;\
  \
  if (getbit==0) {\
    len+=getbit;\
    goto getlen_0bit;\
  }\
  len+=2;\
  while (1) {  \
    x+=x;\
    loadbit;\
    res+=res+getbit;\
    loadbit;\
    if (getbit==0) break;\
    len+=x;\
  }\
  len+=res;\
getlen_0bit: ;\
}

static void unpack_c(byte *src, byte *dst, int left) {
  int ofs=-1;
  int len;
  uint32_t bits=0x80000000;
  uint32_t resbits;
  left--;

copyletter:
//fprintf(stderr,"i");
  *dst++=*src++;
  //was_letter=1;
  len=-1;
  left--;

get_bit:
  if (left<0) return;
  loadbit;
  if (getbit==0) goto copyletter;

  /* unpack lz */
  if (len<0) {
//fprintf(stderr,"\nw");
    len=1;
    loadbit;
    if (!getbit) {
      goto uselastofs;
    }
  }
  len=2;
//fprintf(stderr,"\nn");
  getcode(bits,src,dst-out_buf);
  ofs++;
  if (ofs>=longlen) len++;
  if (ofs>=hugelen) len++;
  ofs=-ofs;
uselastofs:
  getlen(bits,src);
//  printf("lz: %d:%d,left=%d\n",ofs,len,left);
  left-=len;
//    *dst=dst[ofs];
//    dst++;
//    --len;//len is at least 2 bytes - byte1 can be copied without checking
  //memcpy(dst,dst+ofs,len);dst+=len;
  do {
    *dst=dst[ofs];
    dst++;
    /*if (--len==0) goto get_bit;
    *dst=dst[ofs];
    dst++;
    if (--len==0) goto get_bit;
    *dst=dst[ofs];
    dst++;
    if (--len==0) goto get_bit;
    *dst=dst[ofs];
    dst++;*/
  } while(--len);
  /*if (ofs < -3)
  while(len>=4) {
    *(long*)dst=*(long*)(dst+ofs);
    dst+=4;
    len-=4;
  }
  while(len>0) {
    *dst=dst[ofs];
    dst++;
    len--;
  };*/
  goto get_bit;
}

int lzomaunpack( unsigned char *in, int inlen, unsigned char *out, int outlen) {
  unpack_c(in, out, outlen);
  return 0;
}
#if 0
extern unsigned int unpack(byte *src, byte *dst, int left);
#include "e8.h"
int main(int argc,char * argv[]) {
  int ifd,ofd;
  int n,n_unp;
  char shift;

  ifd=open(argv[1],O_RDONLY|O_BINARY);
  ofd=open(argv[2],O_WRONLY|O_TRUNC|O_CREAT|O_BINARY,511);
  while(read(ifd,&n,4)==4) {
    read(ifd,&n_unp,4);
    int use_e8=0;
    read(ifd,&use_e8,1);
    //breaklz=1<<shift;
    //breaklz = 1<<9;
    read(ifd,in_buf,n);
    //for(int t=0;t<10;t++) {
      long unsigned tsc = (long unsigned)__rdtsc();
      unpack(in_buf, out_buf, n_unp);
      tsc=(long unsigned)__rdtsc()-tsc;
      printf("tsc=%lu\n",tsc);
    //}
    if (use_e8) e8back(out_buf,n_unp);
    write(ofd,out_buf,n_unp);
  }

  close(ifd);
  close(ofd);
  return 0;
}
#endif
