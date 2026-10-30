// gpu_plan_emu.cpp - CPU judge of the GPU library's plan (src/aceapex_gpu_plan.h): builds the plan of an archive,
// then executes it job by job on the CPU exactly as src/aceapex_gpu_lib.cu schedules the device - raw copies,
// zstd frames (what nvCOMP does), rANS pieces (axr_decode, what k_rans does), the zstd DNA unpack (k_unpack,
// k_exc), the open DNA pack (k_open_cse, k_open_bases, k_open_exc through the same ax_open_warp.h steps), the
// match (k_decode_g, one lane) - into a temp buffer with the plan's layout, and compares the output with the
// CPU library. Full decode, and random ranges on a zeroed temp with only the jobs agp::select picks: a job the
// selection misses leaves zeros and shows up as a difference. Then byte mutations of each archive: the plan
// builder and the executor must refuse or finish, never read or write outside their buffers.
// Inputs: the conformance fixtures, the chr1 4 MiB fixture slice, and that slice / a text buffer encoded here
// in the default, interactive, rANS-token and open profiles. Prints three claim lines (head_gpu_plan_emu, head_gpu_flip_emu, head_gpu_zstd_validate).
// Build: g++ -std=c++17 -O2 -Isrc scripts/gpu_plan_emu.cpp src/aceapex_api.cpp -lzstd -lpthread
#include "aceapex.h"
#define AGP_WITH_ZSTD   // agp::validate_zstd (ACEAPEX_GPU_VALIDATE_ZSTD)
#include "aceapex_gpu_plan.h"
#include "ax_vec.h"
#include <zstd.h>
#define XXH_INLINE_ALL
#include "xxhash.h"
#include <cstdio>
#include <string>
#include <vector>
#include <random>

static uint64_t nvt_cpu(size_t n, size_t, size_t) { return 64 * n; }   // stand-in for nvCOMP's temp size

struct Exec {
    const agp::Plan& P; const uint8_t* in; std::vector<uint8_t>& T; int err = 0;
    std::mt19937_64* jr = nullptr; uint64_t junk_() { return (*jr)(); }
    Exec(const agp::Plan& p, const uint8_t* i, std::vector<uint8_t>& t) : P(p), in(i), T(t) {}
    uint8_t* at(uint64_t o) { return T.data() + o; }
    void raw(size_t i) { const agp::Raw& r = P.raw[i]; memcpy(at(r.dst), in + r.src, r.n); }
    void nv(size_t i) { const agp::Nv& j = P.nv[i]; size_t r = ZSTD_decompress(at(j.out_off), j.osz, in + j.in_off, j.csz);
        if (ZSTD_isError(r) || r != j.osz) { err |= 4; if (jr) for (uint64_t k = 0; k < j.osz; k++) at(j.out_off)[k] = (uint8_t)junk_(); } }
    void rans(size_t i) { const agp::Rans& r = P.rans[i];
        if (r.mode == 0) { if (r.csz != r.n) err |= 1; else memcpy(at(r.dst), in + r.src, r.n); }
        else if (axr_decode(in + r.src, r.csz, at(r.dst), r.n)) err |= 1; }
    void dna(size_t k) { const agp::Dna& d = P.dna[k]; const uint8_t* seq = at(d.seq); const uint8_t* cse = at(d.cse); uint8_t* dst = at(d.dst);
        for (uint64_t i0 = 0; i0 < d.raw; i0 += 16) {              // k_unpack (AX_VEC 1): 16 bases per thread, one 16-byte store when aligned
            if (i0 + 16 <= d.raw && !((uintptr_t)(dst + i0) & 15)) { uint32_t w[4]; axv_unpack16(seq, cse, i0, w); axv_store16(dst + i0, w[0], w[1], w[2], w[3]); vec16++; continue; }
            for (uint64_t i = i0; i < i0 + 16 && i < d.raw; i++) { uint8_t b = "ACGT"[(seq[i >> 2] >> (6 - 2 * (i & 3))) & 3]; if (cse[i >> 3] & (0x80 >> (i & 7))) b |= 0x20; dst[i] = b; } }
        if (!d.nexc) return;
        const uint32_t* gap = (const uint32_t*)at(d.gap); const uint8_t* val = d.val == agp::NUL ? nullptr : at(d.val); uint32_t pos = 0;
        for (uint32_t e = 0; e < d.nexc; e++) { uint32_t g; memcpy(&g, gap + e, 4); pos += g; if (pos < d.raw) dst[pos] = val ? val[e] : 0; } }
    // k_open_cg (case runs, then the exception positions into the plan's epos region) and k_open_bases_s (blocks of
    // 4096 positions, run-end / exception slices copied as to shared memory, exception bytes in the 16-byte store)
    void open(size_t k) { const agp::Open& d = P.open[k]; const uint8_t* cse = at(d.cse); uint32_t* ends = (uint32_t*)at(d.ends);
        uint32_t* ep = (uint32_t*)at(d.epos); const uint32_t raw = (uint32_t)d.raw;
        bool bad = false; uint64_t sum = 0; uint32_t j = 0;
        for (uint32_t t = 0; t < d.ncse; t++) { bool term = axl_term(t, cse, d.ncse); uint32_t v = axl_value(t, cse, term, bad);
            if (term) { sum += v; axl_cse_end(term, j, v, sum, raw, ends, bad); j++; } }
        if (axl_tail_bad(cse, d.ncse) || sum != d.raw) bad = true;
        uint32_t R = bad ? 0 : j; memcpy(at(d.nrun), &R, 4); if (bad) { err |= 2; return; }
        if (d.nexc) { const uint8_t* gp = at(d.gap); sum = 0; j = 0;
            for (uint32_t t = 0; t < d.ngap; t++) { bool term = axl_term(t, gp, d.ngap); uint32_t v = axl_value(t, gp, term, bad);
                if (term) { sum += v; axl_exc_pos(term, j, v, sum, d.nexc, raw, ep, bad); j++; } }
            if (axl_tail_bad(gp, d.ngap) || j != d.nexc || bad) err |= 2; }
        if (tile) return;                                                             // AX_GPU_TILE: no bases kernel
        for (uint32_t first = 0; first < raw; first += 4096) {                       // k_open_bases_s: one block of 256 threads
            const uint32_t last = std::min(first + 4095u, raw - 1);
            const uint32_t jlo = axl_run_of(ends, R, first), jhi = axl_run_of(ends, R, last);
            const uint32_t elo = axl_exc_in(ep, 0, d.nexc, first), ehi = axl_exc_in(ep, 0, d.nexc, last + 1);
            const uint32_t nj = std::min(jhi, R - 1) - jlo + 1, nx = ehi - elo; const bool sh = nj <= 1024 && nx <= 1024;
            std::vector<uint32_t> se(ends + jlo, ends + jlo + (sh ? nj : 0)), sp(ep + elo, ep + elo + (sh ? nx : 0)); se.push_back(0); sp.push_back(0);
            const uint32_t* E = sh ? se.data() - jlo : ends; const uint32_t* Pp = sh ? sp.data() - elo : ep;
            for (uint32_t g = first / 16; g < first / 16 + 256 && 16 * g < raw; g++)
                axl_bases16_v(g, at(d.seq), E, R, raw, at(d.dst), axl_run_in(E, jlo, jhi, 16 * g), Pp, ehi, at(d.val), axl_exc_in(Pp, elo, ehi, 16 * g)); } }
    static uint32_t varint(const uint8_t* b, uint32_t& p, uint32_t n, bool& bad) {   // rd_varint: <= 5 bytes, else bad
        uint32_t v = 0; for (uint32_t k = 0; k < 5 && p < n; k++) { uint8_t c = b[p++]; v |= (uint32_t)(c & 0x7F) << (7 * k); if (!(c & 0x80)) return v; }
        bad = true; return 0; }
    // k_decode_g with one lane: lengths against the room left, at most cs+1 steps (else the LIMIT bit, 32);
    // steps and copies outside the block are counted for the judge
    uint64_t steps = 0, limit = 0, oob = 0, vec16 = 0, vcopy = 0, tiles = 0;
    bool tile = false;                                    // AX_GPU_TILE schedule (k_open_cg, then k_decode_t)
    static uint32_t match(const uint8_t* lit, const uint8_t* off, const uint8_t* len, const uint8_t* cmd, uint32_t ls, uint32_t os, uint32_t ns, uint32_t cs,
                          uint8_t* dst, uint32_t n, uint64_t& steps, uint64_t& limit, uint64_t& oob, uint64_t* vcopy = nullptr) {
        uint32_t lp = 0, op = 0, np = 0, cp = 0, o = 0, rep[4] = {1, 2, 4, 8}, st = 0;
        while (o < n) { int type = 2; uint32_t l = 0, aux = 0; const uint32_t rem = n - o;
            if (++st > cs + 1) { limit++; break; }
            bool vb = false;
            while (cp < cs) { uint8_t c = cmd[cp++];
                if (c == 0xFF) { rep[0] = 1; rep[1] = 2; rep[2] = 4; rep[3] = 8; continue; }
                if (c < 0x80) { l = c + 1u; if (l > ls - lp || l > rem) break; type = 0; aux = lp; lp += l; }
                else if ((c & 0xC0) == 0x80) { uint32_t ri = (c >> 4) & 3, lv = c & 0x0F; if (lv == 0x0F) lv += varint(len, np, ns, vb);
                    uint32_t d = rep[ri]; if (ri) { for (int i = ri; i > 0; i--) rep[i] = rep[i - 1]; rep[0] = d; }
                    if (vb || (lv < 0x0F && (c & 0x0F) == 0x0F)) break;
                    if (lv > rem || lv + 6 > rem || !d || d > o) break;
                    l = lv + 6; type = 1; aux = d; }
                else { uint32_t lv = c == 0xFE ? varint(len, np, ns, vb) : (uint32_t)(c & 0x3F); uint32_t d = varint(off, op, os, vb);
                    rep[3] = rep[2]; rep[2] = rep[1]; rep[1] = rep[0]; rep[0] = d;
                    if (vb || lv > rem || lv + 6 > rem || !d || d > o) break;
                    l = lv + 6; type = 1; aux = d; }
                break; }
            if (type == 2) break;
            if ((uint64_t)o + l > n) { oob++; break; }
            // k_decode_g (AX_VEC 1): a copy of >= 64 bytes without overlap by the 32 lanes of axv_copy16, lane by lane
            if (l >= 64 && (type == 0 || aux >= l)) { const uint8_t* src = type == 0 ? lit + aux : dst + o - aux; for (uint32_t lg = 0; lg < 32; lg++) axv_copy16(dst + o, src, l, lg, 32); if (vcopy) ++*vcopy; }
            else if (type == 0) memcpy(dst + o, lit + aux, l); else for (uint32_t i = 0; i < l; i++) dst[o + i] = dst[o - aux + i];
            o += l; }
        steps += st; return o; }
    // k_decode_t: the block's literal slice built from the open chunks, 16 stream positions at a time aligned to 16, into a
    // buffer of the kernel's shared size (lanes in any order: the steps are independent), then the tokens from it
    void block_tile(uint32_t b, uint8_t* out) {
        const uint8_t* e = &P.bo[64ull * b]; auto q = [&](int i) { return agp::rd64(e + 8 * i); };
        const uint8_t *off = at(P.o_s[1] + q(1)), *len = at(P.o_s[2] + q(2)), *cmd = at(P.o_s[3] + q(3));
        uint64_t base = (uint64_t)b * P.bs, rem = P.orig - base; uint32_t n = (uint32_t)std::min<uint64_t>(rem, P.bs);
        const uint64_t lo = q(0), ls = q(4), a0 = lo & ~15ull, a1 = (lo + ls + 15) & ~15ull, CH = P.chunk[0];
        static uint8_t sh[16448]; bool bad = ls > P.bs || a1 - a0 > sizeof sh - 16 || lo + ls > P.ssz[0];
        for (uint64_t pos = a0; !bad && pos < a1; pos += 16) {
            const uint64_t k = pos / CH; if (k >= P.cmap.size()) { bad = true; break; }
            if (P.cmap[k] == agp::NUL32) { memcpy(sh + (pos - a0), at(P.o_s[0] + pos), 16); tiles++; continue; }   // open plain chunk, from the literal stream
            const agp::Open& d = P.open[P.cmap[k]]; uint32_t R; memcpy(&R, at(d.nrun), 4); const uint32_t qq = (uint32_t)(pos - k * CH);
            if (!R || d.raw > 0xFFFFFF00ull) { bad = true; break; }
            if (qq >= (uint32_t)d.raw) continue;
            const uint32_t* ends = (const uint32_t*)at(d.ends); const uint32_t* ep = (const uint32_t*)at(d.epos); uint32_t w[4];
            axl_bases16_w(qq, at(d.seq), ends, R, (uint32_t)d.raw, axl_run_of(ends, R, qq), ep, d.nexc, at(d.val), axl_exc_in(ep, 0, d.nexc, qq), w);
            memcpy(sh + (pos - a0), w, 16); tiles++;
        }
        if (bad) { err |= 8; return; }
        if (match(sh + (lo - a0), off, len, cmd, (uint32_t)ls, (uint32_t)q(5), (uint32_t)q(6), (uint32_t)q(7), out + base, n, steps, limit, oob, &vcopy) != n) err |= 8; }
    void block(uint32_t b, uint8_t* out) {               // out = where block 0 would start
        if (tile) { block_tile(b, out); return; }
        const uint8_t* e = &P.bo[64ull * b]; auto q = [&](int i) { return agp::rd64(e + 8 * i); };
        const uint8_t *lit = at(P.o_s[0] + q(0)), *off = at(P.o_s[1] + q(1)), *len = at(P.o_s[2] + q(2)), *cmd = at(P.o_s[3] + q(3));
        uint64_t base = (uint64_t)b * P.bs, rem = P.orig - base; uint32_t n = (uint32_t)std::min<uint64_t>(rem, P.bs);
        if (match(lit, off, len, cmd, (uint32_t)q(4), (uint32_t)q(5), (uint32_t)q(6), (uint32_t)q(7), out + base, n, steps, limit, oob, &vcopy) != n) err |= 8; }
    // the schedule of aceapex_gpu_decompress_async (S == nullptr) / _range_async (S = the selection)
    void run(const agp::Sel* S, uint8_t* out) {
        auto each = [&](agp::Seg g, auto f) { for (uint32_t i = g.lo; i < g.hi; i++) f(i); };
        if (!S) {
            for (size_t i = 0; i < P.raw.size(); i++) raw(i);
            for (size_t i = 0; i < P.nv.size(); i++) nv(i);
            for (size_t i = 0; i < P.rans.size(); i++) rans(i);
            for (size_t k = 0; k < P.dna.size(); k++) dna(k);
            for (size_t k = 0; k < P.open.size(); k++) open(k);
            for (uint32_t b = 0; b < P.nb; b++) block(b, out);
        } else {
            for (int st = 1; st < 4; st++) { each(S->raw[st], [&](uint32_t i) { raw(i); }); each(S->nv_tok[st], [&](uint32_t i) { nv(i); }); each(S->rans_tok[st], [&](uint32_t i) { rans(i); }); }
            each(S->nv_lit, [&](uint32_t i) { nv(i); });
            for (int q = agp::C_SEQ; q < agp::C_N; q++) each(S->rans_cls[q], [&](uint32_t i) { rans(i); });
            each(S->dna, [&](uint32_t k) { dna(k); }); each(S->open, [&](uint32_t k) { open(k); });
            for (uint32_t b = S->b0; b < S->b1; b++) block(b, out);
        }
    }
};

int main(int argc, char** argv) {
    std::vector<std::pair<std::string, std::vector<uint8_t>>> arch;
    auto slurp = [](const std::string& p) { std::vector<uint8_t> v; FILE* f = fopen(p.c_str(), "rb"); if (!f) return v;
        fseek(f, 0, SEEK_END); v.resize(ftell(f)); fseek(f, 0, SEEK_SET); if (fread(v.data(), 1, v.size(), f) != v.size()) v.clear(); fclose(f); return v; };
    const std::string F = "verify/fixtures";
    { FILE* m = fopen((F + "/conf/manifest.tsv").c_str(), "r"); char line[512];
      while (m && fgets(line, sizeof line, m)) { if (line[0] == '#') continue; std::string n(line, strcspn(line, "\t\n")); if (n.empty()) continue;
          const char* rest = strchr(line, '\t'); if (rest) { rest = strchr(rest + 1, '\t'); if (rest) rest = strchr(rest + 1, '\t'); }
          if (rest && strncmp(rest + 1, "-", 1)) continue;                       // LEGACY archives that need a decode environment: skipped
          auto v = slurp(F + "/conf/" + n + ".aet"); if (!v.empty()) arch.push_back({n, v}); }
      if (m) fclose(m); }
    for (const char* v : {"1.4.8", "1.5.5"}) { auto a = slurp(F + "/chr1_4MiB.zstd-" + std::string(v) + ".aet"); if (!a.empty()) arch.push_back({std::string("chr1_4MiB.zstd-") + v, a}); }
    std::vector<uint8_t> slice;
    // encode the slice and a text buffer in four profiles
    { const auto& a = arch.back().second; uint64_t n; memcpy(&n, a.data() + 12, 8); slice.assign(n, 0);
      if (aceapex_decompress(a.data(), a.size(), slice.data(), n) != (int64_t)n) { printf("head_gpu_plan_emu\tfail\tcannot decode the slice\n"); return 1; }
      std::string txt; for (int i = 0; txt.size() < (3u << 20); i++) txt += "record " + std::to_string(i * 7919 % 10007) + " the quick brown fox; ";
      const char* prof[][3] = {{nullptr, nullptr, nullptr}, {"ACEAPEX_BS=16384", "LIT_CHUNK=65536", "FSE_CHUNK=4096"}, {"AX_TOK=rans", nullptr, nullptr},
                               {"AX_PROFILE=open", nullptr, nullptr}, {"AX_PROFILE=open", "ACEAPEX_BS=16384", "LIT_CHUNK=65536"}};
      int pi = 0;
      for (auto& pr : prof) { for (const char* e : pr) if (e) putenv((char*)e);
          for (int k = 0; k < 2; k++) { const uint8_t* s = k ? (const uint8_t*)txt.data() : slice.data(); size_t n2 = k ? txt.size() : slice.size();
              std::vector<uint8_t> z(aceapex_compress_bound(n2)); int64_t zs = aceapex_compress(s, n2, z.data(), z.size(), 2, 2);
              if (zs > 0) { z.resize(zs); arch.push_back({std::string(k ? "text" : "chr1slice") + "-p" + std::to_string(pi), z}); } }
          for (const char* e : pr) if (e) { std::string k(e); unsetenv(k.substr(0, k.find('=')).c_str()); }
          pi++; } }
    int archives = 0, refused = 0, bad = 0, ranges = 0, rbad = 0, mut = 0, mref = 0; uint64_t nvec16 = 0, nvcopy = 0, ntiles = 0; int ntile_arch = 0, ntile_rng = 0, nbatch = 0; std::string fails;
    std::mt19937_64 rng(20261001);
    for (auto& A : arch) {
        agp::Plan P; int e = agp::build(A.second.data(), A.second.size(), P, nvt_cpu);
        uint64_t n; memcpy(&n, A.second.data() + 12, 8);
        std::vector<uint8_t> ref(n + 1);
        if (aceapex_decompress(A.second.data(), A.second.size(), ref.data(), n) != (int64_t)n) continue;
        if (e) { refused++; fails += " refused:" + A.first; continue; }      // e.g. the legacy 4-part FSE literal layout
        archives++;
        std::vector<uint8_t> T(P.temp_bytes), out(n + 64);
        Exec X(P, A.second.data(), T); X.run(nullptr, out.data()); nvec16 += X.vec16; nvcopy += X.vcopy;
        {   // block-range plans (aceapex_gpu_plan_create_blocks): 5 batches of blocks, each its own plan and temp, the jobs reading the
            // archive slice [in_lo, in_hi), the windows concatenated == the original
            const uint32_t K = std::max<uint32_t>(1, (P.nb + 4) / 5); std::vector<uint8_t> cat(n + 64); bool bb = false; uint64_t pos = 0;
            for (uint32_t b0 = 0; b0 < P.nb && !bb; b0 += K) { const uint32_t b1 = std::min(P.nb, b0 + K); agp::Plan Q;
                if (agp::build(A.second.data(), A.second.size(), Q, nvt_cpu, b0, b1) || Q.in_hi > A.second.size() || pos + Q.orig > n) { bb = true; break; }
                std::vector<uint8_t> T2(Q.temp_bytes); Exec Y(Q, A.second.data() + Q.in_lo, T2); Y.run(nullptr, cat.data() + pos); pos += Q.orig; nbatch++;
                if (Y.err) bb = true; }
            if (bb || pos != n || memcmp(cat.data(), ref.data(), n)) { bad++; fails += " batch:" + A.first; continue; }
        }
        if (P.tile_ok) {                                  // AX_GPU_TILE: the same bytes from the tile schedule, full and 10 ranges
            ntile_arch++;
            std::vector<uint8_t> T1(P.temp_bytes), o1(n + 64); Exec X1(P, A.second.data(), T1); X1.tile = true; X1.run(nullptr, o1.data()); ntiles += X1.tiles;
            if (X1.err || memcmp(o1.data(), ref.data(), n)) { bad++; fails += " tile:" + A.first; continue; }
            for (int r = 0; r < 10; r++) { uint64_t len = 1 + rng() % std::min<uint64_t>(n, r & 1 ? 65536 : 17), off = rng() % (n - len + 1); agp::Sel S;
                if (agp::select(P, off, len, S)) continue;
                std::vector<uint8_t> T2(P.temp_bytes, 0), win(agp::window_bytes(P, len), 0xA5); Exec Y(P, A.second.data(), T2); Y.tile = true;
                Y.run(&S, win.data() - (uint64_t)S.b0 * P.bs); ntile_rng++;
                if (Y.err || memcmp(win.data() + S.win_off, ref.data() + off, len)) { rbad++; fails += " tile-range:" + A.first; break; } }
        }
        if (X.err || memcmp(out.data(), ref.data(), n)) { bad++; fails += " full:" + A.first; continue; }
        for (int r = 0; r < 40; r++) {
            uint64_t len = std::min<uint64_t>(n, 1 + rng() % std::min<uint64_t>(n, r % 4 == 0 ? 17 : r % 4 == 1 ? 16384 : r % 4 == 2 ? 65536 : 1u << 20));
            uint64_t off = rng() % (n - len + 1); agp::Sel S; ranges++;
            if (agp::select(P, off, len, S)) { rbad++; fails += " sel:" + A.first; break; }
            std::vector<uint8_t> T2(P.temp_bytes, 0), win(agp::window_bytes(P, len), 0xA5);
            Exec Y(P, A.second.data(), T2); Y.run(&S, win.data() - (uint64_t)S.b0 * P.bs);
            if (Y.err || memcmp(win.data() + S.win_off, ref.data() + off, len)) { rbad++; fails += " range:" + A.first; break; }
        }
        for (int m = 0; m < 60; m++) {                           // mutations: refuse or finish, inside the buffers
            std::vector<uint8_t> z = A.second; size_t at = rng() % z.size(); z[at] ^= (uint8_t)(1 + rng() % 255); mut++;
            agp::Plan Q; if (agp::build(z.data(), z.size(), Q, nvt_cpu)) { mref++; continue; }
            if (Q.temp_bytes > (1ull << 31) || Q.orig > (1ull << 31)) { mref++; continue; }
            std::vector<uint8_t> T3(Q.temp_bytes), o3(Q.orig + 64);
            Exec Z(Q, z.data(), T3); Z.run(nullptr, o3.data());
        }
    }
    // 1000 byte flips of the archive's streams under the plan of the intact archive (what the device sees: the
    // plan is built once on the host, the flips are in d_in), a failed zstd frame leaving random bytes (nvCOMP's
    // output is undefined then): no match loop over its step limit, no copy outside its block
    int flips = 0; uint64_t fsteps = 0, fbound = 0, flimit = 0, foob = 0, fcaught = 0, fhash = 0, fsame = 0, fsilent = 0;
    { std::vector<size_t> ok_ix; for (size_t i = 0; i < arch.size(); i++) { agp::Plan Q; if (!agp::build(arch[i].second.data(), arch[i].second.size(), Q, nvt_cpu) && Q.orig) ok_ix.push_back(i); }
      for (int f = 0; f < 1000 && !ok_ix.empty(); f++) {
          const auto& A = arch[ok_ix[f % ok_ix.size()]].second; agp::Plan Q; agp::build(A.data(), A.size(), Q, nvt_cpu);
          const size_t s0 = 68 + 64ull * Q.nb; if (A.size() <= s0) continue;
          std::vector<uint8_t> z = A; z[s0 + rng() % (z.size() - s0)] ^= (uint8_t)(1 + rng() % 255); flips++;
          std::vector<uint8_t> T3(Q.temp_bytes), o3(Q.orig + 64);
          const uint64_t js = rng(); Exec Z(Q, z.data(), T3); std::mt19937_64 jr(js); Z.jr = &jr; Z.run(nullptr, o3.data());
          for (uint32_t b = 0; b < Q.nb; b++) fbound += agp::rd64(&Q.bo[64ull * b] + 56) + 1;
          fsteps += Z.steps; flimit += Z.limit; foob += Z.oob;
          // ACEAPEX_GPU_VERIFY_XXH3: the status flags or the hash of the output against the header; else the output must be the original
          const bool hash_bad = XXH3_64bits(o3.data(), Q.orig) != Q.xxh;
          if (Z.err) fcaught++; else if (hash_bad) fhash++;
          else { std::vector<uint8_t> ref(Q.orig); aceapex_decompress(A.data(), A.size(), ref.data(), Q.orig);
                 if (memcmp(ref.data(), o3.data(), Q.orig)) fsilent++; else fsame++; } } }
    // the wrap that the length check guards against: a varint length near 2^32 after 100 literal bytes
    uint64_t wsteps = 0, wlim = 0, woob = 0; uint32_t wout;
    { uint8_t lit[128] = {0}, off[8] = {1}, len[8] = {0xF0, 0xFF, 0xFF, 0xFF, 0x0F}, cmd[2] = {99, 0xFE}, dst[256];
      wout = Exec::match(lit, off, len, cmd, 100, 1, 5, 2, dst, 256, wsteps, wlim, woob); }
    const bool wrap_ok = wout == 100 && woob == 0 && wlim == 0;
    // ACEAPEX_GPU_VALIDATE_ZSTD: every intact archive passes; the saved T2T frame nvCOMP 5.3 hangs on passes the header
    // check and is refused; byte flips inside zstd frames: refused, or every frame of the accepted archive decodes to its size
    int vz_arch = 0, vz_bad_intact = 0, vz_flips = 0, vz_ref = 0, vz_leak = 0; uint64_t vz_frames = 0; bool vz_repro = false;
    { std::vector<size_t> zx;
      for (size_t i = 0; i < arch.size(); i++) { agp::Plan Q; if (agp::build(arch[i].second.data(), arch[i].second.size(), Q, nvt_cpu) || Q.nv.empty()) continue;
          zx.push_back(i); vz_arch++; vz_frames += Q.nv.size(); if (agp::validate_zstd(arch[i].second.data(), Q, 4)) vz_bad_intact++; }
      auto rd = [](const char* p) { std::vector<uint8_t> v; FILE* f = fopen(p, "rb");
          if (!f) return v;
          fseek(f, 0, SEEK_END); v.resize(ftell(f)); fseek(f, 0, SEEK_SET);
          if (fread(v.data(), 1, v.size(), f) != v.size()) v.clear();
          fclose(f); return v; };
      auto fo = rd("verify/repro/t2t_frame150180.orig.zst"), ff = rd("verify/repro/t2t_frame150180.flip.zst");
      if (!fo.empty() && fo.size() == ff.size()) { agp::Plan R1; R1.nv.push_back({0, fo.size(), 0, 8192}); R1.max_osz = 8192;
          vz_repro = !agp::zstd_frame_check(fo.data(), fo.size(), 8192) && !agp::zstd_frame_check(ff.data(), ff.size(), 8192)
                     && agp::validate_zstd(fo.data(), R1, 1) == 0 && agp::validate_zstd(ff.data(), R1, 1) == 1; }
      for (int f = 0; f < 400 && !zx.empty(); f++) {
          const auto& A = arch[zx[f % zx.size()]].second; agp::Plan Q; agp::build(A.data(), A.size(), Q, nvt_cpu);
          const agp::Nv& J = Q.nv[rng() % Q.nv.size()]; std::vector<uint8_t> z = A; z[J.in_off + rng() % J.csz] ^= (uint8_t)(1 + rng() % 255); vz_flips++;
          agp::Plan Q2; if (agp::build(z.data(), z.size(), Q2, nvt_cpu) || agp::validate_zstd(z.data(), Q2, 4)) { vz_ref++; continue; }
          std::vector<uint8_t> T3(Q2.temp_bytes), o3(Q2.orig + 64); Exec Z(Q2, z.data(), T3); Z.run(nullptr, o3.data()); if (Z.err & 4) vz_leak++; } }
    const bool vok = vz_arch >= 4 && vz_bad_intact == 0 && vz_repro && vz_flips == 400 && vz_leak == 0;
    printf("head_gpu_zstd_validate\t%s\t%d archives with zstd frames (%llu frames) pass the validation (%d refused); the saved T2T frame (verify/repro, nvCOMP 5.3 does not finish on it) passes the header check and is %s; %d byte flips inside zstd frames: %d archives refused, %d accepted, %d of them with a frame that does not decode to its size\n",
           vok ? "pass" : "fail", vz_arch, (unsigned long long)vz_frames, vz_bad_intact, vz_repro ? "refused" : "NOT REFUSED", vz_flips, vz_ref, vz_flips - vz_ref, vz_leak);
    const bool ok = archives >= 12 && bad == 0 && rbad == 0 && ntile_arch >= 2;
    const bool fok = flips == 1000 && flimit == 0 && foob == 0 && fsilent == 0 && wrap_ok;
    printf("head_gpu_plan_emu\t%s\t%d archives decoded through the plan bit-perfect (%d bad; AX_VEC 16-byte paths: %llu unpack stores, %llu match copies; AX_GPU_TILE: %d archives, %d ranges, %llu literal 16-byte steps, same bytes; block-range plans: %d batches, windows == original), %d ranges on a zeroed temp (%d bad), %d refused by the planner, %d mutations (%d refused, the rest ran inside their buffers)%s\n",
           ok ? "pass" : "fail", archives, bad, (unsigned long long)nvec16, (unsigned long long)nvcopy, ntile_arch, ntile_rng, (unsigned long long)ntiles, nbatch, ranges, rbad, refused, mut, mref, fails.empty() ? "" : (";" + fails).c_str());
    printf("head_gpu_flip_emu\t%s\t%d byte flips of the streams under the plan of the intact archive (as on the device), a failed zstd frame leaving random bytes: %llu match steps of %llu allowed, %llu over the step limit, %llu copies outside a block; %llu flagged by status, %llu more by XXH3, %llu decoded to the original, %llu silent; length 2^32-16 after 100 bytes %s\n",
           fok ? "pass" : "fail", flips, (unsigned long long)fsteps, (unsigned long long)fbound, (unsigned long long)flimit, (unsigned long long)foob,
           (unsigned long long)fcaught, (unsigned long long)fhash, (unsigned long long)fsame, (unsigned long long)fsilent, wrap_ok ? "refused" : "NOT REFUSED");
    return ok && fok && vok ? 0 : 1;
}
