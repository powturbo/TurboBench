/*
 * AVX2 edition using 32 RANS states.  This uses a shared pointer for the
 * compressed buffer.
 *
 * TODO: implement SIMD version of the order-1 encoder (decoder is done).
 */

#include <x86intrin.h>

#define NX 32

/*-------------------------------------------------------------------------- */
/* rans_byte.h from https://github.com/rygorous/ryg_rans */

// Simple byte-aligned rANS encoder/decoder - public domain - Fabian 'ryg' Giesen 2014
//
// Not intended to be "industrial strength"; just meant to illustrate the general
// idea.

#ifndef RANS_BYTE_HEADER
#define RANS_BYTE_HEADER

#include <stdio.h>
#include <stdint.h>

#include <assert.h>

#ifdef assert
#define RansAssert assert
#else
#define RansAssert(x)
#endif

// READ ME FIRST:
//
// This is designed like a typical arithmetic coder API, but there's three
// twists you absolutely should be aware of before you start hacking:
//
// 1. You need to encode data in *reverse* - last symbol first. rANS works
//    like a stack: last in, first out.
// 2. Likewise, the encoder outputs bytes *in reverse* - that is, you give
//    it a pointer to the *end* of your buffer (exclusive), and it will
//    slowly move towards the beginning as more bytes are emitted.
// 3. Unlike basically any other entropy coder implementation you might
//    have used, you can interleave data from multiple independent rANS
//    encoders into the same bytestream without any extra signaling;
//    you can also just write some bytes by yourself in the middle if
//    you want to. This is in addition to the usual arithmetic encoder
//    property of being able to switch models on the fly. Writing raw
//    bytes can be useful when you have some data that you know is
//    incompressible, and is cheaper than going through the rANS encode
//    function. Using multiple rANS coders on the same byte stream wastes
//    a few bytes compared to using just one, but execution of two
//    independent encoders can happen in parallel on superscalar and
//    Out-of-Order CPUs, so this can be *much* faster in tight decoding
//    loops.
//
//    This is why all the rANS functions take the write pointer as an
//    argument instead of just storing it in some context struct.

// --------------------------------------------------------------------------

// L ('l' in the paper) is the lower bound of our normalization interval.
// Between this and our byte-aligned emission, we use 31 (not 32!) bits.
// This is done intentionally because exact reciprocals for 31-bit uints
// fit in 32-bit uints: this permits some optimizations during encoding.
#define RANS_BYTE_L (1u << 15)  // lower bound of our normalization interval

// State for a rANS encoder. Yep, that's all there is to it.
typedef uint32_t RansState;

// Initialize a rANS encoder.
static inline void RansEncInit(RansState* r)
{
    *r = RANS_BYTE_L;
}

// Renormalize the encoder. Internal function.
static inline RansState RansEncRenorm(RansState x, uint8_t** pptr, uint32_t freq, uint32_t scale_bits)
{
    uint32_t x_max = ((RANS_BYTE_L >> scale_bits) << 8) * freq -1; // this turns into a shift.
    if (x > x_max) {
        uint8_t* ptr = *pptr;
        do {
            *--ptr = (uint8_t) (x & 0xff);
            x >>= 8;
        } while (x > x_max);
        *pptr = ptr;
    }
    return x;
}

// Encodes a single symbol with range start "start" and frequency "freq".
// All frequencies are assumed to sum to "1 << scale_bits", and the
// resulting bytes get written to ptr (which is updated).
//
// NOTE: With rANS, you need to encode symbols in *reverse order*, i.e. from
// beginning to end! Likewise, the output bytestream is written *backwards*:
// ptr starts pointing at the end of the output buffer and keeps decrementing.
static inline void RansEncPut(RansState* r, uint8_t** pptr, uint32_t start, uint32_t freq, uint32_t scale_bits)
{
    // renormalize
    RansState x = RansEncRenorm(*r, pptr, freq, scale_bits);

    // x = C(s,x)
    *r = ((x / freq) << scale_bits) + (x % freq) + start;
}

// Flushes the rANS encoder.
static inline void RansEncFlush(RansState* r, uint8_t** pptr)
{
    uint32_t x = *r;
    uint8_t* ptr = *pptr;

    ptr -= 4;
    ptr[0] = (uint8_t) (x >> 0);
    ptr[1] = (uint8_t) (x >> 8);
    ptr[2] = (uint8_t) (x >> 16);
    ptr[3] = (uint8_t) (x >> 24);

    *pptr = ptr;
}

// Initializes a rANS decoder.
// Unlike the encoder, the decoder works forwards as you'd expect.
static inline void RansDecInit(RansState* r, uint8_t** pptr)
{
    uint32_t x;
    uint8_t* ptr = *pptr;

    x  = ptr[0] << 0;
    x |= ptr[1] << 8;
    x |= ptr[2] << 16;
    x |= ptr[3] << 24;
    ptr += 4;

    *pptr = ptr;
    *r = x;
}

// Returns the current cumulative frequency (map it to a symbol yourself!)
static inline uint32_t RansDecGet(RansState* r, uint32_t scale_bits)
{
    return *r & ((1u << scale_bits) - 1);
}

// Advances in the bit stream by "popping" a single symbol with range start
// "start" and frequency "freq". All frequencies are assumed to sum to "1 << scale_bits",
// and the resulting bytes get written to ptr (which is updated).
static inline void RansDecAdvance(RansState* r, uint8_t** pptr, uint32_t start, uint32_t freq, uint32_t scale_bits)
{
    uint32_t mask = (1u << scale_bits) - 1;

    // s, x = D(x)
    uint32_t x = *r;
    x = freq * (x >> scale_bits) + (x & mask) - start;

    // renormalize
    if (x < RANS_BYTE_L) {
        uint8_t* ptr = *pptr;
        do x = (x << 8) | *ptr++; while (x < RANS_BYTE_L);
        *pptr = ptr;
    }

    *r = x;
}

// --------------------------------------------------------------------------

// That's all you need for a full encoder; below here are some utility
// functions with extra convenience or optimizations.

// Encoder symbol description
// This (admittedly odd) selection of parameters was chosen to make
// RansEncPutSymbol as cheap as possible.
typedef struct {
    uint32_t x_max;     // (Exclusive) upper bound of pre-normalization interval
    uint32_t rcp_freq;  // Fixed-point reciprocal frequency
    uint32_t bias;      // Bias

    uint32_t SD; // combined cmpl_freq & shift
    uint16_t cmpl_freq; // Complement of frequency: (1 << scale_bits) - freq
    uint16_t rcp_shift; // Reciprocal shift

    uint16_t freq;
    uint16_t start;
    uint32_t padding1;
    //uint32_t padding2;
} RansEncSymbol;

// Decoder symbols are straightforward.
typedef struct {
    uint16_t start;     // Start of range.
    uint16_t freq;      // Symbol frequency.
} RansDecSymbol;

// Initializes an encoder symbol to start "start" and frequency "freq"
static inline void RansEncSymbolInit(RansEncSymbol* s, uint32_t start, uint32_t freq, uint32_t scale_bits)
{
    RansAssert(scale_bits <= 16);
    RansAssert(start <= (1u << scale_bits));
    RansAssert(freq <= (1u << scale_bits) - start);

    // Say M := 1 << scale_bits.
    //
    // The original encoder does:
    //   x_new = (x/freq)*M + start + (x%freq)
    //
    // The fast encoder does (schematically):
    //   q     = mul_hi(x, rcp_freq) >> rcp_shift   (division)
    //   r     = x - q*freq                         (remainder)
    //   x_new = q*M + bias + r                     (new x)
    // plugging in r into x_new yields:
    //   x_new = bias + x + q*(M - freq)
    //        =: bias + x + q*cmpl_freq             (*)
    //
    // and we can just precompute cmpl_freq. Now we just need to
    // set up our parameters such that the original encoder and
    // the fast encoder agree.
    
#ifdef FP_DIV
    //s->scale_bits = scale_bits;
    s->freq = freq;
    s->start = start;
#endif

    s->x_max = ((RANS_BYTE_L >> scale_bits) << 16) * freq -1;
    s->cmpl_freq = (uint16_t) ((1 << scale_bits) - freq);
    if (freq < 2) {
        // freq=0 symbols are never valid to encode, so it doesn't matter what
        // we set our values to.
        //
        // freq=1 is tricky, since the reciprocal of 1 is 1; unfortunately,
        // our fixed-point reciprocal approximation can only multiply by values
        // smaller than 1.
        //
        // So we use the "next best thing": rcp_freq=0xffffffff, rcp_shift=0.
        // This gives:
        //   q = mul_hi(x, rcp_freq) >> rcp_shift
        //     = mul_hi(x, (1<<32) - 1)) >> 0
        //     = floor(x - x/(2^32))
        //     = x - 1 if 1 <= x < 2^32
        // and we know that x>0 (x=0 is never in a valid normalization interval).
        //
        // So we now need to choose the other parameters such that
        //   x_new = x*M + start
        // plug it in:
        //     x*M + start                   (desired result)
        //   = bias + x + q*cmpl_freq        (*)
        //   = bias + x + (x - 1)*(M - 1)    (plug in q=x-1, cmpl_freq)
        //   = bias + 1 + (x - 1)*M
        //   = x*M + (bias + 1 - M)
        //
        // so we have start = bias + 1 - M, or equivalently
        //   bias = start + M - 1.
        s->rcp_freq = ~0u;
        s->rcp_shift = 0;
        s->bias = start + (1 << scale_bits) - 1;
    } else {
        // Alverson, "Integer Division using reciprocals"
        // shift=ceil(log2(freq))
        uint32_t shift = 0;
        while (freq > (1u << shift))
            shift++;

        s->rcp_freq = (uint32_t) (((1ull << (shift + 31)) + freq-1) / freq);
        s->rcp_shift = shift - 1;

        // With these values, 'q' is the correct quotient, so we
        // have bias=start.
        s->bias = start;
    }

    s->rcp_shift += 32; // Avoid the extra >>32 in RansEncPutSymbol
    s->SD = s->cmpl_freq | (s->rcp_shift<<16);
}

// Initialize a decoder symbol to start "start" and frequency "freq"
static inline void RansDecSymbolInit(RansDecSymbol* s, uint32_t start, uint32_t freq)
{
    RansAssert(start <= (1 << 16));
    RansAssert(freq <= (1 << 16) - start);
    s->start = (uint16_t) start;
    s->freq = (uint16_t) freq;
}

// Encodes a given symbol. This is faster than straight RansEnc since we can do
// multiplications instead of a divide.
//
// See RansEncSymbolInit for a description of how this works.
static inline void RansEncPutSymbol(RansState* r, uint8_t** pptr, RansEncSymbol const* sym)
{
    //RansAssert(sym->x_max != 0); // can't encode symbol with freq=0

    // renormalize
    uint32_t x = *r;
    uint32_t x_max = sym->x_max;

    if (x > x_max) {
	uint16_t* ptr = *(uint16_t **)pptr;
	*--ptr = (uint16_t) (x & 0xffff);
	x >>= 16;
	*pptr = (uint8_t *)ptr;
    }

    // x = C(s,x)
    // NOTE: written this way so we get a 32-bit "multiply high" when
    // available. If you're on a 64-bit platform with cheap multiplies
    // (e.g. x64), just bake the +32 into rcp_shift.
    //uint32_t q = (uint32_t) (((uint64_t)x * sym->rcp_freq) >> 32) >> sym->rcp_shift;

    // Slow method, but robust
//    *r = ((x / sym->freq) << sym->scale_bits) + (x % sym->freq) + sym->start;
//    return;

    // The extra >>32 has already been added to RansEncSymbolInit
    uint32_t q = (uint32_t) (((uint64_t)x * sym->rcp_freq) >> sym->rcp_shift);
    *r = x + sym->bias + q * sym->cmpl_freq;

//    assert(((x / sym->freq) << sym->scale_bits) + (x % sym->freq) + sym->start == *r);
}

// Equivalent to RansDecAdvance that takes a symbol.
static inline void RansDecAdvanceSymbol(RansState* r, uint8_t** pptr, RansDecSymbol const* sym, uint32_t scale_bits)
{
    RansDecAdvance(r, pptr, sym->start, sym->freq, scale_bits);
}

// Advances in the bit stream by "popping" a single symbol with range start
// "start" and frequency "freq". All frequencies are assumed to sum to "1 << scale_bits".
// No renormalization or output happens.
static inline void RansDecAdvanceStep(RansState* r, uint32_t start, uint32_t freq, uint32_t scale_bits)
{
    uint32_t mask = (1u << scale_bits) - 1;

    // s, x = D(x)
    uint32_t x = *r;
    *r = freq * (x >> scale_bits) + (x & mask) - start;
}

// Equivalent to RansDecAdvanceStep that takes a symbol.
static inline void RansDecAdvanceSymbolStep(RansState* r, RansDecSymbol const* sym, uint32_t scale_bits)
{
    RansDecAdvanceStep(r, sym->start, sym->freq, scale_bits);
}

// Renormalize.
static inline void RansDecRenorm(RansState* r, uint8_t** pptr)
{
    // renormalize
    uint32_t x = *r;

#ifndef USE_ASM
    // Better on Atom?
    //
    // clang 464, gcc 485
    if (x >= RANS_BYTE_L) return;
    uint16_t* ptr = *(uint16_t **)pptr;
    x = (x << 16) | *ptr++;
    *pptr = (uint8_t *)ptr;

    // clang 335, gcc 687
//    uint32_t c = x < RANS_BYTE_L;
//    uint16_t* ptr = *(uint16_t **)pptr;
//    uint32_t y = (x << 16) | *ptr;
//    x    = c ? y : x;
//    ptr += c;
//    *pptr = (uint8_t *)ptr;

#else
    // clang 596, gcc 650
    uint16_t  *ptr = *(uint16_t **)pptr;
    __asm__ ("movzwl (%0),  %%eax\n\t"
	     "mov    %1,    %%edx\n\t"
	     "shl    $0x10, %%edx\n\t"
             "or     %%eax, %%edx\n\t"
	     "xor    %%eax, %%eax\n\t"
             "cmp    $0x8000,%1\n\t"
             "cmovb  %%edx, %1\n\t"
	     "lea    2(%0), %%rax\n\t"
	     "cmovb  %%rax, %0\n\t"
             : "=r" (ptr), "=r" (x)
             : "0"  (ptr), "1"  (x)
             : "eax", "edx"
             );
    *pptr = (uint8_t *)ptr;

    *pptr = (uint8_t *)ptr;

#endif

    *r = x;
}

#endif // RANS_BYTE_HEADER

/*-------------------------------------------------------------------------- */
/*
 * Example wrapper to use the rans_byte.h functions included above.
 *
 * This demonstrates how to use, and unroll, an order-0 and order-1 frequency
 * model.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include "r16N.h"
#include "permute.h"

#ifndef TF_SHIFT
#  define TF_SHIFT 12
#endif
#define TOTFREQ (1<<TF_SHIFT)

// 9 is considerably faster on some data sets due to reduced table size.
#ifndef TF_SHIFT_O1
#  define TF_SHIFT_O1 9
#endif
#define TOTFREQ_O1 (1<<TF_SHIFT_O1)


/*-----------------------------------------------------------------------------
 * Memory to memory compression functions.
 *
 * These are original versions without any manual loop unrolling. They
 * are easier to understand, but can be up to 2x slower.
 */

#define MAGIC 8

#ifdef UNUSED
static void hist1(unsigned char *in, unsigned int in_size, int F0[256]) {
    int i;
    for (i = 0; i < in_size; i++)
	F0[in[i]]++;
}

static void hist4p(unsigned char *in, unsigned int in_size, int *F0) {
    int F1[256+MAGIC] = {0}, F2[256+MAGIC] = {0}, F3[256+MAGIC] = {0};
    int i;
    unsigned char *in_end4 = in + (in_size & ~3);
    unsigned char *in_end  = in + in_size;
    while (in < in_end4) {
	F0[*in++]++;
	F1[*in++]++;
	F2[*in++]++;
	F3[*in++]++;
    }
    while (in < in_end)
	F0[*in++]++;

    for (i = 0; i < 256; i++)
	F0[i] += F1[i] + F2[i] + F3[i];
}
#endif

static void hist8(unsigned char *in, unsigned int in_size, int F0[256]) {
    int F1[256+MAGIC] = {0}, F2[256+MAGIC] = {0}, F3[256+MAGIC] = {0};
    int F4[256+MAGIC] = {0}, F5[256+MAGIC] = {0}, F6[256+MAGIC] = {0}, F7[256+MAGIC] = {0};
    int i, i4 = ((in_size-4) & ~7)/4; // permits vnext
    uint32_t *in4 = (uint32_t *)in;
    uint32_t vnext = i4 ? in4[0] : 0;
    for (i = 0; i < i4; i+=2) {
	uint32_t v = vnext; vnext = in4[i+1];
	F0[(unsigned char)(v>> 0)]++;
	F1[(unsigned char)(v>> 8)]++;
	F2[(unsigned char)(v>>16)]++;
	F3[(unsigned char)(v>>24)]++;
	v = vnext; vnext = in4[i+2];
	F4[(unsigned char)(v>> 0)]++;
	F5[(unsigned char)(v>> 8)]++;
	F6[(unsigned char)(v>>16)]++;
	F7[(unsigned char)(v>>24)]++;
    }

    i *= 4;
    while (i < in_size)
	F0[in[i++]]++;

    for (i = 0; i < 256; i++)
	F0[i] += F1[i] + F2[i] + F3[i] + F4[i] + F5[i] + F6[i] + F7[i];
}

unsigned int rans_compress_bound_32x16(unsigned int size, int order, int *tab) {
  int tabsz = order == 0
    ?     257*3 + 4 + NX*4 + NX*4
    : 257*257*3 + 4 + NX*4 + NX*4;
  if (tab) *tab = tabsz;
  return 1.05*size + NX*4 + tabsz;
}

// _mm256__mul_epu32 is:
//  -b -d -f -h
//* -q -s -u -w
//= BQ DS FU HW where BQ=b*q etc
//
// We want
//  abcd efgh  (a)
// *pqrs tuvw  (b)
// =ABCD EFGH
//
// a    mul b      => BQ DS FU HW
// >>= 8           => -B QD SF UH
// &               => -B -D -F -H (1)
// a>>8 mul b>>8   => AP CR ET GV
// &               => A- C- E- G-
// | with (1)      => AB CD EF GH
#if 0
static __m256i _mm256_mulhi_epu32(__m256i a, __m256i b) {
    __m256i ab_lm = _mm256_mul_epu32(a, b);
    ab_lm = _mm256_srli_epi64(ab_lm, 32);
    a = _mm256_srli_epi64(a, 32);

    ab_lm = _mm256_and_si256(ab_lm, _mm256_set1_epi64x(0xffffffff));
    b = _mm256_srli_epi64(b, 32);

    __m256i ab_hm = _mm256_mul_epu32(a, b);
    ab_hm = _mm256_and_si256(ab_hm, _mm256_set1_epi64x((uint64_t)0xffffffff00000000)); 
    ab_hm = _mm256_or_si256(ab_hm, ab_lm);

    return ab_hm;
}
#else
static __m256i _mm256_mulhi_epu32(__m256i a, __m256i b) {
    // Multiply bottom 4 items and top 4 items together.
    __m256i ab_hm = _mm256_mul_epu32(_mm256_srli_epi64(a, 32), _mm256_srli_epi64(b, 32));
    __m256i ab_lm = _mm256_srli_epi64(_mm256_mul_epu32(a, b), 32);

    // Shift to get hi 32-bit of each 64-bit product
    ab_hm = _mm256_and_si256(ab_hm, _mm256_set1_epi64x((uint64_t)0xffffffff00000000));

    return _mm256_or_si256(ab_lm, ab_hm);
}
#endif

#if 0
__m256i _mm256_div_epi32(__m256i a, __m256i b) {
    return _mm256_cvttps_epi32(_mm256_div_ps(_mm256_cvtepi32_ps(a),
					    _mm256_cvtepi32_ps(b)));
}
#else
__m256i _mm256_div_epi32(__m256i a, __m256i b) {
    __m256d a1  = _mm256_cvtepi32_pd(_mm256_extracti128_si256(a, 0));
    __m256d b1  = _mm256_cvtepi32_pd(_mm256_extracti128_si256(b, 0));
    __m128i ab1 = _mm256_cvttpd_epi32(_mm256_div_pd(a1, b1));

    __m256d a2  = _mm256_cvtepi32_pd(_mm256_extracti128_si256(a, 1));
    __m256d b2  = _mm256_cvtepi32_pd(_mm256_extracti128_si256(b, 1));
    __m128i ab2 = _mm256_cvttpd_epi32(_mm256_div_pd(a2, b2));

    return _mm256_inserti128_si256(_mm256_castsi128_si256(ab1), ab2, 1);
}
#endif

#if 1
// Simulated gather.  This is sometimes faster as it can run on other ports.
static inline __m256i _mm256_i32gather_epi32x(int *b, __m256i idx, int size) {
    int c[8] __attribute__((aligned(32)));
    _mm256_store_si256((__m256i *)c, idx);
    return _mm256_set_epi32(b[c[7]], b[c[6]], b[c[5]], b[c[4]], b[c[3]], b[c[2]], b[c[1]], b[c[0]]);
}
#else
#define _mm256_i32gather_epi32x _mm256_i32gather_epi32
#endif

unsigned char *rans_compress_O0_32x16(unsigned char *in, unsigned int in_size,
				     unsigned char *out, unsigned int *out_size) {
    unsigned char *cp, *out_end;
    RansEncSymbol syms[256];
    RansState ransN[NX] __attribute__((aligned(32)));
    uint8_t* ptr;
    int F[256+MAGIC] = {0}, i, j, tab_size, rle, x, fsum = 0;
    int m = 0, M = 0;
    int tabsz;
    int bound = rans_compress_bound_32x16(in_size,0,&tabsz), z;

    //uint64_t sym_mf[256], sym_bfs[256];
    //uint32_t SD[256], SC[256], SB[256], SA[256];
    uint32_t SB[256], SA[256], SD[256], SC[256];
#ifdef FP_DIV
    uint32_t FS[256];
#endif

    if (!out) {
	*out_size = bound;
	out = malloc(*out_size);
    }
    if (!out || bound > *out_size) {
	return NULL;
    }

    ptr = out_end = out + bound;

    // Compute statistics
    hist8(in, in_size, F);
    double p = (double)TOTFREQ/(double)in_size;

    // Normalise so T[i] == TOTFREQ
    for (m = M = j = 0; j < 256; j++) {
	if (!F[j])
	    continue;

	if (m < F[j])
	    m = F[j], M = j;

	if ((F[j] = F[j]*p+0.499) == 0)
	    F[j] = 1;
	fsum += F[j];
    }

    fsum++; // not needed, but can't remove without removing assert x<TOTFREQ (in old code)
    int adjust = TOTFREQ - fsum;
    if (adjust > 0) {
	F[M] += adjust;
    } else if (adjust < 0) {
	if (F[M] > -adjust) {
	    F[M] += adjust;
	} else {
	    adjust += F[M]-1;
	    F[M] = 1;
	    for (j = 0; adjust && j < 256; j++) {
		if (F[j] < 2) continue;

		int d = F[j] > -adjust;
		int m = d ? adjust : 1-F[j];
		F[j]   += m;
		adjust -= m;
	    }
	}
    }

    //printf("F[%d]=%d\n", M, F[M]);
    assert(F[M]>0);

    // Encode statistics.
    cp = out+4;

    for (x = rle = j = 0; j < 256; j++) {
	if (F[j]) {
	    // j
	    if (rle) {
		rle--;
	    } else {
		*cp++ = j;
		if (!rle && j && F[j-1])  {
		    for(rle=j+1; rle<256 && F[rle]; rle++)
			;
		    rle -= j+1;
		    *cp++ = rle;
		}
		//fprintf(stderr, "%d: %d %d\n", j, rle, N[j]);
	    }
	    
	    // F[j]
	    if (F[j]<128) {
		*cp++ = F[j];
	    } else {
		*cp++ = 128 | (F[j]>>8);
		*cp++ = F[j]&0xff;
	    }
	    RansEncSymbolInit(&syms[j], x, F[j], TF_SHIFT);
	    //SB[j] = syms[j].x_max;
	    //SA[i] = syms[j].rcp_freq;
	    //SD[i] = (syms[j].cmpl_freq<<0) | (syms[j].rcp_shift<<16);
	    //SC[i] = syms[j].bias;
	    x += F[j];
	}
    }
    *cp++ = 0;

    //write(2, out+4, cp-(out+4));
    tab_size = cp-out;

    for (z = 0; z < NX; z++)
      RansEncInit(&ransN[z]);

    z = i = in_size&(NX-1);
    while (z-- > 0)
      RansEncPutSymbol(&ransN[z], &ptr, &syms[in[in_size-(i-z)]]);

    for (i = 0; i < 256; i++) {
	//sym_mf[i] = syms[i].rcp_freq | (((uint64_t)syms[i].x_max)<<32);
#ifdef FP_DIV
	SB[i] = syms[i].x_max;
	FS[i] = (syms[i].freq) | (syms[i].start << 16);
#else
	SB[i] = syms[i].x_max;
	SA[i] = syms[i].rcp_freq;
	SD[i] = (syms[i].cmpl_freq<<0) | (syms[i].rcp_shift<<16);
	SC[i] = syms[i].bias;
#endif
    }

    uint16_t *ptr16 = (uint16_t *)ptr;

#define LOAD1(a,b) __m256i a##1 = _mm256_load_si256((__m256i *)&b[0]);
#define LOAD2(a,b) __m256i a##2 = _mm256_load_si256((__m256i *)&b[8]);
#define LOAD3(a,b) __m256i a##3 = _mm256_load_si256((__m256i *)&b[16]);
#define LOAD4(a,b) __m256i a##4 = _mm256_load_si256((__m256i *)&b[24]);
#define LOAD(a,b) LOAD1(a,b);LOAD2(a,b);LOAD3(a,b);LOAD4(a,b)

#define STORE1(a,b) _mm256_store_si256((__m256i *)&b[0],  a##1);
#define STORE2(a,b) _mm256_store_si256((__m256i *)&b[8],  a##2);
#define STORE3(a,b) _mm256_store_si256((__m256i *)&b[16], a##3);
#define STORE4(a,b) _mm256_store_si256((__m256i *)&b[24], a##4);
#define STORE(a,b) STORE1(a,b);STORE2(a,b);STORE3(a,b);STORE4(a,b)

    LOAD(Rv, ransN);

    for (i=(in_size &~(NX-1)); i>0; i-=NX) {
	uint8_t *c = &in[i-32];

// Set vs gather methods of loading data.
// Gather is faster, but can only schedule a few to run in parallel.
#define SET1(a,b) __m256i a##1 = _mm256_set_epi32(b[c[ 7]], b[c[ 6]], b[c[ 5]], b[c[ 4]], b[c[ 3]], b[c[ 2]], b[c[ 1]], b[c[ 0]])
#define SET2(a,b) __m256i a##2 = _mm256_set_epi32(b[c[15]], b[c[14]], b[c[13]], b[c[12]], b[c[11]], b[c[10]], b[c[ 9]], b[c[ 8]])
#define SET3(a,b) __m256i a##3 = _mm256_set_epi32(b[c[23]], b[c[22]], b[c[21]], b[c[20]], b[c[19]], b[c[18]], b[c[17]], b[c[16]])
#define SET4(a,b) __m256i a##4 = _mm256_set_epi32(b[c[31]], b[c[30]], b[c[29]], b[c[28]], b[c[27]], b[c[26]], b[c[25]], b[c[24]])
#define SET(a,b) SET1(a,b);SET2(a,b);SET3(a,b);SET4(a,b)

	// Renorm:
	// if (x > x_max) {*--ptr16 = x & 0xffff; x >>= 16;}
	SET(xmax, SB);
	__m256i cv1 = _mm256_cmpgt_epi32(Rv1, xmax1);
	__m256i cv2 = _mm256_cmpgt_epi32(Rv2, xmax2);
	__m256i cv3 = _mm256_cmpgt_epi32(Rv3, xmax3);
	__m256i cv4 = _mm256_cmpgt_epi32(Rv4, xmax4);

	// Store bottom 16-bits at ptr16
	unsigned int imask1 = _mm256_movemask_ps((__m256)cv1);
	unsigned int imask2 = _mm256_movemask_ps((__m256)cv2);
	unsigned int imask3 = _mm256_movemask_ps((__m256)cv3);
	unsigned int imask4 = _mm256_movemask_ps((__m256)cv4);

	__m256i idx1 = _mm256_load_si256((const __m256i*)permutec[imask1]);
	__m256i idx2 = _mm256_load_si256((const __m256i*)permutec[imask2]);
	__m256i idx3 = _mm256_load_si256((const __m256i*)permutec[imask3]);
	__m256i idx4 = _mm256_load_si256((const __m256i*)permutec[imask4]);

	// Permute; to gather together the rans states that need flushing
	__m256i V1 = _mm256_permutevar8x32_epi32(_mm256_and_si256(Rv1, cv1), idx1);
	__m256i V2 = _mm256_permutevar8x32_epi32(_mm256_and_si256(Rv2, cv2), idx2);
	__m256i V3 = _mm256_permutevar8x32_epi32(_mm256_and_si256(Rv3, cv3), idx3);
	__m256i V4 = _mm256_permutevar8x32_epi32(_mm256_and_si256(Rv4, cv4), idx4);
	
	// We only flush bottom 16 bits, to squash 32-bit states into 16 bit.
	V1 = _mm256_and_si256(V1, _mm256_set1_epi32(0xffff));
	V2 = _mm256_and_si256(V2, _mm256_set1_epi32(0xffff));
	V3 = _mm256_and_si256(V3, _mm256_set1_epi32(0xffff));
	V4 = _mm256_and_si256(V4, _mm256_set1_epi32(0xffff));
	__m256i V12 = _mm256_packus_epi32(V1, V2);
	__m256i V34 = _mm256_packus_epi32(V3, V4);

	// It's BAba order, want BbAa so shuffle.
	V12 = _mm256_permute4x64_epi64(V12, 0xd8);
	V34 = _mm256_permute4x64_epi64(V34, 0xd8);

	// Now we have bottom N 16-bit values in each V12/V34 to flush
	__m128i f =  _mm256_extractf128_si256(V34, 1);
	_mm_storeu_si128((__m128i *)(ptr16-8), f);
	ptr16 -= _mm_popcnt_u32(imask4);

	f =  _mm256_extractf128_si256(V34, 0);
	_mm_storeu_si128((__m128i *)(ptr16-8), f);
	ptr16 -= _mm_popcnt_u32(imask3);

	f =  _mm256_extractf128_si256(V12, 1);
	_mm_storeu_si128((__m128i *)(ptr16-8), f);
	ptr16 -= _mm_popcnt_u32(imask2);

	f =  _mm256_extractf128_si256(V12, 0);
	_mm_storeu_si128((__m128i *)(ptr16-8), f);
	ptr16 -= _mm_popcnt_u32(imask1);

	__m256i Rs;
	Rs = _mm256_srli_epi32(Rv1, 16); Rv1 = _mm256_blendv_epi8(Rv1, Rs, cv1);
	Rs = _mm256_srli_epi32(Rv2, 16); Rv2 = _mm256_blendv_epi8(Rv2, Rs, cv2);
	Rs = _mm256_srli_epi32(Rv3, 16); Rv3 = _mm256_blendv_epi8(Rv3, Rs, cv3);
	Rs = _mm256_srli_epi32(Rv4, 16); Rv4 = _mm256_blendv_epi8(Rv4, Rs, cv4);

	// Cannot trivially replace the multiply as mulhi_epu32 doesn't exist (only mullo).
	// However we can use _mm256_mul_epu32 twice to get 64bit results (half our lanes)
	// and shift/or to get the answer.
	//
	// (AVX512 allows us to hold it all in 64-bit lanes and use mullo_epi64
	// plus a shift.  KNC has mulhi_epi32, but not sure if this is available.)

#ifndef FP_DIV
	SET(rfv,   SA);

	rfv1 = _mm256_mulhi_epu32(Rv1, rfv1);
	rfv2 = _mm256_mulhi_epu32(Rv2, rfv2);
	rfv3 = _mm256_mulhi_epu32(Rv3, rfv3);
	rfv4 = _mm256_mulhi_epu32(Rv4, rfv4);

	SET(SDv,   SD);

	__m256i shiftv1 = _mm256_srli_epi32(SDv1, 16);
	__m256i shiftv2 = _mm256_srli_epi32(SDv2, 16);
	__m256i shiftv3 = _mm256_srli_epi32(SDv3, 16);
	__m256i shiftv4 = _mm256_srli_epi32(SDv4, 16);

	shiftv1 = _mm256_sub_epi32(shiftv1, _mm256_set1_epi32(32));
	shiftv2 = _mm256_sub_epi32(shiftv2, _mm256_set1_epi32(32));
	shiftv3 = _mm256_sub_epi32(shiftv3, _mm256_set1_epi32(32));
	shiftv4 = _mm256_sub_epi32(shiftv4, _mm256_set1_epi32(32));

	__m256i qv1 = _mm256_srlv_epi32(rfv1, shiftv1);
	__m256i qv2 = _mm256_srlv_epi32(rfv2, shiftv2);

	__m256i freqv1 = _mm256_and_si256(SDv1, _mm256_set1_epi32(0xffff));
	__m256i freqv2 = _mm256_and_si256(SDv2, _mm256_set1_epi32(0xffff));
	qv1 = _mm256_mullo_epi32(qv1, freqv1);
	qv2 = _mm256_mullo_epi32(qv2, freqv2);

	__m256i qv3 = _mm256_srlv_epi32(rfv3, shiftv3);
	__m256i qv4 = _mm256_srlv_epi32(rfv4, shiftv4);

	__m256i freqv3 = _mm256_and_si256(SDv3, _mm256_set1_epi32(0xffff));
	__m256i freqv4 = _mm256_and_si256(SDv4, _mm256_set1_epi32(0xffff));
	qv3 = _mm256_mullo_epi32(qv3, freqv3);
	qv4 = _mm256_mullo_epi32(qv4, freqv4);

	SET(biasv, SC);

	qv1 = _mm256_add_epi32(qv1, biasv1);
	qv2 = _mm256_add_epi32(qv2, biasv2);
	qv3 = _mm256_add_epi32(qv3, biasv3);
	qv4 = _mm256_add_epi32(qv4, biasv4);

	Rv1 = _mm256_add_epi32(Rv1, qv1);
	Rv2 = _mm256_add_epi32(Rv2, qv2);
	Rv3 = _mm256_add_epi32(Rv3, qv3);
	Rv4 = _mm256_add_epi32(Rv4, qv4);
#else
	// slow method:
	// *r = ((x / sym->freq) << sym->scale_bits) + (x % sym->freq) + sym->start;
	
	// xdiv = x/freq;
	// xmod = x - xd * f;
	// R = xdiv << TF_SHIFT;
	// R += xmod;
	// R += start;

	SET(FSv,  FS);

	__m256i Fv1 = _mm256_and_si256(FSv1, _mm256_set1_epi32(0xffff));
	__m256i Fv2 = _mm256_and_si256(FSv2, _mm256_set1_epi32(0xffff));
	__m256i Fv3 = _mm256_and_si256(FSv3, _mm256_set1_epi32(0xffff));
	__m256i Fv4 = _mm256_and_si256(FSv4, _mm256_set1_epi32(0xffff));

	__m256i Sv1 = _mm256_srli_epi32(FSv1, 16);
	__m256i Sv2 = _mm256_srli_epi32(FSv2, 16);
	__m256i Sv3 = _mm256_srli_epi32(FSv3, 16);
	__m256i Sv4 = _mm256_srli_epi32(FSv4, 16);

	__m256i Rdiv1 = _mm256_div_epi32(Rv1, Fv1);
	__m256i Rdiv2 = _mm256_div_epi32(Rv2, Fv2);
	__m256i Rdiv3 = _mm256_div_epi32(Rv3, Fv3);
	__m256i Rdiv4 = _mm256_div_epi32(Rv4, Fv4);
	
	__m256i Rmod1 = _mm256_sub_epi32(Rv1, _mm256_mullo_epi32(Rdiv1, Fv1));
	__m256i Rmod2 = _mm256_sub_epi32(Rv2, _mm256_mullo_epi32(Rdiv2, Fv2));
	__m256i Rmod3 = _mm256_sub_epi32(Rv3, _mm256_mullo_epi32(Rdiv3, Fv3));
	__m256i Rmod4 = _mm256_sub_epi32(Rv4, _mm256_mullo_epi32(Rdiv4, Fv4));

	Rv1 = _mm256_slli_epi32(Rdiv1, TF_SHIFT);
	Rv2 = _mm256_slli_epi32(Rdiv2, TF_SHIFT);
	Rv3 = _mm256_slli_epi32(Rdiv3, TF_SHIFT);
	Rv4 = _mm256_slli_epi32(Rdiv4, TF_SHIFT);

	Rv1 = _mm256_add_epi32(Rv1, Rmod1);
	Rv2 = _mm256_add_epi32(Rv2, Rmod2);
	Rv3 = _mm256_add_epi32(Rv3, Rmod3);
	Rv4 = _mm256_add_epi32(Rv4, Rmod4);

	Rv1 = _mm256_add_epi32(Rv1, Sv1);
	Rv2 = _mm256_add_epi32(Rv2, Sv2);
	Rv3 = _mm256_add_epi32(Rv3, Sv3);
	Rv4 = _mm256_add_epi32(Rv4, Sv4);
#endif
    }

    STORE(Rv, ransN);

    ptr = (uint8_t *)ptr16;

    for (z = NX-1; z >= 0; z--)
      RansEncFlush(&ransN[z], &ptr);

    // Finalise block size and return it
    *out_size = (out_end - ptr) + tab_size;

    cp = out;
    *cp++ = (in_size>> 0) & 0xff;
    *cp++ = (in_size>> 8) & 0xff;
    *cp++ = (in_size>>16) & 0xff;
    *cp++ = (in_size>>24) & 0xff;

    memmove(out + tab_size, ptr, out_end-ptr);

    return out;
}

typedef struct {
    unsigned char R[TOTFREQ];
} ari_decoder;

unsigned char *rans_uncompress_O0_32x16(unsigned char *in, unsigned int in_size,
				       unsigned char *out, unsigned int *out_size) {
    /* Load in the static tables */
    unsigned char *cp = in + 4;
    int i, j, x, y, out_sz, rle;
    //uint16_t sfreq[TOTFREQ+32];
    //uint16_t sbase[TOTFREQ+32]; // faster to use 32-bit on clang
    uint8_t  ssym [TOTFREQ+64]; // faster to use 16-bit on clang

    uint32_t s3[TOTFREQ] __attribute__((aligned(32))); // For TF_SHIFT <= 12

    out_sz = ((in[0])<<0) | ((in[1])<<8) | ((in[2])<<16) | ((in[3])<<24);
    if (!out) {
	out = malloc(out_sz);
	*out_size = out_sz;
    }
    if (!out || out_sz > *out_size)
	return NULL;

    // Precompute reverse lookup of frequency.
    rle = x = y = 0;
    j = *cp++;
    do {
	int F, C;
	if ((F = *cp++) >= 128) {
	    F &= ~128;
	    F = ((F & 127) << 8) | *cp++;
	}
	C = x;

        for (y = 0; y < F; y++) {
	  ssym [y + C] = j;
	  //sfreq[y + C] = F;
	  //sbase[y + C] = y;
	  s3[y+C] = (((uint32_t)F)<<(TF_SHIFT+8))|(y<<8)|j;
        }
	x += F;

	if (!rle && j+1 == *cp) {
	    j = *cp++;
	    rle = *cp++;
	} else if (rle) {
	    rle--;
	    j++;
	} else {
	    j = *cp++;
	}
    } while(j);

    assert(x < TOTFREQ);

    int z;

    RansState R[NX] __attribute__((aligned(32)));
    for (z = 0; z < NX; z++)
      RansDecInit(&R[z], &cp);

    uint16_t *sp = (uint16_t *)cp;

    int out_end = (out_sz&~(NX-1));
    const uint32_t mask = (1u << TF_SHIFT)-1;

    __m256i maskv  = _mm256_set1_epi32(mask); // set mask in all lanes
    LOAD(Rv, R);

    for (i=0; i < out_end; i+=NX) {
	//for (z = 0; z < NX; z++)
	//  m[z] = R[z] & mask;
	__m256i masked1 = _mm256_and_si256(Rv1, maskv);
	__m256i masked2 = _mm256_and_si256(Rv2, maskv);

	//  S[z] = s3[m[z]];
	__m256i Sv1 = _mm256_i32gather_epi32x((int *)s3, masked1, sizeof(*s3));
	__m256i Sv2 = _mm256_i32gather_epi32x((int *)s3, masked2, sizeof(*s3));

	//  f[z] = S[z]>>(TF_SHIFT+8);
	__m256i fv1 = _mm256_srli_epi32(Sv1, TF_SHIFT+8);
	__m256i fv2 = _mm256_srli_epi32(Sv2, TF_SHIFT+8);

	//  b[z] = (S[z]>>8) & mask;
	__m256i bv1 = _mm256_and_si256(_mm256_srli_epi32(Sv1, 8), maskv);
	__m256i bv2 = _mm256_and_si256(_mm256_srli_epi32(Sv2, 8), maskv);

	//  s[z] = S[z] & 0xff;
	__m256i sv1 = _mm256_and_si256(Sv1, _mm256_set1_epi32(0xff));
	__m256i sv2 = _mm256_and_si256(Sv2, _mm256_set1_epi32(0xff));

	//  R[z] = f[z] * (R[z] >> TF_SHIFT) + b[z];
	Rv1 = _mm256_add_epi32(_mm256_mullo_epi32(_mm256_srli_epi32(Rv1,TF_SHIFT),fv1),bv1);
	Rv2 = _mm256_add_epi32(_mm256_mullo_epi32(_mm256_srli_epi32(Rv2,TF_SHIFT),fv2),bv2);

	// Tricky one:  out[i+z] = s[z];
	//             ---h---g ---f---e  ---d---c ---b---a
	//             ---p---o ---n---m  ---l---k ---j---i
	// packs_epi32 -p-o-n-m -h-g-f-e  -l-k-j-i -d-c-b-a
	// permute4x64 -p-o-n-m -l-k-j-i  -h-g-f-e -d-c-b-a
	// packs_epi16 ponmlkji ponmlkji  hgfedcba hgfedcba
	sv1 = _mm256_packus_epi32(sv1, sv2);
	sv1 = _mm256_permute4x64_epi64(sv1, 0xd8);
	__m256i Vv1 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i *)sp));
	sv1 = _mm256_packus_epi16(sv1, sv1);

	// c =  R[z] < RANS_BYTE_L;
	__m256i renorm_mask1 = _mm256_xor_si256(Rv1, _mm256_set1_epi32(0x80000000));
	__m256i renorm_mask2 = _mm256_xor_si256(Rv2, _mm256_set1_epi32(0x80000000));
	renorm_mask1 = _mm256_cmpgt_epi32(_mm256_set1_epi32(RANS_BYTE_L-0x80000000), renorm_mask1);
	renorm_mask2 = _mm256_cmpgt_epi32(_mm256_set1_epi32(RANS_BYTE_L-0x80000000), renorm_mask2);
	
	// y = (R[z] << 16) | V[z];
	unsigned int imask1 = _mm256_movemask_ps((__m256)renorm_mask1);
	__m256i idx1 = _mm256_load_si256((const __m256i*)permute[imask1]);
	__m256i Yv1 = _mm256_slli_epi32(Rv1, 16);
	Vv1 = _mm256_permutevar8x32_epi32(Vv1, idx1);
	__m256i Yv2 = _mm256_slli_epi32(Rv2, 16);

	// Shuffle the renorm values to correct lanes and incr sp pointer
	unsigned int imask2 = _mm256_movemask_ps((__m256)renorm_mask2);
	sp += _mm_popcnt_u32(imask1);

	__m256i idx2 = _mm256_load_si256((const __m256i*)permute[imask2]);
	__m256i Vv2 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i *)sp));
	sp += _mm_popcnt_u32(imask2);

	Yv1 = _mm256_or_si256(Yv1, Vv1);
	Vv2 = _mm256_permutevar8x32_epi32(Vv2, idx2);
	Yv2 = _mm256_or_si256(Yv2, Vv2);

	// R[z] = c ? Y[z] : R[z];
	Rv1 = _mm256_blendv_epi8(Rv1, Yv1, renorm_mask1);
	Rv2 = _mm256_blendv_epi8(Rv2, Yv2, renorm_mask2);

	// ------------------------------------------------------------

	//  m[z] = R[z] & mask;
	//  S[z] = s3[m[z]];
	__m256i masked3 = _mm256_and_si256(Rv3, maskv);
	__m256i Sv3 = _mm256_i32gather_epi32x((int *)s3, masked3, sizeof(*s3));

	*(uint64_t *)&out[i+0] = _mm256_extract_epi64(sv1, 0);
	*(uint64_t *)&out[i+8] = _mm256_extract_epi64(sv1, 2);

	__m256i masked4 = _mm256_and_si256(Rv4, maskv);
	__m256i Sv4 = _mm256_i32gather_epi32x((int *)s3, masked4, sizeof(*s3));

	//  f[z] = S[z]>>(TF_SHIFT+8);
	__m256i fv3 = _mm256_srli_epi32(Sv3, TF_SHIFT+8);
	__m256i fv4 = _mm256_srli_epi32(Sv4, TF_SHIFT+8);

	//  b[z] = (S[z]>>8) & mask;
	__m256i bv3 = _mm256_and_si256(_mm256_srli_epi32(Sv3, 8), maskv);
	__m256i bv4 = _mm256_and_si256(_mm256_srli_epi32(Sv4, 8), maskv);

	//  s[z] = S[z] & 0xff;
	__m256i sv3 = _mm256_and_si256(Sv3, _mm256_set1_epi32(0xff));
	__m256i sv4 = _mm256_and_si256(Sv4, _mm256_set1_epi32(0xff));

	//  R[z] = f[z] * (R[z] >> TF_SHIFT) + b[z];
	Rv3 = _mm256_add_epi32(_mm256_mullo_epi32(_mm256_srli_epi32(Rv3,TF_SHIFT),fv3),bv3);
	Rv4 = _mm256_add_epi32(_mm256_mullo_epi32(_mm256_srli_epi32(Rv4,TF_SHIFT),fv4),bv4);

	// Tricky one:  out[i+z] = s[z];
	//             ---h---g ---f---e  ---d---c ---b---a
	//             ---p---o ---n---m  ---l---k ---j---i
	// packs_epi32 -p-o-n-m -h-g-f-e  -l-k-j-i -d-c-b-a
	// permute4x64 -p-o-n-m -l-k-j-i  -h-g-f-e -d-c-b-a
	// packs_epi16 ponmlkji ponmlkji  hgfedcba hgfedcba
	sv3 = _mm256_packus_epi32(sv3, sv4);
	sv3 = _mm256_permute4x64_epi64(sv3, 0xd8);
	__m256i renorm_mask3 = _mm256_xor_si256(Rv3, _mm256_set1_epi32(0x80000000));
	__m256i renorm_mask4 = _mm256_xor_si256(Rv4, _mm256_set1_epi32(0x80000000));
	sv3 = _mm256_packus_epi16(sv3, sv3);
	// c =  R[z] < RANS_BYTE_L;

	renorm_mask3 = _mm256_cmpgt_epi32(_mm256_set1_epi32(RANS_BYTE_L-0x80000000), renorm_mask3);
	renorm_mask4 = _mm256_cmpgt_epi32(_mm256_set1_epi32(RANS_BYTE_L-0x80000000), renorm_mask4);
	
	*(uint64_t *)&out[i+16] = _mm256_extract_epi64(sv3, 0);
	*(uint64_t *)&out[i+24] = _mm256_extract_epi64(sv3, 2);

	// y = (R[z] << 16) | V[z];
	__m256i Vv3 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i *)sp));
	__m256i Yv3 = _mm256_slli_epi32(Rv3, 16);
	unsigned int imask3 = _mm256_movemask_ps((__m256)renorm_mask3);
	__m256i idx3 = _mm256_load_si256((const __m256i*)permute[imask3]);

	// Shuffle the renorm values to correct lanes and incr sp pointer
	Vv3 = _mm256_permutevar8x32_epi32(Vv3, idx3);
	__m256i Yv4 = _mm256_slli_epi32(Rv4, 16);
	unsigned int imask4 = _mm256_movemask_ps((__m256)renorm_mask4);
	sp += _mm_popcnt_u32(imask3);

	__m256i idx4 = _mm256_load_si256((const __m256i*)permute[imask4]);
	__m256i Vv4 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i *)sp));

	//Vv = _mm256_and_si256(Vv, renorm_mask);  (blend does the AND anyway)
	Yv3 = _mm256_or_si256(Yv3, Vv3);
	Vv4 = _mm256_permutevar8x32_epi32(Vv4, idx4);
	Yv4 = _mm256_or_si256(Yv4, Vv4);

	sp += _mm_popcnt_u32(imask4);

	// R[z] = c ? Y[z] : R[z];
	Rv3 = _mm256_blendv_epi8(Rv3, Yv3, renorm_mask3);
	Rv4 = _mm256_blendv_epi8(Rv4, Yv4, renorm_mask4);
    }

    STORE(Rv, R);
    //_mm256_store_si256((__m256i *)&R[0], Rv1);
    //_mm256_store_si256((__m256i *)&R[8], Rv2);
    //_mm256_store_si256((__m256i *)&R[16], Rv3);
    //_mm256_store_si256((__m256i *)&R[24], Rv4);

//#pragma omp simd
//	for (z = 0; z < NX; z++) {
//	  uint32_t m = R[z] & mask;
//	  R[z] = sfreq[m] * (R[z] >> TF_SHIFT) + sbase[m];
//	  out[i+z] = ssym[m];
//	  uint32_t c = R[z] < RANS_BYTE_L;  // NX16=>166MB/s
//	  uint32_t y = (R[z] << 16) | *spN[z];
//	  spN[z] += c ? 1 : 0;
//	  R[z]    = c ? y : R[z];
//
//	}
//    }

    for (z = out_sz & (NX-1); z-- > 0; )
      out[out_end + z] = ssym[R[z] & mask];

    *out_size = out_sz;
    return out;
}

#ifdef UNUSED
static void hist1_1(unsigned char *in, unsigned int in_size,
		    int F0[256][256], int T0[256]) {
    unsigned int last_i, i;
    unsigned char c;

    for (last_i=i=0; i<in_size; i++) {
	F0[last_i][c = in[i]]++;
	T0[last_i]++;
	last_i = c;
    }
}
#endif

static void hist1_4(unsigned char *in, unsigned int in_size,
		    int F0[256][256], int *T0) {
    int T1[256+MAGIC] = {0}, T2[256+MAGIC] = {0}, T3[256+MAGIC] = {0};
    unsigned int idiv4 = in_size/4;
    int i;
    unsigned char c0, c1, c2, c3;

    unsigned char *in0 = in + 0;
    unsigned char *in1 = in + idiv4;
    unsigned char *in2 = in + idiv4*2;
    unsigned char *in3 = in + idiv4*3;

    unsigned char last_0 = 0, last_1 = in1[-1], last_2 = in2[-1], last_3 = in3[-1];
    //unsigned char last_0 = 0, last_1 = 0, last_2 = 0, last_3 = 0;

    unsigned char *in0_end = in1;

    while (in0 < in0_end) {
	F0[last_0][c0 = *in0++]++;
	T0[last_0]++;
	last_0 = c0;

	F0[last_1][c1 = *in1++]++;
	T1[last_1]++;
	last_1 = c1;

	F0[last_2][c2 = *in2++]++;
	T2[last_2]++;
	last_2 = c2;

	F0[last_3][c3 = *in3++]++;
	T3[last_3]++;
	last_3 = c3;
    }

    while (in3 < in + in_size) {
	F0[last_3][c3 = *in3++]++;
	T3[last_3]++;
	last_3 = c3;
    }

    for (i = 0; i < 256; i++) {
	T0[i]+=T1[i]+T2[i]+T3[i];
    }
}

unsigned char *rans_compress_O1_32x16(unsigned char *in, unsigned int in_size,
				     unsigned char *out, unsigned int *out_size) {
    unsigned char *cp, *out_end;
    unsigned int tab_size, rle_i, rle_j;
    RansEncSymbol syms[256][256];
    int bound = rans_compress_bound_32x16(in_size,1, NULL), z;
    RansState ransN[NX] __attribute__((aligned(32)));

    if (!out) {
	*out_size = bound;
	out = malloc(*out_size);
    }
    if (!out || bound > *out_size)
	return NULL;

    out_end = out + bound;
    cp = out+4;

    int F[256][256] = {{0}}, T[256+MAGIC] = {0}, i, j;

    //memset(F, 0, 256*256*sizeof(int));
    //memset(T, 0, 256*sizeof(int));

    hist1_4(in, in_size, F, T);

    for (z = 1; z < NX; z++)
	F[0][in[z*(in_size/NX)]]++;
    T[0]+=NX-1;

    // Normalise so T[i] == TOTFREQ
    for (rle_i = i = 0; i < 256; i++) {
	int t2, m, M;
	unsigned int x;

	if (T[i] == 0)
	    continue;

	//uint64_t p = (TOTFREQ * TOTFREQ) / t;
	double p = ((double)TOTFREQ_O1)/T[i];

	for (t2 = m = M = j = 0; j < 256; j++) {
	    if (!F[i][j])
		continue;

	    if (m < F[i][j])
		m = F[i][j], M = j;

	    if ((F[i][j] *= p) <= 0)
	        F[i][j] = 1;
	    t2 += F[i][j];
	}

	//t2++;

	int adjust = TOTFREQ_O1-t2;
	if (adjust > 0) {
	    // Boost most common
	    F[i][M] += adjust;
	} else if (adjust < 0) {
	    // Reduce highest and distribute remainder
	    if (F[i][M] > -adjust) {
		F[i][M] += adjust;
	    } else {
		adjust += F[i][M]-1;
		F[i][M] = 1;

		for (j = 0; adjust && j < 256; j++) {
		    if (F[i][j] < 2) continue;

		    int d = F[i][j] > -adjust;
		    int m = d ? adjust : 1-F[i][j];
		    F[i][j]   += m;
		    adjust -= m;
		}
	    }
	}

	// Store frequency table
	// i
	if (rle_i) {
	    rle_i--;
	} else {
	    *cp++ = i;
	    // FIXME: could use order-0 statistics to observe which alphabet
	    // symbols are present and base RLE on that ordering instead.
	    if (i && T[i-1]) {
		for(rle_i=i+1; rle_i<256 && T[rle_i]; rle_i++)
		    ;
		rle_i -= i+1;
		*cp++ = rle_i;
	    }
	}

	int *F_i_ = F[i];
	x = 0;
	rle_j = 0;
	for (j = 0; j < 256; j++) {
	    if (F_i_[j]) {
		//fprintf(stderr, "F[%d][%d]=%d, x=%d\n", i, j, F_i_[j], x);

		// j
		if (rle_j) {
		    rle_j--;
		} else {
		    *cp++ = j;
		    if (!rle_j && j && F_i_[j-1]) {
			for(rle_j=j+1; rle_j<256 && F_i_[rle_j]; rle_j++)
			    ;
			rle_j -= j+1;
			*cp++ = rle_j;
		    }
		}

		// F_i_[j]
		if (F_i_[j]<128) {
 		    *cp++ = F_i_[j];
		} else {
		    *cp++ = 128 | (F_i_[j]>>8);
		    *cp++ = F_i_[j]&0xff;
		}

		RansEncSymbolInit(&syms[i][j], x, F_i_[j], TF_SHIFT_O1);
		x += F_i_[j];
	    }
	}
	*cp++ = 0;
    }
    *cp++ = 0;

    //write(2, out+4, cp-(out+4));
    tab_size = cp - out;
    assert(tab_size < 257*257*3);
    
    for (z = 0; z < NX; z++)
      RansEncInit(&ransN[z]);

    uint8_t* ptr = out_end;

    int isz4 = in_size/NX;
    int iN[NX];
    for (z = 0; z < NX; z++)
	iN[z] = (z+1)*isz4-2;

    unsigned char lN[NX];
    for (z = 0; z < NX; z++)
	lN[z] = in[iN[z]+1];

    // Deal with the remainder
    z = NX-1;
    lN[z] = in[in_size-1];
    for (iN[z] = in_size-2; iN[z] > NX*isz4-2; iN[z]--) {
	unsigned char c = in[iN[z]];
	RansEncPutSymbol(&ransN[z], &ptr, &syms[c][lN[z]]);
	lN[z] = c;
    }

#if 0
    for (; iN[0] >= 0; ) {
	uint32_t c[NX];
	RansEncSymbol *sN[NX];
	for (z = 0; z < NX; z++)
	    sN[z] = &syms[c[z] = in[iN[z]]][lN[z]];
	for (z = NX-1; z >= 0; z--)
	    RansEncPutSymbol(&ransN[z], &ptr, sN[z]);
	for (z = 0; z < NX; z++) {
	    lN[z] = c[z];
	    iN[z]--;
	}
    }
#else
    uint16_t *ptr16 = (uint16_t *)ptr;

    LOAD(Rv, ransN);

    for (; iN[0] >= 0; ) {
	uint32_t c[NX];

	// Gather all the symbol values together in adjacent arrays.
	// Better to just use raw set?
	RansEncSymbol *sN[NX];
	for (z = 0; z < NX; z++)
	    sN[z] = &syms[c[z] = in[iN[z]]][lN[z]];

#define SET1x(a,b,x) __m256i a##1 = _mm256_set_epi32(b[ 7]->x, b[ 6]->x, b[ 5]->x, b[ 4]->x, b[ 3]->x, b[ 2]->x, b[ 1]->x, b[ 0]->x)
#define SET2x(a,b,x) __m256i a##2 = _mm256_set_epi32(b[15]->x, b[14]->x, b[13]->x, b[12]->x, b[11]->x, b[10]->x, b[ 9]->x, b[ 8]->x)
#define SET3x(a,b,x) __m256i a##3 = _mm256_set_epi32(b[23]->x, b[22]->x, b[21]->x, b[20]->x, b[19]->x, b[18]->x, b[17]->x, b[16]->x)
#define SET4x(a,b,x) __m256i a##4 = _mm256_set_epi32(b[31]->x, b[30]->x, b[29]->x, b[28]->x, b[27]->x, b[26]->x, b[25]->x, b[24]->x)
#define SETx(a,b,x) SET1x(a,b,x);SET2x(a,b,x);SET3x(a,b,x);SET4x(a,b,x)

	// ------------------------------------------------------------
	//	for (z = NX-1; z >= 0; z--) {
	//	    if (ransN[z] >= x_max[z]) {
	//		*--ptr16 = ransN[z] & 0xffff;
	//		ransN[z] >>= 16;
	//	    }
	//	}
	//LOAD(xmax,x_max);
	SETx(xmax, sN, x_max);
        __m256i cv1 = _mm256_cmpgt_epi32(Rv1, xmax1);
        __m256i cv2 = _mm256_cmpgt_epi32(Rv2, xmax2);
        __m256i cv3 = _mm256_cmpgt_epi32(Rv3, xmax3);
        __m256i cv4 = _mm256_cmpgt_epi32(Rv4, xmax4);

        // Store bottom 16-bits at ptr16                                                     
        //                                                                                   
        // for (z = NX-1; z >= 0; z--) {                                                     
        //     if (cond[z]) *--ptr16 = (uint16_t )(ransN[z] & 0xffff);                       
        // }                                                                                 
        unsigned int imask1 = _mm256_movemask_ps((__m256)cv1);
        unsigned int imask2 = _mm256_movemask_ps((__m256)cv2);
        unsigned int imask3 = _mm256_movemask_ps((__m256)cv3);
        unsigned int imask4 = _mm256_movemask_ps((__m256)cv4);

        __m256i idx1 = _mm256_load_si256((const __m256i*)permutec[imask1]);
        __m256i idx2 = _mm256_load_si256((const __m256i*)permutec[imask2]);
        __m256i idx3 = _mm256_load_si256((const __m256i*)permutec[imask3]);
        __m256i idx4 = _mm256_load_si256((const __m256i*)permutec[imask4]);

        // Permute; to gather together the rans states that need flushing                    
        __m256i V1 = _mm256_permutevar8x32_epi32(_mm256_and_si256(Rv1, cv1), idx1);
        __m256i V2 = _mm256_permutevar8x32_epi32(_mm256_and_si256(Rv2, cv2), idx2);
        __m256i V3 = _mm256_permutevar8x32_epi32(_mm256_and_si256(Rv3, cv3), idx3);
        __m256i V4 = _mm256_permutevar8x32_epi32(_mm256_and_si256(Rv4, cv4), idx4);

        // We only flush bottom 16 bits, to squash 32-bit states into 16 bit.                
        V1 = _mm256_and_si256(V1, _mm256_set1_epi32(0xffff));
        V2 = _mm256_and_si256(V2, _mm256_set1_epi32(0xffff));
        V3 = _mm256_and_si256(V3, _mm256_set1_epi32(0xffff));
        V4 = _mm256_and_si256(V4, _mm256_set1_epi32(0xffff));
        __m256i V12 = _mm256_packus_epi32(V1, V2);
        __m256i V34 = _mm256_packus_epi32(V3, V4);

        // It's BAba order, want BbAa so shuffle.                                            
        V12 = _mm256_permute4x64_epi64(V12, 0xd8);
        V34 = _mm256_permute4x64_epi64(V34, 0xd8);
        // Now we have bottom N 16-bit values in each V12/V34 to flush                       
        __m128i f =  _mm256_extractf128_si256(V34, 1);
        _mm_storeu_si128((__m128i *)(ptr16-8), f);
        ptr16 -= _mm_popcnt_u32(imask4);

        f =  _mm256_extractf128_si256(V34, 0);
        _mm_storeu_si128((__m128i *)(ptr16-8), f);
        ptr16 -= _mm_popcnt_u32(imask3);

        f =  _mm256_extractf128_si256(V12, 1);
        _mm_storeu_si128((__m128i *)(ptr16-8), f);
        ptr16 -= _mm_popcnt_u32(imask2);

        f =  _mm256_extractf128_si256(V12, 0);
        _mm_storeu_si128((__m128i *)(ptr16-8), f);
        ptr16 -= _mm_popcnt_u32(imask1);

        __m256i Rs;
        Rs = _mm256_srli_epi32(Rv1, 16); Rv1 = _mm256_blendv_epi8(Rv1, Rs, cv1);
        Rs = _mm256_srli_epi32(Rv2, 16); Rv2 = _mm256_blendv_epi8(Rv2, Rs, cv2);
        Rs = _mm256_srli_epi32(Rv3, 16); Rv3 = _mm256_blendv_epi8(Rv3, Rs, cv3);
        Rs = _mm256_srli_epi32(Rv4, 16); Rv4 = _mm256_blendv_epi8(Rv4, Rs, cv4);

	// ------------------------------------------------------------
	// uint32_t q = (uint32_t) (((uint64_t)ransN[z] * rcp_freq[z]) >> rcp_shift[z]);
	// ransN[z] = ransN[z] + bias[z] + q * cmpl_freq[z];

        // Cannot trivially replace the multiply as mulhi_epu32 doesn't exist (only mullo).  
        // However we can use _mm256_mul_epu32 twice to get 64bit results (half our lanes)   
        // and shift/or to get the answer.                                                   
        //                                                                                   
        // (AVX512 allows us to hold it all in 64-bit lanes and use mullo_epi64              
        // plus a shift.  KNC has mulhi_epi32, but not sure if this is available.)           

	SETx(rfv,   sN, rcp_freq);

        rfv1 = _mm256_mulhi_epu32(Rv1, rfv1);
        rfv2 = _mm256_mulhi_epu32(Rv2, rfv2);
        rfv3 = _mm256_mulhi_epu32(Rv3, rfv3);
        rfv4 = _mm256_mulhi_epu32(Rv4, rfv4);

	SETx(SDv,   sN, SD);

        __m256i shiftv1 = _mm256_srli_epi32(SDv1, 16);
        __m256i shiftv2 = _mm256_srli_epi32(SDv2, 16);
        __m256i shiftv3 = _mm256_srli_epi32(SDv3, 16);
        __m256i shiftv4 = _mm256_srli_epi32(SDv4, 16);

        shiftv1 = _mm256_sub_epi32(shiftv1, _mm256_set1_epi32(32));
        shiftv2 = _mm256_sub_epi32(shiftv2, _mm256_set1_epi32(32));
        shiftv3 = _mm256_sub_epi32(shiftv3, _mm256_set1_epi32(32));
        shiftv4 = _mm256_sub_epi32(shiftv4, _mm256_set1_epi32(32));

        __m256i qv1 = _mm256_srlv_epi32(rfv1, shiftv1);
        __m256i qv2 = _mm256_srlv_epi32(rfv2, shiftv2);

        __m256i freqv1 = _mm256_and_si256(SDv1, _mm256_set1_epi32(0xffff));
        __m256i freqv2 = _mm256_and_si256(SDv2, _mm256_set1_epi32(0xffff));
        qv1 = _mm256_mullo_epi32(qv1, freqv1);
        qv2 = _mm256_mullo_epi32(qv2, freqv2);

        __m256i qv3 = _mm256_srlv_epi32(rfv3, shiftv3);
        __m256i qv4 = _mm256_srlv_epi32(rfv4, shiftv4);

        __m256i freqv3 = _mm256_and_si256(SDv3, _mm256_set1_epi32(0xffff));
        __m256i freqv4 = _mm256_and_si256(SDv4, _mm256_set1_epi32(0xffff));
        qv3 = _mm256_mullo_epi32(qv3, freqv3);
        qv4 = _mm256_mullo_epi32(qv4, freqv4);

	SETx(biasv, sN, bias);

        qv1 = _mm256_add_epi32(qv1, biasv1);
	qv2 = _mm256_add_epi32(qv2, biasv2);
        qv3 = _mm256_add_epi32(qv3, biasv3);
        qv4 = _mm256_add_epi32(qv4, biasv4);

	Rv1 = _mm256_add_epi32(Rv1, qv1);
        Rv2 = _mm256_add_epi32(Rv2, qv2);
        Rv3 = _mm256_add_epi32(Rv3, qv3);
        Rv4 = _mm256_add_epi32(Rv4, qv4);

	for (z = 0; z < NX; z++) {
	    //uint32_t q = (uint32_t) (((uint64_t)ransN[z] * rcp_freq[z]) >> rcp_shift[z]);
	    //ransN[z] = ransN[z] + bias[z] + q * cmpl_freq[z];
	    
	    lN[z] = c[z];
	    iN[z]--;
	}
    }

    STORE(Rv, ransN);

    ptr = (uint8_t *)ptr16;
#endif

    for (z = NX-1; z>=0; z--)
	RansEncPutSymbol(&ransN[z], &ptr, &syms[0][lN[z]]);

    for (z = NX-1; z>=0; z--)
	RansEncFlush(&ransN[z], &ptr);

    *out_size = (out_end - ptr) + tab_size;

    cp = out;
    *cp++ = (in_size>> 0) & 0xff;
    *cp++ = (in_size>> 8) & 0xff;
    *cp++ = (in_size>>16) & 0xff;
    *cp++ = (in_size>>24) & 0xff;

    memmove(out + tab_size, ptr, out_end-ptr);

    return out;
}

unsigned char *rans_uncompress_O1_32x16(unsigned char *in, unsigned int in_size,
				       unsigned char *out, unsigned int *out_size) {
    /* Load in the static tables */
    unsigned char *cp = in + 4;
    int i, j = -999, x, y, out_sz, rle_i, rle_j;
    uint32_t s3[256][TOTFREQ_O1] __attribute__((aligned(32)));
    
    //memset(D, 0, 256*sizeof(*D));

    out_sz = ((in[0])<<0) | ((in[1])<<8) | ((in[2])<<16) | ((in[3])<<24);
    if (!out) {
	out = malloc(out_sz);
	*out_size = out_sz;
    }
    if (!out || out_sz > *out_size)
	return NULL;

    //fprintf(stderr, "out_sz=%d\n", out_sz);

    //i = *cp++;
    rle_i = 0;
    i = *cp++;
    do {
	rle_j = x = y = 0;
	j = *cp++;
	do {
	    int F, C;
	    if ((F = *cp++) >= 128) {
		F &= ~128;
		F = ((F & 127) << 8) | *cp++;
	    }
	    C = x;

	    //fprintf(stderr, "i=%d j=%d F=%d C=%d\n", i, j, D[i].F[j], D[i].C[j]);

	    if (!F)
		F = TOTFREQ_O1;

	    for (y = 0; y < F; y++) {
		s3[i][y+C] = (((uint32_t)F)<<(TF_SHIFT_O1+8))|(y<<8)|j;
	    }

	    x += F;
	    assert(x <= TOTFREQ_O1);

	    if (!rle_j && j+1 == *cp) {
		j = *cp++;
		rle_j = *cp++;
	    } else if (rle_j) {
		rle_j--;
		j++;
	    } else {
		j = *cp++;
	    }
	} while(j);

	if (!rle_i && i+1 == *cp) {
	    i = *cp++;
	    rle_i = *cp++;
	} else if (rle_i) {
	    rle_i--;
	    i++;
	} else {
	    i = *cp++;
	}
    } while (i);

    // Precompute reverse lookup of frequency.

    RansState ransN[NX] __attribute__((aligned(32)));
    int z;
    uint8_t *ptr = cp;
    for (z = 0; z < NX; z++)
	RansDecInit(&ransN[z], &ptr);

    int isz4 = out_sz/NX;
    int lN[NX] = {0}, iN[NX];
    for (z = 0; z < NX; z++)
	iN[z] = z*isz4;

    uint16_t *sp = (uint16_t *)ptr;
    const uint32_t mask = (1u << TF_SHIFT_O1)-1;
    __m256i maskv  = _mm256_set1_epi32(mask);
    LOAD(Rv, ransN);
    LOAD(Lv, lN);

    union {
	unsigned char tbuf[32][32];
	uint64_t tbuf64[32][4];
    } u;
    int tidx = 0;

    for (; iN[0] < isz4; ) {
	// m[z] = ransN[z] & mask;
	__m256i masked1 = _mm256_and_si256(Rv1, maskv);
	__m256i masked2 = _mm256_and_si256(Rv2, maskv);

	//  S[z] = s3[lN[z]][m[z]];
	Lv1 = _mm256_slli_epi32(Lv1, TF_SHIFT_O1);
	masked1 = _mm256_add_epi32(masked1, Lv1);

	Lv2 = _mm256_slli_epi32(Lv2, TF_SHIFT_O1);
	masked2 = _mm256_add_epi32(masked2, Lv2);

	__m256i masked3 = _mm256_and_si256(Rv3, maskv);
	__m256i masked4 = _mm256_and_si256(Rv4, maskv);

	Lv3 = _mm256_slli_epi32(Lv3, TF_SHIFT_O1);
	masked3 = _mm256_add_epi32(masked3, Lv3);

	Lv4 = _mm256_slli_epi32(Lv4, TF_SHIFT_O1);
	masked4 = _mm256_add_epi32(masked4, Lv4);

	__m256i Sv1 = _mm256_i32gather_epi32x((int *)&s3[0][0], masked1, sizeof(s3[0][0]));
	__m256i Sv2 = _mm256_i32gather_epi32x((int *)&s3[0][0], masked2, sizeof(s3[0][0]));

	//  f[z] = S[z]>>(TF_SHIFT_O1+8);
	__m256i fv1 = _mm256_srli_epi32(Sv1, TF_SHIFT_O1+8);
	__m256i fv2 = _mm256_srli_epi32(Sv2, TF_SHIFT_O1+8);

	__m256i Sv3 = _mm256_i32gather_epi32x((int *)&s3[0][0], masked3, sizeof(s3[0][0]));
	__m256i Sv4 = _mm256_i32gather_epi32x((int *)&s3[0][0], masked4, sizeof(s3[0][0]));

	__m256i fv3 = _mm256_srli_epi32(Sv3, TF_SHIFT_O1+8);
	__m256i fv4 = _mm256_srli_epi32(Sv4, TF_SHIFT_O1+8);

	//  b[z] = (S[z]>>8) & mask;
	__m256i bv1 = _mm256_and_si256(_mm256_srli_epi32(Sv1, 8), maskv);
	__m256i bv2 = _mm256_and_si256(_mm256_srli_epi32(Sv2, 8), maskv);
	__m256i bv3 = _mm256_and_si256(_mm256_srli_epi32(Sv3, 8), maskv);
	__m256i bv4 = _mm256_and_si256(_mm256_srli_epi32(Sv4, 8), maskv);

	//  s[z] = S[z] & 0xff;
	__m256i sv1 = _mm256_and_si256(Sv1, _mm256_set1_epi32(0xff));
	__m256i sv2 = _mm256_and_si256(Sv2, _mm256_set1_epi32(0xff));
	__m256i sv3 = _mm256_and_si256(Sv3, _mm256_set1_epi32(0xff));
	__m256i sv4 = _mm256_and_si256(Sv4, _mm256_set1_epi32(0xff));

	//  R[z] = f[z] * (R[z] >> TF_SHIFT_O1) + b[z];
	Rv1 = _mm256_add_epi32(_mm256_mullo_epi32(_mm256_srli_epi32(Rv1,TF_SHIFT_O1),fv1),bv1);
	Rv2 = _mm256_add_epi32(_mm256_mullo_epi32(_mm256_srli_epi32(Rv2,TF_SHIFT_O1),fv2),bv2);


	//for (z = 0; z < NX; z++) lN[z] = c[z];
	Lv1 = sv1;
	Lv2 = sv2;

	sv1 = _mm256_packus_epi32(sv1, sv2);
	sv1 = _mm256_permute4x64_epi64(sv1, 0xd8);

	// Start loading next batch of normalised states
	__m256i Vv1 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i *)sp));

	sv1 = _mm256_packus_epi16(sv1, sv1);

	// out[iN[z]] = c[z];  // simulate scatter
	// RansDecRenorm(&ransN[z], &ptr);	
	__m256i renorm_mask1 = _mm256_xor_si256(Rv1, _mm256_set1_epi32(0x80000000));
	__m256i renorm_mask2 = _mm256_xor_si256(Rv2, _mm256_set1_epi32(0x80000000));

	renorm_mask1 = _mm256_cmpgt_epi32(_mm256_set1_epi32(RANS_BYTE_L-0x80000000), renorm_mask1);
	renorm_mask2 = _mm256_cmpgt_epi32(_mm256_set1_epi32(RANS_BYTE_L-0x80000000), renorm_mask2);

	unsigned int imask1 = _mm256_movemask_ps((__m256)renorm_mask1);
	__m256i idx1 = _mm256_load_si256((const __m256i*)permute[imask1]);
	__m256i Yv1 = _mm256_slli_epi32(Rv1, 16);
	__m256i Yv2 = _mm256_slli_epi32(Rv2, 16);

	unsigned int imask2 = _mm256_movemask_ps((__m256)renorm_mask2);
	Vv1 = _mm256_permutevar8x32_epi32(Vv1, idx1);
	sp += _mm_popcnt_u32(imask1);

	u.tbuf64[tidx][0] = _mm256_extract_epi64(sv1, 0);
	u.tbuf64[tidx][1] = _mm256_extract_epi64(sv1, 2);

	__m256i idx2 = _mm256_load_si256((const __m256i*)permute[imask2]);
	__m256i Vv2 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i *)sp));
	sp += _mm_popcnt_u32(imask2);
	Vv2 = _mm256_permutevar8x32_epi32(Vv2, idx2);

	//Vv = _mm256_and_si256(Vv, renorm_mask);  (blend does the AND anyway)
	Yv1 = _mm256_or_si256(Yv1, Vv1);
	Yv2 = _mm256_or_si256(Yv2, Vv2);

	Rv1 = _mm256_blendv_epi8(Rv1, Yv1, renorm_mask1);
	Rv2 = _mm256_blendv_epi8(Rv2, Yv2, renorm_mask2);

	/////////////////////////////////////////////////////////////////////

	// Start loading next batch of normalised states
	__m256i Vv3 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i *)sp));

	//  R[z] = f[z] * (R[z] >> TF_SHIFT_O1) + b[z];
	Rv3 = _mm256_add_epi32(_mm256_mullo_epi32(_mm256_srli_epi32(Rv3,TF_SHIFT_O1),fv3),bv3);
	Rv4 = _mm256_add_epi32(_mm256_mullo_epi32(_mm256_srli_epi32(Rv4,TF_SHIFT_O1),fv4),bv4);

	//for (z = 0; z < NX; z++) lN[z] = c[z];
	Lv3 = sv3;
	Lv4 = sv4;

	// out[iN[z]] = c[z];  // simulate scatter
	// RansDecRenorm(&ransN[z], &ptr);	
	__m256i renorm_mask3 = _mm256_xor_si256(Rv3, _mm256_set1_epi32(0x80000000));
	__m256i renorm_mask4 = _mm256_xor_si256(Rv4, _mm256_set1_epi32(0x80000000));

	renorm_mask3 = _mm256_cmpgt_epi32(_mm256_set1_epi32(RANS_BYTE_L-0x80000000), renorm_mask3);
	renorm_mask4 = _mm256_cmpgt_epi32(_mm256_set1_epi32(RANS_BYTE_L-0x80000000), renorm_mask4);

	__m256i Yv3 = _mm256_slli_epi32(Rv3, 16);
	__m256i Yv4 = _mm256_slli_epi32(Rv4, 16);

	unsigned int imask3 = _mm256_movemask_ps((__m256)renorm_mask3);
	unsigned int imask4 = _mm256_movemask_ps((__m256)renorm_mask4);
	__m256i idx3 = _mm256_load_si256((const __m256i*)permute[imask3]);
	sp += _mm_popcnt_u32(imask3);
	Vv3 = _mm256_permutevar8x32_epi32(Vv3, idx3);

	sv3 = _mm256_packus_epi32(sv3, sv4);
	sv3 = _mm256_permute4x64_epi64(sv3, 0xd8);
	sv3 = _mm256_packus_epi16(sv3, sv3);

	u.tbuf64[tidx][2] = _mm256_extract_epi64(sv3, 0);
	u.tbuf64[tidx][3] = _mm256_extract_epi64(sv3, 2);

	iN[0]++;
	if (++tidx == 32) {
	    iN[0]-=32;

	    for (z = 0; z < NX; z++) {
		// replace by gathers?
		*(uint64_t *)&out[iN[z]] =
		    ((uint64_t)(u.tbuf[0][z])<< 0) + ((uint64_t)(u.tbuf[1][z])<< 8) +
		    ((uint64_t)(u.tbuf[2][z])<<16) + ((uint64_t)(u.tbuf[3][z])<<24) +
		    ((uint64_t)(u.tbuf[4][z])<<32) + ((uint64_t)(u.tbuf[5][z])<<40) +
		    ((uint64_t)(u.tbuf[6][z])<<48) + ((uint64_t)(u.tbuf[7][z])<<56);
		*(uint64_t *)&out[iN[z]+8] =
		    ((uint64_t)(u.tbuf[8+0][z])<< 0) + ((uint64_t)(u.tbuf[8+1][z])<< 8) +
		    ((uint64_t)(u.tbuf[8+2][z])<<16) + ((uint64_t)(u.tbuf[8+3][z])<<24) +
		    ((uint64_t)(u.tbuf[8+4][z])<<32) + ((uint64_t)(u.tbuf[8+5][z])<<40) +
		    ((uint64_t)(u.tbuf[8+6][z])<<48) + ((uint64_t)(u.tbuf[8+7][z])<<56);
		*(uint64_t *)&out[iN[z]+16] =
		    ((uint64_t)(u.tbuf[16+0][z])<< 0) + ((uint64_t)(u.tbuf[16+1][z])<< 8) +
		    ((uint64_t)(u.tbuf[16+2][z])<<16) + ((uint64_t)(u.tbuf[16+3][z])<<24) +
		    ((uint64_t)(u.tbuf[16+4][z])<<32) + ((uint64_t)(u.tbuf[16+5][z])<<40) +
		    ((uint64_t)(u.tbuf[16+6][z])<<48) + ((uint64_t)(u.tbuf[16+7][z])<<56);
		*(uint64_t *)&out[iN[z]+24] =
		    ((uint64_t)(u.tbuf[24+0][z])<< 0) + ((uint64_t)(u.tbuf[24+1][z])<< 8) +
		    ((uint64_t)(u.tbuf[24+2][z])<<16) + ((uint64_t)(u.tbuf[24+3][z])<<24) +
		    ((uint64_t)(u.tbuf[24+4][z])<<32) + ((uint64_t)(u.tbuf[24+5][z])<<40) +
		    ((uint64_t)(u.tbuf[24+6][z])<<48) + ((uint64_t)(u.tbuf[24+7][z])<<56);
		iN[z] += 32;
	    }

	    tidx = 0;
	}

	__m256i idx4 = _mm256_load_si256((const __m256i*)permute[imask4]);
	__m256i Vv4 = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i *)sp));

	//Vv = _mm256_and_si256(Vv, renorm_mask);  (blend does the AND anyway)
	Yv3 = _mm256_or_si256(Yv3, Vv3);
	Vv4 = _mm256_permutevar8x32_epi32(Vv4, idx4);
	Yv4 = _mm256_or_si256(Yv4, Vv4);

	sp += _mm_popcnt_u32(imask4);

	Rv3 = _mm256_blendv_epi8(Rv3, Yv3, renorm_mask3);
	Rv4 = _mm256_blendv_epi8(Rv4, Yv4, renorm_mask4);
    }

    STORE(Rv, ransN);
    //_mm256_store_si256((__m256i *)&ransN[ 0], Rv1);
    //_mm256_store_si256((__m256i *)&ransN[ 8], Rv2);
    //_mm256_store_si256((__m256i *)&ransN[16], Rv3);
    //_mm256_store_si256((__m256i *)&ransN[24], Rv4);
    STORE(Lv, lN);
    //_mm256_store_si256((__m256i *)&lN[ 0], Lv1);
    //_mm256_store_si256((__m256i *)&lN[ 8], Lv2);
    //_mm256_store_si256((__m256i *)&lN[16], Lv3);
    //_mm256_store_si256((__m256i *)&lN[24], Lv4);
    ptr = (uint8_t *)sp;

    if (1) {
	iN[0]-=tidx;
	int T;
	for (z = 0; z < NX; z++)
	    for (T = 0; T < tidx; T++)
		out[iN[z]++] = u.tbuf[T][z];
    }

    // Remainder
    z = NX-1;
    for (; iN[z] < out_sz; ) {
	uint32_t m = ransN[z] & ((1u<<TF_SHIFT_O1)-1);
	uint32_t S = s3[lN[z]][m];
	unsigned char c = S & 0xff;
	out[iN[z]++] = c;
	ransN[z] = (S>>(TF_SHIFT_O1+8)) * (ransN[z]>>TF_SHIFT_O1) +
	    ((S>>8) & ((1u<<TF_SHIFT_O1)-1));
	RansDecRenorm(&ransN[z], &ptr);
	lN[z] = c;
    }
    
    *out_size = out_sz;

    return out;
}

/*-----------------------------------------------------------------------------
 * Simple interface to the order-0 vs order-1 encoders and decoders.
 */
unsigned char *rans_compress_to_32x16(unsigned char *in,  unsigned int in_size,
				     unsigned char *out, unsigned int *out_size,
				     int order) {
    return order
	? rans_compress_O1_32x16(in, in_size, out, out_size)
	: rans_compress_O0_32x16(in, in_size, out, out_size);
}

unsigned char *rans_compress_32x16(unsigned char *in, unsigned int in_size,
				  unsigned int *out_size, int order) {
    return rans_compress_to_32x16(in, in_size, NULL, out_size, order);
}

unsigned char *rans_uncompress_to_32x16(unsigned char *in,  unsigned int in_size,
				       unsigned char *out, unsigned int *out_size,
				       int order) {
    return order
	? rans_uncompress_O1_32x16(in, in_size, out, out_size)
	: rans_uncompress_O0_32x16(in, in_size, out, out_size);
}

unsigned char *rans_uncompress_32x16(unsigned char *in, unsigned int in_size,
				    unsigned int *out_size, int order) {
    return rans_uncompress_to_32x16(in, in_size, NULL, out_size, order);
}


#ifdef TEST_MAIN

// NB: Best at a non-power of 2 for order-1 coding, eg 507*1121 or 1013*1041
#ifndef BLK_SIZE
#  define BLK_SIZE 1024*1024
#endif

// Room to allow for expanded BLK_SIZE on worst case compression.
#define BLK_SIZE2 ((int)(1.05*BLK_SIZE))

/*-----------------------------------------------------------------------------
 * Main
 */
int main(int argc, char **argv) {
    int opt, order = 0;
    unsigned char in_buf[BLK_SIZE2+257*257*3];
    int decode = 0, test = 0;
    FILE *infp = stdin, *outfp = stdout;
    struct timeval tv1, tv2, tv3, tv4;
    size_t bytes = 0;

    extern char *optarg;
    extern int optind;

    while ((opt = getopt(argc, argv, "o:dt")) != -1) {
	switch (opt) {
	case 'o':
	    order = atoi(optarg);
	    break;

	case 'd':
	    decode = 1;
	    break;
	    
	case 't':
	    test = 1;
	    break;
	}
    }

    order = order ? 1 : 0; // Only support O(0) and O(1)

    if (optind < argc) {
	if (!(infp = fopen(argv[optind], "rb"))) {
	    perror(argv[optind]);
	    return 1;
	}
	optind++;
    }

    if (optind < argc) {
	if (!(outfp = fopen(argv[optind], "wb"))) {
	    perror(argv[optind]);
	    return 1;
	}
	optind++;
    }

    gettimeofday(&tv1, NULL);

    if (test) {
	size_t len, in_sz = 0, out_sz = 0;
	typedef struct {
	    unsigned char *blk;
	    uint32_t sz;
	} blocks;
	blocks *b = NULL, *bc = NULL, *bu = NULL;
	int nb = 0, i;
	
	while ((len = fread(in_buf, 1, BLK_SIZE, infp)) != 0) {
	    // inefficient, but it'll do for testing
	    b = realloc(b, (nb+1)*sizeof(*b));
	    bu = realloc(bu, (nb+1)*sizeof(*bu));
	    bc = realloc(bc, (nb+1)*sizeof(*bc));
	    b[nb].blk = malloc(len);
	    b[nb].sz = len;
	    memcpy(b[nb].blk, in_buf, len);
	    bc[nb].sz = rans_compress_bound_32x16(BLK_SIZE, order, NULL);
	    bc[nb].blk = malloc(bc[nb].sz);
	    bu[nb].sz = BLK_SIZE;
	    bu[nb].blk = malloc(BLK_SIZE);
	    nb++;
	    in_sz += len;
	}

#ifndef NTRIALS
#  define NTRIALS 10
#endif
	int trials = NTRIALS;
	while (trials--) {
	    // Warmup
	    for (i = 0; i < nb; i++) memset(bc[i].blk, 0, bc[i].sz);

	    gettimeofday(&tv1, NULL);

	    out_sz = 0;
	    for (i = 0; i < nb; i++) {
		unsigned int csz = bc[i].sz;
		bc[i].blk = rans_compress_to_32x16(b[i].blk, b[i].sz, bc[i].blk, &csz, order);
		assert(csz <= bc[i].sz);
		out_sz += 5 + csz;
	    }

	    gettimeofday(&tv2, NULL);

	    // Warmup
	    for (i = 0; i < nb; i++) memset(bu[i].blk, 0, BLK_SIZE);

	    gettimeofday(&tv3, NULL);

	    for (i = 0; i < nb; i++)
		bu[i].blk = rans_uncompress_to_32x16(bc[i].blk, bc[i].sz, bu[i].blk, &bu[i].sz, order);

	    gettimeofday(&tv4, NULL);

	    for (i = 0; i < nb; i++) {
	      if (b[i].sz != bu[i].sz || memcmp(b[i].blk, bu[i].blk, b[i].sz)) {
		int z;
		for (z = 0; z < b[i].sz; z++)
		  if (b[i].blk[z] != bu[i].blk[z])
		    break;
		fprintf(stderr, "Mismatch in block %d, sz %d/%d, at %d\n", i, b[i].sz, bu[i].sz, z);
	      }
		//free(bc[i].blk);
		//free(bu[i].blk);
	    }

	    fprintf(stderr, "%5.1f MB/s enc, %5.1f MB/s dec\t %lld bytes -> %lld bytes\n",
		    (double)in_sz / ((long)(tv2.tv_sec - tv1.tv_sec)*1000000 +
				     tv2.tv_usec - tv1.tv_usec),
		    (double)in_sz / ((long)(tv4.tv_sec - tv3.tv_sec)*1000000 +
				     tv4.tv_usec - tv3.tv_usec),
		    (long long)in_sz, (long long)out_sz);
	}

	exit(0);
	
    }

    if (decode) {
	// Only used in some test implementations of RC_GetFreq()
	//RC_init();
	//RC_init2();

	for (;;) {
	    uint32_t in_size, out_size;
	    unsigned char *out;

	    order = fgetc(infp);
	    if (4 != fread(&in_size, 1, 4, infp))
		break;
	    if (in_size != fread(in_buf, 1, in_size, infp)) {
		fprintf(stderr, "Truncated input\n");
		exit(1);
	    }
	    out = rans_uncompress_32x16(in_buf, in_size, &out_size, order);
	    if (!out)
		abort();

	    fwrite(out, 1, out_size, outfp);
	    free(out);

	    bytes += out_size;
	}
    } else {
	for (;;) {
	    uint32_t in_size, out_size;
	    unsigned char *out;

	    in_size = fread(in_buf, 1, BLK_SIZE, infp);
	    if (in_size <= 0)
		break;

	    out = rans_compress_32x16(in_buf, in_size, &out_size,
				      order && in_size >= 4);

	    fputc(order && in_size >= 4, outfp);
	    fwrite(&out_size, 1, 4, outfp);
	    fwrite(out, 1, out_size, outfp);
	    free(out);

	    bytes += in_size;
	}
    }

    gettimeofday(&tv2, NULL);

    fprintf(stderr, "Took %ld microseconds, %5.1f MB/s\n",
	    (long)(tv2.tv_sec - tv1.tv_sec)*1000000 +
	    tv2.tv_usec - tv1.tv_usec,
	    (double)bytes / ((long)(tv2.tv_sec - tv1.tv_sec)*1000000 +
			     tv2.tv_usec - tv1.tv_usec));
    return 0;
}
#endif
