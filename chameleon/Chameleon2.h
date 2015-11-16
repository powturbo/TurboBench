#pragma once
#ifndef _CB_CHAMELEON2_H_
#define _CB_CHAMELEON2_H_
/*****************************************************************
*
* Chameleon2
*
* experimental ; SIMD proof of concept
*
* SSE3 & SSE4 code paths not even compiled yet ; YMMV
* SSE2 tested works
*
* set CHAMELEON_IMPL to get functions
*
* set CHAMELEON2_SSE_IMPL to get the SSE implementations
*
**************************************************************/

#include "Chameleon.h"

// Chameleon2 uses the same hash object as Chameleon
// call Chameleon_Reset

CHAMELEON_FUNC size_t Chameleon2_Encode(Chameleon * c,void * to,const void * from, size_t from_size);
CHAMELEON_FUNC size_t Chameleon2_Encode_SSE(Chameleon * c,void * to,const void * from, size_t from_size);
CHAMELEON_FUNC void Chameleon2_Decode(Chameleon * c,void * to,  size_t raw_size, const void * from);

//===================================================================

#ifdef CHAMELEON_IMPL

// toggle shift or mul for hash :
//#define C2HASH_SHIFTS 1  // 714
#define C2HASH_SHIFTS 0 // 689.35

#if C2HASH_SHIFTS

// shift-based hash for SSE2 SIMD that has no native 32*32 4-wide mul
// hmm doesn't seem to help speed at all
// compression is almost the same as mul hash

static inline uint32 c2hash(uint32 v)
{
	//uint32 h = v; h += v>>9; h += v>>13;  // 63193974
	//uint32 h = v; h += v>>9; h += _lrotl(v,11); // 62971018
	//uint32 h = v; h += v>>7; h += _lrotl(v,11); // 62612866
	//return h&0xFFFF; 
	// good , but SSE2 doesn't have a native rotate!
	//uint32 h = v; h += _lrotr(v,7); h += _lrotl(v,11); // 62466024
	//uint32 h = v; h += v>>13; h += v>>17; // 63224594
	//uint32 h = v; h += v>>13; h += v>>19; // 62940964
	//uint32 h = v; h += v>>13; h += v>>23; // 62818854
	//return h&0xFFFF;
	//uint32 h = v; h += v<<7; h += v<<19; // 62567904
	//uint32 h = v; h += v<<11; h += v<<19; // 62489364 ; 19328740
	//uint32 h = v; h += v<<11; h += h<<7; // 62340190 ; 19387110
	uint32 h = v; h ^= v<<11; h ^= v<<19; // 62629378
	return h>>16;
}

#define CHAMELEON2_HASH(v)	c2hash(v)

#else

#define CHAMELEON2_HASH_MULTIPLIER         2641295638lu
#define CHAMELEON2_HASH(v)	(((uint32)((v)*CHAMELEON2_HASH_MULTIPLIER))>>16)

#endif

// non-SIMD encoder
// faster on SSE2 than the SIMD encoder

CHAMELEON_FUNC size_t Chameleon2_Encode(Chameleon * c,void * to,const void * from, size_t from_size)
{
	uint8 * to8 = (uint8 *)to;
		
	size_t from_size32 = from_size >> 2;
	const uint32 * fm32 = (const uint32 *)from;

	size_t from_size32_8 = from_size32 >> 3;
	size_t from_size32_tail = from_size32 & 7;

	uint32 * hash = c->hash;

	while( from_size32_8-- )
	{
		uint32 flags = 0;
		
		uint32 c0 = fm32[0]; uint32 h0 = CHAMELEON2_HASH(c0); uint32 check0 = hash[h0] == c0;
		uint32 c1 = fm32[1]; uint32 h1 = CHAMELEON2_HASH(c1); uint32 check1 = hash[h1] == c1;
		uint32 c2 = fm32[2]; uint32 h2 = CHAMELEON2_HASH(c2); uint32 check2 = hash[h2] == c2;
		uint32 c3 = fm32[3]; uint32 h3 = CHAMELEON2_HASH(c3); uint32 check3 = hash[h3] == c3;
		
		uint32 mask0 = (check0) ? 0 : -1;
		uint32 mask1 = (check1) ? 0 : -1;
		uint32 mask2 = (check2) ? 0 : -1;
		uint32 mask3 = (check3) ? 0 : -1;
		
		flags += mask0 & (1<<0);
		flags += mask1 & (1<<1);
		flags += mask2 & (1<<2);
		flags += mask3 & (1<<3);
		
		uint32 out0 = h0 + (mask0 & (c0 - h0));
		uint32 out1 = h1 + (mask1 & (c1 - h1));
		uint32 out2 = h2 + (mask2 & (c2 - h2));
		uint32 out3 = h3 + (mask3 & (c3 - h3));
		
		hash[h0] = c0;
		hash[h1] = c1;
		hash[h2] = c2;
		hash[h3] = c3;
		
		uint32 c4 = fm32[4]; uint32 h4 = CHAMELEON2_HASH(c4); uint32 check4 = hash[h4] == c4;
		uint32 c5 = fm32[5]; uint32 h5 = CHAMELEON2_HASH(c5); uint32 check5 = hash[h5] == c5;
		uint32 c6 = fm32[6]; uint32 h6 = CHAMELEON2_HASH(c6); uint32 check6 = hash[h6] == c6;
		uint32 c7 = fm32[7]; uint32 h7 = CHAMELEON2_HASH(c7); uint32 check7 = hash[h7] == c7;
		
		uint32 mask4 = (check4) ? 0 : -1;
		uint32 mask5 = (check5) ? 0 : -1;
		uint32 mask6 = (check6) ? 0 : -1;
		uint32 mask7 = (check7) ? 0 : -1;
	
		flags += mask4 & (1<<4);
		flags += mask5 & (1<<5);
		flags += mask6 & (1<<6);
		flags += mask7 & (1<<7);
		
		uint32 out4 = h4 + (mask4 & (c4 - h4));
		uint32 out5 = h5 + (mask5 & (c5 - h5));
		uint32 out6 = h6 + (mask6 & (c6 - h6));
		uint32 out7 = h7 + (mask7 & (c7 - h7));
		
		hash[h4] = c4;
		hash[h5] = c5;
		hash[h6] = c6;
		hash[h7] = c7;
		
		*to8++ = (uint8)flags;
				
		*((uint32 *)to8) = out0; to8 += 2 + (mask0 & 2);
		*((uint32 *)to8) = out1; to8 += 2 + (mask1 & 2);
		*((uint32 *)to8) = out2; to8 += 2 + (mask2 & 2);
		*((uint32 *)to8) = out3; to8 += 2 + (mask3 & 2);
		*((uint32 *)to8) = out4; to8 += 2 + (mask4 & 2);
		*((uint32 *)to8) = out5; to8 += 2 + (mask5 & 2);
		*((uint32 *)to8) = out6; to8 += 2 + (mask6 & 2);
		*((uint32 *)to8) = out7; to8 += 2 + (mask7 & 2);

		fm32 += 8;		
	}

	/*
	// @@ todo 
	if ( from_size32_tail > 0 )
	{
	}
	*/
		
	size_t tail_count = from_size32_tail*4 + (from_size&3);
	ASSERT( tail_count == (from_size&31) );
	if ( tail_count > 0 )
		minimemcpy(to8,fm32,tail_count); 

	return ((char *)to8 - (char *)to) + tail_count;
}

#ifdef CHAMELEON2_SSE_IMPL

// SSE SIMD encoder

#if defined(__clang__)
#include <x86intrin.h>
#else
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

#ifdef __SSE3__ 
#include <pmmintrin.h>
#endif

#ifdef __SSE4_1__ 
#include <smmintrin.h>
#endif

#ifdef _MSC_VER

#define ALIGN16 __declspec(align,16)

#elif defined(__clang__) || defined(__GNUC__)

#define ALIGN16 __attribute__ ((aligned(16)))

#else

#error ALIGN16

#endif



#if C2HASH_SHIFTS

static inline __m128i simd_c2hash(const __m128i &a)
{
	__m128i h = _mm_xor_si128( 
		_mm_xor_si128(a,
		_mm_slli_epi32(a,11)),
		_mm_slli_epi32(a,19) );
	return _mm_srli_epi32( h , 16 );
}

#else
       
static inline __m128i simd_mul32(const __m128i &a, const __m128i &b)
{
#ifdef __SSE4_1__  // modern CPU - use SSE 4.1
    return _mm_mullo_epi32(a, b);
#else               // old CPU - use SSE 2
    __m128i tmp1 = _mm_mul_epu32(a,b); /* mul 2,0*/
    __m128i tmp2 = _mm_mul_epu32( _mm_srli_si128(a,4), _mm_srli_si128(b,4)); /* mul 3,1 */
    return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE (0,0,2,0)), _mm_shuffle_epi32(tmp2, _MM_SHUFFLE (0,0,2,0))); /* shuffle results to [63..0] and pack */
#endif
}

#endif // SSE4

// SS4 : _mm_extract_epi32
//  //.m128i_u32[i];
#ifdef __SSE4_1__  // modern CPU - use SSE 4.1
static inline int get_0(const __m128i& vec){return _mm_extract_epi32(vec,0);}
static inline int get_1(const __m128i& vec){return _mm_extract_epi32(vec,1);}
static inline int get_2(const __m128i& vec){return _mm_extract_epi32(vec,2);}
static inline int get_3(const __m128i& vec){return _mm_extract_epi32(vec,3);}
#else
static inline int get_0(const __m128i& vec){return _mm_cvtsi128_si32 (vec);}
static inline int get_1(const __m128i& vec){return _mm_cvtsi128_si32 (_mm_shuffle_epi32(vec,0x55));}
static inline int get_2(const __m128i& vec){return _mm_cvtsi128_si32 (_mm_shuffle_epi32(vec,0xAA));}
static inline int get_3(const __m128i& vec){return _mm_cvtsi128_si32 (_mm_shuffle_epi32(vec,0xFF));}
#endif // SSE4

// nasty pack-down store :
// storing either 2 or 4 bytes of each field

#ifdef __SSE3__

// with pshufb use a table lookup on [flags&0xF]
// _mm_shuffle_epi8 easy peasy

// c_pack_2_4_store_count = 8 + 2*popcnt(flags&0xF);
static const int c_pack_2_4_store_count[16] = {
	8,8+2,8+2,8+2*2,8+2,8+2*2,8+2*2,8+2*3,
	10,10+2,10+2,10+2*2,10+2,10+2*2,10+2*2,10+2*3
};

static const ALIGN16 uint8 c_pack_2_4_store_shuffles[16][16] = {
	/* 2222 */ { 0,1,     4,5,     8,9,       12,13 },
	/* 4222 */ { 0,1,2,3, 4,5,     8,9,       12,13 },
	/* 2422 */ { 0,1,     4,5,6,7, 8,9,       12,13 },
	/* 4422 */ { 0,1,2,3, 4,5,6,7, 8,9,       12,13 },
	/* 2242 */ { 0,1,     4,5,     8,9,10,11, 12,13 },
	/* 4242 */ { 0,1,2,3, 4,5,     8,9,10,11, 12,13 },
	/* 2442 */ { 0,1,     4,5,6,7, 8,9,10,11, 12,13 },
	/* 4442 */ { 0,1,2,3, 4,5,6,7, 8,9,10,11, 12,13 },
	/* 2224 */ { 0,1,     4,5,     8,9,       12,13,14,15 },
	/* 4224 */ { 0,1,2,3, 4,5,     8,9,       12,13,14,15 },
	/* 2424 */ { 0,1,     4,5,6,7, 8,9,       12,13,14,15 },
	/* 4424 */ { 0,1,2,3, 4,5,6,7, 8,9,       12,13,14,15 },
	/* 2244 */ { 0,1,     4,5,     8,9,10,11, 12,13,14,15 },
	/* 4244 */ { 0,1,2,3, 4,5,     8,9,10,11, 12,13,14,15 },
	/* 2444 */ { 0,1,     4,5,6,7, 8,9,10,11, 12,13,14,15 },
	/* 4444 */ { 0,1,2,3, 4,5,6,7, 8,9,10,11, 12,13,14,15 },
};

static inline uint8 * pack_2_4_store(uint8 * to8,__m128i out,uint32 flags)
{
	flags = flags&0xF;
	__m128i mask = _mm_load_si128((const __m128i *)c_pack_2_4_store_shuffles[flags]);
	__m128i packed = _mm_shuffle_epi8(out,mask);
	_mm_storeu_si128((__m128i *)to8,packed);
	to8 += c_pack_2_4_store_count[flags];
	return to8;
}

#else // SSE3
		
static inline uint8 * pack_2_4_store(uint8 * to8,__m128i out,uint32 flags)
{
	// uses little-endian to store word or dword :
	*((uint32 *)to8) = get_0(out); to8 += 2 + ((flags<<1) & 2);
	*((uint32 *)to8) = get_1(out); to8 += 2 + ((flags   ) & 2);
	*((uint32 *)to8) = get_2(out); to8 += 2 + ((flags>>1) & 2);
	*((uint32 *)to8) = get_3(out); to8 += 2 + ((flags>>2) & 2);
	return to8;
}

#endif // SSE3
		
CHAMELEON_FUNC size_t Chameleon2_Encode_SSE(Chameleon * c,void * to,const void * from, size_t from_size)
{
	uint8 * to8 = (uint8 *)to;
		
	size_t from_size32 = from_size >> 2;
	const uint32 * fm32 = (const uint32 *)from;

	size_t from_size32_8 = from_size32 >> 3;
	size_t from_size32_tail = from_size32 & 7;

	uint32 * hash = c->hash;

	#if ! C2HASH_SHIFTS
	__m128i hash_mul =  _mm_set1_epi32( CHAMELEON2_HASH_MULTIPLIER );
	#endif
	
	while( from_size32_8-- )
	{			
		//-----------------------------------------
		
        __m128i c0 = _mm_loadu_si128((const __m128i *)fm32);        
        #if C2HASH_SHIFTS
        __m128i h0 = simd_c2hash(c0);
        #else
        __m128i h0 = _mm_srli_epi32( simd_mul32(c0,hash_mul) , 16 );
        #endif
        
        // gather load :
        uint32 * ph00 = hash + get_0(h0);
        uint32 * ph01 = hash + get_1(h0);
        uint32 * ph02 = hash + get_2(h0);
        uint32 * ph03 = hash + get_3(h0);
        __m128i load = _mm_setr_epi32(*ph00,*ph01,*ph02,*ph03);
		
		__m128i mask0 = _mm_cmpeq_epi32(load,c0); // if equal, mask is ffff
		// mask is the opposite of non-simd case
				
		#if 0 //def __SSE4_1__ // works but slower
		__m128i out0 = _mm_blendv_epi8(c0, h0, mask0);
		#else
		__m128i out0 = _mm_or_si128(_mm_and_si128(h0, mask0), _mm_andnot_si128(mask0, c0));
		#endif

		//__m128i out0 = _mm_xor_si128( c0, _mm_and_si128 ( mask0, _mm_xor_si128( h0, c0 ) ) );
		// out = h or c    
    	
    	// scatter store :
    	*ph00 = get_0(c0);
    	*ph01 = get_1(c0);
    	*ph02 = get_2(c0);
    	*ph03 = get_3(c0);
		
		fm32 += 4;
		//-----------------------------------------
		
        __m128i c1 = _mm_loadu_si128((const __m128i *)fm32);        
        #if C2HASH_SHIFTS     
        __m128i h1 = simd_c2hash(c1);
        #else
        __m128i h1 = _mm_srli_epi32( simd_mul32(c1,hash_mul) , 16 );
        #endif
        
        // gather load :
        uint32 * ph10 = hash + get_0(h1);
        uint32 * ph11 = hash + get_1(h1);
        uint32 * ph12 = hash + get_2(h1);
        uint32 * ph13 = hash + get_3(h1);
        load = _mm_setr_epi32(*ph10,*ph11,*ph12,*ph13);
		
		__m128i mask1 = _mm_cmpeq_epi32(load,c1); // if equal, mask is ffff
		// mask is the opposite of non-simd case			
		#if 0 //def __SSE4_1__ // blendv works but slower
		__m128i out1 = _mm_blendv_epi8(c1, h1, mask1);
		#else
		__m128i out1 = _mm_or_si128(_mm_and_si128(h1, mask1), _mm_andnot_si128(mask1, c1));
		#endif
		// out = h or c    
    	
    	// scatter store :
    	*ph10 = get_0(c1);
    	*ph11 = get_1(c1);
    	*ph12 = get_2(c1);
    	*ph13 = get_3(c1); //.m128i_u32[3];
	
		fm32 += 4;
		//-----------------------------------------
		
		#if defined(__clang__)
		// flags reversed <- could remove this
		uint32 flags0 = ~ _mm_movemask_ps((__m128)(mask0));
		uint32 flags1 = ~ _mm_movemask_ps((__m128)(mask1));
		uint32 flags = (flags0&0xF) | (flags1<<4);
		
		*to8++ = (uint8)flags;
				
		to8 = pack_2_4_store(to8,out0,flags0);
		to8 = pack_2_4_store(to8,out1,flags1);
		
		#else
	
		// MSVC needs _mm_castsi128_ps which is VC2010 +
		//uint32 flags0 = _mm_movemask_ps(_mm_castsi128_ps(mask0));
		//uint32 flags1 = _mm_movemask_ps(_mm_castsi128_ps(mask1));
		//uint32 flags = flags0 + (flags1<<4);
		__m128i mask01 = _mm_packs_epi32(mask0,mask1);
		mask01 = _mm_packs_epi16(mask01,mask01);
		uint32 flags = _mm_movemask_epi8(mask01);
		// flags are (1<<0) for the first dword, ...		
		flags = ~flags; // opposite sense <- could remove this
		*to8++ = (uint8)flags;
				
		to8 = pack_2_4_store(to8,out0,(flags));
		to8 = pack_2_4_store(to8,out1,(flags>>4));
		
		#endif
	}
	
	/*
	// @@ todo 
	if ( from_size32_tail > 0 )
	{
	}
	*/
		
	size_t tail_count = from_size32_tail*4 + (from_size&3);
	ASSERT( tail_count == (from_size&31) );
	if ( tail_count > 0 )
		minimemcpy(to8,fm32,tail_count); 

	return ((char *)to8 - (char *)to) + tail_count;
}

#endif

CHAMELEON_FUNC void Chameleon2_Decode(Chameleon * c,void * to,  size_t raw_size, const void * from)
{
	const uint8 * fm8 = (uint8 *)from;
		
	size_t raw_size32 = raw_size >> 2;
	uint32 * to32 = (uint32 *)to;

	size_t raw_size32_8 = raw_size32 >> 3;
	size_t raw_size32_tail = raw_size32 & 7;
	
	uint32 * hash = c->hash;
	
	while( raw_size32_8-- )
	{			
		uint32 flags = *fm8++;
	
		uint32 c0,c1,c2,c3;
		uint32 h0,h1,h2,h3;
	
		if ( flags & (1<<0) ) { c0 = *((const uint32 *)fm8); fm8 += 4; h0 = CHAMELEON2_HASH(c0); }
		else { h0 = *((const uint16 *)fm8); fm8 += 2; c0 = hash[h0]; }
	
		if ( flags & (1<<1) ) { c1 = *((const uint32 *)fm8); fm8 += 4; h1 = CHAMELEON2_HASH(c1); }
		else { h1 = *((const uint16 *)fm8); fm8 += 2; c1 = hash[h1]; }
		
		if ( flags & (1<<2) ) { c2 = *((const uint32 *)fm8); fm8 += 4; h2 = CHAMELEON2_HASH(c2); }
		else { h2 = *((const uint16 *)fm8); fm8 += 2; c2 = hash[h2]; }
		
		if ( flags & (1<<3) ) { c3 = *((const uint32 *)fm8); fm8 += 4; h3 = CHAMELEON2_HASH(c3); }
		else { h3 = *((const uint16 *)fm8); fm8 += 2; c3 = hash[h3]; }
		
		to32[0] = c0;
		to32[1] = c1;
		to32[2] = c2;
		to32[3] = c3;
		
		hash[h0] = c0;
		hash[h1] = c1;
		hash[h2] = c2;
		hash[h3] = c3;
		
		uint32 c4,c5,c6,c7;
		uint32 h4,h5,h6,h7;
		
		if ( flags & (1<<4) ) { c4 = *((const uint32 *)fm8); fm8 += 4; h4 = CHAMELEON2_HASH(c4); }
		else { h4 = *((const uint16 *)fm8); fm8 += 2; c4 = hash[h4]; }
		
		if ( flags & (1<<5) ) { c5 = *((const uint32 *)fm8); fm8 += 4; h5 = CHAMELEON2_HASH(c5); }
		else { h5 = *((const uint16 *)fm8); fm8 += 2; c5 = hash[h5]; }
		
		if ( flags & (1<<6) ) { c6 = *((const uint32 *)fm8); fm8 += 4; h6 = CHAMELEON2_HASH(c6); }
		else { h6 = *((const uint16 *)fm8); fm8 += 2; c6 = hash[h6]; }
		
		if ( flags & (1<<7) ) { c7 = *((const uint32 *)fm8); fm8 += 4; h7 = CHAMELEON2_HASH(c7); }
		else { h7 = *((const uint16 *)fm8); fm8 += 2; c7 = hash[h7]; }
		
		to32[4] = c4;
		to32[5] = c5;
		to32[6] = c6;
		to32[7] = c7;
		to32 += 8;
		
		hash[h4] = c4;
		hash[h5] = c5;
		hash[h6] = c6;
		hash[h7] = c7;
		
	}
	
	/*
	// @@ todo
	if ( raw_size32_tail > 0 )
	{
	}
	*/
		
	size_t tail_count = raw_size32_tail*4 + (raw_size&3);
	ASSERT( tail_count == (raw_size&31) );
	if ( tail_count > 0 )
		minimemcpy(to32,fm8,tail_count); 
}

#endif // CHAMELEON_IMPL

#endif // _CB_CHAMELEON2_H_
