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

//TODO
//improve header encoding
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <math.h>

//config
//#define NDEBUG
#define NUM_STREAMS 3
#define MAX_BIT_LEN 11
#define MAX_SYM_NUM 256

#define ADAPTIVE_STEP 2048
#define BLOCK_OVERHEAD 100 //assume header used for bit lengths is 100 bytes

#define AMAX_BIT_LEN 14
#define HEADER_SIZE (2*(NUM_STREAMS-1))
#define MAX_HEADER_STAT_SIZE 128
#define MIN_COMPRESSIBLE_SIZE 32

#if UINTPTR_MAX > 0x100000000ULL
#	define ARCH64
#else
#	define ARCH32
#endif

#ifdef ARCH64
#	if MAX_BIT_LEN <= 11
#		define RENORM_NUM 5
#	else
#		define RENORM_NUM 4
#	endif
#else
#	if MAX_BIT_LEN <= 12
#		define RENORM_NUM 2
#	else
#		define RENORM_NUM 1
#	endif
#endif

//special arch optimisations
#if defined(__amd64)
#	define UNROLL_NUM 2
#	define USE_PREFETCH 0
#elif defined(__i386)
#	define UNROLL_NUM 1
#	define USE_PREFETCH 0
#elif defined(__arm__) || defined (__aarch64__)
#	define UNROLL_NUM 2
#	define USE_PREFETCH 1
#else
#	define UNROLL_NUM 1
#	define USE_PREFETCH 0
#endif

//macros
#define MIN(A,B) ((A) < (B)?(A):(B))
#define CHECK(expr) if(unlikely(expr))
#define CAT(a, ...) XCAT(a, __VA_ARGS__)
#define XCAT(a, ...) a##__VA_ARGS__

#define REPEAT_ARG(N,X) CAT(REPA_,N)(X)
#define REPA_0(X)
#define REPA_1(X) X(0)
#define REPA_2(X) REPA_1(X) X(1)
#define REPA_3(X) REPA_2(X) X(2)
#define REPA_4(X) REPA_3(X) X(3)
#define REPA_5(X) REPA_4(X) X(4)

#define REPEAT(N,...) CAT(REP_,N)(__VA_ARGS__)
#define REP_0(...)
#define REP_1(...) __VA_ARGS__
#define REP_2(...) REP_1(__VA_ARGS__) __VA_ARGS__
#define REP_3(...) REP_2(__VA_ARGS__) __VA_ARGS__
#define REP_4(...) REP_3(__VA_ARGS__) __VA_ARGS__
#define REP_5(...) REP_4(__VA_ARGS__) __VA_ARGS__

#define DEC(X) CAT(DEC_,X)
#define DEC_1 0
#define DEC_2 1
#define DEC_3 2
#define DEC_4 3
#define DEC_5 4

//compiler specific stuff

// Compatibility with non-clang compilers
#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif

#if defined(__GNUC__)
#	define INLINE static inline __attribute__ ((always_inline))
#	define likely(x) (__builtin_expect((x) != 0,1))
#	define unlikely(x) (__builtin_expect((x) != 0,0))
#	define PREFETCH(x) __builtin_prefetch(x)
#	if (__GNUC__ * 100 + __GNUC_MINOR__ >= 403) ||\
(defined(__clang__) && __has_builtin(__builtin_bswap32) && __has_builtin(__builtin_bswap64))
#		define BSWAP32(x) __builtin_bswap32(x)
#		define BSWAP64(x) __builtin_bswap64(x)
#	endif
#else
#	define INLINE static inline
#	define likely(x) (x)
#	define unlikely(x) (x)
#	define PREFETCH(x)
#endif

#ifndef BSWAP32
#	define BSWAP32(x)\
		(((x) << 24) & 0xff000000 ) |\
		(((x) <<  8) & 0x00ff0000 ) |\
		(((x) >>  8) & 0x0000ff00 ) |\
		(((x) >> 24) & 0x000000ff )
#	define BSWAP64(x) \
		(((x) << 56) & 0xff00000000000000ULL) |\
		(((x) << 40) & 0x00ff000000000000ULL) |\
		(((x) << 24) & 0x0000ff0000000000ULL) |\
		(((x) << 8)  & 0x000000ff00000000ULL) |\
		(((x) >> 8)  & 0x00000000ff000000ULL) |\
		(((x) >> 24) & 0x0000000000ff0000ULL) |\
		(((x) >> 40) & 0x000000000000ff00ULL) |\
		(((x) >> 56) & 0x00000000000000ffULL)
#endif

//types
#define U16MAX 65535
#define ARCH_SIZE sizeof(size_t)
typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;

typedef struct{
	U8 len,sym;
}Dnode;

typedef struct{
	U16 val,len;
}Enode;

typedef struct{
	U16 freq,sym;
}Fsym;

//memory stuff

//endian detection taken from nemequ psnip
/* GCC (and compilers masquerading as GCC) define  __BYTE_ORDER__. */
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#	define MEM_LITTLE_ENDIAN
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#	define MEM_BIG_ENDIAN
/* We know the endianness of some common architectures.  Common
 * architectures not listed (ARM, POWER, MIPS, etc.) here are
 * bi-endian. */
#elif defined(__amd64) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#	define MEM_LITTLE_ENDIAN
#elif defined(__s390x__) || defined(__zarch__)
#	define MEM_BIG_ENDIAN
/* Looks like we'll have to rely on the platform.  If we're missing a
 * platform, please let us know. */
#elif defined(_WIN32)
#	define MEM_LITTLE_ENDIAN
#elif defined(sun) || defined(__sun) /* Solaris */
#	include <sys/byteorder.h>
#	if defined(_LITTLE_ENDIAN)
#		define MEM_LITTLE_ENDIAN
#	elif defined(_BIG_ENDIAN)
#		define MEM_BIG_ENDIAN
#	endif
#elif defined(__APPLE__)
#	include <libkern/OSByteOrder.h>
#	if defined(__LITTLE_ENDIAN__)
#		define MEM_LITTLE_ENDIAN
#	elif defined(__BIG_ENDIAN__)
#		define MEM_BIG_ENDIAN
#	endif
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__DragonFly__) || defined(BSD)
#	include <machine/endian.h>
#	if defined(__BYTE_ORDER) && (__BYTE_ORDER == __LITTLE_ENDIAN)
#		define MEM_LITTLE_ENDIAN
#	elif defined(__BYTE_ORDER) && (__BYTE_ORDER == __BIG_ENDIAN)
#		define MEM_BIG_ENDIAN
#	endif
#elif defined(__linux__) || defined(__linux) || defined(__gnu_linux__)
#	include <endian.h>
#	if defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && (__BYTE_ORDER == __LITTLE_ENDIAN)
#		define MEM_LITTLE_ENDIAN
#	elif defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && (__BYTE_ORDER == __BIG_ENDIAN)
#		define MEM_BIG_ENDIAN
#	endif
#endif

#if ! (defined(MEM_LITTLE_ENDIAN) || defined(MEM_BIG_ENDIAN))
#	error "ERROR:Can not detect endian"
#endif

#define LOAD(name,T,F)\
	INLINE T name (const void* ptr){\
		T result;                            \
		memcpy(&result, ptr, sizeof(result));\
		return F(result);                    \
	}
#define WRITE(name,T,F)\
	INLINE void name (void *ptr,T data){\
		data = F(data);                 \
		memcpy(ptr,&data,sizeof(data)); \
	}

LOAD(L16,U16,)
LOAD(L32,U32,)
LOAD(L64,U64,)
LOAD(LARCH,size_t,)
WRITE(W16,U16,)
WRITE(W32,U32,)
WRITE(W64,U64,)
WRITE(WARCH,size_t,)

#ifdef MEM_LITTLE_ENDIAN
#	define L16_LE L16
#	define L32_LE L32
#	define L64_LE L64
#	define LARCH_LE LARCH
#	define W16_LE W16
#	define W32_LE W32
#	define W64_LE W64
#	define WARCH_LE WARCH
#else
INLINE U16 L16_LE(const void* ptr)
{
	U8 *p = (U8 *)ptr;
	return p[0] + (p[1] << 8);
}
INLINE void W16_LE(void *ptr,U16 data)
{
	U8 *p = (U8 *)ptr;
	p[0] = (U8)data;
	p[1] = (U8)(data >> 8);
}
	LOAD(L32_LE,U32,BSWAP32)
	LOAD(L64_LE,U64,BSWAP64)
	WRITE(W32_LE,U32,)
	WRITE(W64_LE,U64,)
#	ifdef ARCH64
		LOAD(LARCH_LE,size_t,BSWAP64)
		WRITE(WARCH_LE,size_t,BSWAP64)
#	else
		LOAD(LARCH_LE,size_t,BSWAP32)
		WRITE(WARCH_LE,size_t,BSWAP32)
#	endif
#endif

/* compute table
#include <stdio.h>
int brev(int num,int len)
{
	int a,tmp0,tmp1;
	for(a = 0;a < len/2;a++){
		tmp0 = (num >> a) & 1;
		tmp1 = (num >> (len - a-1)) & 1;
		num &= ~((1 << a) | (1 << (len-a-1)));
		num |= tmp1 << a;
		num |= tmp0 << (len-a-1);
	}
	return num;
}

int main()
{
	int a;
	for(a = 0;a < 128;a++)
		printf(" %d,",brev(a,7));
}
*/

const U8 trev[128] = {
	0, 64, 32, 96, 16, 80, 48, 112, 8, 72, 40, 104, 24, 88, 56, 120,
	4, 68, 36, 100, 20, 84, 52, 116, 12, 76, 44, 108, 28, 92, 60, 124,
	2, 66, 34, 98, 18, 82, 50, 114, 10, 74, 42, 106, 26, 90, 58, 122,
	6, 70, 38, 102, 22, 86, 54, 118, 14, 78, 46, 110, 30, 94, 62, 126,
	1, 65, 33, 97, 17, 81, 49, 113, 9, 73, 41, 105, 25, 89, 57, 121,
	5, 69, 37, 101, 21, 85, 53, 117, 13, 77, 45, 109, 29, 93, 61, 125,
	3, 67, 35, 99, 19, 83, 51, 115, 11, 75, 43, 107, 27, 91, 59, 123,
	7, 71, 39, 103, 23, 87, 55, 119, 15, 79, 47, 111, 31, 95, 63, 127};

//assume num is 14 bits long
INLINE U32 brev(U32 num)
{
	return ((U32)trev[num >> 7]) + (((U32)trev[num & 127]) << 7);
}

/* compute table
#include <stdio.h>
#include <math.h>
int main(void)
{
	printf("{0");
	for(int a = 0;a < 64;a++){
		for(int b = 0;b < 16;b++){
			if(a == 0 && b == 0)
				continue;
			if(b != 0)
				printf(", ");
			else
				printf(",\n");
			printf("%.0lf",round((16*log2(a*16+b))));
		}
	}
	printf("};\n");
	return 0;
}
*/
const U8 lookup_log2[1024] =
{0, 0, 16, 25, 32, 37, 41, 45, 48, 51, 53, 55, 57, 59, 61, 63,
64, 65, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 79,
80, 81, 81, 82, 83, 83, 84, 85, 85, 86, 86, 87, 87, 88, 88, 89,
89, 90, 90, 91, 91, 92, 92, 93, 93, 93, 94, 94, 95, 95, 95, 96,
96, 96, 97, 97, 97, 98, 98, 98, 99, 99, 99, 100, 100, 100, 101, 101,
101, 101, 102, 102, 102, 103, 103, 103, 103, 104, 104, 104, 104, 105, 105, 105,
105, 106, 106, 106, 106, 107, 107, 107, 107, 107, 108, 108, 108, 108, 109, 109,
109, 109, 109, 110, 110, 110, 110, 110, 111, 111, 111, 111, 111, 111, 112, 112,
112, 112, 112, 113, 113, 113, 113, 113, 113, 114, 114, 114, 114, 114, 114, 115,
115, 115, 115, 115, 115, 116, 116, 116, 116, 116, 116, 116, 117, 117, 117, 117,
117, 117, 117, 118, 118, 118, 118, 118, 118, 118, 119, 119, 119, 119, 119, 119,
119, 119, 120, 120, 120, 120, 120, 120, 120, 121, 121, 121, 121, 121, 121, 121,
121, 121, 122, 122, 122, 122, 122, 122, 122, 122, 123, 123, 123, 123, 123, 123,
123, 123, 123, 124, 124, 124, 124, 124, 124, 124, 124, 124, 125, 125, 125, 125,
125, 125, 125, 125, 125, 125, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 128, 128, 128, 128, 128,
128, 128, 128, 128, 128, 128, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129,
129, 129, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 131, 131,
131, 131, 131, 131, 131, 131, 131, 131, 131, 131, 132, 132, 132, 132, 132, 132,
132, 132, 132, 132, 132, 132, 132, 132, 133, 133, 133, 133, 133, 133, 133, 133,
133, 133, 133, 133, 133, 134, 134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
134, 134, 134, 134, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135,
135, 135, 135, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
136, 136, 137, 137, 137, 137, 137, 137, 137, 137, 137, 137, 137, 137, 137, 137,
137, 137, 137, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138,
138, 138, 138, 138, 139, 139, 139, 139, 139, 139, 139, 139, 139, 139, 139, 139,
139, 139, 139, 139, 139, 139, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140,
140, 140, 140, 140, 140, 140, 140, 140, 141, 141, 141, 141, 141, 141, 141, 141,
141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 141, 142, 142, 142, 142,
142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142, 142,
143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143, 143,
143, 143, 143, 143, 143, 143, 144, 144, 144, 144, 144, 144, 144, 144, 144, 144,
144, 144, 144, 144, 144, 144, 144, 144, 144, 144, 144, 144, 145, 145, 145, 145,
145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145, 145,
145, 145, 145, 146, 146, 146, 146, 146, 146, 146, 146, 146, 146, 146, 146, 146,
146, 146, 146, 146, 146, 146, 146, 146, 146, 146, 146, 147, 147, 147, 147, 147,
147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147, 147,
147, 147, 147, 147, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148,
148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 148, 149,
149, 149, 149, 149, 149, 149, 149, 149, 149, 149, 149, 149, 149, 149, 149, 149,
149, 149, 149, 149, 149, 149, 149, 149, 149, 149, 150, 150, 150, 150, 150, 150,
150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
150, 150, 150, 150, 150, 150, 150, 151, 151, 151, 151, 151, 151, 151, 151, 151,
151, 151, 151, 151, 151, 151, 151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
151, 151, 151, 151, 151, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152, 152,
152, 152, 152, 152, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154,
154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154,
154, 154, 154, 154, 154, 154, 154, 155, 155, 155, 155, 155, 155, 155, 155, 155,
155, 155, 155, 155, 155, 155, 155, 155, 155, 155, 155, 155, 155, 155, 155, 155,
155, 155, 155, 155, 155, 155, 155, 155, 155, 155, 155, 156, 156, 156, 156, 156,
156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156,
156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156, 156,
157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
157, 157, 157, 157, 157, 157, 157, 158, 158, 158, 158, 158, 158, 158, 158, 158,
158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158,
159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159,
159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159,
159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 159, 160, 160, 160, 160, 160,
160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160};

INLINE int log_int(int a)
{
	return (a >= 1024? (16-10)*16 + lookup_log2[a >> 6]:lookup_log2[a]);
}

//0 <= symbols <= sym_num-1 < 256
void byte_count(U8 *s,int len,U32 *res,int sym_num)
{
	U32 a,b1[3*MAX_SYM_NUM]={0};//avoid 3 memsets
	U32 *b2 = b1 + MAX_SYM_NUM,*b3 = b2 + MAX_SYM_NUM;
	U8 tmp0,tmp1,tmp2,tmp3;

	for(a = 0;a < (len & (~7));a += 8){
		tmp0 = s[a];
		tmp1 = s[a+1];
		tmp2 = s[a+2];
		tmp3 = s[a+3];
		res[tmp0]++;
		b1[tmp1]++;
		b2[tmp2]++;
		b3[tmp3]++;

		tmp0 = s[a+4];
		tmp1 = s[a+5];
		tmp2 = s[a+6];
		tmp3 = s[a+7];
		res[tmp0]++;
		b1[tmp1]++;
		b2[tmp2]++;
		b3[tmp3]++;
	}
	for(;a < len;)
		res[s[a++]]++;
	for(a = 0;a < sym_num;a++)
		res[a] += b1[a] + b2[a] + b3[a];
}

//freq should be strictly less than 2^16
void sort_inc(Fsym *input,int num)
{
	U32 a,b,c,sum0,sum1,f0[256] = {0},f1[256] = {0};
	Fsym tmp[MAX_SYM_NUM],s;
	for(a = 0;a < num;a++){
		b = input[a].freq;
		f0[b & 255]++;
		f1[b >> 8]++;
	}
	sum0 = f0[0];
	f0[0] = 0;
	sum1 = f1[0];
	f1[0] = 0;
	for(a = 1;a <= 255;a++){
		b = f0[a];
		c = f1[a];
		f0[a] = sum0;
		f1[a] = sum1;
		sum0 += b;
		sum1 += c;
	}
	//sort
	for(a = 0;a < num;a++){
		s = input[a];
		b = s.freq & 255;
		tmp[f0[b]] = s;
		f0[b]++;
	}
	for(a = 0;a < num;a++){
		s = tmp[a];
		b = s.freq >> 8;
		input[f1[b]] = s;
		f1[b]++;
	}
}
//on success return 0
int construct_dec_table(U8 *header_len,Dnode *lookup,int sym_num)
{
	U32 a,b,prev_cum = 0,prev_num = 0,d,base;
	U32 count_bit[AMAX_BIT_LEN+1] = {0};
	U16 tmp;

#ifndef NDEBUG
	for(a = 0,b = 0;a < sym_num;a++){
		if(header_len[a] == 0)
			continue;
		b += 1 << (MAX_BIT_LEN - header_len[a]);
	}
	assert(b == (1 << MAX_BIT_LEN));
#endif

	//count sort
	for(a = 0;a < sym_num;a++){
		count_bit[header_len[a]]++;
		assert(header_len[a] <= MAX_BIT_LEN);
	}
	for(a = MAX_BIT_LEN;a != 0;a--){
		prev_cum += prev_num;
		prev_num = count_bit[a] << (AMAX_BIT_LEN - a);
		count_bit[a] = prev_cum;
	}

	CHECK(count_bit[1] + prev_num != (1 << AMAX_BIT_LEN))
		return 1;

	//place
	//TODO it needs a check
	for(a = 0;a < sym_num;a++){
		b = header_len[a];
		if(b == 0)
			continue;
		d = 1 << b;
		//we combine a(symbol) and b(len) into one U16 to avoid 2 stores
		#ifdef MEM_LITTLE_ENDIAN
			tmp = (a << 8) | b;
		#else
			tmp = (b << 8) | a;
		#endif
		base = brev(count_bit[b]);
		count_bit[b] += 1 << (AMAX_BIT_LEN - b);

		if(d < (1 << MAX_BIT_LEN)){
			for(;base < (1 << MAX_BIT_LEN);base += d){
				*(((U16 *)lookup) + base) = tmp;
				base += d;
				*(((U16 *)lookup) + base) = tmp;
			}
		}
		else
		//The for loop is not needed because it iterates only once
			//for(;base < (1 << MAX_BIT_LEN);base += d) 
				*(((U16 *)lookup) + base) = tmp;
	}
	return 0;
}

//assumes it does not contain non existant symbols
//assume sort not stable
void construct_enc_table(Enode *lookup,Fsym *s,int num)
{
	U32 a,count[AMAX_BIT_LEN+1] = {0},prev_num = 0,prev_cum = 0;
	for(a = 0;a < num;a++){
		count[s[a].freq]++;
		lookup[s[a].sym].len = s[a].freq;
	}
	for(a = MAX_BIT_LEN;a != 0;a--){
		prev_cum += prev_num;
		prev_num = count[a] << (AMAX_BIT_LEN - a);
		count[a] = prev_cum;
	}
	for(a = 0;a < num;a++){
		if(lookup[a].len == 0)
			continue;
		lookup[a].val = brev(count[lookup[a].len]);
		count[lookup[a].len] += 1 << (AMAX_BIT_LEN - lookup[a].len);
	}
}

//WARNING!!!:input must have 1 additional free space not counted in num
void build_prefix_codes(Fsym *input,int num)
{
	//packages[L] conctains elements of L in packages
	int packages[2][MAX_SYM_NUM+1],L,leaf_pos,package_pos,package_num = 0;
	int tmp,len,a,b,pnum;
	U8 M[MAX_BIT_LEN][2*MAX_SYM_NUM];//number of leafs up to there - 1
	input[num].freq = U16MAX;

	//L = 0
	package_num = num / 2;
	for(a = 0;a < (package_num << 1);a++)
		M[0][a] = a;
	for(a = 0,b = 0;a < package_num;a++,b += 2)
		packages[0][a] = input[b].freq + input[b+1].freq;

	for(L = 1;L < MAX_BIT_LEN;L++){
		len = (package_num + num) / 2;
		packages[(L-1)%2][package_num] = U16MAX;
		leaf_pos = package_pos = 0;
		//M[L][0] is always a leaf
		for(pnum = 0,a = 0;pnum < len;pnum++,a++){
			assert(package_pos <= package_num);
			if(input[leaf_pos].freq < packages[(L-1)%2][package_pos]){
				tmp = input[leaf_pos].freq;
				M[L][a] = leaf_pos++;
			}else{
				tmp = packages[(L-1)%2][package_pos];
				package_pos++;
				M[L][a] = leaf_pos-1;
			}
			a++;
			assert(package_pos <= package_num);
			if(input[leaf_pos].freq < packages[(L-1)%2][package_pos]){
				packages[L%2][pnum] = tmp + input[leaf_pos].freq;
				M[L][a] = leaf_pos++;
			}else{
				packages[L%2][pnum] = tmp + packages[(L-1)%2][package_pos];
				package_pos++;
				M[L][a] = leaf_pos-1;
			}
		}
		package_num = pnum;
	}

	for(a = 0;a < num;a++)
		input[a].freq = 0;

	//calculate result,take 2*n-2 first
	//bit lenght is the number of leaf occurences for each symbol
	len = 2*num-2;
	for(a = L-1;a >= 0 && len > 0;a--){
		leaf_pos = M[a][len-1];
		input[leaf_pos].freq++;
		//printf("len = %d,leaf_pos = %d\n",len,leaf_pos);
		len = 2*(len -1 -leaf_pos );
	}
	for(b = input[num-1].freq,a = num-2;a >= 0;a--){
		b += input[a].freq;
		input[a].freq = b;
	}
}

U8 *byte_pos,*init_pos,*byte_end,c;
U32 nibble_count;

INLINE void init_nibble(U8 *pos,U8 *end)
{
	byte_pos = pos;
	init_pos = pos;
	byte_end = end;
	nibble_count = 0;
}

INLINE U8 get_nibble()
{
	//branchless
	/*byte_pos += nibble_count%2;
	int res = ((*byte_pos)>>((nibble_count%2)<<3))&15;
	nibble_count++;
	return res;*/
	//with branch
	if(nibble_count++%2 == 0){
		CHECK(byte_pos >= byte_end)
			return 100;
		c = *byte_pos++;
		return c & 15;
	}else{
		return c >> 4;
	}
}

INLINE void put_nibble(U8 n)
{
	if(nibble_count++%2 == 0){
		c = n;
	}else{
		c |= n << 4;
		*byte_pos++ = (U8)c;
	}
}

//returns number of bytes writen
INLINE U32 flush_nibbles()
{
	if(nibble_count%2 == 1)
		*byte_pos++ = c;
	return byte_pos - init_pos;
}

INLINE U32 get_input_nibbles()
{
	return byte_pos - init_pos;
}

//return bytes written
U32 write_prefix_descr(Enode *lookup,U8 *res,int sym_num)
{
	U32 previous,count,a;
	init_nibble(res,0);
	for(a = 0;a < sym_num;a++){
		count = 1;
		previous = lookup[a].len;

		while(a+1 < sym_num && previous == lookup[a+1].len)
			count++,a++;

		if(count == 1){
			put_nibble(previous);
		}else if(count == 2){
			put_nibble(previous);
			put_nibble(previous);
		}else{
			if(previous == 0 && count == a+1)
				count++;
			else
				put_nibble(previous);
			if(count <= 16-MAX_BIT_LEN)
				put_nibble(MAX_BIT_LEN+count-2);
			else{
				put_nibble(15);
				count -= 17-MAX_BIT_LEN;
				while(count >= 15){
					put_nibble(15);
					count -= 15;
				}
				put_nibble(count);
			}
		}
	}
	return flush_nibbles();
}

//on error return 0
U32 read_prefix_descr(U8 *len,U8 *in,U8 *end,int sym_num)
{
	int bl,previous = 0,a = 0,c;
	init_nibble(in,end);
	while(a < sym_num){
		bl = get_nibble();
		CHECK(bl == 100)
			return 0;
		if(bl <= MAX_BIT_LEN){
			previous = bl;
			len[a++] = bl;
		}else if(bl < 15){
			c = 1+bl-MAX_BIT_LEN;
			CHECK(a + c > sym_num)
				return 0;
			while(c-- > 0)
				len[a++] = previous;
		}else{
			c = 16-MAX_BIT_LEN;
			CHECK(a + c > sym_num)
				return 0;
			while(c-- > 0)
				len[a++] = previous;
			do{
				c = bl = get_nibble();
				CHECK(c == 100 || a + c > sym_num)
					return 0;
				while(c-->0)
					len[a++] = previous;
			}while(bl == 15);
		}
	}
	return get_input_nibbles();
}

INLINE void write_header(U16 *pos,U32 *stream_size)
{
	for(U32 a = 0;a < NUM_STREAMS-1;a++)
		W16_LE(pos+a,stream_size[a]);//missaligned LE
}

//1 stream a time
//dest should have some 8 more free bytes
INLINE int prefix_codes_encode(U8 *dest,U8 *src,int sym_num,const Enode *lookup)
{
	U8 *src_end = src + sym_num - (sym_num%(RENORM_NUM*NUM_STREAMS));
	U8 *dest_start = dest,sym,bl;
	U32 bits_av = 0,tmp;
	size_t bits = 0,code;

	while(src < src_end){
		REPEAT(RENORM_NUM,
			sym = *src;
			code = lookup[sym].val;
			bl = lookup[sym].len;
			src += NUM_STREAMS;
			bits |= code << bits_av;
			bits_av += bl;
		)
		WARCH_LE(dest,bits);
		tmp = bits_av >> 3;
		bits_av &= 7;
		bits >>= tmp << 3;
		dest += tmp;
	}

	//at most RENORM_NUM-1 times
	src_end += sym_num %(RENORM_NUM * NUM_STREAMS);
	while(src < src_end){
		sym = *src;
		src += NUM_STREAMS;
		code = lookup[sym].val;
		bl = lookup[sym].len;
		bits |= code << bits_av;
		bits_av += bl;
	}
	//renormalise
	WARCH_LE(dest,bits);
	dest += (bits_av+7) >> 3;

	return dest - dest_start;
}


#ifdef ARCH64
#	define SUB_CONST 63
#	define OR_CONST 56
#else
#	define SUB_CONST 31
#	define OR_CONST 24
#endif

#define PREFETCH_STREAM(A)\
	PREFETCH(stream_pos##A+320);

#define RENORM_DEC(A){\
	bits##A |= LARCH_LE(stream_pos##A) << bits_av##A;\
	stream_pos##A += (SUB_CONST - bits_av##A) >> 3;\
	bits_av##A |= OR_CONST;\
}

#define RENORM_DEC_END(A)\
	if(bits_av##A < MAX_BIT_LEN){\
		int dist;\
		if((dist = stream_end - stream_pos##A) > 1){\
			bits##A |= ((size_t)L16_LE(stream_pos##A)) << bits_av##A;\
			stream_pos##A += 2;\
			bits_av##A += 16;\
		}else if (dist > 0){\
			bits##A |= ((size_t)*stream_pos##A) << bits_av##A;\
			stream_pos##A ++;\
			bits_av##A += 8;\
		}\
	}

#define DEC_INIT(A){\
	stream_pos##A = other;\
	other += L16_LE(src);\
	src += 2;\
}

#define PREFIX_DEC(A){\
	code = bits##A & ((1 << MAX_BIT_LEN)-1);\
	bl = lookup[code].len;\
	bits##A >>= bl;\
	bits_av##A -= bl;\
	*dest++ = lookup[code].sym;\
}

#define PREFIX_DEC_END(A)\
	if(dest >= dest_end)break;\
	code = bits##A & ((1 << MAX_BIT_LEN)-1);\
	bl = lookup[code].len;\
	bits##A >>= bl;\
	bits_av##A -= bl;\
	*dest++ = lookup[code].sym;

#define DEC_DECLARE(A)\
	U32 bits_av##A = 0;\
	size_t bits##A = 0;\
	U8 *stream_pos##A;

#define TEST_STREAM_END(A)\
	 && stream_pos##A <= stream_end

//on success return 0
int prefix_codes_decode(U8 *dest,int dest_size,U8 *src,int src_size,const Dnode *lookup)
{
	REPEAT_ARG(NUM_STREAMS,DEC_DECLARE);
	U32 code,bl;
	U8 *dest_end = dest + dest_size - (dest_size%(RENORM_NUM * NUM_STREAMS));
	U8 *other = src + HEADER_SIZE;
	U8 *stream_end = src + src_size;
	//src_size -= HEADER_SIZE;

	CHECK(src + HEADER_SIZE > stream_end)
		return 1;

	stream_end -= UNROLL_NUM * ARCH_SIZE;

	REPEAT_ARG(DEC(NUM_STREAMS),DEC_INIT)
	CAT(stream_pos,DEC(NUM_STREAMS)) = other;

	while(likely(dest < dest_end REPEAT_ARG(NUM_STREAMS,TEST_STREAM_END))){

#if USE_PREFETCH == 1
		REPEAT_ARG(NUM_STREAMS,PREFETCH_STREAM)
#endif

		REPEAT(UNROLL_NUM,// We unroll loop
			//renormalise
			REPEAT_ARG(NUM_STREAMS,RENORM_DEC)

			//dec
			REPEAT(RENORM_NUM,
				REPEAT_ARG(NUM_STREAMS,PREFIX_DEC))
		)
	}
	//finalise
	stream_end += UNROLL_NUM * ARCH_SIZE;

	dest_end += dest_size%(RENORM_NUM * NUM_STREAMS);
	while(1){
		REPEAT_ARG(NUM_STREAMS,RENORM_DEC_END);
		REPEAT_ARG(NUM_STREAMS,PREFIX_DEC_END);
	}
	return 0;
}

//encode bytes,return bytes written
//size < 64Kb
int FPC_compress_block(void *output,const void *in,int size,int sym_num)
{
	U32 a,b,count[MAX_SYM_NUM] = {0},stream_size[NUM_STREAMS],compressed_size;
	U8 *out_start = (U8 *)output,*header_start,*out = (U8 *)output;
	Fsym s[MAX_SYM_NUM+1];
	Enode lookup[MAX_SYM_NUM];

	if(size < MIN_COMPRESSIBLE_SIZE)
		goto no_comp;

	byte_count((U8 *)in,size,count,256);

	for(a = 0;a < sym_num;a++)
		s[a] = (Fsym){(U16)count[a],(U16)a};

	sort_inc(s,sym_num);

	if(s[sym_num - 1].freq == size){
		*out = s[sym_num -1].sym;
		return 1;
	}

	//cut 0 freq
	for(a = 0;a < sym_num && s[a].freq == 0;a++);
	assert(sym_num - a != 1);

	build_prefix_codes(s+a,sym_num - a);

	//fast path for uncompressed
	//printf("bit_len = %d,%d,%d\n",s[sym_num-1].freq,s[sym_num-2].freq,s[sym_num-3].freq);
	if(sym_num >= 2 && s[sym_num-1].freq >= 7 && s[sym_num-2].freq == 8)
		goto no_comp;

	construct_enc_table(lookup,s,sym_num);

	//U32 t[17] = {0};
	//for(a = 0;a < 256;a++)t[lookup[a].len]++;//debug
	//for(a = 0;a <= 12;a++)printf("len %d = %.3lf %% \n",a,100*((double)t[a])/256);

	out += write_prefix_descr(lookup,out,sym_num);
	header_start = out;
	out += HEADER_SIZE;

	for(a = 0;a < NUM_STREAMS;a++){
		b = prefix_codes_encode(out,((U8 *)in)+a,size-a,lookup);
		stream_size[a] = b;
		out += b;
	}

	compressed_size = (U32 )(out - out_start);
	if(compressed_size >= size){
no_comp:
		memcpy(output,in,size);
		return size;
	}

	write_header((U16 *)header_start,stream_size);

	return compressed_size;
}

//return uncompressed bytes
//on error return -1
int FPC_decompress_block(void * output,int out_size,const void *input,int in_size,int sym_num)
{
	if(in_size == 1){//RLE
		memset(output,*((char*)input),out_size);
		return in_size;
	}

	if(in_size == out_size){//uncompressed
		memcpy(output,input,in_size);
		return in_size;
	}

	Dnode lookup[1 << MAX_BIT_LEN];
	U8 *in = (U8 *)input,*out = (U8 *)output;
	U8 bit_len[MAX_SYM_NUM];
	U32 bit_descr_size;

	bit_descr_size = read_prefix_descr(bit_len,in,in + in_size,sym_num);
	CHECK(bit_descr_size == 0)
		return -1;

	CHECK(construct_dec_table(bit_len,lookup,sym_num) != 0)
		return -1;

	//decode
	CHECK(prefix_codes_decode(out,out_size,in + bit_descr_size,in_size - bit_descr_size,lookup))
		return -1;
	return in_size;
}

INLINE size_t block_encode(void *output,void *input,int bsize)
{
	W16_LE(output,bsize);//LE
	size_t tmp = FPC_compress_block(((char *)output) + 4,input,bsize,256);
	W16_LE(((char*)output) + 2,tmp);//LE
	return 4 + tmp;
}

size_t comp_adaptive(void * output,void * input,size_t inlen)
{

#define STEP ADAPTIVE_STEP
#define ADAPT_MOD ((1<<16)/STEP)
#define MBLOCK (((1 << 16)-1)/STEP)
#define LOG2(A) log_int(A)
//#define LOG2(A) (A == 0 ? 0 : round(16*log2(A)))
//todo int <-> size_t

	int Cfreq[ADAPT_MOD][256];
	int dp[ADAPT_MOD];
	U8 *block_size = (U8 *) malloc((inlen/STEP)+1);
	U8 *in = (U8 *)input,*out = (U8 *)output,*out_start = (U8 *) output;

	CHECK(block_size == 0)
		return 0;//ERROR

	//init
	int block_end = inlen / STEP;
	for(int a = 0;a < 256;a++){
		dp[block_end%ADAPT_MOD] = 0;
		Cfreq[block_end%ADAPT_MOD][a] = 0;
	}

	//compute block sizes
	int b = inlen-1;
	for(int a = inlen - STEP;a >= 0;a -= STEP){
		int cur = (a / STEP) % ADAPT_MOD;
		int prev = (cur + 1) % ADAPT_MOD;

		memcpy(Cfreq[cur],Cfreq[prev],256*sizeof(int));
		for(;b >= a;b--)
			Cfreq[cur][in[b]]++;

		int best = INT_MAX,bsize = 0;
		for(int c = 1;c <= MBLOCK && (c*STEP) <= inlen - a;c++){
			//bits = -sum(ni*log2(ni) + total*log2(total))
			int res = 0;
			int next = (cur+c)%ADAPT_MOD;
			for(int d = 0;d < 256;d += 4){ //we unroll
				int n0 = Cfreq[cur][ d ] - Cfreq[next][d];
				int n1 = Cfreq[cur][d+1] - Cfreq[next][d+1];
				int n2 = Cfreq[cur][d+2] - Cfreq[next][d+2];
				int n3 = Cfreq[cur][d+3] - Cfreq[next][d+3];
				res -= n0*LOG2(n0) + n1*LOG2(n1) + n2*LOG2(n2) + n3*LOG2(n3);
			}
			res += c*STEP*LOG2(c*STEP);
			if(res == 0)
				res = 5;
			else if(res < 16*c*STEP)
				res = (c*STEP)/8 + BLOCK_OVERHEAD;
			else
				res = (res/16)/8 + BLOCK_OVERHEAD;
			if(res >= c*STEP)
				res = c*STEP + 4;
			//printf("pos = %d,bpb = %.2f,bsize = %d,est = %d,real = %d\n",a,8.0*((double)res)/(c*STEP),c*STEP/1024,res,4+prefix_encode(out,in + a,c*STEP,256));
			res += dp[(cur + c)%ADAPT_MOD];
			//res = 4 + prefix_encode(out,in + a,c*STEP,256) + dp[(cur + c)%ADAPT_MOD];
			if(res <= best){
				best = res;
				bsize = c;
			}
		}
		assert(bsize != 0);
		block_size[a / STEP] = bsize;
		dp[cur] = best;
	}
	//now encode using block sizes
	int a = 0;
	out += block_encode(out,in,(inlen % STEP) + (block_size[0] * STEP));
	in += block_size[0] * STEP;
	a = block_size[0];

	in += inlen % STEP;
	for(;a < block_end;a += block_size[a]){
		//printf("bsize = %d\n", block_size[a]*STEP/1024);
		out += block_encode(out,in,block_size[a] * STEP);
		in += block_size[a] * STEP;
	}
	free(block_size);
	return out - out_start;
}

//return compressed size
//bsize < 64KB
//if bsize == 0 then adaptive
size_t FPC_compress(void * output,void * input,size_t inlen,int bsize)
{
	if(bsize == 0)
		return comp_adaptive(output,input,inlen);
	char *in = (char *) input,*out = (char *)output,*out_start = (char *)output;
	while(inlen > 0){
		U32 step = MIN(inlen,bsize);
		out += block_encode(out,in,step);
		in += step;
		inlen -= step;
	}
	return (size_t)(out - out_start);
}

size_t FPC_decompress(void * output,size_t max_output,void * input,size_t inlen)
{
	char *in = (char *)input,*out_start = (char *)output,*out = (char *)output;
	char *out_end = out + max_output;
	while(out < out_end && inlen >= 4){
		U32 d = L16_LE(in);//LE
		U32 e = L16_LE(in+2);//LE
		in += 4;
		inlen -= 4;
		CHECK(e > inlen || out + d > out_end)
			return 0;
		CHECK(FPC_decompress_block(out,d,in,e,256) == -1)
#ifdef TEST_CORRUPT
			;
#else
			return 0;
#endif
		out += d;
		in += e;
		inlen -= e;
	}
	CHECK(inlen != 0)
		return 0;
	return (size_t)(out - out_start);
}
