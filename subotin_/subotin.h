#ifdef __cplusplus
extern "C" {
#endif

int subenc(unsigned char *in, int inlen, unsigned char *out, int outsize, int symnum);
int subdec(unsigned char *in, int inlen, unsigned char *out, int outsize, int symnum);
#ifdef __cplusplus
}
#endif


