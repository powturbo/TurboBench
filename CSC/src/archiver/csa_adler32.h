#ifndef _CSA_ADLER32_H_
#define _CSA_ADLER32_H_
#include <stdint.h>

uint32_t adler32(uint32_t adler, const uint8_t *buf, uint64_t len);

#endif

