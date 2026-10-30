// aceapex_encode.cpp
// Reference implementation for the ACEAPEX ENCODE-BOTTLENECK challenge.
//
// Emits LZ77 factorization with ABSOLUTE source positions (project identity).
// Compares two match finders:
//   (1) SA-EXACT   : full suffix array + exact Longest-Previous-Factor (ground truth,
//                    the "dense" factorization analogous to Shun/Zhao). SLOW on purpose.
//   (2) HASHCHAIN  : no suffix array. hash of first-k bytes + bounded chains. FAST.
// Verifies BIT-PERFECT reconstruction for both (byte-compare + FNV-1a).
// Reports: factor count, coverage, encode throughput (MB/s), and ratio under two
// offset encodings: ABSOLUTE varint vs DISTANCE(=i-src) varint, plus order-0 entropy
// estimates of each stream (a fair proxy for what a range/ANS coder achieves).
//
// Single-file, stdlib-only, C++17. Compile: g++ -O3 -march=native -fopenmp
//
// HONESTY NOTES:
//  * order-0 entropy is an ESTIMATE of an entropy coder's output, not an exact codec.
//  * Throughput here is single-machine; GPU/parallel scaling must be measured on the pod.

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <cmath>
#ifdef _OPENMP
#include <omp.h>
#endif

using std::vector;
using u8  = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i64 = int64_t;

static double now_s() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

static u64 fnv1a(const u8* p, size_t n) {
    u64 h = 1469598103934665603ULL;
    for (size_t i = 0; i < n; ++i) { h ^= p[i]; h *= 1099511628211ULL; }
    return h;
}

// ---------- token model ----------
// A factor is either a literal byte, or a match (abs_src, len) with len >= MINMATCH.
struct Token { u32 src; u32 len; u8 lit; bool is_match; };

static const int MINMATCH = 4;

// ---------- reconstruction (bit-perfect, handles overlap via byte-by-byte copy) ----------
static vector<u8> reconstruct(const vector<Token>& toks) {
    vector<u8> out;
    out.reserve(1u << 20);
    for (const Token& t : toks) {
        if (!t.is_match) { out.push_back(t.lit); }
        else {
            u32 s = t.src;                         // ABSOLUTE position into output
            for (u32 j = 0; j < t.len; ++j) out.push_back(out[s + j]); // overlap-safe
        }
    }
    return out;
}

// ================= (1) SA-EXACT ground truth =================
// prefix-doubling suffix array (robust O(n log^2 n) with std::sort).
static vector<int> build_sa(const u8* T, int n) {
    vector<int> sa(n), rank(n), tmp(n);
    for (int i = 0; i < n; i++) { sa[i] = i; rank[i] = T[i]; }
    for (int k = 1;; k <<= 1) {
        auto cmp = [&](int a, int b) {
            if (rank[a] != rank[b]) return rank[a] < rank[b];
            int ra = (a + k < n) ? rank[a + k] : -1;
            int rb = (b + k < n) ? rank[b + k] : -1;
            return ra < rb;
        };
        std::sort(sa.begin(), sa.end(), cmp);
        tmp[sa[0]] = 0;
        for (int i = 1; i < n; i++) tmp[sa[i]] = tmp[sa[i - 1]] + (cmp(sa[i - 1], sa[i]) ? 1 : 0);
        for (int i = 0; i < n; i++) rank[i] = tmp[i];
        if (rank[sa[n - 1]] == n - 1) break;
    }
    return sa;
}

// exact Longest-Previous-Factor via PSV/NSV over SA (Crochemore/Ilie style),
// LCP by direct comparison. Returns best_src[i], best_len[i] (len 0 if none).
static void lpf_exact(const u8* T, int n, vector<u32>& best_src, vector<u32>& best_len) {
    vector<int> sa = build_sa(T, n);
    best_src.assign(n, 0);
    best_len.assign(n, 0);
    // PrevSmaller / NextSmaller of SA (sequence indexed by rank r, value = text pos SA[r]).
    vector<int> psv(n, -1), nsv(n, -1), st;
    st.reserve(n);
    for (int r = 0; r < n; r++) {                 // prev smaller
        while (!st.empty() && sa[st.back()] > sa[r]) st.pop_back();
        psv[r] = st.empty() ? -1 : st.back();
        st.push_back(r);
    }
    st.clear();
    for (int r = n - 1; r >= 0; r--) {            // next smaller
        while (!st.empty() && sa[st.back()] > sa[r]) st.pop_back();
        nsv[r] = st.empty() ? -1 : st.back();
        st.push_back(r);
    }
    auto lcp = [&](int a, int b) -> u32 {         // a,b are text positions
        int m = n - std::max(a, b), j = 0;
        while (j < m && T[a + j] == T[b + j]) j++;
        return (u32)j;
    };
    for (int r = 0; r < n; r++) {
        int p = sa[r];                            // text position
        u32 bl = 0, bs = 0;
        if (psv[r] >= 0) { int c = sa[psv[r]]; u32 l = lcp(p, c); if (l > bl) { bl = l; bs = c; } }
        if (nsv[r] >= 0) { int c = sa[nsv[r]]; u32 l = lcp(p, c); if (l > bl) { bl = l; bs = c; } }
        best_src[p] = bs; best_len[p] = bl;       // src < p guaranteed (smaller text pos)
    }
}

// ================= (2) HASHCHAIN finder (NO suffix array) =================
static inline u32 hash4(const u8* p, u32 bits) {
    u32 v; std::memcpy(&v, p, 4);
    return (v * 2654435761u) >> (32 - bits);      // Fibonacci hashing
}
static inline u32 hashK(const u8* p, int k, u32 bits) {
    u64 h = 1469598103934665603ULL;
    for (int i=0;i<k;i++){ h ^= p[i]; h *= 1099511628211ULL; }
    return (u32)((h * 2654435761u) >> (64 - bits));
}

// LSD radix sort of u64 keys (8-bit passes). Only the low ~48 bits are ever set
// (hash<=24 bits in high half, pos<=32 in low half) so 6 passes suffice.
static void radix_sort_u64(vector<u64>& a, int passes) {
    size_t n = a.size();
    vector<u64> b(n);
    for (int p = 0; p < passes; ++p) {
        int shift = p * 8;
        size_t cnt[256] = {0};
        for (size_t i = 0; i < n; i++) cnt[(a[i] >> shift) & 0xFF]++;
        size_t sum = 0;
        for (int c = 0; c < 256; c++) { size_t t = cnt[c]; cnt[c] = sum; sum += t; }
        for (size_t i = 0; i < n; i++) { u8 d = (a[i] >> shift) & 0xFF; b[cnt[d]++] = a[i]; }
        a.swap(b);
    }
}

// GPU-style two-phase: (A) index build = sort positions by (hash,pos); (B) per-position
// independent search over same-hash LEFT neighbors (all guaranteed earlier). Both phases
// are data-parallel and map directly to cub::DeviceRadixSort + a parallel-for on GPU.
static void lpf_hashchain(const u8* T, int n, int max_chain, int hbits, int klen,
                          vector<u32>& best_src, vector<u32>& best_len,
                          double& t_build, double& t_find) {
    best_src.assign(n, 0);
    best_len.assign(n, 0);
    const int last = n - klen;
    if (last < 0) return;
    const int m = last + 1;                       // number of indexable positions
    // --- Phase A: pack key = (hash<<32 | pos), sort. On GPU: cub::DeviceRadixSort. ---
    double ta = now_s();
    int posbits = 1; while ((1 << posbits) < m) posbits++;   // bits to hold a position
    vector<u64> keyed(m);
    for (int i = 0; i < m; i++) {
        u64 h = hashK(T + i, klen, hbits);
        keyed[i] = (h << posbits) | (u32)i;       // hash high, pos low (ascending inside bucket)
    }
    int total_bits = posbits + hbits;
    int passes = (total_bits + 7) / 8;            // cover every significant bit
    // This is the primitive GPUs excel at (cub::DeviceRadixSort), unlike suffix-array
    // induced sorting which is memory-bound and hard to parallelize.
    radix_sort_u64(keyed, passes);                // groups equal hashes; pos ascending inside
    t_build = now_s() - ta;
    // --- Phase B: for each sorted entry, scan LEFT neighbors in same hash bucket ---
    double tb = now_s();
    const u64 posmask = ((u64)1 << posbits) - 1;
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic, 4096)
#endif
    for (int s = 0; s < m; s++) {
        u64 hcur = keyed[s] >> posbits;
        int p = (int)(keyed[s] & posmask);        // this position
        const int mmax = n - p;
        u32 bl = 0, bs = 0;
        int depth = 0;
        for (int s2 = s - 1; s2 >= 0 && depth < max_chain; --s2) {
            if ((keyed[s2] >> posbits) != hcur) break;   // left bucket boundary
            int q = (int)(keyed[s2] & posmask);   // q < p guaranteed (ascending pos in bucket)
            int j = 0;
            while (j < mmax && T[p + j] == T[q + j]) j++;
            if ((u32)j > bl) { bl = (u32)j; bs = (u32)q; if (j == mmax) break; }
            depth++;
        }
        best_len[p] = bl; best_src[p] = bs;
    }
    t_find = now_s() - tb;
}

// ================= greedy parse over an LPF table =================
static vector<Token> greedy_parse(const u8* T, int n,
                                  const vector<u32>& best_src, const vector<u32>& best_len) {
    vector<Token> toks; toks.reserve(n / 2);
    int i = 0;
    while (i < n) {
        u32 L = (i < (int)best_len.size()) ? best_len[i] : 0;
        if ((int)L >= MINMATCH) {
            Token t; t.is_match = true; t.src = best_src[i]; t.len = L; t.lit = 0;
            toks.push_back(t);
            i += (int)L;
        } else {
            Token t; t.is_match = false; t.src = 0; t.len = 0; t.lit = T[i];
            toks.push_back(t);
            i += 1;
        }
    }
    return toks;
}

// ================= size / ratio analysis =================
static int leb_len(u64 v) { int b = 1; while (v >= 0x80) { v >>= 7; b++; } return b; }

static double order0_bits(const vector<u64>& stream) {
    if (stream.empty()) return 0.0;
    // histogram over values (map-free: use sort)
    vector<u64> s = stream; std::sort(s.begin(), s.end());
    double bits = 0.0; size_t N = s.size();
    size_t i = 0;
    while (i < N) {
        size_t j = i; while (j < N && s[j] == s[i]) j++;
        double p = double(j - i) / double(N);
        bits += (j - i) * (-std::log2(p));
        i = j;
    }
    return bits; // total bits to code this stream at order-0
}

struct RatioReport {
    size_t n, factors, matches, literals, covered;
    double raw_abs_ratio, raw_dist_ratio;      // byte-packed varint
    double ent_abs_ratio, ent_dist_ratio;      // order-0 entropy estimate
};

static RatioReport analyze(const u8* T, int n, const vector<Token>& toks) {
    RatioReport R{}; R.n = n; R.factors = toks.size();
    // streams
    vector<u64> lits, lens, abs_off, dist_off, flags;
    size_t rawA = 0, rawD = 0, covered = 0, matches = 0;
    int pos = 0;
    for (const Token& t : toks) {
        flags.push_back(t.is_match ? 1 : 0);
        if (!t.is_match) { lits.push_back(t.lit); pos += 1; }
        else {
            matches++;
            lens.push_back(t.len);
            abs_off.push_back(t.src);
            dist_off.push_back((u64)pos - t.src);   // i - src, always > 0
            covered += t.len;
            rawA += leb_len(t.src) + leb_len(t.len);
            rawD += leb_len((u64)pos - t.src) + leb_len(t.len);
            pos += t.len;
        }
    }
    R.matches = matches; R.literals = lits.size(); R.covered = covered;
    // raw byte-packed: flags as 1 bit each (n/8), literals 1 byte each, offsets+lens as above
    double flag_bytes = toks.size() / 8.0;
    double lit_bytes  = (double)lits.size();
    double rawAbs  = flag_bytes + lit_bytes + rawA;
    double rawDist = flag_bytes + lit_bytes + rawD;
    R.raw_abs_ratio  = n / rawAbs;
    R.raw_dist_ratio = n / rawDist;
    // entropy estimate: each stream order-0
    double b_flags = order0_bits(flags);
    double b_lits  = order0_bits(lits);
    double b_lens  = order0_bits(lens);
    double b_abs   = order0_bits(abs_off);
    double b_dist  = order0_bits(dist_off);
    double entAbs  = (b_flags + b_lits + b_lens + b_abs)  / 8.0;
    double entDist = (b_flags + b_lits + b_lens + b_dist) / 8.0;
    R.ent_abs_ratio  = n / entAbs;
    R.ent_dist_ratio = n / entDist;
    return R;
}

// ================= driver =================
static vector<u8> read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); exit(1); }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    vector<u8> v(sz);
    if (fread(v.data(), 1, sz, f) != (size_t)sz) { fprintf(stderr, "read err\n"); exit(1); }
    fclose(f);
    return v;
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <file> [run_sa=1] [max_chain=64] [hbits=18]\n", argv[0]); return 1; }
    vector<u8> Tv = read_file(argv[1]);
    int n = (int)Tv.size();
    const u8* T = Tv.data();
    int run_sa   = argc > 2 ? atoi(argv[2]) : 1;
    int max_chain= argc > 3 ? atoi(argv[3]) : 64;
    int hbits    = argc > 4 ? atoi(argv[4]) : 18;
    int klen     = argc > 5 ? atoi(argv[5]) : 4;
    u64 fin = fnv1a(T, n);
    int threads = 1;
#ifdef _OPENMP
    threads = omp_get_max_threads();
#endif
    printf("=== %s  n=%d bytes  threads=%d  max_chain=%d hbits=%d k=%d ===\n",
           argv[1], n, threads, max_chain, hbits, klen);
    printf("input FNV=%016llx\n", (unsigned long long)fin);

    // ---- HASHCHAIN ----
    {
        vector<u32> bs, bl; double tbuild=0, tfind=0;
        double t0 = now_s();
        lpf_hashchain(T, n, max_chain, hbits, klen, bs, bl, tbuild, tfind);
        vector<Token> toks = greedy_parse(T, n, bs, bl);
        double t1 = now_s();
        vector<u8> rec = reconstruct(toks);
        bool ok = (rec.size() == (size_t)n) && (fnv1a(rec.data(), rec.size()) == fin)
                  && (memcmp(rec.data(), T, n) == 0);
        double mbps = (n / 1e6) / (t1 - t0);
        RatioReport R = analyze(T, n, toks);
        printf("\n[HASHCHAIN]  bit-perfect=%s\n", ok ? "YES" : "NO*** MISMATCH");
        printf("  encode: %.2f ms  (%.1f MB/s)   [build %.2f ms | find %.2f ms | parse %.2f ms]\n",
               (t1-t0)*1e3, mbps, tbuild*1e3, tfind*1e3, (t1-t0-tbuild-tfind)*1e3);
        printf("  factors=%zu  matches=%zu  literals=%zu  coverage=%.2f%%\n",
               R.factors, R.matches, R.literals, 100.0*R.covered/n);
        printf("  ratio  ABS-varint=%.3f  DIST-varint=%.3f | ABS-ent=%.3f  DIST-ent=%.3f\n",
               R.raw_abs_ratio, R.raw_dist_ratio, R.ent_abs_ratio, R.ent_dist_ratio);
    }

    // ---- SA-EXACT ground truth (optional; slow) ----
    if (run_sa) {
        vector<u32> bs, bl;
        double t0 = now_s();
        lpf_exact(T, n, bs, bl);
        vector<Token> toks = greedy_parse(T, n, bs, bl);
        double t1 = now_s();
        vector<u8> rec = reconstruct(toks);
        bool ok = (rec.size() == (size_t)n) && (fnv1a(rec.data(), rec.size()) == fin)
                  && (memcmp(rec.data(), T, n) == 0);
        double mbps = (n / 1e6) / (t1 - t0);
        RatioReport R = analyze(T, n, toks);
        printf("\n[SA-EXACT]   bit-perfect=%s\n", ok ? "YES" : "NO*** MISMATCH");
        printf("  encode: %.2f ms  (%.1f MB/s)  [includes full SA build]\n", (t1-t0)*1e3, mbps);
        printf("  factors=%zu  matches=%zu  literals=%zu  coverage=%.2f%%\n",
               R.factors, R.matches, R.literals, 100.0*R.covered/n);
        printf("  ratio  ABS-varint=%.3f  DIST-varint=%.3f | ABS-ent=%.3f  DIST-ent=%.3f\n",
               R.raw_abs_ratio, R.raw_dist_ratio, R.ent_abs_ratio, R.ent_dist_ratio);
    }
    printf("\n");
    return 0;
}
