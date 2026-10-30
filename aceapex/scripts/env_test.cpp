// env_test.cpp - the library built WITHOUT ACEAPEX_ENV_TUNING (as inside lzbench) must not read the environment
// (lzbench #336): the tuning variables set to non-default values change nothing. Five rows (no variable; ACEAPEX_BS;
// AX_PROFILE=open; the encoder knobs AX_ENC/AX_HLOG/AX_MINL/AX_SKIP/AX_ATT/AX_NOFLAT; the stream knobs FSE_CHUNK/
// LIT_CHUNK/LIT_LANES/LIT_LEVEL/AX_TOK/AX_LIT/LIT_LANES_DEC/ACEAPEX_DUMP) compress the same inputs at level 1 with one
// thread (lzbench -I1) and must give the same bytes, and decode them with one thread; a sixth row sets the decoder
// knobs of 2.3 (AX_LIT_TILE, AX_NT*, AX_HUGE, AX_PREFAULT, AX_RANS_SIMD, AX_PHASE_TIMES, AX_TILE_CHUNKS). Run under strace by
// scripts/cdec_test.sh: no thread may be started (LIT_LANES_DEC used to start them at threads=1).
// Inputs: a text buffer built here, and the files given on the command line (silesia/xml when present).
// Build: g++ -std=c++17 -O2 -Isrc scripts/env_test.cpp src/aceapex_api.cpp -lzstd -lpthread   (no ACEAPEX_ENV_TUNING)
#include "aceapex.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#ifdef ACEAPEX_ENV_TUNING
#error "env_test checks the library built without ACEAPEX_ENV_TUNING"
#endif
static const char* ROWS[6][9] = {
    {nullptr},
    {"ACEAPEX_BS=16384", nullptr},
    {"AX_PROFILE=open", nullptr},
    {"AX_ENC=chain", "AX_HLOG=12", "AX_MINL=8", "AX_SKIP=1", "AX_ATT=4", "AX_NOFLAT=0", nullptr},
    {"FSE_CHUNK=4096", "LIT_CHUNK=65536", "LIT_LANES=8", "LIT_LEVEL=19", "AX_TOK=rans", "AX_LIT=open", "LIT_LANES_DEC=8", "ACEAPEX_DUMP=1", nullptr},
    {"AX_LIT_TILE=0", "AX_NT=1", "AX_NT_THREADS=1", "AX_HUGE=0", "AX_PREFAULT=1", "AX_RANS_SIMD=512", "AX_PHASE_TIMES=1", "AX_TILE_CHUNKS=1", nullptr}};
int main(int argc, char** argv) {
    std::vector<std::pair<std::string, std::vector<uint8_t>>> in;
    { std::string t; for (int i = 0; t.size() < (3u << 20); i++) t += "<record id=\"" + std::to_string(i * 7919 % 10007) + "\">the quick brown fox</record>\n";
      in.push_back({"text", std::vector<uint8_t>(t.begin(), t.end())}); }
    for (int i = 1; i < argc; i++) { FILE* f = fopen(argv[i], "rb"); if (!f) continue; std::vector<uint8_t> v; fseek(f, 0, SEEK_END); v.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET);
        if (fread(v.data(), 1, v.size(), f) == v.size()) in.push_back({argv[i], v}); fclose(f); }
    int bad = 0; std::string sizes;
    for (auto& I : in) {
        std::vector<uint8_t> ref;
        for (int r = 0; r < 6; r++) {
            for (int k = 0; ROWS[r][k]; k++) putenv((char*)ROWS[r][k]);
            std::vector<uint8_t> z(aceapex_compress_bound(I.second.size())), d(I.second.size() + 8);
            const int64_t zs = aceapex_compress(I.second.data(), I.second.size(), z.data(), z.size(), 1, 1);
            if (zs <= 0) bad++; else z.resize((size_t)zs);
            if (r == 0) { ref = z; sizes += " " + I.first.substr(I.first.find_last_of('/') + 1) + " " + std::to_string(zs); }
            else if (z != ref) bad++;
            if (aceapex_decompress_mt(z.data(), z.size(), d.data(), d.size(), 1) != (int64_t)I.second.size() || memcmp(d.data(), I.second.data(), I.second.size())) bad++;
            for (int k = 0; ROWS[r][k]; k++) { std::string e(ROWS[r][k]); unsetenv(e.substr(0, e.find('=')).c_str()); }
        }
    }
    printf("%d %s\n", bad, sizes.c_str());
    return bad ? 1 : 0;
}
