// axf in lzbench (local experiment, #D1; not for upstream): the fast engine of aceapex (research/fastblk/axf.h)
// in its own small container: [u8 min_match][u32 block_size][u32 nblocks][u32 csize x nblocks][blocks].
// Levels: 1 = min match 16 greedy/lazy, 2 = 12, 3 = 8, 4 = 5 optimal parse (depth 512, lambda 2), 5 = 5 greedy/lazy,
// 6 = 16 optimal, 7 = 12 optimal, 8 = 8 optimal. One thread; every block but the last decodes in place.
#include "codecs.h"
#include "../lz/axf/axf.h"
#include <cstdint>
#include <cstring>
#include <vector>
static const uint32_t AXF_BS = 1u << 20;
static void axf_level(int lv, uint32_t& mm, int& el) {
    switch (lv) { case 1: mm = 16; el = 3; break; case 2: mm = 12; el = 3; break; case 3: mm = 8; el = 3; break;
        case 4: mm = 5; el = 4; break; case 6: mm = 16; el = 4; break; case 7: mm = 12; el = 4; break; case 8: mm = 8; el = 4; break;
        default: mm = 5; el = 3; }
}
int64_t lzbench_axf_compress(char* inbuf, size_t insize, char* outbuf, size_t outsize, codec_options_t* opts) {
    uint32_t mm; int el; axf_level((int)opts->level, mm, el); g_mm = mm; g_lambda = 2.0; g_depth4 = 512;
    const uint32_t nb = (uint32_t)((insize + AXF_BS - 1) / AXF_BS);
    size_t hdr = 9 + 4ull * nb; if (outsize < hdr) return -1;
    uint8_t* o = (uint8_t*)outbuf; o[0] = (uint8_t)mm; memcpy(o + 1, &AXF_BS, 4); memcpy(o + 5, &nb, 4);
    size_t pos = hdr; static thread_local Enc e; std::vector<uint8_t> tmp;
    for (uint32_t b = 0; b < nb; b++) {
        size_t off = (size_t)b * AXF_BS; uint32_t k = (uint32_t)std::min<size_t>(AXF_BS, insize - off);
        tmp.resize(enc_bound(k) + SLACK); size_t c = enc_block((const uint8_t*)inbuf + off, k, tmp.data(), el, e);
        if (pos + c > outsize) return -1;
        memcpy(o + pos, tmp.data(), c); uint32_t c32 = (uint32_t)c; memcpy(o + 9 + 4ull * b, &c32, 4); pos += c;
    }
    return (int64_t)pos;
}
int64_t lzbench_axf_decompress(char* inbuf, size_t insize, char* outbuf, size_t outsize, codec_options_t*) {
    const uint8_t* s = (const uint8_t*)inbuf; if (insize < 9) return -1;
    g_mm = s[0]; uint32_t bs, nb; memcpy(&bs, s + 1, 4); memcpy(&nb, s + 5, 4);
    size_t pos = 9 + 4ull * nb; if (pos > insize) return -1;
    static thread_local std::vector<uint8_t> priv;
    for (uint32_t b = 0; b < nb; b++) {
        uint32_t c; memcpy(&c, s + 9 + 4ull * b, 4); size_t off = (size_t)b * bs; uint32_t k = (uint32_t)std::min<size_t>(bs, outsize - off);
        if (pos + c > insize) return -1;
        if (b + 1 < nb) { if (dec_block(s + pos, c, (uint8_t*)outbuf + off, k) != k) return -1; }
        else { priv.resize(k + SLACK); if (dec_block(s + pos, c, priv.data(), k) != k) return -1; memcpy(outbuf + off, priv.data(), k); }
        pos += c;
    }
    return (int64_t)outsize;
}
// axf_safe: the same container through dec_block_safe - no padding on either side, every block decoded in place
// (the last one too), header fields checked against insize and outsize before any block is touched.
int64_t lzbench_axf_safe_decompress(char* inbuf, size_t insize, char* outbuf, size_t outsize, codec_options_t*) {
    const uint8_t* s = (const uint8_t*)inbuf; if (insize < 9) return -1;
    const uint32_t mm = s[0]; uint32_t bs, nb; memcpy(&bs, s + 1, 4); memcpy(&nb, s + 5, 4);
    if (mm < 1 || mm > 64 || bs == 0 || (uint64_t)nb != (outsize + bs - 1) / bs) return -1;
    size_t pos = 9 + 4ull * nb; if (pos > insize) return -1;
    g_mm = mm;
    for (uint32_t b = 0; b < nb; b++) {
        uint32_t c; memcpy(&c, s + 9 + 4ull * b, 4); size_t off = (size_t)b * bs; uint32_t k = (uint32_t)std::min<size_t>(bs, outsize - off);
        if (c > insize - pos) return -1;
        if (dec_block_safe(s + pos, c, (uint8_t*)outbuf + off, k) != k) return -1;
        pos += c;
    }
    return pos == insize ? (int64_t)outsize : -1;
}
