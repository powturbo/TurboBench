#pragma once
#ifndef _CB_CHAMELEON_H_
#define _CB_CHAMELEON_H_

/*********************************************************
*
* Chameleon
*
* implementation by cbloom
*
* very fast encoding symmetric compressor
*
* Original Chameleon algorithm is by Centaurean aka gpnuma aka Guillaume Voirin ?
* see :
* https://github.com/centaurean/density
*
*
* WARNING : I have not done endian-agnosticism here
*  I also have not checked for compressed buffer overrun
*  so allocate enough for expansion (CHAMELEON_MAXIMUM_OUTPUT_SIZE)
*
* !! THIS IS NOT PRODUCTION CODE INTENDED FOR USE AS-IS !!
*
* #define CHAMELEON_IMPL to get the implementations
*
**********************************************************/

/*********************************************************

// Usage :

Chameleon c;

Chameleon_Reset(&c);

comp_len = Chameleon_Encode(&c, comp_buf, in_buf, in_size );

Chameleon_Reset(&c);

Chameleon_Decode(&c, out_buf, in_size, comp_buf );

**********************************************************/

#ifdef _MSC_VER

#ifndef uint8
#define uint8 unsigned char
#endif
#ifndef int16
#define int16 __int16
#endif
#ifndef uint16
#define uint16 unsigned __int16
#endif
#ifndef uint32
#define uint32 unsigned __int32
#endif

#elif defined(__clang__) || defined(__GNUC__)

#include <stdint.h>

#ifndef uint8
#define uint8 uint8_t
#endif
#ifndef int16
#define int16 int16_t
#endif
#ifndef uint16
#define uint16 uint16_t
#endif
#ifndef uint32
#define uint32 uint32_t
#endif

#else

#error types

#endif


#ifndef CHAMELEON_FUNC
#define CHAMELEON_FUNC
#endif

// use CHAMELEON_FUNC to get a func prefix
// #define CHAMELEON_FUNC static // or whatever

//===================================================================

typedef struct Chameleon
{
	uint32	hash[1<<16];
} Chameleon;

// you must Reset before Encode or Decode to keep the Chameleon in sync :

CHAMELEON_FUNC void Chameleon_Reset(Chameleon * c);

CHAMELEON_FUNC size_t Chameleon_Encode(Chameleon * c,void * to,const void * from, size_t from_size);

CHAMELEON_FUNC void Chameleon_Decode(Chameleon * c,void * to,  size_t raw_size, const void * from);

// #define CHAMELEON_IMPL // to get the code

#define CHAMELEON_MAXIMUM_OUTPUT_SIZE(in_size)	(((33*(in_size))>>5)+2)

//===================================================================

#ifdef CHAMELEON_IMPL

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifndef ASSERT
#define ASSERT	assert
#endif

#ifdef UNROLL4
#error WTF UNROLL4
#endif
#define UNROLL4(op)	op; op; op; op
#define UNROLL8(op)	UNROLL4(op); UNROLL4(op)
#define UNROLL16(op) UNROLL8(op); UNROLL8(op)

CHAMELEON_FUNC void Chameleon_Reset(Chameleon * c)
{
	memset(c,0,sizeof(Chameleon));
}

#define CHAMELEON_HASH_MULTIPLIER         ((uint32)2641295638LU)
#define CHAMELEON_HASH(v)	(((uint32)((v)*CHAMELEON_HASH_MULTIPLIER))>>16)

#ifndef minimemcpy
// copy less than 4 bytes
void minimemcpy(void * to,const void *fm,size_t size)
{
	//ASSERT( size < 4 );
	char * toc = (char *)to;
	const char * fmc = (const char *)fm;
	for(size_t i=0;i<size;i++) toc[i] = fmc[i];
}
#endif

CHAMELEON_FUNC size_t Chameleon_Encode_Simple(Chameleon * c,void * to,const void * from, size_t from_size)
{
	uint16 * to16 = (uint16 *)to;
		
	size_t from_size32 = from_size >> 2;
	const uint32 * fm32 = (const uint32 *)from;

	uint16 * toFlagsPtr = to16++;
	uint32 flags = 0;
	int flags_left = 16;
	
	uint32 * hash = c->hash;
	
	while( from_size32-- )
	{
		uint32 cur = *fm32++;
		
		uint32 h = CHAMELEON_HASH(cur);
		
		flags <<= 1;
		flags_left --;
		
		if ( hash[h] == cur )
		{
			flags ++;
			*to16++ = (uint16) h;
		}
		else
		{
			hash[h] = cur;
			*((uint32 *)to16) = cur;
			to16 += 2;
		}
		
		if ( flags_left == 0 )
		{
			*toFlagsPtr = (uint16)flags;
			
			toFlagsPtr = to16++;
			flags = 0;
			flags_left = 16;
		}	
	}
	
	// flush flags :
	if ( flags_left == 16 )
	{
		// undo step
		to16--;
		ASSERT( toFlagsPtr == to16 );
	}
	else
	{
		flags <<= flags_left;
		*toFlagsPtr = (uint16)flags;		
	}
	
	size_t tail_count = from_size&3;
	if ( tail_count > 0 )
		minimemcpy(to16,fm32,tail_count); 

	return ((char *)to16 - (char *)to) + tail_count;
}

CHAMELEON_FUNC size_t Chameleon_Encode(Chameleon * c,void * to,const void * from, size_t from_size)
{
	uint16 * to16 = (uint16 *)to;
		
	size_t from_size32 = from_size >> 2;
	const uint32 * fm32 = (const uint32 *)from;

	size_t from_size32_16 = from_size32 >> 4;

	uint32 cur,h;
	
	uint32 * hash = c->hash;
	
	while( from_size32_16-- )
	{
		uint16 * toFlagsPtr = to16++;
		uint32 flags = 0;
	
		/*
		UNROLL16( \
			cur = *fm32++; h = CHAMELEON_HASH(cur); flags <<= 1; \
			if ( hash[h] == cur ) { flags ++; *to16++ = (uint16) h; } \
			else { hash[h] = cur; *((uint32 *)to16) = cur; to16 += 2; } \
			);
			
		*/

		//#define YKXZ(h0,c0) flags <<= 1; if ( hash[h0] == c0 ) { flags ++; *to16++ = (uint16) h0; } else { hash[h0] = c0; *((uint32 *)to16) = c0; to16 += 2; }
		#define YKXZ(h0,c0) if ( hash[h0] == c0 ) { flags <<= 1; flags ++; *to16++ = (uint16) h0; } else { flags <<= 1; hash[h0] = c0; *((uint32 *)to16) = c0; to16 += 2; }
		
		uint32 c0,c1,c2,c3;
		uint32 h0,h1,h2,h3;
		UNROLL4( c0 = fm32[0]; c1 = fm32[1]; c2 = fm32[2]; c3 = fm32[3]; \
			h0 = CHAMELEON_HASH(c0); h1 = CHAMELEON_HASH(c1); h2 = CHAMELEON_HASH(c2); h3 = CHAMELEON_HASH(c3); \
			YKXZ(h0,c0); YKXZ(h1,c1); YKXZ(h2,c2); YKXZ(h3,c3); \
			fm32 += 4; \
			);					
		
		#undef YKXZ
		
		*toFlagsPtr = (uint16) flags;
	}

	size_t from_size32_tail = from_size32 & 15;
	
	if ( from_size32_tail > 0 )
	{
		uint16 * toFlagsPtr = to16++;
		uint16 flags = 0;
		int flags_left = 16;
			
		while( from_size32_tail-- )
		{
			cur = *fm32++;
			h = CHAMELEON_HASH(cur);
			
			flags <<= 1;
			flags_left --;
			
			if ( hash[h] == cur )
			{
				flags ++;
				*to16++ = (uint16) h;
			}
			else
			{
				hash[h] = cur;
				*((uint32 *)to16) = cur; to16 += 2;
			}
		}
		
		flags <<= flags_left;
		*toFlagsPtr = flags;
	}
		
	size_t tail_count = from_size&3;
	if ( tail_count > 0 )
		minimemcpy(to16,fm32,tail_count); 

	return ((char *)to16 - (char *)to) + tail_count;
}

CHAMELEON_FUNC void Chameleon_Decode_Simple(Chameleon * c,void * to,  size_t raw_size, const void * from)
{
	const uint16 * fm16 = (uint16 *)from;
		
	size_t raw_size32 = raw_size >> 2;
	uint32 * to32 = (uint32 *)to;

	uint32 flags = *fm16++;
	int flags_left = 16;
	uint32 cur;
	
	uint32 * hash = c->hash;
	
	while( raw_size32-- )
	{
		flags <<= 1;
		flags_left--;

		if ( flags & (1<<16) )
		{
			cur = hash[ *fm16++ ];
		}
		else
		{
			cur = *((const uint32 *)fm16); fm16 += 2;
			hash[ CHAMELEON_HASH(cur) ] = cur;
		}
		
		*to32++ = cur;

		if ( flags_left == 0 )
		{
			flags = *fm16++;
			flags_left = 16;
		}
	}
	
	// flush flags :
	if ( flags_left == 16 )
	{
		// undo step
		fm16--;
	}
	
	size_t tail_count = raw_size&3;
	if ( tail_count > 0 )
		minimemcpy(to32,fm16,tail_count); 
}


CHAMELEON_FUNC void Chameleon_Decode(Chameleon * c,void * to,  size_t raw_size, const void * from)
{
	const uint16 * fm16 = (uint16 *)from;
		
	size_t raw_size32 = raw_size >> 2;
	uint32 * to32 = (uint32 *)to;

	size_t raw_size32_16 = raw_size32 >> 4;
	size_t raw_size32_tail = raw_size32 & 15;
	
	uint32 cur;
		
	uint32 * hash = c->hash;
	
	while( raw_size32_16-- )
	{
		uint32 flags = *fm16++;
	
		// 1434.87
		//*
		UNROLL16( \
			if ( (int16)flags < 0 ) { cur = hash[ *fm16++ ]; } \
			else { cur = *((const uint32 *)fm16); fm16 += 2; hash[ CHAMELEON_HASH(cur) ] = cur; } \
			flags <<= 1;	*to32++ = cur; \
			);
		/*/
		
		#define YKXZ(i) \
			if ( (int16)flags < 0 ) { to32[i] = hash[ *fm16++ ]; flags <<= 1; } \
			else { uint32 cur = *((const uint32 *)fm16); fm16 += 2; hash[ CHAMELEON_HASH(cur) ] = cur; to32[i] = cur; flags <<= 1; }
		
		UNROLL4( YKXZ(0) YKXZ(1) YKXZ(2) YKXZ(3) to32 += 4; );
		
		#undef YKXZ
		
		/**/
	}
	
	if ( raw_size32_tail > 0 )
	{
		uint16 flags = *fm16++;
		
		while( raw_size32_tail-- )
		{
			if ( (int16)flags < 0 )
			{
				cur = hash[ *fm16++ ];
			}
			else
			{
				cur = *((const uint32 *)fm16); fm16 += 2;
				hash[ CHAMELEON_HASH(cur) ] = cur;
			}
		
			flags <<= 1;
			*to32++ = cur;
		}
	}
		
	size_t tail_count = raw_size&3;
	if ( tail_count > 0 )
		minimemcpy(to32,fm16,tail_count); 
}

#endif // CHAMELON_IMPL

#endif // _CB_CHAMELEON_H_
