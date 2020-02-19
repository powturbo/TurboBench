/* Version 30 October 2001 */

#ifndef RANGE_HEADER
#define RANGE_HEADER

#include <limits.h>

/* Static or adaptive model? */
#define  STATIC    0U
#define  ADAPT     1U

/* Error codes */
#define RC_OK        0U
#define RC_ERROR     1U
#define RC_IO_ERROR  2U

#if UINT_MAX >= 0xffffffffU
	typedef   signed int  Sint;
	typedef unsigned int  Uint;
#else
	typedef   signed long Sint;
	typedef unsigned long Uint;
#endif

typedef unsigned char uc;

#define  TOP       ((Uint)1 << 24)
#define  BOT       ((Uint)1 << 16)

typedef struct {
	Uint  low, range, passed, error;
	FILE *fp;
	uc   *ptr, *eptr;
} rc_encoder;

typedef struct {
	Uint  low, code, range, passed, error;
	FILE *fp;
	uc   *ptr, *eptr;
} rc_decoder;

typedef struct {
	Uint *freq, totFreq, incr, maxFreq, adapt, halfFreq, firstFreq, lastFreq;
	Uint lastSym, lastCumFreq;
	Uint nsym, nsym2, nsym3, nsym4, nsym23;
} rc_model;


void StartEncode  (rc_encoder *, FILE *, uc *, Uint);
void FinishEncode (rc_encoder *);
void StartDecode  (rc_decoder *, FILE *, uc *, Uint);
Sint EncodeSymbol (rc_encoder *, rc_model *, Uint);
Sint DecodeSymbol (rc_decoder *, rc_model *);
Uint ModelInit    (rc_model *, Uint, Uint *, Uint *, Uint, Uint, uc);


#endif

