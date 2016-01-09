#ifdef __cplusplus
extern "C" {
#endif

int lzomapack(   unsigned char *in, int inlen, unsigned char *out, int lev);
int lzomaunpack( unsigned char *in, int inlen, unsigned char *out, int outlen);

#ifdef __cplusplus
}
#endif

