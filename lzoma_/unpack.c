// test file decompression using LZOMA algoritm
// (c) Alexandr Efimov, 2015
// License: GPL v2 or later

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
//#include <x86intrin.h>

#ifndef O_BINARY
  #ifdef _O_BINARY
    #define O_BINARY _O_BINARY
  #else
    #define O_BINARY 0
  #endif
#endif

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

static void unpack_c(int history_size, byte *src, byte *dst, byte *start, int left) {
  int ofs=-1;
  int len;
  uint32_t bits=0x80000000;
  uint32_t resbits;
  left--;

  if (history_size) {
    history_size-=dst-start;
    goto nextblock;
  }

copyletter:
  *dst++=*src++;
  left--;
nextblock:
  len=-1;

get_bit:
  if (left<0) return;
  loadbit;
  if (getbit==0) goto copyletter;

  /* unpack lz */
  if (len<0) {
    len=1;
    loadbit;
    if (!getbit) {
      goto uselastofs;
    }
  }
  len=2;
  getcode(bits,src,dst-start+history_size);
  ofs++;
  if (ofs>=longlen) len++;
  if (ofs>=hugelen) len++;
  ofs=-ofs;
uselastofs:
  getlen(bits,src);
  left-=len;

  // Note: on some platforms memcpy may be faster here
  int ptr = dst-start+ofs;
  do {
    *dst=start[ptr&(MAX_SIZE-1)];
    ptr++;
    dst++;
  } while(--len);
  goto get_bit;
}

#ifdef ASM_X86
extern unsigned int unpack_x86(byte *src, byte *dst, int left);
#endif

//#include "../lzoma/e8.h"

int lzomaunpack( unsigned char *in, int inlen, unsigned char *out, int outlen) {
    #if 0
  int history_size = 0;
  int ofs = 0;
  int use_e8=0;
  unsigned char *ip = in;
  while(ip < in+inlen /*read(ifd,&n,4)==4*/) {
    n = *(unsigned *)ip; ip += 4;
    if (use_e8) e8(out_buf, n_unp);
    n_unp = *(unsigned *)ip; ip += 4; //  read(ifd,&n_unp,4);
    if (n != n_unp && !history_size) {
      use_e8 = *ip++; //read(ifd,&use_e8,1);
    } else
      use_e8 = 0;
    //long unsigned tsc = (long unsigned)__rdtsc();
    if (n == n_unp) {
      memcpy(op, ip, n_unp);   	//	read(ifd,out_buf,n_unp);
      op += n_unp;				//	write(ofd,out_buf+ofs,n_unp);
    } else {
      in_buf = ip;	ip += n;			//read(ifd,in_buf,n);
#ifdef ASM_X86
#error Asm version not yet updated for recent format changes. Please use C version right now.
      unpack_x86(in_buf, out_buf, n_unp);
#else
      unpack_c(history_size, in_buf, out_buf+ofs, out_buf, n_unp);
#endif
      //tsc=(long unsigned)__rdtsc()-tsc;
      //printf("tsc=%lu\n",tsc);
      if (use_e8) e8back(out_buf,n_unp);
      write(ofd,out_buf+ofs,n_unp);
    }
    ofs+=n_unp;
    ofs &= (MAX_SIZE-1);
    history_size = MAX_SIZE-NEXT_SIZE;
  }
  //if(*in++) e8(out_buf,n_unp);
  if(inlen == outlen) {
    memcpy(out, inlen, in);
    return outlen;
  }
  unpack_c(0, in, out, outlen);
    #endif
  return 0;
}

#if 0
int main(int argc,char * argv[]) {
  int ifd,ofd;
  int n,n_unp;
  char shift;

  ifd=open(argv[1],O_RDONLY|O_BINARY);
  ofd=open(argv[2],O_WRONLY|O_TRUNC|O_CREAT|O_BINARY,511);
  int history_size = 0;
  int ofs = 0;
  int use_e8=0;
  while(read(ifd,&n,4)==4) {
    if (use_e8) e8(out_buf,n_unp);
    read(ifd,&n_unp,4);
    if (n != n_unp && !history_size) 
      read(ifd,&use_e8,1);
    else
      use_e8 = 0;
    //long unsigned tsc = (long unsigned)__rdtsc();
    if (n == n_unp) {
      read(ifd,out_buf,n_unp);
      write(ofd,out_buf+ofs,n_unp);
    } else {
      read(ifd,in_buf,n);
#ifdef ASM_X86
#error Asm version not yet updated for recent format changes. Please use C version right now.
      unpack_x86(in_buf, out_buf, n_unp);
#else
      unpack_c(history_size, in_buf, out_buf+ofs, out_buf, n_unp);
#endif
      //tsc=(long unsigned)__rdtsc()-tsc;
      //printf("tsc=%lu\n",tsc);
      if (use_e8) e8back(out_buf,n_unp);
      write(ofd,out_buf+ofs,n_unp);
    }
    ofs+=n_unp;
    ofs &= (MAX_SIZE-1);
    history_size = MAX_SIZE-NEXT_SIZE;
  }

  close(ifd);
  close(ofd);
  return 0;
}
#endif
