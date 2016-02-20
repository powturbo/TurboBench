// test file compression based on lzoma algorith
// (c) 2015,2016 Alexandr Efimov
// License: GPL 2.0 or later
// Uses divsufsort library for faster initialization (thanks to xezz for suggestion), see divsufsort.h for its license details.
//
// Discussion thread: http://encode.ru/threads/2280-LZOMA
//
// Notes:
//
// Pros:
// Compression ratio is very good (much higher than lzo, ucl, gzip).
// Decompression speed is very high (faster than gzip, much faster than bzip2,lzham, lzma,xz)
// tiny decompressor code (asm version of decompress function less than 400 bytes)
//
// compressed data format is somewhere between lzo and lzma
// uses static encoding and byte-aligned literals and byte-aligned parts of match offset for decompression speed
//
// Cons:
// compressor is VERY slow. It is possible to implement faster compressor at the cost of some compression ratio.
// may be it is possible to adapt lzma compressor code.
//
// Other:
// Code of both compression/decompression utils is experimental.
// compressed data format is not stable yet.
// compressor source code is more like a ground for experiments, not a finished product yet.
// some commented out code was intended for experiments with Reduced-offset LZ, RLE step before LZ, LZX-style encoding of matches, various heuristics, etc.
//
#include <stdio.h>
#include <stdlib.h>
#include"divsufsort.h"

#include "../lzoma/lzoma.h"
#include "../lzoma/bpe.h"
//#include "e8.h"

#define MINOLEN 1
#define MINLZ 2

int level, short_match_level, match_level;
  int levels[9][3]= {
    {1,1,2},
    {1,2,3},
    {2,3,5},
    {3,5,15},
    {3,7,30},
    {3,10,100},
    {3,20,200}, // default level -7
    {3,40,500},
    {3,100,1000}
  };
int verbose = 0;

int dict_size, history_size, block_size;

FILE *flzlit=NULL;
FILE *flit=NULL;
FILE *folz=NULL;
FILE *flen=NULL;
FILE *fdist=NULL;
#ifdef EXPERIMENTS
// this is for some experimention only.
FILE *test=NULL;
FILE *test2=NULL;
FILE *test3=NULL;
#endif

uint32_t *rle;
uint8_t *in_buf; /* text to be encoded */
//uint8_t *out_buf; - shared with rle
#define out_buf ((uint8_t *)rle)

typedef struct {
  int32_t cache; /* best possible result in bits if we start with lz or letter code */
  int32_t best_ofs; /* best way to start, assuming we do not start with OLD OFFSET code */
  int32_t best_len; /* best way to start - match len, assuming we do not start with OLD OFFSET code */
  int32_t use_olz; /* if not zero, repeat same offset after this number of literals */
  int32_t olz_len; /* length of repeated lz */
  int32_t use_olz2; /* if not zero, repeat same offset after this number of literals after first repeat */
  int32_t olz_len2; /* length of repeated lz */
} FutureState;

typedef struct {
  int32_t same; /* pointer to previous match of at least 2 bytes. for checking nearby short matches */
  int32_t samelen; /* length of match between this and previous string pointed by "same" */

  // sorted tree in order to quickly check long matches starting from the longest match
  int32_t sorted_len;
  int32_t sorted_prev;
  int32_t sorted_next;
} PastState;

// PastState and FutureState share the same memory buffer
// sizeof(PastState) should be < sizeof(FutureState)
void *state;
void *past_state;

#define sorted ((int32_t *)((uint8_t *)state)) // used very early in initialization

#define cache(i) ((FutureState *)state)[i-in_offset].cache
#define best_ofs(i) ((FutureState *)state)[i-in_offset].best_ofs
#define best_len(i) ((FutureState *)state)[i-in_offset].best_len
#define use_olz(i) ((FutureState *)state)[i-in_offset].use_olz
#define olz_len(i) ((FutureState *)state)[i-in_offset].olz_len
#define use_olz2(i) ((FutureState *)state)[i-in_offset].use_olz2
#define olz_len2(i) ((FutureState *)state)[i-in_offset].olz_len2

#define same(i) (((PastState *)past_state)[i].same)
#define samelen(i) (((PastState *)past_state)[i].samelen)
#define sorted_len(i) (((PastState *)past_state)[i].sorted_len)
#define sorted_prev(i) (((PastState *)past_state)[i].sorted_prev)
#define sorted_next(i) (((PastState *)past_state)[i].sorted_next)

int in_offset = 0;

static inline int price_offset(int num,int total) {
  if (total<=256) return 8;//top=0;
  register int res=8;
  register int x=256;

  int top = lzlow(total);
  while (1) {
    x+=x;
    if (x>=total+top) break; /* only 1 bit to be outputted left */
      if (x & lzmagic)
        top=lzshift(top);
    //if (x>=breaklz) {
      if (num<top) { return res;}
      num+=top;
      total+=top;
      top+=top;
    //}
    res++;
  }
  if (num>=x-total) { res++;}
  return res;
}

static inline int price_replen(int num) {//num>=2
  if (num<4) return 2;// 00 01
  num-=2;
  #define REPLEN_SKEW 1
  return REPLEN_SKEW+((31-__builtin_clz(num))<<1);
}

static inline int price_len(int num) {//num>=2
  if (num<4) return 2;// 00 01 10 
  num-=2;
  #define LEN_SKEW 1
  return LEN_SKEW+((31-__builtin_clz(num))<<1);
}

int lastpos;
unsigned int bit_cnt;
int outpos;

static inline void putbit(int bit) {
  bit_cnt>>=1;
  if (bit_cnt==0) {
    lastpos=outpos;
    *(unsigned long*)(out_buf+lastpos)=0;
    outpos+=4;
    bit_cnt=0x80000000;
  }
  if (bit) *(unsigned long *)(out_buf+lastpos)|=bit_cnt;
}

int stlet=0;
int stlz=0;
int stolz=0;
int bitslzlen=0;
int bitsolzlen=0;
int bitslen=0;
int bitsdist=0;
int bitslit=0;

static inline void putenc(int num,int total, int break_at, int debug) {
  char bits[100];
  int res=0;
  int x=1;
  int obyte=0;
  if (fdist) fwrite(&num,1,4,fdist);
  obyte=1;
  bits[0]=0;
  bits[1]=0;
  bits[2]=0;
  bits[3]=0;
  bits[4]=0;
  bits[5]=0;
  bits[6]=0;
  bits[7]=0;
  //if (debug) fprintf(stderr,"ofs=%d total=%d\n",num,total);

  int top=lzlow(total);
  //if (total<=256) top=0;
  while (1) {
    x+=x;
    if (x>=512&& x>=total+top) break; /* only 1 bit to be outputted left */
      if (x & lzmagic) 
        top=lzshift(top);
    if (x>=break_at) {
      if (num<top) {  goto doneit;}
      num+=top;
      total+=top;
      top+=top;
    }
    bits[res++]=2;
  }
  x-=total;
  if (num>=x) {
    num+=x;
    bits[res++]=2;
  }

doneit: 
  for(;res<8;res++) {
    bits[res++]=2;
  }
  for(x=res-1;x>=0;x--) {
    if (bits[x]==2) {
      bits[x]=num&1;
      num>>=1;
    }
  }
  if (obyte) {
    //printf("res=%d\n", res);
    uint8_t b=0;
    for(x=0;x<8;x++) {
      if (debug) printf("%d",bits[x]);
      if (bits[x]) b|=128>>x;
    }
    if (debug) printf(" ");
    if (!debug) out_buf[outpos++]=b;
    for(;x<res;x++) {
      if (debug) printf("%d",bits[x]);
      if (!debug) putbit(bits[x]);
    }
  }
  else 
    for(x=0;x<res;x++) {
      if (debug) printf("%d",bits[x]);
      putbit(bits[x]);
  }
  bitsdist+=res;
}

static inline void putenc_l(int num, int break_at) {
  char bits[100];
  int res=0;
  int x=1;
  int obyte=0;
  if (flen) fwrite(&num,1,4,flen);

  if (num==0) {bitslen+=2; putbit(0);putbit(0);return;}
  if (num==1) {bitslen+=2; putbit(0);putbit(1);return;}
  putbit(1);num-=2;bitslen++;

  while (1) {
    x+=x;
    if (x>=break_at) {
      if (num<(x>>1)) {bits[res++]=0; break;}
      bits[res++]=1;
      num-=x>>1;
    }
    bits[res++]=2;
  }

  for(x=res-1;x>=0;x--) {
    if (bits[x]==2) {
      bits[x]=num&1;
      num>>=1;
    }
  }
  for(x=0;x<res;x++) {
    putbit(bits[x]);
  }
  bitslen+=res;
}

int old_ofs=0;
int was_letter=1;

void initout(int start) {
  outpos = 0;
  if (start==1) {
    out_buf[outpos++]=in_buf[0];
  }
  old_ofs=0;
  bit_cnt=1;
  was_letter=1;
}

static inline int min(int a,int b) {
  return a<b? a:b;
}

static inline void put_lz(int offset,int length,int used) {
#ifdef EXPERIMENTS
  uint16_t code512 = 0x100;
#endif

  if (flzlit) fprintf(flzlit,"%c",1);
  putbit(1); bitslzlen++;
  offset=-offset; /* 1.. */
  offset--; /* 0.. */
  if (was_letter) { bitsolzlen++;
    was_letter=0;
    if (old_ofs==offset) {
      stolz++;
      if (folz) fprintf(folz,"%c",0);

#ifdef EXPERIMENTS
// test combining everything into one model for simple entropy coding
      code512 |= 0x80;
      if (length-MINOLEN < 0x7F) {
        code512 |= length-MINOLEN;
      } else {
        code512 |= 0x3F;
        length-=MINOLEN+0x7F;
        fwrite(&length, 4, 1, test2);
        length+=MINOLEN+0x7F;
      }
      code512 = (code512 & 0xFF) << 8 | (code512>>8);
      fwrite(&code512, 2, 1, test);
#endif

      putbit(0);
      putenc_l(length-MINOLEN,breaklen);
      return;
    }
    if (folz) fprintf(folz,"%c",1);
    putbit(1);
  }
  length-=MINLZ;
  stlz++;
#if LZX
  int total=used;
  int h=min(used,hugelen);
  int l=min(used,longlen);
  total+=h+l;
  if (length==0) {
    putenc(offset,total,breaklz, 0);
  } else if (length==1) {
    putenc(offset+l,total,breaklz, 0);
  } else {
    putenc(offset+l+h,total,breaklz, 0);
    putenc_l(length-2,breaklen);
  }
#else
  if (offset+1>=longlen) { length--; }
  if (offset+1>=hugelen) { length--; }

#ifdef EXPERIMENTS
  if (length < 15) {
    code512 |= length;
  } else {
    code512 |= 15;
    length-=15;
    fwrite(&length, 4, 1, test2);
    length+=15;
  }
  code512 |= (offset & 0x7) << 4;
  code512 = (code512 & 0xFF) << 8 | (code512>>8);
  fwrite(&code512, 2, 1, test);
  uint tmpofs = offset >> 4;
  fwrite(&tmpofs, 4, 1, test3);
#endif
  
  putenc(offset,used,breaklz, 0);
  putenc_l(length-MINLZ+2,breaklen);
#endif

  old_ofs=offset;
}

static inline void put_letter(uint8_t b) {
#ifdef EXPERIMENTS
  uint16_t code512 = b;
  code512 = (code512 & 0xFF) << 8 | (code512>>8);
  fwrite(&code512, 2, 1, test);
#endif

  if (flzlit) fprintf(flzlit,"%c",0);
  if (flit) fprintf(flit,"%c",b);
  putbit(0); bitslzlen++;
  out_buf[outpos++]=b; bitslit+=8;
  was_letter++;
  stlet++;
}

static inline int price_lz(int offset, int length, int used) { // offset>=1, length>=2, 
                                                    // if offset=>0xD00  length>=3
  int res=1; /* 1 bit = not a letter */
#if LZX
  int l=min(used,longlen);
  int h=min(used,hugelen);
  used+=l+h;
  offset--;
  if (length==2)
    return res+price_offset(offset,used);
  if (length==3)
    return res+price_offset(offset+l,used);
  res+=price_offset(offset+l+h,used);
  res+=price_len(length-MINLZ+2-2);
#else
  if (offset>=longlen) { length--; }
  if (offset>=hugelen) { length--; }

  offset--; // 0.. 

  res+=price_offset(offset,used);
  res+=price_len(length-MINLZ+2);
#endif
  return res;
}

static inline int price_replz_minus_lz(int offset, int length, int used) { // offset>=1, length>=2, 
                                                    // if offset=>0xD00  length>=3
  int res=2 /* lzlit flag, replz flag */ +price_replen(length+2-MINOLEN);
  return res-price_lz(offset,length,used);
}

static inline int cmpstr(int src,int src2) {
  int res=0;
  int b;

  for(;;) {
    if (in_buf[src]!=in_buf[src2]) return res;
    b=rle[src2];
    if (!b) return res;
    if (b>rle[src]) {return res+rle[src];}
    res+=b;
    src+=b;
    src2+=b;
  }
  return res;
}

int cmpstrsort(int *psrc,int *psrc2) {
  int b;
  int src = *psrc;
  int src2 = *psrc2;
//  printf("%d:%d:%d\n",src,src2,left);
  do {
    if (in_buf[src]<in_buf[src2]) return -1;
    if (in_buf[src]>in_buf[src2]) return 1;
    b=rle[src2];
    if (!b) return 1; // first string is longer
    if (b>rle[src]) b=rle[src];
    if (!b) return -1; // second string is longer
    src+=b;
    src2+=b;
  } while(1);
}

void init_same(int start, int n) {
  int i;
  uint16_t bb;
  int run_len;
  int gen_same[256*256+256];

  /*
    Notes: the slowest parts here are PLCP array construction and divsufsort.
    On slower levels -7 .. -9 it does not matter.
    But on fast levels -1..-3 (that still provide good compression),
    initialization takes about 20-30% processing time.
    
    Also, it reprocesses whole history each time a new block is read, 
    which is clearly not optimal.
    
    possible optimizations:
    1. store SA for later reuse, do divsufsort for new block only, then
    merge them. not sure if it will be faster. still need to recalculate rlcp
    2. get rid of SA completely, construct suffix tree directly.
  */
  for(i=0;i<256+256*256;i++) gen_same[i] =0;	// for bucketA & bucketB
  divsufsort(in_buf,sorted,gen_same,n);
  // reuse sorted_prev for temp buffer
#define rank(i) rle[i]
  /* 
   calculate plcp in O(n) time
   see http://www.cs.ucr.edu/~stelo/cpm/cpm09/04_karkk.pdf
   http://www.mi.fu-berlin.de/wiki/pub/ABI/Sequence_analysi_2013/2004_ManziniTwo_Space_Saving_Tricks_for_Linear_Time_LCP_Array_Computation.pdf
  */
  for(i=1;i<=n-1;i++) rank(sorted[i]) = sorted[i-1];
  rank(sorted[0]) = sorted[n-1];
  
  sorted_prev(sorted[0])=-1;
  for(i=1;i<n;i++) sorted_prev(sorted[i])=sorted[i-1];

  for(i=0;i<n-1;i++) sorted_next(sorted[i])=sorted[i+1];
  sorted_next(sorted[n-1])=-1;
  
  int h=0;
  for(i=0;i<=n-1;i++) {
    int j = rank(i);
    while(i+h<=n && j+h<=n && in_buf[i+h]==in_buf[j+h]) h++;
    sorted_len(i) = h;
    if (h<=MINLZ) {
      sorted_prev(i)=-1;
      sorted_next(j)=-1;
    }
    if(h>0) h--;
  }

  rle[n] = run_len = 0;
  uint8_t b = in_buf[n-1];
  for(i=n-1;i>=0;i--) {
    if (in_buf[i]==b) 
      run_len++;
    else {
      b=in_buf[i];
      run_len = 1;
    }
    rle[i]=run_len;
  }

  bb=0;

  for(i=0;i<65536;i++) {gen_same[i]=-1; }
  for(i=0;i<n-1;i++) { 
    bb=in_buf[i]; bb<<=8; bb|=in_buf[i+1];
    same(i)=gen_same[bb]; 
    if (gen_same[bb]>=0) { samelen(i)=1+cmpstr(i+1,same(i)+1);}
    gen_same[bb]=i; 
  }
  same(i)=-1;

  in_buf[n]=0;

  if (verbose) printf("init done.\n");
}

static inline int max(int a,int b) {
  return a>b? a:b;
}

#define CHECK_REPLZ \
        int k;\
	int jjj;\
        int d=level;\
        int tmp=price_lz(used-pos,len,used);\
        int olen=0;\
        for(k=len+1;k<left-2;k++) {\
          tmp+=9;\
          if (best_ofs(used+k)==pos-used) {\
            int tmp2=tmp+price_replz_minus_lz(used-pos,best_len(used+k),used+k);\
            tmp2+=cache(used+k);\
            if (tmp2<res || (tmp2==res && my_best_ofs<pos-used)) {\
              res=tmp2;\
              my_best_ofs=pos-used;\
              my_best_len=len;\
              my_use_olz=k-len;\
              my_olz_len=best_len(used+k);\
              my_use_olz2=use_olz(used+k);\
	      my_olz_len2=olz_len2(used+k);\
            }\
          }\
          if (olen==0) {\
            olen=cmpstr(used+k,pos+k);\
            for (j=MINOLEN;j<olen;j++) {\
              int tmp2=tmp+1+1+price_replen(j+2-MINOLEN);\
              tmp2+=cache(used+k+j);\
              if (tmp2<res || (tmp2==res && my_best_ofs<pos-used)) {\
                res=tmp2;\
                my_best_ofs=pos-used;\
                my_best_len=len;\
                my_use_olz=k-len;\
                my_olz_len=j;\
                my_use_olz2=0;\
              }\
            }\
	    if (olen>=MINOLEN) {\
              int tmp2=tmp+2+price_replen(olen+2-MINOLEN);\
              tmp2+=cache(used+k+olen);\
              if (best_len(used+k+olen)==1) {\
	        int jj;\
	        for(jj=1;jj<=8;jj++) {\
                  if (best_len(used+k+olen+jj)>1) {\
                    if (best_ofs(used+k+olen+jj)==pos-used) {\
                      tmp2+=price_replz_minus_lz(used-pos,best_len(used+k+olen+jj),used+k+olen+jj);\
		      break;\
                    }\
	          }\
                      int olen2=cmpstr(used+k+olen+jj,pos+k+olen+jj);\
            for (jjj=MINOLEN;jjj<=olen2;jjj++) {\
		     /* if (olen2>=MINOLEN) {*/\
		        int tmp3=-cache(used+k+olen);\
			tmp3+=jj*9+2+price_replen(jjj+2-MINOLEN);\
			tmp3+=cache(used+k+olen+jj+jjj);\
			if (tmp3<0) { tmp3+=tmp2;\
              if (tmp3<res || (tmp3==res && my_best_ofs<pos-used)) {\
                res=tmp3;\
                my_best_ofs=pos-used;\
                my_best_len=len;\
                my_use_olz=k-len;\
                my_olz_len=olen;\
                my_use_olz2=jj;\
                my_olz_len2=jjj;\
              }\
			}\
		      }\
	            break;\
	        }\
              }\
              if (tmp2<res || (tmp2==res && my_best_ofs<pos-used)) {\
                res=tmp2;\
                my_best_ofs=pos-used;\
                my_best_len=len;\
                my_use_olz=k-len;\
                my_olz_len=olen;\
                my_use_olz2=0;\
              }\
	    }\
          } else olen--;\
          if (best_ofs(used+k)) {\
            d--; if (d==0) break;\
          }\
        }


int pack(int start, int n) {
  int res;
  int i;

  if (n<start) { return 0; }

  init_same(start,n);
  cache(n-1)=9; /* last letter cannot be packed as a lz */
  best_ofs(n-1)=0;
  best_len(n-1)=1;
  use_olz(n-1)=0;
  use_olz(n-1)=0;

  if (sorted_prev(n-1)>=0) {
    sorted_next(sorted_prev(n-1))=sorted_next(n-1);
  }
  if (sorted_next(n-1)>=0) {
    sorted_len(sorted_next(n-1)) = min(sorted_len(sorted_next(n-1)),
                                            sorted_len(n-1));
    sorted_prev(sorted_next(n-1))=sorted_prev(n-1);
  }

  for(i=n-2;i>=start;i--) {
    int used=i;
    int left=n-i;
    int res;
    int pos;
    int max_match;
    int len;
    int j;

    int my_best_ofs=0;
    int my_best_len=1;
    int my_use_olz=0;
    int my_use_olz2=0;
    int my_olz_len=0;
    int my_olz_len2=0;
    int match_check_max;
    int notskip = 1; 

    res=9+cache(used+1);
    if (best_ofs(used+1)) {
      res++;
      if (in_buf[used]==in_buf[used+1]) {
        if (in_buf[used]==in_buf[used-1]) {
          if (in_buf[used]==in_buf[used+best_ofs(used+1)]) {
            if ((best_len(used+1)>3)||(best_len(used+1)==3&&-best_ofs(used+1)<hugelen)|| (-best_ofs(used+1)<longlen)) {
              int tmp=cache(used+1)-price_lz(-best_ofs(used+1),best_len(used+1),used+1)
                  +price_lz(-best_ofs(used+1),best_len(used+1)+1,used);
              if (tmp<=res) {
                res=tmp;
                my_best_ofs=best_ofs(used+1);
                my_best_len=best_len(used+1)+1;
                my_use_olz=use_olz(used+1);
                my_olz_len=olz_len(used+1);
                my_use_olz2=use_olz2(used+1);
                my_olz_len2=olz_len2(used+1);
              }
              if (my_best_len>=5)
                notskip = 0;
            }
          }
        }
      }
    }

    int k;
    for(k=1;k<4;k++)
      if (n-i>2+k && best_ofs(used+2+k) && -best_ofs(used+2+k) < longlen && best_ofs(used+2+k)!=best_ofs(used+1+k)) {
        if (in_buf[used]==in_buf[used+best_ofs(used+2+k)]) {
          if (in_buf[used+1]==in_buf[used+1+best_ofs(used+2+k)]) {
              int tmp=cache(used+2+k)+price_replz_minus_lz(-best_ofs(used+2+k),best_len(used+2+k),used+2+k)
                  +9*k+price_lz(-best_ofs(used+2+k),2,used);
	      if (tmp<=res) {
	        res=tmp;
                my_best_ofs=best_ofs(used+2+k);
                my_best_len=2;
                my_use_olz=k;
                my_olz_len=best_len(used+2+k);
                my_use_olz2=use_olz(used+2+k);
                my_olz_len2=olz_len(used+2+k);
	      }
          }
        }
      }

    for(k=1;k<4;k++)
      if (n-i>3+k && best_ofs(used+3+k) && -best_ofs(used+3+k) < hugelen && best_ofs(used+3+k)!=best_ofs(used+2+k)) {
        if (in_buf[used]==in_buf[used+best_ofs(used+3+k)]) {
          if (in_buf[used+1]==in_buf[used+1+best_ofs(used+3+k)]) {
            if (in_buf[used+2]==in_buf[used+2+best_ofs(used+3+k)]) {
              int tmp=cache(used+3+k)+price_replz_minus_lz(-best_ofs(used+3+k),best_len(used+3+k),used+3+k)
                  +9*k+price_lz(-best_ofs(used+3+k),3,used);
	      if (tmp<=res) {
	        res=tmp;
                my_best_ofs=best_ofs(used+3+k);
                my_best_len=3;
                my_use_olz=k;
                my_olz_len=best_len(used+3+k);
                my_use_olz2=use_olz(used+3+k);
                my_olz_len2=olz_len(used+3+k);
	      }
            }
          }
        }
      }

    for(k=1;k<4;k++)
      if (n-i>4+k && best_ofs(used+4+k) && best_ofs(used+4+k)!=best_ofs(used+3+k)) {
        if (in_buf[used]==in_buf[used+best_ofs(used+4+k)]) {
          if (in_buf[used+1]==in_buf[used+1+best_ofs(used+4+k)]) {
            if (in_buf[used+2]==in_buf[used+2+best_ofs(used+4+k)]) {
              if (in_buf[used+3]==in_buf[used+3+best_ofs(used+4+k)]) {
                int tmp=cache(used+4+k)+price_replz_minus_lz(-best_ofs(used+4+k),best_len(used+4+k),used+4+k)
                  +9*k+price_lz(-best_ofs(used+4+k),4,used);
	        if (tmp<=res) {
	          res=tmp;
                  my_best_ofs=best_ofs(used+4+k);
                  my_best_len=4;
                  my_use_olz=k;
                  my_olz_len=best_len(used+4+k);
                  my_use_olz2=use_olz(used+4+k);
                  my_olz_len2=olz_len(used+4+k);
	        }
              }
            }
          }
        }
      }
    pos=same(used);
    if (pos<0) goto done;

    if (notskip) {
      len=samelen(used);
      int ll=(used-pos>=longlen)?1:0;
      if (used-pos>=hugelen) ll=2;
      if (len<left && len>=2+ll) {
        CHECK_REPLZ
      }
      for(j=MINLZ+ll;j<=len;j++) {
        int tmp=price_lz(used-pos,2-MINLZ+j,used);
        tmp+=cache(used+j);
        if (tmp<res) {
          res=tmp;
          my_best_ofs=pos-used;
          my_best_len=j;
          my_use_olz=0;
          my_olz_len=0;
        }
      }
      max_match=len;

    }
    if (max_match<MINLZ) max_match=MINLZ;
    match_check_max = short_match_level;
    if (notskip) 
      for(;;) {
        int slen=samelen(pos);
        pos=same(pos);
        int ll=(used-pos>=longlen)?1:0;
        if (used-pos>=hugelen) ll=2;
        //if (used-pos>=longlen) break;
        if (pos<0) break;
        if (len>slen) {
          len=slen;
        } else if (len==slen) {
          len+=cmpstr(used+len,pos+len);
        } 
        if (len<left && len>=2+ll) {
          CHECK_REPLZ
        }
        if (len>max_match) {
          for(j=max_match+1+ll;j<=len;j++) {
            int tmp=price_lz(used-pos,j-MINLZ+2,used);
            tmp+=cache(used+j);
            if (tmp<res) {
              res=tmp;
              my_best_ofs=pos-used;
              my_best_len=j;
              my_use_olz=0;
              my_olz_len=0;
            }
          }
	  max_match=len;
        } else { match_check_max--; if (match_check_max <= 0) break; }
        
      }
    if (!notskip) goto done;
    
    max_match=my_best_len+1;//2;
    if (max_match<MINLZ) max_match=MINLZ;
    int top=sorted_prev(used);
    int bottom=sorted_next(used);
    int len_top=top >= 0 ? sorted_len(used) : 0;
    int len_bottom=bottom >= 0 ? sorted_len(bottom) : 0;

    match_check_max = match_level;
    int my_min_ofs=used+1;
    while (top>=0 || bottom >=0) {
      match_check_max--;
      if (match_check_max<=0) goto done;
      if (len_top>len_bottom) {
        pos=top;
	len=len_top;
        len_top = min(len_top,top >= 0 ? sorted_len(top):0);
	top=sorted_prev(pos);
      } else {
        pos=bottom;
	len=len_bottom;
	bottom=sorted_next(pos);
        len_bottom = min(len_bottom,bottom >= 0 ? sorted_len(bottom):0);
      }
//      if (used-pos<longlen) continue; // we already checked it
      if (len<=MINLZ) goto done;
      if (len<=MINLZ+1 && used-pos>=hugelen) continue; // 
      if (len<left) {
          CHECK_REPLZ
      }
      if (my_min_ofs>used-pos) {
        my_min_ofs=used-pos;//we are checking matches in decreasing order. we need to check next matches only if those are shorter
        int ll=(used-pos>=hugelen)?1:0;
        for(j=MINLZ+1+ll;j<=len;j++) {
          int tmp=price_lz(used-pos,j-MINLZ+2,used);
          tmp+=cache(used+j);
          if (tmp<res || (tmp==res && my_best_ofs<pos-used)) {
            res=tmp;
            my_best_ofs=pos-used;
            my_best_len=j;
            my_use_olz=0;
            my_olz_len=0;
          }
        }
      }

    }    

done:
    if (sorted_prev(used)>=0) {
      sorted_next(sorted_prev(used))=sorted_next(used);
    }
    if (sorted_next(used)>=0) {
      sorted_len(sorted_next(used)) = min(sorted_len(sorted_next(used)),
                                            sorted_len(used));
      sorted_prev(sorted_next(used))=sorted_prev(used);
    }

    best_ofs(used)=my_best_ofs;
    best_len(used)=my_best_len;
    use_olz(used)=my_use_olz;
    olz_len(used)=my_olz_len;
    use_olz2(used)=my_use_olz2;
    olz_len2(used)=my_olz_len2;
    cache(used)=res;

    if (verbose && (i&0xFFF)==0) {
      printf("\x0D%d left ",i-start);
      fflush(stdout);
    }
  }

  res=8+cache(start);
  if (verbose) printf("\nres=%d\n",res);
  res+=7;
  res>>=3;
  if (verbose) printf("res bytes=%d\n",res);
  if (res>=n-start) {
    return n;
  };

  /* now we can easily generate compressed stream */
  initout(start);
  for(i=start;i<n;) {
    if (best_len(i)==1) {
      put_letter(in_buf[i]); i++;
    } else {
      int k,ofs,len,k2,len2;
dolz:
//      printf("do_lz %d:%d,left=%d\n",best_ofs[i],best_len[i],n-i);
      put_lz(best_ofs(i),best_len(i),i);
      ofs=best_ofs(i);
      len=olz_len(i);
      len2=olz_len2(i);
      k=use_olz(i);
      k2=use_olz2(i);
      i+=best_len(i);
      if (k>0) {
        for(;k>0;k--) put_letter(in_buf[i++]);
        if ((use_olz(i))&&(len==best_len(i))&&(ofs==best_ofs(i))) goto dolz;
//        printf("put_lz %d:%d,left=%d\n",ofs,len,n-i);
        put_lz(ofs,len,i);
        i+=len;
      if (k2>0) {
        for(;k2>0;k2--) put_letter(in_buf[i++]);
        if ((use_olz(i))&&(len2==best_len(i))&&(ofs==best_ofs(i))) goto dolz;
//        printf("put_lz %d:%d,left=%d\n",ofs,len,n-i);
        put_lz(ofs,len2,i);
        i+=len2;
	
      }
	
      }
    }
  }
  if (verbose) printf("out bytes=%d\n",outpos);
  return outpos;
}

#if 0
int main(int argc,char *argv[]) {
  FILE *ifd,*ofd;
  int n,i,bres,blz;
  uint8_t b;

  if (argc<3) {
    // note: -d0 (32k history) does not work right now
    printf("usage: lzoma [OPTION] input output [lzlit lit olz len dist]\n"
           "\t-1 .. -9 Compression level (default 7)\n"
           "\t-d[1..15] History size (default 9: 16M history; compression currently requires about 30x*history RAM)\n"
           "\t-v Be verbose\n"
           );
    printf("Notice: this program is at experimental stage of development. Compression format is not stable yet.\n");
    if (argc>1 && argv[1][0]=='%') { // undocumented debug feature to check correctness of offset encoding, when tuning parameters in lzoma.h
      int i;
      int total=atoi(argv[1]+1);//16*1024*1024;
      printf("%d\n",total);
      for(i=total-10;i<total;i++) {
        printf("%04d:",i);
        putenc(i, total,breaklz, 1);
        printf("\n");
      }
    }
    exit(0);
  }
  int arg=1;
  int metalevel = 7;
  int dict_size=9;
  while (arg<argc && argv[arg][0]=='-') {
    if (argv[arg][1]>='1' && argv[arg][1]<='9')
      metalevel = argv[arg][1]-'0';
    if (argv[arg][1]=='v')
      verbose = 1;
    if (argv[arg][1]=='d') {
      dict_size = atoi(argv[arg]+2);
      if (dict_size <1) dict_size=1; 
      if (dict_size >15) dict_size=15; 
    }
    arg++;
  }
  history_size=HISTORY_SIZE(dict_size);
  block_size=BLOCK_SIZE(dict_size);
  metalevel--;
  level=levels[metalevel][0];
  short_match_level=levels[metalevel][1];
  match_level=levels[metalevel][2];
  in_buf = (void *)malloc(history_size * sizeof(uint8_t)+1);
  rle = (void *)malloc(history_size * sizeof(uint32_t));
  state = (void *)malloc(max(block_size * sizeof(FutureState), history_size * sizeof(uint32_t)));
  past_state = (void *)malloc(history_size * sizeof(PastState));
  char *inf=argv[arg++];
  char *ouf=argv[arg++];
  ifd=fopen(inf,"rb");
  ofd=fopen(ouf,"wb");
  if (arg<argc) flzlit=fopen(argv[arg++],"wb");
  if (arg<argc) flit=fopen(argv[arg++],"wb");
  if (arg<argc) folz=fopen(argv[arg++],"wb");
  if (arg<argc) flen=fopen(argv[arg++],"wb");
  if (arg<argc) fdist=fopen(argv[arg++],"wb");

#ifdef EXPERIMENTS
  test=fopen("test","wb");
  test2=fopen("test2","wb");
  test3=fopen("test3","wb");
#endif
  
  int blocknum;
  uint32_t blk;
  for(blocknum=0;;blocknum++) {
    if (in_offset>history_size-block_size) {
      memmove(in_buf, in_buf+block_size, history_size-block_size);
      in_offset -= block_size;
    }
    n=fread(in_buf+in_offset,1,block_size,ifd);
    if (n<=0) {
      blk = BLOCK_STORED | BLOCK_LAST;
      fwrite(&blk,4,1,ofd);
      break;
    }
    if (verbose) printf("got %d bytes, packing...\n",n);
    if (blocknum==0) {
      /*
      int b1=cnt_bpes(in_buf,n);
      int use_e8=1;
      e8(in_buf, n);
      int b2=cnt_bpes(in_buf,n);
      printf("stats noe8 %d e8 %d\n",b1,b2);
      if (b2<=b1) {
        use_e8=0;
        printf("reverted e8\n");

        e8back(in_buf,n);
      }
      */
      /* 
         write compressed file header 
         we do it here only after we read some data
         TODO:
           at this stage we should decide if we will use any file-level compression filters
      */
      uint8_t header[8];
      header[0] = AuthorID >> 8;
      header[1] = AuthorID & 0xFF;
      header[2] = AlgoID[0];
      header[3] = AlgoID[1];
      header[4] = AlgoID[2];
      header[5] = AlgoID[3];
      header[6] = Version;
      int flags=0;
      header[7] = flags << 4 | dict_size; 
      fwrite(header,8,1,ofd);

      bres=pack(1,n);
    } else { // next blocks
      bres=pack(in_offset,in_offset+n);
    }
    uint32_t blk = (n < block_size) ? BLOCK_LAST : 0;
    if (bres==n) {
      blk |= BLOCK_STORED;
      blk |= n;
      fwrite(&blk,4,1,ofd);
      fwrite(in_buf+in_offset,1,n,ofd);
    } else {
      blk |= bres;
      fwrite(&blk,4,1,ofd);
      if (blk & BLOCK_LAST)
        fwrite(&n,4,1,ofd);
      fwrite(out_buf,1,bres,ofd);
      if (blk & BLOCK_LAST)
        break;
    }
    
    in_offset += n;
  }
  if (verbose) printf("closing files let=%d lz=%d olz=%d\n",stlet,stlz,stolz);
  if (verbose) printf("bits lzlit=%d let=%d olz=%d match=%d len=%d\n",bitslzlen,bitslit,bitsolzlen,bitsdist,bitslen);
  fclose(ifd);
  fclose(ofd);

#ifdef EXPERIMENTS
  fclose(test);
  fclose(test2);
  fclose(test3);
#endif

  return 0;
}
#else
#include "lzoma.h"

#define _fwrite(_buf_, _esize_, _size_, _fd_) { size_t _size = (_esize_)*(_size_); memcpy(_fd_, _buf_, _size); _fd_ += _size; }
#define _fread(_buf_, _esize_, _size_, _fd_, _fd__) ({ size_t _s = _fd__ - _fd_, _size = (_esize_)*(_size_); if(_s > _size) _s = _size; memcpy(_buf_, _fd_, _s); _fd_ += _s; _s; })

int lzomapack( unsigned char *in, int inlen, unsigned char *out, int metalevel) {							// TurboBench
  state = NULL;
  past_state = NULL;
  in_offset = 0;

  history_size=0;
  lastpos=0;
  bit_cnt=0;
  outpos=0;

 stlet=0;
 stlz=0;
 stolz=0;
 bitslzlen=0;
 bitsolzlen=0;
 bitslen=0;
 bitsdist=0;
 bitslit=0;

  unsigned char *ifd=in,*ifd_=in+inlen,*ofd=out;
  //FILE *ifd,*ofd;
  int n,i,bres,blz;
  uint8_t b;

  /*if (argc<3) {
    // note: -d0 (32k history) does not work right now
    printf("usage: lzoma [OPTION] input output [lzlit lit olz len dist]\n"
           "\t-1 .. -9 Compression level (default 7)\n"
           "\t-d[1..15] History size (default 9: 16M history; compression currently requires about 30x*history RAM)\n"
           "\t-v Be verbose\n"
           );
    printf("Notice: this program is at experimental stage of development. Compression format is not stable yet.\n");
    if (argc>1 && argv[1][0]=='%') { // undocumented debug feature to check correctness of offset encoding, when tuning parameters in lzoma.h
      int i;
      int total=atoi(argv[1]+1);//16*1024*1024;
      printf("%d\n",total);
      for(i=total-10;i<total;i++) {
        printf("%04d:",i);
        putenc(i, total,breaklz, 1);
        printf("\n");
      }
    }
    exit(0);
  }*/
  int arg=1;
  //int metalevel = 7;
  int dict_size=9;
  /*while (arg<argc && argv[arg][0]=='-') {
    if (argv[arg][1]>='1' && argv[arg][1]<='9')
      metalevel = argv[arg][1]-'0';
    if (argv[arg][1]=='v')
      verbose = 1;
    if (argv[arg][1]=='d') {
      dict_size = atoi(argv[arg]+2);
      if (dict_size <1) dict_size=1; 
      if (dict_size >15) dict_size=15; 
    }
    arg++;
  }*/
  history_size=HISTORY_SIZE(dict_size);
  block_size=BLOCK_SIZE(dict_size);
  metalevel--;
  level=levels[metalevel][0];
  short_match_level=levels[metalevel][1];
  match_level=levels[metalevel][2];
  in_buf = (void *)malloc(history_size * sizeof(uint8_t)+1);
  rle = (void *)malloc(history_size * sizeof(uint32_t));
  state = (void *)malloc(max(block_size * sizeof(FutureState), history_size * sizeof(uint32_t)));
  past_state = (void *)malloc(history_size * sizeof(PastState));
  /*char *inf=argv[arg++];
  char *ouf=argv[arg++];
  ifd=fopen(inf,"rb");
  ofd=fopen(ouf,"wb");
  if (arg<argc) flzlit=fopen(argv[arg++],"wb");
  if (arg<argc) flit=fopen(argv[arg++],"wb");
  if (arg<argc) folz=fopen(argv[arg++],"wb");
  if (arg<argc) flen=fopen(argv[arg++],"wb");
  if (arg<argc) fdist=fopen(argv[arg++],"wb");*/

#ifdef EXPERIMENTS
  test=fopen("test","wb");
  test2=fopen("test2","wb");
  test3=fopen("test3","wb");
#endif
  
  int blocknum;
  uint32_t blk;
  for(blocknum=0;;blocknum++) {
    if (in_offset>history_size-block_size) {
      memmove(in_buf, in_buf+block_size, history_size-block_size);
      in_offset -= block_size;
    }
    n=_fread(in_buf+in_offset,1,block_size,ifd,ifd_);
    if (n<=0) {
      blk = BLOCK_STORED | BLOCK_LAST;
      _fwrite(&blk,4,1,ofd);
      break;
    }
    if (verbose) printf("got %d bytes, packing...\n",n);
    if (blocknum==0) {
      /*
      int b1=cnt_bpes(in_buf,n);
      int use_e8=1;
      e8(in_buf, n);
      int b2=cnt_bpes(in_buf,n);
      printf("stats noe8 %d e8 %d\n",b1,b2);
      if (b2<=b1) {
        use_e8=0;
        printf("reverted e8\n");

        e8back(in_buf,n);
      }
      */
      /* 
         write compressed file header 
         we do it here only after we read some data
         TODO:
           at this stage we should decide if we will use any file-level compression filters
      */
      uint8_t header[8];
      header[0] = AuthorID >> 8;
      header[1] = AuthorID & 0xFF;
      header[2] = AlgoID[0];
      header[3] = AlgoID[1];
      header[4] = AlgoID[2];
      header[5] = AlgoID[3];
      header[6] = Version;
      int flags=0;
      header[7] = flags << 4 | dict_size; 
      _fwrite(header,8,1,ofd);

      bres=pack(1,n);
    } else { // next blocks
      bres=pack(in_offset,in_offset+n);
    }
    uint32_t blk = (n < block_size) ? BLOCK_LAST : 0;
    if (bres==n) {
      blk |= BLOCK_STORED;
      blk |= n;
      _fwrite(&blk,4,1,ofd);
      _fwrite(in_buf+in_offset,1,n,ofd);
    } else {
      blk |= bres;
      _fwrite(&blk,4,1,ofd);
      if (blk & BLOCK_LAST)
        _fwrite(&n,4,1,ofd);
      _fwrite(out_buf,1,bres,ofd);
      if (blk & BLOCK_LAST)
        break;
    }
    
    in_offset += n;
  }
  if (verbose) printf("closing files let=%d lz=%d olz=%d\n",stlet,stlz,stolz);
  if (verbose) printf("bits lzlit=%d let=%d olz=%d match=%d len=%d\n",bitslzlen,bitslit,bitsolzlen,bitsdist,bitslen);
  fclose(ifd);
  fclose(ofd);

#ifdef EXPERIMENTS
  fclose(test);
  fclose(test2);
  fclose(test3);
#endif

 // return 0;
  free(in_buf);
  free(rle);
  free(state);
  free(past_state);
  return ofd - out;
}
#endif
