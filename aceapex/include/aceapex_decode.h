/*
 * ACEAPEX Decoder — single-header decoder
 * Requires: libzstd, libpthread
 * Usage:
 *   #include "aceapex_decode.h"
 *   int ok = aceapex_decompress(in_buf, in_size, out_buf, out_size, threads);
 */
#pragma once
#define ACEAPEX_NO_MAIN
#include "../src/aceapex_main.cpp"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Decompress ACEPX2 format.
 * Returns 1 on success, 0 on error.
 * out_buf must be pre-allocated to at least aceapex_decompress_bound(in_buf, in_size) bytes.
 */
static inline int aceapex_decompress(
    const void* in_buf, size_t in_size,
    void* out_buf, size_t out_size,
    int threads)
{
    const uint8_t* p = (const uint8_t*)in_buf;
    AetHeader hdr; memcpy(&hdr, p, sizeof(hdr));
    if (memcmp(hdr.magic, "ACEPX2\0\0", 8) != 0) return 0;
    if (hdr.orig_size > out_size) return 0;
    p += sizeof(hdr);
    std::vector<BlockOffsets> boffs(hdr.num_blocks);
    memcpy(boffs.data(), p, hdr.num_blocks * sizeof(BlockOffsets));
    p += hdr.num_blocks * sizeof(BlockOffsets);
    uint8_t* zl=(uint8_t*)malloc(hdr.zlit_sz); memcpy(zl,p,hdr.zlit_sz); p+=hdr.zlit_sz;
    uint8_t* zo=(uint8_t*)malloc(hdr.zoff_sz); memcpy(zo,p,hdr.zoff_sz); p+=hdr.zoff_sz;
    uint8_t* zn=(uint8_t*)malloc(hdr.zlen_sz); memcpy(zn,p,hdr.zlen_sz); p+=hdr.zlen_sz;
    uint8_t* zc=(uint8_t*)malloc(hdr.zcmd_sz); memcpy(zc,p,hdr.zcmd_sz);
    size_t os=*(uint64_t*)zo, ns=*(uint64_t*)zn, cs=*(uint64_t*)zc;
    size_t ls=0; uint8_t* l=lit_decompress(zl,hdr.zlit_sz,ls);
    uint8_t* o=(uint8_t*)malloc(os); fse_chunked_decomp(zo,os,o);
    uint8_t* n=(uint8_t*)malloc(ns); fse_chunked_decomp(zn,ns,n);
    uint8_t* c=(uint8_t*)malloc(cs); fse_chunked_decomp(zc,cs,c);
    free(zl);free(zo);free(zn);free(zc);
    parallel_decode(l,o,n,c,boffs.data(),hdr.num_blocks,
                    (uint8_t*)out_buf,hdr.orig_size,hdr.block_size,threads);
    free(l);free(o);free(n);free(c);
    return 1;
}

/*
 * Returns required output buffer size.
 * Returns 0 if not a valid ACEPX2 file.
 */
static inline size_t aceapex_decompress_bound(const void* in_buf, size_t in_size) {
    if (in_size < sizeof(AetHeader)) return 0;
    AetHeader hdr; memcpy(&hdr, in_buf, sizeof(hdr));
    if (memcmp(hdr.magic, "ACEPX2\0\0", 8) != 0) return 0;
    return (size_t)hdr.orig_size;
}

#ifdef __cplusplus
}
#endif
