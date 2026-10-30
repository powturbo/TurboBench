// approx_est.cpp - offline estimate of a "match with substitutions" token (no format change): every block of a FASTA
// file (the archive's block size; blocks stay independent) parsed greedily over its bases (case folded, line ends and
// header lines left out, N never matched) with matches that may carry up to k substitutions per 100 bases (k = 0..8;
// k = 0 is the exact matcher of the same model). Costs in bits, measured on the archive by the caller:
// literal base c_lit, token head c_tok (command + length); the distance costs log2(d) + 2 bits, a repeat of the last
// distance 2 bits; a token with substitutions costs 4 more (their count) + c_sub each (default 10: a position gap and
// the base). With a flat token cost random 12-mer hits 1 MiB away looked profitable (47 % of chr1 "covered"); with the
// distance priced the exact model (k = 0) lands near the archive's own match share. A match is taken where it gains (len x c_lit - its cost > 0); candidates:
// the last distance (rep) and the 4 latest positions of the 12-mer seed. Output: one row per block, per k the covered
// bases, substitutions, tokens and the gain in bits; scripts/region_bits.py sums them by region class.
// Build: g++ -std=c++17 -O3 -march=native -pthread scripts/approx_est.cpp -o approx_est
// A match must be at least MINLEN bases (argument, default 32 - the l1 encoder's shortest match; the real encoder
// loses on shorter ones: chr1 AX_MINL=16 +0.2 %, so the model's flat literal cost overrates them).
// Usage: approx_est <fasta> <block_size> <c_lit> <c_tok> [c_sub] [threads] [minlen] > rows.tsv
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
static const int NK = 9, SEED = 12, BUCKET = 4;
struct Row { uint64_t bases = 0, cov[NK] = {}, subs[NK] = {}, toks[NK] = {}; double gain[NK] = {}; };
static double C_LIT, C_TOK, C_SUB; static uint32_t MINLEN = 32;
static inline int code2(uint8_t c) { switch (c) { case 'A': return 0; case 'C': return 1; case 'G': return 2; case 'T': return 3; default: return -1; } }
static void parse(const std::vector<uint8_t>& s, int k, Row& R, std::vector<uint32_t>& code, std::vector<uint8_t>& ok, std::vector<uint32_t>& tab) {
    const uint32_t n = (uint32_t)s.size(); if (n < SEED) return;
    uint32_t lg = 10; while ((1u << lg) < 2 * n) lg++; tab.assign((size_t)BUCKET << lg, 0);
    auto H = [&](uint32_t c) { return ((c * 2654435761u) >> (32 - lg)) * BUCKET; };
    auto ins = [&](uint32_t i) { if (!ok[i]) return; uint32_t* b = &tab[H(code[i])]; memmove(b + 1, b, (BUCKET - 1) * 4); b[0] = i + 1; };
    uint32_t i = 0, last_d = 0;
    while (i < n) {
        if (s[i] == 'N') { i++; continue; }
        uint32_t cand[BUCKET + 1]; int nc = 0;
        if (last_d && last_d <= i) cand[nc++] = i - last_d;
        if (i + SEED <= n && ok[i]) { const uint32_t* b = &tab[H(code[i])]; for (int q = 0; q < BUCKET; q++) if (b[q] && code[b[q] - 1] == code[i] && ok[b[q] - 1]) cand[nc++] = b[q] - 1; }
        double bg = 0; uint32_t bl = 0, bm = 0, bj = 0;
        for (int c = 0; c < nc; c++) { const uint32_t j = cand[c]; if (j >= i) continue;
            uint32_t m = 0; const uint32_t cap = std::min<uint32_t>(n - i, 1u << 20);
            const double dcost = (i - j == last_d) ? 2.0 : std::log2((double)(i - j)) + 2.0;
            for (uint32_t t = 0; t < cap; t++) { const uint8_t a = s[i + t], b = s[j + t]; if (a == 'N' || b == 'N') break;
                if (a != b) { m++; if (m > (uint32_t)k * (t + 1) / 100 + (k ? 3 : 0)) break; continue; }
                if (m <= (uint32_t)k * (t + 1) / 100) { const double g = (t + 1) * C_LIT - C_TOK - dcost - (m ? 4 + m * C_SUB : 0); if (g > bg && t + 1 >= MINLEN) { bg = g; bl = t + 1; bm = m; bj = j; } } } }
        if (bl) { R.cov[k] += bl; R.subs[k] += bm; R.toks[k]++; R.gain[k] += bg; last_d = i - bj; for (uint32_t t = 0; t < bl; t++) ins(i + t); i += bl; }
        else { ins(i); i++; }
    }
}
int main(int argc, char** argv) {
    if (argc < 5) { fprintf(stderr, "usage: %s <fasta> <block_size> <c_lit> <c_tok> [c_sub] [threads]\n", argv[0]); return 1; }
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END); std::vector<uint8_t> a((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(a.data(), 1, a.size(), f) != a.size()) return 1; fclose(f);
    const uint64_t bs = strtoull(argv[2], 0, 10); C_LIT = atof(argv[3]); C_TOK = atof(argv[4]); C_SUB = argc > 5 ? atof(argv[5]) : 10.0;
    int T = argc > 6 ? atoi(argv[6]) : 0; if (T <= 0) T = (int)std::thread::hardware_concurrency();
    if (argc > 7) MINLEN = (uint32_t)atoi(argv[7]);
    const uint64_t nb = (a.size() + bs - 1) / bs; std::vector<Row> rows(nb); std::atomic<uint64_t> next{0};
    // header lines: a byte is in a header from '>' to the line end (precomputed so blocks may start inside one)
    std::vector<uint8_t> hdr(a.size(), 0); { bool in = false; for (size_t i = 0; i < a.size(); i++) { if (a[i] == '>' && (i == 0 || a[i - 1] == '\n')) in = true; hdr[i] = in; if (a[i] == '\n') in = false; } }
    std::vector<std::thread> th;
    for (int t = 0; t < T; t++) th.emplace_back([&] { std::vector<uint8_t> s, ok; std::vector<uint32_t> code, tab;
        for (uint64_t b; (b = next++) < nb; ) { const uint64_t lo = b * bs, hi = std::min<uint64_t>(a.size(), lo + bs); s.clear();
            for (uint64_t i = lo; i < hi; i++) { if (hdr[i]) continue; uint8_t c = a[i]; if (c == '\n' || c == '\r') continue; c &= 0xDF; if (code2(c) < 0) c = 'N'; s.push_back(c); }
            const uint32_t n = (uint32_t)s.size(); code.assign(n, 0); ok.assign(n, 0); uint32_t run = 0, cd = 0;
            for (uint32_t i = 0; i < n; i++) { const int x = code2(s[i]); if (x < 0) { run = 0; continue; } cd = (cd << 2 | (uint32_t)x) & ((1u << (2 * SEED)) - 1); if (++run >= (uint32_t)SEED) { code[i + 1 - SEED] = cd; ok[i + 1 - SEED] = 1; } }
            Row& R = rows[b]; for (uint32_t i = 0; i < n; i++) R.bases += s[i] != 'N';
            for (int k = 0; k < NK; k++) parse(s, k, R, code, ok, tab); } });
    for (auto& x : th) x.join();
    printf("block\tbases"); for (int k = 0; k < NK; k++) printf("\tcov%d\tsubs%d\ttoks%d\tgain%d", k, k, k, k); printf("\n");
    for (uint64_t b = 0; b < nb; b++) { const Row& R = rows[b]; printf("%llu\t%llu", (unsigned long long)b, (unsigned long long)R.bases);
        for (int k = 0; k < NK; k++) printf("\t%llu\t%llu\t%llu\t%.0f", (unsigned long long)R.cov[k], (unsigned long long)R.subs[k], (unsigned long long)R.toks[k], R.gain[k]); printf("\n"); }
    return 0;
}
