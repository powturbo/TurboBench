// linemodel_test.cpp - claim head_linemodel: AX_LINEMODEL (src/ax_linemodel.h, tuning builds) gives every input back
// byte for byte - regular FASTA, irregular lines with empty lines and a header-only record and no final '\n', text
// without headers, one long line - through the full decode (1 and 4 threads) and 300 regions each; the container is
// smaller than the plain archive on the FASTA. Run once as is (fused sink on the tile path) and once with AX_LIT_TILE=0
// (the two-pass path; AX_LIT_TILE is read once per process).
// Build: g++ -std=c++17 -O2 -DACEAPEX_ENV_TUNING -Isrc scripts/linemodel_test.cpp src/aceapex_api.cpp -lzstd -lpthread
#include "aceapex.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>
int main() {
    std::mt19937_64 g(99); const char* B = "ACGT"; std::vector<std::pair<std::string, std::string>> in;
    auto seqline = [&](std::string& s, size_t n, bool lower) { for (size_t i = 0; i < n; i++) s += (char)(B[g() & 3] | (lower ? 0x20 : 0)); };
    { std::string s; std::string motif; seqline(motif, 3000, false);
      for (int r = 0; r < 4; r++) { s += ">rec" + std::to_string(r) + " test\n"; size_t n = 900000 + g() % 300000;
          std::string q; while (q.size() < n) { if (g() % 4 == 0) q += motif; else seqline(q, 200, g() % 3 == 0); }
          q.resize(n); for (size_t i = 0; i < n; i += 60) s += q.substr(i, 60) + "\n"; }
      in.push_back({"regular FASTA", s}); }
    { std::string s = ">a\n"; for (int i = 0; i < 20000; i++) { std::string l; seqline(l, g() % 3 ? 70 : g() % 140, g() % 5 == 0); s += l + "\n"; if (g() % 500 == 0) s += "\n"; if (g() % 3000 == 0) s += ">empty record\n>next\n"; }
      s += "ACGTN"; in.push_back({"irregular, empty lines, no final newline", s}); }
    { std::string s; for (int i = 0; i < 30000; i++) { std::string l; for (int k = 0, n = (int)(g() % 90); k < n; k++) l += (char)('a' + g() % 26); s += l + "\n"; } in.push_back({"text lines", s}); }
    { std::string s; seqline(s, 500000, false); in.push_back({"one line", s}); }
    int bad = 0; std::string note; size_t lm_fa = 0, plain_fa = 0;
    for (auto& [nm, s] : in) {
        std::vector<uint8_t> z(aceapex_compress_bound(s.size()) + 4096), o(s.size() + 64);
        unsetenv("AX_LINEMODEL"); const int64_t zp = aceapex_compress(s.data(), s.size(), z.data(), z.size(), 2, 4);
        setenv("AX_LINEMODEL", "1", 1); const int64_t zn = aceapex_compress(s.data(), s.size(), z.data(), z.size(), 2, 4); unsetenv("AX_LINEMODEL");
        if (zn <= 0 || memcmp(z.data(), "AXLINE01", 8)) { bad++; note += " " + nm + ": compress"; continue; }
        if (nm == "regular FASTA") { lm_fa = (size_t)zn; plain_fa = (size_t)zp; }
        for (int T : {1, 4}) { const int64_t r = aceapex_decompress_mt(z.data(), (size_t)zn, o.data(), o.size(), T);
            if (r != (int64_t)s.size() || memcmp(o.data(), s.data(), s.size())) { bad++; note += " " + nm + ": full T" + std::to_string(T); } }
        for (int i = 0; i < 300; i++) { const uint64_t len = 1 + g() % 9000, off = g() % (s.size() - std::min<size_t>(len, s.size() - 1)); const uint64_t L = std::min<uint64_t>(len, s.size() - off);
            const int64_t r = aceapex_decompress_region(z.data(), (size_t)zn, o.data(), o.size(), off, L);
            if (r != (int64_t)L || memcmp(o.data(), s.data() + off, L)) { bad++; note += " " + nm + ": region"; break; } }
    }
    const bool pass = !bad && lm_fa && lm_fa < plain_fa;
    printf("head_linemodel\t%s\tAX_LINEMODEL: 4 inputs (regular FASTA, irregular lines + empty lines + no final newline, text, one line) back byte for byte, full decode 1 / 4 threads and 300 regions each; FASTA %zu B against %zu B plain%s\n",
           pass ? "pass" : "fail", lm_fa, plain_fa, note.c_str());
    return pass ? 0 : 1;
}
