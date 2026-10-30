# ACEAPEX_CUDA → lzbench: пакет v2 (гибрид, единый формат, ноль зависимостей)

АРХИТЕКТУРА (после разведки 2026-06-10): bundled nvcomp-2.2 ANS = заглушка
(nvcompErrorNotSupported), внешний nvcomp-5 = новая проприетарная зависимость.
Поэтому: aceapex_cuda декодирует ТОТ ЖЕ .aet, что merged-кодек aceapex.
compress-слот = существующий lzbench_aceapex_compress (нового кода компрессии нет).
decompress = CPU-энтропия (in-tree lit_decompress/fse_chunked_decomp) ->
H2D -> GPU warp LZ decode -> D2H. Только CUDA Runtime.

Порядок: A → B → C → D → E (под нужен только на E).

-----

## A. Новые файлы

- `lz/aceapex/cuda/aceapex_cuda.h`  (из pr_v2/)
- `lz/aceapex/cuda/aceapex_cuda.cu` (из pr_v2/)

## B. Патч `lz/aceapex/aceapex_api.cpp` — добавить В КОНЕЦ (идемпотентно):

```bash
cd ~/lzbench/lz/aceapex && python3 - << 'PY'
f="aceapex_api.cpp"; s=open(f).read()
if "aceapex_decode_streams" in s:
    print("already patched"); raise SystemExit
ADD = r'''

// ---- stream-level decode export for the CUDA decoder (lz/aceapex/cuda) ----
extern "C" {
typedef struct {
    uint8_t *lit, *off, *len, *cmd;
    uint64_t lit_sz, off_sz, len_sz, cmd_sz;
    void    *boffs_vec;
    const void *boffs;
    size_t   num_blocks;
    uint32_t block_size;
    uint64_t orig_size;
} aceapex_streams_t;

int aceapex_decode_streams(const void* src, size_t src_size, aceapex_streams_t* out)
{
    if (!src || !out || src_size < sizeof(AetHeader)) return -1;
    const uint8_t* p=(const uint8_t*)src;
    AetHeader hdr; memcpy(&hdr,p,sizeof(hdr));
    if (memcmp(hdr.magic,"ACEPX2\0\0",8)!=0) return -1;
    p+=sizeof(hdr);
    std::vector<BlockOffsets>* bv=new std::vector<BlockOffsets>(hdr.num_blocks);
    memcpy(bv->data(),p,hdr.num_blocks*sizeof(BlockOffsets));
    p+=hdr.num_blocks*sizeof(BlockOffsets);
    uint8_t* zl=(uint8_t*)malloc(hdr.zlit_sz);
    uint8_t* zo=(uint8_t*)malloc(hdr.zoff_sz);
    uint8_t* zn=(uint8_t*)malloc(hdr.zlen_sz);
    uint8_t* zc=(uint8_t*)malloc(hdr.zcmd_sz);
    if(!zl||!zo||!zn||!zc){free(zl);free(zo);free(zn);free(zc);delete bv;return -2;}
    memcpy(zl,p,hdr.zlit_sz); p+=hdr.zlit_sz;
    memcpy(zo,p,hdr.zoff_sz); p+=hdr.zoff_sz;
    memcpy(zn,p,hdr.zlen_sz); p+=hdr.zlen_sz;
    memcpy(zc,p,hdr.zcmd_sz);
    size_t os=*(uint64_t*)zo, ns=*(uint64_t*)zn, cs=*(uint64_t*)zc;
    size_t ls=0; uint8_t* l=lit_decompress(zl,hdr.zlit_sz,ls);
    if(!l){free(zl);free(zo);free(zn);free(zc);delete bv;return -2;}
    uint8_t* o=(uint8_t*)malloc(os);
    uint8_t* n=(uint8_t*)malloc(ns);
    uint8_t* c=(uint8_t*)malloc(cs);
    if(!o||!n||!c){free(o);free(n);free(c);free(l);free(zl);free(zo);free(zn);free(zc);delete bv;return -2;}
    fse_chunked_decomp(zo,os,o); fse_chunked_decomp(zn,ns,n); fse_chunked_decomp(zc,cs,c);
    free(zl);free(zo);free(zn);free(zc);
    out->lit=l; out->off=o; out->len=n; out->cmd=c;
    out->lit_sz=ls; out->off_sz=os; out->len_sz=ns; out->cmd_sz=cs;
    out->boffs_vec=(void*)bv; out->boffs=(const void*)bv->data();
    out->num_blocks=hdr.num_blocks; out->block_size=hdr.block_size;
    out->orig_size=hdr.orig_size;
    return 0;
}

void aceapex_streams_free(aceapex_streams_t* s)
{
    if(!s) return;
    free(s->lit); free(s->off); free(s->len); free(s->cmd);
    delete (std::vector<BlockOffsets>*)s->boffs_vec;
    s->lit=s->off=s->len=s->cmd=nullptr; s->boffs_vec=nullptr; s->boffs=nullptr;
}
} // extern "C"
'''
open(f,"a").write(ADD)
print("PATCH APPLIED (decode_streams)")
PY
```

(Логика 1:1 повторяет существующий aceapex_decompress из этого же файла,
останавливаясь ПЕРЕД parallel_decode — энтропийная часть без LZ.)

## C. Файл `lz/aceapex/cuda/aceapex_cuda_lzbench.cpp` (новый):

```cpp
// lzbench glue for the ACEAPEX CUDA decoder (same .aet as the CPU codec).
#ifndef BENCH_REMOVE_ACEAPEX
#ifdef BENCH_HAS_CUDA
#include "../../bench/codecs.h"
#include "aceapex_cuda.h"
#include <cstdio>

char* lzbench_aceapex_cuda_init(size_t insize, size_t level, size_t threads)
{
    (void)insize; (void)level; (void)threads;
    if(!aceapex_cg_available()){
        fprintf(stderr, "aceapex_cuda: no CUDA device available at runtime\n");
        return NULL;
    }
    return (char*)1;
}

void lzbench_aceapex_cuda_deinit(char* workmem)
{
    (void)workmem;
    aceapex_cg_release();
}

int64_t lzbench_aceapex_cuda_decompress(char *inbuf, size_t insize, char *outbuf,
                                        size_t outsize, codec_options_t *codec_options)
{
    (void)codec_options;
    aceapex_streams_t s;
    if(aceapex_decode_streams(inbuf, insize, &s) != 0) return 0;
    int64_t r = aceapex_cg_match_decode(&s, outbuf, outsize);
    aceapex_streams_free(&s);
    return r;
}
#endif // BENCH_HAS_CUDA
#endif // BENCH_REMOVE_ACEAPEX
```

compress-функции НЕТ — строка регистрации использует существующий
lzbench_aceapex_compress (тот же формат).

## D. Регистрация

`bench/codecs.h` (рядом с aceapex-блоком):

```c
#ifdef BENCH_HAS_CUDA
    char* lzbench_aceapex_cuda_init(size_t insize, size_t level, size_t threads);
    void lzbench_aceapex_cuda_deinit(char* workmem);
    int64_t lzbench_aceapex_cuda_decompress(char *inbuf, size_t insize, char *outbuf, size_t outsize, codec_options_t *codec_options);
#endif
```

`bench/lzbench.h`, в comp_desc[] под `#ifdef BENCH_HAS_CUDA` (рядом с nvcomp_lz4, образец строки 236):

```c
#ifdef BENCH_HAS_CUDA
    { "aceapex_cuda","aceapex_cuda 0.9",        1,   2,    0,  BENCH_POOL_MT, lzbench_aceapex_compress,    lzbench_aceapex_cuda_decompress, lzbench_aceapex_cuda_init, lzbench_aceapex_cuda_deinit },
#endif
```

И в alias-строку (строка ~318): “memcpy/cudaMemcpy/nvcomp_lz4/bsc_cuda/aceapex_cuda”.

## E. Makefile (внутрь существующего `ifeq "$(ENABLE_CUDA)" "1"` … блока, рядом с NVCOMP, ~строки 731-741):

```make
    ACEAPEX_CUDA_FILES = lz/aceapex/cuda/aceapex_cuda.cu.o lz/aceapex/cuda/aceapex_cuda_lzbench.o
```

Правило компиляции (рядом с правилом BSC_CUDA_FILES, ~строка 888):

```make
lz/aceapex/cuda/aceapex_cuda.cu.o: lz/aceapex/cuda/aceapex_cuda.cu
	$(CUDA_CC) $(CUDA_CXXFLAGS) $(CXXFLAGS) -c $< -o $@
lz/aceapex/cuda/aceapex_cuda_lzbench.o: lz/aceapex/cuda/aceapex_cuda_lzbench.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
```

И добавить `$(ACEAPEX_CUDA_FILES)` в зависимость цели `lzbench:` (строка 748, рядом с $(NVCOMP_FILES)).
ПРИМЕЧАНИЕ sm_90: глобальный CUDA_ARCH кончается на 89 — compute_89 PTX
JIT-ится на H100, менять список НЕ нужно (минимальный blast radius).
Если захочется нативный sm_90 только для нашего файла — добавить в его правило
`--generate-code=arch=compute_90,code=[compute_90,sm_90]`.

## F. Сборка и замер (короткая pod-сессия)

```bash
cd ~/lzbench   # клон ветки на поде
make ENABLE_CUDA=1 -j8        # ВАЖНО: ENABLE_CUDA, не CUDA
./lzbench -eaceapex,aceapex_cuda -t3,5 /workspace/data/enwik9      # host<->host, обе строки
ACEAPEX_CUDA_TIMING=1 ./lzbench -eaceapex_cuda -i1,1 /workspace/data/enwik9   # разбивка
```

lzbench сам верифицирует распаковку — это и есть bit-perfect в их харнессе.

## G. Текст PR (черновик, честный)

> **Add aceapex_cuda: GPU decoder for the ACEAPEX format (optional, ENABLE_CUDA=1)**
> 
> Follow-up to #288 (accepted in principle). Adds a CUDA decoder variant for
> the already-merged ACEAPEX codec — same .aet format, same compressor:
> 
> - decompress = in-tree CPU entropy stage + GPU warp-per-block LZ match
>   decode (one warp per independent block) + transfers. Bit-perfect
>   (lzbench verification; also verified standalone on enwik9 / silesia.tar /
>   FASTQ NA12878).
> - Dependencies: CUDA Runtime only (no nvcomp, no new libraries).
> - Disabled by default; built only with `make ENABLE_CUDA=1`; all code under
>   `#ifdef BENCH_HAS_CUDA` in `lz/aceapex/cuda/`. Zero impact otherwise.
> - Honest numbers: the lzbench host<->host call is dominated by the CPU
>   entropy stage and PCIe transfers: [X GB/s — ВПИСАТЬ] vs [Y GB/s — ВПИСАТЬ]
>   for the CPU codec at the same ratio (2.67 enwik9 / 3.03 silesia). The LZ
>   kernel itself is device-resident [Z GB/s] (ACEAPEX_CUDA_TIMING=1 prints
>   the breakdown); fully device-resident pipelines (entropy on GPU) reach
>   131–247 GB/s and live in the upstream repo as research code, out of
>   lzbench scope.
> - Tested locally on H100 SXM, CUDA 12.4 (CI has no GPU runners — same as
>   nvcomp_lz4 / bsc_cuda).

## Хвосты (финализация на сборке)

1. Имена lit_decompress/fse_chunked_decomp/AetHeader/BlockOffsets — из того же TU
   (подтверждено его aceapex_api.cpp); если сигнатуры в lzbench-копии чуть
   отличаются — правится на месте.
1. mt_mode: BENCH_POOL_MT по образцу nvcomp_lz4 (строка 236) — подтверждено.
1. CHANGELOG + README строка — добавить при оформлении ветки.