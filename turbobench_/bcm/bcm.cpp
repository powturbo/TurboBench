/*

BCM - A BWT-based file compressor

Copyright (C) 2008-2021 Ilya Muravyov

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/
// Adapted to in-memory compression by powturbo 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef NO_UTIME
#  include <sys/types.h>
#  include <sys/stat.h>

#  ifdef _MSC_VER
#    include <sys/utime.h>
#  else
#    include <utime.h>
#  endif
#endif
#include "bcm.h"
#include "../libbsc/libbsc/bwt/libsais/libsais.h"

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef signed long long S64;

// Globals

//#define BCM_ID 0x214D4342 // "BCM!"

static unsigned char *in,*in_;
static unsigned char* out;
#define _putc(__ch, __out) *__out++ = (__ch)
#define _getc(in, in_) (in<in_?*in++:-1)

struct Encoder
{
    U32 low;
    U32 high;
    U32 code;

    Encoder()
    {
        low=0;
        high=0xFFFFFFFF;
        code=0;
    }

    void Flush()
    {
        for (int i=0; i<4; ++i)
        {
            _putc(low>>24, out);
            low<<=8;
        }
    }

    void Init()
    {
        for (int i=0; i<4; ++i)
            code=(code<<8)|_getc(in,in_);
    }

    template<int P_LOG>
    void EncodeBit(int bit, U32 p)
    {
        const U32 mid=low+((U64(high-low)*p)>>P_LOG);

        if (bit)
            high=mid;
        else
            low=mid+1;

        // Renormalize
        while ((low^high)<(1<<24))
        {
            _putc(low>>24, out);
            low<<=8;
            high=(high<<8)|255;
        }
    }

    template<int P_LOG>
    int DecodeBit(U32 p)
    {
        const U32 mid=low+((U64(high-low)*p)>>P_LOG);

        const int bit=(code<=mid);
        if (bit)
            high=mid;
        else
            low=mid+1;

        // Renormalize
        while ((low^high)<(1<<24))
        {
            low<<=8;
            high=(high<<8)|255;
            code=(code<<8)|_getc(in,in_);
        }

        return bit;
    }
};

template<int RATE>
struct Counter
{
    U16 p;

    Counter()
    {
        p=1<<15; // 0.5
    }

    void Update1()
    {
        p+=(p^0xFFFF)>>RATE;
    }

    void Update0()
    {
        p-=p>>RATE;
    }
};

struct CM: Encoder
{
    Counter<2> counter0[256];
    Counter<4> counter1[256][256];
    Counter<6> counter2[2][256][17];
    int run;
    int c1;
    int c2;

    CM()
    {
        run=0;
        c1=0;
        c2=0;

        for (int i=0; i<2; ++i)
        {
            for (int j=0; j<256; ++j)
            {
                for (int k=0; k<=16; ++k)
                    counter2[i][j][k].p=(k<<12)-(k==16);
            }
        }
    }

    void Put32(U32 x)
    {
        for (U32 i=1<<31; i>0; i>>=1)
            EncodeBit<1>(x&i, 1); // p=0.5
    }

    U32 Get32()
    {
        U32 x=0;
        for (int i=0; i<32; ++i)
            x+=x+DecodeBit<1>(1); // p=0.5

        return x;
    }

    void Put(int c)
    {
        const int f=(run>2);

        int ctx=1;
        for (int i=128; i>0; i>>=1)
        {
            const int p0=counter0[ctx].p;
            const int p1=counter1[c1][ctx].p;
            const int p2=counter1[c2][ctx].p;
            const int p=(((p0+p1)*7)+p2+p2)>>4;

            // SSE with linear interpolation
            const int j=p>>12;
            const int x1=counter2[f][ctx][j].p;
            const int x2=counter2[f][ctx][j+1].p;
            const int ssep=x1+(((x2-x1)*(p&4095))>>12);

            if (c&i)
            {
                EncodeBit<18>(1, p+ssep+ssep+ssep);

                counter0[ctx].Update1();
                counter1[c1][ctx].Update1();
                counter2[f][ctx][j].Update1();
                counter2[f][ctx][j+1].Update1();

                ctx+=ctx+1;
            }
            else
            {
                EncodeBit<18>(0, p+ssep+ssep+ssep);

                counter0[ctx].Update0();
                counter1[c1][ctx].Update0();
                counter2[f][ctx][j].Update0();
                counter2[f][ctx][j+1].Update0();

                ctx+=ctx;
            }
        }

        c2=c1;
        c1=ctx-256;

        if (c1==c2)
            ++run;
        else
            run=0;
    }

    int Get()
    {
        const int f=(run>2);

        int ctx=1;
        while (ctx<256)
        {
            const int p0=counter0[ctx].p;
            const int p1=counter1[c1][ctx].p;
            const int p2=counter1[c2][ctx].p;
            const int p=(((p0+p1)*7)+p2+p2)>>4;

            // SSE with linear interpolation
            const int j=p>>12;
            const int x1=counter2[f][ctx][j].p;
            const int x2=counter2[f][ctx][j+1].p;
            const int ssep=x1+(((x2-x1)*(p&4095))>>12);

            if (DecodeBit<18>(p+ssep+ssep+ssep))
            {
                counter0[ctx].Update1();
                counter1[c1][ctx].Update1();
                counter2[f][ctx][j].Update1();
                counter2[f][ctx][j+1].Update1();

                ctx+=ctx+1;
            }
            else
            {
                counter0[ctx].Update0();
                counter1[c1][ctx].Update0();
                counter2[f][ctx][j].Update0();
                counter2[f][ctx][j+1].Update0();

                ctx+=ctx;
            }
        }

        c2=c1;
        c1=ctx-256;

        if (c1==c2)
            ++run;
        else
            run=0;

        return c1;
    }
} cm;

struct CRC
{
    U32 tab[8][256];
    U32 crc;

    CRC()
    {
        for (int i=0; i<256; ++i)
        {
            U32 x=i;
            for (int j=0; j<8; ++j)
                x=(x>>1)^(0xEDB88320&-int(x&1));
            tab[0][i]=x;
        }
        for (int i=0; i<256; ++i)
        {
            tab[1][i]=(tab[0][i]>>8)^tab[0][tab[0][i]&255];
            tab[2][i]=(tab[1][i]>>8)^tab[0][tab[1][i]&255];
            tab[3][i]=(tab[2][i]>>8)^tab[0][tab[2][i]&255];
            tab[4][i]=(tab[3][i]>>8)^tab[0][tab[3][i]&255];
            tab[5][i]=(tab[4][i]>>8)^tab[0][tab[4][i]&255];
            tab[6][i]=(tab[5][i]>>8)^tab[0][tab[5][i]&255];
            tab[7][i]=(tab[6][i]>>8)^tab[0][tab[6][i]&255];
        }
        crc=0xFFFFFFFF;
    }

    U32 operator()() const
    {
        return crc^0xFFFFFFFF;
    }

    void Update(U8* s, int n)
    {
        U32 x=crc;
        while (n>=8)
        {
            x^=*reinterpret_cast<const U32*>(s);
            const U32 t=*reinterpret_cast<const U32*>(s+4);
            x=tab[0][t>>24]
              ^tab[1][(t>>16)&255]
              ^tab[2][(t>>8)&255]
              ^tab[3][t&255]
              ^tab[4][x>>24]
              ^tab[5][(x>>16)&255]
              ^tab[6][(x>>8)&255]
              ^tab[7][x&255];
            s+=8;
            n-=8;
        }
        while (n--)
            x=(x>>8)^tab[0][(x^*s++)&255];
        crc=x;
    }
} crc;

template<typename T>
inline T* MemAlloc(size_t n)
{
    T* p=reinterpret_cast<T*>(malloc(n*sizeof(T)));
    if (!p)
    {
        perror("Malloc() failed");
        exit(1);
    }
    return p;
}

unsigned bcmcompress(unsigned char *in, int n, unsigned char *_out) {
  CM cm;
  out = _out;

  U8*  buf = MemAlloc<U8>(n+16);
  int* ptr = MemAlloc<int>(n+1024);

  const int idx=libsais_bwt(in, buf, ptr, n, 0, 0);
  if (idx<1) {
    perror("Libsais_bwt() failed");
    exit(1);
  }
  cm.Put32(idx); // BWT index
  for (int i=0; i<n; ++i)
    cm.Put(buf[i]);
  cm.Put32(0); // EOF
  cm.Flush();

  free(buf);
  free(ptr);
  return out - _out;
}

unsigned bcmdecompress(unsigned char *_in, int n, unsigned char *_out, int outlen) {
  CM cm;
  in  = _in;
  in_ = _in + n;
  out = _out;

  if (outlen <= 0)
    return 0;

  int cnt[257];
  U8*  buf = MemAlloc<U8>(outlen + 16);
  int* ptr = MemAlloc<int>(outlen + 1024);

  cm.Init();

  const int idx = cm.Get32();
  if (idx < 1 || idx > outlen) { free(buf); free(ptr); return 0; /* or error */ }

    // Inverse BW-transform
  memset(cnt, 0, sizeof(cnt));
  for (int i = 0; i < outlen; ++i)
    ++cnt[(buf[i] = cm.Get()) + 1];
  for (int i = 1; i < 256; ++i)
    cnt[i] += cnt[i - 1];
  for (int i = 0; i < outlen; ++i)
    ptr[cnt[buf[i]]++] = i - (i < idx);

  int p = idx - 1;
  for (int i = 0; i < outlen; ++i) {
    int c = 0;
    int half = 127;
    for (int j = 0; j < 8; ++j) {
      if (cnt[c + half] <= p)
        c += half + 1;
      half >>= 1;
    }
    buf[i] = (U8)c;
    p = ptr[p];
  }
  memcpy(_out, buf, outlen);

  (void)cm.Get32();
  free(buf);
  free(ptr);
  return (unsigned)outlen;
}

unsigned bcmenc(unsigned char *in, int n, unsigned char *_out) {
  CM cm;
  out = _out; 
  unsigned char *ip = in; while(ip < in+n) cm.Put(*ip++);
  cm.Flush();
  return out - _out;
}

unsigned bcmdec(unsigned char *_in, unsigned inlen, unsigned char *out, unsigned n) {
  CM cm;
  in = _in; in_ = _in+inlen; 
  cm.Init();
  unsigned char *op = out; while(op < out+n) *op++= cm.Get();
  return in - _in;
}

