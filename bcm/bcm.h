#ifdef __cplusplus
extern "C" {
#endif
int divbwt(const unsigned char *T, unsigned char *U, int *A, int n, unsigned char * num_indexes, int * indexes, int openMP);

unsigned bcmbwti(unsigned char *buf, unsigned n, unsigned p, unsigned char *out);
unsigned bcmcompress(  unsigned char *_in, int n,unsigned char *_out);
unsigned bcmdecompress(unsigned char *_in, int n,unsigned char *_out, int outlen);

unsigned bcmenc(unsigned char *_in, int n, unsigned char *_out);
unsigned bcmdec(unsigned char *_in, unsigned inlen, unsigned char *out, unsigned n);
#ifdef __cplusplus
}
#endif
