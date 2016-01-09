#include "../heatshrink/heatshrink_common.h"

#ifdef __cplusplus
extern "C" {
#endif

int hscompress(   unsigned char *in, int inlen, unsigned char *out);
int hsdecompress( unsigned char *in, int inlen, unsigned char *out, int outlen);
#ifdef __cplusplus
}
#endif
