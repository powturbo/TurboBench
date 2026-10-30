/* rans1_seg.c — dense-open v2 (измерение, формат ACEPX2 не меняется): order-1 rANS литералов,
 * раскладка под GPU. Чанк 64 KiB = NS (32) непрерывных отрезков; у каждого своё 32-битное
 * состояние и свой подпоток, контекст (предыдущий символ) сбрасывается в начале отрезка на
 * символ 0 алфавита чанка. GPU: lane = отрезок, символ — таблица slot->symbol на контекст
 * (K x 4096 B в shared), без поиска.
 * Схема rANS как в rans1_v4.c (htscodecs rans_static4x16): L = 1<<15, ренорм по 16 бит,
 * таблицы 12 бит, ремап алфавита на чанк, та же битовая упаковка order-1 таблицы.
 * Контейнер "AR2L": [AR2L][ver 1][NS][TF 12][0], затем на чанк [n u32][cs u32][чанк cs B].
 * Чанк: [flag u8: 0 rans / 1 raw][K][sym K][таблица][NS x u16 длина подпотока][подпотоки][2 B паддинг].
 * Подпоток отрезка: [x u32 LE][слова ренорма u16 в порядке декода]. Отрезок s: символы
 * [s*q, s*q+q), q = (n/NS) & ~3, последний — до n (длины кратны 4: GPU пишет словами).
 * Raw: K > KMAX или rANS не выигрывает.
 *
 * Сборка: cc -O3 -march=native -o rans1_seg rans1_seg.c -lm
 * Запуск: rans1_seg t <file> [passes] | c <in> <out> | d <in> <out> | r <file> [trials]
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define TF_BITS 12
#define TF_M    (1u << TF_BITS)
#define RANS_L  (1u << 15)
#define CHUNK   (64 * 1024)
#ifndef NS
#define NS      32
#endif
#define KMAX    64

/* ---------------- bit I/O, LSB-first ---------------- */
typedef struct { uint8_t *p; uint64_t acc; int nb; } BW;
static void bw_init(BW *w, uint8_t *p) { w->p = p; w->acc = 0; w->nb = 0; }
static void bw_put(BW *w, uint32_t v, int bits) {
    w->acc |= (uint64_t)(v & ((1u << bits) - 1)) << w->nb;
    w->nb += bits;
    while (w->nb >= 8) { *w->p++ = (uint8_t)w->acc; w->acc >>= 8; w->nb -= 8; }
}
static uint8_t *bw_flush(BW *w) {
    if (w->nb) { *w->p++ = (uint8_t)w->acc; w->acc = 0; w->nb = 0; }
    return w->p;
}
typedef struct { const uint8_t *p; uint64_t acc; int nb; } BR;
static void br_init(BR *r, const uint8_t *p) { r->p = p; r->acc = 0; r->nb = 0; }
static uint32_t br_get(BR *r, int bits) {
    while (r->nb < bits) { r->acc |= (uint64_t)(*r->p++) << r->nb; r->nb += 8; }
    uint32_t v = (uint32_t)(r->acc & ((1u << bits) - 1));
    r->acc >>= bits; r->nb -= bits;
    return v;
}
static const uint8_t *br_align(BR *r) { r->acc = 0; r->nb = 0; return r->p; }

/* -------- нормализация частот к сумме M, largest remainder -------- */
static void normalize_freqs(const uint32_t *cnt, int K, uint16_t *F) {
    uint64_t tot = 0;
    for (int i = 0; i < K; i++) tot += cnt[i];
    if (!tot) { memset(F, 0, K * sizeof *F); return; }
    uint32_t rem[KMAX];
    uint32_t sum = 0;
    for (int i = 0; i < K; i++) {
        if (!cnt[i]) { F[i] = 0; rem[i] = 0; continue; }
        uint64_t t = (uint64_t)cnt[i] * TF_M;
        F[i] = (uint16_t)(t / tot);
        rem[i] = (uint32_t)(t % tot);
        if (!F[i]) { F[i] = 1; rem[i] = 0; }
        sum += F[i];
    }
    int diff = (int)TF_M - (int)sum;
    while (diff > 0) {                       /* раздать по наибольшим остаткам */
        int best = -1; uint32_t bv = 0;
        for (int i = 0; i < K; i++)
            if (F[i] && (best < 0 || rem[i] > bv)) { bv = rem[i]; best = i; }
        F[best]++; rem[best] = 0; diff--;
    }
    while (diff < 0) {                       /* забрать у наибольших F > 1 */
        int best = -1; uint32_t bv = 0;
        for (int i = 0; i < K; i++)
            if (F[i] > 1 && (best < 0 || F[i] > bv)) { bv = F[i]; best = i; }
        F[best]--; diff++;
    }
}

/* ---------------- энкодер: символ без деления (Ryg) ---------------- */
typedef struct { uint32_t xmax, rcp, bias; uint16_t cmpl, shift; } EncSym;
static void enc_sym_init(EncSym *e, uint32_t start, uint32_t freq) {
    e->xmax = (freq << (15 + 16 - TF_BITS)) - 1;     /* ((L>>TF)<<16)*f - 1, включительно */
    e->cmpl = (uint16_t)(TF_M - freq);
    if (freq < 2) { e->rcp = ~0u; e->shift = 0; e->bias = start + TF_M - 1; }
    else {
        uint32_t sh = 0;
        while (freq > (1u << sh)) sh++;
        e->rcp = (uint32_t)(((1ull << (sh + 31)) + freq - 1) / freq);
        e->shift = (uint16_t)(sh - 1);
        e->bias = start;
    }
    e->shift += 32;                                  /* один 64-бит сдвиг в горячем цикле */
}

static void put32(uint8_t *q, uint32_t v) { q[0]=(uint8_t)v; q[1]=(uint8_t)(v>>8); q[2]=(uint8_t)(v>>16); q[3]=(uint8_t)(v>>24); }
static uint32_t get32(const uint8_t *q) { return (uint32_t)q[0]|((uint32_t)q[1]<<8)|((uint32_t)q[2]<<16)|((uint32_t)q[3]<<24); }
static int seg_q(int n) { return (n / NS) & ~3; }

/* ---------------- чанк: кодирование ---------------- */
static int enc_chunk(const uint8_t *in, int n, uint8_t *out, int *hdr_bytes) {
    *hdr_bytes = 0;
    if (n == 0) { out[0] = 1; return 1; }
    uint32_t hist[256] = {0};
    for (int i = 0; i < n; i++) hist[in[i]]++;
    int K = 0; uint8_t sym[256], map[256];
    memset(map, 0, sizeof map);
    for (int i = 0; i < 256; i++) if (hist[i]) { map[i] = (uint8_t)K; sym[K] = (uint8_t)i; K++; }
    if (K > KMAX) { out[0] = 1; memcpy(out + 1, in, n); return 1 + n; }

    const int q = seg_q(n);
    int st[NS], len[NS];
    for (int s = 0; s < NS; s++) { st[s] = s * q; len[s] = s == NS - 1 ? n - s * q : q; }

    static uint32_t cnt[KMAX][KMAX];
    memset(cnt, 0, sizeof cnt);
    for (int s = 0; s < NS; s++) { int prev = 0;
        for (int i = 0; i < len[s]; i++) { int c = map[in[st[s] + i]]; cnt[prev][c]++; prev = c; } }

    static uint16_t F[KMAX][KMAX], C[KMAX][KMAX];
    uint64_t rowused = 0;
    for (int i = 0; i < K; i++) {
        uint32_t tot = 0; for (int j = 0; j < K; j++) tot += cnt[i][j];
        if (!tot) continue;
        rowused |= 1ull << i;
        normalize_freqs(cnt[i], K, F[i]);
        uint32_t c = 0; for (int j = 0; j < K; j++) { C[i][j] = (uint16_t)c; c += F[i][j]; }
    }
    uint8_t *hp = out;
    *hp++ = 0; *hp++ = (uint8_t)K; memcpy(hp, sym, K); hp += K;
    BW bw; bw_init(&bw, hp);
    for (int i = 0; i < K; i++) bw_put(&bw, (uint32_t)(rowused >> i) & 1, 1);
    for (int i = 0; i < K; i++) {
        if (!((rowused >> i) & 1)) continue;
        for (int j = 0; j < K; j++) bw_put(&bw, F[i][j] != 0, 1);
        for (int j = 0; j < K; j++) if (F[i][j]) bw_put(&bw, F[i][j] - 1u, TF_BITS);
    }
    hp = bw_flush(&bw);
    static EncSym es[KMAX][KMAX];
    for (int i = 0; i < K; i++) { if (!((rowused >> i) & 1)) continue;
        for (int j = 0; j < K; j++) if (F[i][j]) enc_sym_init(&es[i][j], C[i][j], F[i][j]); }

    /* каждый отрезок пишется с конца своего окна scratch */
    static uint8_t scratch[NS][2 * CHUNK / NS * 2 + 4 * CHUNK + 64];
    const size_t W = sizeof scratch[0];
    int sl[NS], payload = 0;
    for (int s = 0; s < NS; s++) {
        uint8_t *end = scratch[s] + W, *sp = end;
        uint32_t x = RANS_L;
        const uint8_t *b = in + st[s];
        for (int t = len[s] - 1; t >= 0; t--) {
            uint32_t c = map[b[t]], cx = t ? map[b[t - 1]] : 0;
            const EncSym *e = &es[cx][c];
            if (x > e->xmax) { sp -= 2; sp[0] = (uint8_t)x; sp[1] = (uint8_t)(x >> 8); x >>= 16; }
            uint32_t qd = (uint32_t)(((uint64_t)x * e->rcp) >> e->shift);
            x = x + e->bias + qd * e->cmpl;
        }
        sp -= 4; put32(sp, x);
        sl[s] = (int)(end - sp); payload += sl[s];
        if (sl[s] > 65535) { out[0] = 1; memcpy(out + 1, in, n); return 1 + n; }
    }
    int hdr = (int)(hp - out) + 1 + 2 * NS;
    if (hdr + payload + 2 >= n + 1) { out[0] = 1; memcpy(out + 1, in, n); return 1 + n; }
    *hp++ = NS;
    for (int s = 0; s < NS; s++) { hp[0] = (uint8_t)sl[s]; hp[1] = (uint8_t)(sl[s] >> 8); hp += 2; }
    for (int s = 0; s < NS; s++) { memcpy(hp, scratch[s] + W - sl[s], sl[s]); hp += sl[s]; }
    hp[0] = 0; hp[1] = 0;
    *hdr_bytes = hdr;
    return hdr + payload + 2;
}

/* ---------------- чанк: декодирование ---------------- */
typedef struct {
    uint8_t ssym[KMAX][TF_M];    /* [ctx][слот] -> индекс символа */
    uint32_t fb[KMAX][KMAX];     /* [ctx][sym] -> (freq<<16)|start */
    uint8_t sym[KMAX];
} DecTab;

/* разбор таблицы; возврат: указатель на [NS][длины] */
static const uint8_t *dec_table(const uint8_t *p, DecTab *dt, int *Kp) {
    int K = *p++; *Kp = K;
    memcpy(dt->sym, p, K); p += K;
    BR br; br_init(&br, p);
    uint64_t rowused = 0;
    for (int i = 0; i < K; i++) rowused |= (uint64_t)br_get(&br, 1) << i;
    for (int i = 0; i < K; i++) {
        if (!((rowused >> i) & 1)) continue;
        uint64_t colused = 0;
        for (int j = 0; j < K; j++) colused |= (uint64_t)br_get(&br, 1) << j;
        uint32_t c = 0;
        for (int j = 0; j < K; j++) {
            if (!((colused >> j) & 1)) continue;
            uint32_t f = br_get(&br, TF_BITS) + 1;
            memset(dt->ssym[i] + c, j, f);
            dt->fb[i][j] = (f << 16) | c; c += f;
        }
    }
    return br_align(&br);
}

static int dec_chunk(const uint8_t *in, uint8_t *out, int n, DecTab *dt) {
    const uint8_t *p = in;
    if (*p++ == 1) { memcpy(out, p, n); return 0; }
    int K; p = dec_table(p, dt, &K);
    if (*p++ != NS) return -1;
    const uint8_t *pp[NS]; const uint8_t *b = p + 2 * NS;
    for (int s = 0; s < NS; s++) { pp[s] = b; b += p[2 * s] | (p[2 * s + 1] << 8); }
    const int q = seg_q(n);
    uint32_t x[NS], ctx[NS];
    for (int s = 0; s < NS; s++) { x[s] = get32(pp[s]); pp[s] += 4; ctx[s] = 0; }
    const uint8_t *sy = dt->sym;
#define DS(s, t) do {                                                    \
        uint32_t m = x[s] & (TF_M - 1), c = dt->ssym[ctx[s]][m];          \
        uint32_t v = dt->fb[ctx[s]][c];                                   \
        x[s] = (v >> 16) * (x[s] >> TF_BITS) + m - (uint16_t)v;          \
        out[(s) * q + (t)] = sy[c]; ctx[s] = c;                           \
        uint16_t w_; memcpy(&w_, pp[s], 2);                               \
        uint32_t y_ = (x[s] << 16) | w_; int r_ = x[s] < RANS_L;         \
        x[s] = r_ ? y_ : x[s]; pp[s] += r_ ? 2 : 0;                       \
    } while (0)
    for (int t = 0; t < q; t++)
        for (int s = 0; s < NS; s++) DS(s, t);
    for (int t = q; t < n - (NS - 1) * q; t++) DS(NS - 1, t);
#undef DS
    return 0;
}

/* диапазон [from,to) внутри чанка: каждый задетый отрезок — с его начала */
static int dec_range(const uint8_t *in, uint8_t *out, int n, DecTab *dt, int from, int to, int *work) {
    const uint8_t *p = in;
    if (*p++ == 1) { memcpy(out, p + from, to - from); if (work) *work = to - from; return 0; }
    int K; p = dec_table(p, dt, &K);
    if (*p++ != NS) return -1;
    const uint8_t *base[NS]; const uint8_t *b = p + 2 * NS;
    for (int s = 0; s < NS; s++) { base[s] = b; b += p[2 * s] | (p[2 * s + 1] << 8); }
    const int q = seg_q(n); int done = 0;
    for (int s = 0; s < NS; s++) {
        int lo = s * q, ln = s == NS - 1 ? n - lo : q;
        if (ln <= 0 || lo >= to || lo + ln <= from) continue;
        int e = to < lo + ln ? to - lo : ln;
        uint32_t x = get32(base[s]), ctx = 0; const uint8_t *pk = base[s] + 4;
        for (int t = 0; t < e; t++) {
            uint32_t m = x & (TF_M - 1), c = dt->ssym[ctx][m], v = dt->fb[ctx][c];
            x = (v >> 16) * (x >> TF_BITS) + m - (uint16_t)v;
            if (lo + t >= from) out[lo + t - from] = dt->sym[c];
            ctx = c; done++;
            if (x < RANS_L) { x = (x << 16) | pk[0] | (pk[1] << 8); pk += 2; }
        }
    }
    if (work) *work = done;
    return 0;
}

/* ---------------- контейнер ---------------- */
static size_t compress_all(const uint8_t *in, size_t n, uint8_t *out, size_t *tables, int *raw_chunks) {
    uint8_t *op = out;
    memcpy(op, "AR2L", 4); op[4] = 1; op[5] = NS; op[6] = TF_BITS; op[7] = 0; op += 8;
    size_t tb = 0; int rc = 0;
    for (size_t off = 0; off < n; off += CHUNK) {
        int cn = (int)((n - off < CHUNK) ? n - off : CHUNK), hb = 0;
        int cs = enc_chunk(in + off, cn, op + 8, &hb);
        put32(op, (uint32_t)cn); put32(op + 4, (uint32_t)cs);
        tb += (size_t)hb; if (op[8] == 1) rc++;
        op += 8 + cs;
    }
    if (tables) *tables = tb;
    if (raw_chunks) *raw_chunks = rc;
    return (size_t)(op - out);
}
static long decompress_all(const uint8_t *in, size_t clen, uint8_t *out, size_t cap, DecTab *dt) {
    if (clen < 8 || memcmp(in, "AR2L", 4) || in[5] != NS || in[6] != TF_BITS) return -1;
    const uint8_t *p = in + 8, *end = in + clen; size_t n = 0;
    while (p + 8 <= end) {
        uint32_t cn = get32(p), cs = get32(p + 4); p += 8;
        if (p + cs > end || n + cn > cap) return -1;
        if (dec_chunk(p, out + n, (int)cn, dt) < 0) return -1;
        p += cs; n += cn;
    }
    return (long)n;
}

/* ---------------- утилиты ---------------- */
static double now_s(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec * 1e-9; }
static uint8_t *read_file(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb"); if (!f) { perror(path); exit(1); }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *b = malloc((size_t)sz + 16);
    if (!b || fread(b, 1, (size_t)sz, f) != (size_t)sz) { fprintf(stderr, "read failed: %s\n", path); exit(1); }
    memset(b + sz, 0, 16); fclose(f); *n = (size_t)sz; return b;
}
static void write_file(const char *path, const uint8_t *b, size_t n) {
    FILE *f = fopen(path, "wb"); if (!f) { perror(path); exit(1); }
    if (fwrite(b, 1, n, f) != n) { fprintf(stderr, "write failed\n"); exit(1); }
    fclose(f);
}
static size_t comp_bound(size_t n) { return n + (n / CHUNK + 1) * 16 + 4096; }

static void test_mode(const char *path, int passes) {
    size_t n; uint8_t *b = read_file(path, &n);
    uint8_t *cb = malloc(comp_bound(n)), *ob = malloc(n + 1); DecTab *dt = malloc(sizeof *dt);
    size_t tables = 0; int rawc = 0;
    size_t cs = compress_all(b, n, cb, &tables, &rawc);
    double te = 1e30, td = 1e30;
    for (int p = 0; p < passes; p++) { double t0 = now_s(); compress_all(b, n, cb, NULL, NULL); double d = now_s() - t0; if (d < te) te = d; }
    long dn = decompress_all(cb, cs, ob, n, dt);
    int ok = dn == (long)n && !memcmp(b, ob, n);
    for (int p = 0; p < passes; p++) { double t0 = now_s(); decompress_all(cb, cs, ob, n, dt); double d = now_s() - t0; if (d < td) td = d; }
    size_t chunks = (n + CHUNK - 1) / CHUNK, body = cs - 8 - chunks * 8;
    printf("raw:     %zu bytes, %zu chunks (64 KiB), segments=%d\n", n, chunks, NS);
    printf("file:    %zu bytes  (%.4f bits/byte; tables+seg headers %.4f)%s\n", cs, 8.0 * body / n, 8.0 * tables / n, rawc ? "  [raw-chunks]" : "");
    printf("encode:  %.0f MB/s, decode %.0f MB/s (best of %d)\n", n / te / 1e6, n / td / 1e6, passes);
    printf("round-trip: %s\n", ok ? "MATCHES" : "DIFFERS");
    if (!ok) exit(2);
}
/* регион: случайные диапазоны внутри одного чанка против полного декода */
static void range_mode(const char *path, int trials) {
    size_t n; uint8_t *b = read_file(path, &n);
    uint8_t *cb = malloc(comp_bound(n)), *full = malloc(n + 1), *part = malloc(CHUNK); DecTab *dt = malloc(sizeof *dt);
    size_t cs = compress_all(b, n, cb, NULL, NULL);
    if (decompress_all(cb, cs, full, n, dt) != (long)n || memcmp(full, b, n)) { printf("full decode DIFFERS\n"); exit(2); }
    int RLEN = getenv("RLEN") ? atoi(getenv("RLEN")) : 16000, bad = 0; long work = 0;
    size_t nchunks = n / CHUNK; srand(12345);
    for (int i = 0; i < trials; i++) {
        size_t c = (size_t)rand() % nchunks; const uint8_t *p = cb + 8;
        for (size_t k = 0; k < c; k++) p += 8 + get32(p + 4);
        int from = rand() % (CHUNK - RLEN), w = 0;
        dec_range(p + 8, part, CHUNK, dt, from, from + RLEN, &w);
        if (memcmp(part, full + c * CHUNK + from, RLEN)) bad++;
        work += w;
    }
    printf("ranges: %d x %d B, differing: %d, mean work %.0f of %d symbols (%.1f %%)\n", trials, RLEN, bad, (double)work / trials, CHUNK, 100.0 * work / trials / CHUNK);
    if (bad) exit(2);
}
int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s c <in> <out> | d <in> <out> | t <in> [passes] | r <in> [trials]\n", argv[0]); return 1; }
    if (argv[1][0] == 't') { test_mode(argv[2], argc > 3 ? atoi(argv[3]) : 5); return 0; }
    if (argv[1][0] == 'r') { range_mode(argv[2], argc > 3 ? atoi(argv[3]) : 200); return 0; }
    if (argv[1][0] == 'c' && argc > 3) {
        size_t n; uint8_t *b = read_file(argv[2], &n), *cb = malloc(comp_bound(n));
        size_t cs = compress_all(b, n, cb, NULL, NULL); write_file(argv[3], cb, cs);
        printf("%zu -> %zu (%.4f bits/byte)\n", n, cs, n ? 8.0 * cs / n : 0.0); return 0;
    }
    if (argv[1][0] == 'd' && argc > 3) {
        size_t cn; uint8_t *cb = read_file(argv[2], &cn); size_t cap = 0;
        for (const uint8_t *p = cb + 8; p + 8 <= cb + cn; p += 8 + get32(p + 4)) cap += get32(p);
        uint8_t *ob = malloc(cap + 1); DecTab *dt = malloc(sizeof *dt);
        long dn = decompress_all(cb, cn, ob, cap, dt);
        if (dn < 0) { fprintf(stderr, "bad stream\n"); return 2; }
        write_file(argv[3], ob, (size_t)dn); printf("%zu -> %ld\n", cn, dn); return 0;
    }
    fprintf(stderr, "unknown mode\n"); return 1;
}
