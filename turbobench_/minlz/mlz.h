/* C bindings for github.com/minio/minlz (raw MinLZ block format). */
#ifndef MLZ_H
#define MLZ_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compression levels (match minlz's constants). */
#define MLZ_LEVEL_SUPERFAST    (-1)
#define MLZ_LEVEL_UNCOMPRESSED 0
#define MLZ_LEVEL_FASTEST      1
#define MLZ_LEVEL_BALANCED     2
#define MLZ_LEVEL_SMALLEST     3

/* Compress in[0..inlen) into out (capacity outsize).
 * inlen must be > 0 and <= 8 MiB (minlz MaxBlockSize).
 * in and out must not overlap.
 * Returns the compressed length, or 0 on error (incl. outsize too small). */
size_t mlzcompress(uint8_t *in, size_t inlen, uint8_t *out, size_t outsize);
size_t mlzcompresslevel(uint8_t *in, size_t inlen, uint8_t *out, size_t outsize, int level);

/* Decompress in[0..inlen) into out (capacity outlen).
 * in and out must not overlap.
 * Returns the number of decompressed bytes (== the original inlen on a clean
 * round-trip), or 0 on error. S2/Snappy blocks are accepted too (automatic
 * fallback in the decoder). */
size_t mlzdecompress(uint8_t *in, size_t inlen, uint8_t *out, size_t outlen);

/* Size helpers: 0 means "invalid / too large".
 *   mlzdeclen         - exact uncompressed size of a compressed block
 *   mlzmaxencodedlen  - worst-case compressed size for mlzcompress's outsize */
size_t mlzdeclen(uint8_t *in, size_t inlen);
size_t mlzmaxencodedlen(size_t inlen);

#ifdef __cplusplus
}
#endif
#endif
