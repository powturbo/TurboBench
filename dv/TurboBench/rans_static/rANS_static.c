#include <stdio.h>
#include "rANS_static.h"
#include "rANS_static4x8.h"
#include "rANS_static4x16.h"

unsigned int rans_compress_bound(int renorm_size, unsigned int size, int order) {
    return renorm_size == 8
	? rans_compress_bound_4x8(size, order)
	: rans_compress_bound_4x16(size, order);
}

unsigned char *rans_compress_to(int renorm_size,
				unsigned char *in,  unsigned int in_size,
				unsigned char *out, unsigned int *out_size,
				int order) {
    return renorm_size == 8
	? rans_compress_to_4x8(in, in_size, out, out_size, order)
	: rans_compress_to_4x16(in, in_size, out, out_size, order);
}

unsigned char *rans_compress(int renorm_size,
			     unsigned char *in, unsigned int in_size,
			     unsigned int *out_size, int order) {
    return renorm_size == 8
	? rans_compress_to_4x8(in, in_size, NULL, out_size, order)
	: rans_compress_to_4x16(in, in_size, NULL, out_size, order);
}

unsigned char *rans_uncompress_to(int renorm_size,
				  unsigned char *in,  unsigned int in_size,
				  unsigned char *out, unsigned int *out_size,
				  int order) {
    return renorm_size == 8
	? rans_uncompress_to_4x8(in, in_size, out, out_size, order)
	: rans_uncompress_to_4x16(in, in_size, out, out_size, order);
}

unsigned char *rans_uncompress(int renorm_size,
			       unsigned char *in, unsigned int in_size,
			       unsigned int *out_size, int order) {
    return renorm_size == 8
	? rans_uncompress_to_4x8(in, in_size, NULL, out_size, order)
	: rans_uncompress_to_4x16(in, in_size, NULL, out_size, order);
}
