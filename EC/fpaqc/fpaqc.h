#define FILET unsigned char

#ifdef __cplusplus
extern "C" {
#endif
int absc(unsigned char *f, int f_len, FILET *g, unsigned osize);
int absd(FILET *f, int f_len, unsigned char *g);
#ifdef __cplusplus
}
#endif
