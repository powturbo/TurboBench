// Where real data sits on the match-length curve (Paper 4, Section 4.1).
//
// Runs a greedy hash-chain LZ factorization over a real file and reports the match-length
// distribution: mean length, coverage, and the fraction of matches shorter than the
// cooperation width (32). That fraction is what pins decode throughput to the low end
// of the curve produced by match_length_curve.cu.
//
// Build: g++ -O3 -march=native -std=c++17 -o match_histogram match_histogram.cpp
// Run:   ./match_histogram <file> [bytes] [min_match_len]
//
// Our measurements (256 MB prefix, min_match_len=4):
//   enwik9 : 32.5M matches, coverage 82.8%, mean length  6.5  (99.1% below 32)
//   FASTQ  : 11.9M matches, coverage 93.0%, mean length 20.0  (84.8% below 32)

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

int main(int argc, char** argv)
{
    if (argc < 2) { printf("usage: %s <file> [bytes] [min_len]\n", argv[0]); return 1; }
    uint32_t MINLEN = (argc > 3) ? (uint32_t)atoi(argv[3]) : 4;

    FILE* f = fopen(argv[1], "rb");
    if (!f) { printf("cannot open %s\n", argv[1]); return 1; }
    fseek(f, 0, SEEK_END); long fsz = ftell(f); fseek(f, 0, SEEK_SET);
    uint32_t n = (argc > 2) ? (uint32_t)atoll(argv[2]) : (uint32_t)fsz;
    if ((long)n > fsz) n = (uint32_t)fsz;
    std::vector<uint8_t> buf(n);
    if (fread(buf.data(), 1, n, f) != n) { printf("read failed\n"); return 1; }
    fclose(f);

    const uint32_t HBITS = 22, HSIZE = 1u << HBITS, HMASK = HSIZE - 1;
    std::vector<int32_t> head(HSIZE, -1);

    uint64_t n_match = 0, covered = 0, sum_len = 0, below32 = 0;
    uint64_t hist[7] = {0};   // <8, <16, <32, <64, <128, <256, >=256

    uint32_t i = 0;
    while (i + 4 <= n) {
        uint32_t v; memcpy(&v, &buf[i], 4);
        uint32_t h = (v * 2654435761u) & HMASK;
        int32_t s = head[h];
        head[h] = (int32_t)i;

        uint32_t best = 0;
        if (s >= 0 && (uint32_t)s < i) {
            uint32_t l = 0, lim = n - i;
            while (l < lim && buf[i + l] == buf[s + l] && l < 65535) l++;
            if (l >= MINLEN) best = l;
        }
        if (best >= MINLEN) {
            n_match++; covered += best; sum_len += best;
            if (best < 32) below32++;
            if      (best <   8) hist[0]++;
            else if (best <  16) hist[1]++;
            else if (best <  32) hist[2]++;
            else if (best <  64) hist[3]++;
            else if (best < 128) hist[4]++;
            else if (best < 256) hist[5]++;
            else                 hist[6]++;
            i += best;
        } else {
            i++;   // literal
        }
    }

    printf("=== %s (%u bytes, min_match_len=%u) ===\n", argv[1], n, MINLEN);
    printf("matches        : %llu\n", (unsigned long long)n_match);
    printf("coverage       : %.1f%% of output\n", 100.0 * (double)covered / n);
    printf("mean length    : %.1f bytes\n", n_match ? (double)sum_len / n_match : 0.0);
    printf("shorter than 32: %.1f%% of matches  <-- these underload the warp\n",
           n_match ? 100.0 * (double)below32 / n_match : 0.0);
    printf("\nlength histogram:\n");
    const char* lbl[7] = {"  <8", " <16", " <32", " <64", "<128", "<256", ">=256"};
    for (int k = 0; k < 7; k++)
        printf("  %s : %10llu (%.1f%%)\n", lbl[k], (unsigned long long)hist[k],
               n_match ? 100.0 * (double)hist[k] / n_match : 0.0);
    return 0;
}
