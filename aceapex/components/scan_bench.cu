/* scan_bench.cu — A/B: последовательная rep-цепь против префиксного скана.
 *
 * Что меряем. В нашем формате декодер восстанавливает состояние парсера — четыре
 * последних расстояния — идя команда за командой. Это единственное место, где
 * блок не распараллеливается. Утверждение теоремы: каждая команда задаёт
 * отображение, у которого каждый выходной слот либо константа, либо копия
 * входного слота; такие отображения замкнуты относительно композиции,
 * ассоциативны, и состояние в любой точке берётся префиксным сканом.
 *
 * Математика проверена покомандно на CPU: 21 132 882 команды, два противоположных
 * профиля, 0 расхождений (composition_check.py, 11.08.2026). Здесь НЕ проверяем
 * математику — только скорость, и сверяем состояния как контроль корректности.
 *
 * Представление элемента моноида для k=4:
 *   src[i] >= 0  -> слот i берёт значение входного слота src[i]
 *   src[i] <  0  -> слот i получает константу val[i]
 * Композиция (сначала g, потом f):
 *   (f∘g).src[i] = f.src[i] >= 0 ? g.src[f.src[i]] : -1
 *   (f∘g).val[i] = f.src[i] >= 0 ? (g.src[f.src[i]] >= 0 ? 0 : g.val[f.src[i]])
 *                                : f.val[i]
 * Нейтральный элемент: src = {0,1,2,3}, val = {0,0,0,0}.
 *
 * Сборка:  nvcc -O3 -arch=sm_90 -o scan_bench scan_bench.cu
 * CPU-проверка перед подом:  nvcc -DCPU_ONLY -O3 -o scan_cpu scan_bench.cu
 * Запуск:  ./scan_bench streams.bin <first_block> <count>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef CPU_ONLY
/* CPU-режим: обычный компилятор не знает атрибутов CUDA */
#define __host__
#define __device__
#define __global__
#define __shared__
#define __restrict__
#else
#include <cuda_runtime.h>
#define CK(x) do{ cudaError_t e=(x); if(e!=cudaSuccess){ \
    fprintf(stderr,"CUDA %s at %d: %s\n",#x,__LINE__,cudaGetErrorString(e)); \
    exit(1);} }while(0)
#endif

#define K 4                      /* размер rep-кэша */
#define TPB 256

/* --- формат входа: streams.bin = AetHeader(68) + BlockOffsets[nb] + LIT+OFF+LEN+CMD --- */
#pragma pack(push,1)
typedef struct {
    char     magic[8];
    uint32_t version;
    uint64_t orig_size;
    uint32_t block_size;
    uint32_t num_blocks;
    uint64_t zlit_sz, zoff_sz, zlen_sz, zcmd_sz;
    uint8_t  xxhash[8];
} AetHeader;                      /* 68 байт, см. docs/FORMAT_STREAMS.md */
#pragma pack(pop)

typedef struct {
    uint64_t lit_off, off_off, len_off, cmd_off;
    uint64_t lit_sz,  off_sz,  len_sz,  cmd_sz;
} BlockOffsets;

/* --- элемент моноида --- */
typedef struct { int8_t src[K]; uint32_t val[K]; } Mono;

__host__ __device__ static inline Mono mono_id(void){
    Mono m; for(int i=0;i<K;i++){ m.src[i]=(int8_t)i; m.val[i]=0; } return m;
}

/* f после g */
__host__ __device__ static inline Mono mono_compose(const Mono g, const Mono f){
    Mono r;
    for(int i=0;i<K;i++){
        int s=f.src[i];
        if(s>=0){ r.src[i]=g.src[s]; r.val[i]=(g.src[s]>=0)?0u:g.val[s]; }
        else    { r.src[i]=-1;       r.val[i]=f.val[i]; }
    }
    return r;
}

/* --- разбор команд блока в элементы моноида ---
 * Команда кодируется одним байтом cmd[], длинные расстояния — варинтом в off[].
 * Литерал: состояние не меняется -> тождество.
 * rep_i:   move-to-front, слот i переходит вперёд.
 * явное расстояние d: сдвиг со вставкой, слот 0 = константа d.
 * 0xFF:    сброс, все слоты константы (начало блока).
 * Разбор повторяет composition_check.py; при расхождении с ним замер недействителен.
 */
static uint64_t read_varint(const uint8_t* b, size_t n, size_t* p){
    uint64_t v=0; int sh=0;
    while(*p<n){ uint8_t c=b[(*p)++]; v |= (uint64_t)(c&0x7F)<<sh;
                 if(!(c&0x80)) break; sh+=7; }
    return v;
}

static int build_monos(const uint8_t* cmd, size_t cmd_sz,
                       const uint8_t* off, size_t off_sz,
                       Mono* out, int cap)
{
    size_t pc=0, po=0; int n=0;
    while(pc<cmd_sz && n<cap){
        uint8_t c = cmd[pc++];
        Mono m = mono_id();
        if(c==0xFF){                                  /* сброс */
            for(int i=0;i<K;i++){ m.src[i]=-1; m.val[i]=(uint32_t)(1u<<i); }
        } else if(c & 0x80){                          /* rep_i, индекс в младших битах */
            int idx = c & 0x03;
            if(idx>0){                                /* move-to-front */
                m.src[0]=(int8_t)idx;
                for(int i=1;i<=idx;i++) m.src[i]=(int8_t)(i-1);
            }
        } else {                                      /* явное расстояние */
            uint64_t d = read_varint(off, off_sz, &po);
            m.src[0]=-1; m.val[0]=(uint32_t)d;
            for(int i=1;i<K;i++) m.src[i]=(int8_t)(i-1);
        }
        out[n++]=m;
    }
    return n;
}

/* --- эталон на CPU: последовательный проход --- */
static void seq_states_cpu(const Mono* m, int n, uint32_t* st /*n*K*/){
    uint32_t r[K]={1,2,4,8};
    for(int t=0;t<n;t++){
        uint32_t nr[K];
        for(int i=0;i<K;i++) nr[i] = (m[t].src[i]>=0) ? r[m[t].src[i]] : m[t].val[i];
        for(int i=0;i<K;i++){ r[i]=nr[i]; st[(size_t)t*K+i]=nr[i]; }
    }
}

#ifndef CPU_ONLY
/* --- A: последовательная цепь, одна нить на блок (как сейчас) --- */
__global__ void k_parse_seq(const Mono* __restrict__ m, const int* __restrict__ nb_off,
                            const int* __restrict__ nb_cnt, uint32_t* __restrict__ st)
{
    int b = blockIdx.x * blockDim.x + threadIdx.x;
    int base = nb_off[b], cnt = nb_cnt[b];
    uint32_t r[K]={1,2,4,8};
    for(int t=0;t<cnt;t++){
        Mono e = m[base+t];
        uint32_t nr[K];
        #pragma unroll
        for(int i=0;i<K;i++) nr[i] = (e.src[i]>=0) ? r[e.src[i]] : e.val[i];
        #pragma unroll
        for(int i=0;i<K;i++) r[i]=nr[i];
        #pragma unroll
        for(int i=0;i<K;i++) st[(size_t)(base+t)*K+i]=r[i];
    }
}

/* --- B: префиксный скан по моноиду, блок нитей на блок данных ---
 * Классический Blelloch в shared memory, оператор — композиция.
 * Обрабатываем чанками по TPB, состояние переносится между чанками.
 */
__global__ void k_parse_scan(const Mono* __restrict__ m, const int* __restrict__ nb_off,
                             const int* __restrict__ nb_cnt, uint32_t* __restrict__ st)
{
    __shared__ Mono sh[TPB];
    int b = blockIdx.x;
    int base = nb_off[b], cnt = nb_cnt[b];
    int tid = threadIdx.x;

    Mono carry = mono_id();
    for(int i=0;i<K;i++){ carry.src[i]=-1; carry.val[i]=(uint32_t)(1u<<i); }

    for(int chunk=0; chunk<cnt; chunk+=TPB){
        int idx = chunk + tid;
        sh[tid] = (idx<cnt) ? m[base+idx] : mono_id();
        __syncthreads();

        /* inclusive scan по shared, Hillis-Steele: log2(TPB) шагов */
        for(int d=1; d<TPB; d<<=1){
            Mono v;
            bool act = (tid>=d);
            if(act) v = mono_compose(sh[tid-d], sh[tid]);
            __syncthreads();
            if(act) sh[tid]=v;
            __syncthreads();
        }

        if(idx<cnt){
            Mono tot = mono_compose(carry, sh[tid]);
            #pragma unroll
            for(int i=0;i<K;i++)
                st[(size_t)(base+idx)*K+i] = (tot.src[i]>=0) ? 0u : tot.val[i];
            /* при корректной композиции src всегда <0: цепь начинается со сброса */
        }
        __syncthreads();
        Mono last = sh[(cnt-chunk<TPB ? cnt-chunk : TPB)-1];
        carry = mono_compose(carry, last);
        __syncthreads();
    }
}
#endif

int main(int argc, char** argv){
    if(argc<4){ fprintf(stderr,"usage: %s streams.bin <first_block> <count>\n",argv[0]); return 1; }
    const char* path=argv[1];
    int first=atoi(argv[2]), want=atoi(argv[3]);

    FILE* f=fopen(path,"rb"); if(!f){ perror("open"); return 1; }
    AetHeader h; if(fread(&h,sizeof h,1,f)!=1){ fprintf(stderr,"short header\n"); return 1; }
    if(memcmp(h.magic,"ACEPX2\0\0",8)){ fprintf(stderr,"bad magic\n"); return 1; }
    printf("blocks %u, block_size %u, orig %llu\n",
           h.num_blocks, h.block_size, (unsigned long long)h.orig_size);

    BlockOffsets* bo=(BlockOffsets*)malloc((size_t)h.num_blocks*sizeof(BlockOffsets));
    if(fread(bo,sizeof(BlockOffsets),h.num_blocks,f)!=h.num_blocks){ fprintf(stderr,"short table\n"); return 1; }
    long base_pos=ftell(f);

    uint64_t tot_lit=0, tot_off=0, tot_len=0, tot_cmd=0;
    for(uint32_t i=0;i<h.num_blocks;i++){
        tot_lit+=bo[i].lit_sz; tot_off+=bo[i].off_sz;
        tot_len+=bo[i].len_sz; tot_cmd+=bo[i].cmd_sz;
    }
    uint8_t* OFF=(uint8_t*)malloc(tot_off);
    uint8_t* CMD=(uint8_t*)malloc(tot_cmd);
    fseek(f, base_pos+tot_lit, SEEK_SET);            if(fread(OFF,1,tot_off,f)!=tot_off) return 1;
    fseek(f, base_pos+tot_lit+tot_off+tot_len, SEEK_SET);
                                                     if(fread(CMD,1,tot_cmd,f)!=tot_cmd) return 1;
    fclose(f);

    if(first+want > (int)h.num_blocks) want = h.num_blocks - first;
    printf("разбираю блоки %d..%d\n", first, first+want);

    /* разбор в моноиды */
    int cap=0; for(int b=first;b<first+want;b++) cap += (int)bo[b].cmd_sz;
    Mono* M=(Mono*)malloc((size_t)cap*sizeof(Mono));
    int* nb_off=(int*)malloc(want*sizeof(int));
    int* nb_cnt=(int*)malloc(want*sizeof(int));
    int cur=0;
    for(int b=first;b<first+want;b++){
        nb_off[b-first]=cur;
        int n=build_monos(CMD+bo[b].cmd_off, bo[b].cmd_sz,
                          OFF+bo[b].off_off, bo[b].off_sz, M+cur, cap-cur);
        nb_cnt[b-first]=n; cur+=n;
    }
    printf("команд всего %d\n", cur);

    uint32_t* ref=(uint32_t*)malloc((size_t)cur*K*sizeof(uint32_t));
    for(int b=0;b<want;b++) seq_states_cpu(M+nb_off[b], nb_cnt[b], ref+(size_t)nb_off[b]*K);
    printf("эталон CPU построен\n");

#ifdef CPU_ONLY
    printf("CPU_ONLY: разбор и эталон готовы, GPU не собирался.\n");
    printf("Проверь, что число команд совпадает с composition_check.py на тех же блоках.\n");
    return 0;
#else
    Mono *dM; int *dOff,*dCnt; uint32_t *dS;
    CK(cudaMalloc(&dM,(size_t)cur*sizeof(Mono)));
    CK(cudaMalloc(&dOff,want*sizeof(int)));
    CK(cudaMalloc(&dCnt,want*sizeof(int)));
    CK(cudaMalloc(&dS,(size_t)cur*K*sizeof(uint32_t)));
    CK(cudaMemcpy(dM,M,(size_t)cur*sizeof(Mono),cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dOff,nb_off,want*sizeof(int),cudaMemcpyHostToDevice));
    CK(cudaMemcpy(dCnt,nb_cnt,want*sizeof(int),cudaMemcpyHostToDevice));

    uint32_t* got=(uint32_t*)malloc((size_t)cur*K*sizeof(uint32_t));
    cudaEvent_t e0,e1; CK(cudaEventCreate(&e0)); CK(cudaEventCreate(&e1));
    const int REP=30, WARM=10;

    /* A */
    int grid=(want+TPB-1)/TPB;
    for(int i=0;i<WARM;i++) k_parse_seq<<<grid,TPB>>>(dM,dOff,dCnt,dS);
    CK(cudaDeviceSynchronize());
    float bestA=1e9f;
    for(int i=0;i<REP;i++){
        CK(cudaEventRecord(e0));
        k_parse_seq<<<grid,TPB>>>(dM,dOff,dCnt,dS);
        CK(cudaEventRecord(e1)); CK(cudaEventSynchronize(e1));
        float ms; CK(cudaEventElapsedTime(&ms,e0,e1)); if(ms<bestA) bestA=ms;
    }
    CK(cudaMemcpy(got,dS,(size_t)cur*K*sizeof(uint32_t),cudaMemcpyDeviceToHost));
    size_t badA=0; for(size_t i=0;i<(size_t)cur*K;i++) if(got[i]!=ref[i]) badA++;

    /* B */
    CK(cudaMemset(dS,0,(size_t)cur*K*sizeof(uint32_t)));
    for(int i=0;i<WARM;i++) k_parse_scan<<<want,TPB>>>(dM,dOff,dCnt,dS);
    CK(cudaDeviceSynchronize());
    float bestB=1e9f;
    for(int i=0;i<REP;i++){
        CK(cudaEventRecord(e0));
        k_parse_scan<<<want,TPB>>>(dM,dOff,dCnt,dS);
        CK(cudaEventRecord(e1)); CK(cudaEventSynchronize(e1));
        float ms; CK(cudaEventElapsedTime(&ms,e0,e1)); if(ms<bestB) bestB=ms;
    }
    CK(cudaMemcpy(got,dS,(size_t)cur*K*sizeof(uint32_t),cudaMemcpyDeviceToHost));
    size_t badB=0; for(size_t i=0;i<(size_t)cur*K;i++) if(got[i]!=ref[i]) badB++;

    printf("\n  A последовательная цепь : %8.3f ms   расхождений %zu\n", bestA, badA);
    printf("  B префиксный скан       : %8.3f ms   расхождений %zu\n", bestB, badB);
    if(badA==0 && badB==0)
        printf("  => скан %s в %.2f раза\n", bestB<bestA?"БЫСТРЕЕ":"медленнее",
               bestB<bestA? bestA/bestB : bestB/bestA);
    else
        printf("  => ЗАМЕР НЕДЕЙСТВИТЕЛЕН: состояния расходятся\n");
    return (badA||badB)?2:0;
#endif
}
