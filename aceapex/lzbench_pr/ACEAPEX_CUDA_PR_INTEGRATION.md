# ACEAPEX_CG → lzbench: интеграционный пакет (PR-каркас)

Все куски ниже самодостаточны. Порядок применения: A → B → C → D → E.
Pod нужен только на шаге E (сборка + замер).

-----

## A. Файл `lz/aceapex/cuda/aceapex_cuda_lzbench.cpp` (новый)

```cpp
// lzbench-facing glue for the ACEAPEX_CG (CUDA) codec variant.
#ifndef BENCH_REMOVE_ACEAPEX
#ifdef BENCH_HAS_CUDA
#include "../../bench/codecs.h"
#include "aceapex_cuda.h"
#include <cstdio>

static size_t s_cg_level = 2;   // stashed from init (lzbench passes level there)

char* lzbench_aceapex_cuda_init(size_t insize, size_t level, size_t threads)
{
    (void)insize; (void)threads;
    s_cg_level = level;
    if(!aceapex_cg_available()){
        fprintf(stderr, "aceapex_cuda: no CUDA device available at runtime\n");
        return NULL;
    }
    return (char*)1;  // non-NULL sentinel; context is internal/global
}

void lzbench_aceapex_cuda_deinit(char* workmem)
{
    (void)workmem;
    aceapex_cg_release();
}

// level → block_size: 1=64KB, 2=32KB, 3=16KB (speed grows, ratio ~flat on CPU
// profile; GPU profile pays a bit more at smaller blocks — documented in PR).
static uint32_t cg_block_size(void)
{
    switch(s_cg_level){ case 1: return 65536; case 2: return 32768; default: return 16384; }
}

int64_t lzbench_aceapex_cuda_compress(char *inbuf, size_t insize, char *outbuf,
                                      size_t outsize, codec_options_t *codec_options)
{
    int thr = (codec_options && codec_options->threads > 0) ? (int)codec_options->threads : 8;
    return aceapex_cg_compress(inbuf, insize, outbuf, outsize, cg_block_size(), thr);
}

int64_t lzbench_aceapex_cuda_decompress(char *inbuf, size_t insize, char *outbuf,
                                        size_t outsize, codec_options_t *codec_options)
{
    (void)codec_options;
    return aceapex_cg_decompress(inbuf, insize, outbuf, outsize);
}
#endif // BENCH_HAS_CUDA
#endif // BENCH_REMOVE_ACEAPEX
```

-----

## B. Патч в `lz/aceapex/aceapex_api.cpp` (добавить В КОНЕЦ файла)

Экспортирует сырые потоки match-фазы для CUDA-модуля (тот же TU, что encoder —
никаких дублей символов). Применять идемпотентным python-сниппетом:

```bash
cd ~/lzbench/lz/aceapex && python3 - << 'PY'
f="aceapex_api.cpp"; s=open(f).read()
if "aceapex_encode_raw" in s:
    print("already patched"); raise SystemExit
ADD = r'''

// ---- raw match-phase export for the CUDA codec (lz/aceapex/cuda) ----
extern "C" {
typedef struct {
    uint8_t *lit, *off, *len, *cmd;
    size_t   lit_sz, off_sz, len_sz, cmd_sz;
    void    *boffs;
    size_t   num_blocks;
    uint32_t block_size;
    uint64_t orig_size;
} aceapex_raw_t;

static uint32_t g_raw_bs_override = 0;

int aceapex_encode_raw(const void* src, size_t src_size, int threads,
                       uint32_t block_size, aceapex_raw_t* out)
{
    if (!src || !out) return -1;
    if (threads <= 0) threads = 8;
    g_raw_bs_override = block_size;          // consumed below via g_block_size patch
    std::vector<BlockOffsets>* boffs = new std::vector<BlockOffsets>();
    uint8_t *rl,*ro,*rn,*rc; size_t tl,to,tn,tc,nb;
    if (!encode_file((const uint8_t*)src, src_size, threads, 2,
                     *boffs, rl, tl, ro, to, rn, tn, rc, tc, nb)) {
        delete boffs; g_raw_bs_override = 0; return -2;
    }
    g_raw_bs_override = 0;
    out->lit=rl; out->off=ro; out->len=rn; out->cmd=rc;
    out->lit_sz=tl; out->off_sz=to; out->len_sz=tn; out->cmd_sz=tc;
    out->boffs=(void*)boffs->data();         // kept alive via leaked vector below
    out->num_blocks=nb; out->block_size=(uint32_t)g_block_size;
    out->orig_size=src_size;
    // stash vector pointer right after struct for free():
    out->len_sz = tn; // (no-op, keep layout)
    // store for free:
    *((std::vector<BlockOffsets>**)&out->boffs) = boffs;  // see note below
    out->boffs = (void*)boffs;
    return 0;
}

void aceapex_raw_free(aceapex_raw_t* r)
{
    if (!r) return;
    free(r->lit); free(r->off); free(r->len); free(r->cmd);
    delete (std::vector<BlockOffsets>*)r->boffs;
    r->lit=r->off=r->len=r->cmd=nullptr; r->boffs=nullptr;
}
} // extern "C"
'''
open(f,"a").write(ADD)
print("PATCH APPLIED (api)")
PY
```

ВАЖНО для потребителя: `out->boffs` хранит `std::vector<BlockOffsets>*`;
данные таблицы = `((std::vector<BlockOffsets>*)r.boffs)->data()`. В
`aceapex_cuda.cu` поэтому заменить ОДНУ строку:
`memcpy(p, r.boffs, ...)` → `memcpy(p, ((std::vector<...>*)r.boffs)->data(), ...)`
— ИЛИ (проще) на шаге сборки на поде я дам финальную выверенную версию обеих
сторон. Это известная шероховатость каркаса, помечена сознательно.

И второй мини-патч там же — поддержка block_size override в
`compute_block_size` (lzbench-копия aceapex_main.cpp):

```bash
cd ~/lzbench/lz/aceapex && python3 - << 'PY'
f="aceapex_main.cpp"; s=open(f).read()
A="static size_t compute_block_size(size_t src_size, int threads) {"
INS=A+'\n    extern "C" { extern uint32_t g_raw_bs_override_hook(void); }\n'
# Simpler, header-order-safe variant: file-local extern of the override var
INS=A+"\n    { extern uint32_t g_raw_bs_override; if(g_raw_bs_override>=4096) return g_raw_bs_override; }"
if "g_raw_bs_override" in s: print("already patched")
elif s.count(A)==1: open(f,"w").write(s.replace(A,INS)); print("PATCH APPLIED (bs)")
else: print("ANCHOR?!", s.count(A))
PY
```

(Примечание: `g_raw_bs_override` объявлен в api-патче выше как static —
для линковки между ними на сборке уберём `static`. Финальную сверку обеих
правок делаем на поде перед компиляцией — это 2 строки.)

-----

## C. Регистрация в `bench/codecs.h` (рядом с другими aceapex-декларациями, ~строка 627)

```c
#ifdef BENCH_HAS_CUDA
    char* lzbench_aceapex_cuda_init(size_t insize, size_t level, size_t threads);
    void lzbench_aceapex_cuda_deinit(char* workmem);
    int64_t lzbench_aceapex_cuda_compress(char *inbuf, size_t insize, char *outbuf, size_t outsize, codec_options_t *codec_options);
    int64_t lzbench_aceapex_cuda_decompress(char *inbuf, size_t insize, char *outbuf, size_t outsize, codec_options_t *codec_options);
#endif
```

## C2. Строка в таблице `bench/lzbench.h` (в блок #ifdef BENCH_HAS_CUDA, рядом с nvcomp/cudaMemcpy, ~строки 317-318)

```c
    { "aceapex_cuda", "aceapex_cuda 0.9",        1,   3,    0, /* mt_mode: скопировать у nvcomp_lz4 */ BENCH_POOL_MT,
      lzbench_aceapex_cuda_compress, lzbench_aceapex_cuda_decompress,
      lzbench_aceapex_cuda_init,     lzbench_aceapex_cuda_deinit },
```

И добавить `aceapex_cuda` в CUDA-alias строку (как в твоей записи CONTEXT:
“memcpy/cudaMemcpy/nvcomp_lz4/bsc_cuda/aceapex_cuda”).

## D. Makefile (в существующую CUDA-секцию, по образцу nvcomp)

```make
ifeq "$(CUDA)" "1"
ACEAPEX_CUDA_FILES = lz/aceapex/cuda/aceapex_cuda.o lz/aceapex/cuda/aceapex_cuda_lzbench.o
lz/aceapex/cuda/aceapex_cuda.o: lz/aceapex/cuda/aceapex_cuda.cu
	$(NVCC) -O3 -arch=native -I$(NVCOMP_INC) -c $< -o $@
# линковка: добавить $(ACEAPEX_CUDA_FILES) в цель lzbench и -lnvcomp в LDFLAGS
endif
```

(Точные имена переменных CUDA-секции — сверить с master Makefile;
nvcomp уже линкуется, переиспользовать их флаги.)

-----

## E. Сборка и честный замер (короткая pod-сессия, ~20 мин)

```bash
cd ~/lzbench    # на поде: свежий клон + наши файлы
make CUDA=1 -j8
# host<->host число (то, что увидит любой ревьюер):
./lzbench -eaceapex_cuda -t3,5 /workspace/data/enwik9
./lzbench -eaceapex_cuda -t3,5 /workspace/data/NA12878_1gb.fastq
# разбивка стадий (для README, device-resident с пометкой):
ACEAPEX_CUDA_TIMING=1 ./lzbench -eaceapex_cuda -i1,1 /workspace/data/enwik9
```

-----

## F. Черновик текста PR (честный)

> **Add aceapex_cuda: GPU decode variant (optional, CUDA=1)**
> 
> Follows up #288 (accepted in principle). Adds an optional CUDA codec
> variant of ACEAPEX under `lz/aceapex/cuda/`:
> 
> - `compress`: ACEAPEX LZ match phase (CPU, same core as merged codec) +
>   nvcomp batched rANS entropy → self-contained “GPU profile” container.
> - `decompress`: full host↔host path — H2D, nvcomp ANS on device,
>   warp-per-block LZ decode on device, D2H. Bit-perfect (verified on
>   enwik9 / silesia.tar / FASTQ NA12878).
> - Disabled by default; `make CUDA=1` only. Zero impact otherwise.
> - Honest numbers: lzbench measures the full host↔host call, which is
>   PCIe-bound for GPU codecs (same as nvcomp_lz4 here). Measured:
>   [host↔host X GB/s — ВПИСАТЬ ПОСЛЕ ЗАМЕРА]. Device-resident decode
>   (data stays on GPU): [Y GB/s, пометка “excl. PCIe”] — relevant for
>   GPU-resident pipelines; details in lz/aceapex/cuda/README.
> - Ratio note: the GPU profile uses rANS chunks (vs FSE/zstd in the CPU
>   profile): enwik9 ~2.1 vs ~2.67. The two profiles are distinct formats;
>   aceapex (CPU) remains unchanged.
> 
> Happy to adjust structure/flags to match repo conventions.

-----

## Что сознательно отложено (честные хвосты каркаса)

1. Связка `g_raw_bs_override` между двумя патчами (static → extern) — финализируем
   при сборке (2 строки), там же выверю boffs-указатель.
1. mt_mode в таблице — скопировать у nvcomp_lz4 из master (не угадываю).
1. CPU-fallback в рантайме без GPU: сейчас init возвращает NULL с сообщением
   (кодек просто пропускается) — это поведение nvcomp-кодеков; полный CPU-декод
   GPU-профиля возможен позже (nvcomp CPU backend + наш host_decode), вне scope PR v1.