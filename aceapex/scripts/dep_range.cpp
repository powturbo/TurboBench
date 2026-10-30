// dep_range.cpp - how far back the matches of an archive reach: every token of every block parsed as the decoder parses
// it (src/aceapex_main.cpp decompress_streams), each match's source position compared with its destination - the block
// distance (dst_block - src_block) and the byte distance - and the share of the output bytes that come from matches.
// A match whose source lies before its own block would make the decoders stop (dist > out); the count of such matches is
// printed too (it must be 0: blocks are independent, docs/FORMAT_ACEPX2.md s1.3).
// Build: g++ -std=c++17 -O2 -Isrc scripts/dep_range.cpp src/aceapex_api.cpp -lzstd -lpthread
// Usage: dep_range <archive.aet> [name]      Output: one DEPROW line (tab separated) + a readable line
#include "aceapex.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
struct BO { uint64_t lit_off, off_off, len_off, cmd_off, lit_sz, off_sz, len_sz, cmd_sz; };
static uint32_t rv(const uint8_t* b, size_t& p, size_t n) {          // read_varint: <= 5 bytes, stops at the end
    uint32_t v = 0, s = 0;
    while (p < n && s <= 28) { const uint8_t c = b[p++]; v |= (uint32_t)(c & 0x7F) << s; if (!(c & 0x80)) return v; s += 7; }
    return v;
}
int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <archive.aet> [name]\n", argv[0]); return 1; }
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; }
    std::vector<uint8_t> a; fseek(f, 0, SEEK_END); a.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET);
    if (fread(a.data(), 1, a.size(), f) != a.size()) return 1;
    fclose(f);
    aceapex_streams_t S; if (aceapex_decode_streams(a.data(), a.size(), &S)) { printf("decode_streams failed\n"); return 2; }
    const BO* bo = (const BO*)S.boffs; const uint64_t bs = S.block_size;
    // block distance buckets: 0 (own block), 1, 2..8, 9..64, > 64; byte distance buckets (match bytes)
    static const uint64_t DB[] = {64, 1024, 4096, 16384, 131072, 1 << 20, ~0ull}; const int ND = 7;
    uint64_t blk[5] = {0}, byt[ND] = {0}, mbytes = 0, lbytes = 0, out_total = 0, matches = 0, outside = 0;
    for (uint64_t b = 0; b < S.num_blocks; b++) {
        const uint64_t base = b * bs, dsz = S.orig_size - base < bs ? S.orig_size - base : bs;
        const uint8_t *off = S.off + bo[b].off_off, *len = S.len + bo[b].len_off, *cmd = S.cmd + bo[b].cmd_off;
        size_t op = 0, np = 0, cp = 0; uint64_t out = 0, lp = 0; uint32_t rep[4] = {1, 2, 4, 8};
        while (out < dsz && cp < bo[b].cmd_sz) {
            const uint8_t c = cmd[cp++];
            if (c == 0xFF) { rep[0] = 1; rep[1] = 2; rep[2] = 4; rep[3] = 8; continue; }
            if (c < 0x80) { const uint32_t l = c + 1u; out += l; lp += l; lbytes += l; continue; }
            uint32_t l, d;
            if ((c & 0xC0) == 0x80) { uint32_t ri = (c >> 4) & 3, lv = c & 0x0F; if (lv == 0x0F) lv += rv(len, np, bo[b].len_sz);
                l = lv + 6; d = rep[ri]; if (ri) { for (int i = (int)ri; i > 0; i--) rep[i] = rep[i - 1]; rep[0] = d; } }
            else { const uint32_t lv = c == 0xFE ? rv(len, np, bo[b].len_sz) : (uint32_t)(c & 0x3F); l = lv + 6; d = rv(off, op, bo[b].off_sz);
                rep[3] = rep[2]; rep[2] = rep[1]; rep[1] = rep[0]; rep[0] = d; }
            matches++; mbytes += l;
            if (d > out) { outside++; break; }                     // before the block: the decoders stop here
            const uint64_t src = base + out - d, sb = src / bs, db = b - sb;
            blk[db == 0 ? 0 : db == 1 ? 1 : db <= 8 ? 2 : db <= 64 ? 3 : 4] += l;
            int k = 0; while (k < ND - 1 && d > DB[k]) k++; byt[k] += l;
            out += l;
        }
        out_total += dsz;
    }
    const char* nm = argc > 2 ? argv[2] : argv[1];
    auto pc = [&](uint64_t x) { return mbytes ? 100.0 * (double)x / (double)mbytes : 0.0; };
    printf("%s: block %llu B, %llu blocks; output %llu B: matches %.2f %% of the bytes (%llu matches), literals %.2f %%; match bytes by block "
           "distance: own block %.2f %%, 1 block %.2f %%, 2-8 %.2f %%, 9-64 %.2f %%, > 64 %.2f %%; matches reaching before their block: %llu; "
           "by byte distance: <=64 %.1f %%, <=1K %.1f %%, <=4K %.1f %%, <=16K %.1f %%, <=128K %.1f %%, <=1M %.1f %%, more %.1f %%\n",
           nm, (unsigned long long)bs, (unsigned long long)S.num_blocks, (unsigned long long)out_total, 100.0 * mbytes / (double)out_total,
           (unsigned long long)matches, 100.0 * lbytes / (double)out_total, pc(blk[0]), pc(blk[1]), pc(blk[2]), pc(blk[3]), pc(blk[4]),
           (unsigned long long)outside, pc(byt[0]), pc(byt[1]), pc(byt[2]), pc(byt[3]), pc(byt[4]), pc(byt[5]), pc(byt[6]));
    printf("DEPROW\t%s\t%llu\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%llu\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n", nm, (unsigned long long)bs,
           100.0 * mbytes / (double)out_total, pc(blk[0]), pc(blk[1]), pc(blk[2]), pc(blk[3]), pc(blk[4]), (unsigned long long)outside,
           pc(byt[0]), pc(byt[1]), pc(byt[2]), pc(byt[3]), pc(byt[4]), pc(byt[5]), pc(byt[6]));
    aceapex_streams_free(&S);
    return outside ? 3 : 0;
}
