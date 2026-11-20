#ifndef BLARIS_TB_LEVEL
#define BLARIS_TB_LEVEL 7
#endif

#define BLARIS_LEVEL BLARIS_TB_LEVEL
#define BLARIS_COMPRESS

#define TB_BLARIS_CAT2(a, b) a##b
#define TB_BLARIS_CAT(a, b) TB_BLARIS_CAT2(a, b)

#define blaris_compress_bound TB_BLARIS_CAT(blaris_compress_bound_l, BLARIS_TB_LEVEL)
#define blaris_compress       TB_BLARIS_CAT(blaris_compress_l, BLARIS_TB_LEVEL)
#define blaris_init           TB_BLARIS_CAT(blaris_init_l, BLARIS_TB_LEVEL)
#define blaris_decompress     TB_BLARIS_CAT(blaris_decompress_l, BLARIS_TB_LEVEL)

#include "blaris.h"
#include "blaris-compress.c"
#include "blaris-decompress.c"

size_t TB_BLARIS_CAT(tb_blaris_compress_l, BLARIS_TB_LEVEL)(
    const uint8_t *in, size_t inlen, uint8_t *out, size_t outsize) {
    size_t outlen = outsize;
    return blaris_compress(in, inlen, out, &outlen) == 0 ? outlen : 0;
}

size_t TB_BLARIS_CAT(tb_blaris_decompress_l, BLARIS_TB_LEVEL)(
    const uint8_t *in, size_t inlen, uint8_t *out, size_t outlen) {
    struct Decoder dec;
    struct Checkpoint cp = {0};
    blaris_init((const uint32_t *)in, &dec);
    blaris_decompress(&dec, 0, out, (uint32_t)outlen, &cp);
    return inlen;
}
