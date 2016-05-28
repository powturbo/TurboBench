/*

BCM - A BWT-based file compressor

Copyright (C) 2008-2016 Ilya Muravyov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifdef __GNUC__

#define _FILE_OFFSET_BITS 64
#define _fseeki64 fseeko64
#define _ftelli64 ftello64

#ifdef HAVE_GETC_UNLOCKED
#undef getc
#define getc getc_unlocked
#endif

#ifdef HAVE__putc_UNLOCKED
#undef _putc
#define _putc _putc_unlocked
#endif

#endif // __GNUC__

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "divsufsort.h" // libdivsufsort-lite
#include "bcm.h"						//TurboBench
#include "../libbsc/libbsc/bwt/bwt.h"	//TurboBench

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned long long QWORD;

const char magic[]="BCM!";

//FILE* in;
//FILE* out;
static unsigned char *in,*in_;
static unsigned char* out;
#define _putc(__ch, __out) *__out++ = (__ch)
#define _getc(in, in_) (in<in_?*in++:-1)

struct Encoder
{
	DWORD low;
	DWORD high;
	DWORD code;

	Encoder()
	{
		low=0;
		high=DWORD(-1);
		code=0;
	}

	void EncodeBit0(DWORD p)
	{
#ifdef _WIN64
		low+=((QWORD(high-low)*p)>>18)+1;
#else
		low+=((QWORD(high-low)*(p<<(32-18)))>>32)+1;
#endif
		while ((low^high)<(1<<24))
		{
			_putc(low>>24, out);
			low<<=8;
			high=(high<<8)+255;
		}
	}

	void EncodeBit1(DWORD p)
	{
#ifdef _WIN64
		high=low+((QWORD(high-low)*p)>>18);
#else
		high=low+((QWORD(high-low)*(p<<(32-18)))>>32);
#endif
		while ((low^high)<(1<<24))
		{
			_putc(low>>24, out);
			low<<=8;
			high=(high<<8)+255;
		}
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
			code=(code<<8)+_getc(in,in_);
	}

	int DecodeBit(DWORD p)
	{
#ifdef _WIN64
		const DWORD mid=low+((QWORD(high-low)*p)>>18);
#else
		const DWORD mid=low+((QWORD(high-low)*(p<<(32-18)))>>32);
#endif
		const int bit=(code<=mid);
		if (bit)
			high=mid;
		else
			low=mid+1;

		while ((low^high)<(1<<24))
		{
			low<<=8;
			high=(high<<8)+255;
			code=(code<<8)+_getc(in,in_);
		}

		return bit;
	}
};

template<int RATE>
struct Counter
{
	WORD p;

	Counter()
	{
		p=1<<15;
	}

	void UpdateBit0()
	{
		p-=p>>RATE;
	}

	void UpdateBit1()
	{
		p+=(p^65535)>>RATE;
	}
};

struct CM: Encoder
{
	Counter<2> counter0[256];
	Counter<4> counter1[256][256];
	Counter<6> counter2[2][256][17];
	int c1;
	int c2;
	int run;

	CM()
	{
		c1=0;
		c2=0;
		run=0;
		
		for (int i=0; i<2; ++i)
		{
			for (int j=0; j<256; ++j)
			{
				for (int k=0; k<17; ++k)
					counter2[i][j][k].p=(k<<12)-(k==16);
			}
		}
	}

	void Encode32(DWORD n)
	{
		for (int i=0; i<32; ++i)
		{
			if (n&(1<<31))
				Encoder::EncodeBit1(1<<17);
			else
				Encoder::EncodeBit0(1<<17);
			n+=n;
		}
	}

	DWORD Decode32()
	{
		DWORD n=0;
		for (int i=0; i<32; ++i)
			n+=n+Encoder::DecodeBit(1<<17);

		return n;
	}

	void Encode(int c)
	{
		if (c1==c2)
			++run;
		else
			run=0;
		const int f=(run>2);

		int ctx=1;
		while (ctx<256)
		{
			const int p0=counter0[ctx].p;
			const int p1=counter1[c1][ctx].p;
			const int p2=counter1[c2][ctx].p;
			const int p=((p0+p1)*7+p2+p2)>>4;

			const int j=p>>12;
			const int x1=counter2[f][ctx][j].p;
			const int x2=counter2[f][ctx][j+1].p;
			const int ssep=x1+(((x2-x1)*(p&4095))>>12);

			const int bit=c&128;
			c+=c;

			if (bit)
			{
				Encoder::EncodeBit1(ssep*3+p);
				counter0[ctx].UpdateBit1();
				counter1[c1][ctx].UpdateBit1();
				counter2[f][ctx][j].UpdateBit1();
				counter2[f][ctx][j+1].UpdateBit1();
				ctx+=ctx+1;
			}
			else
			{
				Encoder::EncodeBit0(ssep*3+p);
				counter0[ctx].UpdateBit0();
				counter1[c1][ctx].UpdateBit0();
				counter2[f][ctx][j].UpdateBit0();
				counter2[f][ctx][j+1].UpdateBit0();
				ctx+=ctx;
			}
		}

		c2=c1;
		c1=ctx&255;
	}

	int Decode()
	{
		if (c1==c2)
			++run;
		else
			run=0;
		const int f=(run>2);

		int ctx=1;
		while (ctx<256)
		{
			const int p0=counter0[ctx].p;
			const int p1=counter1[c1][ctx].p;
			const int p2=counter1[c2][ctx].p;
			const int p=((p0+p1)*7+p2+p2)>>4;

			const int j=p>>12;
			const int x1=counter2[f][ctx][j].p;
			const int x2=counter2[f][ctx][j+1].p;
			const int ssep=x1+(((x2-x1)*(p&4095))>>12);

			const int bit=Encoder::DecodeBit(ssep*3+p);

			if (bit)
			{
				counter0[ctx].UpdateBit1();
				counter1[c1][ctx].UpdateBit1();
				counter2[f][ctx][j].UpdateBit1();
				counter2[f][ctx][j+1].UpdateBit1();
				ctx+=ctx+1;
			}
			else
			{
				counter0[ctx].UpdateBit0();
				counter1[c1][ctx].UpdateBit0();
				counter2[f][ctx][j].UpdateBit0();
				counter2[f][ctx][j+1].UpdateBit0();
				ctx+=ctx;
			}
		}

		c2=c1;
		return c1=ctx&255;
	}
};// cm;

struct CRC
{
	DWORD t[4][256];
	DWORD crc;

	CRC()
	{
		for (int i=0; i<256; ++i)
		{
			DWORD r=i;
			for (int j=0; j<8; ++j)
				r=(r>>1)^(-int(r&1)&0xedb88320);
			t[0][i]=r;
		}

		for (int i=0; i<256; ++i)
		{
			t[1][i]=t[0][t[0][i]&255]^(t[0][i]>>8);
			t[2][i]=t[0][t[1][i]&255]^(t[1][i]>>8);
			t[3][i]=t[0][t[2][i]&255]^(t[2][i]>>8);
		}

		crc=DWORD(-1);
	}

	DWORD operator()() const
	{
		return ~crc;
	}

	void Clear()
	{
		crc=DWORD(-1);
	}

	void Update(int c)
	{
		crc=t[0][(crc^c)&255]^(crc>>8);
	}

	void Update(BYTE* p, int n)
	{
#ifdef _WIN32
		for (; n>=4; n-=4)
		{
			crc^=*(const DWORD*)p;
			p+=4;
			crc=t[0][crc>>24]
				^t[1][(crc>>16)&255]
				^t[2][(crc>>8)&255]
				^t[3][crc&255];
		}
#endif
		for (; n>0; --n)
			crc=t[0][(crc^*p++)&255]^(crc>>8);
	}
} crc;

//BYTE* buf;

//void compress(int bsize)
/* cm;

byte* buf;

void compress(int b)*/
unsigned bcmcompress(unsigned char *in, int n, unsigned char *_out)
{
  CM cm;
  out = _out;

	/*if (_fseeki64(in, 0, SEEK_END))
	{
		perror("Fseek() failed");
		exit(1);
	}
	const long long flen=_ftelli64(in);
	if (flen<0)
	{
		perror("Ftell() failed");
		exit(1);
	}
	if (bsize>flen)
		bsize=int(flen);
	rewind(in);

	buf=(byte*)calloc(b, 5);*/
	unsigned char *buf = (BYTE*)malloc(n+16);

	if (!buf)
	{
		fprintf(stderr, "Out of memory!\n");
		exit(1);
	}

	/*_putc(magic[0], out);
	_putc(magic[1], out);
	_putc(magic[2], out);
	_putc(magic[3], out);

	int n;
	while ((n=fread(buf, 1, bsize, in))>0)
	{
		crc.Update(buf, n);

		const int idx=divbwt(buf, buf, (int*)&buf[bsize], n);*/
		const int idx = divbwt(in, buf, NULL, n, NULL, NULL, 0); // TurboBench
		if (idx<1)
		{
			perror("Divbwt() failed");
			exit(1);
		}

		//cm.Encode32(n);
		cm.Encode32(idx);

		for (int i=0; i<n; ++i)
			cm.Encode(buf[i]);
	//}

	cm.Encode32(0); // EOF
	//cm.Encode32(crc());

	cm.Flush();
	free(buf); // TurboBench
    return out - _out; // TurboBench
}

unsigned bcmdecompress(unsigned char *_in, int inlen, unsigned char *_out, int n)
{
  CM cm;
  in = _in; in_ = _in+inlen; out = _out; //printf("n=%d ", _n);
  unsigned char *buf;
/*void decompress()
{
	if (_getc(in,in_)!=magic[0]
		|| _getc(in,in_)!=magic[1]
		|| _getc(in,in_)!=magic[2]
		|| _getc(in,in_)!=magic[3])
	{
		fprintf(stderr, "Not in BCM format!\n");
		exit(1);
	}*/

	cm.Init();

	int bsize=0;

	/*int n;
	while ((n=cm.Decode32())>0)
	{
		if (!bsize)*/
		{
			buf=(BYTE*)calloc(bsize=n, 5);
			if (!buf)
			{
				fprintf(stderr, "Out of memory!\n");
				exit(1);
			}
		}

		const int idx=cm.Decode32();
		if (n>bsize || idx<1 || idx>n)
		{
			fprintf(stderr, "File corrupted!\n");
			exit(1);
		}
		// Inverse BW-transform
		int t[257]={0};
		for (int i=0; i<n; ++i)
			++t[(buf[i]=cm.Decode())+1];
		for (int i=1; i<256; ++i)
			t[i]+=t[i-1];
		int* next=(int*)&buf[bsize];
		for (int i=0; i<n; ++i)
			next[t[buf[i]]++]=i+(i>=idx);
		for (int p=idx; p;)
		{
			p=next[p-1];
			const int c=buf[p-(p>=idx)];
			_putc(c, out);
			//crc.Update(c);
		}
	//}
  free(buf);
  return in - _in;		
	

	/*if (cm.Decode32()!=crc())
	{
		fprintf(stderr, "CRC error!\n");
		exit(1);
	}*/
}

unsigned bcmenc(unsigned char *in, int n, unsigned char *_out) {
  CM cm;
  out = _out; 
  unsigned char *ip = in; while(ip < in+n) cm.Encode(*ip++);
  cm.Flush();
  return out - _out;
}

unsigned bcmdec(unsigned char *_in, unsigned inlen, unsigned char *out, unsigned n) {
  CM cm;
  in = _in; in_ = _in+inlen; 
  cm.Init();
  unsigned char *op = out; while(op < out+n) *op++= cm.Decode();
  return in - _in;
}
#if 0
int main(int argc, char** argv)
{
	const clock_t start=clock();

	int bsize=32<<20; // 32 MB
	bool do_decomp=false;
	bool overwrite=false;

	while (argc>1 && *argv[1]=='-')
	{
		switch (argv[1][1])
		{
		case 'b':
			bsize=atoi(&argv[1][2])
				<<(argv[1][strlen(argv[1])-1]=='k'?10:20);
			if (bsize<1)
			{
				fprintf(stderr, "Block size is out of range!\n");
				exit(1);
			}
			break;
		case 'd':
			do_decomp=true;
			break;
		case 'f':
			overwrite=true;
			break;
		default:
			fprintf(stderr, "Unknown option: %s\n", argv[1]);
			exit(1);
		}

		--argc;
		++argv;
	}

	if (argc<2)
	{
		fprintf(stderr,
			"BCM - A BWT-based file compressor, v1.10 beta\n"
			"Copyright (C) 2008-2016 Ilya Muravyov\n"
			"\n"
			"Usage: BCM [options] infile [outfile]\n"
			"\n"
			"Options:\n"
			"  -b#[k] Set block size to # MB or KB (default is 32 MB)\n"
			"  -d     Decompress\n"
			"  -f     Force overwrite of output file\n");
		exit(1);
	}

	in=fopen(argv[1], "rb");
	if (!in)
	{
		perror(argv[1]);
		exit(1);
	}

	char ofname[FILENAME_MAX];
	if (argc<3)
	{
		strcpy(ofname, argv[1]);
		if (do_decomp)
		{
			const int p=strlen(ofname)-4;
			if (p>0 && !strcmp(&ofname[p], ".bcm"))
				ofname[p]='\0';
			else
				strcat(ofname, ".out");
		}
		else
			strcat(ofname, ".bcm");
	}
	else
		strcpy(ofname, argv[2]);

	if (!overwrite)
	{
		FILE* f=fopen(ofname, "rb");
		if (f)
		{
			fclose(f);

			fprintf(stderr, "%s already exists. Overwrite (y/n)? ", ofname);
			fflush(stderr);

			if (getchar()!='y')
				exit(1);
		}
	}

	out=fopen(ofname, "wb");
	if (!out)
	{
		perror(ofname);
		exit(1);
	}

	if (do_decomp)
	{
		fprintf(stderr, "Decompressing %s: ", argv[1]);
		fflush(stderr);

		decompress();
	}
	else
	{
		fprintf(stderr, "Compressing %s: ", argv[1]);
		fflush(stderr);

		compress(bsize);
	}

	fprintf(stderr, "%lld->%lld in %.1fs\n", _ftelli64(in), _ftelli64(out),
		double(clock()-start)/CLOCKS_PER_SEC);

	fprintf(stderr, "CRC = %08X\n", crc()); // DEBUG

	fclose(in);
	fclose(out);

	free(buf);

	return 0;
}
#endif
