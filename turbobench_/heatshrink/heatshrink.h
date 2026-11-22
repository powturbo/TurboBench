#include "../../heatshrink/heatshrink_common.h"

#ifdef __cplusplus
extern "C" {
#endif
int hscompress(   unsigned char *in, unsigned inlen, unsigned char *out, unsigned outsize, unsigned lev);
int hsdecompress( unsigned char *in, unsigned inlen, unsigned char *out, unsigned outlen);
#ifdef __cplusplus
}
#endif
