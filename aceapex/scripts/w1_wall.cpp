// w1_wall.cpp - W1: where the FAST decoder's single-core wall is, and what moves it. One core (pin it: taskset -c 2),
// median of R runs, every run compared with the input. Run it under `perf stat` with the DRAM counters of
// results/cpu-ceiling-2026-10-01.log to get bytes of DRAM traffic per output byte (scripts/w1_run.sh does that).
//
//   memcpy     the input copied into the output buffer: the reference lzbench prints as `memcpy`
//   base       axf decoder (src/ax_fast.h, guarded runs) straight into the output: 1 B written + ~1 B read for
//              ownership + ratio B read per output byte
//   E0         the first <= 8 MiB of output decoded 16 times into a cache-resident buffer: memory-bound or core-bound?
//   pfw        PREFETCHW on the output lines up to AHEAD bytes past the write pointer: the read for ownership still
//              happens (traffic unchanged) but earlier - separates latency from traffic
//   clz        CLZERO (AMD Zen, CPUID 8000_0008 EBX[0]) on the output lines up to AHEAD past the write pointer: the line
//              is allocated zeroed in the cache with no read from DRAM and stays cached for the matches that read it
//              back; skipped on CPUs without it
//   nt         each block into a cache-resident scratch, then streamed to the output with non-temporal stores
//   x2         two blocks decoded alternately, 16 tokens at a time, on the one core: two independent miss streams
//              (memory-level parallelism) with the same traffic - if x2 beats base, the core waits on latency
//   stats      where the bytes come from: literal share, match bytes by distance (<= 32 KiB ~ L1/L2 hot, <= 1 MiB),
//              and the share of output lines no later match of the block reads ("cold": nobody needs them cached)
//
// Archive built here with axf::enc_block at any level (1..8 = library 11..18) and block size (4..1024 KiB).
// A measurement tool, not a library path. Build: g++ -O3 -std=c++17 -Isrc -o w1_wall scripts/w1_wall.cpp -lpthread
// Usage: w1_wall file [level=1] [block_KiB=1024] [reps=7] [ahead=512] [modes=all|comma list]
#include "ax_fast_enc.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <immintrin.h>
#if defined(__x86_64__)
#include <cpuid.h>
#endif
static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static bool has_clzero() {
#if defined(__x86_64__)
    unsigned a, b, c, d; if (!__get_cpuid(0x80000000u, &a, &b, &c, &d) || a < 0x80000008u) return false;
    __cpuid(0x80000008u, a, b, c, d); return (b & 1u) != 0;
#else
    return false;
#endif
}
static inline void clzero_line(void* p) {
#if defined(__x86_64__)
    __asm__ volatile(".byte 0x0f, 0x01, 0xfc" : : "a"(p) : "memory");   // clzero [rax]: the 64-byte line holding rax
#else
    (void)p;
#endif
}
enum Pol { P_NONE = 0, P_PFW = 1, P_CLZ = 2 };

// Resumable decoder state: the decoder of src/ax_fast.h split into steps of at most `budget` tokens, so two blocks can
// be interleaved on one core. Output-line policy (PFW / CLZ) applied after every token up to d + ahead, only on whole
// lines at or past the line holding d (a line the decoder already wrote into is never zeroed; bytes past d are
// don't-care until written, so zeroing a line a wide copy already touched is harmless).
struct St {
    const uint8_t *tp, *ext, *lit, *lend, *eend; uint8_t *dst, *d, *dend, *zhi, *zend; uint32_t i, ntok, tail, mm; bool bad;
    bool open(const uint8_t* src, size_t ssz, uint8_t* o, uint32_t n, uint32_t m) {
        bad = false; if (ssz < AXF_BHDR + AXF_PAD) return false;
        ntok = axf_rd32(src); const uint32_t next = axf_rd32(src + 4); tail = axf_rd32(src + 8);
        if ((uint64_t)AXF_BHDR + 4ull * ntok + next + AXF_PAD > ssz) return false;
        tp = src + AXF_BHDR; ext = tp + 4ull * ntok; lit = ext + next; eend = lit; lend = src + ssz - AXF_PAD;
        dst = d = o; dend = o + n; i = 0; mm = m;
        zhi = (uint8_t*)(((uintptr_t)o + 63) & ~(uintptr_t)63); zend = (uint8_t*)((uintptr_t)dend & ~(uintptr_t)63);
        return true;
    }
    template <int POL> inline void policy(size_t ahead) {
        if (POL == P_NONE) return;
        uint8_t* want = d + ahead; if (want > zend) want = zend;
        if (zhi < d) zhi = (uint8_t*)(((uintptr_t)d + 63) & ~(uintptr_t)63);
        for (; zhi + 64 <= want; zhi += 64) { if (POL == P_CLZ) clzero_line(zhi); else __builtin_prefetch(zhi, 1, 3); }
    }
    // up to `budget` tokens; returns false on a malformed block (bad = true) or when the block is finished (i == ntok)
    template <int POL> bool step(uint32_t budget, size_t ahead) {
        uint32_t stop = ntok - i < budget ? ntok : i + budget;
        while (i < stop) {
            uint32_t k = stop - i;
            { const size_t kd = (size_t)(dend - d) / AXF_ROOM, kl = (size_t)(lend - lit) / 62u + 1u;
              if (kd < k) k = (uint32_t)kd;
              if (kl < k) k = (uint32_t)kl; }
            for (; k; k--, i++) {
                const uint32_t t = axf_rd32(tp + 4ull * i);
                const uint32_t ll = t & 63u, mc = (t >> 6) & 63u, dist = (t >> 12) + AXF_MIN_DIST;
                if (AXF_UNLIKELY(ll == 63u || mc == 63u)) break;
                policy<POL>(ahead);
                axf_cp32(d, lit); if (ll > 32u) axf_cp32(d + 32, lit + 32);
                d += ll; lit += ll;
                if (AXF_UNLIKELY((size_t)(d - dst) < dist)) { bad = true; return false; }
                axf_cpw(d, d - dist, mc + mm); d += mc + mm;
            }
            if (AXF_UNLIKELY(lit > lend)) { bad = true; return false; }
            if (i >= stop) break;
            {   const uint32_t t = axf_rd32(tp + 4ull * i); uint32_t ll = t & 63u, mc = (t >> 6) & 63u, ml; const uint32_t dist = (t >> 12) + AXF_MIN_DIST;
                i++; int b = 0; uint32_t c0;
                if (ll == 63u) { ll += axf_ext(&ext, eend, &b); if (b) { bad = true; return false; } }
                if ((size_t)(lend - lit) < ll || (size_t)(dend - d) < ll) { bad = true; return false; }
                policy<POL>(ahead + ll);
                memcpy(d, lit, ll); d += ll; lit += ll;
                if ((size_t)(d - dst) < dist) { bad = true; return false; }
                ml = mc + mm; if (mc == 63u) { ml = AXF_COPY + 1 + axf_ext(&ext, eend, &b); if (b) { bad = true; return false; } }
                if ((size_t)(dend - d) < ml) { bad = true; return false; }
                policy<POL>(ahead + ml);
                for (c0 = 0; c0 < ml; ) { uint32_t c = ml - c0 < dist ? ml - c0 : dist; memcpy(d + c0, d + c0 - dist, c); c0 += c; }
                d += ml;
            }
        }
        return i < ntok;
    }
    bool finish() {
        if (bad || i != ntok) return false;
        if ((size_t)(lend - lit) != tail || (size_t)(dend - d) != tail) return false;
        memcpy(d, lit, tail); return true;
    }
};
template <int POL> static bool dec_block(const uint8_t* src, size_t ssz, uint8_t* dst, uint32_t n, uint32_t mm, size_t ahead) {
    St s; if (!s.open(src, ssz, dst, n, mm)) return false;
    while (s.step<POL>(0xffffffffu, ahead)) {}
    return s.finish();
}
static void nt_copy(uint8_t* d, const uint8_t* s, size_t n) {   // head/tail by memcpy, the aligned middle with 16-byte NT stores
    size_t h = (16 - ((uintptr_t)d & 15)) & 15; if (h > n) h = n; memcpy(d, s, h); d += h; s += h; n -= h;
    for (; n >= 64; n -= 64, d += 64, s += 64) {
        const __m128i a = _mm_loadu_si128((const __m128i*)s), b = _mm_loadu_si128((const __m128i*)(s + 16)),
                      c = _mm_loadu_si128((const __m128i*)(s + 32)), e = _mm_loadu_si128((const __m128i*)(s + 48));
        _mm_stream_si128((__m128i*)d, a); _mm_stream_si128((__m128i*)(d + 16), b); _mm_stream_si128((__m128i*)(d + 32), c); _mm_stream_si128((__m128i*)(d + 48), e); }
    memcpy(d, s, n);
}

struct Arc { std::vector<uint8_t> z; std::vector<uint64_t> off; uint32_t bs, nb, mm; size_t orig; };
static Arc build(const std::vector<uint8_t>& in, int level, uint32_t bs) {
    Arc a; axf::Params P; axf::level_params(level, P); a.bs = bs; a.mm = P.mm; a.orig = in.size();
    a.nb = (uint32_t)((in.size() + bs - 1) / bs); a.off.resize(a.nb + 1);
    std::vector<std::vector<uint8_t>> blocks(a.nb);
    axf::Job j; j.src = in.data(); j.n = in.size(); j.bs = bs; j.nb = a.nb; j.P = P; j.blocks = &blocks; j.next = 0; j.err = 0;
    std::vector<std::thread> th; for (int t = 0; t < 8; t++) th.emplace_back([&] { axf::enc_worker(&j); }); for (auto& x : th) x.join();
    size_t o = 0; for (uint32_t b = 0; b < a.nb; b++) { a.off[b] = o; o += blocks[b].size(); } a.off[a.nb] = o;
    a.z.resize(o); for (uint32_t b = 0; b < a.nb; b++) memcpy(a.z.data() + a.off[b], blocks[b].data(), blocks[b].size());
    return a;
}
static inline uint32_t blen(const Arc& a, uint32_t b) { size_t st = (size_t)b * a.bs; return (uint32_t)std::min<size_t>(a.bs, a.orig - st); }

// stats from the token streams: literal share, match bytes by distance, cold lines (no later match of the block reads them)
static void stats(const Arc& a) {
    uint64_t lit = 0, mb = 0, m32k = 0, m256k = 0, lines = 0, hot = 0, toks = 0;
    for (uint32_t b = 0; b < a.nb; b++) {
        const uint8_t* s = a.z.data() + a.off[b]; const uint32_t ntok = axf_rd32(s), next = axf_rd32(s + 4);
        const uint8_t* tp = s + AXF_BHDR; const uint8_t* ext = tp + 4ull * ntok; const uint8_t* eend = ext + next;
        const uint32_t n = blen(a, b); std::vector<uint8_t> h((n + 63) / 64, 0); uint64_t pos = 0;
        for (uint32_t i = 0; i < ntok; i++) {
            const uint32_t t = axf_rd32(tp + 4ull * i); uint32_t ll = t & 63u, mc = (t >> 6) & 63u; const uint32_t dist = (t >> 12) + AXF_MIN_DIST; int bad = 0;
            if (ll == 63u) ll += axf_ext(&ext, eend, &bad);
            uint32_t ml = mc + a.mm; if (mc == 63u) ml = AXF_COPY + 1 + axf_ext(&ext, eend, &bad);
            pos += ll; lit += ll; mb += ml; if (dist <= 32768) m32k += ml; if (dist <= 262144) m256k += ml;
            const uint64_t s0 = pos - dist; for (uint64_t L = s0 / 64; L <= (s0 + ml - 1) / 64 && L < h.size(); L++) h[L] = 1;
            pos += ml; toks++;
        }
        lit += n - pos; lines += h.size(); for (uint8_t x : h) hot += x;
    }
    printf("stats: %.1f tokens/KiB, literals %.1f %% of output, match bytes at distance <= 32 KiB %.1f %%, <= 256 KiB %.1f %%; "
           "output lines read again by a later match %.1f %% (cold %.1f %%)\n", toks / (a.orig / 1024.0), 100.0 * lit / a.orig,
           mb ? 100.0 * m32k / mb : 0, mb ? 100.0 * m256k / mb : 0, 100.0 * hot / lines, 100.0 - 100.0 * hot / lines);
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s file [level=1] [block_KiB=1024] [reps=7] [ahead=512] [modes=all]\n", argv[0]); return 1; }
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END); size_t n = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> in(n); if (fread(in.data(), 1, n, f) != n) return 1; fclose(f);
    const int level = argc > 2 ? atoi(argv[2]) : 1; const uint32_t bs = (argc > 3 ? (uint32_t)atoi(argv[3]) : 1024u) * 1024u;
    const int R = argc > 4 ? atoi(argv[4]) : 7; const size_t ahead = argc > 5 ? (size_t)atoi(argv[5]) : 512;
    const std::string modes = argc > 6 ? argv[6] : "all";
    auto want = [&](const char* m) { return modes == "all" || ("," + modes + ",").find(std::string(",") + m + ",") != std::string::npos; };
    if (bs > AXF_BLOCK_MAX || bs < 4096 || level < 1 || level > 8) { fprintf(stderr, "level 1..8, block 4..1024 KiB\n"); return 1; }
    Arc a = build(in, level, bs);
    printf("W1 %s: %zu B, level %d (min match %u), %u blocks of %u KiB -> %zu B = %.2f %%; reps %d, CLZERO %s, ahead %zu\n",
           argv[1], n, level, a.mm, a.nb, bs >> 10, a.z.size(), 100.0 * a.z.size() / n, R, has_clzero() ? "yes" : "no", ahead);
    if (want("stats")) stats(a);
    uint8_t* out = (uint8_t*)aligned_alloc(64, (n + 63) & ~(size_t)63); memset(out, 0, n);     // touched once, as lzbench's buffer
    uint8_t* scratch = (uint8_t*)aligned_alloc(64, ((size_t)bs + 63) & ~(size_t)63); memset(scratch, 0, bs);
    // perf counting of the timed calls only: run under `perf stat -D -1 --control fifo:CTL,ACK` with W1_PERF_CTL=CTL and
    // W1_PERF_ACK=ACK; the counters are enabled around each timed call (not the archive build, not the comparison)
    FILE* pctl = getenv("W1_PERF_CTL") ? fopen(getenv("W1_PERF_CTL"), "w") : nullptr;
    FILE* pack = getenv("W1_PERF_ACK") ? fopen(getenv("W1_PERF_ACK"), "r") : nullptr;
    auto perf_cmd = [&](const char* c) { if (!pctl) return; fputs(c, pctl); fputc('\n', pctl); fflush(pctl);
                                         if (pack) { char buf[16]; if (!fgets(buf, sizeof buf, pack)) {} } };
    auto run = [&](const char* name, auto&& body, size_t bytes, bool check_all) {
        std::vector<double> ts; bool ok = true;
        for (int r = 0; r < R; r++) { if (check_all) memset(out, 0xEE, n);
            perf_cmd("enable"); const double t0 = now(); ok = body() && ok; ts.push_back(now() - t0); perf_cmd("disable");
            if (check_all) ok = ok && memcmp(out, in.data(), n) == 0; }
        std::sort(ts.begin(), ts.end());
        printf("%-36s %8.0f MB/s  (median of %d, best %.0f)  %s  [counted bytes of output: %zu]\n", name, bytes / ts[R / 2] / 1e6, R, bytes / ts[0] / 1e6, ok ? "bit-perfect" : "MISMATCH", bytes * (size_t)R);
        return ok;
    };
    auto all_blocks = [&](auto dec) { for (uint32_t b = 0; b < a.nb; b++) if (!dec(a.z.data() + a.off[b], a.off[b + 1] - a.off[b], out + (size_t)b * bs, blen(a, b))) return false; return true; };
    bool all = true;
    if (want("memcpy")) all &= run("memcpy (DRAM)", [&] { memcpy(out, in.data(), n); return true; }, n, true);
    if (want("base")) all &= run("base: decode -> DRAM buffer", [&] { return all_blocks([&](const uint8_t* s, size_t z, uint8_t* o, uint32_t k) { return dec_block<P_NONE>(s, z, o, k, a.mm, 0); }); }, n, true);
    if (want("E0")) {
        const uint32_t kb = std::max<uint32_t>(1, std::min<uint32_t>(a.nb, (8u << 20) / bs)); size_t sb = 0; for (uint32_t b = 0; b < kb; b++) sb += blen(a, b);
        all &= run("E0: decode -> cache-resident 8 MiB", [&] { for (int rep = 0; rep < 16; rep++) for (uint32_t b = 0; b < kb; b++) if (!dec_block<P_NONE>(a.z.data() + a.off[b], a.off[b + 1] - a.off[b], out + (size_t)b * bs, blen(a, b), a.mm, 0)) return false;
                                                         return memcmp(out, in.data(), sb) == 0; }, 16 * sb, false);
    }
    if (want("pfw")) all &= run("pfw: PREFETCHW ahead -> DRAM", [&] { return all_blocks([&](const uint8_t* s, size_t z, uint8_t* o, uint32_t k) { return dec_block<P_PFW>(s, z, o, k, a.mm, ahead); }); }, n, true);
    if (want("clz")) {
        if (has_clzero()) all &= run("clz: CLZERO ahead -> DRAM", [&] { return all_blocks([&](const uint8_t* s, size_t z, uint8_t* o, uint32_t k) { return dec_block<P_CLZ>(s, z, o, k, a.mm, ahead); }); }, n, true);
        else printf("%-36s skipped: no CLZERO on this CPU\n", "clz: CLZERO ahead -> DRAM");
    }
    if (want("nt")) all &= run("nt: scratch + NT stream -> DRAM", [&] { for (uint32_t b = 0; b < a.nb; b++) { const uint32_t k = blen(a, b);
                                                          if (!dec_block<P_NONE>(a.z.data() + a.off[b], a.off[b + 1] - a.off[b], scratch, k, a.mm, 0)) return false;
                                                          nt_copy(out + (size_t)b * bs, scratch, k); } _mm_sfence(); return true; }, n, true);
    if (want("x2")) all &= run("x2: two blocks interleaved -> DRAM", [&] {
        uint32_t b = 0;
        for (; b + 1 < a.nb; b += 2) {
            St p, q; if (!p.open(a.z.data() + a.off[b], a.off[b + 1] - a.off[b], out + (size_t)b * bs, blen(a, b), a.mm)) return false;
            if (!q.open(a.z.data() + a.off[b + 1], a.off[b + 2] - a.off[b + 1], out + (size_t)(b + 1) * bs, blen(a, b + 1), a.mm)) return false;
            bool mp = true, mq = true;
            while (mp || mq) { if (mp) mp = p.step<P_NONE>(16, 0); if (mq) mq = q.step<P_NONE>(16, 0); if (p.bad || q.bad) return false; }
            if (!p.finish() || !q.finish()) return false;
        }
        for (; b < a.nb; b++) if (!dec_block<P_NONE>(a.z.data() + a.off[b], a.off[b + 1] - a.off[b], out + (size_t)b * bs, blen(a, b), a.mm, 0)) return false;
        return true; }, n, true);
    printf("W1 %s\n", all ? "all runs bit-perfect" : "MISMATCH");
    free(out); free(scratch); return all ? 0 : 1;
}
