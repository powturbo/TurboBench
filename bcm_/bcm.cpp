/*

BCM - A BWT-based file compressor
Written and placed in the public domain by Ilya Muravyov

*/

#ifdef __GNUC__
#define _FILE_OFFSET_BITS 64
#define _fseeki64 fseeko64
#define _ftelli64 ftello64
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "divsufsort.h" // libdivsufsort-lite
#include "bcm.h"
#include "../libbsc/libbsc/bwt/bwt.h"

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long long ulonglong;

const char magic[]="BCM1";

//FILE* in;
//FILE* out;
static unsigned char *in,*in_;
static unsigned char* out;
#define _putc(__ch, __out) *__out++ = (__ch)
#define _getc(in, in_) (in<in_?*in++:-1)

struct Encoder
{
	uint code;
	uint low;
	uint high;

	Encoder()
	{
		code=0;
		low=0;
		high=uint(-1);
	}

	void EncodeBit0(uint p)
	{
#ifdef _WIN64
		low+=((ulonglong(high-low)*p)>>18)+1;
#else
		low+=((ulonglong(high-low)*(p<<14))>>32)+1;
#endif
		while ((low^high)<(1<<24))
		{
			_putc(low>>24, out);
			low<<=8;
			high=(high<<8)|255;
		}
	}

	void EncodeBit1(uint p)
	{
#ifdef _WIN64
		high=low+((ulonglong(high-low)*p)>>18);
#else
		high=low+((ulonglong(high-low)*(p<<14))>>32);
#endif
		while ((low^high)<(1<<24))
		{
			_putc(low>>24, out);
			low<<=8;
			high=(high<<8)|255;
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
			code=(code<<8)|_getc(in,in_);
	}

	int DecodeBit(uint p)
	{
#ifdef _WIN64
		const uint mid=low+((ulonglong(high-low)*p)>>18);
#else
		const uint mid=low+((ulonglong(high-low)*(p<<14))>>32);
#endif
		const int bit=(code<=mid);
		if (bit)
			high=mid;
		else
			low=mid+1;

		while ((low^high)<(1<<24))
		{
			code=(code<<8)|_getc(in,in_);
			low<<=8;
			high=(high<<8)|255;
		}

		return bit;
	}
};

template<int RATE>
struct Counter
{
	int p;

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
					counter2[i][j][k].p=(k-(k==16))<<12;
			}
		}
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
			const int p=(p0+p0+p0+p0+p1+p1+p1+p2)>>3;

			const int idx=p>>12;
			const int x1=counter2[f][ctx][idx].p;
			const int x2=counter2[f][ctx][idx+1].p;
			const int ssep=x1+(((x2-x1)*(p&4095))>>12);

			const int bit=c&128;
			c+=c;

			if (bit)
			{
				Encoder::EncodeBit1(p+ssep+ssep+ssep);
				counter0[ctx].UpdateBit1();
				counter1[c1][ctx].UpdateBit1();
				counter2[f][ctx][idx].UpdateBit1();
				counter2[f][ctx][idx+1].UpdateBit1();
				ctx+=ctx+1;
			}
			else
			{
				Encoder::EncodeBit0(p+ssep+ssep+ssep);
				counter0[ctx].UpdateBit0();
				counter1[c1][ctx].UpdateBit0();
				counter2[f][ctx][idx].UpdateBit0();
				counter2[f][ctx][idx+1].UpdateBit0();
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
			const int p=(p0+p0+p0+p0+p1+p1+p1+p2)>>3;

			const int idx=p>>12;
			const int x1=counter2[f][ctx][idx].p;
			const int x2=counter2[f][ctx][idx+1].p;
			const int ssep=x1+(((x2-x1)*(p&4095))>>12);

			const int bit=Encoder::DecodeBit(p+ssep+ssep+ssep);

			if (bit)
			{
				counter0[ctx].UpdateBit1();
				counter1[c1][ctx].UpdateBit1();
				counter2[f][ctx][idx].UpdateBit1();
				counter2[f][ctx][idx+1].UpdateBit1();
				ctx+=ctx+1;
			}
			else
			{
				counter0[ctx].UpdateBit0();
				counter1[c1][ctx].UpdateBit0();
				counter2[f][ctx][idx].UpdateBit0();
				counter2[f][ctx][idx+1].UpdateBit0();
				ctx+=ctx;
			}
		}

		c2=c1;
		return c1=ctx&255;
	}
}/* cm;

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
	if (b>flen)
		b=int(flen);
	rewind(in);

	buf=(byte*)calloc(b, 5);*/
	unsigned char *buf = (byte*)malloc(n+16);
	if (!buf)
	{
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}

	/*putc(magic[0], out);									
	putc(magic[1], out);
	putc(magic[2], out);
	putc(magic[3], out);

	int n;
	while ((n=fread(buf, 1, b, in))>0)*/						
	{
//		const int p=divbwt(buf, buf, (int*)&buf[b], n);
		const int p = divbwt(in, buf, NULL, n, NULL, NULL, 0); // TurboBench
		if (p<1)
		{
			perror("Divbwt() failed");
			exit(1);
		}

		/*cm.Encode(n>>24); // TurboBench
		cm.Encode(n>>16);
		cm.Encode(n>>8);
		cm.Encode(n);*/
		cm.Encode(p>>24);
		cm.Encode(p>>16);
		cm.Encode(p>>8);
		cm.Encode(p);

		for (int i=0; i<n; ++i)
			cm.Encode(buf[i]);
	}

	cm.Encode(0); // EOF
	cm.Encode(0);
	cm.Encode(0);
	cm.Encode(0);

	cm.Flush();
	free(buf); // TurboBench
  return out - _out; // TurboBench
}

//void decompress()
unsigned bcmdecompress(unsigned char *_in, int inlen, unsigned char *_out, int n)
{
  CM cm;
  in = _in; in_ = _in+inlen; out = _out; //printf("n=%d ", _n);
  unsigned char *buf;

	/*if (getc(in)!=magic[0]
		|| getc(in)!=magic[1]
		|| getc(in)!=magic[2]
		|| getc(in)!=magic[3])
	{
		fprintf(stderr, "Not in BCM format\n");
		exit(1);
	}*/

	cm.Init();

	int b=0;

	/*for (;;)
	{
		const int n=(cm.Decode()<<24)
			|(cm.Decode()<<16)
			|(cm.Decode()<<8)
			|cm.Decode();
		if (!n) // EOF
			break;
		if (!b)*/
		{
			buf=(byte*)calloc(b=n, 5);
			if (!buf)
			{
				fprintf(stderr, "Out of memory\n");
				exit(1);
			}
		}
		const int p=(cm.Decode()<<24)
			|(cm.Decode()<<16)
			|(cm.Decode()<<8)
			|cm.Decode();
		if (n<1 || n>b || p<1 || p>n)
		{
			fprintf(stderr, "File corrupted\n");
			exit(1);
		}
		// Inverse BWT
		int t[257]={0};
		for (int i=0; i<n; ++i)
			++t[(buf[i]=cm.Decode())+1];
		for (int i=1; i<256; ++i)
			t[i]+=t[i-1];
		int* next=(int*)&buf[b];
		for (int i=0; i<n; ++i)
			next[t[buf[i]]++]=i+(i>=p);
		for (int i=p; i;)
		{
			i=next[i-1];
			_putc(buf[i-(i>=p)], out);
		}
	//}
  free(buf);
  return in - _in;		
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

	int block_size=20<<20; // 20 MB
	bool do_decomp=false;
	bool overwrite=false;

	while (argc>1 && *argv[1]=='-')
	{
		switch (argv[1][1])
		{
		case 'b':
			block_size=atoi(&argv[1][2])
				<<(argv[1][strlen(argv[1])-1]=='k'?10:20);
			if (block_size<1)
			{
				fprintf(stderr, "Block size is out of range\n");
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
			"BCM - A BWT-based file compressor, v1.02\n"
			"\n"
			"Usage: BCM [options] infile [outfile]\n"
			"\n"
			"Options:\n"
			"  -b#[k] Set block size to # MB or KB (default is 20 MB)\n"
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

	if (!strcmp(ofname, argv[1]))
	{
		fprintf(stderr, "%s: Cannot %scompress onto itself\n", argv[1],
			do_decomp?"de":"");
		exit(1);
	}

	if (!overwrite)
	{
		FILE* f=fopen(ofname, "rb");
		if (f)
		{
			fclose(f);
			fprintf(stderr, "%s already exists\n", ofname);
			exit(1);
		}
	}

	out=fopen(ofname, "wb");
	if (!out)
	{
		perror(ofname);
		exit(1);
	}

	fprintf(stderr, "%s: ", argv[1]);
	fflush(stderr);

	if (do_decomp)
		decompress();
	else
		compress(block_size);

	fprintf(stderr, "%lld->%lld in %.3fs\n", _ftelli64(in), _ftelli64(out),
		double(clock()-start)/CLOCKS_PER_SEC);

	fclose(in);
	fclose(out);

	free(buf);

	return 0;
}
#endif
