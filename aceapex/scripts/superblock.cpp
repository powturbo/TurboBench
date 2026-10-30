// superblock.cpp - prototype of a compact block table (measurement; ACEPX2 unchanged): the 64-byte entries of an
// archive (4 stream offsets + 4 sizes per block) rewritten as superblocks of S blocks - 4 x u64 offsets of the first
// block, then 4 sizes per block as u16 (u32 when a size does not fit) - and as the 4 size columns varint + zstd -19.
// The slices are back to back in every stream (offset = previous offset + size; checked), so the offsets follow from
// the sizes. Checks: the table rebuilt from each form == the original bytes (so the archive rebuilt == the archive), and
// 10^7 random lookups (block -> its 8 fields) of the superblock form == the original; lookup time per call.
// Build: g++ -std=c++17 -O3 -march=native scripts/superblock.cpp -lzstd -o superblock
// Usage: superblock <archive.aet> [S=64]      Last line: SBROW <tab> archive blocks table superblock zstd_columns ...
#include <zstd.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <vector>
struct E { uint64_t off[4], sz[4]; };
static void put_v(std::vector<uint8_t>& o, uint64_t x) { while (x >= 0x80) { o.push_back((uint8_t)(x | 0x80)); x >>= 7; } o.push_back((uint8_t)x); }
static uint64_t get_v(const uint8_t*& p) { uint64_t v = 0; int s = 0; for (;;) { const uint8_t c = *p++; v |= (uint64_t)(c & 0x7F) << s; if (!(c & 0x80)) return v; s += 7; } }
int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <archive.aet> [S]\n", argv[0]); return 1; }
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; }
    uint8_t h[68]; if (fread(h, 1, 68, f) != 68) return 1; uint32_t nb; memcpy(&nb, h + 24, 4);
    std::vector<E> T(nb); if (fread(T.data(), 64, nb, f) != nb) return 1; fseek(f, 0, SEEK_END); const uint64_t asz = (uint64_t)ftell(f); fclose(f);
    const uint32_t S = argc > 2 ? (uint32_t)atoi(argv[2]) : 64;
    for (uint32_t b = 1; b < nb; b++) for (int s = 0; s < 4; s++) if (T[b].off[s] != T[b - 1].off[s] + T[b - 1].sz[s]) { printf("slices not back to back at block %u\n", b); return 2; }
    // superblock form: per superblock 4 x u64 + 1 byte (u16 or u32 sizes); per block 4 sizes
    std::vector<uint8_t> sb; std::vector<uint64_t> sb_at;
    for (uint32_t s0 = 0; s0 < nb; s0 += S) { const uint32_t s1 = std::min(nb, s0 + S); sb_at.push_back(sb.size());
        for (int s = 0; s < 4; s++) { const uint64_t o = T[s0].off[s]; sb.insert(sb.end(), (const uint8_t*)&o, (const uint8_t*)&o + 8); }
        bool w16 = true; for (uint32_t b = s0; b < s1; b++) for (int s = 0; s < 4; s++) if (T[b].sz[s] > 0xFFFF) w16 = false;
        sb.push_back(w16 ? 2 : 4);
        for (uint32_t b = s0; b < s1; b++) for (int s = 0; s < 4; s++) { const uint32_t v = (uint32_t)T[b].sz[s]; sb.insert(sb.end(), (const uint8_t*)&v, (const uint8_t*)&v + (w16 ? 2 : 4)); } }
    auto look = [&](uint32_t b, E& e) {                            // O(S): the superblock entry + a prefix sum inside it
        const uint32_t k = b / S; const uint8_t* p = sb.data() + sb_at[k]; uint64_t o[4]; memcpy(o, p, 32); const int w = p[32]; p += 33;
        for (uint32_t c = k * S; c < b; c++) for (int s = 0; s < 4; s++) { uint32_t v = 0; memcpy(&v, p, w); o[s] += v; p += w; }
        for (int s = 0; s < 4; s++) { uint32_t v = 0; memcpy(&v, p, w); e.off[s] = o[s]; e.sz[s] = v; p += w; } };
    // checks: the whole table rebuilt, then random lookups timed
    bool ok = true; for (uint32_t b = 0; b < nb && ok; b++) { E e; look(b, e); ok = !memcmp(&e, &T[b], 64); }
    std::mt19937 g(1); const int NL = 10000000; std::vector<uint32_t> q(NL); for (auto& x : q) x = g() % nb; uint64_t sink = 0;
    const auto t0 = std::chrono::steady_clock::now(); for (uint32_t b : q) { E e; look(b, e); sink += e.off[0] + e.sz[3]; }
    const double ns = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() / NL * 1e9;
    for (uint32_t b : q) if (sink == 1) { E e; look(b, e); ok = ok && !memcmp(&e, &T[b], 64); }
    // columns: the 4 sizes as varints, column by column, zstd -19 (sequential decode of the column)
    std::vector<uint8_t> col; for (int s = 0; s < 4; s++) for (uint32_t b = 0; b < nb; b++) put_v(col, T[b].sz[s]);
    std::vector<uint8_t> zc(ZSTD_compressBound(col.size())); zc.resize(ZSTD_compress(zc.data(), zc.size(), col.data(), col.size(), 19));
    { std::vector<uint8_t> back(col.size()); ZSTD_decompress(back.data(), back.size(), zc.data(), zc.size()); const uint8_t* p = back.data();
      std::vector<uint64_t> sz(4 * (size_t)nb); for (auto& v : sz) v = get_v(p);
      uint64_t o[4] = {T[0].off[0], T[0].off[1], T[0].off[2], T[0].off[3]};
      for (uint32_t b = 0; b < nb && ok; b++) { E e; for (int s = 0; s < 4; s++) { e.off[s] = o[s]; e.sz[s] = sz[(size_t)s * nb + b]; o[s] += e.sz[s]; } ok = !memcmp(&e, &T[b], 64); } }
    const uint64_t tab = 64ull * nb, a_sb = asz - tab + sb.size(), a_zc = asz - tab + zc.size() + 32;
    printf("%s: %u blocks; table %llu B -> superblocks of %u: %zu B (%.2f B/block, lookup %.0f ns), size columns + zstd: %zu B (%.2f B/block); "
           "archive %llu -> %llu (%.2f %%) / %llu (%.2f %%); %s\n", argv[1], nb, (unsigned long long)tab, S, sb.size(), (double)sb.size() / nb, ns, zc.size(), (double)zc.size() / nb,
           (unsigned long long)asz, (unsigned long long)a_sb, 100.0 * ((double)a_sb / asz - 1), (unsigned long long)a_zc, 100.0 * ((double)a_zc / asz - 1), ok ? "tables rebuilt == original" : "MISMATCH");
    printf("SBROW\t%s\t%llu\t%u\t%llu\t%zu\t%zu\t%llu\t%llu\t%.0f\t%s\n", argv[1], (unsigned long long)asz, nb, (unsigned long long)tab, sb.size(), zc.size(), (unsigned long long)a_sb, (unsigned long long)a_zc, ns, ok ? "ok" : "MISMATCH");
    return ok ? 0 : 3;
}
