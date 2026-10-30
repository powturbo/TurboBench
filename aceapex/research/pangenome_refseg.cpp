// pangenome_refseg.cpp - research (not a format; tuning builds only): one FASTA holding several HPRC assemblies
// (records named <sample>#<hap>#..., an assembly = consecutive records with the same <sample>#<hap>#) compressed as
// one container in the open literal profile, either plainly (AX_REFSEG=0: every 1 MiB block alone) or with reference
// segments (AX_REFSEG=1: a block of a later assembly may copy from up to 3 blocks of an earlier one, forward or
// reverse-complemented; scripts/refseg.cpp). Depth <= 2: only assemblies that are themselves at depth <= 1 (the
// first, and ones whose blocks refer only to it) are indexed as references. Decoded from the container alone and
// compared byte for byte with the input. Prints the size of every assembly's part (what each next one costs).
//
// AX_KMER=1 adds research I1 - canonical 31-mers (case folded, no N, inside one record) sampled 1/64 by hash:
//   naive : every window of the decoded sequence;
//   tokens: only windows not inside one match whose source window lies inside one record (those k-mers are copies of
//           k-mers counted before: in the same assembly earlier, or in the reference assembly, forward or reverse
//           complement - canonical k-mers do not see the orientation);
//   the two sets must be identical; reports the windows visited and the time of each.
// Build: g++ -std=c++17 -O3 -march=native -DACEAPEX_ENV_TUNING -Isrc research/pangenome_refseg.cpp -lzstd -lpthread
// Usage: [AX_REFSEG=1] [AX_KMER=1] pangenome_refseg <out.axr> <assemblies.fa>
// Last line: PGROW <tab> refs assemblies fasta_bytes container_bytes part1,part2,... enc_s dec_s dec_GBps ok [kmer fields]
#include "../src/aceapex_api.cpp"
#include <zstd.h>
#include <unordered_map>
#include <string>
#include <thread>
#include <numeric>
#include <mutex>

namespace pg {
static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static void put_v(std::vector<uint8_t>& o, uint64_t x) { while (x >= 0x80) { o.push_back((uint8_t)(x | 0x80)); x >>= 7; } o.push_back((uint8_t)x); }
static uint64_t get_v(const uint8_t*& p) { uint64_t v = 0; int s = 0; for (;;) { const uint8_t c = *p++; v |= (uint64_t)(c & 0x7F) << s; if (!(c & 0x80)) return v; s += 7; } }
static void put64(std::vector<uint8_t>& o, uint64_t x) { for (int i = 0; i < 8; i++) o.push_back((uint8_t)(x >> (8 * i))); }
static uint64_t get64(const uint8_t*& p) { uint64_t v; memcpy(&v, p, 8); p += 8; return v; }
// FASTA -> line model + sequence; record starts (sequence offsets) too
static void split(const uint8_t* f, size_t n, std::vector<uint8_t>& lm, std::vector<uint8_t>& seq) {
    seq.reserve(n + 64); size_t i = 0; uint64_t rl = 0, rc = 0;
    auto flush = [&] { if (rc) { lm.push_back(1); put_v(lm, rl); put_v(lm, rc); rc = 0; } };
    while (i < n) { const uint8_t* nl = (const uint8_t*)memchr(f + i, '\n', n - i); const size_t e = nl ? (size_t)(nl - f) : n;
        if (f[i] == '>') { flush(); lm.push_back(0); put_v(lm, e - i); lm.insert(lm.end(), f + i, f + e); }
        else { const uint64_t L = e - i; if (rc && L == rl) rc++; else { flush(); rl = L; rc = 1; } seq.insert(seq.end(), f + i, f + e); }
        if (e == n) { flush(); lm.push_back(2); break; } i = e + 1; }
    flush(); lm.push_back(3);
}
static std::vector<uint8_t> join(const std::vector<uint8_t>& lm, const uint8_t* seq, size_t seq_n) {
    std::vector<uint8_t> f; f.reserve(seq_n + seq_n / 50 + 1024); const uint8_t* p = lm.data(); size_t sp = 0; bool un = false;
    for (;;) { const uint8_t t = *p++;
        if (t == 0) { const uint64_t L = get_v(p); f.insert(f.end(), p, p + L); p += L; f.push_back('\n'); }
        else if (t == 1) { const uint64_t L = get_v(p), C = get_v(p); for (uint64_t c = 0; c < C && sp + L <= seq_n; c++) { f.insert(f.end(), seq + sp, seq + sp + L); sp += L; f.push_back('\n'); } }
        else if (t == 2) un = true; else break; }
    if (un && !f.empty()) f.pop_back(); return f;
}
static std::vector<uint64_t> record_starts(const std::vector<uint8_t>& lm) {     // sequence offset where each record begins
    std::vector<uint64_t> r; const uint8_t* p = lm.data(); uint64_t sp = 0;
    for (;;) { const uint8_t t = *p++;
        if (t == 0) { const uint64_t L = get_v(p); p += L; r.push_back(sp); }
        else if (t == 1) { const uint64_t L = get_v(p), C = get_v(p); sp += L * C; }
        else if (t == 2) continue; else break; }
    if (r.empty()) r.push_back(0);
    return r;
}
static inline uint8_t comp(uint8_t c) { switch (c) { case 'A': return 'T'; case 'C': return 'G'; case 'G': return 'C'; case 'T': return 'A';
    case 'a': return 't'; case 'c': return 'g'; case 'g': return 'c'; case 't': return 'a'; default: return c; } }
static std::vector<uint8_t> revcomp(const std::vector<uint8_t>& s) { const size_t n = s.size(); std::vector<uint8_t> r(n + 64, 0); r.resize(n);
    const int T = 16; std::vector<std::thread> th; for (int t = 0; t < T; t++) th.emplace_back([&, t] { for (size_t i = n * t / T; i < n * (t + 1) / T; i++) r[n - 1 - i] = comp(s[i]); });
    for (auto& x : th) x.join(); return r; }
static inline int b2(uint8_t c) { switch (c | 0x20) { case 'a': return 0; case 'c': return 1; case 'g': return 2; case 't': return 3; default: return -1; } }
static inline uint64_t mix(uint64_t x) { x ^= x >> 33; x *= 0xff51afd7ed558ccdull; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ull; x ^= x >> 33; return x; }
static const int K = 20; static const uint64_t KM = (1ull << (2 * K)) - 1; static const uint64_t SAMP = 64;
struct Ent { uint64_t code; uint64_t pos; };
template <class F> static void kmers(const uint8_t* s, size_t n, F f) {
    uint64_t c = 0; int run = 0;
    for (size_t i = 0; i < n; i++) { const int x = b2(s[i]); if (x < 0) { run = 0; continue; } c = ((c << 2) | (uint64_t)x) & KM; if (++run >= K) f(i + 1 - K, c); }
}
static inline uint64_t rc_code(uint64_t c) { uint64_t r = 0; for (int i = 0; i < K; i++) { r = (r << 2) | (3 - (c & 3)); c >>= 2; } return r; }
struct Asm { std::string name; const uint8_t* fa = nullptr; size_t fa_n = 0; std::vector<uint8_t> lm, lmz, seq, rcs; int level = 0; };
struct Ref { uint8_t asmi = 0xFF, rc = 0; uint32_t rb0 = 0; uint8_t nrb = 0; uint32_t votes = 0; };
struct Prov { const std::vector<Asm>* A; const std::vector<Ref>* refs; size_t bs; };
static size_t provide(void* ctx, size_t b, const uint8_t** pre) {
    const Prov& P = *(const Prov*)ctx; const Ref& r = (*P.refs)[b]; if (r.asmi == 0xFF) return 0;
    const Asm& a = (*P.A)[r.asmi]; const size_t n = a.seq.size(), lo = (size_t)r.rb0 * P.bs, hi = std::min(n, lo + (size_t)r.nrb * P.bs);
    if (!r.rc) { *pre = a.seq.data() + lo; return hi - lo; }
    *pre = a.rcs.data() + (n - hi); return hi - lo;
}
// I1: canonical 31-mers
static const int KK = 31; static const uint64_t KKM = (1ull << (2 * KK)) - 1;
static inline uint64_t canon31(uint64_t f, uint64_t r) { return f < r ? f : r; }
struct KMatch { uint64_t s, e; int src_asm; int64_t src; bool rc; };   // assembly coords [s,e); source window start for a window at s (fwd)
}  // namespace pg

int main(int argc, char** argv) {
    using namespace pg;
    if (argc < 3) { fprintf(stderr, "usage: %s <out.axr> <assemblies.fa>\n", argv[0]); return 1; }
    const bool refs_on = ax_getenv("AX_REFSEG") && atoi(ax_getenv("AX_REFSEG"));
    const bool kmer_on = ax_getenv("AX_KMER") && atoi(ax_getenv("AX_KMER"));
    setenv("ACEAPEX_BS", "1048576", 1); setenv("AX_PROFILE", "open", 1); setenv("AX_HASH12", "1", 1); setenv("AX_HLOG", "22", 1);
    const size_t BS = 1048576; const int T = (int)std::thread::hardware_concurrency();
    std::vector<uint8_t> F; { FILE* f = fopen(argv[2], "rb"); if (!f) { perror(argv[2]); return 1; } fseek(f, 0, SEEK_END); F.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET);
        if (fread(F.data(), 1, F.size(), f) != F.size()) return 1; fclose(f); }
    // assemblies: consecutive records with the same "<sample>#<hap>#" (no '#': one assembly)
    std::vector<Asm> A; { size_t i = 0; std::string cur; size_t start = 0;
        while (i < F.size()) { if (F[i] == '>' && (i == 0 || F[i - 1] == '\n')) { size_t e = i; while (e < F.size() && F[e] != '\n') e++;
                std::string h((const char*)&F[i + 1], e - i - 1); size_t a = h.find('#'), b = a == std::string::npos ? a : h.find('#', a + 1);
                std::string key = b == std::string::npos ? "all" : h.substr(0, b);
                if (A.empty() || key != cur) { if (!A.empty()) A.back().fa_n = i - start; A.emplace_back(); A.back().name = key; A.back().fa = F.data() + i; start = i; cur = key; } i = e; }
            else { const uint8_t* nl = (const uint8_t*)memchr(F.data() + i, '\n', F.size() - i); i = nl ? (size_t)(nl - F.data()) + 1 : F.size(); } }
        if (!A.empty()) A.back().fa_n = F.size() - start;
        if (A.empty() || A[0].fa != F.data()) { fprintf(stderr, "input must start with a header\n"); return 1; } }
    for (auto& a : A) { split(a.fa, a.fa_n, a.lm, a.seq); a.lmz.resize(ZSTD_compressBound(a.lm.size())); a.lmz.resize(ZSTD_compress(a.lmz.data(), a.lmz.size(), a.lm.data(), a.lm.size(), 19)); a.seq.reserve(a.seq.size() + 64); }
    fprintf(stderr, "[pg] %zu assemblies in %s (%zu B)\n", A.size(), argv[2], F.size());
    std::vector<uint8_t> out; out.insert(out.end(), {'A', 'X', 'R', 'E', 'F', 'S', 'G', '2'}); put64(out, A.size());
    std::vector<std::vector<Ref>> R(A.size()); std::vector<uint64_t> part(A.size());
    std::vector<Ent> idx; double t_enc0 = now(), t_idx = 0;
    for (size_t j = 0; j < A.size(); j++) {
        const size_t nb = (A[j].seq.size() + BS - 1) / BS; R[j].assign(nb, Ref());
        double t0 = now();
        if (refs_on && j > 0 && !idx.empty()) {
            std::atomic<size_t> next{0}; std::vector<std::thread> th;
            for (int t = 0; t < T; t++) th.emplace_back([&] { std::unordered_map<uint64_t, uint32_t> v;
                for (size_t b; (b = next++) < nb; ) { v.clear(); const size_t lo = b * BS, n = std::min(BS, A[j].seq.size() - lo);
                    kmers(A[j].seq.data() + lo, n, [&](size_t t, uint64_t c) { for (int o = 0; o < 2; o++) { const uint64_t key = o ? rc_code(c) : c; if (mix(key) % SAMP) continue;
                        auto it = std::lower_bound(idx.begin(), idx.end(), key, [](const Ent& e, uint64_t k) { return e.code < k; }); if (it == idx.end() || it->code != key) continue;
                        const uint64_t p = it->pos & ((1ull << 40) - 1), as = it->pos >> 40; const int64_t d = o ? (int64_t)(p + t + K - 1) : (int64_t)p - (int64_t)t; if (d < 0) continue;
                        v[(as << 56) | ((uint64_t)o << 55) | ((uint64_t)d >> 16)]++; } });
                    uint64_t best = 0; uint32_t bv = 0; for (auto& kv : v) if (kv.second > bv) { bv = kv.second; best = kv.first; }
                    if (bv < 16) continue;
                    const size_t as = best >> 56; const int o = (best >> 55) & 1; const int64_t d = (int64_t)((best & ((1ull << 55) - 1)) << 16);
                    const int64_t M = 256 * 1024, rn = (int64_t)A[as].seq.size(); int64_t lo2 = o ? d - (int64_t)BS - M : d - M, hi2 = o ? d + 65536 + M : d + 65536 + (int64_t)BS + M;
                    lo2 = std::max<int64_t>(0, lo2); hi2 = std::min<int64_t>(rn, hi2); if (hi2 <= lo2) continue;
                    size_t rb0 = (size_t)lo2 / BS, rb1 = ((size_t)hi2 + BS - 1) / BS; if (rb1 - rb0 > 3) { const size_t c = (rb0 + rb1) / 2; rb0 = c > 1 ? c - 1 : 0; rb1 = rb0 + 3; }
                    R[j][b] = Ref{(uint8_t)as, (uint8_t)o, (uint32_t)rb0, (uint8_t)(rb1 - rb0), bv}; } });
            for (auto& x : th) x.join();
        }
        int lv = 0; for (const Ref& r : R[j]) if (r.asmi != 0xFF) lv = std::max(lv, A[r.asmi].level + 1); A[j].level = lv;
        t_idx += now() - t0;
        Prov P{&A, &R[j], BS}; g_ax_ref_fn = (refs_on && j > 0) ? provide : nullptr; g_ax_ref_ctx = &P;
        std::vector<uint8_t> z(aceapex_compress_bound(A[j].seq.size()));
        const int64_t zs = aceapex_compress(A[j].seq.data(), A[j].seq.size(), z.data(), z.size(), 2, T); g_ax_ref_fn = nullptr;
        if (zs <= 0) { fprintf(stderr, "compress failed\n"); return 2; }
        z.resize((size_t)zs); const size_t at = out.size();
        put64(out, A[j].name.size()); out.insert(out.end(), A[j].name.begin(), A[j].name.end()); put64(out, A[j].fa_n); put64(out, A[j].seq.size());
        put64(out, A[j].lmz.size()); out.insert(out.end(), A[j].lmz.begin(), A[j].lmz.end());
        put64(out, nb); for (const Ref& r : R[j]) { out.push_back(r.asmi); out.push_back(r.rc); put64(out, r.rb0); out.push_back(r.nrb); }
        put64(out, z.size()); out.insert(out.end(), z.begin(), z.end()); part[j] = out.size() - at;
        size_t nref = 0; for (const Ref& r : R[j]) nref += r.asmi != 0xFF;
        fprintf(stderr, "[pg] %s: %zu blocks, %zu with a reference, depth %d, part %llu B\n", A[j].name.c_str(), nb, nref, A[j].level, (unsigned long long)part[j]);
        if (refs_on && A[j].level <= 1 && j + 1 < A.size()) {           // indexed as a reference only at depth <= 1 (depth of any block <= 2)
            A[j].rcs = revcomp(A[j].seq);
            std::vector<Ent> add; kmers(A[j].seq.data(), A[j].seq.size(), [&](size_t p, uint64_t c) { if (!(mix(c) % SAMP)) add.push_back({c, p | ((uint64_t)j << 40)}); });
            idx.insert(idx.end(), add.begin(), add.end()); std::sort(idx.begin(), idx.end(), [](const Ent& a, const Ent& b) { return a.code < b.code || (a.code == b.code && a.pos < b.pos); });
            std::vector<Ent> u; u.reserve(idx.size()); for (size_t i = 0; i < idx.size(); ) { size_t e = i; while (e < idx.size() && idx[e].code == idx[i].code) e++; if (e - i == 1) u.push_back(idx[i]); i = e; }
            idx.swap(u); }
    }
    const double t_enc = now() - t_enc0;
    { FILE* f = fopen(argv[1], "wb"); if (!f || fwrite(out.data(), 1, out.size(), f) != out.size()) { fprintf(stderr, "write failed\n"); return 3; } fclose(f); }
    for (auto& a : A) { std::vector<uint8_t>().swap(a.seq); std::vector<uint8_t>().swap(a.rcs); }
    // ---- decode from the container alone (read back from the file) ----
    std::vector<uint8_t> in; { FILE* f = fopen(argv[1], "rb"); fseek(f, 0, SEEK_END); in.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(in.data(), 1, in.size(), f) != in.size()) return 3; fclose(f); }
    std::vector<uint8_t>().swap(out);
    const double td0 = now(); const uint8_t* p = in.data() + 8; const uint64_t na = get64(p); bool ok = true;
    std::vector<std::vector<uint8_t>> S(na), SR(na), LM(na); std::vector<std::vector<Ref>> DR(na); std::vector<std::vector<uint64_t>> RS(na);
    std::vector<std::vector<KMatch>> MA(na); std::vector<uint64_t> fasta_n(na); double t_join = 0;
    for (uint64_t j = 0; j < na && ok; j++) {
        const uint64_t nl = get64(p); p += nl; fasta_n[j] = get64(p); const uint64_t sn = get64(p);
        const uint64_t lzn = get64(p); LM[j].resize(ZSTD_getFrameContentSize(p, lzn)); ZSTD_decompress(LM[j].data(), LM[j].size(), p, lzn); p += lzn;
        RS[j] = record_starts(LM[j]);
        const uint64_t nb = get64(p); DR[j].resize(nb); for (uint64_t b = 0; b < nb; b++) { Ref& r = DR[j][b]; r.asmi = *p++; r.rc = *p++; r.rb0 = (uint32_t)get64(p); r.nrb = *p++; }
        const uint64_t zn = get64(p); const uint8_t* img = p; p += zn;
        aceapex_streams_t st; if (aceapex_decode_streams(img, zn, &st) || st.orig_size != sn || st.num_blocks != nb) { ok = false; break; }
        S[j].assign(sn + 64, 0); const BlockOffsets* bo = (const BlockOffsets*)st.boffs;
        std::vector<std::vector<KMatch>> bm(kmer_on ? nb : 0);
        std::atomic<size_t> next{0}; std::atomic<int> bad{0}; std::vector<std::thread> th;
        for (int t = 0; t < T; t++) th.emplace_back([&] { std::vector<uint8_t> V;
            for (size_t b; (b = next++) < nb; ) {
                const size_t lo = b * BS, n = std::min<size_t>(BS, sn - lo); const Ref& r = DR[j][b]; size_t pl = 0, lo2 = 0, hi2 = 0;
                if (r.asmi != 0xFF) { if (r.asmi >= j) { bad = 1; continue; }
                    const std::vector<uint8_t>& src = r.rc ? SR[r.asmi] : S[r.asmi]; const size_t rn = src.size();
                    lo2 = (size_t)r.rb0 * BS; hi2 = std::min(rn, lo2 + (size_t)r.nrb * BS); pl = hi2 - lo2; V.resize(pl + n + 64);
                    if (r.rc) memcpy(V.data(), src.data() + (rn - hi2), pl); else memcpy(V.data(), src.data() + lo2, pl); }
                else V.resize(n + 64);
                decompress_streams(V.data(), pl + n, st.lit + bo[b].lit_off, bo[b].lit_sz, st.off + bo[b].off_off, bo[b].off_sz, st.len + bo[b].len_off, bo[b].len_sz, st.cmd + bo[b].cmd_off, bo[b].cmd_sz, pl);
                memcpy(S[j].data() + lo, V.data() + pl, n);
                if (kmer_on) {                                      // the block's matches in assembly coordinates (as decompress_streams parses them)
                    const uint8_t *off = st.off + bo[b].off_off, *len = st.len + bo[b].len_off, *cmd = st.cmd + bo[b].cmd_off;
                    size_t op = 0, np = 0, cp = 0, o = pl; uint32_t rep[4] = {1, 2, 4, 8};
                    while (o < pl + n && cp < bo[b].cmd_sz) { const uint8_t c = cmd[cp++];
                        if (c == 0xFF) { rep[0] = 1; rep[1] = 2; rep[2] = 4; rep[3] = 8; continue; }
                        if (c < 0x80) { o += c + 1u; continue; }
                        uint32_t l, d; bool bad2 = false;
                        if ((c & 0xC0) == 0x80) { uint32_t ri = (c >> 4) & 3, lv = c & 0x0F; if (lv == 0x0F) lv += read_varint(len, np, bo[b].len_sz); l = lv + 6; d = rep[ri]; if (ri) { for (int q = (int)ri; q > 0; q--) rep[q] = rep[q - 1]; rep[0] = d; } }
                        else { const uint32_t lv = c == 0xFE ? read_varint(len, np, bo[b].len_sz) : (uint32_t)(c & 0x3F); l = lv + 6; d = read_varint(off, op, bo[b].off_sz); rep[3] = rep[2]; rep[2] = rep[1]; rep[1] = rep[0]; rep[0] = d; }
                        (void)bad2; const size_t sv = o - d; KMatch m; m.s = lo + (o - pl); m.e = m.s + l;
                        if (sv >= pl) { m.src_asm = (int)j; m.src = (int64_t)(lo + (sv - pl)); m.rc = false; }
                        else if (!r.rc) { m.src_asm = r.asmi; m.src = (int64_t)(lo2 + sv); m.rc = false; }
                        else { m.src_asm = r.asmi; m.src = (int64_t)(hi2 - sv); m.rc = true; }          // window at offset u: ref [src - u - k, src - u)
                        bm[b].push_back(m); o += l; } } } });
        for (auto& x : th) x.join(); aceapex_streams_free(&st);
        if (bad) { ok = false; break; }
        if (kmer_on) for (auto& v : bm) MA[j].insert(MA[j].end(), v.begin(), v.end());
        S[j].resize(sn);
        if (j + 1 < na) SR[j] = revcomp(S[j]);
        const double tj = now(); std::vector<uint8_t> f = join(LM[j], S[j].data(), sn); t_join += now() - tj;
        const bool same = f.size() == A[j].fa_n && !memcmp(f.data(), A[j].fa, f.size()); ok = ok && same;
        fprintf(stderr, "[pg] decode %s: %s\n", A[j].name.c_str(), same ? "FASTA bit-perfect" : "DIFFERS");
    }
    const double t_dec = now() - td0;
    uint64_t seq_tot = 0; for (auto& s2 : S) seq_tot += s2.size();
    std::string parts; for (size_t j = 0; j < A.size(); j++) parts += (j ? "," : "") + std::to_string(part[j]);
    fprintf(stderr, "[pg] refs %d: %zu B FASTA, %zu assemblies -> %zu B (ratio %.3f); encode %.1f s (reference search %.1f s); decode %.1f s (FASTA rebuild %.1f s) = %.2f GB/s; %s\n",
            refs_on, F.size(), A.size(), in.size(), (double)F.size() / in.size(), t_enc, t_idx, t_dec, t_join, F.size() / t_dec / 1e9, ok ? "all bit-perfect" : "FAILED");
    printf("PGROW\t%d\t%zu\t%zu\t%zu\t%s\t%.1f\t%.1f\t%.2f\t%s", refs_on, A.size(), F.size(), in.size(), parts.c_str(), t_enc, t_dec, F.size() / t_dec / 1e9, ok ? "ok" : "FAILED");
    if (kmer_on && ok) {
        // I1: naive (every window) against tokens (windows not inside one match with a one-record source)
        auto rec_of = [&](int a, int64_t x) { const auto& r = RS[a]; return (size_t)(std::upper_bound(r.begin(), r.end(), (uint64_t)x) - r.begin()); };
        auto count = [&](bool tokens, uint64_t& visited, uint64_t& covered) {
            std::vector<uint64_t> all; std::mutex mu; std::atomic<uint64_t> vis{0}, cov{0};
            for (uint64_t j = 0; j < na; j++) {
                // skip intervals of window starts (assembly coords), sorted
                std::vector<std::pair<uint64_t, uint64_t>> skip;
                if (tokens) for (const KMatch& m : MA[j]) { if (m.e - m.s < (uint64_t)KK) continue;
                    const uint64_t u_hi = m.e - m.s - KK;                                    // window offsets u in [0, u_hi]
                    for (uint64_t u = 0; u <= u_hi; ) {                                     // split by the source's records
                        int64_t s0 = m.rc ? m.src - (int64_t)u - KK : m.src + (int64_t)u; if (s0 < 0) { u++; continue; }
                        const size_t ra = rec_of(m.src_asm, s0); const auto& rr = RS[m.src_asm];
                        const uint64_t rend = ra < rr.size() ? rr[ra] : S[m.src_asm].size(), rbeg = ra ? rr[ra - 1] : 0;
                        uint64_t run;                                                       // windows of this source record
                        if (!m.rc) run = (uint64_t)s0 + KK <= rend ? std::min<uint64_t>(u_hi - u + 1, rend - KK - (uint64_t)s0 + 1) : 0;
                        else run = (uint64_t)s0 >= rbeg && (uint64_t)s0 + KK <= rend ? std::min<uint64_t>(u_hi - u + 1, (uint64_t)s0 - rbeg + 1) : 0;
                        if (run) { skip.push_back({m.s + u, m.s + u + run}); u += run; } else u++; } }
                std::sort(skip.begin(), skip.end());
                const auto& rs = RS[j]; const uint64_t sn = S[j].size(); const uint8_t* sq = S[j].data();
                std::atomic<size_t> nr{0}; std::vector<std::thread> th;
                for (int t = 0; t < T; t++) th.emplace_back([&] { std::vector<uint64_t> loc; uint64_t v = 0, c = 0;
                    for (size_t r; (r = nr++) < rs.size(); ) { const uint64_t a = rs[r], e = r + 1 < rs.size() ? rs[r + 1] : sn; if (e - a < (uint64_t)KK) continue;
                        size_t si = (size_t)(std::lower_bound(skip.begin(), skip.end(), std::make_pair(a, (uint64_t)0)) - skip.begin()); if (si) si--;
                        uint64_t w = a;                                                     // next window start to visit
                        while (w + KK <= e) {
                            while (si < skip.size() && skip[si].second <= w) si++;
                            const uint64_t stop = (si < skip.size() && skip[si].first <= w) ? w : (si < skip.size() ? std::min<uint64_t>(skip[si].first, e - KK + 1) : e - KK + 1);
                            if (stop == w && si < skip.size() && skip[si].first <= w) { c += std::min<uint64_t>(skip[si].second, e - KK + 1) - w; w = std::min<uint64_t>(skip[si].second, e - KK + 1); continue; }
                            uint64_t f = 0, rv = 0; int run = 0;                             // prime and roll over windows w .. stop-1
                            for (uint64_t i = w; i < stop + KK - 1; i++) { const int x = b2(sq[i]); if (x < 0) { run = 0; continue; }
                                f = ((f << 2) | (uint64_t)x) & KKM; rv = (rv >> 2) | ((uint64_t)(3 - x) << (2 * (KK - 1)));
                                if (++run >= KK) { v++; const uint64_t cn = canon31(f, rv); if (!(mix(cn) % SAMP)) loc.push_back(cn); } }
                            w = stop; } }
                    std::lock_guard<std::mutex> lk(mu); all.insert(all.end(), loc.begin(), loc.end()); vis += v; cov += c; });
                for (auto& x : th) x.join(); }
            visited = vis; covered = cov; std::sort(all.begin(), all.end()); all.erase(std::unique(all.begin(), all.end()), all.end()); return all; };
        uint64_t v0, c0, v1, c1, mb = 0, nm = 0, nm31 = 0; for (auto& m : MA) for (const KMatch& x : m) { mb += x.e - x.s; nm++; nm31 += x.e - x.s >= (uint64_t)KK; }
        double t0 = now(); const std::vector<uint64_t> naive = count(false, v0, c0); const double tn = now() - t0;
        t0 = now(); const std::vector<uint64_t> tok = count(true, v1, c1); const double tt = now() - t0;
        const bool same = naive == tok;
        fprintf(stderr, "[pg] k-mers (31, canonical, sampled 1/64): match bytes %.2f %% of the sequence; naive %llu windows in %.2f s (+ decode %.1f s), tokens %llu windows (%llu skipped) in %.2f s; sets %s (%zu sampled distinct)\n",
                100.0 * mb / seq_tot, (unsigned long long)v0, tn, t_dec, (unsigned long long)v1, (unsigned long long)c1, tt, same ? "identical" : "DIFFER", naive.size());
        printf("\t%.3f\t%.2f\t%.2f\t%llu\t%llu\t%s\t%llu\t%llu\t%llu\t%llu", 100.0 * mb / seq_tot, tn, tt, (unsigned long long)v0, (unsigned long long)v1, same ? "identical" : "DIFFER",
               (unsigned long long)seq_tot, (unsigned long long)mb, (unsigned long long)nm, (unsigned long long)nm31);   // + sequence bytes, match bytes, matches, matches >= k
    }
    printf("\n");
    return ok ? 0 : 4;
}
