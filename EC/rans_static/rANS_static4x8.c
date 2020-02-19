// Use 11 for order-1?
#define TF_SHIFT 12
#define TOTFREQ (1<<TF_SHIFT)

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
#define RANS_BYTE_L (1u << 23)  // lower bound of our normalization interval

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
} RansEncSymbol;

// Decoder symbols are straightforward.
// 32-bit means more memory, but oddly faster on old gcc? Why?
// 322MB/s vs 309MB/s for order-1.
typedef struct {
    uint16_t freq;      // Symbol frequency.
    uint16_t start;     // Start of range.
} RansDecSymbol;

typedef struct {
    uint32_t freq;      // Symbol frequency.
    uint32_t start;     // Start of range.
} RansDecSymbol32;

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

    s->x_max = ((RANS_BYTE_L >> scale_bits) << 8) * freq;
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
	uint8_t* ptr = *pptr;
	do {
	    *--ptr = (uint8_t) (x & 0xff);
	    x >>= 8;
	} while (x >= x_max);
	*pptr = ptr;
    }

    //uint32_t q = (uint32_t) (((uint64_t)x * sym->rcp_freq) >> sym->rcp_shift);
    //*r = q * sym->cmpl_freq + x + sym->bias;

    // x = C(s,x)
    // NOTE: written this way so we get a 32-bit "multiply high" when
    // available. If you're on a 64-bit platform with cheap multiplies
    // (e.g. x64), just bake the +32 into rcp_shift.
    //uint32_t q = (uint32_t) (((uint64_t)x * sym->rcp_freq) >> 32) >> sym->rcp_shift;

    // The extra >>32 has already been added to RansEncSymbolInit
    uint32_t q = (uint32_t) (((uint64_t)x * sym->rcp_freq) >> sym->rcp_shift);
    *r = q * sym->cmpl_freq + x + sym->bias;
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
#ifdef __x86_64
/*
 * Assembly variants of the RansDecRenorm code.
 * These are based on joint ideas from Rob Davies and from looking at
 * the clang assembly output.
 */
static inline void RansDecRenorm(RansState* r, uint8_t** pptr) {
    uint32_t  x   = *r;
    uint8_t  *ptr = *pptr;

    __asm__ ("movzbl (%0), %%eax\n\t"
	     "mov    %1, %%edx\n\t"
	     "shl    $0x8,%%edx\n\t"
             "or     %%eax,%%edx\n\t"
             "cmp    $0x800000,%1\n\t"
             "cmovb  %%edx,%1\n\t"
             "adc    $0x0,%0\n\t"
             : "=r" (ptr), "=r" (x)
             : "0" (ptr), "1" (x)
             : "eax", "edx"
             );
    if (x < 0x800000) x = (x << 8) | *ptr++;
    *pptr = ptr;
    *r = x;
}

/*
 * A variant that normalises two rans states.
 * The only minor tweak here is to adjust the reorder a few opcodes
 * to reduce dependency delays.
 */
static inline void RansDecRenorm2(RansState* r1, RansState* r2, uint8_t** pptr) {
    uint32_t  x1   = *r1;
    uint32_t  x2   = *r2;
    uint8_t  *ptr = *pptr;

    __asm__ ("movzbl (%0), %%eax\n\t"
             "mov    %1, %%edx\n\t"
             "shl    $0x8, %%edx\n\t"
             "or     %%eax, %%edx\n\t"
             "cmp    $0x800000, %1\n\t"
             "cmovb  %%edx, %1\n\t"
             "adc    $0x0, %0\n\t"
             "mov    %2, %%edx\n\t"
             "shl    $0x8, %%edx\n\t"
             "cmp    $0x800000, %1\n\t"
             "jae    1f\n\t"
             "movzbl (%0), %%eax\n\t"
             "shl    $0x8, %1\n\t"
             "or     %%eax, %1\n\t"
             "add    $0x1, %0\n\t"
             "1:\n\t"
             "movzbl (%0), %%eax\n\t"
             "or     %%eax, %%edx\n\t"
             "cmp    $0x800000, %2\n\t"
             "cmovb  %%edx, %2\n\t"
             "adc    $0x0, %0\n\t"
             "cmp    $0x800000, %2\n\t"
             "jae    2f\n\t"
             "movzbl (%0), %%eax\n\t"
             "shl    $0x8, %2\n\t"
             "or     %%eax, %2\n\t"
             "add    $0x1, %0\n\t"
             "2:\n\t"
             : "=r" (ptr), "=r" (x1), "=r" (x2)
             : "0" (ptr), "1" (x1), "2" (x2)
             : "eax", "edx"
             );

    *pptr = ptr;
    *r1 = x1;
    *r2 = x2;
}

#else /* __x86_64 */

static inline void RansDecRenorm(RansState* r, uint8_t** pptr)
{
    // renormalize
    uint32_t x = *r;

#ifdef __clang__
    // Generates cmov instructions on clang, but alas not gcc
    uint8_t* ptr = *pptr;
    uint32_t y = (x << 8) | *ptr;
    uint32_t cond = x < RANS_BYTE_L;
    x    = cond ? y : x;
    ptr += cond ? 1 : 0;
    if (x < RANS_BYTE_L) x = (x<<8) | *ptr++;
    *pptr = ptr;
#else
    if (x >= RANS_BYTE_L) return;
    uint8_t* ptr = *pptr;
    x = (x << 8) | *ptr++;
    while (x < RANS_BYTE_L) x = (x << 8) | *ptr++;
    *pptr = ptr;
#endif /* __clang__ */

    *r = x;
}

static inline void RansDecRenorm2(RansState* r1, RansState* r2, uint8_t** pptr) {
    RansDecRenorm(r1, pptr);
    RansDecRenorm(r2, pptr);
}

#endif /* __x86_64 */

static inline void RansDecRenormSafe(RansState* r, uint8_t** pptr, uint8_t *ptr_end)
{
    uint32_t x = *r;
    uint8_t* ptr = *pptr;
    if (x >= RANS_BYTE_L || ptr >= ptr_end) return;
    x = (x << 8) | *ptr++;
    if (x < RANS_BYTE_L && ptr < ptr_end)
	x = (x << 8) | *ptr++;
    *pptr = ptr;
    *r = x;
}

static inline void RansDecSymbolInit32(RansDecSymbol32* s, uint32_t start, uint32_t freq)
{
    RansAssert(start <= (1 << 16));
    RansAssert(freq <= (1 << 16) - start);
    s->start = (uint16_t) start;
    s->freq = (uint16_t) freq;
}

static inline void RansDecAdvanceSymbol32(RansState* r, uint8_t** pptr, RansDecSymbol32 const* sym, uint32_t scale_bits)
{
    RansDecAdvance(r, pptr, sym->start, sym->freq, scale_bits);
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
#include "rANS_static4x8.h"

/*-----------------------------------------------------------------------------
 * Memory to memory compression functions.
 *
 * These are original versions without any manual loop unrolling. They
 * are easier to understand, but can be up to 2x slower.
 */

#define MAGIC 8

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

unsigned int rans_compress_bound_4x8(unsigned int size, int order) {
    return order == 0
	? 1.05*size + 257*3 + 4
	: 1.05*size + 257*257*3 + 4;
}

unsigned char *rans_compress_O0_4x8(unsigned char *in, unsigned int in_size,
				    unsigned char *out, unsigned int *out_size) {
    unsigned char *cp, *out_end;
    RansEncSymbol syms[256];
    RansState rans0;
    RansState rans2;
    RansState rans1;
    RansState rans3;
    uint8_t* ptr;
    int F[256+MAGIC] = {0}, i, j, tab_size, rle, x, fsum = 0;
    int m = 0, M = 0;
    int bound = rans_compress_bound_4x8(in_size,0);

    if (!out) {
	*out_size = bound;
	out = malloc(*out_size);
    }
    if (!out || bound > *out_size)
	return NULL;

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

    fsum++;
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

    RansEncInit(&rans0);
    RansEncInit(&rans1);
    RansEncInit(&rans2);
    RansEncInit(&rans3);

    switch (i=(in_size&3)) {
    case 3: RansEncPutSymbol(&rans2, &ptr, &syms[in[in_size-(i-2)]]);
    case 2: RansEncPutSymbol(&rans1, &ptr, &syms[in[in_size-(i-1)]]);
    case 1: RansEncPutSymbol(&rans0, &ptr, &syms[in[in_size-(i-0)]]);
    case 0:
	break;
    }
    for (i=(in_size &~3); i>0; i-=4) {
	RansEncSymbol *s3 = &syms[in[i-1]];
	RansEncSymbol *s2 = &syms[in[i-2]];
	RansEncSymbol *s1 = &syms[in[i-3]];
	RansEncSymbol *s0 = &syms[in[i-4]];

	RansEncPutSymbol(&rans3, &ptr, s3);
	RansEncPutSymbol(&rans2, &ptr, s2);
	RansEncPutSymbol(&rans1, &ptr, s1);
	RansEncPutSymbol(&rans0, &ptr, s0);
    }

    RansEncFlush(&rans3, &ptr);
    RansEncFlush(&rans2, &ptr);
    RansEncFlush(&rans1, &ptr);
    RansEncFlush(&rans0, &ptr);

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

unsigned char *rans_uncompress_O0_4x8(unsigned char *in, unsigned int in_size,
				      unsigned char *out, unsigned int *out_size) {
    /* Load in the static tables */
    unsigned char *cp = in + 4;
    unsigned char *cp_end = in + in_size - 8; // within 8 => be extra safe
    const uint32_t mask = (1u << TF_SHIFT)-1;
    int i, j, x, y, out_sz, rle;
    RansState R[4];
    RansState m[4];
    uint16_t sfreq[TOTFREQ+32];
    uint16_t ssym [TOTFREQ+32]; // faster, but only needs uint8_t
    uint32_t sbase[TOTFREQ+16]; // faster, but only needs uint16_t

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
            sfreq[y + C] = F;
            sbase[y + C] = y;
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

    RansDecInit(&R[0], &cp);
    RansDecInit(&R[1], &cp);
    RansDecInit(&R[2], &cp);
    RansDecInit(&R[3], &cp);

    int out_end = (out_sz&~3);
    for (i=0; i < out_end; i+=4) {
	m[0] = R[0] & mask;
        out[i+0] = ssym[m[0]];
        R[0] = sfreq[m[0]] * (R[0] >> TF_SHIFT) + sbase[m[0]];

        m[1] = R[1] & mask;
	out[i+1] = ssym[m[1]];
        R[1] = sfreq[m[1]] * (R[1] >> TF_SHIFT) + sbase[m[1]];

        m[2] = R[2] & mask;
	out[i+2] = ssym[m[2]];
        R[2] = sfreq[m[2]] * (R[2] >> TF_SHIFT) + sbase[m[2]];

        m[3] = R[3] & mask;
	out[i+3] = ssym[m[3]];
        R[3] = sfreq[m[3]] * (R[3] >> TF_SHIFT) + sbase[m[3]];

	if (cp < cp_end) {
	    RansDecRenorm2(&R[0], &R[1], &cp);
	    RansDecRenorm2(&R[2], &R[3], &cp);
	} else {
	    RansDecRenormSafe(&R[0], &cp, cp_end+8);
	    RansDecRenormSafe(&R[1], &cp, cp_end+8);
	    RansDecRenormSafe(&R[2], &cp, cp_end+8);
	    RansDecRenormSafe(&R[3], &cp, cp_end+8);
	}
    }

    switch(out_sz&3) {
    case 3:
        out[out_end + 2] = ssym[R[2] & mask];
    case 2:
        out[out_end + 1] = ssym[R[1] & mask];
    case 1:
        out[out_end] = ssym[R[0] & mask];
    default:
        break;
    }

    *out_size = out_sz;
    return out;
}

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

unsigned char *rans_compress_O1_4x8(unsigned char *in, unsigned int in_size,
				    unsigned char *out, unsigned int *out_size) {
    unsigned char *cp, *out_end;
    unsigned int tab_size, rle_i, rle_j;
    RansEncSymbol syms[256][256];
    int bound = rans_compress_bound_4x8(in_size,1);

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

	double p = ((double)TOTFREQ)/T[i];
	for (t2 = m = M = j = 0; j < 256; j++) {
	    if (!F[i][j])
		continue;

	    if (m < F[i][j])
		m = F[i][j], M = j;

	    if ((F[i][j] *= p) == 0)
		F[i][j] = 1;
	    t2 += F[i][j];
	}

	t2++;

	int adjust = TOTFREQ-t2;
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

		RansEncSymbolInit(&syms[i][j], x, F_i_[j], TF_SHIFT);
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
	unsigned char c3 = in[i3];
	unsigned char c2 = in[i2];
	unsigned char c1 = in[i1];
	unsigned char c0 = in[i0];

	RansEncSymbol *s3 = &syms[c3][l3];
	RansEncSymbol *s2 = &syms[c2][l2];
	RansEncSymbol *s1 = &syms[c1][l1];
	RansEncSymbol *s0 = &syms[c0][l0];

	RansEncPutSymbol(&rans3, &ptr, s3);
	RansEncPutSymbol(&rans2, &ptr, s2);
	RansEncPutSymbol(&rans1, &ptr, s1);
	RansEncPutSymbol(&rans0, &ptr, s0);

	l3 = c3;
	l2 = c2;
	l1 = c1;
	l0 = c0;
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

//-----------------------------------------------------------------------------
// Sort by symbol freq so part of D[] accessed via the first rows.
// This trades an extra indirection via map[] with improved cache
// locaility.
unsigned char *rans_uncompress_O1_4x8(unsigned char *in, unsigned int in_size,
				      unsigned char *out, unsigned int *out_size) {
    /* Load in the static tables */
    unsigned char *cp = in + 4;
    unsigned char *ptr_end = in + in_size - 8; // within 8 => be extra safe
    int i, j = -999, x, out_sz, rle_i, rle_j;
    ari_decoder D[256];
    RansDecSymbol32 syms[256][256+6];
    int16_t map[256], map_i = 0;

    memset(map, -1, 256*sizeof(*map));

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
	if (map[i] == -1)
	    map[i] = map_i++;
	int m_i = map[i];

	rle_j = x = 0;
	j = *cp++;
	do {
	    if (map[j] == -1)
		map[j] = map_i++;

	    int F, C;
	    if ((F = *cp++) >= 128) {
		F &= ~128;
		F = ((F & 127) << 8) | *cp++;
	    }
	    C = x;

	    //fprintf(stderr, "i=%d j=%d F=%d C=%d\n", i, j, F, C);

	    if (!F)
		F = TOTFREQ;

	    RansDecSymbolInit32(&syms[m_i][j], C, F);

	    /* Build reverse lookup table */
	    //if (!D[i].R) D[i].R = (unsigned char *)malloc(TOTFREQ);
	    memset(&D[m_i].R[x], j, F);
	    x += F;

	    assert(x <= TOTFREQ);

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

    RansState R[4];
    R[0] = rans0;
    R[1] = rans1;
    R[2] = rans2;
    R[3] = rans3;

    int isz4 = out_sz>>2;
    uint32_t l0 = 0;
    uint32_t l1 = 0;
    uint32_t l2 = 0;
    uint32_t l3 = 0;
    
    int i4[] = {0*isz4, 1*isz4, 2*isz4, 3*isz4};

    uint8_t cc0 = D[map[l0]].R[R[0] & ((1u << TF_SHIFT)-1)];
    uint8_t cc1 = D[map[l1]].R[R[1] & ((1u << TF_SHIFT)-1)];
    uint8_t cc2 = D[map[l2]].R[R[2] & ((1u << TF_SHIFT)-1)];
    uint8_t cc3 = D[map[l3]].R[R[3] & ((1u << TF_SHIFT)-1)];

    for (; i4[0] < isz4; i4[0]++, i4[1]++, i4[2]++, i4[3]++) {
	out[i4[0]] = cc0;
	out[i4[1]] = cc1;
	out[i4[2]] = cc2;
	out[i4[3]] = cc3;

	{
	    uint32_t m[4];

	    // Ordering to try and improve OoO cpu instructions
	    m[0] = R[0] & ((1u << TF_SHIFT)-1);
	    R[0] = syms[l0][cc0].freq * (R[0]>>TF_SHIFT);
	    m[1] = R[1] & ((1u << TF_SHIFT)-1);
	    R[0] += m[0] - syms[l0][cc0].start;
	    R[1] = syms[l1][cc1].freq * (R[1]>>TF_SHIFT);
	    m[2] = R[2] & ((1u << TF_SHIFT)-1);
	    R[1] += m[1] - syms[l1][cc1].start;
	    R[2] = syms[l2][cc2].freq * (R[2]>>TF_SHIFT);
	    m[3] = R[3] & ((1u << TF_SHIFT)-1);
	    R[3] = syms[l3][cc3].freq * (R[3]>>TF_SHIFT);
	    R[2] += m[2] - syms[l2][cc2].start;
	    R[3] += m[3] - syms[l3][cc3].start;
	}

	l0 = map[cc0];
	l1 = map[cc1];
	l2 = map[cc2];
	l3 = map[cc3];

	if (ptr < ptr_end) {
	    RansDecRenorm2(&R[0], &R[1], &ptr);
	    RansDecRenorm2(&R[2], &R[3], &ptr);
	} else {
	    RansDecRenormSafe(&R[0], &ptr, ptr_end+8);
	    RansDecRenormSafe(&R[1], &ptr, ptr_end+8);
	    RansDecRenormSafe(&R[2], &ptr, ptr_end+8);
	    RansDecRenormSafe(&R[3], &ptr, ptr_end+8);
	}

	cc0 = D[l0].R[R[0] & ((1u << TF_SHIFT)-1)];
	cc1 = D[l1].R[R[1] & ((1u << TF_SHIFT)-1)];
	cc2 = D[l2].R[R[2] & ((1u << TF_SHIFT)-1)];
	cc3 = D[l3].R[R[3] & ((1u << TF_SHIFT)-1)];
    }

    // Remainder
    for (; i4[3] < out_sz; i4[3]++) {
	unsigned char c3 = D[l3].R[RansDecGet(&R[3], TF_SHIFT)];
	out[i4[3]] = c3;

	uint32_t m = R[3] & ((1u << TF_SHIFT)-1);
	R[3] = syms[l3][c3].freq * (R[3]>>TF_SHIFT) + m - syms[l3][c3].start;
	RansDecRenormSafe(&R[3], &ptr, ptr_end+8);
	l3 = map[c3];
    }
    
    *out_size = out_sz;

    return (unsigned char *)out;
}

/*-----------------------------------------------------------------------------
 * Simple interface to the order-0 vs order-1 encoders and decoders.
 */
unsigned char *rans_compress_to_4x8(unsigned char *in,  unsigned int in_size,
				    unsigned char *out, unsigned int *out_size,
				    int order) {
    return order
	? rans_compress_O1_4x8(in, in_size, out, out_size)
	: rans_compress_O0_4x8(in, in_size, out, out_size);
}

unsigned char *rans_compress_4x8(unsigned char *in, unsigned int in_size,
				 unsigned int *out_size, int order) {
    return rans_compress_to_4x8(in, in_size, NULL, out_size, order);
}

unsigned char *rans_uncompress_to_4x8(unsigned char *in,  unsigned int in_size,
				      unsigned char *out, unsigned int *out_size,
				      int order) {
    return order
	? rans_uncompress_O1_4x8(in, in_size, out, out_size)
	: rans_uncompress_O0_4x8(in, in_size, out, out_size);
}

unsigned char *rans_uncompress_4x8(unsigned char *in, unsigned int in_size,
				   unsigned int *out_size, int order) {
    return rans_uncompress_to_4x8(in, in_size, NULL, out_size, order);
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
	    uint32_t csz;
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
	    bc[nb].sz = rans_compress_bound_4x8(BLK_SIZE, order);
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
		bc[i].csz = bc[i].sz;
		bc[i].blk = rans_compress_to_4x8(b[i].blk, b[i].sz, bc[i].blk, &bc[i].csz, order);
		assert(bc[i].csz <= bc[i].sz);
		out_sz += 5 + bc[i].csz;
	    }

	    gettimeofday(&tv2, NULL);

	    // Warmup
	    for (i = 0; i < nb; i++) memset(bu[i].blk, 0, BLK_SIZE);

	    gettimeofday(&tv3, NULL);

	    for (i = 0; i < nb; i++)
		bu[i].blk = rans_uncompress_to_4x8(bc[i].blk, bc[i].csz, bu[i].blk, &bu[i].sz, order);

	    gettimeofday(&tv4, NULL);

	    for (i = 0; i < nb; i++) {
		if (b[i].sz != bu[i].sz || memcmp(b[i].blk, bu[i].blk, b[i].sz))
		    fprintf(stderr, "Mismatch in block %d, sz %d/%d\n", i, b[i].sz, bu[i].sz);
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

//	    // To test buffer overruns, the following code example may
//	    // be used to create a memory buffer with the data ending
//	    // at an invalid address.  This demonstrates the perils of
//	    // a branchless decoder without guards.
//#define M 4096
//	    unsigned char *in_cpy = mmap((void *)0x706050403000LL, M, PROT_READ|PROT_WRITE,
//					 MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
//	    in_cpy += M-in_size;
//	    memcpy(in_cpy, in_buf, in_size);
//	    fprintf(stderr, "in=%p to %p\n", in_cpy, in_cpy + in_size-1);
//	    out = rans_uncompress_4x8(in_cpy, in_size, &out_size, order);
	    out = rans_uncompress_4x8(in_buf, in_size, &out_size, order);
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

	    out = rans_compress_4x8(in_buf, in_size, &out_size,
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
