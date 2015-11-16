#define SYMNUM 0x100

#ifdef __cplusplus
extern "C" {
#endif
int polarenc(unsigned char *in, int inlen, unsigned char *out);
int polardec(unsigned char *in, unsigned char *out, int outlen );
#ifdef __cplusplus
}
#endif
