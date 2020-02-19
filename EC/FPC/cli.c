/*
 *  Copyright (C) 2017, Konstantinos Agiannis
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "fpc.h"

//#define NDEBUG
#define MAGIC_NUM 0xf1f2
#define BLOCK_READ (64 << 20)
#define MIN(A,B) ((A) < (B)?(A):(B))
#define ERROR_READ my_error("ERROR:Can not read from file");
#define ERROR_WRITE my_error("ERROR:Can not write to file");
#define CHECK(expr) if(unlikely(expr))

#if defined(__GNUC__)
#	define INLINE static inline __attribute__ ((always_inline))
#	define likely(x) (__builtin_expect((x) != 0,1))
#	define unlikely(x) (__builtin_expect((x) != 0,0))
#else
#	define INLINE static inline
#	define likely(x) (x)
#	define unlikely(x) (x)
#endif

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;

INLINE
void my_error(const char *s)
{
	if(errno != 0)
		perror(s);
	else
		fprintf(stderr,"%s\n",s);
	exit(EXIT_FAILURE);
}

INLINE
void * my_malloc(size_t size)
{
	void * ptr;
	ptr = malloc(size);

	CHECK(ptr == 0 && size != 0)
		my_error("ERROR:Could not allocate memory\n");

	return ptr;
}

INLINE
void W16_FILE_LE(FILE *out,U16 a)
{
	CHECK(fputc(a,out) == EOF || fputc(a >> 8,out) == EOF)\
		ERROR_WRITE
}

INLINE
U16 R16_FILE_LE(FILE *in)
{
	int tmp1,tmp2 = 0;//silence warning
	CHECK((tmp1 = fgetc(in)) == EOF || (tmp2 = fgetc(in)) == EOF)
		ERROR_READ
	return tmp1 + (tmp2 << 8);
}

//simple adler32 checksum
//not the best just for testing
U32 hash(void *input, size_t len)
{

#define MOD_ADLER 65521
#define MAX_NONMOD 5552

	U32 a = 1,b = 0;
	size_t index;
	unsigned char *data = (unsigned char *)input;
again:
	for(index = 0;index < MIN(len,MAX_NONMOD); index++){
		assert(b <= b+a);
		assert(a <= a + data[index]);
		a = a + data[index];
		b = b + a;
	}
	if(index < len){
		len -= MAX_NONMOD;
		a = a % MOD_ADLER;
		b = b % MOD_ADLER;
		goto again;
	}
	return ((b % MOD_ADLER) << 16) | (a % MOD_ADLER);
}

//format bits
//magic 16
//for every block
// 16 dec size | 16 enc size| enc size bytes
//if enc size == 1 then read a byte and repeate dec size times
//if enc size == dec size then memcpy
//return compressed size on success else 0

//in and out must be open
U64 comp_file(FILE *in,FILE *out,int bsize)
{
	int block = (bsize == 0 ? BLOCK_READ : bsize );
	U8 *output = (U8 *) my_malloc(FPC_MAX_OUTPUT(block,bsize)),*input = (U8 *) my_malloc(FPC_MAX_OUTPUT(block,bsize));
	U64 res = 4;
	U32 a,magic = MAGIC_NUM;

	W16_FILE_LE(out,magic);
	while ((a = fread(input,1,block,in)) > 0){
		U32 c = FPC_compress(output,input,a,bsize);
		CHECK(fwrite(output,1,c,out) != c)
			ERROR_WRITE
		res += (U64)c;
	}
	a = 0;
	fwrite(&a,2,1,out);
	free(input);
	free(output);
	return res;
}

//return decompressed size
U64 dec_file(FILE *in,FILE *out)
{
	U8 output[1<<16],input[1 << 16];
	U64 res = 0;
	U32 a = 0,c;

	a = R16_FILE_LE(in);//LE
	CHECK(a != MAGIC_NUM)
		my_error("ERROR:File not compressed");

	while((a = R16_FILE_LE(in)) != 0){
		c = R16_FILE_LE(in);
		CHECK(fread(input,1,c,in) != c)
			my_error("ERROR:File corrupted");
		CHECK(FPC_decompress_block(output,a,input,c,256) == -1)
			my_error("ERROR:File corrupted");
		res += (U64)a;
		fwrite(output,1,a,out);
	}
	return res;
}

void bench_file(FILE *in,U32 chunk_size,int bsize)
{
	U64 csize = 0,size = 0,a;
	size_t max_out = FPC_MAX_OUTPUT(chunk_size,bsize);
	char *output = (char *)my_malloc(max_out);//TODO
	char *input = (char *)my_malloc(chunk_size);
	clock_t t0,t1,t2,t3,t4,compt = 0,dect = 0;

	//bench
	while ((a = fread(input,1,chunk_size,in)) > 0){
		U32 h1 = hash(input,a);

		t0 = clock();
		FPC_compress(output,input,a,bsize);
		t1 = clock();
		FPC_compress(output,input,a,bsize);
		t2 = clock();
		FPC_compress(output,input,a,bsize);
		t3 = clock();
		size_t tmp = FPC_compress(output,input,a,bsize);
		t4 = clock();

		compt += MIN(MIN(MIN(t4-t3,t3-t2),t2-t1),t1-t0);

		t0 = clock();
		FPC_decompress(input,a,output,tmp);
		t1 = clock();
		FPC_decompress(input,a,output,tmp);
		t2 = clock();
		FPC_decompress(input,a,output,tmp);
		t3 = clock();
		size_t b = FPC_decompress(input,a,output,tmp);
		t4 = clock();
		dect += MIN(MIN(MIN(t4-t3,t3-t2),t2-t1),t1-t0);

		U32 h2 = hash(input,b);
		CHECK(a != b || h1 != h2)
			my_error("ERROR:Input differs from output");
		csize += tmp;
		size += a;
	}
	printf("%" PRIu64 " -> %" PRIu64 ", %.2lf%% of original,ratio = %.3lf\n"
			"compression speed %.2lf MB/s, decompression speed %.2lf MB/s\n",size,csize,
	((double)csize)/((double)size)*100,((double)size)/((double)csize),
	((double)size)/1024/1024/((double)compt / CLOCKS_PER_SEC),((double)size)/1024/1024/((double)dect / CLOCKS_PER_SEC));

	free(input);
	free(output);
}

void help(char **argv)
{
	//make decompress default
	printf("Fast Prefix Coder by Konstantinos Agiannis\n\n"
			"usage: %s [options] input [output]\n\n"
			"  -B           : benchmark file\n"
			"  -b num       : block size in KB, 1<= num <= 63, 0 for adaptive (default 16)\n"
			//"  -c           : compress\n"
			"  -d           : decompress\n",*argv);
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv)
{
	int bsize = 16 * 1024;
	int count = 1,bench = 0,compress = 1;

	while(count < argc && argv[count][0] == '-'){
		if(argv[count][2] != 0)
			help(argv);
		switch(argv[count][1]){
		case 'B':
			bench = 1;
			break;
		case 'b':
			if(++count < argc)
				bsize = atoi(argv[count]) * 1024;
			else
				help(argv);
			break;
		case 'd':
			compress = 0;
			break;
		default:
			help(argv);
		}
		count++;
	}
	if(count >= argc || bsize < 0 || bsize > 63*1024)
		help(argv);
	FILE *in = fopen(argv[count++],"rb");
	if(in == 0)
		my_error("ERROR:Unable to open input");
	if(bench == 0){
		FILE *out = fopen(argv[count++],"wb");
		if(out == 0)
			my_error("ERROR:Unable to open output");
		if(count != argc)
			help(argv);
		if(compress == 1)
			comp_file(in,out,bsize);
		else
			dec_file(in,out);
		fclose(out);
	}else{
		if(count != argc)
			help(argv);
		bench_file(in,BLOCK_READ,bsize);
	}
	fclose(in);
	return 0;
}
