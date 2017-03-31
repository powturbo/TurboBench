#ifndef RANS_STATIC4x8_H
#define RANS_STATIC4x8_H

unsigned int rans_compress_bound_4x8(unsigned int size, int order);
unsigned char *rans_compress_to_4x8(unsigned char *in,  unsigned int in_size,
				    unsigned char *out, unsigned int *out_size,
				    int order);
unsigned char *rans_compress_4x8(unsigned char *in, unsigned int in_size,
				 unsigned int *out_size, int order);
unsigned char *rans_uncompress_to_4x8(unsigned char *in,  unsigned int in_size,
				      unsigned char *out, unsigned int *out_size,
				      int order);
unsigned char *rans_uncompress_4x8(unsigned char *in, unsigned int in_size,
				   unsigned int *out_size, int order);

#endif /* RANS_STATIC4x8_H */
