#define FILET unsigned char

#ifdef __cplusplus
extern "C" {
#endif
int rcshc(unsigned char *f, int f_len, FILET *g);
int rcshd(FILET *f, unsigned char *g, int f_len);
#ifdef __cplusplus
}
#endif
