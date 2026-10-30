// region_bits.cpp - where the bits of an archive go, block by block: every block's share of each compressed stream
// (a chunk's compressed bytes spread over the blocks whose raw slices it holds, in proportion to their raw bytes), its
// block-table entry (64 B), its match bytes (tokens parsed as the decoder parses them), and from the original: its
// lowercase (soft-masked) bases, uppercase bases, N and other bytes. One TSV row per block for the class tables of
// results/reality-2026-10-02.log (scripts/region_bits.py). Chunk tables and stream headers are reported once (overhead).
// Build: g++ -std=c++17 -O2 -Isrc scripts/region_bits.cpp src/aceapex_api.cpp -lzstd -lpthread
// --classes: byte-level instead - every output byte is a literal (the block's literal bytes per literal byte) or part of
// a match (the block's token bytes per match, spread over its bytes) and counts for its class: uppercase base, lowercase
// (soft-masked) base, N, other (line ends, headers); each in or out of a dense stretch (the 1 MiB of blocks around its
// block is at least half non-N and has >= 10 % of its non-N bytes in matches: satellite / centromeric arrays).
// Usage: region_bits <archive.aet> <original> [--classes] > rows.tsv
#include "aceapex.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
struct BO { uint64_t off[4], sz[4]; };
static uint32_t rv(const uint8_t* b, size_t& p, size_t n) {
    uint32_t v = 0, s = 0;
    while (p < n && s <= 28) { const uint8_t c = b[p++]; v |= (uint32_t)(c & 0x7F) << s; if (!(c & 0x80)) return v; s += 7; }
    return v;
}
static std::vector<uint8_t> slurp(const char* p) { std::vector<uint8_t> v; FILE* f = fopen(p, "rb"); if (!f) { perror(p); exit(1); }
    fseek(f, 0, SEEK_END); v.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(v.data(), 1, v.size(), f) != v.size()) exit(1); fclose(f); return v; }
static uint64_t rd64(const uint8_t* p) { uint64_t v; memcpy(&v, p, 8); return v; }
int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s <archive.aet> <original>\n", argv[0]); return 1; }
    const std::vector<uint8_t> a = slurp(argv[1]), o = slurp(argv[2]);
    aceapex_streams_t S; if (aceapex_decode_streams(a.data(), a.size(), &S)) { fprintf(stderr, "decode_streams failed\n"); return 2; }
    const uint64_t nb = S.num_blocks, bs = S.block_size; const BO* bo = (const BO*)S.boffs;
    const uint64_t zsz[4] = {rd64(&a[36]), rd64(&a[44]), rd64(&a[52]), rd64(&a[60])};
    uint64_t zo = 68 + 64 * nb; const uint64_t rawsz[4] = {S.lit_sz, S.off_sz, S.len_sz, S.cmd_sz};
    // chunks of every stream: raw [lo,hi) and compressed bytes
    struct Ch { uint64_t lo, hi, z; }; std::vector<Ch> ch[4]; uint64_t hdr[4] = {0, 0, 0, 0};
    for (int s = 0; s < 4; s++) { const uint8_t* z = &a[zo]; const uint64_t Z = zsz[s];
        if (s == 0 && Z >= 16 && ((rd64(z) >> 61) & 1)) {                              // chunked literal stream
            const uint64_t cz = rd64(z + 8), NW = (rawsz[0] + cz - 1) / cz; uint64_t sum = 0;
            for (uint64_t t = 0; t < NW; t++) { const uint64_t c = rd64(z + 16 + 8 * t); ch[0].push_back({t * cz, std::min(rawsz[0], (t + 1) * cz), c}); sum += c; }
            hdr[0] = Z - sum;
        } else if (s > 0 && Z >= 8) {
            const uint64_t w = rd64(z), osz = w & ((1ull << 48) - 1), CH = ((w >> 48) & 0x7fff) * 4096 ? ((w >> 48) & 0x7fff) * 4096 : 524288, nc = (osz + CH - 1) / CH; uint64_t sum = 0;
            for (uint64_t i = 0; i < nc; i++) { const uint64_t cs = rd64(z + 8 + 8 * i), raw = std::min(CH, osz - i * CH), c = (cs >> 63) ? raw : (cs & ((1ull << 48) - 1));
                ch[s].push_back({i * CH, i * CH + raw, c}); sum += c; }
            hdr[s] = Z - sum;
        } else { ch[s].push_back({0, rawsz[s], Z}); }                               // one piece (legacy literal layouts)
        zo += Z; }
    fprintf(stderr, "stream headers + chunk tables: lit %llu, off %llu, len %llu, cmd %llu B; archive header 68 B\n",
            (unsigned long long)hdr[0], (unsigned long long)hdr[1], (unsigned long long)hdr[2], (unsigned long long)hdr[3]);
    const bool classes = argc > 3 && !strcmp(argv[3], "--classes");
    if (!classes) printf("block\torig\tlower\tupper\tN\tother\tmatch_bytes\tmatches\tz_lit\tz_off\tz_len\tz_cmd\ttable\n");
    std::vector<double> mshare(nb, 0), mnon(nb, 0); std::vector<std::vector<uint8_t>> mark;     // classes: per block, match bytes; then dense
    double cz[8] = {0}, cn[8] = {0}, cm[8] = {0}; std::vector<uint8_t> ism;
    auto cls_of = [&](uint8_t c) { return c == 'N' || c == 'n' ? 2 : (c >= 'a' && c <= 'z') ? 1 : (c >= 'A' && c <= 'Z') ? 0 : 3; };
    for (int pass = classes ? 0 : 1; pass < 2; pass++) {
    std::vector<uint8_t> dense(nb, 0);
    if (classes && pass == 1) { std::vector<double> pre(nb + 1, 0), prn(nb + 1, 0); for (uint64_t b = 0; b < nb; b++) { pre[b + 1] = pre[b] + mshare[b]; prn[b + 1] = prn[b] + mnon[b]; }
        const uint64_t W = std::max<uint64_t>(1, ((uint64_t)1 << 20) / bs);   // the 1 MiB around the block
        for (uint64_t b = 0; b < nb; b++) { const uint64_t lo = b >= W / 2 ? b - W / 2 : 0, hi = std::min<uint64_t>(nb, lo + W);
            const double non = prn[hi] - prn[lo]; dense[b] = non >= 0.5 * (double)(hi - lo) * (double)bs && (pre[hi] - pre[lo]) >= 0.10 * non; } }
    for (int s = 0; s < 4; s++) { (void)s; }
    size_t ci_[4] = {0, 0, 0, 0}; size_t* ci = ci_;
    for (uint64_t b = 0; b < nb; b++) {
        const uint64_t base = b * bs, dsz = std::min(bs, S.orig_size - base);
        uint64_t lo = 0, up = 0, nn = 0, ot = 0;
        for (uint64_t i = base; i < base + dsz; i++) { const uint8_t c = o[i];
            if (c == 'N' || c == 'n') nn++; else if (c >= 'a' && c <= 'z') lo++; else if (c >= 'A' && c <= 'Z') up++; else ot++; }
        double zc[4] = {0, 0, 0, 0};
        for (int s = 0; s < 4; s++) { const uint64_t x0 = bo[b].off[s], x1 = x0 + bo[b].sz[s]; if (x1 == x0) continue;
            while (ci[s] < ch[s].size() && ch[s][ci[s]].hi <= x0) ci[s]++;
            for (size_t k = ci[s]; k < ch[s].size() && ch[s][k].lo < x1; k++) { const uint64_t ov = std::min(x1, ch[s][k].hi) - std::max(x0, ch[s][k].lo);
                zc[s] += (double)ch[s][k].z * (double)ov / (double)(ch[s][k].hi - ch[s][k].lo); } }
        // match bytes of the block
        const uint8_t *off = S.off + bo[b].off[1], *len = S.len + bo[b].off[2], *cmd = S.cmd + bo[b].off[3];
        size_t op = 0, np = 0, cp = 0; uint64_t out = 0, mb = 0, nm = 0; uint32_t rep[4] = {1, 2, 4, 8};
        if (classes) ism.assign(dsz, 0);
        while (out < dsz && cp < bo[b].sz[3]) { const uint8_t c = cmd[cp++];
            if (c == 0xFF) { rep[0] = 1; rep[1] = 2; rep[2] = 4; rep[3] = 8; continue; }
            if (c < 0x80) { out += c + 1u; continue; }
            uint32_t l, d;
            if ((c & 0xC0) == 0x80) { uint32_t ri = (c >> 4) & 3, lv = c & 0x0F; if (lv == 0x0F) lv += rv(len, np, bo[b].sz[2]);
                l = lv + 6; d = rep[ri]; if (ri) { for (int i = (int)ri; i > 0; i--) rep[i] = rep[i - 1]; rep[0] = d; } }
            else { const uint32_t lv = c == 0xFE ? rv(len, np, bo[b].sz[2]) : (uint32_t)(c & 0x3F); l = lv + 6; d = rv(off, op, bo[b].sz[1]);
                rep[3] = rep[2]; rep[2] = rep[1]; rep[1] = rep[0]; rep[0] = d; }
            (void)d; if (classes) memset(ism.data() + out, 1, std::min<uint64_t>(l, dsz - std::min(out, dsz))); out += l; mb += l; nm++; }
        if (classes) { if (pass == 0) { uint64_t mm = 0, nn2 = 0; for (uint64_t i = 0; i < dsz; i++) { const uint8_t c = o[base + i]; if (c == 'N' || c == 'n') continue; nn2++; mm += ism[i]; }
                mshare[b] = (double)mm; mnon[b] = (double)nn2; continue; }        // dense: match bytes among the non-N bytes
            const double lc = (dsz - mb) ? zc[0] / (double)(dsz - mb) : 0, tc = mb ? (zc[1] + zc[2] + zc[3]) / (double)mb : 0, tab = 64.0 / (double)dsz;
            for (uint64_t i = 0; i < dsz; i++) { const int k = cls_of(o[base + i]) + (dense[b] ? 4 : 0); cz[k] += (ism[i] ? tc : lc) + tab; cn[k] += 1; cm[k] += ism[i]; }
            continue; }
        printf("%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%.1f\t%.1f\t%.1f\t%.1f\t64\n", (unsigned long long)b, (unsigned long long)dsz, (unsigned long long)lo,
               (unsigned long long)up, (unsigned long long)nn, (unsigned long long)ot, (unsigned long long)mb, (unsigned long long)nm, zc[0], zc[1], zc[2], zc[3]);
    }
    }
    if (classes) { const char* nm[8] = {"upper", "lower (masked)", "N", "other", "dense: upper", "dense: lower", "dense: N", "dense: other"};
        double tn = 0, tz = 0; for (int k = 0; k < 8; k++) { tn += cn[k]; tz += cz[k]; }
        printf("class            bytes MB  bytes %%  archive MB  archive %%  bits/byte  in matches %%\n");
        for (int k = 0; k < 8; k++) if (cn[k]) printf("%-16s %8.1f  %7.2f  %10.2f  %9.2f  %9.3f  %12.2f\n", nm[k], cn[k] / 1e6, 100 * cn[k] / tn, cz[k] / 1e6, 100 * cz[k] / tz, 8 * cz[k] / cn[k], 100 * cm[k] / cn[k]);
        printf("%-16s %8.1f  %7.2f  %10.2f  %9.2f  %9.3f  (+ stream headers / chunk tables, once)\n", "total", tn / 1e6, 100.0, tz / 1e6, 100.0, 8 * tz / tn); }
    aceapex_streams_free(&S);
    return 0;
}
