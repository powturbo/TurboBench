// v3proto.cpp - P1 of docs/ARCH_V3.md: the same matches (the parse of an ACEPX2 archive) decoded
// from two token layouts in one harness, with the same copy functions, so only the parse differs.
//
//   ACEPX2  commands as stored: cmd stream (literal runs <= 128, matches, rep codes, reset),
//           varint lengths and offsets in two side streams
//   v3      one fixed-width token per sequence "literals + match": SoA planes
//           litlen u16 | mlen u16 | dist u32 (mlen 0 = literals only; long runs split into tokens)
//
// Input: streams.bin written by `ACEAPEX_DUMP=1 aceapex d` (header, block table, decoded
// lit/off/len/cmd) and the original file (bit-perfect check of both decoders).
// Measured: (1) token+match phase, 1 thread and N threads, both layouts; (2) entropy cost of the
// token streams: ACEPX2 off/len/cmd and v3 planes, each compressed with zstd -1 in 512 KiB chunks
// (the ACEPX2 default), compressed size and decompression time; (3) the literal stream is common.
// Build: g++ -O3 -march=native -std=c++17 -o v3proto v3proto.cpp -lzstd -lpthread
// Usage: v3proto streams.bin original [threads=8] [reps=7]
#include <zstd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <immintrin.h>

static std::vector<uint8_t> slurp(const char* p) {
    FILE* f = fopen(p, "rb"); if (!f) { perror(p); exit(1); }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> v((size_t)n + 64); if (fread(v.data(), 1, n, f) != (size_t)n) exit(1); fclose(f); v.resize(n); return v;
}
static inline uint64_t rd64(const uint8_t* p) { uint64_t v; memcpy(&v, p, 8); return v; }
static inline uint32_t rd32(const uint8_t* p) { uint32_t v; memcpy(&v, p, 4); return v; }
static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
struct BO { uint64_t lit_off, off_off, len_off, cmd_off, lit_sz, off_sz, len_sz, cmd_sz; };

// ---- shared copy primitives (both decoders use exactly these)
static inline void copy16(uint8_t* d, const uint8_t* s) { _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((const __m128i*)s)); }
static inline void wild_copy(uint8_t* d, const uint8_t* s, uint32_t n) { for (uint32_t i = 0; i < n; i += 16) copy16(d + i, s + i); }
static inline void match_copy(uint8_t* d, uint32_t dist, uint32_t n) {
    const uint8_t* s = d - dist;
    if (dist >= 16) { for (uint32_t i = 0; i < n; i += 16) copy16(d + i, s + i); return; }
    for (uint32_t i = 0; i < n; i++) d[i] = s[i];
}
static inline void match_copy_exact(uint8_t* d, uint32_t dist, uint32_t n) { const uint8_t* s = d - dist; for (uint32_t i = 0; i < n; i++) d[i] = s[i]; }

static inline uint32_t varint(const uint8_t* b, size_t& p, size_t lim) {
    uint32_t v = 0, sh = 0;
    while (p < lim && sh <= 28) { uint8_t c = b[p++]; v |= (uint32_t)(c & 0x7F) << sh; if (!(c & 0x80)) return v; sh += 7; }
    return v;
}

// ---- ACEPX2 command decoder (semantics of src/aceapex_main.cpp decompress_streams), fast zone + tail
static void dec_acepx2(uint8_t* dst, size_t dsz, const uint8_t* lit, size_t lsz, const uint8_t* off, size_t osz,
                       const uint8_t* len, size_t nsz, const uint8_t* cmd, size_t csz) {
    size_t lp = 0, op = 0, np = 0, cp = 0, out = 0; uint32_t rep[4] = {1, 2, 4, 8};
    const size_t FZ = 128 + 32;
    while (cp < csz && out < dsz) {
        uint8_t c = cmd[cp++];
        bool fast = out + FZ <= dsz && lp + FZ <= lsz;
        if (c < 0x80) { uint32_t l = c + 1; if (lp + l > lsz || out + l > dsz) return;
            if (fast) wild_copy(dst + out, lit + lp, l); else memcpy(dst + out, lit + lp, l); out += l; lp += l; continue; }
        if (c == 0xFF) { rep[0] = 1; rep[1] = 2; rep[2] = 4; rep[3] = 8; continue; }
        uint32_t l, dist;
        if ((c & 0xC0) == 0x80) { uint32_t ri = (c >> 4) & 3, lv = c & 0x0F; if (lv == 0x0F) lv += varint(len, np, nsz);
            l = lv + 6; dist = rep[ri]; if (ri > 0) { for (int i = ri; i > 0; i--) rep[i] = rep[i - 1]; rep[0] = dist; } }
        else { uint32_t lv = (c == 0xFE) ? varint(len, np, nsz) : (uint32_t)(c & 0x3F); l = lv + 6; dist = varint(off, op, osz);
            rep[3] = rep[2]; rep[2] = rep[1]; rep[1] = rep[0]; rep[0] = dist; }
        if (!dist || dist > out || out + l > dsz) return;
        if (out + l + 32 <= dsz) match_copy(dst + out, dist, l); else match_copy_exact(dst + out, dist, l);
        out += l;
    }
}

// ---- v3: one token per sequence; planes per block
struct Planes { std::vector<uint16_t> ll, ml; std::vector<uint32_t> d; std::vector<uint32_t> tok0; };
static void dec_v3(uint8_t* dst, size_t dsz, const uint8_t* lit, size_t lsz,
                   const uint16_t* LL, const uint16_t* ML, const uint32_t* D, size_t nt) {
    size_t lp = 0, out = 0;
    for (size_t i = 0; i < nt; i++) {
        uint32_t ll = LL[i], ml = ML[i], d = D[i];
        if (out + ll + ml + 32 <= dsz && lp + ll + 32 <= lsz) {          // fast: wide copies, no length checks inside
            wild_copy(dst + out, lit + lp, ll); out += ll; lp += ll;
            if (ml) { if (d > out) return; match_copy(dst + out, d, ml); out += ml; }
            continue;
        }
        if (lp + ll > lsz || out + ll > dsz) return;
        memcpy(dst + out, lit + lp, ll); out += ll; lp += ll;
        if (ml) { if (!d || d > out || out + ml > dsz) return; match_copy_exact(dst + out, d, ml); out += ml; }
    }
}

// ACEPX2 commands of one block -> v3 tokens (literal runs merged into the next match)
static void to_v3(const uint8_t* off, size_t osz, const uint8_t* len, size_t nsz, const uint8_t* cmd, size_t csz, Planes& P) {
    size_t op = 0, np = 0, cp = 0; uint32_t rep[4] = {1, 2, 4, 8}; uint32_t pend = 0;
    auto emit = [&](uint32_t ll, uint32_t ml, uint32_t d) {
        while (ll > 65535) { P.ll.push_back(65535); P.ml.push_back(0); P.d.push_back(0); ll -= 65535; }
        uint32_t first = ml > 65535 ? 65535 : ml;
        P.ll.push_back((uint16_t)ll); P.ml.push_back((uint16_t)first); P.d.push_back(ml ? d : 0); ml -= first;
        while (ml) { uint32_t k = ml > 65535 ? 65535 : ml; P.ll.push_back(0); P.ml.push_back((uint16_t)k); P.d.push_back(d); ml -= k; }
    };
    while (cp < csz) {
        uint8_t c = cmd[cp++];
        if (c < 0x80) { pend += c + 1; continue; }
        if (c == 0xFF) { rep[0] = 1; rep[1] = 2; rep[2] = 4; rep[3] = 8; continue; }
        uint32_t l, dist;
        if ((c & 0xC0) == 0x80) { uint32_t ri = (c >> 4) & 3, lv = c & 0x0F; if (lv == 0x0F) lv += varint(len, np, nsz);
            l = lv + 6; dist = rep[ri]; if (ri > 0) { for (int i = ri; i > 0; i--) rep[i] = rep[i - 1]; rep[0] = dist; } }
        else { uint32_t lv = (c == 0xFE) ? varint(len, np, nsz) : (uint32_t)(c & 0x3F); l = lv + 6; dist = varint(off, op, osz);
            rep[3] = rep[2]; rep[2] = rep[1]; rep[1] = rep[0]; rep[0] = dist; }
        emit(pend, l, dist); pend = 0;
    }
    if (pend) emit(pend, 0, 0);
}

// zstd -1 in 512 KiB chunks: compressed size and median decompression time
static void zcost(const uint8_t* p, size_t n, size_t& csz, double& dsec, int reps) {
    const size_t CH = 524288; std::vector<std::vector<uint8_t>> fr; csz = 0;
    for (size_t o = 0; o < n; o += CH) { size_t k = std::min(CH, n - o); std::vector<uint8_t> b(ZSTD_compressBound(k));
        size_t r = ZSTD_compress(b.data(), b.size(), p + o, k, 1); b.resize(r); csz += r; fr.push_back(std::move(b)); }
    std::vector<uint8_t> out(CH); std::vector<double> t;
    for (int r = 0; r < reps; r++) { double t0 = now(); for (auto& f : fr) ZSTD_decompress(out.data(), CH, f.data(), f.size()); t.push_back(now() - t0); }
    std::sort(t.begin(), t.end()); dsec = t[t.size() / 2];
}

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s streams.bin original [threads=8] [reps=7]\n", argv[0]); return 1; }
    int T = argc > 3 ? atoi(argv[3]) : 8, R = argc > 4 ? atoi(argv[4]) : 7;
    std::vector<uint8_t> s = slurp(argv[1]), orig = slurp(argv[2]);
    uint64_t osz = rd64(&s[12]); uint32_t bs = rd32(&s[20]), nb = rd32(&s[24]);
    uint64_t zl = rd64(&s[36]), zo = rd64(&s[44]), zn = rd64(&s[52]), zc = rd64(&s[60]);
    if (osz != orig.size()) { fprintf(stderr, "size mismatch\n"); return 1; }
    std::vector<BO> bo(nb); memcpy(bo.data(), &s[68], sizeof(BO) * nb);
    uint64_t L = 0, O = 0, N = 0, C = 0;
    for (auto& b : bo) { L = std::max(L, b.lit_off + b.lit_sz); O = std::max(O, b.off_off + b.off_sz); N = std::max(N, b.len_off + b.len_sz); C = std::max(C, b.cmd_off + b.cmd_sz); }
    const uint8_t* lit = &s[68 + sizeof(BO) * nb]; const uint8_t* off = lit + L; const uint8_t* len = off + O; const uint8_t* cmd = len + N;
    if ((size_t)(cmd + C - s.data()) > s.size()) { fprintf(stderr, "streams.bin truncated\n"); return 1; }
    // literal stream padded copy (wild copies read up to 32 B past a block's literal slice)
    std::vector<uint8_t> litp(L + 64, 0); memcpy(litp.data(), lit, L);

    // v3 planes per block (+ token start per block)
    Planes P; P.tok0.resize(nb + 1);
    for (uint32_t b = 0; b < nb; b++) { P.tok0[b] = (uint32_t)P.ll.size(); to_v3(off + bo[b].off_off, bo[b].off_sz, len + bo[b].len_off, bo[b].len_sz, cmd + bo[b].cmd_off, bo[b].cmd_sz, P); }
    P.tok0[nb] = (uint32_t)P.ll.size(); size_t NT = P.ll.size();

    std::vector<uint8_t> dst(osz + 64);
    auto run = [&](int which, int threads) {
        std::atomic<uint32_t> next{0};
        auto worker = [&]() { for (;;) { uint32_t b = next.fetch_add(1); if (b >= nb) break;
            size_t o = (size_t)b * bs, dsz = std::min<uint64_t>(bs, osz - o); const BO& x = bo[b];
            if (which == 0) dec_acepx2(dst.data() + o, dsz, litp.data() + x.lit_off, x.lit_sz, off + x.off_off, x.off_sz, len + x.len_off, x.len_sz, cmd + x.cmd_off, x.cmd_sz);
            else dec_v3(dst.data() + o, dsz, litp.data() + x.lit_off, x.lit_sz, P.ll.data() + P.tok0[b], P.ml.data() + P.tok0[b], P.d.data() + P.tok0[b], P.tok0[b + 1] - P.tok0[b]); } };
        if (threads <= 1) { worker(); return; }
        std::vector<std::thread> th; for (int t = 0; t < threads; t++) th.emplace_back(worker); for (auto& t : th) t.join();
    };
    auto bench = [&](int which, int threads) {
        std::vector<double> t; for (int r = 0; r < R; r++) { memset(dst.data(), 0, osz); double t0 = now(); run(which, threads); t.push_back(now() - t0); }
        bool ok = memcmp(dst.data(), orig.data(), osz) == 0; std::sort(t.begin(), t.end()); return std::make_pair(t[t.size() / 2], ok);
    };
    printf("%s: %llu B, %u blocks of %u, %zu v3 tokens (%.2f per block), literals %llu B\n", argv[2], (unsigned long long)osz, nb, bs, NT, (double)NT / nb, (unsigned long long)L);
    for (int th : {1, T}) {
        auto a = bench(0, th), v = bench(1, th);
        printf("match phase, %2d thread(s): ACEPX2 %.4f s (%.0f MB/s, %s) | v3 %.4f s (%.0f MB/s, %s) | x%.2f\n", th,
               a.first, osz / a.first / 1e6, a.second ? "bit-perfect" : "MISMATCH", v.first, osz / v.first / 1e6, v.second ? "bit-perfect" : "MISMATCH", a.first / v.first);
    }
    // entropy cost of the token streams (zstd -1, 512 KiB chunks, 1 thread)
    std::vector<uint8_t> acat; acat.insert(acat.end(), off, off + O); acat.insert(acat.end(), len, len + N); acat.insert(acat.end(), cmd, cmd + C);
    size_t ac, vc_ll, vc_ml, vc_d, lc; double ad, vd1, vd2, vd3, ld;
    zcost(acat.data(), acat.size(), ac, ad, R);
    zcost((const uint8_t*)P.ll.data(), NT * 2, vc_ll, vd1, R); zcost((const uint8_t*)P.ml.data(), NT * 2, vc_ml, vd2, R); zcost((const uint8_t*)P.d.data(), NT * 4, vc_d, vd3, R);
    zcost(lit, L, lc, ld, R);
    size_t vc = vc_ll + vc_ml + vc_d; double vd = vd1 + vd2 + vd3;
    printf("archive token streams as stored: %llu B (off %llu + len %llu + cmd %llu), literals %llu B, total archive payload %llu B\n",
           (unsigned long long)(zo + zn + zc), (unsigned long long)zo, (unsigned long long)zn, (unsigned long long)zc, (unsigned long long)zl, (unsigned long long)(zl + zo + zn + zc));
    printf("tokens, zstd -1 / 512 KiB, same method both: ACEPX2 %zu B, decode %.4f s | v3 planes %zu B (ll %zu ml %zu d %zu), decode %.4f s\n", ac, ad, vc, vc_ll, vc_ml, vc_d, vd);
    printf("literals zstd -1 / 512 KiB: %zu B, decode %.4f s (common to both)\n", lc, ld);
    double s_a = (double)(zl + ac), s_v = (double)(zl + vc);   // literal stream as stored + tokens by the same method
    printf("size estimate (stored literals + tokens zstd -1): ACEPX2 %.0f B = %.2f %% | v3 %.0f B = %.2f %% (%+.2f pp)\n", s_a, 100 * s_a / osz, s_v, 100 * s_v / osz, 100 * (s_v - s_a) / osz);
    auto a1 = bench(0, 1), v1 = bench(1, 1);
    double t_a = ld + ad + a1.first, t_v = ld + vd + v1.first;
    printf("1-thread total estimate (literal entropy + token entropy + match): ACEPX2 %.4f s = %.0f MB/s | v3 %.4f s = %.0f MB/s\n", t_a, osz / t_a / 1e6, t_v, osz / t_v / 1e6);
    return 0;
}
