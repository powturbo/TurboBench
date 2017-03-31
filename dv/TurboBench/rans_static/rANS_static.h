#ifndef RANS_STATIC_H
#define RANS_STATIC_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned int rans_compress_bound(int renorm_size,
				 unsigned int size, int order);
unsigned char *rans_compress_to(int renorm_size,
				unsigned char *in,  unsigned int in_size,
				unsigned char *out, unsigned int *out_size,
				int order);
unsigned char *rans_compress(int renorm_size,
			     unsigned char *in, unsigned int in_size,
			     unsigned int *out_size, int order);
unsigned char *rans_uncompress_to(int renorm_size,
				  unsigned char *in,  unsigned int in_size,
				  unsigned char *out, unsigned int *out_size,
				  int order);
unsigned char *rans_uncompress(int renorm_size,
			       unsigned char *in, unsigned int in_size,
			       unsigned int *out_size, int order);

#ifdef __cplusplus
}
#endif

#endif /* RANS_STATIC_H */
