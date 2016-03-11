#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
//namespace crush
//{
	uint32_t crush_compress(int level, uint8_t* buf, int size, uint8_t* outbuf);
	uint32_t crush_decompress(uint8_t* inbuf, uint8_t* outbuf, int outsize);
//}
#ifdef __cplusplus
}
#endif

