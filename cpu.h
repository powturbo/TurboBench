#pragma once
#ifndef CPUISA_H_
#define CPUISA_H_
#define AVX512F     0x001
#define AVX512DQ    0x002
#define AVX512IFMA  0x004
#define AVX512PF    0x008
#define AVX512ER    0x010
#define AVX512CD    0x020
#define AVX512BW    0x040
#define AVX512VL    0x080
#define AVX512VNNI  0x100
#define AVX512VBMI  0x200
#define AVX512VBMI2 0x400

#define IS_SSE       0x10
#define IS_SSE2      0x20
#define IS_SSE3      0x30
#define IS_SSSE3     0x32
#define IS_POWER9    0x34 // powerpc
#define IS_LSX       0x37 // Loogson LSX
#define IS_NEON      0x38 // arm neon
#define IS_RISCV     0x39 // risc-v
#define IS_SSE41     0x40
#define IS_SSE41x    0x41 //+popcount
#define IS_SSE42     0x42
#define IS_AVX       0x50
#define IS_AVX2      0x60
#define IS_ASX       0x61 // Loogson ASX
#define IS_AVX512    0x800
#endif

//------- CPU instruction set
// cpuiset  = 0: return current simd set,
// cpuiset != 0: set simd set 0:scalar, 20:sse2, 52:avx2
unsigned cpuini(unsigned cpuiset);

// convert simd set to string "sse3", "sse3", "sse4.1" or "avx2"
// Ex.: printf("current cpu set=%s\n", cpustr(cpuini(0)) );
char *cpustr(unsigned cpuisa);

unsigned cpuisa(void);

char *cpubrand(char *s, size_t maxlen);

