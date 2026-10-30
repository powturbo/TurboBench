// refseg.cpp - AX_REFSEG prototype (not a format; tuning builds only): assemblies of one species compressed with
// reference segments. Each FASTA is split into a line model (header lines + run-length line lengths, zstd -19) and its
// sequence bytes (case kept). The first assembly is compressed alone; every block (1 MiB) of a later one may copy from a
// reference segment: up to 3 blocks of an earlier assembly - forward or reverse-complemented - placed before the block
// (src/aceapex_main.cpp compress_block wlo, worker hook g_ax_ref_fn). The segment is found by voting sampled unique
// 20-mers of the earlier assemblies (bins of 64 KiB on the diagonal). Depth <= 2: assembly 2 refers to 1, assembly 3 to
// 1 or 2. The decoder (decompress_streams with the prefix as out0) rebuilds every assembly and the FASTA files are
// compared byte for byte with the originals. With AX_REFSEG=0 the same pipeline runs without references (the line
// model alone; AX_HASH12=1 the 12-byte head table), for the density table of results/pangenome-2026-10-02.log.
// The container (magic AXREFSG1) is the prototype's own: no ACEAPEX decoder reads it.
// Build: g++ -std=c++17 -O3 -march=native -DACEAPEX_ENV_TUNING -Isrc scripts/refseg.cpp -lzstd -lpthread -o refseg
// Usage: [AX_REFSEG=1] [AX_HASH12=1] refseg <out.axr> <a.fa> <b.fa> [c.fa ...]
//   Last line: REFSEGROW <tab> refs hash12 assemblies fasta_bytes container_bytes per-assembly bytes ... enc_s dec_s ok
#include "aceapex_api.cpp"
#include <zstd.h>
#include <unordered_map>
#include <string>
#include <thread>
#include <numeric>

namespace rs {
static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::vector<uint8_t> slurp(const char* p) { std::vector<uint8_t> v; FILE* f = fopen(p, "rb"); if (!f) { perror(p); exit(1); }
    fseek(f, 0, SEEK_END); v.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(v.data(), 1, v.size(), f) != v.size()) exit(1); fclose(f); return v; }
static void put_v(std::vector<uint8_t>& o, uint64_t x) { while (x >= 0x80) { o.push_back((uint8_t)(x | 0x80)); x >>= 7; } o.push_back((uint8_t)x); }
static uint64_t get_v(const uint8_t*& p) { uint64_t v = 0; int s = 0; for (;;) { const uint8_t c = *p++; v |= (uint64_t)(c & 0x7F) << s; if (!(c & 0x80)) return v; s += 7; } }
static void put64(std::vector<uint8_t>& o, uint64_t x) { for (int i = 0; i < 8; i++) o.push_back((uint8_t)(x >> (8 * i))); }
static uint64_t get64(const uint8_t*& p) { uint64_t v; memcpy(&v, p, 8); p += 8; return v; }

// FASTA -> line model (raw, before zstd) + sequence bytes. Lines end with '\n'; a header line starts with '>'.
// Model: per line either a header (tag 0, text) or a run of sequence lines (tag 1, length, count); tag 2 = the file
// does not end with '\n' (then the last line is unterminated).
static void split(const std::vector<uint8_t>& f, std::vector<uint8_t>& lm, std::vector<uint8_t>& seq) {
    seq.reserve(f.size()); size_t i = 0; uint64_t rl = 0, rc = 0; const size_t n = f.size();
    auto flush = [&] { if (rc) { lm.push_back(1); put_v(lm, rl); put_v(lm, rc); rc = 0; } };
    while (i < n) {
        size_t e = i; while (e < n && f[e] != '\n') e++;
        if (f[i] == '>') { flush(); lm.push_back(0); put_v(lm, e - i); lm.insert(lm.end(), f.begin() + (long)i, f.begin() + (long)e); }
        else { const uint64_t L = e - i; if (rc && L == rl) rc++; else { flush(); rl = L; rc = 1; } seq.insert(seq.end(), f.begin() + (long)i, f.begin() + (long)e); }
        if (e == n) { flush(); lm.push_back(2); break; }
        i = e + 1;
    }
    flush(); lm.push_back(3);
}
static std::vector<uint8_t> join(const std::vector<uint8_t>& lm, const uint8_t* seq, size_t seq_n) {
    std::vector<uint8_t> f; f.reserve(seq_n + seq_n / 50 + 1024); const uint8_t* p = lm.data(); size_t sp = 0; bool unterminated = false;
    for (;;) { const uint8_t t = *p++;
        if (t == 0) { const uint64_t L = get_v(p); f.insert(f.end(), p, p + L); p += L; f.push_back('\n'); }
        else if (t == 1) { const uint64_t L = get_v(p), C = get_v(p); for (uint64_t c = 0; c < C && sp + L <= seq_n; c++) { f.insert(f.end(), seq + sp, seq + sp + L); sp += L; f.push_back('\n'); } }
        else if (t == 2) unterminated = true;
        else break; }
    if (unterminated && !f.empty()) f.pop_back();
    return f;
}
static inline uint8_t comp(uint8_t c) { switch (c) { case 'A': return 'T'; case 'C': return 'G'; case 'G': return 'C'; case 'T': return 'A';
    case 'a': return 't'; case 'c': return 'g'; case 'g': return 'c'; case 't': return 'a'; default: return c; } }
static std::vector<uint8_t> revcomp(const std::vector<uint8_t>& s) { std::vector<uint8_t> r(s.size() + 64, 0); const size_t n = s.size();
    for (size_t i = 0; i < n; i++) r[n - 1 - i] = comp(s[i]); r.resize(n); r.reserve(n + 64); return r; }
static inline int b2(uint8_t c) { switch (c | 0x20) { case 'a': return 0; case 'c': return 1; case 'g': return 2; case 't': return 3; default: return -1; } }
static inline uint64_t mix(uint64_t x) { x ^= x >> 33; x *= 0xff51afd7ed558ccdull; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ull; x ^= x >> 33; return x; }
static const int K = 20; static const uint64_t KM = (1ull << (2 * K)) - 1; static const uint64_t SAMP = 64;
struct Ent { uint64_t code; uint64_t pos; };                                  // pos: bits 0..39 position, 40.. assembly
// every position whose forward 20-mer (no N) passes the sampling rule
template <class F> static void kmers(const uint8_t* s, size_t n, F f) {
    uint64_t c = 0; int run = 0;
    for (size_t i = 0; i < n; i++) { const int x = b2(s[i]); if (x < 0) { run = 0; continue; } c = ((c << 2) | (uint64_t)x) & KM;
        if (++run >= K) f(i + 1 - K, c); }
}
static inline uint64_t rc_code(uint64_t c) { uint64_t r = 0; for (int i = 0; i < K; i++) { r = (r << 2) | (3 - (c & 3)); c >>= 2; } return r; }

struct Asm { std::string path, name; std::vector<uint8_t> lm, lmz, seq, rcs; uint64_t fasta = 0; };
struct Ref { uint8_t asmi = 0xFF, rc = 0; uint32_t rb0 = 0; uint8_t nrb = 0; uint32_t votes = 0; };
struct Prov { const std::vector<Asm>* A; const std::vector<Ref>* refs; size_t bs; };
static size_t provide(void* ctx, size_t b, const uint8_t** pre) {
    const Prov& P = *(const Prov*)ctx; const Ref& r = (*P.refs)[b]; if (r.asmi == 0xFF) return 0;
    const Asm& a = (*P.A)[r.asmi]; const size_t n = a.seq.size(), lo = (size_t)r.rb0 * P.bs, hi = std::min(n, lo + (size_t)r.nrb * P.bs);
    if (!r.rc) { *pre = a.seq.data() + lo; return hi - lo; }
    *pre = a.rcs.data() + (n - hi); return hi - lo;                            // rc(seq[lo, hi)) = rcs[n - hi, n - lo)
}
}  // namespace rs

int main(int argc, char** argv) {
    using namespace rs;
    if (argc < 4) { fprintf(stderr, "usage: %s <out.axr> <a.fa> <b.fa> [...]\n", argv[0]); return 1; }
    const bool refs_on = ax_getenv("AX_REFSEG") && atoi(ax_getenv("AX_REFSEG"));
    const bool h12 = ax_getenv("AX_HASH12") && atoi(ax_getenv("AX_HASH12"));
    setenv("ACEAPEX_BS", "1048576", 1); if (h12) setenv("AX_HLOG", "22", 1);
    const size_t BS = 1048576; const int T = (int)std::thread::hardware_concurrency();
    std::vector<Asm> A(argc - 2);
    for (size_t j = 0; j < A.size(); j++) { A[j].path = argv[j + 2]; std::string nm = A[j].path; nm = nm.substr(nm.find_last_of('/') + 1); A[j].name = nm;
        std::vector<uint8_t> f = slurp(argv[j + 2]); A[j].fasta = f.size(); split(f, A[j].lm, A[j].seq);
        A[j].lmz.resize(ZSTD_compressBound(A[j].lm.size())); A[j].lmz.resize(ZSTD_compress(A[j].lmz.data(), A[j].lmz.size(), A[j].lm.data(), A[j].lm.size(), 19));
        A[j].seq.reserve(A[j].seq.size() + 64);
        if (refs_on && j + 1 < A.size()) A[j].rcs = revcomp(A[j].seq);
        fprintf(stderr, "[refseg] %s: %llu B FASTA, %llu B sequence, line model %zu -> %zu B\n", A[j].name.c_str(), (unsigned long long)A[j].fasta, (unsigned long long)A[j].seq.size(), A[j].lm.size(), A[j].lmz.size()); }
    std::vector<uint8_t> out; out.insert(out.end(), {'A', 'X', 'R', 'E', 'F', 'S', 'G', '1'}); put64(out, A.size());
    std::vector<std::vector<Ref>> R(A.size()); std::vector<uint64_t> img_bytes(A.size()), part(A.size());
    std::vector<Ent> idx;                                                       // sampled unique 20-mers of the assemblies so far
    double t_enc = 0, t_idx = 0;
    for (size_t j = 0; j < A.size(); j++) {
        const size_t nb = (A[j].seq.size() + BS - 1) / BS; R[j].assign(nb, Ref());
        double t0 = now();
        if (refs_on && j > 0) {                                                 // votes per block: (assembly, orientation, diagonal bin)
            std::atomic<size_t> next{0}; std::vector<std::thread> th;
            for (int t = 0; t < T; t++) th.emplace_back([&] {
                std::unordered_map<uint64_t, uint32_t> v;
                for (size_t b; (b = next++) < nb; ) {
                    v.clear(); const size_t lo = b * BS, n = std::min(BS, A[j].seq.size() - lo);
                    kmers(A[j].seq.data() + lo, n, [&](size_t t, uint64_t c) {
                        for (int o = 0; o < 2; o++) { const uint64_t key = o ? rc_code(c) : c; if (mix(key) % SAMP) continue;
                            auto it = std::lower_bound(idx.begin(), idx.end(), key, [](const Ent& e, uint64_t k) { return e.code < k; });
                            if (it == idx.end() || it->code != key) continue;
                            const uint64_t p = it->pos & ((1ull << 40) - 1), as = it->pos >> 40;
                            const int64_t d = o ? (int64_t)(p + t + K - 1) : (int64_t)p - (int64_t)t;       // fwd: ref of block start; rc: A = ref + target
                            if (d < 0) continue;
                            v[(as << 56) | ((uint64_t)o << 55) | ((uint64_t)d >> 16)]++; } });
                    uint64_t best = 0; uint32_t bv = 0; for (auto& kv : v) if (kv.second > bv) { bv = kv.second; best = kv.first; }
                    if (bv < 16) continue;
                    const size_t as = best >> 56; const int o = (best >> 55) & 1; const int64_t d = (int64_t)((best & ((1ull << 55) - 1)) << 16);
                    const int64_t M = 256 * 1024, rn = (int64_t)A[as].seq.size();
                    int64_t lo2 = o ? d - (int64_t)BS - M : d - M, hi2 = o ? d + 65536 + M : d + 65536 + (int64_t)BS + M;
                    lo2 = std::max<int64_t>(0, lo2); hi2 = std::min<int64_t>(rn, hi2); if (hi2 <= lo2) continue;
                    size_t rb0 = (size_t)lo2 / BS, rb1 = ((size_t)hi2 + BS - 1) / BS; if (rb1 - rb0 > 3) { const size_t c = (rb0 + rb1) / 2; rb0 = c > 1 ? c - 1 : 0; rb1 = rb0 + 3; }
                    R[j][b] = Ref{(uint8_t)as, (uint8_t)o, (uint32_t)rb0, (uint8_t)(rb1 - rb0), bv}; } });
            for (auto& x : th) x.join();
        }
        t_idx += now() - t0; t0 = now();
        Prov P{&A, &R[j], BS};
        g_ax_ref_fn = (refs_on && j > 0) ? provide : nullptr; g_ax_ref_ctx = &P;
        std::vector<uint8_t> z(aceapex_compress_bound(A[j].seq.size()));
        const int64_t zs = aceapex_compress(A[j].seq.data(), A[j].seq.size(), z.data(), z.size(), 2, T);
        g_ax_ref_fn = nullptr;
        if (zs <= 0) { fprintf(stderr, "compress failed\n"); return 2; }
        uint32_t bsz; memcpy(&bsz, z.data() + 20, 4); if (bsz != BS) { fprintf(stderr, "block size %u, not %zu\n", bsz, BS); return 2; }
        t_enc += now() - t0; z.resize((size_t)zs); img_bytes[j] = (uint64_t)zs;
        const size_t at = out.size();
        put64(out, A[j].name.size()); out.insert(out.end(), A[j].name.begin(), A[j].name.end()); put64(out, A[j].fasta); put64(out, A[j].seq.size());
        put64(out, A[j].lmz.size()); out.insert(out.end(), A[j].lmz.begin(), A[j].lmz.end());
        put64(out, nb); for (const Ref& r : R[j]) { out.push_back(r.asmi); out.push_back(r.rc); put64(out, r.rb0); out.push_back(r.nrb); }   // 11 B per block
        put64(out, z.size()); out.insert(out.end(), z.begin(), z.end());
        part[j] = out.size() - at;
        size_t nref = 0, nrc = 0; for (const Ref& r : R[j]) { nref += r.asmi != 0xFF; nrc += r.asmi != 0xFF && r.rc; }
        fprintf(stderr, "[refseg] %s: %zu blocks, %zu with a reference (%zu reverse-complemented), sequence image %lld B, part %llu B\n",
                A[j].name.c_str(), nb, nref, nrc, (long long)zs, (unsigned long long)part[j]);
        if (refs_on && j + 1 < A.size()) {                                       // this assembly's sampled unique 20-mers join the index
            t0 = now(); std::vector<Ent> add; kmers(A[j].seq.data(), A[j].seq.size(), [&](size_t p, uint64_t c) { if (!(mix(c) % SAMP)) add.push_back({c, p | ((uint64_t)j << 40)}); });
            idx.insert(idx.end(), add.begin(), add.end()); std::sort(idx.begin(), idx.end(), [](const Ent& a, const Ent& b) { return a.code < b.code || (a.code == b.code && a.pos < b.pos); });
            std::vector<Ent> u; u.reserve(idx.size());
            for (size_t i = 0; i < idx.size(); ) { size_t e = i; while (e < idx.size() && idx[e].code == idx[i].code) e++; if (e - i == 1) u.push_back(idx[i]); i = e; }
            idx.swap(u); t_idx += now() - t0; fprintf(stderr, "[refseg] index: %zu unique sampled 20-mers\n", idx.size()); }
    }
    { FILE* f = fopen(argv[1], "wb"); if (!f || fwrite(out.data(), 1, out.size(), f) != out.size()) { fprintf(stderr, "write failed\n"); return 3; } fclose(f); }
    for (auto& a : A) { std::vector<uint8_t>().swap(a.seq); std::vector<uint8_t>().swap(a.rcs); }
    // ---- decode from the container alone ----
    const double td0 = now(); bool ok = true; const std::vector<uint8_t> in = slurp(argv[1]); const uint8_t* p = in.data() + 8; const uint64_t na = get64(p);
    std::vector<std::vector<uint8_t>> S(na), SR(na); std::vector<std::vector<Ref>> DR(na); double t_ra = 0; uint64_t ra_blocks = 0, ra_n = 0, ra_max = 0;
    for (uint64_t j = 0; j < na && ok; j++) {
        const uint64_t nl = get64(p); std::string nm((const char*)p, nl); p += nl; const uint64_t fasta = get64(p), sn = get64(p);
        const uint64_t lzn = get64(p); std::vector<uint8_t> lm(ZSTD_getFrameContentSize(p, lzn)); ZSTD_decompress(lm.data(), lm.size(), p, lzn); p += lzn;
        const uint64_t nb = get64(p); DR[j].resize(nb); for (uint64_t b = 0; b < nb; b++) { Ref& r = DR[j][b]; r.asmi = *p++; r.rc = *p++; r.rb0 = (uint32_t)get64(p); r.nrb = *p++; }
        const uint64_t zn = get64(p); const uint8_t* img = p; p += zn;
        aceapex_streams_t st; if (aceapex_decode_streams(img, zn, &st) || st.orig_size != sn || st.num_blocks != nb) { ok = false; break; }
        S[j].assign(sn + 64, 0); const BlockOffsets* bo = (const BlockOffsets*)st.boffs;
        std::atomic<size_t> next{0}; std::atomic<int> bad{0}; std::vector<std::thread> th;
        for (int t = 0; t < T; t++) th.emplace_back([&] { std::vector<uint8_t> V;
            for (size_t b; (b = next++) < nb; ) {
                const size_t lo = b * BS, n = std::min<size_t>(BS, sn - lo); const Ref& r = DR[j][b]; size_t pl = 0;
                if (r.asmi != 0xFF) { if (r.asmi >= j) { bad = 1; continue; }
                    const std::vector<uint8_t>& src = r.rc ? SR[r.asmi] : S[r.asmi]; const size_t rn = src.size();
                    const size_t lo2 = (size_t)r.rb0 * BS, hi2 = std::min(rn, lo2 + (size_t)r.nrb * BS); pl = hi2 - lo2; V.resize(pl + n + 64);
                    if (r.rc) memcpy(V.data(), src.data() + (rn - hi2), pl); else memcpy(V.data(), src.data() + lo2, pl); }
                else V.resize(n + 64);
                decompress_streams(V.data(), pl + n, st.lit + bo[b].lit_off, bo[b].lit_sz, st.off + bo[b].off_off, bo[b].off_sz, st.len + bo[b].len_off, bo[b].len_sz,
                                   st.cmd + bo[b].cmd_off, bo[b].cmd_sz, pl);
                memcpy(S[j].data() + lo, V.data() + pl, n); } });
        for (auto& x : th) x.join(); aceapex_streams_free(&st);
        if (bad) { ok = false; break; }
        S[j].resize(sn);
        if (j + 1 < na) { SR[j] = revcomp(S[j]); S[j].reserve(sn + 64); }
        const std::vector<uint8_t> f = join(lm, S[j].data(), sn), orig = slurp(argv[j + 2]);
        const bool same = f.size() == fasta && f == orig; ok = ok && same;
        fprintf(stderr, "[refseg] decode %s: %s\n", nm.c_str(), same ? "FASTA bit-perfect" : "DIFFERS");
        // random access: blocks to decode for one block of this assembly (its references, theirs, ...)
        for (uint64_t b = 0; b < nb; b++) { uint64_t cnt = 1; std::vector<std::pair<uint64_t, uint64_t>> todo{{j, b}};
            std::vector<std::pair<uint64_t, uint64_t>> seen;
            while (!todo.empty()) { auto [aj, ab] = todo.back(); todo.pop_back(); const Ref& r = DR[aj][ab]; if (r.asmi == 0xFF) continue;
                for (uint32_t q = 0; q < r.nrb; q++) { std::pair<uint64_t, uint64_t> k{r.asmi, r.rb0 + q}; if (std::find(seen.begin(), seen.end(), k) != seen.end()) continue; seen.push_back(k); cnt++; todo.push_back(k); } }
            ra_blocks += cnt; ra_n++; ra_max = std::max(ra_max, cnt); }
    }
    const double t_dec = now() - td0; (void)t_ra;
    uint64_t fasta_tot = 0; for (auto& a : A) fasta_tot += a.fasta;
    fprintf(stderr, "[refseg] refs %d hash12 %d: %llu B FASTA -> %zu B (ratio %.2f); encode %.1f s (+ reference search %.1f s), decode + check %.1f s; random access: %.2f blocks per block on average, %llu at most; %s\n",
            refs_on, h12, (unsigned long long)fasta_tot, out.size(), (double)fasta_tot / out.size(), t_enc, t_idx, t_dec, (double)ra_blocks / std::max<uint64_t>(1, ra_n), (unsigned long long)ra_max, ok ? "all bit-perfect" : "FAILED");
    printf("REFSEGROW\t%d\t%d\t%zu\t%llu\t%zu", refs_on, h12, A.size(), (unsigned long long)fasta_tot, out.size());
    for (size_t j = 0; j < A.size(); j++) printf("\t%s:%llu", A[j].name.c_str(), (unsigned long long)part[j]);
    printf("\t%.1f\t%.1f\t%.2f\t%s\n", t_enc + t_idx, t_dec, (double)ra_blocks / std::max<uint64_t>(1, ra_n), ok ? "ok" : "FAILED");
    return ok ? 0 : 4;
}
