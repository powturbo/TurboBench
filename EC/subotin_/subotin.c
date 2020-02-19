#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "../subotin/range.h"
#include "../subotin/range.c"

#define SYMS 320

int subenc(unsigned char *in, int inlen, unsigned char *out, int outsize, int symnum) { int i; 
	Uint ftbl[SYMS];  /* The frequency table */

	Sint sym;
	Uint n;
	rc_encoder rc;
	rc_model rcm;
	
	n = ModelInit (&rcm, symnum, ftbl, NULL, 4, (Uint)1<<16, ADAPT);
	if(n != RC_OK) {
	  fprintf(stderr, "Error: maxFreq or totFreq > 65536\n");
	  exit(1);
	}
	StartEncode (&rc, NULL, out, outsize);
	for (i = 0; i < inlen; i++) {
		EncodeSymbol (&rc, &rcm, in[i]);
		switch (rc.error) {
			case RC_OK:
				break;
			case RC_ERROR:
				fprintf(stderr, "Encode error: Symbol outside range\n");
				exit(1);
			case RC_IO_ERROR:
				fprintf(stderr, "Encode error: Could not output compressed data\n");
				exit(1);
		}
	}
	FinishEncode (&rc);
	return rc.passed;	
}

int subdec(unsigned char *in, int inlen, unsigned char *out, int outsize, int symnum) { int i; 
	Uint ftbl[SYMS];  /* The frequency table */
	Uint n;
	rc_decoder rd;
	rc_model rcm;
	
	n = ModelInit (&rcm, symnum, ftbl, NULL, 4, (Uint)1<<16, ADAPT);
	if (n != RC_OK) {
		fprintf(stderr, "Error: maxFreq or totFreq > 65536\n");
		exit(1);
	}
	StartDecode (&rd, NULL, in, outsize);
	n = 0; 
	for ( i = 0; i < inlen; i++) {
		out[i] = DecodeSymbol (&rd, &rcm);
		switch (rd.error) {
			case RC_OK:
				break;
			case RC_ERROR:
				fprintf(stderr, "Decode error: Symbol outside range\n");
				exit(1);
			case RC_IO_ERROR:
				fprintf(stderr, "Decode error: Could not input compressed data\n");
				exit(1);
		}
	}
	return n;
}
