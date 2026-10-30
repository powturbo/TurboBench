// Why raising the minimum match length improves ratio (Paper 4, Section 4.3).
//
// Computes the order-0 entropy of the three streams a tile-ANS backend codes separately:
// literals, varint-encoded offsets, varint-encoded lengths. Sweeping min_match_len shows
// that on genomic data the offset stream dominates the compressed size at low thresholds
// (66% of the stream at min_len=4) — short matches to far, near-random offsets spend more
// bits on the offset than they save in replaced literals. Removing them shrinks the stream.
//
// This is a model of the entropy stage, not the shipping encoder: the real encoder is
// stronger (dist-dependent thresholds, repeat offsets, depth factorization), so its
// absolute ratios are higher. Use this to see the MECHANISM; use aceapex_depth for
// the ratios reported in Tables 3-4.
//
// Build: g++ -O3 -std=c++17 -o offset_entropy offset_entropy.cpp
// Run:   ./offset_entropy <file> [bytes]

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>

static double entropy_bytes(const std::vector<uint8_t>& v)
{
    if (v.empty()) return 0.0;
    uint64_t c[256] = {0};
    for (uint8_t b : v) c[b]++;
    double H = 0.0, n = (double)v.size();
    for (int i = 0; i < 256; i++)
        if (c[i]) { double p = c[i] / n; H -= p * log2(p); }
    return H * n / 8.0;                     // bytes after ideal order-0 coding
}

static void emit_varint(std::vector<uint8_t>& o, uint32_t x)
{
    while (x >= 128) { o.push_back((uint8_t)((x & 127) | 128)); x >>= 7; }
    o.push_back((uint8_t)x);
}

int main(int argc, char** argv)
{
    if (argc < 2) { printf("usage: %s <file> [bytes]\n", argv[0]); return 1; }
    FILE* f = fopen(argv[1], "rb");
    if (!f) { printf("cannot open %s\n", argv[1]); return 1; }
    fseek(f, 0, SEEK_END); long fsz = ftell(f); fseek(f, 0, SEEK_SET);
    uint32_t n = (argc > 2) ? (uint32_t)atoll(argv[2]) : (uint32_t)fsz;
    if ((long)n > fsz) n = (uint32_t)fsz;
    std::vector<uint8_t> buf(n);
    if (fread(buf.data(), 1, n, f) != n) { printf("read failed\n"); return 1; }
    fclose(f);

    printf("=== %s (%u bytes) ===\n", argv[1], n);
    printf("min_len  literals   offsets   lengths     total   ratio  mean_len  off_share\n");

    const uint32_t mins[] = {4, 8, 16, 24, 32, 48};
    for (uint32_t ML : mins) {
        const uint32_t HBITS = 22, HSIZE = 1u << HBITS, HMASK = HSIZE - 1;
        std::vector<int32_t> head(HSIZE, -1);
        std::vector<uint8_t> lits, offs, lens;
        uint64_t covered = 0, n_match = 0;

        uint32_t i = 0;
        while (i + 4 <= n) {
            uint32_t v; memcpy(&v, &buf[i], 4);
            uint32_t h = (v * 2654435761u) & HMASK;
            int32_t s = head[h];
            head[h] = (int32_t)i;

            uint32_t best = 0, bsrc = 0;
            if (s >= 0 && (uint32_t)s < i) {
                uint32_t l = 0, lim = n - i;
                while (l < lim && buf[i + l] == buf[s + l] && l < 65535) l++;
                if (l >= ML) { best = l; bsrc = (uint32_t)s; }
            }
            if (best >= ML) {
                emit_varint(offs, i - bsrc);
                emit_varint(lens, best - ML);
                covered += best; n_match++; i += best;
            } else {
                lits.push_back(buf[i]); i++;
            }
        }
        while (i < n) { lits.push_back(buf[i]); i++; }

        double lb = entropy_bytes(lits);
        double ob = entropy_bytes(offs);
        double nb = entropy_bytes(lens);
        double total = lb + ob + nb;
        printf("%6u %10.0f %9.0f %9.0f %9.0f  %6.2f  %7.1f    %5.1f%%\n",
               ML, lb, ob, nb, total, (double)n / total,
               n_match ? (double)covered / n_match : 0.0,
               total > 0 ? 100.0 * ob / total : 0.0);
    }
    return 0;
}
