/* rans1.c — order-1 interleaved rANS для литерального потока ACEAPEX.
 *
 * Схема — htscodecs rans_static4x16 (rANS_word.h):
 *   N чередующихся 32-битных состояний, L = 1<<15, ренорм по 16 бит,
 *   таблицы 12 бит (M = 4096), контекст = предыдущий символ,
 *   ремап алфавита в 0..K-1 на чанк, чанк 64 KiB.
 * Таблица в потоке: битмап строк (K бит) + на использованную строку
 *   битмап колонок (K бит) + 12 бит (F-1) на ненулевой переход.
 * Энкодер без деления: обратные величины Ryg (rans_byte.h RansEncSymbolInit).
 * Fallback: raw-чанк при K > 64 или если сжатие не выигрывает.
 * Декодер читает спекулятивно до 2 байт вперёд: rans-чанк несёт 2 байта
 * паддинга в конце payload, буфер целого чанка самодостаточен.
 *
 * Сборка:  cc -O3 -o rans1 rans1.c -lm        (число линий: -DRANS_N=8)
 * Запуск:  rans1 t <file> [passes]   — размер + скорость + round-trip
 *          rans1 h <file>            — почанковые H0/H1 (сверка 3.024/2.099)
 *          rans1 c <in> <out> / d <in> <out>
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifndef RANS_N
#define RANS_N 4   /* N энкодера; на ace-core (Zen 4) N=4 лучше 8/16 по всем осям.
                      Декодер читает N из заголовка потока и понимает 4/8/16. */
#endif
#define TF_BITS 12
#define TF_M    (1u << TF_BITS)
#define RANS_L  (1u << 15)
#define CHUNK   (64 * 1024)
/* v4: чекпоинты внутри линии. Каждые CKPT символов сохраняем состояние rANS,
 * смещение в подпотоке и контекст — тогда вход в середину линии стоит CKPT
 * символов вместо всей линии. 9 байт на чекпоинт. */
#ifndef CKPT
#define CKPT    4096
#endif
#define MAXCK   16
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

/* ---------------- чанк: кодирование ---------------- */
/* out: [flags u8][K u8][sym K][таблицы bit-packed][payload]; возврат: размер.
 * flags: 0 = rans, 1 = raw. hdr_bytes: размер до payload (учёт таблиц). */
static void put32le(uint8_t *q, uint32_t v) {
    q[0] = (uint8_t)v; q[1] = (uint8_t)(v >> 8);
    q[2] = (uint8_t)(v >> 16); q[3] = (uint8_t)(v >> 24);
}

static int enc_chunk(const uint8_t *in, int n, uint8_t *out, int *hdr_bytes) {
    *hdr_bytes = 0;
    if (n == 0) { out[0] = 1; return 1; }

    uint32_t hist[256] = {0};
    for (int i = 0; i < n; i++) hist[in[i]]++;
    int K = 0; uint8_t sym[256], map[256];
    memset(map, 0, sizeof map);
    for (int i = 0; i < 256; i++)
        if (hist[i]) { map[i] = (uint8_t)K; sym[K] = (uint8_t)i; K++; }
    if (K > KMAX) { out[0] = 1; memcpy(out + 1, in, n); return 1 + n; }

    int q = n / RANS_N, st[RANS_N], len[RANS_N];
    for (int j = 0; j < RANS_N; j++) {
        st[j] = j * q;
        len[j] = (j == RANS_N - 1) ? n - j * q : q;
    }

    static uint32_t cnt[KMAX][KMAX];
    memset(cnt, 0, sizeof cnt);
    for (int j = 0; j < RANS_N; j++) {
        int prev = 0;
        for (int i = 0; i < len[j]; i++) {
            int c = map[in[st[j] + i]];
            cnt[prev][c]++; prev = c;
        }
    }

    static uint16_t F[KMAX][KMAX], C[KMAX][KMAX];
    uint64_t rowused = 0;
    for (int i = 0; i < K; i++) {
        uint32_t tot = 0;
        for (int j = 0; j < K; j++) tot += cnt[i][j];
        if (!tot) continue;
        rowused |= 1ull << i;
        normalize_freqs(cnt[i], K, F[i]);
        uint32_t c = 0;
        for (int j = 0; j < K; j++) { C[i][j] = (uint16_t)c; c += F[i][j]; }
    }

    uint8_t *hp = out;
    *hp++ = 0; *hp++ = (uint8_t)K;
    memcpy(hp, sym, K); hp += K;
    BW bw; bw_init(&bw, hp);
    for (int i = 0; i < K; i++) bw_put(&bw, (uint32_t)(rowused >> i) & 1, 1);
    for (int i = 0; i < K; i++) {
        if (!((rowused >> i) & 1)) continue;
        for (int j = 0; j < K; j++) bw_put(&bw, F[i][j] != 0, 1);
        for (int j = 0; j < K; j++)
            if (F[i][j]) bw_put(&bw, F[i][j] - 1u, TF_BITS);
    }
    hp = bw_flush(&bw);

    static EncSym es[256][256];        /* индексация сырыми байтами, как в эталоне */
    for (int i = 0; i < K; i++) {
        if (!((rowused >> i) & 1)) continue;
        for (int j = 0; j < K; j++)
            if (F[i][j]) enc_sym_init(&es[sym[i]][sym[j]], C[i][j], F[i][j]);
    }
    uint8_t sym0 = sym[0];             /* ремап-контекст 0 = наименьший байт */

    /* v3: раздельные подпотоки. Каждая линия пишет с конца своей четверти,
     * поэтому её байты не перемешаны с чужими и читаются независимо. */
    static uint8_t scratch[2 * CHUNK + 64];
    const size_t SEG = sizeof scratch / RANS_N;
    uint8_t *sp[RANS_N], *seg_end[RANS_N];
    for (int j = 0; j < RANS_N; j++) {
        seg_end[j] = scratch + (size_t)(j + 1) * SEG;
        sp[j] = seg_end[j];
    }
    uint32_t x[RANS_N];
    for (int j = 0; j < RANS_N; j++) x[j] = RANS_L;

    /* обратный порядок: зеркало порядка декодера (t возр., j возр.).
     * Сначала хвост последней линии, потом основной блок; ренорм без ветвления. */
    /* cs[k] — сырой байт позиции t; при обратном ходе контекст шага t
     * равен символу шага t-1: одна загрузка входа на символ, без ремапа. */
    uint32_t cs[RANS_N];
    static uint32_t ck_x[RANS_N][MAXCK], ck_d[RANS_N][MAXCK];
    static uint8_t  ck_c[RANS_N][MAXCK];
    int nck = q / CKPT - 1; if (nck < 0) nck = 0; if (nck > MAXCK) nck = MAXCK;
#define EL(k) do {                                                        \
        uint32_t s_  = cs[k];                                             \
        uint32_t cx_ = t ? (uint32_t)in[st[k] + t - 1] : (uint32_t)sym0;  \
        const EncSym *e = &es[cx_][s_];                                   \
        uint32_t xx = x[k];                                               \
        int c_ = (xx > e->xmax) * 2;                                      \
        memcpy(sp[k] - 2, &xx, 2);          /* безусловный store, эталон */  \
        xx >>= c_ * 8;                                                    \
        sp[k] -= c_;                                                         \
        uint32_t qd = (uint32_t)(((uint64_t)xx * e->rcp) >> e->shift);    \
        x[k] = xx + e->bias + qd * e->cmpl;                               \
        cs[k] = cx_;                                                      \
    } while (0)
    if (len[RANS_N - 1] > q) {
        cs[RANS_N - 1] = in[st[RANS_N - 1] + len[RANS_N - 1] - 1];
        for (int t = len[RANS_N - 1] - 1; t >= q; t--) EL(RANS_N - 1);
    }
    if (q > 0) {
        for (int j = 0; j < RANS_N; j++) cs[j] = in[st[j] + q - 1];
        int t = q - 1;
#if RANS_N == 4
        for (; t >= 0; t--) {
            EL(3); EL(2); EL(1); EL(0);
            if (t > 0 && (t % CKPT) == 0) {
                int ci = t / CKPT - 1;
                if (ci < MAXCK) for (int j = 0; j < RANS_N; j++) {
                    ck_x[j][ci] = x[j];
                    ck_d[j][ci] = (uint32_t)(seg_end[j] - sp[j]);
                    ck_c[j][ci] = in[st[j] + t - 1];
                }
            }
        }
#elif RANS_N == 8
        for (; t >= 0; t--) {
            EL(7); EL(6); EL(5); EL(4); EL(3); EL(2); EL(1); EL(0);
        }
#elif RANS_N == 16
        for (; t >= 0; t--) {
            EL(15); EL(14); EL(13); EL(12); EL(11); EL(10); EL(9); EL(8);
            EL(7); EL(6); EL(5); EL(4); EL(3); EL(2); EL(1); EL(0);
        }
#else
        for (; t >= 0; t--)
            for (int j = RANS_N - 1; j >= 0; j--) EL(j);
#endif
    }
#undef EL
    for (int j = 0; j < RANS_N; j++) {
        sp[j] -= 4;
        sp[j][0] = (uint8_t)x[j];         sp[j][1] = (uint8_t)(x[j] >> 8);
        sp[j][2] = (uint8_t)(x[j] >> 16); sp[j][3] = (uint8_t)(x[j] >> 24);
    }

    int sl[RANS_N], payload = 0;
    for (int j = 0; j < RANS_N; j++) {
        sl[j] = (int)(seg_end[j] - sp[j]);
        payload += sl[j];
    }
    int hdr = (int)(hp - out) + 4 * RANS_N + 1 + 9 * RANS_N * nck;
    if (hdr + payload + 2 >= n + 1) { out[0] = 1; memcpy(out + 1, in, n); return 1 + n; }
    for (int j = 0; j < RANS_N; j++) { put32le(hp, (uint32_t)sl[j]); hp += 4; }
    *hp++ = (uint8_t)nck;
    for (int j = 0; j < RANS_N; j++)
        for (int c = 0; c < nck; c++) {
            put32le(hp, ck_x[j][c]);                  hp += 4;
            put32le(hp, (uint32_t)(sl[j] - ck_d[j][c])); hp += 4;
            *hp++ = ck_c[j][c];
        }
    for (int j = 0; j < RANS_N; j++) { memcpy(hp, sp[j], sl[j]); hp += sl[j]; }
    hp[0] = 0; hp[1] = 0;                      /* паддинг спекулятивного чтения */
    *hdr_bytes = hdr;
    return hdr + payload + 2;
}

/* ---------------- чанк: декодирование ---------------- */
typedef struct {
    uint8_t ssym[256][TF_M];     /* [сырой ctx][слот m] -> сырой символ */
    uint32_t fb[256][256];       /* [сырой ctx][сырой sym] -> (freq<<16)|start */
} DecTab;

static int dec_core4(const uint8_t *p, uint8_t *out, int n, DecTab *dt, uint8_t s0);
static int dec_core8(const uint8_t *p, uint8_t *out, int n, DecTab *dt, uint8_t s0);
static int dec_core16(const uint8_t *p, uint8_t *out, int n, DecTab *dt, uint8_t s0);

static int dec_chunk(const uint8_t *in, int clen, uint8_t *out, int n, DecTab *dt, int N) {
    (void)clen;
    const uint8_t *p = in;
    if (*p++ == 1) { memcpy(out, p, n); return 0; }

    int K = *p++;
    uint8_t sym[KMAX];
    memcpy(sym, p, K); p += K;
    BR br; br_init(&br, p);
    uint64_t rowused = 0;
    for (int i = 0; i < K; i++) rowused |= (uint64_t)br_get(&br, 1) << i;
    for (int i = 0; i < K; i++) {
        if (!((rowused >> i) & 1)) continue;
        uint64_t colused = 0;
        for (int j = 0; j < K; j++) colused |= (uint64_t)br_get(&br, 1) << j;
        uint32_t c = 0;
        uint8_t *row = dt->ssym[sym[i]];
        uint32_t *fbr = dt->fb[sym[i]];
        for (int j = 0; j < K; j++) {
            if (!((colused >> j) & 1)) continue;
            uint32_t f = br_get(&br, TF_BITS) + 1;
            memset(row + c, sym[j], f);
            fbr[sym[j]] = (f << 16) | c;
            c += f;
        }
    }
    p = br_align(&br);
    switch (N) {
    case 4:  return dec_core4(p, out, n, dt, sym[0]);
    case 8:  return dec_core8(p, out, n, dt, sym[0]);
    case 16: return dec_core16(p, out, n, dt, sym[0]);
    default: return -1;
    }
}

/* v4: декод диапазона [from,to) внутри чанка. Для каждой задетой линии
 * берём ближайший чекпоинт перед началом её части диапазона и разворачиваем
 * оттуда. Возвращает число реально раскодированных символов — это и есть
 * цена региона, ради которой всё делалось. */
static int dec_range_core(const uint8_t *p, uint8_t *out, int n, DecTab *dt,
                          uint8_t sym0, int N, int from, int to, int *work);

static uint32_t get32le(const uint8_t *q) {
    return (uint32_t)q[0] | ((uint32_t)q[1] << 8)
         | ((uint32_t)q[2] << 16) | ((uint32_t)q[3] << 24);
}

/* ядро: N — компайл-константа в каждом инстансе (always_inline + подстановка) */
static inline __attribute__((always_inline))
int dec_core(const uint8_t *p, uint8_t *out, int n, DecTab *dt,
             uint8_t sym0, const int N) {
    /* v3: N раздельных подпотоков. Заголовок несёт N длин, дальше потоки
     * подряд; состояние линии лежит в начале её собственного потока. */
    const uint8_t *pp[16];
    {
        int sl[16];
        for (int j = 0; j < N; j++) { sl[j] = (int)get32le(p); p += 4; }
        int nck = *p++;
        p += 9 * N * nck;            /* чекпоинты: для полного декода не нужны */
        const uint8_t *b = p;
        for (int j = 0; j < N; j++) { pp[j] = b; b += sl[j]; }
    }
    uint32_t x[16];
    for (int j = 0; j < N; j++) {
        x[j] = get32le(pp[j]);
        pp[j] += 4;
    }
    int q = n / N, len_last = n - (N - 1) * q;
    uint32_t ctx[16];
    uint8_t *o[16];
    for (int j = 0; j < N; j++) {
        ctx[j] = sym0;
        o[j] = out + j * q;
    }

    /* Символьный шаг и ренорм разнесены как в эталоне: сначала все линии
     * делают шаг, затем группа безветвевых ренормов (cmov, спекулятивное
     * чтение 2 байт — покрыто паддингом чанка). */
#define DSYM(k) do {                                                \
        uint32_t m = x[k] & (TF_M - 1);                             \
        uint32_t c = dt->ssym[ctx[k]][m];                           \
        uint32_t v = dt->fb[ctx[k]][c];                             \
        x[k] = (v >> 16) * (x[k] >> TF_BITS) + m - (uint16_t)v;    \
        o[k][t] = (uint8_t)c;                                       \
        ctx[k] = c;                                                 \
    } while (0)
#ifdef __x86_64__
/* безветвевой ренорм эталона (htscodecs rANS_word.h, идея Rob Davies) */
#define DRENORM(k) do {                                             \
        uint32_t xx = x[k];                                         \
        const uint8_t *pk = pp[k];                                  \
        __asm__ ("movzwl (%0),  %%eax\n\t"                          \
                 "mov    %1,    %%edx\n\t"                          \
                 "shl    $0x10, %%edx\n\t"                          \
                 "or     %%eax, %%edx\n\t"                          \
                 "xor    %%eax, %%eax\n\t"                          \
                 "cmp    $0x8000,%1\n\t"                            \
                 "cmovb  %%edx, %1\n\t"                             \
                 "lea    2(%0), %%rax\n\t"                          \
                 "cmovb  %%rax, %0\n\t"                             \
                 : "=r" (pk), "=r" (xx)                              \
                 : "0"  (pk), "1"  (xx)                              \
                 : "eax", "edx");                                   \
        pp[k] = pk;                                                 \
        x[k] = xx;                                                  \
    } while (0)
#else
#define DRENORM(k) do {                                             \
        uint32_t xx = x[k];                                         \
        uint16_t w_; memcpy(&w_, pp[k], 2);                             \
        uint32_t y_ = (xx << 16) | w_;                              \
        x[k] = xx < RANS_L ? y_ : xx;                               \
        pp[k] += xx < RANS_L ? 2 : 0;                                   \
    } while (0)
#endif
#define DL(k) do { DSYM(k); DRENORM(k); } while (0)

    for (int t = 0; t < q; t++) {
        for (int j = 0; j < N; j++) DSYM(j);
        for (int j = 0; j < N; j++) DRENORM(j);
    }
    for (int t = q; t < len_last; t++) DL(N - 1);
#undef DL
#undef DSYM
#undef DRENORM
    return 0;
}

static int dec_range_core(const uint8_t *p, uint8_t *out, int n, DecTab *dt,
                          uint8_t sym0, int N, int from, int to, int *work) {
    int sl[16], nck;
    const uint8_t *hdr_ck;
    {
        for (int j = 0; j < N; j++) { sl[j] = (int)get32le(p); p += 4; }
        nck = *p++; hdr_ck = p; p += 9 * N * nck;
    }
    const uint8_t *base[16]; { const uint8_t *b = p;
        for (int j = 0; j < N; j++) { base[j] = b; b += sl[j]; } }

    int q = n / N, len_last = n - (N - 1) * q, done = 0;
    for (int j = 0; j < N; j++) {
        int lo = j * q, ln = (j == N - 1) ? len_last : q;
        int a = from > lo ? from - lo : 0;          /* начало внутри линии */
        int b2 = to < lo + ln ? to - lo : ln;       /* конец внутри линии  */
        if (b2 <= 0 || a >= ln) continue;           /* линия не задета     */

        /* ближайший чекпоинт не позже a */
        int ci = a / CKPT - 1; if (ci >= nck) ci = nck - 1;
        uint32_t x; const uint8_t *pk; uint32_t ctx; int t0;
        if (ci >= 0) {
            const uint8_t *e = hdr_ck + (size_t)(j * nck + ci) * 9;
            x = get32le(e); pk = base[j] + get32le(e + 4); ctx = e[8];
            t0 = (ci + 1) * CKPT;
        } else {
            x = get32le(base[j]); pk = base[j] + 4; ctx = sym0; t0 = 0;
        }
        for (int t = t0; t < b2; t++) {
            uint32_t m = x & (TF_M - 1);
            uint32_t c = dt->ssym[ctx][m];
            uint32_t v = dt->fb[ctx][c];
            x = (v >> 16) * (x >> TF_BITS) + m - (uint16_t)v;
            if (t >= a) out[lo + t - from] = (uint8_t)c;
            ctx = c; done++;
            uint16_t w_; memcpy(&w_, pk, 2);
            uint32_t y_ = (x << 16) | w_;
            if (x < RANS_L) { x = y_; pk += 2; }
        }
    }
    if (work) *work = done;
    return 0;
}

static int dec_core4(const uint8_t *p, uint8_t *out, int n, DecTab *dt, uint8_t s0)  { return dec_core(p, out, n, dt, s0, 4); }
static int dec_core8(const uint8_t *p, uint8_t *out, int n, DecTab *dt, uint8_t s0)  { return dec_core(p, out, n, dt, s0, 8); }
static int dec_core16(const uint8_t *p, uint8_t *out, int n, DecTab *dt, uint8_t s0) { return dec_core(p, out, n, dt, s0, 16); }

/* ---------------- контейнер ---------------- */
static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* обход чанков: encode всего буфера. возврат: полный размер */
static size_t compress_all(const uint8_t *in, size_t n, uint8_t *out,
                           size_t *tables, int *raw_chunks) {
    uint8_t *op = out;
    memcpy(op, "AR1L", 4); op[4] = 1; op[5] = RANS_N; op[6] = TF_BITS; op[7] = 0;
    op += 8;
    size_t tb = 0; int rc = 0;
    for (size_t off = 0; off < n || (n == 0 && off == 0); off += CHUNK) {
        int cn = (int)((n - off < CHUNK) ? n - off : CHUNK);
        if (n == 0) cn = 0;
        int hb = 0;
        int cs = enc_chunk(in + off, cn, op + 8, &hb);
        put32(op, (uint32_t)cn); put32(op + 4, (uint32_t)cs);
        tb += (size_t)hb;
        if (op[8] == 1) rc++;
        op += 8 + cs;
        if (n == 0) break;
    }
    if (tables) *tables = tb;
    if (raw_chunks) *raw_chunks = rc;
    return (size_t)(op - out);
}

static long decompress_all(const uint8_t *in, size_t clen, uint8_t *out,
                           size_t cap, DecTab *dt) {
    if (clen < 8 || memcmp(in, "AR1L", 4) || in[6] != TF_BITS)
        return -1;
    int N = in[5];
    if (N != 4 && N != 8 && N != 16)
        return -1;
    const uint8_t *p = in + 8, *end = in + clen;
    size_t n = 0;
    while (p + 8 <= end) {
        uint32_t cn = get32(p), cs = get32(p + 4);
        p += 8;
        if (p + cs > end || n + cn > cap) return -1;
        if (dec_chunk(p, (int)cs, out + n, (int)cn, dt, N) < 0) return -1;
        p += cs; n += cn;
    }
    return (long)n;
}

/* ---------------- утилиты ---------------- */
static double now_s(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}
static uint8_t *read_file(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *b = malloc((size_t)sz + 16);
    if (b) memset(b + sz, 0, 16);
    if (!b || fread(b, 1, (size_t)sz, f) != (size_t)sz) {
        fprintf(stderr, "read failed: %s\n", path); exit(1);
    }
    fclose(f); *n = (size_t)sz;
    return b;
}
static void write_file(const char *path, const uint8_t *b, size_t n) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    if (fwrite(b, 1, n, f) != n) { fprintf(stderr, "write failed\n"); exit(1); }
    fclose(f);
}
static size_t comp_bound(size_t n) {
    size_t chunks = n / CHUNK + 1;
    return n + chunks * 16 + 4096;
}

/* ---------------- почанковые H0/H1 ---------------- */
static void entropy_mode(const char *path) {
    size_t n; uint8_t *b = read_file(path, &n);
    double h0w = 0, h1w = 0; size_t n0 = 0, n1 = 0; int chunks = 0;
    for (size_t off = 0; off < n; off += CHUNK) {
        int cn = (int)((n - off < CHUNK) ? n - off : CHUNK);
        const uint8_t *c = b + off;
        uint32_t h[256] = {0};
        for (int i = 0; i < cn; i++) h[c[i]]++;
        for (int i = 0; i < 256; i++)
            if (h[i]) h0w += -(double)h[i] * log2((double)h[i] / cn);
        n0 += (size_t)cn;
        static uint32_t p2[256][256]; static uint32_t rs[256];
        memset(rs, 0, sizeof rs);
        for (int i = 1; i < cn; i++) { p2[c[i-1]][c[i]]++; rs[c[i-1]]++; }
        for (int a = 0; a < 256; a++) {
            if (!rs[a]) continue;
            for (int s2 = 0; s2 < 256; s2++)
                if (p2[a][s2]) {
                    h1w += -(double)p2[a][s2] * log2((double)p2[a][s2] / rs[a]);
                    p2[a][s2] = 0;
                }
        }
        n1 += (size_t)(cn > 0 ? cn - 1 : 0);
        chunks++;
    }
    printf("chunked H0: %.4f bits/byte\n", n0 ? h0w / n0 : 0.0);
    printf("chunked H1: %.4f bits/byte   (%d chunks, 64 KiB)\n",
           n1 ? h1w / n1 : 0.0, chunks);
    free(b);
}

/* ---------------- t: размер + скорость + round-trip ---------------- */
static void test_mode(const char *path, int passes) {
    size_t n; uint8_t *b = read_file(path, &n);
    uint8_t *cb = malloc(comp_bound(n));
    uint8_t *ob = malloc(n + 1);
    DecTab *dt = malloc(sizeof *dt);
    if (!cb || !ob || !dt) { fprintf(stderr, "oom\n"); exit(1); }

    size_t tables = 0; int rawc = 0;
    size_t cs = compress_all(b, n, cb, &tables, &rawc);
    double te = 1e30;
    for (int p = 0; p < passes; p++) {
        double t0 = now_s();
        compress_all(b, n, cb, NULL, NULL);
        double dtm = now_s() - t0;
        if (dtm < te) te = dtm;
    }
    long dn = decompress_all(cb, cs, ob, n, dt);
    int ok = (dn == (long)n) && (n == 0 || !memcmp(b, ob, n));
    double td = 1e30;
    for (int p = 0; p < passes; p++) {
        double t0 = now_s();
        decompress_all(cb, cs, ob, n, dt);
        double dtm = now_s() - t0;
        if (dtm < td) td = dtm;
    }

    size_t chunks = (n + CHUNK - 1) / CHUNK; if (!n) chunks = 0;
    size_t body = cs - 8 - chunks * 8;             /* без контейнерных рамок */
    printf("raw:     %zu bytes, %zu chunks (64 KiB), lanes=%d\n", n, chunks, RANS_N);
    printf("comp:    %zu bytes  %.4f bits/byte  (tables %.4f, payload %.4f)%s\n",
           body,
           n ? 8.0 * body / n : 0.0,
           n ? 8.0 * tables / n : 0.0,
           n ? 8.0 * (body - tables) / n : 0.0,
           rawc ? "  [raw-chunks!]" : "");
    if (n) {
        printf("encode:  %.0f MB/s (best of %d)\n", n / te / 1e6, passes);
        printf("decode:  %.0f MB/s (best of %d)\n", n / td / 1e6, passes);
    }
    printf("round-trip: %s\n", ok ? "MATCHES" : "DIFFERS");
    free(b); free(cb); free(ob); free(dt);
    if (!ok) exit(2);
}

/* v4 судья: сравнить dec_range с куском полного декода, побайтно.
 * Заодно посчитать, сколько символов реально разворачивается. */
static void range_mode(const char *path, int trials) {
    size_t n; uint8_t *b = read_file(path, &n);
    if (n > CHUNK) n = CHUNK;                 /* один чанк */
    uint8_t *cb = malloc(comp_bound(n));
    int hb = 0; int cs = enc_chunk(b, (int)n, cb, &hb);
    if (cb[0] == 1) { printf("  чанк ушёл в raw, тест не применим\n"); return; }

    DecTab *dt = malloc(sizeof *dt);
    uint8_t *full = malloc(n), *part = malloc(n);
    if (dec_chunk(cb, cs, full, (int)n, dt, RANS_N) < 0) {
        printf("  полный декод упал\n"); return; }
    if (memcmp(full, b, n)) { printf("  ЭТАЛОН НЕ СХОДИТСЯ\n"); return; }

    /* p указывает на начало после таблиц — повторяем разбор dec_chunk */
    const uint8_t *p = cb + 1; int K = *p++; p += K;
    BR br; br_init(&br, p);
    uint64_t ru = 0;
    for (int i = 0; i < K; i++) ru |= (uint64_t)br_get(&br, 1) << i;
    for (int i = 0; i < K; i++) { if (!((ru >> i) & 1)) continue;
        uint64_t cu = 0;
        for (int j = 0; j < K; j++) cu |= (uint64_t)br_get(&br, 1) << j;
        for (int j = 0; j < K; j++) if ((cu >> j) & 1) br_get(&br, TF_BITS); }
    p = br_align(&br);
    uint8_t sym0 = cb[2];

    int bad = 0; long total_work = 0;
    int RLEN = getenv("RLEN") ? atoi(getenv("RLEN")) : 16000;
    srand(12345);
    for (int i = 0; i < trials; i++) {
        int from = rand() % ((int)n - RLEN), to = from + RLEN, work = 0;
        memset(part, 0, n);
        dec_range_core(p, part, (int)n, dt, sym0, RANS_N, from, to, &work);
        if (memcmp(part, full + from, RLEN)) bad++;
        total_work += work;
    }
    printf("  диапазонов: %d, расхождений: %d\n", trials, bad);
    printf("  средняя работа: %.0f символов из %zu (%.1f%%)\n",
           (double)total_work / trials, n, 100.0 * total_work / trials / n);
    printf("  ВЕРДИКТ: %s\n", bad ? "ЧЕКПОИНТЫ НЕВЕРНЫ" : "ЧЕКПОИНТЫ РАБОТАЮТ");
    free(b); free(cb); free(full); free(part); free(dt);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr,
            "usage: %s c <in> <out> | d <in> <out> | t <in> [passes] | h <in>\n",
            argv[0]);
        return 1;
    }
    if (argv[1][0] == 'h') { entropy_mode(argv[2]); return 0; }
    if (argv[1][0] == 'r') {
        range_mode(argv[2], argc > 3 ? atoi(argv[3]) : 100);
        return 0;
    }
    if (argv[1][0] == 't') {
        test_mode(argv[2], argc > 3 ? atoi(argv[3]) : 5);
        return 0;
    }
    if (argv[1][0] == 'c') {
        size_t n; uint8_t *b = read_file(argv[2], &n);
        uint8_t *cb = malloc(comp_bound(n));
        size_t cs = compress_all(b, n, cb, NULL, NULL);
        write_file(argv[3], cb, cs);
        printf("%zu -> %zu (%.4f bits/byte)\n", n, cs, n ? 8.0 * cs / n : 0.0);
        free(b); free(cb);
        return 0;
    }
    if (argv[1][0] == 'd') {
        size_t cn; uint8_t *cb = read_file(argv[2], &cn);
        size_t cap = 0;
        {   /* суммарный raw из рамок чанков */
            const uint8_t *p = cb + 8, *end = cb + cn;
            while (p + 8 <= end) { cap += get32(p); p += 8 + get32(p + 4); }
        }
        uint8_t *ob = malloc(cap + 1);
        DecTab *dt = malloc(sizeof *dt);
        long dn = decompress_all(cb, cn, ob, cap, dt);
        if (dn < 0) { fprintf(stderr, "bad stream\n"); return 2; }
        write_file(argv[3], ob, (size_t)dn);
        printf("%zu -> %ld\n", cn, dn);
        free(cb); free(ob); free(dt);
        return 0;
    }
    fprintf(stderr, "unknown mode\n");
    return 1;
}
