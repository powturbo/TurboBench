#ifdef __cplusplus
extern "C" {
#endif

unsigned balzcompress(  unsigned char *_in, int n,unsigned char *_out, int max);
unsigned balzdecompress(unsigned char *_in, int n,unsigned char *_out, int outlen);
#ifdef __cplusplus
}
#endif
