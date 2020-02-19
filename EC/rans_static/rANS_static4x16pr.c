// As per standard rANS_static but using optional RLE or bit-packing
// techniques prior to entropy encoding.  This is a significant
// reduction in some data sets.

// top bits in order byte
#define X_PACK 0x80
#define X_RLE  0x40

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

//    uint32_t c = x < sym->x_max;
//    uint16_t *p16 = (uint16_t *)(*pptr-2);
//    uint32_t p1 = x, x1 = x >> 16;
//    *p16  = c ? *p16 : p1;
//    *pptr = c ? *pptr : (uint8_t *)p16;
//    x     = c ? x : x1;

    if (x >= x_max) {
	uint16_t* ptr = *(uint16_t **)pptr;
	*--ptr = x;//(uint16_t) (x & 0xffff);
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

#ifndef __x86_64
    // clang 464, gcc 485
    if (x >= RANS_BYTE_L) return;
    uint16_t* ptr = *(uint16_t **)pptr;
    x = (x << 16) | *ptr++;
    *pptr = (uint8_t *)ptr;

//    // clang 335, gcc 687
//    uint16_t* ptr = *(uint16_t **)pptr;
//    uint32_t y = (x << 16) | *ptr;
//    x    = (x < RANS_BYTE_L) ? y : x;
//    ptr += (x < RANS_BYTE_L) ? 1 : 0;
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

#define TF_SHIFT 12
#define TOTFREQ (1<<TF_SHIFT)

// 9-11 is considerably faster in the O1sfb variant due to reduced table size.
#ifndef TF_SHIFT_O1
#define TF_SHIFT_O1 11
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

static void present8(unsigned char *in, unsigned int in_size, int F0[256]) {
    int F1[256+MAGIC] = {0}, F2[256+MAGIC] = {0}, F3[256+MAGIC] = {0};
    int F4[256+MAGIC] = {0}, F5[256+MAGIC] = {0}, F6[256+MAGIC] = {0}, F7[256+MAGIC] = {0};
    int i, i8 = in_size & ~7;
    for (i = 0; i < i8; i+=8) {
	F0[in[i+0]]=1;
	F1[in[i+1]]=1;
	F2[in[i+2]]=1;
	F3[in[i+3]]=1;
	F4[in[i+4]]=1;
	F5[in[i+5]]=1;
	F6[in[i+6]]=1;
	F7[in[i+7]]=1;
    }
    while (i < in_size)
	F0[in[i++]]=1;

    for (i = 0; i < 256; i++)
	F0[i] += F1[i] + F2[i] + F3[i] + F4[i] + F5[i] + F6[i] + F7[i];
}

static void normalise_freq(int *F, int size, int tot) {
    int m = 0, M = 0, fsum = 0, j;
    uint64_t tr = ((uint64_t)tot<<31)/size + (1<<30)/size;

    for (m = M = j = 0; j < 256; j++) {
	if (!F[j])
	    continue;

	if (m < F[j])
	    m = F[j], M = j;

	if ((F[j] = (F[j]*tr)>>31) == 0)
	    F[j] = 1;
	fsum += F[j];
    }

    int adjust = tot - fsum;
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
}

static int encode_freq(uint8_t *cp, int *F) {
    uint8_t *op = cp;
    int rle, j;

    for (rle = j = 0; j < 256; j++) {
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
	}
    }
    *cp++ = 0;
    
    return cp - op;
}

static int decode_freq(uint8_t *cp, int *F) {
    uint8_t *op = cp;
    int rle = 0;
    int j = *cp++;
    do {
	int f;
	if ((f = *cp++) >= 128) {
	    f &= ~128;
	    f = ((f & 127) << 8) | *cp++;
	}
	F[j] = f;

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

    return cp - op;
}

// symbols only
static int encode_freq0(uint8_t *cp, int *F) {
    uint8_t *op = cp;
    int rle, j;

    for (rle = j = 0; j < 256; j++) {
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
	}
    }
    *cp++ = 0;
    
    return cp - op;
}

static int decode_freq0(uint8_t *cp, int *F) {
    uint8_t *op = cp;
    int rle = 0;
    int j = *cp++;
    do {
	F[j] = 1;
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

    return cp - op;
}

// Use the order-0 freqs in F0 to encode the order-1 stats in F.
// All symbols present in F are present in F0, but some in F0 will
// be empty in F.  Thus we run-length encode the 0 frequencies.
static int encode_freq_d(uint8_t *cp, int *F0, int *F) {
    uint8_t *op = cp;
    int rle, j, dz;

    for (dz = rle = j = 0; j < 256; j++) {
	if (F0[j]) {
	    if (F[j] != 0) {
		if (dz) {
		    // Replace dz zeros with zero + dz-1 run length
		    cp -= dz-1;
		    *cp++ = dz-1;
		}
		dz = 0;
		if (F[j]<128) {
		    *cp++ = F[j];
		} else {
		    *cp++ = 128 | (F[j]>>8);
		    *cp++ = F[j]&0xff;
		}
	    } else {
		//fprintf(stderr, "2: j=%d F0[j]=%d, F[j]=%d, dz=%d\n", j, F0[j], F[j], dz);
		dz++;
		*cp++ = 0;
	    }
	} else {
	    assert(F[j] == 0);
	}
    }
    
    if (dz) {
	cp -= dz-1;
	*cp++ = dz-1;
    }

    return cp - op;
}

static int decode_freq_d(uint8_t *cp, int *F0, int *F, RansDecSymbol *syms, unsigned char *R, int *total) {
    uint8_t *op = cp;
    int j, dz, x, T = 0;

    for (x = j = dz = 0; j < 256; j++) {
	//if (F0[j]) fprintf(stderr, "F0[%d]=%d\n", j, F0[j]);
	if (!F0[j])
	    continue;

	int f;
	if (dz) {
	    f = 0;
	    dz--;
	} else {
	    if ((f = *cp++) >= 128) {
		f &= ~128;
		f = ((f & 127) << 8) | *cp++;
	    }
	    if (f == 0)
		dz = *cp++;
	}
	F[j] = f;
	T += f;

	if (syms && F[j]) {
	    RansDecSymbolInit(&syms[j], x, F[j]);
	    memset(&R[x], j, F[j]);
	    x += F[j];
	}
    }

    if (total) *total = T;
    return cp - op;
}

unsigned int rans_compress_bound_4x16(unsigned int size, int order) {
    return (order == 0
	? 1.05*size + 257*3 + 4
	: 1.05*size + 257*257*3 + 4) +
	((order & X_PACK) ? 1 : 0) + 
	((order & X_RLE) ? 1 : 0) + 5;
}

unsigned char *rans_compress_O0_4x16(unsigned char *in, unsigned int in_size,
				     unsigned char *out, unsigned int *out_size) {
    unsigned char *cp, *out_end;
    RansEncSymbol syms[256];
    RansState rans0;
    RansState rans2;
    RansState rans1;
    RansState rans3;
    uint8_t* ptr;
    int F[256+MAGIC] = {0}, i, j, tab_size = 0, rle, x, fsum = 0;
    int m = 0, M = 0;
    int bound = rans_compress_bound_4x16(in_size,0)-5; // -5 for order/size

    if (!out) {
	*out_size = bound;
	out = malloc(*out_size);
    }
    if (!out || bound > *out_size)
	return NULL;

    ptr = out_end = out + bound;

    if (in_size == 0)
	goto empty;

    // Compute statistics
    hist8(in, in_size, F);

    // Normalise so T[i] == TOTFREQ
    normalise_freq(F, in_size, TOTFREQ);

    // Encode statistics.
    cp = out+4;

    for (x = rle = j = 0; j < 256; j++) {
	if (F[j]) {
	    RansEncSymbolInit(&syms[j], x, F[j], TF_SHIFT);
	    x += F[j];
	}
    }

    cp += encode_freq(cp, F);
    tab_size = cp-out;
    //write(2, out+4, cp-(out+4));

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

#if 1
	RansEncPutSymbol(&rans3, &ptr, s3);
	RansEncPutSymbol(&rans2, &ptr, s2);
	RansEncPutSymbol(&rans1, &ptr, s1);
	RansEncPutSymbol(&rans0, &ptr, s0);
#else
	// Slightly beter on gcc, much better on clang
	uint16_t *ptr16 = (uint16_t *)ptr;

	if (rans3 >= s3->x_max) *--ptr16 = (uint16_t)rans3, rans3 >>= 16;
	if (rans2 >= s2->x_max) *--ptr16 = (uint16_t)rans2, rans2 >>= 16;
	uint32_t q3 = (uint32_t) (((uint64_t)rans3 * s3->rcp_freq) >> s3->rcp_shift);
	uint32_t q2 = (uint32_t) (((uint64_t)rans2 * s2->rcp_freq) >> s2->rcp_shift);
	rans3 += s3->bias + q3 * s3->cmpl_freq;
	rans2 += s2->bias + q2 * s2->cmpl_freq;

	if (rans1 >= s1->x_max) *--ptr16 = (uint16_t)rans1, rans1 >>= 16;
	if (rans0 >= s0->x_max) *--ptr16 = (uint16_t)rans0, rans0 >>= 16;
	uint32_t q1 = (uint32_t) (((uint64_t)rans1 * s1->rcp_freq) >> s1->rcp_shift);
	uint32_t q0 = (uint32_t) (((uint64_t)rans0 * s0->rcp_freq) >> s0->rcp_shift);
	rans1 += s1->bias + q1 * s1->cmpl_freq;
	rans0 += s0->bias + q0 * s0->cmpl_freq;

	ptr = (uint8_t *)ptr16;
#endif
    }

    RansEncFlush(&rans3, &ptr);
    RansEncFlush(&rans2, &ptr);
    RansEncFlush(&rans1, &ptr);
    RansEncFlush(&rans0, &ptr);

 empty:
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

unsigned char *rans_uncompress_O0_4x16(unsigned char *in, unsigned int in_size,
				       unsigned char *out, unsigned int *out_size) {
    /* Load in the static tables */
    unsigned char *cp = in + 4;
    int i, j, x, y, out_sz, rle;
    uint16_t sfreq[TOTFREQ+32];
    uint16_t sbase[TOTFREQ+32]; // faster to use 32-bit on clang
    uint8_t  ssym [TOTFREQ+64]; // faster to use 16-bit on clang

    out_sz = ((in[0])<<0) | ((in[1])<<8) | ((in[2])<<16) | ((in[3])<<24);
    if (!out) {
	out = malloc(out_sz);
	*out_size = out_sz;
    }
    if (!out || out_sz > *out_size)
	return NULL;

    // Precompute reverse lookup of frequency.
    int F[256] = {0};
    cp += decode_freq(cp, F);

    // Build symbols; fixme, do as part of decode, see the _d variant
    for (j = x = 0; j < 256; j++) {
	if (F[j]) {
	    for (y = 0; y < F[j]; y++) {
		ssym [y + x] = j;
		sfreq[y + x] = F[j];
		sbase[y + x] = y;
	    }
	    x += F[j];
	}
    }

    assert(x <= TOTFREQ);

    RansState R[4];
    RansDecInit(&R[0], &cp);
    RansDecInit(&R[1], &cp);
    RansDecInit(&R[2], &cp);
    RansDecInit(&R[3], &cp);

    int out_end = (out_sz&~3);
    const uint32_t mask = (1u << TF_SHIFT)-1;

    for (i=0; i < out_end; i+=4) {
	RansState m[4];
	m[0] = R[0] & mask;
        R[0] = sfreq[m[0]] * (R[0] >> TF_SHIFT) + sbase[m[0]];

        m[1] = R[1] & mask;
        R[1] = sfreq[m[1]] * (R[1] >> TF_SHIFT) + sbase[m[1]];

        m[2] = R[2] & mask;
        R[2] = sfreq[m[2]] * (R[2] >> TF_SHIFT) + sbase[m[2]];

        m[3] = R[3] & mask;
        out[i+0] = ssym[m[0]];
	out[i+1] = ssym[m[1]];
	out[i+2] = ssym[m[2]];
	out[i+3] = ssym[m[3]];
        R[3] = sfreq[m[3]] * (R[3] >> TF_SHIFT) + sbase[m[3]];

	RansDecRenorm(&R[0], &cp);
	RansDecRenorm(&R[1], &cp);
	RansDecRenorm(&R[2], &cp);
	RansDecRenorm(&R[3], &cp);
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

//-----------------------------------------------------------------------------
static uint8_t *rle_encode(uint8_t *data, int64_t len,
			   uint8_t *out_meta, int *out_meta_len, int64_t *out_len) {
    uint64_t i, j, k = 0;
    int last = -1;
    unsigned char *out = malloc(len*2);

    int run_len = 0;
    
    // Two pass:
    int64_t saved[256] = {0};
    for (i = 0; i < len; i++) {
	if (data[i] == last) {
	    saved[data[i]]+=run_len;
	    run_len++;
	} else {
	    saved[data[i]]--;
	    run_len = 0;
	    last = data[i];
	}
    }

    // fixme: pack into bits instead of bytes if many symbols?
    for (i =j = 0; i < 256; i++)
	if (saved[i] > 0)
	    j++;
    out[k++] = j;
    for (i = 0; i < 256; i++) {
	if (saved[i] > 0) {
	    //fprintf(stderr, "Saved '%c' = %d\n", (uint8_t)i, (int)saved[i]);
	    out[k++] = i; // fixme store in meta instead?
	}
    }

    for (i = j = 0; i < len; i++) {
	out[k++] = data[i];
	if (saved[data[i]] > 0) {
	    int run_len = i;
	    int last = data[i];
	    while (i < len && data[i] == last)
		i++;
	    i--;
	    run_len = i-run_len;
	    do {
		out_meta[j++] = (run_len & 0x7f) + (run_len>=128 ?128 : 0);
		//out[k++] = (run_len & 0x7f) + (run_len>=128 ?128 : 0);

		run_len>>=7;
	    } while (run_len);
	}
    }
    
    *out_meta_len = j;
    *out_len = k;
    return out;
}

static unsigned char *grow(unsigned char *b, uint64_t *alloc, uint64_t used, uint64_t growth) {
    while (used + growth > *alloc) {
	*alloc *= 2;
	b = realloc(b, *alloc);
    }
    return b;
}

static uint8_t *rle_decode(uint8_t *in, int64_t in_len, uint8_t *meta, unsigned char *out, int64_t *out_len) {
    uint64_t o_len = in_len;
    uint64_t i, j, m;

    int saved[256] = {0};
    for (i = 0, j = in[i++]; j; j--)
	saved[in[i++]]=1;

    j = m = 0;
    while (i < in_len) {
	uint8_t b = in[i++];
	if (saved[b]) {
	    uint32_t run_len = 0;
	    unsigned char c, s = 0;
	    do {
		//c = in[i++];
		c = meta[m++];
		run_len |= (c & 0x7f) << s;
		s += 7;
	    } while (c & 0x80);
	    run_len++;
	    //fprintf(stderr, "run_len=%d %x\n", run_len, run_len);
	    memset(&out[j], b, run_len);
	    j += run_len;
	} else {
	    out[j++] = b;
	}
    }

    *out_len = j;
    return out;
}

//-----------------------------------------------------------------------------
static uint8_t *pack(uint8_t *data, int64_t len,
		     uint8_t *out_meta, int *out_meta_len, int64_t *out_len) {
    int p[256] = {0}, n;
    int64_t i, j;
    uint8_t *out = malloc(len+1);
    if (!out)
	return NULL;

    // count syms
    for (i = 0; i < len; i++)
	p[data[i]]=1;
    
    for (i = n = 0; i < 256; i++) {
	if (p[i]) {
	    p[i] = n++; // p[i] is now the code number
	    out_meta[n] = i;
	}
    }
    j = n+1;
    out_meta[j++] = 0;

    //fprintf(stderr, "n=%d\n", n);
    // 1 value per byte
    if (n > 16 || len < j + len/2) {
	out_meta[0] = 1;
	*out_meta_len = 1;
	// FIXME shortcut this by returning data and checking later.
	memcpy(out, data, len);
	*out_len = len;
	return out;
    }

    // Encode original length
    int64_t len_copy = len;
    do {
	out_meta[j++] = (len_copy & 0x7f) | ((len_copy >= 0x80) << 7);
	len_copy >>= 7;
    } while (len_copy);

    *out_meta_len = j;
    j = 0;

    // 2 values per byte
    if (n > 4) {
	out_meta[0] = 2;
	for (i = 0; i < (len & ~1); i+=2)
	    out[j++] = (p[data[i]]<<4) | (p[data[i+1]]<<0);
	switch (len-i) {
	case 1: out[j++] = p[data[i]]<<4;
	}
	*out_len = j;
	return out;
    }

    // 4 values per byte
    if (n > 2) {
	out_meta[0] = 4;
	for (i = 0; i < (len & ~3); i+=4)
	    out[j++] = (p[data[i]]<<6) | (p[data[i+1]]<<4) | (p[data[i+2]]<<2) | (p[data[i+3]]<<0);
	out[j] = 0;
	int s = len-i;
	switch (s) {
	case 3: out[j] = (out[j]<<2) | p[data[i++]];  // -ABC want ABC-
	case 2: out[j] = (out[j]<<2) | p[data[i++]];  // --AB want AB--
	case 1: out[j] = (out[j]<<2) | p[data[i++]];  // ---A wan  A---
	        out[j++] <<= (4-s)*2;
	}
	*out_len = j;
	return out;
    }

    // 8 values per byte
    if (n > 1) {
	out_meta[0] = 8;
	for (i = 0; i < (len & ~7); i+=8)
	    out[j++] = (p[data[i+0]]<<7) | (p[data[i+1]]<<6) | (p[data[i+2]]<<5) | (p[data[i+3]]<<4)
		     | (p[data[i+4]]<<3) | (p[data[i+5]]<<2) | (p[data[i+6]]<<1) | (p[data[i+7]]<<0);
	out[j] = 0;
	int s = len-i;
	switch (s) {
	case 7: out[j] = (out[j]<<1) | p[data[i++]];
	case 6: out[j] = (out[j]<<1) | p[data[i++]];
	case 5: out[j] = (out[j]<<1) | p[data[i++]];
	case 4: out[j] = (out[j]<<1) | p[data[i++]];
	case 3: out[j] = (out[j]<<1) | p[data[i++]];
	case 2: out[j] = (out[j]<<1) | p[data[i++]];
	case 1: out[j] = (out[j]<<1) | p[data[i++]];
	        out[j++] <<= 8-s;
	}
	*out_len = j;
	return out;
    }

    // infinite values as only 1 type present.
    out_meta[0] = 0;
    *out_len = j;
    return out;
}

// expands the unpack meta data and returns the number of bytes read.
static uint8_t unpack_meta(uint8_t *data, uint64_t udata_len, uint8_t *map, int *nsym, uint64_t *unpacked_len) {
    *nsym = data[0];

    if (*nsym == 1) {
	*unpacked_len = udata_len;
	return 1; // raw data
    }

    // Decode symbol map
    int j = 1, c = 0;
    do {
	map[c++] = data[j++];
    } while (data[j] != 0);
    j++;

    // Decode original length
    uint64_t out_len = 0;
    int shift = 0;
    do {
	c = data[j++];
	out_len |= (c & 0x7f) << shift;
	shift += 7;
    } while (c & 0x80);

    *unpacked_len = out_len;
    return j;
}

static uint8_t *unpack(uint8_t *data, int64_t len, uint8_t *out, uint64_t out_len, int nsym, uint8_t *p) {
    //uint8_t *out;
    uint8_t c = 0;
    int64_t i, j = 0, olen;

    if (nsym == 1) {
	// raw data; FIXME: shortcut the need for malloc & memcpy here
	//if (!(out = malloc(len))) return NULL;
	memcpy(out, data, len);
	return out;
    }

    switch(nsym) {
    case 8:
	olen = out_len & ~7;
	for (i = 0; i < olen; i+=8) {
	    c = data[j++];
	    out[i+0] = p[(c>>7)&1];
	    out[i+1] = p[(c>>6)&1];
	    out[i+2] = p[(c>>5)&1];
	    out[i+3] = p[(c>>4)&1];
	    out[i+4] = p[(c>>3)&1];
	    out[i+5] = p[(c>>2)&1];
	    out[i+6] = p[(c>>1)&1];
	    out[i+7] = p[(c>>0)&1];
	}
	if (out_len != olen) {
	    c = data[j++];
	    switch (out_len - olen) {
		case 8: out[i+7] = p[(c>>0)&1];
		case 7: out[i+6] = p[(c>>1)&1];
		case 6: out[i+5] = p[(c>>2)&1];
		case 5: out[i+4] = p[(c>>3)&1];
		case 4: out[i+3] = p[(c>>4)&1];
		case 3: out[i+2] = p[(c>>5)&1];
		case 2: out[i+1] = p[(c>>6)&1];
		case 1: out[i+0] = p[(c>>7)&1];
	    }
	}
	break;

    case 4:
#if 0
	// Yay a real world usage of Duff's Device!
	// Sadly it's slower due to needing a reverse loop in this scenario.
	i = out_len;
	j += out_len/4;
	c = data[j--];
	switch(out_len % 4) {
	    while (i) {
	    case 0: out[--i] = p[(c>>0)&3];
	    case 3: out[--i] = p[(c>>2)&3];
	    case 2: out[--i] = p[(c>>4)&3];
	    case 1: out[--i] = p[(c>>6)&3];
		c = data[j--];
	    }
	}
#else
	olen = out_len & ~3;
	for (i = 0; i < olen; i+=4) {
	    c = data[j++];
	    out[i+0] = p[(c>>6)&3];
	    out[i+1] = p[(c>>4)&3];
	    out[i+2] = p[(c>>2)&3];
	    out[i+3] = p[(c>>0)&3];
	}
	if (out_len != olen) {
	    c = data[j++];
	    switch (out_len - olen) {
		case 4: out[i+3] = p[(c>>0)&3];
		case 3: out[i+2] = p[(c>>2)&3];
		case 2: out[i+1] = p[(c>>4)&3];
		case 1: out[i+0] = p[(c>>6)&3];
	    }
	}
#endif
	break;

    case 2:
	olen = out_len & ~1;
	for (i = 0; i < olen; i+=2) {
	    c = data[j++];
	    out[i+0] = p[(c>>4)&15];
	    out[i+1] = p[(c>>0)&15];
	}
	if (out_len != olen) {
	    c = data[j++];
	    out[i+0] = p[(c>>4)&15];
	}
	break;

    case 0:
	memset(out, p[0], out_len);
	break;

    default:
	free(out);
	return NULL;
    }

    return out;
}

#if 0
// Marginally faster, but not worth extra complexity currently until we
// optimise further
static uint8_t *rle_decode_unpack(uint8_t *in, int64_t in_len, uint8_t *meta, int nsym, uint8_t *p,
				  unsigned char *out, int64_t out_len) {
    uint64_t o_len = in_len;
    uint64_t i, j, m;

    int saved[256] = {0};
    for (i = 0, j = in[i++]; j; j--)
	saved[in[i++]]=1;

    j = m = 0;
    switch (nsym) {
    case 0:
	return NULL; // shouldn't happen.

    case 1:
	while (i < in_len) {
	    uint8_t b = in[i++];
	    if (saved[b]) {
		uint32_t run_len = 0;
		unsigned char c, s = 0;
		do {
		    //c = in[i++];
		    c = meta[m++];
		    run_len |= (c & 0x7f) << s;
		    s += 7;
		} while (c & 0x80);
		run_len++;
		//fprintf(stderr, "run_len=%d %x\n", run_len, run_len);
		memset(&out[j], b, run_len);
		j += run_len;
	    } else {
		out[j++] = b;
	    }
	}
	break;

    case 2:
	while (i < in_len) {
	    uint8_t b = in[i++];
	    if (saved[b]) {
		uint32_t run_len = 0;
		unsigned char c, s = 0;
		do {
		    //c = in[i++];
		    c = meta[m++];
		    run_len |= (c & 0x7f) << s;
		    s += 7;
		} while (c & 0x80);
		run_len++;
		//fprintf(stderr, "run_len=%d %x\n", run_len, run_len);
		int z;
		for (z = 0; z < run_len; z++, j+=2) {
		    out[j+0] = p[(b>>4)&15];
		    out[j+1] = p[(b>>0)&15];
		}
	    } else {
		out[j+0] = p[(b>>4)&15];
		out[j+1] = p[(b>>0)&15];
		j += 2;
	    }
	}
	break;

    case 4:
	while (i < in_len) {
	    uint8_t b = in[i++];
	    if (saved[b]) {
		uint32_t run_len = 0;
		unsigned char c, s = 0;
		do {
		    //c = in[i++];
		    c = meta[m++];
		    run_len |= (c & 0x7f) << s;
		    s += 7;
		} while (c & 0x80);
		run_len++;
		//fprintf(stderr, "run_len=%d %x\n", run_len, run_len);
		int z;
		for (z = 0; z < run_len; z++, j+=4) {
		    out[j+0] = p[(b>>6)&3];
		    out[j+1] = p[(b>>4)&3];
		    out[j+2] = p[(b>>2)&3];
		    out[j+3] = p[(b>>0)&3];
		}
	    } else {
		out[j+0] = p[(b>>6)&3];
		out[j+1] = p[(b>>4)&3];
		out[j+2] = p[(b>>2)&3];
		out[j+3] = p[(b>>0)&3];
		j += 4;
	    }
	}
	break;

    case 8:
	while (i < in_len) {
	    uint8_t b = in[i++];
	    if (saved[b]) {
		uint32_t run_len = 0;
		unsigned char c, s = 0;
		do {
		    //c = in[i++];
		    c = meta[m++];
		    run_len |= (c & 0x7f) << s;
		    s += 7;
		} while (c & 0x80);
		run_len++;
		//fprintf(stderr, "run_len=%d %x\n", run_len, run_len);
		int z;
		for (z = 0; z < run_len; z++, j+=8) {
		    out[j+0] = p[(b>>7)&1];
		    out[j+1] = p[(b>>6)&1];
		    out[j+2] = p[(b>>5)&1];
		    out[j+3] = p[(b>>4)&1];
		    out[j+4] = p[(b>>3)&1];
		    out[j+5] = p[(b>>2)&1];
		    out[j+6] = p[(b>>1)&1];
		    out[j+7] = p[(b>>0)&1];
		}
	    } else {
		out[j+0] = p[(b>>7)&1];
		out[j+1] = p[(b>>6)&1];
		out[j+2] = p[(b>>5)&1];
		out[j+3] = p[(b>>4)&1];
		out[j+4] = p[(b>>3)&1];
		out[j+5] = p[(b>>2)&1];
		out[j+6] = p[(b>>1)&1];
		out[j+7] = p[(b>>0)&1];
		j += 8;
	    }
	}
	break;
    }

    return out;
}
#endif

//-----------------------------------------------------------------------------

unsigned char *rans_compress_O1_4x16(unsigned char *in, unsigned int in_size,
				     unsigned char *out, unsigned int *out_size) {
    unsigned char *cp, *out_end, *op;
    unsigned int tab_size, rle_i, rle_j;
    RansEncSymbol syms[256][256];
    int bound = rans_compress_bound_4x16(in_size,1)-5; // -5 for order/size

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

    op = cp;
    *cp++ = 0; // uncompressed header marker

    // Encode the order-0 symbols for use in the order-1 frequency tables
    int F0[256+MAGIC] = {0};
    present8(in, in_size, F0);

    int n = encode_freq0(cp, F0);
    //fprintf(stderr, "tab0part=%d\n", (int)n);
    cp += n;

    F[0][in[1*(in_size>>2)]]++;
    F[0][in[2*(in_size>>2)]]++;
    F[0][in[3*(in_size>>2)]]++;
    T[0]+=3;

    
    // Normalise so T[i] == TOTFREQ_O1
    for (rle_i = i = 0; i < 256; i++) {
	int t2, m, M;
	unsigned int x;

	if (T[i] == 0)
	    continue;

	// Store frequency table; outer level.
	// i
	if (rle_i) {
	    rle_i--;
	} else {
	    *cp++ = i;
	    if (i && T[i-1]) {
		for(rle_i=i+1; rle_i<256 && T[rle_i]; rle_i++)
		    ;
		rle_i -= i+1;
		*cp++ = rle_i;
	    }
	}

#ifdef FAST
	normalise_freq(F[i], T[i], TOTFREQ_O1);
	cp += encode_freq_d(cp, F0, F[i]);
#else
	// Order-1 frequencies often end up totalling under TOTFREQ.
	// In this case it's smaller to output the real frequencies
	// prior to normalisation and normalise after (with an extra
	// normalisation step needed in the decoder too).
	if (T[i] > TOTFREQ_O1)
	    normalise_freq(F[i], T[i], TOTFREQ_O1);

	cp += encode_freq_d(cp, F0, F[i]);

	if (T[i] < TOTFREQ_O1)
	    normalise_freq(F[i], T[i], TOTFREQ_O1);
#endif

	int *F_i_ = F[i];
	for (x = j = 0; j < 256; j++) {
	    RansEncSymbolInit(&syms[i][j], x, F_i_[j], TF_SHIFT_O1);
	    x += F_i_[j];
	}

    }
    *cp++ = 0;

    if (cp - op > 1000 && cp - op < 100000) {
	// try rans0 compression of header
	unsigned int c_freq_sz;
	unsigned char *c_freq = rans_compress_O0_4x16(op+1, cp-(op+1), NULL, &c_freq_sz);
	if (c_freq && c_freq_sz < 65536 && c_freq_sz + 3 < cp-op) {
	    *op++ = 1; // compressed
	    *op++ = c_freq_sz & 0xff;
	    *op++ = c_freq_sz>>8;
	    memcpy(op, c_freq, c_freq_sz);
	    cp = op+c_freq_sz;
	}
	free(c_freq);
    }

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

typedef struct {
    uint16_t f;
    uint16_t b;
} sb_t;

unsigned char *rans_uncompress_O1sfb_4x16(unsigned char *in, unsigned int in_size,
					  unsigned char *out, unsigned int *out_size) {
    /* Load in the static tables */
    unsigned char *cp = in + 4, *out_orig = NULL;
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

    // compressed header? If so uncompress it
    unsigned char *tab_end = NULL;
    unsigned char *c_freq = NULL;
    if (*cp++ == 1) {
	unsigned int c_freq_sz = cp[0] | (cp[1]<<8), u_freq_sz;
	cp += 2;
	tab_end = cp + c_freq_sz;
	c_freq = rans_uncompress_O0_4x16(cp, c_freq_sz, NULL, &u_freq_sz);
	cp = c_freq;
    }

    // Decode order-0 symbol list; avoids needing in order-1 tables
    int F0[256] = {0};
    cp += decode_freq0(cp, F0);

    //i = *cp++;
    rle_i = 0;
    i = *cp++;
    do {
	int F[256] = {0}, T;
	cp += decode_freq_d(cp, F0, F, NULL, NULL, &T);

	if (T < TOTFREQ_O1)
	    normalise_freq(F, T, TOTFREQ_O1);

	// Build symbols; fixme, do as part of decode, see the _d variant
	for (j = x = 0; j < 256; j++) {
	    if (F[j]) {
		for (y = 0; y < F[j]; y++) {
		    ssym[i][y + x]   = j;
		    sfb [i][y + x].f = F[j];
		    sfb [i][y + x].b = y;
		}
		x += F[j];
	    }
	}

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

    if (tab_end)
	cp = tab_end;
    if (c_freq)
	free(c_freq);

    RansState rans0, rans1, rans2, rans3;
    uint8_t *ptr = cp;
    RansDecInit(&rans0, &ptr);
    RansDecInit(&rans1, &ptr);
    RansDecInit(&rans2, &ptr);
    RansDecInit(&rans3, &ptr);

    int isz4 = out_sz>>2;
    int l0 = 0, l1 = 0, l2 = 0, l3 = 0;
    int i4[] = {0*isz4, 1*isz4, 2*isz4, 3*isz4};

    RansState R[4];
    R[0] = rans0;
    R[1] = rans1;
    R[2] = rans2;
    R[3] = rans3;

    const uint32_t mask = ((1u << TF_SHIFT_O1)-1);
    for (; i4[0] < isz4; i4[0]++, i4[1]++, i4[2]++, i4[3]++) {
	uint32_t m[4];
	uint32_t c[4];

	m[0] = R[0] & mask;
	R[0] = sfb[l0][m[0]].f * (R[0]>>TF_SHIFT_O1) + sfb[l0][m[0]].b;
	c[0] = ssym[l0][m[0]];

	m[1] = R[1] & mask;
	R[1] = sfb[l1][m[1]].f * (R[1]>>TF_SHIFT_O1) + sfb[l1][m[1]].b;
	c[1] = ssym[l1][m[1]];

	m[2] = R[2] & mask;
	R[2] = sfb[l2][m[2]].f * (R[2]>>TF_SHIFT_O1) + sfb[l2][m[2]].b;
	c[2] = ssym[l2][m[2]];

	m[3] = R[3] & mask;
	R[3] = sfb[l3][m[3]].f * (R[3]>>TF_SHIFT_O1) + sfb[l3][m[3]].b;
	c[3] = ssym[l3][m[3]];

	// TODO: inline expansion of 4-way packing here is about 4% faster.
	out[i4[0]] = c[0];
	out[i4[1]] = c[1];
	out[i4[2]] = c[2];
	out[i4[3]] = c[3];

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

    return out;
}

/*-----------------------------------------------------------------------------
 * Simple interface to the order-0 vs order-1 encoders and decoders.
 */
unsigned char *rans_compress_to_4x16(unsigned char *in,  unsigned int in_size,
				     unsigned char *out, unsigned int *out_size,
				     int order) {
    unsigned int c_meta_len;
    uint8_t *meta = NULL, *rle = NULL, *packed = NULL;

    int do_pack = order & X_PACK;
    int do_rle  = order & X_RLE;

    if (!out) {
	*out_size = rans_compress_bound_4x16(in_size, order); //fixme, +pack, +rle
	out = malloc(*out_size);
    }

    out[0] = order;
    c_meta_len = 1;

    if (do_pack || do_rle) {
	*(uint32_t *)(&out[1]) = in_size;
	c_meta_len += 4;
    }

    order &= 0xf;

    // Format is compressed meta-data, compressed data.
    // Meta-data can be empty, pack, rle lengths, or pack + rle lengths.
    // Data is either the original data, bit-packed packed, rle literals or
    // packed + rle literals.

    if (do_pack) {
	// PACK 2, 4 or 8 symbols into one byte.
	int pmeta_len;
	uint64_t packed_len;
	packed = pack(in, in_size, out+c_meta_len, &pmeta_len, &packed_len);
	if (pmeta_len == 1 && out[c_meta_len] == 1) {
	    out[0] &= ~X_PACK;
	    do_pack = 0;
	    if (!do_rle)
		c_meta_len -= 4;
	    free(packed);
	    packed = NULL;
	} else {
	    in = packed;
	    in_size = packed_len;
	    c_meta_len += pmeta_len;
	}
    }

    if (do_rle) {
	// RLE 'in' -> rle_length + rle_literals arrays
	unsigned int rmeta_len, c_rmeta_len;
	uint64_t rle_len;
	c_rmeta_len = in_size;
	if (!(meta = malloc(c_rmeta_len)))
	    return NULL;

	rle = rle_encode(in, in_size, meta, &rmeta_len, &rle_len);
	if (rle_len + rmeta_len >= .99*in_size) {
	    // Not worth the speed hit.
	    out[0] &= ~X_RLE;
	    do_rle = 0;
	    if (!do_pack)
		c_meta_len -= 4;
	    free(rle);
	    rle = NULL;
	} else {
	    // Compress lengths with O0 and literals with O0/O1 ("order" param)
	    rans_compress_O0_4x16(meta, rmeta_len, out+c_meta_len+4, &c_rmeta_len);
	    *(uint32_t *)(out+c_meta_len) = c_rmeta_len;
	    c_meta_len += 4 + c_rmeta_len;

	    in = rle;
	    in_size = rle_len;
	}

	free(meta);
    }

    *out_size -= c_meta_len;
    if (order && in_size < 8) {
	out[0] &= ~1;
	order  &= ~1;
    }
    if (order)
	rans_compress_O1_4x16(in, in_size, out+c_meta_len, out_size);
    else
	rans_compress_O0_4x16(in, in_size, out+c_meta_len, out_size);
    free(rle);
    free(packed);

    *out_size += c_meta_len;
    return out;
}

unsigned char *rans_compress_4x16(unsigned char *in, unsigned int in_size,
				  unsigned int *out_size, int order) {
    return rans_compress_to_4x16(in, in_size, NULL, out_size, order);
}

unsigned char *rans_uncompress_to_4x16(unsigned char *in,  unsigned int in_size,
				       unsigned char *out, unsigned int *out_size) {
    int order = *in++;  in_size--;

    int do_pack = order & X_PACK;
    int do_rle  = order & X_RLE;
    order &= 0xf;

    if (!out) {
	// FIXME: check
	*out_size = ((in[0])<<0) | ((in[1])<<8) | ((in[2])<<16) | ((in[3])<<24);
	out = malloc(*out_size);
    }

    if (do_pack || do_rle)
	in += 4; // size field not needed when pure rANS

    uint32_t c_meta_size = 0;
    unsigned int tmp1_size = *out_size;
    unsigned int tmp2_size = *out_size;
    unsigned int tmp3_size = *out_size;
    unsigned char *tmp1, *tmp2, *tmp3, *tmp = NULL;

    // Need In, Out and Tmp buffers with temporary buffer of the same size
    // as output.  All use rANS, but with optional transforms (none, RLE,
    // Pack, or both).
    //
    //                    rans   unrle  unpack
    // If none:     in -> out
    // If RLE:      in -> tmp -> out
    // If Pack:     in -> tmp        -> out
    // If RLE+Pack: in -> out -> tmp -> out
    //                    tmp1   tmp2   tmp3
    //
    // So rans is in   -> tmp1
    // RLE     is tmp1 -> tmp2
    // Unpack  is tmp2 -> tmp3

    // Format is meta data (Pack and RLE in that order if present),
    // followed by rANS compressed data.

    if (do_pack || do_rle) {
	tmp = malloc(*out_size); // FIXME: check
	if (do_pack && do_rle) {
	    tmp1 = out;
	    tmp2 = tmp;
	    tmp3 = out;
	    // If we use rle_decode_unpack as a single step
	    //tmp1 = tmp2 = tmp;
	    //tmp3 = out;
	} else if (do_pack) {
	    tmp1 = tmp;
	    tmp2 = tmp1;
	    tmp3 = out;
	} else if (do_rle) {
	    tmp1 = tmp;
	    tmp2 = out;
	    tmp3 = out;
	}
    } else {
	// neither
	tmp  = NULL;
	tmp1 = out;
	tmp2 = out;
	tmp3 = out;
    }

    
    // Decode the bit-packing map.
    uint8_t map[16];
    int npacked_sym;
    uint64_t unpacked_sz;
    if (do_pack) {
	// FIXME: may need original size somewhere?
	c_meta_size = unpack_meta(in, *out_size, map, &npacked_sym, &unpacked_sz);
	in      += c_meta_size;
	in_size -= c_meta_size;
    }

    uint8_t *meta = NULL;
    if (do_rle) {
	// Uncompress meta data
	c_meta_size = *(uint32_t *)in;
	uint32_t meta_size;
	meta = rans_uncompress_O0_4x16(in+4, in_size-4, NULL, &meta_size);
	in      += c_meta_size+4;
	in_size -= c_meta_size+4;
    }
   
    // uncompress RLE data.  in -> tmp1
    tmp1 = order
	? rans_uncompress_O1sfb_4x16(in, in_size, tmp1, &tmp1_size)
	: rans_uncompress_O0_4x16(in, in_size, tmp1, &tmp1_size);
    tmp2_size = tmp3_size = tmp1_size;

//    if (do_rle && do_pack) {
//	// Unrle and unpack in one step
//	if (!rle_decode_unpack(tmp1, tmp1_size, meta, npacked_sym, map, tmp3, unpacked_sz))
//	    return NULL;
//	tmp3_size = unpacked_sz;
//    } else {
    if (do_rle) {
	// Unpack RLE.  tmp1 -> tmp2.
	uint64_t unrle_size;
	if (!rle_decode(tmp1, tmp1_size, meta, tmp2, &unrle_size))
	    return NULL;
	tmp3_size = tmp2_size = unrle_size;
	free(meta);
    }
    if (do_pack) {
	// Unpack bits via pack-map.  tmp2 -> tmp3
	if (npacked_sym == 1)
	    unpacked_sz = tmp2_size;
	//uint8_t *porig = unpack(tmp2, tmp2_size, unpacked_sz, npacked_sym, map);
	//memcpy(tmp3, porig, unpacked_sz);
	if (!unpack(tmp2, tmp2_size, tmp3, unpacked_sz, npacked_sym, map))
	    return NULL;
	tmp3_size = unpacked_sz;
    }

    if (tmp)
	free(tmp);

    *out_size = tmp3_size;
    return tmp3;
}

unsigned char *rans_uncompress_4x16(unsigned char *in, unsigned int in_size,
				    unsigned int *out_size) {
    return rans_uncompress_to_4x16(in, in_size, NULL, out_size);
}


#ifdef TEST_MAIN

#ifndef BLK_SIZE
#  define BLK_SIZE 1013*1047
#endif

// Room to allow for expanded BLK_SIZE on worst case compression.
#define BLK_SIZE2 ((105LL*BLK_SIZE)/100)

/*-----------------------------------------------------------------------------
 * Main
 */
static unsigned char in_buf[BLK_SIZE2+257*257*3];

int main(int argc, char **argv) {
    int opt, order = 0;
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

    //order = order ? 1 : 0; // Only support O(0) and O(1)

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
	    bc[nb].sz = rans_compress_bound_4x16(BLK_SIZE, order);
	    bc[nb].blk = malloc(bc[nb].sz);
	    bu[nb].sz = BLK_SIZE;
	    bu[nb].blk = malloc(BLK_SIZE);
	    nb++;
	    in_sz += len;
	}
	fprintf(stderr, "Testing %d blocks\n", nb);

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
		bu[i].blk = rans_uncompress_to_4x16(bc[i].blk, bc[i].sz, bu[i].blk, &bu[i].sz);

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

	    if (4 != fread(&in_size, 1, 4, infp))
		break;
	    if (in_size != fread(in_buf, 1, in_size, infp)) {
		fprintf(stderr, "Truncated input\n");
		exit(1);
	    }
	    out = rans_uncompress_4x16(in_buf, in_size, &out_size);
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

	    if (in_size < 4)
		order &= ~1;

	    out = rans_compress_4x16(in_buf, in_size, &out_size, order);

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

/*

r4x16_orig is original 16-bit rans code without any optimisation in O1
frequency table.

r4x16 is as 16-bit rans with an order-0 encoded freq table for O1 rans.

r4x16p  is r4x16 + bit-packing
r4x16r  is r4x16 + RLE
r4x16pr is r4x16 + bit-packing + RLE
r4x16A  is r4x16 + auto-sense of RLE & pack, also TF_SHIFT_O1=11 now

All are order-1 stats with TF_SHIFT_O1 = 12 (except r4x16A)

Sizes:
           r4x16_orig      r4x16     r4x16p     r4x16r    r4x16pr     r4x16A    bsc -e2 -b1 -p   qlfc(1M)
      qc      1211185    1146768    1146865     967259     967356     966417*     879244           889201
      q4       767367     767185     722214     742453     718175     713873*     682642           690656
      q8     15372654   15369060   14979244   15161338   14965968   14884319*   14887392         14802115
      q40    42341484	42323006*  42323096   42324035   42324125   42322813    41139094         44655332

Speeds (enc/dec MBps):
           r4x16_orig      r4x16     r4x16p     r4x16r    r4x16pr     r4x16A    bsc    qlfc(1M)
      qc      174/286    159/279    108/200    315/1677   317/1646   251/2518   56/36  66/?
      q4      167/331    153/329    311/433    150/381    211/523    266/507    21/22  30/?
      q8      152/289    141/290    182/260     77/132     82/129    112/204     8/10  11/?
      q40     122/173    113/172    109/172     74/119     52/80      69/209     5/5    3/?

Qc (crumble quality) is RLE heavy and benefits.  No bit-packing helps.
Q4 (4-qual illumina) and Q8 (8 qual) benefits from packing. RLE is marginal benefit.
Q40 (40 qual illumina) gains from neither packing nor RLE.

BWT doesn't save much, so qlfc alone is sufficient for good ratio.
However both bsc and qlfc are an order of magnitude slower. 

 */
