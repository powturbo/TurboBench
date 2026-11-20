#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TB_BLARIS_DECL(L) \
    size_t tb_blaris_compress_l##L(const uint8_t *in, size_t inlen, uint8_t *out, size_t outsize); \
    size_t tb_blaris_decompress_l##L(const uint8_t *in, size_t inlen, uint8_t *out, size_t outlen);

TB_BLARIS_DECL(1)
TB_BLARIS_DECL(2)
TB_BLARIS_DECL(3)
TB_BLARIS_DECL(4)
TB_BLARIS_DECL(5)
TB_BLARIS_DECL(6)
TB_BLARIS_DECL(7)
TB_BLARIS_DECL(8)
TB_BLARIS_DECL(9)

static inline size_t tb_blaris_compress(int lev, const uint8_t *in, size_t inlen, uint8_t *out, size_t outsize) {
    switch (lev) {
        case 1: return tb_blaris_compress_l1(in, inlen, out, outsize);
        case 2: return tb_blaris_compress_l2(in, inlen, out, outsize);
        case 3: return tb_blaris_compress_l3(in, inlen, out, outsize);
        case 4: return tb_blaris_compress_l4(in, inlen, out, outsize);
        case 5: return tb_blaris_compress_l5(in, inlen, out, outsize);
        case 6: return tb_blaris_compress_l6(in, inlen, out, outsize);
        case 8: return tb_blaris_compress_l8(in, inlen, out, outsize);
        case 9: return tb_blaris_compress_l9(in, inlen, out, outsize);
        case 7:
        default: return tb_blaris_compress_l7(in, inlen, out, outsize);
    }
}

static inline size_t tb_blaris_decompress(int lev, const uint8_t *in, size_t inlen, uint8_t *out, size_t outlen) {
    switch (lev) {
        case 1: return tb_blaris_decompress_l1(in, inlen, out, outlen);
        case 2: return tb_blaris_decompress_l2(in, inlen, out, outlen);
        case 3: return tb_blaris_decompress_l3(in, inlen, out, outlen);
        case 4: return tb_blaris_decompress_l4(in, inlen, out, outlen);
        case 5: return tb_blaris_decompress_l5(in, inlen, out, outlen);
        case 6: return tb_blaris_decompress_l6(in, inlen, out, outlen);
        case 8: return tb_blaris_decompress_l8(in, inlen, out, outlen);
        case 9: return tb_blaris_decompress_l9(in, inlen, out, outlen);
        case 7:
        default: return tb_blaris_decompress_l7(in, inlen, out, outlen);
    }
}

#ifdef __cplusplus
}
#endif
