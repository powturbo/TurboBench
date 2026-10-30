/* aceapex_decode.h - standalone C99 decoder for ACEPX2 archives (.aet).
 *
 * One translation unit, one dependency (libzstd), no threads, no globals. Meant for
 * embedding: a database, a bioinformatics tool or a language binding links this file
 * and reads regions of an archive without the ACEAPEX CLI or its C++ library.
 * The functions mirror aceapex.h (same names, same error codes) so a caller can swap
 * between this file and the full library. Encoding is not provided here.
 */
#ifndef ACEAPEX_DECODE_H
#define ACEAPEX_DECODE_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

#define ACEAPEX_DECODE_VERSION "2.5.0"   /* tracks ACEAPEX_VERSION_STRING in src/aceapex.h */

#define ACEAPEX_OK           0
#define ACEAPEX_ERR_BUFFER  -1
#define ACEAPEX_ERR_DATA    -2
#define ACEAPEX_ERR_MEMORY  -3

/* Size of the original input recorded in the archive, or a negative error code. */
int64_t aceapex_decoded_size(const void* src, size_t src_size);

/* Whole archive -> dst. Returns bytes written or a negative error code. */
int64_t aceapex_decompress(const void* src, size_t src_size, void* dst, size_t dst_capacity);

/* Bytes [offset, offset+length) of the original input, touching only the blocks that
 * cover the range and, in each stream, only the compressed chunks those blocks use. */
int64_t aceapex_decompress_region(const void* src, size_t src_size, void* dst,
                                  size_t dst_capacity, uint64_t offset, uint64_t length);

/* Many ranges from one archive: each distinct block is decoded once and sliced.
 * 'written' receives the byte count per entry, or a negative code for that entry.
 * Returns the number of entries that succeeded, or a negative code for a bad archive. */
typedef struct {
    uint64_t offset;
    uint64_t length;
    void*    dst;
    int64_t  written;
} aceapex_range_t;
int64_t aceapex_decompress_ranges(const void* src, size_t src_size,
                                  aceapex_range_t* ranges, size_t count, int threads);

/* Persistent decoder: parses the archive once and keeps the per-stream chunk tables and
 * the last decoded window of each stream between calls, so consecutive region reads that
 * touch the same chunks do not decode them twice. The archive bytes must stay valid and
 * unchanged for the life of the handle. A handle is not thread-safe; open one per thread. */
typedef struct aceapex_dec aceapex_dec_t;
aceapex_dec_t* aceapex_dec_open(const void* src, size_t src_size);   /* NULL on a bad archive or OOM */
int64_t        aceapex_dec_size(const aceapex_dec_t* d);
int64_t        aceapex_dec_region(aceapex_dec_t* d, void* dst, size_t dst_capacity, uint64_t offset, uint64_t length);
int64_t        aceapex_dec_ranges(aceapex_dec_t* d, aceapex_range_t* ranges, size_t count);
void           aceapex_dec_close(aceapex_dec_t* d);

#ifdef __cplusplus
}
#endif
#endif
