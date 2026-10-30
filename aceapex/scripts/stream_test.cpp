// stream_test.cpp - claim head_stream: the streaming decoder (aceapex_decompress_stream) gives the original byte for
// byte and its peak memory does not grow with the archive. A DNA-like input of N MiB (runs of bases, soft-masked
// stretches, N runs, 60-column lines) is encoded in memory with the default profile (DNA: 16 KiB blocks, 64 KiB literal
// chunks) and written to a file; a child process (this program re-executed, a fresh address space) streams it back through pread + a hashing sink (XXH3 of the
// handed-out bytes against XXH3 of the input, and the archive header's hash through ACEAPEX_STREAM_VERIFY) and its
// peak RSS is read from wait4. Sizes 64 and 512 MiB on the default thread budget: both bit-perfect, and the 512 MiB
// peak at most 8 MiB above the 64 MiB peak (the buffers are threads x tile and threads x token window - a window is
// full from ~16 MiB on - the chunk tables grow by ~15 B per 64 KiB of literals).
// Build: g++ -std=c++17 -O2 -DACEAPEX_ENV_TUNING -Isrc scripts/stream_test.cpp src/aceapex_api.cpp -lzstd -lpthread
#define XXH_INLINE_ALL
#include "xxhash.h"
#include "aceapex.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
struct Sink { XXH3_state_t* st; uint64_t n; };
static int64_t rd(void* c, uint64_t off, void* b, size_t n) { return pread(*(int*)c, b, n, (off_t)off); }
static int wr(void* c, const void* b, size_t n) { Sink* s = (Sink*)c; XXH3_64bits_update(s->st, b, n); s->n += n; return 0; }
static std::vector<uint8_t> dna(size_t n, uint64_t seed) {
    std::mt19937_64 r(seed); std::vector<uint8_t> v; v.reserve(n + 64); std::string line;
    const char* B = "ACGT"; size_t col = 0; bool lower = false; size_t left = 0;
    std::vector<uint8_t> motif(300); for (auto& m : motif) m = B[r() & 3];
    while (v.size() < n) {
        if (!left) { const uint64_t k = r() % 100; lower = k < 30; left = 50 + r() % 4000;
            if (k >= 97) { for (size_t i = 0; i < 200 && v.size() < n; i++) { v.push_back('N'); if (++col == 60) { v.push_back('\n'); col = 0; } } continue; }
            if (k >= 90) { const size_t at = r() % motif.size(); for (size_t i = 0; i < 300 && v.size() < n; i++) { uint8_t c = motif[(at + i) % motif.size()]; v.push_back(lower ? c | 0x20 : c); if (++col == 60) { v.push_back('\n'); col = 0; } } continue; } }
        uint8_t c = B[r() & 3]; v.push_back(lower ? c | 0x20 : c); left--;
        if (++col == 60) { v.push_back('\n'); col = 0; }
    }
    v.resize(n); return v;
}
int main(int argc, char** argv) {
    if (argc == 4 && !strcmp(argv[1], "--child")) {                      // child: stream the archive, report
        int afd = open(argv[2], O_RDONLY); if (afd < 0) return 2; Sink s{XXH3_createState(), 0}; XXH3_64bits_reset(s.st);
        const int64_t n = aceapex_decompress_stream(rd, &afd, wr, &s, 0, ACEAPEX_STREAM_VERIFY);
        const uint64_t h = XXH3_64bits_digest(s.st); const uint64_t want = strtoull(argv[3], 0, 16);
        return n > 0 && (uint64_t)n == s.n && h == want ? 0 : 1;
    }
    const size_t sizes[2] = {(size_t)64 << 20, (size_t)512 << 20}; long rss[2] = {0, 0}; bool ok[2] = {false, false}; std::string note;
    for (int k = 0; k < 2; k++) {
        std::vector<uint8_t> in = dna(sizes[k], 7 + k);
        std::vector<uint8_t> z(aceapex_compress_bound(in.size()));
        const int64_t zs = aceapex_compress(in.data(), in.size(), z.data(), z.size(), 2, 4);
        if (zs <= 0) { note += " encode failed"; continue; }
        char path[] = "/tmp/ax_stream_XXXXXX"; int fd = mkstemp(path); if (fd < 0) { note += " mkstemp"; continue; }
        if (write(fd, z.data(), (size_t)zs) != zs) { note += " write"; close(fd); unlink(path); continue; }
        close(fd);
        const uint64_t want = XXH3_64bits(in.data(), in.size());
        { std::vector<uint8_t>().swap(in); std::vector<uint8_t>().swap(z); }   // the parent's big buffers go before the fork: the child's
        char hex[32]; snprintf(hex, sizeof hex, "%llx", (unsigned long long)want);
        const pid_t pid = fork();
        if (pid == 0) { execl("/proc/self/exe", argv[0], "--child", path, hex, (char*)nullptr); _exit(3); }
        int st = 0; rusage ru; wait4(pid, &st, 0, &ru); if (getenv("KEEP_ARCHIVE")) fprintf(stderr, "kept %s\n", path); else unlink(path);
        ok[k] = WIFEXITED(st) && WEXITSTATUS(st) == 0; rss[k] = ru.ru_maxrss;
    }
    const long grow = rss[1] - rss[0]; const bool pass = ok[0] && ok[1] && grow <= 8 * 1024;
    printf("head_stream\t%s\tstreaming decode (pread + hashing sink, default thread budget): 64 MiB %s, 512 MiB %s; peak RSS %ld -> %ld KiB (%+ld KiB, limit +8192: the memory is threads x tile, not the archive)%s\n",
           pass ? "pass" : "fail", ok[0] ? "bit-perfect" : "WRONG", ok[1] ? "bit-perfect" : "WRONG", rss[0], rss[1], grow, note.c_str());
    return pass ? 0 : 1;
}
