// Hard coded 16 way 16-bit normalisation.
// This is the same logic as r16N with NX=16, but coded in AVX512 intrinsics directly.

/*
[sanger@node03 rans_static]$ icc -O3 -g -xMIC-AVX512 r16_16.c -DTEST_MAIN -DNX=16
[sanger@node03 rans_static]$ ./a.out -t -o0 ~/jkb/q8;echo;./a.out -t -o0 ~/jkb/q40
 75.2 MB/s enc, 559.7 MB/s dec   73124567 bytes -> 16854740 bytes
 76.4 MB/s enc, 564.3 MB/s dec   73124567 bytes -> 16854740 bytes
 74.6 MB/s enc, 545.2 MB/s dec   73124567 bytes -> 16854740 bytes
 76.3 MB/s enc, 567.0 MB/s dec   73124567 bytes -> 16854740 bytes
 72.5 MB/s enc, 558.2 MB/s dec   73124567 bytes -> 16854740 bytes
 73.0 MB/s enc, 530.3 MB/s dec   73124567 bytes -> 16854740 bytes
 75.4 MB/s enc, 562.5 MB/s dec   73124567 bytes -> 16854740 bytes
 75.9 MB/s enc, 564.2 MB/s dec   73124567 bytes -> 16854740 bytes
 75.6 MB/s enc, 533.0 MB/s dec   73124567 bytes -> 16854740 bytes
 76.5 MB/s enc, 564.5 MB/s dec   73124567 bytes -> 16854740 bytes

 62.9 MB/s enc, 546.8 MB/s dec   94602182 bytes -> 53697503 bytes
 61.8 MB/s enc, 503.2 MB/s dec   94602182 bytes -> 53697503 bytes
 62.2 MB/s enc, 490.4 MB/s dec   94602182 bytes -> 53697503 bytes
 62.9 MB/s enc, 540.9 MB/s dec   94602182 bytes -> 53697503 bytes
 61.9 MB/s enc, 505.1 MB/s dec   94602182 bytes -> 53697503 bytes
 62.9 MB/s enc, 548.6 MB/s dec   94602182 bytes -> 53697503 bytes
 62.9 MB/s enc, 541.7 MB/s dec   94602182 bytes -> 53697503 bytes
 62.7 MB/s enc, 481.3 MB/s dec   94602182 bytes -> 53697503 bytes
 62.9 MB/s enc, 547.9 MB/s dec   94602182 bytes -> 53697503 bytes
 63.0 MB/s enc, 548.5 MB/s dec   94602182 bytes -> 53697503 bytes
 */

#include <x86intrin.h>

/*-------------------------------------------------------------------------- */
/* rans_byte.h from https://github.com/rygorous/ryg_rans */

// Simple byte-aligned rANS encoder/decoder - public domain - Fabian 'ryg' Giesen 2014
//
// Not intended to be "industrial strength"; just meant to illustrate the general
// idea.

#ifndef NX
#define NX 16
#endif

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
    uint32_t x_max = ((RANS_BYTE_L >> scale_bits) << 8) * freq; // this turns into a shift.
    if (x >= x_max) {
        uint8_t* ptr = *pptr;
        do {
            *--ptr = (uint8_t) (x & 0xff);
            x >>= 8;
        } while (x >= x_max);
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
    uint16_t cmpl_freq; // Complement of frequency: (1 << scale_bits) - freq
    uint16_t rcp_shift; // Reciprocal shift

    // FIXME: temporary
    uint16_t scale_bits;
    uint16_t freq;
    uint16_t start;
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
    
    // FIXME: temporary
    s->scale_bits = scale_bits;
    s->freq = freq;
    s->start = start;

    s->x_max = ((RANS_BYTE_L >> scale_bits) << 16) * freq;
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
    RansAssert(sym->x_max != 0); // can't encode symbol with freq=0

    // renormalize
    uint32_t x = *r;
    uint32_t x_max = sym->x_max;

    if (x >= x_max) {
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

#define TF_SHIFT 12
#define TOTFREQ (1<<TF_SHIFT)

// 9 is considerably faster in the O1sfb variant due to reduced table size.
#define TF_SHIFT_O1 9
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
    int i, i8 = in_size & ~7;
    for (i = 0; i < i8; i+=8) {
	F0[in[i+0]]++;
	F1[in[i+1]]++;
	F2[in[i+2]]++;
	F3[in[i+3]]++;
	F4[in[i+4]]++;
	F5[in[i+5]]++;
	F6[in[i+6]]++;
	F7[in[i+7]]++;
    }
    while (i < in_size)
	F0[in[i++]]++;

    for (i = 0; i < 256; i++)
	F0[i] += F1[i] + F2[i] + F3[i] + F4[i] + F5[i] + F6[i] + F7[i];
}

unsigned int rans_compress_bound_4x16(unsigned int size, int order, int *tab) {
  int tabsz = order == 0
    ?     257*3 + 4 + NX*4 + NX*4
    : 257*257*3 + 4 + NX*4 + NX*4;
  if (tab) *tab = tabsz;
  return 1.05*size + tabsz;
}

unsigned char *rans_compress_O0_4x16(unsigned char *in, unsigned int in_size,
				     unsigned char *out, unsigned int *out_size) {
  unsigned char *cp, *out_endN[NX];
    RansEncSymbol syms[256];
    RansState ransN[NX];
    uint8_t* ptrN[NX];
    int F[256+MAGIC] = {0}, i, j, tab_size, rle, x, fsum = 0;
    int m = 0, M = 0;
    int tabsz;
    int bound = rans_compress_bound_4x16(in_size,0,&tabsz), z;

    if (!out) {
	*out_size = bound;
	out = malloc(*out_size);
    }
    if (!out || bound > *out_size) {
	return NULL;
    }

    for (z = 0; z < NX; z++)
      ptrN[z] = out_endN[z] = out + tabsz + (z+1)*(uint64_t)(bound-tabsz)/NX;

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
    cp = out+4 + NX*4;

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
      RansEncPutSymbol(&ransN[z], &ptrN[z], &syms[in[in_size-(i-z)]]);

    for (i=(in_size &~(NX-1)); i>0; i-=NX) {
      for (z = 0; z < NX; z++) {
	RansEncSymbol *s = &syms[in[i-(NX-z)]];
	RansEncPutSymbol(&ransN[z], &ptrN[z], s);
      }
    }

    for (z = 0; z < NX; z++)
      RansEncFlush(&ransN[z], &ptrN[z]);

    // Finalise block size and return it
    *out_size = tab_size;
    for (z = 0; z < NX; z++)
      *out_size += out_endN[z] - ptrN[z];

    cp = out;
    *cp++ = (in_size>> 0) & 0xff;
    *cp++ = (in_size>> 8) & 0xff;
    *cp++ = (in_size>>16) & 0xff;
    *cp++ = (in_size>>24) & 0xff;

    for (z = 0; z < NX; z++) {
      // ensure no overflows
      assert( (z == 0 && ptrN[0] > out+tab_size) || (ptrN[z] > out_endN[z-1]) );
      *cp++ = ((out_endN[z] - ptrN[z]) >> 0) & 0xff;
      *cp++ = ((out_endN[z] - ptrN[z]) >> 8) & 0xff;
      *cp++ = ((out_endN[z] - ptrN[z]) >> 16) & 0xff;
      *cp++ = ((out_endN[z] - ptrN[z]) >> 24) & 0xff;
    }

    cp = out + tab_size;
    for (z = 0; z < NX; z++) {
      memmove(cp, ptrN[z], out_endN[z]-ptrN[z]);
      cp += out_endN[z]-ptrN[z];
    }

    return out;
}

typedef struct {
    unsigned char R[TOTFREQ];
} ari_decoder;

unsigned char *rans_uncompress_O0_4x16(unsigned char *in, unsigned int in_size,
				       unsigned char *out, unsigned int *out_size) {
    /* Load in the static tables */
    unsigned char *cp = in + 4 + NX*4;
    int i, j, x, y, out_sz, rle;
    //uint16_t sfreq[TOTFREQ+32];
    //uint16_t sbase[TOTFREQ+32]; // faster to use 32-bit on clang
    uint8_t  ssym [TOTFREQ+64]; // faster to use 16-bit on clang

    uint32_t s3[TOTFREQ]; // For TF_SHIFT <= 12

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
    unsigned char *cpN[NX];
    cpN[0] = cp;
    for (z = 1; z < NX; z++)
      cpN[z] = cpN[z-1]
	+ (in[4+(z-1)*4+0]<<0)
	+ (in[4+(z-1)*4+1]<<8)
	+ (in[4+(z-1)*4+2]<<16)
	+ (in[4+(z-1)*4+3]<<24);

    RansState R[NX];
    for (z = 0; z < NX; z++)
      RansDecInit(&R[z], &cpN[z]);

    int out_end = (out_sz&~(NX-1));
    const uint32_t mask = (1u << TF_SHIFT)-1;

    uint16_t *spN[NX];
    for (z = 0; z < NX; z++)
      spN[z] = (uint16_t *)cpN[z];

    uint32_t idx[NX];
    for (z = 0; z < NX; z++)
      //idx[z] = 0; // with spN[z][idx[z]] faster on avx512, but not core2-avx
      idx[z] = spN[z]-spN[0]; // use with spN[0][idx[z]]; faster on core2-avx
    uint16_t *sp = spN[0];


#if 1
    __m512i maskv = _mm512_set1_epi32(mask); // set mask in all lanes
    __m512i Rz = _mm512_load_epi32(R);
    __m512i indices = _mm512_load_epi32(idx); // offsets from cpN[0]

    __m512i masked = _mm512_and_epi32(Rz, maskv);
    __m512i S = _mm512_i32gather_epi32(masked, (int *)s3, sizeof(*s3));
    for (i=0; i < out_end; i+=NX) {
      //for (z = 0; z < 16; z++) {  Implict loop, now vectorized 

      // For V[z]=sp[idx[z]] below; gather now so AND later isn't delayed
      __m512i V = _mm512_i32gather_epi32(indices, (int *)sp, sizeof(*sp));

      //uint16_t f = S>>(TF_SHIFT+8), b = (S>>8) & mask;
      __m512i f = _mm512_srli_epi32(S, TF_SHIFT+8);
      __m512i b = _mm512_and_epi32(_mm512_srli_epi32(S, 8), maskv);

      //R[z] = f * (R[z] >> TF_SHIFT) + b;
      Rz = _mm512_add_epi32(_mm512_mullo_epi32(_mm512_srli_epi32(Rz, TF_SHIFT), f), b);

      //out[i+z] = S;
      _mm_storeu_si128((__m128i *)(out+i), _mm512_cvtepi32_epi8(S));

      // V[z] = sp[idx[z]];
      V = _mm512_and_epi32(V, _mm512_set1_epi32(0xffff)); // want 16-bit components only.

      //R[z]    = R[z] < RANS_BYTE_L ? (R[z] << 16) | V[z] : R[z];
      // aka:  R[z] = R[z] < RANS_BYTE_L ? R[z]<<16  : R[z];
      //       R[z] = R[z] < RANS_BYTE_L ? R[z]|V[z] : R[z];
      __mmask16 renorm_mask = _mm512_cmplt_epu32_mask(Rz, _mm512_set1_epi32(RANS_BYTE_L));
      Rz = _mm512_mask_slli_epi32(Rz, renorm_mask, Rz, 16);
      Rz = _mm512_mask_or_epi32(Rz, renorm_mask, Rz, V);
      
      // Start the gather running for next loop iteration.
      //uint32_t S = s3[R[z] & mask];
      masked = _mm512_and_epi32(Rz, maskv);
      S = _mm512_i32gather_epi32(masked, (int *)s3, sizeof(*s3));

      //idx[z] += R[z] < RANS_BYTE_L ? 1 : 0;
      indices = _mm512_mask_add_epi32(indices, renorm_mask, indices, _mm512_set1_epi32(1));
    }      
    _mm512_store_epi32(R, Rz);

#else
    // AVX2
    __m256i mask1 = _mm256_set1_epi32(0xffffffff);
    __m256 maskv  = (__m256)_mm256_set1_epi32(mask); // set mask in all lanes
    __m256i Rz1 = _mm256_maskload_epi32(&R[0], mask1);
    __m256i Rz2 = _mm256_maskload_epi32(&R[8], mask1);
    __m256i indices1 = _mm256_maskload_epi32(&idx[0], mask1); // offsets from cpN[0]
    __m256i indices2 = _mm256_maskload_epi32(&idx[8], mask1); // offsets from cpN[0]

    __m256i masked1 = (__m256i)_mm256_and_ps((__m256)Rz1, maskv);
    __m256i masked2 = (__m256i)_mm256_and_ps((__m256)Rz2, maskv);
    __m256i S1 = _mm256_i32gather_epi32(s3, masked1, sizeof(*s3));
    __m256i S2 = _mm256_i32gather_epi32(s3, masked2, sizeof(*s3));
    for (i=0; i < out_end; i+=16) {
      //for (z = 0; z < 16; z++) {  Implict loop, now vectorized 

      // For V[z]=sp[idx[z]] below; gather now so AND later isn't delayed
      __m256i V1 = _mm256_i32gather_epi32((int *)sp, indices1, sizeof(*sp));
      __m256i V2 = _mm256_i32gather_epi32((int *)sp, indices2, sizeof(*sp));

      //uint16_t f = S>>(TF_SHIFT+8), b = (S>>8) & mask;
      __m256i f1 = _mm256_srli_epi32(S1, TF_SHIFT+8);
      __m256i f2 = _mm256_srli_epi32(S2, TF_SHIFT+8);
      __m256i b1 = (__m256i)_mm256_and_ps((__m256)_mm256_srli_epi32(S1, 8), maskv);
      __m256i b2 = (__m256i)_mm256_and_ps((__m256)_mm256_srli_epi32(S2, 8), maskv);

      //R[z] = f * (R[z] >> TF_SHIFT) + b;
      Rz1 = _mm256_add_epi32(_mm256_mullo_epi32(_mm256_srli_epi32(Rz1, TF_SHIFT), f1), b1);
      Rz2 = _mm256_add_epi32(_mm256_mullo_epi32(_mm256_srli_epi32(Rz2, TF_SHIFT), f2), b2);

      //out[i+z] = S;
      //FIXME: need AND + shuffle or AND + packs_epi32/16.
      S1 = (__m256i)_mm256_and_ps((__m256)S1, (__m256)_mm256_set1_epi32(0xff));
      S2 = (__m256i)_mm256_and_ps((__m256)S1, (__m256)_mm256_set1_epi32(0xff));
      __m256i P = _mm256_packs_epi16(S1, S2);
      P = _mm256_permute4x64_epi64(P, 0xd8);
      P = _mm256_packs_epi16(P, P);
      P = _mm256_permute4x64_epi64(P, 0xd8);
      _mm256_maskstore_epi32((int *)(out+i), _mm256_set_epi32(0,0,0,0,-1,-1,-1,-1), P);


      // V[z] = sp[idx[z]];
      V1 = (__m256i)_mm256_and_ps((__m256)V1, (__m256)_mm256_set1_epi32(0xffff)); // want 16-bit components only.
      V2 = (__m256i)_mm256_and_ps((__m256)V2, (__m256)_mm256_set1_epi32(0xffff)); // want 16-bit components only.

      //R[z]    = R[z] < RANS_BYTE_L ? (R[z] << 16) | V[z] : R[z];
      // aka:  R[z] = R[z] < RANS_BYTE_L ? R[z]<<16  : R[z];
      //       R[z] = R[z] < RANS_BYTE_L ? R[z]|V[z] : R[z];
      // FIXME: No _mm256_cmplt_epu32_mask on avx2
      // Need _mm256_cmpgt_epi32(Rz1, _mm256_set1_epi32(RANS_BYTE_L)-1); => 1 if >, 0 if <=
      // and then xor -1 to flip bits.  Also mask with me __mm256 not __mmask16.

      //__mmask16 renorm_mask1 = _mm256_cmplt_epu32_mask(Rz1, _mm256_set1_epi32(RANS_BYTE_L));
      //__mmask16 renorm_mask2 = _mm256_cmplt_epu32_mask(Rz2, _mm256_set1_epi32(RANS_BYTE_L));
      __m256i renorm_mask1 = _mm256_xor_si256(Rz1, _mm256_set1_epi32(0x80000000));
      renorm_mask1 = _mm256_cmpgt_epi32(_mm256_set1_epi32(RANS_BYTE_L-0x80000000), renorm_mask1);
      __m256i renorm_mask2 = _mm256_xor_si256(Rz2, _mm256_set1_epi32(0x80000000));
      renorm_mask2 = _mm256_cmpgt_epi32(_mm256_set1_epi32(RANS_BYTE_L-0x80000000), renorm_mask2);

      //Rz1 = _mm256_mask_slli_epi32(Rz1, renorm_mask1, Rz1, 16);
      //Rz2 = _mm256_mask_slli_epi32(Rz2, renorm_mask2, Rz2, 16);
      __m256i Rz1_shift = _mm256_slli_epi32(Rz1, 16);
      Rz1 = _mm256_blendv_epi8(Rz1, Rz1_shift, renorm_mask1);
      __m256i Rz2_shift = _mm256_slli_epi32(Rz2, 16);
      Rz2 = _mm256_blendv_epi8(Rz2, Rz2_shift, renorm_mask2);

      // mask_or_epi32: dst = mask ? a | b : src
      // We want Rz1 = mask ? Rz1 | V1 : Rz1
      // 1) V1 |= Rz1
      // 2) V1 &= renorm_mask
      //Rz1 = _mm256_mask_or_epi32(Rz1, renorm_mask1, Rz1, V1);
      //Rz2 = _mm256_mask_or_epi32(Rz2, renorm_mask2, Rz2, V2);
      __m256i Rz1_or = _mm256_or_si256(Rz1, V1);
      Rz1 = _mm256_blendv_epi8(Rz1, Rz1_or, renorm_mask1);
      __m256i Rz2_or = _mm256_or_si256(Rz2, V2);
      Rz2 = _mm256_blendv_epi8(Rz2, Rz2_or, renorm_mask2);
      
      // Start the gather running for next loop iteration.
      //uint32_t S = s3[R[z] & mask];
      masked1 = (__m256i)_mm256_and_ps((__m256)Rz1, maskv);
      masked2 = (__m256i)_mm256_and_ps((__m256)Rz2, maskv);
      S1 = _mm256_i32gather_epi32(s3, masked1, sizeof(*s3));
      S2 = _mm256_i32gather_epi32(s3, masked2, sizeof(*s3));

      //idx[z] += R[z] < RANS_BYTE_L ? 1 : 0;
      //indices1 = _mm256_mask_add_epi32(indices1, renorm_mask1, indices1, _mm256_set1_epi32(1));
      //indices2 = _mm256_mask_add_epi32(indices2, renorm_mask2, indices2, _mm256_set1_epi32(1));
      renorm_mask1 = _mm256_and_si256(renorm_mask1, _mm256_set1_epi32(1)); // ffffffff -> 00000001
      renorm_mask2 = _mm256_and_si256(renorm_mask2, _mm256_set1_epi32(1));
      indices1 = _mm256_add_epi32(indices1, renorm_mask1);
      indices2 = _mm256_add_epi32(indices2, renorm_mask2);
    }      
    _mm256_maskstore_epi32(&R[0], mask1, Rz1);
    _mm256_maskstore_epi32(&R[8], mask1, Rz2);
#endif

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

unsigned char *rans_compress_O1_4x16(unsigned char *in, unsigned int in_size,
				     unsigned char *out, unsigned int *out_size) {
    unsigned char *cp, *out_end;
    unsigned int tab_size, rle_i, rle_j;
    RansEncSymbol syms[256][256];
    int bound = rans_compress_bound_4x16(in_size,1, NULL);

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

    F[0][in[1*(in_size>>2)]]++;
    F[0][in[2*(in_size>>2)]]++;
    F[0][in[3*(in_size>>2)]]++;
    T[0]+=3;

    
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
    
    RansState rans0, rans1, rans2, rans3;
    RansEncInit(&rans0);
    RansEncInit(&rans1);
    RansEncInit(&rans2);
    RansEncInit(&rans3);

    uint8_t* ptr = out_end;

    int isz4 = in_size>>2;
    int i0 = 1*isz4-2;
    int i1 = 2*isz4-2;
    int i2 = 3*isz4-2;
    int i3 = 4*isz4-2;

    unsigned char l0 = in[i0+1];
    unsigned char l1 = in[i1+1];
    unsigned char l2 = in[i2+1];
    unsigned char l3 = in[i3+1];

    // Deal with the remainder
    l3 = in[in_size-1];
    for (i3 = in_size-2; i3 > 4*isz4-2; i3--) {
	unsigned char c3 = in[i3];
	RansEncPutSymbol(&rans3, &ptr, &syms[c3][l3]);
	l3 = c3;
    }

    for (; i0 >= 0; i0--, i1--, i2--, i3--) {
	unsigned char c0, c1, c2, c3;
	RansEncSymbol *s3 = &syms[c3 = in[i3]][l3];
	RansEncSymbol *s2 = &syms[c2 = in[i2]][l2];
	RansEncSymbol *s1 = &syms[c1 = in[i1]][l1];
	RansEncSymbol *s0 = &syms[c0 = in[i0]][l0];

	RansEncPutSymbol(&rans3, &ptr, s3);
	RansEncPutSymbol(&rans2, &ptr, s2);
	RansEncPutSymbol(&rans1, &ptr, s1);
	RansEncPutSymbol(&rans0, &ptr, s0);

	l0 = c0;
	l1 = c1;
	l2 = c2;
	l3 = c3;
    }

    RansEncPutSymbol(&rans3, &ptr, &syms[0][l3]);
    RansEncPutSymbol(&rans2, &ptr, &syms[0][l2]);
    RansEncPutSymbol(&rans1, &ptr, &syms[0][l1]);
    RansEncPutSymbol(&rans0, &ptr, &syms[0][l0]);

    RansEncFlush(&rans3, &ptr);
    RansEncFlush(&rans2, &ptr);
    RansEncFlush(&rans1, &ptr);
    RansEncFlush(&rans0, &ptr);

    *out_size = (out_end - ptr) + tab_size;

    cp = out;
    *cp++ = (in_size>> 0) & 0xff;
    *cp++ = (in_size>> 8) & 0xff;
    *cp++ = (in_size>>16) & 0xff;
    *cp++ = (in_size>>24) & 0xff;

    memmove(out + tab_size, ptr, out_end-ptr);

    return out;
}

unsigned char *rans_uncompress_O1c_4x16(unsigned char *in, unsigned int in_size,
					unsigned char *out, unsigned int *out_size) {
    /* Load in the static tables */
    unsigned char *cp = in + 4;
    int i, j = -999, x, out_sz, rle_i, rle_j;
    ari_decoder D[256];
    RansDecSymbol syms[256][256];
    
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
	rle_j = x = 0;
	j = *cp++;
	do {
	    int F, C;
	    if ((F = *cp++) >= 128) {
		F &= ~128;
		F = ((F & 127) << 8) | *cp++;
	    }
	    C = x;

	    //fprintf(stderr, "i=%d j=%d F=%d C=%d\n", i, j, F, C);

	    if (!F)
		F = TOTFREQ_O1;

	    RansDecSymbolInit(&syms[i][j], C, F);

	    /* Build reverse lookup table */
	    //if (!D[i].R) D[i].R = (unsigned char *)malloc(TOTFREQ_O1);
	    memset(&D[i].R[x], j, F);
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

    RansState rans0, rans1, rans2, rans3;
    uint8_t *ptr = cp;
    RansDecInit(&rans0, &ptr);
    RansDecInit(&rans1, &ptr);
    RansDecInit(&rans2, &ptr);
    RansDecInit(&rans3, &ptr);

    int isz4 = out_sz>>2;
    int l0 = 0;
    int l1 = 0;
    int l2 = 0;
    int l3 = 0;
    int i4[] = {0*isz4, 1*isz4, 2*isz4, 3*isz4};

    RansState R[4];
    R[0] = rans0;
    R[1] = rans1;
    R[2] = rans2;
    R[3] = rans3;

    uint16_t *ptr16 = (uint16_t *)ptr;

    for (; i4[0] < isz4; i4[0]++, i4[1]++, i4[2]++, i4[3]++) {
	uint8_t c0, c1, c2, c3;
	{
	    uint32_t m0 = R[0] & ((1u << TF_SHIFT_O1)-1);
	    uint32_t m1 = R[1] & ((1u << TF_SHIFT_O1)-1);

	    c0 = D[l0].R[m0]; out[i4[0]] = c0;
	    c1 = D[l1].R[m1]; out[i4[1]] = c1;

	    RansDecAdvanceSymbolStep(&R[0], &syms[l0][c0], TF_SHIFT_O1);
	    RansDecAdvanceSymbolStep(&R[1], &syms[l1][c1], TF_SHIFT_O1);
	}

	{
	    uint32_t m2 = R[2] & ((1u << TF_SHIFT_O1)-1);
	    uint32_t m3 = R[3] & ((1u << TF_SHIFT_O1)-1);

	    c2 = D[l2].R[m2]; out[i4[2]] = c2;
	    c3 = D[l3].R[m3]; out[i4[3]] = c3;

	    RansDecAdvanceSymbolStep(&R[2], &syms[l2][c2], TF_SHIFT_O1);
	    RansDecAdvanceSymbolStep(&R[3], &syms[l3][c3], TF_SHIFT_O1);
	}

//#define BRANCHLESS
#ifdef BRANCHLESS
	// Faster on deskpro for q40
	// Slower on deskpro for q8
	// Slower on seq3 for both.
	// Slower on farm (AMD) for both
	uint32_t j4[] = {
	    !(R[0] & ~(RANS_BYTE_L-1)),
	    !(R[1] & ~(RANS_BYTE_L-1)),
	    !(R[2] & ~(RANS_BYTE_L-1)),
	    !(R[3] & ~(RANS_BYTE_L-1))
	};

	R[0] <<= j4[0]<<4;
	R[1] <<= j4[1]<<4;
	R[2] <<= j4[2]<<4;
	R[3] <<= j4[3]<<4;

	uint32_t xx[] = {0,(1<<16)-1};
	R[0] |= (*ptr16) & xx[j4[0]];
	ptr16 += j4[0];
	
	R[1] |= (*ptr16) & xx[j4[1]];
	ptr16 += j4[1];
	
	R[2] |= (*ptr16) & xx[j4[2]];
	ptr16 += j4[2];
	
	R[3] |= (*ptr16) & xx[j4[3]];
	ptr16 += j4[3];
#else
	RansDecRenorm(&R[0], (uint8_t **)&ptr16);
	RansDecRenorm(&R[1], (uint8_t **)&ptr16);
	RansDecRenorm(&R[2], (uint8_t **)&ptr16);
	RansDecRenorm(&R[3], (uint8_t **)&ptr16);
#endif

	l0 = c0;
	l1 = c1;
	l2 = c2;
	l3 = c3;
    }
    ptr = (uint8_t *)ptr16;

    rans0 = R[0];
    rans1 = R[1];
    rans2 = R[2];
    rans3 = R[3];

    // Remainder
    for (; i4[3] < out_sz; i4[3]++) {
	unsigned char c3 = D[l3].R[RansDecGet(&rans3, TF_SHIFT_O1)];
	out[i4[3]] = c3;
	RansDecAdvanceSymbol(&rans3, &ptr, &syms[l3][c3], TF_SHIFT_O1);
	l3 = c3;
    }
    
    *out_size = out_sz;

//    for (i = 0; i < 256; i++)
//	if (D[i].R) free(D[i].R);

    return out;
}

typedef struct {
    uint16_t f;
    uint16_t b;
} sb_t;

unsigned char *rans_uncompress_O1sfb_4x16(unsigned char *in, unsigned int in_size,
					  unsigned char *out, unsigned int *out_size) {
    /* Load in the static tables */
    unsigned char *cp = in + 4;
    int i, j = -999, x, y, out_sz, rle_i, rle_j;
    sb_t sfb[256][TOTFREQ_O1+32];
    // uint16_t for ssym sometimes works faster, but considter 8-bit if cache is tight
    uint16_t ssym [256][TOTFREQ_O1+32];
    
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
		ssym [i][y + C] = j;
		sfb[i][y + C].f = F;
		sfb[i][y + C].b = y;
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

    RansState rans0, rans1, rans2, rans3;
    uint8_t *ptr = cp;
    RansDecInit(&rans0, &ptr);
    RansDecInit(&rans1, &ptr);
    RansDecInit(&rans2, &ptr);
    RansDecInit(&rans3, &ptr);

    int isz4 = out_sz>>2;
    int l0 = 0;
    int l1 = 0;
    int l2 = 0;
    int l3 = 0;
    int i4[] = {0*isz4, 1*isz4, 2*isz4, 3*isz4};

    RansState R[4];
    R[0] = rans0;
    R[1] = rans1;
    R[2] = rans2;
    R[3] = rans3;

    for (; i4[0] < isz4; i4[0]++, i4[1]++, i4[2]++, i4[3]++) {
	uint32_t m[4];
	uint32_t c[4];

	m[0] = R[0] & ((1u << TF_SHIFT_O1)-1);
	c[0] = ssym[l0][m[0]];
	R[0] = sfb[l0][m[0]].f * (R[0]>>TF_SHIFT_O1) + sfb[l0][m[0]].b;

	m[1] = R[1] & ((1u << TF_SHIFT_O1)-1);
	c[1] = ssym[l1][m[1]];
	R[1] = sfb[l1][m[1]].f * (R[1]>>TF_SHIFT_O1) + sfb[l1][m[1]].b;

	m[2] = R[2] & ((1u << TF_SHIFT_O1)-1);
	c[2] = ssym[l2][m[2]];
	R[2] = sfb[l2][m[2]].f * (R[2]>>TF_SHIFT_O1) + sfb[l2][m[2]].b;

	m[3] = R[3] & ((1u << TF_SHIFT_O1)-1);
	c[3] = ssym[l3][m[3]];
	out[i4[0]] = c[0];
	out[i4[1]] = c[1];
	out[i4[2]] = c[2];
	out[i4[3]] = c[3];
	R[3] = sfb[l3][m[3]].f * (R[3]>>TF_SHIFT_O1) + sfb[l3][m[3]].b;

	RansDecRenorm(&R[0], &ptr);
	RansDecRenorm(&R[1], &ptr);
	RansDecRenorm(&R[2], &ptr);
	RansDecRenorm(&R[3], &ptr);

	l0 = c[0];
	l1 = c[1];
	l2 = c[2];
	l3 = c[3];
    }

    // Remainder
    for (; i4[3] < out_sz; i4[3]++) {
	uint32_t m3 = R[3] & ((1u<<TF_SHIFT_O1)-1);
	unsigned char c3 = ssym[l3][m3];
	out[i4[3]] = c3;
	R[3] = sfb[l3][m3].f * (R[3]>>TF_SHIFT_O1) + sfb[l3][m3].b;
	RansDecRenorm(&R[3], &ptr);
	l3 = c3;
    }
    
    *out_size = out_sz;

//    for (i = 0; i < 256; i++)
//	if (D[i].R) free(D[i].R);

    return out;
}

/*-----------------------------------------------------------------------------
 * Simple interface to the order-0 vs order-1 encoders and decoders.
 */
unsigned char *rans_compress_to_4x16(unsigned char *in,  unsigned int in_size,
				     unsigned char *out, unsigned int *out_size,
				     int order) {
    return order
	? rans_compress_O1_4x16(in, in_size, out, out_size)
	: rans_compress_O0_4x16(in, in_size, out, out_size);
}

unsigned char *rans_compress_4x16(unsigned char *in, unsigned int in_size,
				  unsigned int *out_size, int order) {
    return rans_compress_to_4x16(in, in_size, NULL, out_size, order);
}

unsigned char *rans_uncompress_to_4x16(unsigned char *in,  unsigned int in_size,
				       unsigned char *out, unsigned int *out_size,
				       int order) {
    return order
	? rans_uncompress_O1sfb_4x16(in, in_size, out, out_size)
      //? rans_uncompress_O1c_4x16(in, in_size, out, out_size)
	: rans_uncompress_O0_4x16(in, in_size, out, out_size);
}

unsigned char *rans_uncompress_4x16(unsigned char *in, unsigned int in_size,
				    unsigned int *out_size, int order) {
    return rans_uncompress_to_4x16(in, in_size, NULL, out_size, order);
}


#ifdef TEST_MAIN

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
	    bc[nb].sz = rans_compress_bound_4x16(BLK_SIZE, order, NULL);
	    bc[nb].blk = malloc(bc[nb].sz);
	    bu[nb].sz = BLK_SIZE;
	    bu[nb].blk = malloc(BLK_SIZE);
	    nb++;
	    in_sz += len;
	}

#ifndef NTRIALS
#define NTRIALS 10
#endif
	int trials = NTRIALS;
	while (trials--) {
	    // Warmup
	    for (i = 0; i < nb; i++) memset(bc[i].blk, 0, bc[i].sz);

	    gettimeofday(&tv1, NULL);

	    out_sz = 0;
	    for (i = 0; i < nb; i++) {
		unsigned int csz = bc[i].sz;
		bc[i].blk = rans_compress_to_4x16(b[i].blk, b[i].sz, bc[i].blk, &csz, order);
		assert(csz <= bc[i].sz);
		out_sz += 5 + csz;
	    }

	    gettimeofday(&tv2, NULL);

	    // Warmup
	    for (i = 0; i < nb; i++) memset(bu[i].blk, 0, BLK_SIZE);

	    gettimeofday(&tv3, NULL);

	    for (i = 0; i < nb; i++)
		bu[i].blk = rans_uncompress_to_4x16(bc[i].blk, bc[i].sz, bu[i].blk, &bu[i].sz, order);

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
	    out = rans_uncompress_4x16(in_buf, in_size, &out_size, order);
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

	    out = rans_compress_4x16(in_buf, in_size, &out_size,
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
