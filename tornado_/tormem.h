#ifndef TOR_H
#define TOR_H
#define TOR_VERSION "0.5"

unsigned torcompress(unsigned char method, unsigned char* inbuf, unsigned char* outbuf, unsigned size);
unsigned tordecompress(unsigned char* inbuf, unsigned char* outbuf, unsigned size, unsigned outsize);
int torhenc(unsigned char *in, int inlen, unsigned char *_out, int outsize);
int torhdec(unsigned char *in, int inlen, unsigned char *out, int outlen );

#endif 
