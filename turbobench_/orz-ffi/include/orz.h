/* SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * C bindings for the orz ROLZ data compressor
 * https://github.com/richox/orz
 *
 * Link with the static or shared library produced by:
 *   cargo build --release
 * (crate-type already includes staticlib + cdylib)
 */

#ifndef ORZ_H
#define ORZ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> /* size_t */
#include <stdint.h>

/* -------------------------------------------------------------------------
 * Error codes
 * ------------------------------------------------------------------------- */
typedef enum OrzError {
    ORZ_OK              =  0,
    ORZ_INVALID_LEVEL   = -1,
    ORZ_NULL_POINTER    = -2,
    ORZ_ENCODE_FAILED   = -3,
    ORZ_DECODE_FAILED   = -4,
    ORZ_BUFFER_TOO_SMALL= -5,
    ORZ_OUT_OF_MEMORY   = -6,
    ORZ_INVALID_INPUT   = -7
} OrzError;

/* -------------------------------------------------------------------------
 * Configuration
 * ------------------------------------------------------------------------- */
typedef struct OrzCfg {
    size_t match_depth;
    size_t lazy_match_depth1;
    size_t lazy_match_depth2;
} OrzCfg;

/**
 * Fill *out_cfg with the parameters that correspond to the classic CLI
 * levels 0, 1 or 2.  Returns ORZ_OK on success, ORZ_INVALID_LEVEL otherwise.
 */
int orz_cfg_from_level(int level, OrzCfg *out_cfg);

/* -------------------------------------------------------------------------
 * Allocated-buffer API
 * ------------------------------------------------------------------------- */
typedef struct OrzBuffer {
    uint8_t *data;      /* owned; free with orz_buffer_free() */
    size_t   len;       /* valid bytes */
    size_t   capacity;  /* allocated size (for the free helper) */
} OrzBuffer;

/**
 * Compress src[0..src_len) using the given level (0..2).
 * On success *out is filled with a newly allocated buffer that the caller
 * must release with orz_buffer_free().
 */
int orz_encode(const uint8_t *src, size_t src_len, int level, OrzBuffer *out);

/**
 * Same as orz_encode but takes an explicit configuration.
 */
int orz_encode_with_cfg(const uint8_t *src, size_t src_len,
                        const OrzCfg *cfg, OrzBuffer *out);

/**
 * Decompress src[0..src_len).  On success *out is filled and must be
 * released with orz_buffer_free().
 */
int orz_decode(const uint8_t *src, size_t src_len, OrzBuffer *out);

/**
 * Release a buffer previously returned by orz_encode / orz_decode.
 * Safe to call with a NULL pointer or an already-freed buffer.
 */
void orz_buffer_free(OrzBuffer *buf);

/**
 * Low-level free for a raw pointer + capacity pair (useful when the
 * caller has extracted the fields from an OrzBuffer).
 */
void orz_free(void *ptr, size_t capacity);

/* -------------------------------------------------------------------------
 * Caller-provided-buffer API
 * ------------------------------------------------------------------------- */

/**
 * Compress into a pre-allocated destination buffer.
 * *dst_len is an in/out parameter: on entry it holds the capacity of dst,
 * on successful return it holds the number of bytes written.
 * Returns ORZ_BUFFER_TOO_SMALL when the supplied buffer is not large enough.
 */
int orz_encode_into(const uint8_t *src, size_t src_len, int level,
                    uint8_t *dst, size_t *dst_len);

/**
 * Decompress into a pre-allocated destination buffer.
 * *dst_len is an in/out parameter (capacity in, written size out).
 * Returns ORZ_BUFFER_TOO_SMALL when the supplied buffer is not large enough.
 */
int orz_decode_into(const uint8_t *src, size_t src_len,
                    uint8_t *dst, size_t *dst_len);

#ifdef __cplusplus
}
#endif

#endif /* ORZ_H */
