#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define ABS(a)   ((a)>0?(a):-(a))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#include "clr.cdr"
#include "simple_model.h"
#include "f_o0.h"

#define SYM_EOF 256

//#define BLK_SIZE 0x100000
//static unsigned char in[BLK_SIZE], out[BLK_SIZE*2];

void compress_block(unsigned char *in,  unsigned int in_size,
			   unsigned char *out, unsigned int *out_size) {
    SIMPLE_MODEL<257> order0;
    RangeCoder rc;
    unsigned int i;

    rc.output((char *)out);
    rc.StartEncode();

    for (i = 0; i < in_size; i++)
	order0.encodeSymbol(&rc, in[i]);

    rc.FinishEncode();
    *out_size = rc.size_out();
}

void uncompress_block(unsigned char *in,
			     unsigned char *out, int out_size) {
    SIMPLE_MODEL<257> order0;
    RangeCoder rc;
    unsigned int i;

    rc.input((char *)in);
    rc.StartDecode();

    for (i = 0; i < out_size; i++)
	out[i] = order0.decodeSymbol(&rc);

    rc.FinishDecode();
}

 #if 0
int main(int argc, char **argv) {
    int c, sz;
    FILE *infp = stdin, *outfp = stdout;

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
	// decode
	for (;;) {
	    uint32_t in_size, out_size;

	    if (4 != fread(&in_size, 1, 4, infp))
		break;
	    if (4 != fread(&out_size, 1, 4, infp))
		break;
	    if (out_size != fread(out, 1, out_size, infp)) {
		fprintf(stderr, "Truncated input\n");
		exit(1);
	    }

	    uncompress_block(out, in, in_size);
	    fwrite(in, 1, in_size, outfp);
	}
    } else {
	// encode
	for (;;) {
	    uint32_t in_size, out_size;

	    in_size = fread(in, 1, BLK_SIZE, infp);
	    if (in_size <= 0)
		break;

	    compress_block(in, in_size, out, &out_size);
	    fwrite(&in_size,  1, 4, outfp);
	    fwrite(&out_size, 1, 4, outfp);
	    fwrite(out, 1, out_size, outfp);
	}
    }

    return 0;
}
  #endif
