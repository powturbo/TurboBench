#include <string.h>
#include "conf.h"
#include "cpu.h"
static unsigned _cpuisa;
//--------------------- CPU detection -------------------------------------------
    #if defined(__i386__) || defined(__x86_64__)
      #if _MSC_VER >=1300
#include <intrin.h>
      #elif defined (__INTEL_COMPILER)
#include <x86intrin.h>
      #endif

static inline void cpuid(int reg[4], int id) {
      #if defined (_MSC_VER) //|| defined (__INTEL_COMPILER)
  __cpuidex(reg, id, 0);
      #elif defined(__i386__) || defined(__x86_64__)
  __asm("cpuid" : "=a"(reg[0]),"=b"(reg[1]),"=c"(reg[2]),"=d"(reg[3]) : "a"(id),"c"(0) : );
      #endif
}

static inline uint64_t xgetbv (int ctr) {
      #if(defined _MSC_VER && (_MSC_FULL_VER >= 160040219) || defined(__INTEL_COMPILER))
  return _xgetbv(ctr);
      #elif defined(__i386__) || defined(__x86_64__)
  unsigned a, d;
  __asm("xgetbv" : "=a"(a),"=d"(d) : "c"(ctr) : );
  return (uint64_t)d << 32 | a;
      #else
  unsigned a=0, d=0;
  return (uint64_t)d << 32 | a;
      #endif
}
    #endif

unsigned cpuisa(void) {
  int c[4] = {0};
  if(_cpuisa) return _cpuisa;
  _cpuisa++;
    #if defined(__i386__) || defined(__x86_64__)
  cpuid(c, 0);
  if(c[0]) {
    cpuid(c, 1);
    //family = ((c >> 8) & 0xf) + ((c >> 20) & 0xff)
    //model  = ((c >> 4) & 0xf) + ((c >> 12) & 0xf0)
    if( c[3] & (1 << 25)) {         _cpuisa  = IS_SSE;
    if( c[3] & (1 << 26)) {         _cpuisa  = IS_SSE2;
    if( c[2] & (1 <<  0)) {         _cpuisa  = IS_SSE3;
      //                            _cpuisa  = IS_SSE3SLOW; // Atom SSSE3 slow
    if( c[2] & (1 <<  9)) {         _cpuisa  = IS_SSSE3;
    if( c[2] & (1 << 19)) {         _cpuisa  = IS_SSE41;
    if( c[2] & (1 << 23)) {         _cpuisa  = IS_SSE41x; // +popcount
    if( c[2] & (1 << 20)) {         _cpuisa  = IS_SSE42;  // SSE4.2
    if((c[2] & (1 << 28)) &&
       (c[2] & (1 << 27)) &&                           // OSXSAVE
       (c[2] & (1 << 26)) &&                           // XSAVE
       (xgetbv(0) & 6)==6) {        _cpuisa  = IS_AVX; // AVX
      if(c[2]& (1 <<  3))           _cpuisa |= 1;      // +FMA3
      if(c[2]& (1 << 16))           _cpuisa |= 2;      // +FMA4
      if(c[2]& (1 << 25))           _cpuisa |= 4;      // +AES
      cpuid(c, 7);
      if(c[1] & (1 << 5)) {         _cpuisa = IS_AVX2;
        if(c[1] & (1 << 16)) {
          cpuid(c, 0xd);
          if((c[0] & 0x60)==0x60) { _cpuisa = IS_AVX512;
            cpuid(c, 7);
            if(c[1] & (1<<16))      _cpuisa |= AVX512F;
            if(c[1] & (1<<17))      _cpuisa |= AVX512DQ;
            if(c[1] & (1<<21))      _cpuisa |= AVX512IFMA;
            if(c[1] & (1<<26))      _cpuisa |= AVX512PF;
            if(c[1] & (1<<27))      _cpuisa |= AVX512ER;
            if(c[1] & (1<<28))      _cpuisa |= AVX512CD;
            if(c[1] & (1<<30))      _cpuisa |= AVX512BW;
            if(c[1] & (1u<<31))     _cpuisa |= AVX512VL;
            if(c[2] & (1<< 1))      _cpuisa |= AVX512VBMI;
            if(c[2] & (1<<11))      _cpuisa |= AVX512VNNI;
            if(c[2] & (1<< 6))      _cpuisa |= AVX512VBMI2;
      }}}
    }}}}}}}}}
    #elif defined(__powerpc64__)
  _cpuisa = IS_POWER9; // power9
    #elif defined(__ARM_NEON)
  _cpuisa = IS_NEON; // ARM_NEON
    #elif defined(__riscv_vector)
  _cpuisa = IS_RISCV; // risc-v
    #elif defined(__loongarch_asx)
  _cpuisa = IS_ASX;
    #elif defined(__loongarch_lsx)
  _cpuisa = IS_LSX;
    #endif
  return _cpuisa;
}

unsigned cpuini(unsigned cpuisa) { if(cpuisa) _cpuisa = cpuisa; return _cpuisa; }

char *cpustr(unsigned cpuisa) {
  if(!cpuisa) cpuisa = _cpuisa;
    #if defined(__i386__) || defined(__x86_64__)
  if(cpuisa >= IS_AVX512) {
    if(cpuisa & AVX512VBMI2) return "avx512vbmi2";
    if(cpuisa & AVX512VBMI)  return "avx512vbmi";
    if(cpuisa & AVX512VNNI)  return "avx512vnni";
    if(cpuisa & AVX512VL)    return "avx512vl";
    if(cpuisa & AVX512BW)    return "avx512bw";
    if(cpuisa & AVX512CD)    return "avx512cd";
    if(cpuisa & AVX512ER)    return "avx512er";
    if(cpuisa & AVX512PF)    return "avx512pf";
    if(cpuisa & AVX512IFMA)  return "avx512ifma";
    if(cpuisa & AVX512DQ)    return "avx512dq";
    if(cpuisa & AVX512F)     return "avx512f";
    return "avx512";
  }
  else if(cpuisa >= IS_AVX2)    return "avx2";
  else if(cpuisa >= IS_AVX)
    switch(cpuisa&0xf) {
      case 1: return "avx+fma3";
      case 2: return "avx+fma4";
      case 4: return "avx+aes";
      case 5: return "avx+fma3+aes";
      default:return "avx";
    }
  else if(cpuisa >= IS_SSE42)   return "sse4.2";
  else if(cpuisa >= IS_SSE41x)  return "sse4.1+popcnt";
  else if(cpuisa >= IS_SSE41)   return "sse4.1";
  else if(cpuisa >= IS_SSSE3)   return "ssse3";
  else if(cpuisa >= IS_SSE3)    return "sse3";
  else if(cpuisa >= IS_SSE2)    return "sse2";
  else if(cpuisa >= IS_SSE)     return "sse";
     #elif defined(__powerpc64__)
  if(cpuisa >= IS_POWER9)       return "power9";
    #elif defined(__ARM_NEON)
  if(cpuisa >= IS_NEON)         return "arm_neon";
    #elif defined(__riscv_vector)
  if(cpuisa >= IS_RISCV)        return "riscv";
    #elif defined(__loongarch_lsx)
  if(cpuisa >= IS_LSX)          return "loongson_lsx";
     #elif defined(__loongarch_asx)
  if(cpuisa >= IS_ASX)          return "loongson_asx";
    #endif
   return "none";
}

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
  #if defined(_MSC_VER)
#include <intrin.h>
  #else
#include <cpuid.h>
  #endif

char *cpubrand(char* brand, size_t max_len) {
  unsigned int regs[4];
  char temp[49];
  for (int i = 0; i < 3; i++) { // CPUID leaves 0x80000002 to 0x80000004 contain the brand string
    unsigned int leaf = 0x80000002 + i;
      #if defined(_MSC_VER)
    __cpuid((int*)regs, leaf);
      #else
    __get_cpuid(leaf, &regs[0], &regs[1], &regs[2], &regs[3]);
      #endif
    memcpy(temp + i * 16, regs, 16);
  }
  temp[48] = '\0';  
  strncpy(brand, temp, max_len - 1);
  brand[max_len - 1] = '\0';
  max_len = strlen(brand);
  while(max_len > 0 && brand[max_len - 1] == ' ') brand[--max_len] = 0;
  return brand;
}

#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__riscv) || defined(__arm__)
  #if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>

char *cpubrand(char* brand, size_t max_len) {
  size_t size = max_len;
  if(sysctlbyname("machdep.cpu.brand_string", brand, &size, NULL, 0) != 0) {
    strncpy(brand, "Apple Silicon (Unknown)", max_len - 1);
    brand[max_len - 1] = '\0';
  }
  return brand;
}
  #elif defined(__linux__)
static void set_str(char *dst, size_t max_len, const char *src) {
  if (max_len == 0) return;
  strncpy(dst, src, max_len - 1);
  dst[max_len - 1] = '\0';
}

static char *trim(char *s) {
  while (isspace((unsigned char)*s)) s++;
  size_t n = strlen(s);
  while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
  return s;
}

static int match_key(const char *line, const char *key, char *value, size_t vlen) {
  const char *colon = strchr(line, ':');
  if (!colon) return 0;

  char kbuf[64];
  size_t klen = (size_t)(colon - line);
  if (klen >= sizeof(kbuf)) return 0;
  memcpy(kbuf, line, klen);
  kbuf[klen] = '\0';
  if (strcmp(trim(kbuf), key) != 0) return 0;

  char vbuf[256];
  set_str(vbuf, sizeof(vbuf), colon + 1);
  char *v = trim(vbuf);
  if (*v == '\0') return 0;   // key present but empty: keep looking 

  set_str(value, vlen, v);
  return 1;
}

static int read_cpuinfo_field(const char *key, char *value, size_t vlen) {
  FILE *fp = fopen("/proc/cpuinfo", "r");
  if (!fp) return 0;

  char line[1024];
  int found = 0;
  while (!found && fgets(line, sizeof(line), fp)) {
    size_t n = strlen(line);
    if (n > 0 && line[n - 1] != '\n' && !feof(fp)) {
      /* Line longer than buffer: discard the remainder */
      int c;
      while ((c = fgetc(fp)) != '\n' && c != EOF) {
      }
    }
    found = match_key(line, key, value, vlen);
  }
  fclose(fp);
  return found;
}

/* Device-tree model (ARM boards, many RISC-V boards). */
static int read_devicetree_model(char *value, size_t vlen) {
  static const char *paths[] = {
    "/proc/device-tree/model",
    "/sys/firmware/devicetree/base/model",
  };
  for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
    FILE *fp = fopen(paths[i], "rb");
    if (!fp) continue;
    char buf[256];
    size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    buf[n] = '\0';
    char *v = trim(buf);
    if (*v != '\0') {
      set_str(value, vlen, v);
      return 1;
    }
  }
  return 0;
}

typedef struct { uint8_t implementer, part; const char *name; } arm64cpu_t;
static arm64cpu_t arm64cpu[] = {
  {0x41, 0xd03, "ARM Cortex-A53"},
  {0x41, 0xd04, "ARM Cortex-A35"},
  {0x41, 0xd05, "ARM Cortex-A55"},
  {0x41, 0xd06, "ARM Cortex-A65"},
  {0x41, 0xd07, "ARM Cortex-A57"},
  {0x41, 0xd08, "ARM Cortex-A72"},
  {0x41, 0xd09, "ARM Cortex-A73"},
  {0x41, 0xd0a, "ARM Cortex-A75"},
  {0x41, 0xd0b, "ARM Cortex-A76"},
  {0x41, 0xd0c, "ARM Neoverse-N1"},
  {0x41, 0xd0d, "ARM Cortex-A77"},
  {0x41, 0xd0e, "ARM Cortex-A76AE"},
  {0x41, 0xd40, "ARM Neoverse-V1"},
  {0x41, 0xd41, "ARM Cortex-A78"},
  {0x41, 0xd42, "ARM Cortex-A78AE"},
  {0x41, 0xd43, "ARM Cortex-A65AE"},
  {0x41, 0xd44, "ARM Cortex-X1"},
  {0x41, 0xd46, "ARM Cortex-A510"},
  {0x41, 0xd47, "ARM Cortex-A710"},
  {0x41, 0xd48, "ARM Cortex-X2"},
  {0x41, 0xd49, "ARM Neoverse-N2"},
  {0x41, 0xd4a, "ARM Neoverse-E1"},
  {0x41, 0xd4b, "ARM Cortex-A78C"},
  {0x41, 0xd4c, "ARM Cortex-X1C"},
  {0x41, 0xd4d, "ARM Cortex-A715"},
  {0x41, 0xd4e, "ARM Cortex-X3"},
  {0x41, 0xd4f, "ARM Neoverse-V2"},
  {0x41, 0xd80, "ARM Cortex-A520"},
  {0x41, 0xd81, "ARM Cortex-A720"},
  {0x41, 0xd82, "ARM Cortex-X4"},
  {0x41, 0xd83, "ARM Neoverse-V3AE"},
  {0x41, 0xd84, "ARM Neoverse-V3"},
  {0x41, 0xd85, "ARM Cortex-X925"},
  {0x41, 0xd87, "ARM Cortex-A725"},
  {0x41, 0xd8e, "ARM Neoverse-N3"},
  {0x43, 0x0a1, "Cavium ThunderX"},
  {0x43, 0x0af, "Cavium ThunderX2"},
  {0x46, 0x001, "Fujitsu A64FX"},
  {0x48, 0xd01, "HiSilicon Kunpeng-920"},
  {0x4e, 0x004, "NVIDIA Carmel"},
  {0x51, 0x001, "Qualcomm Oryon"},
  {0xc0, 0xac3, "Ampere AmpereOne"},
  {0xc0, 0xac4, "Ampere AmpereOne"},
};
#define ARM64CPU_NUM sizeof(arm64cpu)/sizeof(arm64cpu[0])

char *cpubrand(char *brand, size_t max_len) {
  if (max_len == 0) return brand;
  brand[0] = '\0';
  char tmp[256];

  if (read_cpuinfo_field("model name", tmp, sizeof(tmp)) ||
      read_cpuinfo_field("Hardware", tmp, sizeof(tmp)) ||
      read_cpuinfo_field("uarch", tmp, sizeof(tmp)) ||
      read_devicetree_model(tmp, sizeof(tmp))) {
    set_str(brand, max_len, tmp);
    return brand;
  }
  char impl[64], part[64], isa[256];
  if(read_cpuinfo_field("CPU implementer", impl, sizeof(impl)) &&
      read_cpuinfo_field("CPU part", part, sizeof(part))) {
    snprintf(tmp, sizeof(tmp), "ARM implementer %s part %s", impl, part);
      #if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) 
    for(arm64cpu_t *s = arm64cpu; s < arm64cpu + ARM64CPU_NUM; s++)
      if(s->implementer == impl && s->part == part) { 
        snprintf(tmp, sizeof(tmp), "%s", s->name);
        break;
      }
      #endif
    set_str(brand, max_len, tmp);
    return brand;
  }
  
  if(read_cpuinfo_field("isa", isa, sizeof(isa))) { //riscv64: no uarch on many kernels, fall back to the ISA string
    snprintf(tmp, sizeof(tmp), "RISC-V %s", isa);
    set_str(brand, max_len, tmp);
    return brand;
  }
  set_str(brand, max_len, "Unknown Linux CPU");
  return brand;
}
    #else
char *cpubrand(char* brand, size_t max_len) {
  strncpy(brand, "Unknown OS on ARM/RISC-V", max_len - 1);
  brand[max_len - 1] = '\0';
  return brand;
}
    #endif
  #else
char *cpubrand(char* brand, size_t max_len) {
  strncpy(brand, "Unkwoun Architecture", max_len - 1);
  brand[max_len - 1] = '\0';
  return brand;
}
  #endif

