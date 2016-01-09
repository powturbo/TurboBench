/* Carryless rangecoder (c) 1999 by Dmitry Subbotin      */
/* Modified into C and extended 2001 by Mikael Lundqvist */
/* Version 30 October 2001 */

#include <stdio.h>
#include <assert.h>
#include "range.h"

/* The two IO functions */
static
void OutByte (rc_encoder *rc, uc c) {
	if (rc->ptr) {
		if (rc->ptr < rc->eptr) {
			rc->passed++;
			*rc->ptr++ = c;
		}
		else
			rc->error = RC_IO_ERROR;
	}
	else if (rc->fp) {
		rc->passed++;
		if (putc(c, rc->fp) == EOF)
			rc->error = RC_IO_ERROR;
	}
	else
		rc->error = RC_IO_ERROR;
}

static
uc InByte (rc_decoder *rd) {
	int sym;
	
	if (rd->ptr) {
		if (rd->ptr < rd->eptr) {
			rd->passed++;
			return *rd->ptr++;
		}
		else
			rd->error = RC_IO_ERROR;
	}
	else if (rd->fp) {
		rd->passed++;
		sym = getc(rd->fp);
		if (sym == EOF)
			rd->error = RC_IO_ERROR;
		return sym;
	}
	else
		rd->error = RC_IO_ERROR;
	
	return 0;
}

/* Rangecoder start and end functions */
void StartEncode (rc_encoder *rc, FILE *F, uc *ptr, Uint length) {
	rc->fp = F;
	rc->passed = rc->low = 0;
	rc->range = (Uint) -1;
	rc->ptr = ptr;
	rc->eptr = ptr + length;
	rc->error = RC_OK;
}

void FinishEncode (rc_encoder *rc) {
	OutByte(rc, rc->low>>24), rc->low<<=8;
	OutByte(rc, rc->low>>24), rc->low<<=8;
	OutByte(rc, rc->low>>24), rc->low<<=8;
	OutByte(rc, rc->low>>24), rc->low<<=8;
}

void StartDecode (rc_decoder *rd, FILE *F, uc *ptr, Uint length) {
	rd->passed = rd->low = rd->code = 0;
	rd->range = (Uint) -1;
	rd->fp = F;
	rd->ptr = ptr;
	rd->eptr = ptr + length;
	rd->error = RC_OK;
	rd->code = rd->code<<8 | InByte(rd);
	rd->code = rd->code<<8 | InByte(rd);
	rd->code = rd->code<<8 | InByte(rd);
	rd->code = rd->code<<8 | InByte(rd);
}

/* Rangecoder frequency encoding and decoding */
static
void Encode (rc_encoder *rc, rc_model *rcm, Uint cumFreq, Uint sym) {
	/* assert(cumFreq+rcm->freq[sym] <= rcm->totFreq &&
			rcm->freq[sym] && rcm->totFreq <= rcm->maxFreq); */
	rc->low  += cumFreq * (rc->range /= rcm->totFreq);
	rc->range *= rcm->freq[sym];
	while ( (rc->low ^ (rc->low+rc->range)) < TOP || (rc->range < BOT &&
			((rc->range = -rc->low & (BOT-1)), 1)) )
	{	OutByte(rc, rc->low>>24);
		rc->range <<= 8;
		rc->low <<= 8;
	}
}

static
void Decode (rc_decoder *rd, rc_model *rdm, Uint cumFreq, Uint sym) {
	/* assert(cumFreq + rdm->freq[sym] <= rdm->totFreq &&
			rdm->freq[sym] && rdm->totFreq <= rdm->maxFreq); */
	rd->low  += cumFreq * rd->range;
	rd->range *= rdm->freq[sym];
	while ( (rd->low ^ (rd->low+rd->range)) < TOP || (rd->range < BOT &&
			((rd->range = -rd->low & (BOT-1)),1)) )
	{	rd->code = rd->code<<8 | InByte(rd);
		rd->range <<= 8;
		rd->low <<= 8;
	}
}

/* Symbol encode and decode */
Sint EncodeSymbol(rc_encoder *rc, rc_model *rcm, Uint sym)
{
	Uint i, cumFreq;
	
	if (sym >= rcm->nsym) {
		rc->error = RC_ERROR;
		return -1;
	}
	
	/* Calculate cumFreq choosing the shortest path possible */
	if (sym >= rcm->lastSym)
		if (rcm->nsym - sym > sym - rcm->lastSym) {
			for (i = rcm->lastSym, cumFreq = rcm->lastCumFreq; i < sym; cumFreq += rcm->freq[i++]) ;
			rcm->lastCumFreq = cumFreq;
			rcm->lastSym = sym;
		}
		else
			for (i = sym, cumFreq = rcm->totFreq; i < rcm->nsym; cumFreq -= rcm->freq[i++]) ;
	else
		if (rcm->lastSym >= sym + sym) {
			for (cumFreq = i = 0; i < sym; cumFreq += rcm->freq[i++]) ;
			if (rcm->adapt) rcm->lastCumFreq += rcm->incr;
		}
		else {
			for (i = sym, cumFreq = rcm->lastCumFreq; i < rcm->lastSym; cumFreq -= rcm->freq[i++]) ;
			rcm->lastCumFreq = cumFreq;
			rcm->lastSym = sym;
		}
	
	Encode(rc, rcm, cumFreq, sym);
	
	/* Update statistics */
	if (rcm->adapt) {
		rcm->freq[sym] += rcm->incr;
		rcm->totFreq += rcm->incr;
		if (rcm->totFreq > rcm->maxFreq) {
			for (i = rcm->totFreq = 0; i < rcm->nsym; i++)
				rcm->totFreq += (rcm->freq[i] -= (rcm->freq[i] >> 1));
			rcm->lastCumFreq = 0;
			rcm->lastSym = 0;
		}
	}
	
	return 0;
}

Sint DecodeSymbol(rc_decoder *rd, rc_model *rdm)
{
	Uint i, sym, cumFreq, rfreq;
	
	rfreq = (rd->code - rd->low) / (rd->range /= rdm->totFreq); /* GetFreq */
	if (rfreq >= rdm->totFreq) {
		rd->error = RC_ERROR;
		return -1;
	}
	
	/* Find symbol choosing the shortest possible path */
	if (rdm->lastCumFreq > rfreq)
		if (rfreq > rdm->halfFreq || (rdm->lastSym <= rdm->nsym23 && rfreq >= rdm->firstFreq)) {
			for (sym = rdm->lastSym-1, cumFreq = rdm->lastCumFreq-rdm->freq[sym];;) {
				if (cumFreq <= rfreq) break;
				cumFreq -= rdm->freq[--sym];
			}
			rdm->lastCumFreq = cumFreq;
			rdm->lastSym = sym;
		}
		else { /* if rfreq < halfFreq && (lastSym > nsym23 || rfreq < firstFreq) */
			for (sym = 0, cumFreq = rdm->freq[0];;) {
				if (cumFreq > rfreq) break;
				cumFreq += rdm->freq[++sym];
			}
			cumFreq -= rdm->freq[sym];
			if (rdm->adapt) rdm->lastCumFreq += rdm->incr;
		}
	else /* lastCumFreq <= rfreq */
		if (rfreq < rdm->halfFreq || (rdm->lastSym >= rdm->nsym3 && rfreq <= rdm->lastFreq)) {
			for (sym = rdm->lastSym, cumFreq = rdm->lastCumFreq + rdm->freq[sym];;) {
				if (cumFreq > rfreq) break;
				cumFreq += rdm->freq[++sym];
			}
			cumFreq -= rdm->freq[sym];
			rdm->lastCumFreq = cumFreq;
			rdm->lastSym = sym;
		}
		else /* if rfreq > halfFreq && (rdm->lastSym < rdm->nsym3 || rfreq > lastFreq) */
			for (sym = rdm->nsym-1, cumFreq = rdm->totFreq-rdm->freq[sym];;) {
				if (cumFreq <= rfreq) break;
				cumFreq -= rdm->freq[--sym];
			}
	
	Decode(rd, rdm, cumFreq, sym);
	
	/* Update statistics */
	if (rdm->adapt) {
		rdm->freq[sym] += rdm->incr;
		if (sym < rdm->nsym23) {
			rdm->lastFreq += rdm->incr;
			if (sym < rdm->nsym2) {
				rdm->halfFreq += rdm->incr;
				if (sym < rdm->nsym4)
					rdm->firstFreq += rdm->incr;
			}
		}
		rdm->totFreq += rdm->incr;
		if (rdm->totFreq > rdm->maxFreq) {
			for (i = rdm->totFreq = 0; i < rdm->nsym; i++)
				rdm->totFreq += (rdm->freq[i] -= (rdm->freq[i] >> 1));
			rdm->halfFreq /= 2;
			rdm->firstFreq /= 2;
			rdm->lastFreq /= 2;
			rdm->lastCumFreq = 0;
			rdm->lastSym = 0;
		}
	}
	
	return sym;
}

/* Initializing the model */
Uint ModelInit(rc_model *rcm, Uint nsym, Uint *ftbl, Uint *ifreq,
				Uint incr, Uint maxFreq, uc adapt) {
	Uint i;
	
	rcm->nsym = nsym;  /* Nr of symbols */
	/* nsym2 - nsym23 are just constants designed to speed things up */
	rcm->nsym2 = nsym/2;
	rcm->nsym3 = nsym/3;
	rcm->nsym4 = nsym*5/22;  /* For firstFreq */
	rcm->nsym23 = nsym*2/3;  /* For lastFreq */
	
	rcm->freq = ftbl;
	rcm->totFreq = 0;
	if (ifreq != NULL)
		for (i = 0; i < nsym; i++)
			rcm->totFreq += (ftbl[i] = ifreq[i]);
	else
		for (i = 0; i < nsym; i++)
			rcm->totFreq += (ftbl[i] = 1);
	
	/* Sums to help DecodeSymbol make better decisions */
	for (rcm->firstFreq = i = 0; i < rcm->nsym4; i++)
		rcm->firstFreq += rcm->freq[i];
	for (rcm->halfFreq = rcm->firstFreq, i = rcm->nsym4; i < rcm->nsym2; i++)
		rcm->halfFreq += rcm->freq[i];
	for (rcm->lastFreq = rcm->halfFreq, i = rcm->nsym2; i < rcm->nsym23; i++)
		rcm->lastFreq += rcm->freq[i];
	
	rcm->incr = incr;
	rcm->maxFreq = maxFreq;
	rcm->adapt = adapt;
	rcm->lastCumFreq = 0;
	rcm->lastSym = 0;
	
	/* Total frequency MUST be kept below 1 << 16 (= 65536) */
	if (rcm->maxFreq > BOT || rcm->totFreq > BOT)
		return RC_ERROR;
		
	return RC_OK;
}
