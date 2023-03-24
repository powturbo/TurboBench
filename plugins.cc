/**
    Copyright (C) powturbo 2013-2023
    GPL v2 License

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    - homepage : https://sites.google.com/site/powturbo/
    - github   : https://github.com/powturbo
    - twitter  : https://twitter.com/powturbo
    - email    : powturbo [_AT_] gmail [_DOT_] com
**/
//      TurboBench: plugins.cc - compressor plugins
enum {
#define _MEMCPY 1
 P_LMCPY,   // must be 0
 P_MCPY,    // must be 1
#ifndef _AOM
#define _AOM 0
#endif
 P_AOM,
#ifndef _BRIEFLZ
#define _BRIEFLZ 0
#endif
 P_BRIEFLZ,
#ifndef _BROTLI
#define _BROTLI 0
#endif
 P_BROTLI,
#ifndef _BZIP2
#define _BZIP2 0
#endif
 P_BZIP2, 
#ifndef _BZIP3
#define _BZIP3 0
#endif
 P_BZIP3, 
#ifndef _CHAMELEON
#define _CHAMELEON 0
#endif
 P_CHAMELEON,
#ifndef _C_BLOSC2
#define _C_BLOSC2 0
#endif
 P_C_BLOSC2,
#ifndef _CSC
#define _CSC 0
#endif
 P_CSC,
#ifndef _DAALA
#define _DAALA 0
#endif
 P_DAALA,
#ifndef _DENSITY
#define _DENSITY 0
#endif
 P_DENSITY,
#ifndef _DOBOZ
#define _DOBOZ 0
#endif
 P_DOBOZ,
#ifndef _FASTLZ
#define _FASTLZ 0
#endif
 P_FASTLZ,
#ifndef _FLZMA2
#define _FLZMA2 0
#endif
 P_FLZMA2,
#ifndef _GIPFELI
#define _GIPFELI 0
#endif
 P_GIPFELI,
#ifndef _GLZA
#define _GLZA 0
#endif
 P_GLZA,
#ifndef _HEATSHRINK
#define _HEATSHRINK 0
#endif
 P_HEATSHRINK,
#ifndef _IGZIP
#define _IGZIP 0
#endif
 P_IGZIP,
#ifndef _LIBBSC
#define _LIBBSC 0
#endif
 P_LIBBSC,
 P_LIBBSCC,   //QLFC
 P_ST,        //st 
 P_LIBBSCBWT, //bwt

#ifndef _DIVBWT
#define _DIVBWT 0
#endif
 P_DIVBWT,    //bwt

#ifndef _LIBDEFLATE
#define _LIBDEFLATE 0
#endif
 P_LIBDEFLATE,
#ifndef _LIBLZF
#define _LIBLZF 0
#endif
 P_LIBLZF,
#ifndef _LIBLZG
#define _LIBLZG 0
#endif
 P_LIBLZG,
#ifndef _SLZ
#define _SLZ 0
#endif
 P_SLZ,
#ifndef _ZPAQ
#define _ZPAQ 0
#endif
 P_ZPAQ,
#ifndef _LZ4
#define _LZ4 0
#endif
 P_LZ4,
#ifndef _LZ4ULTRA
#define _LZ4ULTRA 0
#endif
 P_LZ4ULTRA,
#ifndef _LIZARD
#define _LIZARD 0
#endif
 P_LIZARD,
#ifndef _LZHAM
#define _LZHAM 0
#endif
 P_LZHAM,
#ifndef _LZFSE
#define _LZFSE 0
#endif
 P_LZFSE,
#ifndef _LZFSEA
#define _LZFSEA 0
#endif
 P_LZFSEA,
#ifndef _LZLIB
#define _LZLIB 0
#endif
 P_LZLIB,
#ifndef _LZMA
#define _LZMA 0
#endif
 P_LZMA,
#ifndef _LZMAT
#define _LZMAT 0
#endif
 P_LZMAT,
#ifndef _LZO
#define _LZO 0
#endif
 P_LZO1b, P_LZO1c, P_LZO1f, P_LZO1x, P_LZO1y, P_LZO1z, P_LZO2a,
#ifndef _LZOMA
#define _LZOMA 0
#endif
 P_LZOMA,
#ifndef _LZSA
#define _LZSA 0
#endif
 P_LZSA,
#ifndef _LZSSE
#define _LZSSE 0
#endif
 P_LZSSE2,
 P_LZSSE4,
 P_LZSSE8,
#ifndef _MINIZ
#define _MINIZ 0
#endif
 P_MINIZ,
#ifndef _MSCOMPRESS
#define _MSCOMPRESS 0
#endif
 P_MSCOMPRESS,
#ifndef _NAKA
#define _NAKA 0
#endif
 P_NAKA,
#ifndef _PITHY
#define _PITHY 0
#endif
 P_PITHY,
#ifndef _QUICKLZ
#define _QUICKLZ 0
#endif
 P_QUICKLZ,
#ifndef _SHRINKER
#define _SHRINKER 0
#endif
 P_SHRINKER,
#ifndef _PYSAP
#define _PYSAP 0
#endif
 P_PYSAP,
#ifndef _OODLE 
#define _OODLE 0
#endif
#ifndef _OODLESRC 
#define _OODLESRC 0
#endif
 P_OODLE, 
#ifndef _SHOCO
#define _SHOCO 0
#endif
 P_SHOCO,
#ifndef _SMAZ
#define _SMAZ 0
#endif
 P_SMAZ,
#ifndef _SNAPPY
#define _SNAPPY 0
#endif
 P_SNAPPY,
#ifndef _SNAPPY_C
#define _SNAPPY_C 0
#endif
 P_SNAPPY_C,
#ifndef _SMALLZ4
#define _SMALLZ4 0
#endif
 P_SMALLZ4,
#ifndef _TORNADO
#define _TORNADO 0
#endif
 P_TORNADO,
#ifndef _UNISHOX2
#define _UNISHOX2 0
#endif
 P_UNISHOX2,
#ifndef _UNISHOX3
#define _UNISHOX3 0
#endif
 P_UNISHOX3,
#ifndef _WFLZ
#define _WFLZ 0
#endif
 P_WFLZ,
#ifndef _YALZ77
#define _YALZ77 0
#endif
 P_YALZ77,
#ifndef _YAPPY
#define _YAPPY 0
#endif
 P_YAPPY,
#ifndef _ZLIB
#define _ZLIB 0
#endif
 P_ZLIB,
 P_ZRLE,
 P_ZLIBH,
#ifndef _ZLIB_NG
#define _ZLIB_NG 0
#endif
 P_ZLIB_NG,
#ifndef _ZLING
#define _ZLING 0
#endif
 P_ZLING,
#ifndef _ZOPFLI
#define _ZOPFLI 0
#endif
 P_ZOPFLI,
#ifndef _ZSTD
#define _ZSTD 0
#endif
 P_ZSTD,
#ifndef _FSE
#define _FSE 0
#endif
 P_FSE,
#ifndef _FSEHUF
#define _FSEHUF 0
#endif 
 P_FSEH,
  // --------- Encoding -------------------
#ifndef _TURBORLE
#define _TURBORLE 0
#endif
 P_RLES,
 P_RLET,

#ifndef _MRLE
#define _MRLE 0
#endif
 P_RLEM,
 
#ifndef _RLE8
#define _RLE8 0
#endif
 P_RLE8,
 
  //---------- Transform ------------------

#ifndef _BRC
#define _BRC 0
#endif
 P_BRC,
  // --------- Entropy coders -------------

#ifndef _FASTAC
#define _FASTAC 0
#endif
 P_FASTAC,
#ifndef _FASTARI
#define _FASTARI 0
#endif
 P_FASTARI,
#ifndef _FASTHF
#define _FASTHF 0
#endif
 P_FASTHF,
#ifndef _FREQTAB
#define _FREQTAB 0
#endif
 P_FREQTAB,
#ifndef _FSC
#define _FSC 0
#endif
 P_FSC,
#ifndef _FPC
#define _FPC 0
#endif
 P_FPC,
#ifndef _FQZ0
#define _FQZ0 0
#endif
 P_FQZ0,
#ifndef _JAC
#define _JAC 0
#endif
 P_JAC,
#ifndef _HTSCODECS
#define _HTSCODECS 0
#endif
 P_RANS_S8,
 P_RANS_S16,
 P_RANS_S32,
#ifndef _FPAQC
#define _FPAQC 0
#endif
 P_FPAQC,
#ifndef _MARLIN
#define _MARLIN 0
#endif
 P_MARLIN,
#ifndef _NIBRANS
#define _NIBRANS 0
#endif
 P_NIBRANS,
#ifndef _PPMDEC
#define _PPMDEC 0
#endif
 P_PPMDEC,
#ifndef _RECIPARITH
#define _RECIPARITH 0
#endif
 P_RECIPARITH,      
#ifndef _SHRC
#define _SHRC 0
#endif
 P_SHRC,
#ifndef _VECRC
#define _VECRC 0
#endif
 P_SHRCV,
#ifndef _SUBOTIN
#define _SUBOTIN 0
#endif
 P_SUBOTIN,
#ifndef _POLHF
#define _POLHF 0
#endif
 P_POLHF,
 P_TORNADOHF,
#ifndef _TURBORC
#define _TURBORC 0
#endif
 P_TURBORC,
#ifndef _XPACK
#define _XPACK 0
#endif
 P_XPACK,
 P_MYCODEC, // User plugin
  #ifdef _LZTURBO
#include "../dev/x/beplug.h"
  #endif
};

//-------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "conf.h"
#include "plugins.h"

  #if _AOM
#include "EC/aom_/aom.h"
  #endif

  #if _C_BLOSC2
    #ifdef C_C_BLOSC2LZ
#include "c-blosc2/blosclz.h"
    #else
#include "c-blosc2/include/blosc2.h"
    #endif
  #endif

  #if _BRIEFLZ
#include "brieflz/include/brieflz.h"
  #endif

  #if _LIBBSC
#include "libbsc/libbsc/libbsc.h"
#include "libbsc/libbsc/st/st.h"
#include "libbsc/libbsc/lzp/lzp.h"
  #endif

  #if _LIBDEFLATE
#include "libdeflate/libdeflate.h"
  #endif

  #if _BZIP2
#include "bzip2/bzlib.h"
  #endif

  #if _BROTLI
#include "brotli/c/include/brotli/encode.h"
#include "brotli/c/include/brotli/decode.h"
  #endif

  #if _LZMA
#include "lzma/C/Alloc.h"
#include "lzma/C/LzmaEnc.h"
#include "lzma/C/LzmaDec.h"
  #endif

  #if _CSC
#include "CSC/src/libcsc/csc_enc.h"
#include "CSC/src/libcsc/csc_dec.h"
struct MemISeqInStream {
  ISeqInStream   s;
  unsigned char *in;
  size_t         inlen;
};

struct MemISeqOutStream {
  ISeqOutStream  s;
  unsigned char *out;
  size_t         outlen;
};

static int cscread(MemISeqInStream *si, void *in, size_t *inlen) {
  if(*inlen > si->inlen) *inlen = si->inlen;
  memcpy(in, si->in, *inlen);
  si->in    += *inlen;
  si->inlen -= *inlen;
  return 0;
}

static size_t cscwrite(MemISeqOutStream *so, const void *out, size_t outlen) {
  memcpy(so->out, out, outlen);
  so->out    += outlen;
  so->outlen += outlen;
  return outlen;
}
  #endif

  #if _DAALA
#include "EC/daala_/daala.h"
  #endif

  #if _DOBOZ
#include "doboz/Source/Doboz/Compressor.h"
#include "doboz/Source/Doboz/Decompressor.h"
  #endif

  #if _FLZMA2
#include "fast-lzma2/lzma2_enc.h"
#include "fast-lzma2/lzma2_dec.h"
  #endif

  #if _GIPFELI
#include "gipfeli/gipfeli.h"
  #endif

  #if _GLZA
#include "glza/GLZAcomp.h"
#include "glza/GLZAdecode.h"
  #endif

  #if _HEATSHRINK
#include "heatshrink_/heatshrink.h"
  #endif

  #if _IGZIP
    #ifdef HAVE_IGZIP
#include <isa-l.h>
    #else
#include "isa-l/include/igzip_lib.h"
    #endif
  #endif

  #if _LIBLZG
#include "liblzg/src/include/lzg.h"
  #endif

  #if _ZPAQ
#include "zpaq/libzpaq.h"
void libzpaq::error(const char* msg) {
  fprintf(stderr, "zpaq error: %s\n", msg);
  exit(1);
}

static unsigned char *zin,*zin_;
static unsigned char *zout;
#define _putc(__ch, __out) *__out++ = (__ch)
#define _getc(in, in_) (in<in_?*in++:-1)

class In: public libzpaq::Reader {
  public:
    int get() { return _getc(zin, zin_); }
} zmemin;

class Out: public libzpaq::Writer {
  public:
    void put(int c) { _putc(c, zout); }
} zmemout;
  #endif

  #if _LZ4
#include "lz4/lib/lz4.h"
#include "lz4/lib/lz4hc.h"
#include "lz4/lib/lz4frame.h"
  #endif

  #if _LIZARD
#include "lizard/lib/lizard_compress.h"    //v2.0
#include "lizard/lib/lizard_decompress.h"
  #endif

  #if _LZFSEA
#include <compression.h>
  #endif

  #if _LZHAM
#include "lzham_codec_devel/include/lzham.h"
  #endif

  #if _LZLIB
#include "lzlib_/bbexample.h"
  struct Lzma_options
    {
    int dictionary_size;        /* 4 KiB .. 512 MiB */
    int match_len_limit;        /* 5 .. 273 */
    };
  /* Mapping from gzip/bzip2 style 1..9 compression modes
     to the corresponding LZMA compression modes. */
  const struct Lzma_options option_mapping[] =
    {
    {   65535,  16 },       /* -0 (65535,16 chooses fast encoder) */
    { 1 << 20,   5 },       /* -1 */
    { 3 << 19,   6 },       /* -2 */
    { 1 << 21,   8 },       /* -3 */
    { 3 << 20,  12 },       /* -4 */
    { 1 << 22,  20 },       /* -5 */
    { 1 << 23,  36 },       /* -6 */
    { 1 << 24,  68 },       /* -7 */
    { 3 << 23, 132 },       /* -8 */
    { 1 << 25, 273 } };     /* -9 */
  struct Lzma_options encoder_options;
  #endif

  #if _LZMAT
#include "lzmat/lzmat.h"
  #endif

  #if _LZO
#include "lzo/include/lzo/lzo1b.h"
#include "lzo/include/lzo/lzo1c.h"
#include "lzo/include/lzo/lzo1f.h"
#include "lzo/include/lzo/lzo1x.h"
#include "lzo/include/lzo/lzo1y.h"
#include "lzo/include/lzo/lzo1z.h"
#include "lzo/include/lzo/lzo2a.h"
  #endif

  #if _LZOMA
#include "lzoma_/lzoma.h"
  #endif

  #if _LZSA
#include "lzsa/src/shrink_inmem.h"
#include "lzsa/src/expand_inmem.h"
#define LZSA_FLAG_FAVOR_RATIO    (1<<0)      /**< 1 to compress with the best ratio, 0 to trade some compression ratio for extra decompression speed */
#define LZSA_FLAG_RAW_BLOCK      (1<<1)      /**< 1 to emit raw block */
  #endif

  #if _LZSSE
#include "LZSSE/lzsse2/lzsse2.h"
#include "LZSSE/lzsse4/lzsse4.h"
#include "LZSSE/lzsse8/lzsse8.h"
  #endif

  #if _MSCOMPRESS
#include "ms-compress/include/mscomp.h"
  #endif

  #if _LZSS
#include "lzss/lzss.h"
  #endif

  #if _NAKA
#include "nakamichi/nakamichi.h"
  #endif

  #if _PITHY
#include "pithy/pithy.h"
  #endif

  #if _QUICKLZ
#include "quicklz_/quicklz-c.h"
  #endif

  #if _PYSAP
#include "pysap/pysapcompress/hpa101saptype.h"
#include "pysap/pysapcompress/hpa104CsObject.h"
#include "pysap/pysapcompress/hpa106cslzc.h"
#include "pysap/pysapcompress/hpa107cslzh.h"
#include "pysap/pysapcompress/hpa105CsObjInt.h"
#undef max
#undef min
  #endif

  #if _SHRINKER
#include "shrinker/Shrinker.h"
  #endif

  #if _SHOCO
#include "shoco/shoco.h"
  #endif

  #if _SNAPPY
#include "snappy/snappy.h"
  #endif

  #if _TORNADO
#include "tornado_/tormem.h"
  #endif

  #if _TURBORC
#include "Turbo-Range-Coder/include/turborc.h"
//#include "Turbo-Range-Coder/rcutil.h"
  #endif

  #if _WFLZ
#include "wflz/wfLZ.h"
  #endif

  #if _WIMLIB
#include "../wimlib/include/wimlib.h"
  #endif

  #if _XPACK
#include "xpack/libxpack.h"
  #endif

  #if _YALZ77
#include "yalz77/lz77.h"
  #endif

  #if _YAPPY
#include "yappy/yappy.hpp"
  #endif

  #if _ZLIB || _SLZ
     #if _ZLIBLIB
#include <zlib.h>
//     #elif defined(ZLIB_NG)          // zlib-ng.a compatible mode : "./configure --zlib-compat" (see zlib-ng/INSTALL)
//#include "zlib/zlib.h"
     #elif defined(ZLIB_INTEL)
#include "zlib_intel/zlib.h"
     #else
#include "zlib/zlib.h"
     #endif
  #endif


  #if _ZLING
#include "libzling/src/libzling.h"
#include "libzling_/libzling_utils_mem.h"
  #endif

  #ifdef _LZTURBO
#include "../dev/x/beplugi.h"
  #endif
//-----------------------------
  #if _BRC
#include "Behemoth-Rank-Coding/brc.hpp"
int vsrc_forwards(unsigned char * src, unsigned char * dst, size_t src_size);
int vsrc_reverse(unsigned char * src, unsigned char * dst, size_t src_size);
  #endif

  #if _RLE8
#include "rle8/src/rle8.h"
  #endif

  #if __cplusplus
extern "C" {
  #endif

  #if _BZIP3
#include "bzip3/include/libbz3.h"
  #endif

  #if _CHAMELEON
#include "chameleon/Chameleon2.h"
  #endif

  #if _DENSITY
#include "density/src/density_api.h"
  #endif

  #if _FASTLZ
#include "FastLZ/fastlz.h"
  #endif

  #if _LIBLZF
#include "liblzf/lzf.h"
  #endif

  #if _SLZ
#include "libslz/src/slz.h"
  #endif

  #if _LZFSE
#include "lzfse/src/lzfse.h"
  #endif

  #if _MINIZ
typedef unsigned long mz_ulong;
int mz_compress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len, int level);
int mz_uncompress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len);
  #endif

  #if _OODLE
#ifndef _WIN32
#include <dlfcn.h>  
#endif
typedef struct OodleLZ_CompressOptions {
  char dummy[255];
};
#define __cdecl
typedef long long (__cdecl *fOodleLZ_Compress)(int codec, void *in, long long inlen, void *out, int lev, void *options, void *dict, void *p6, void *tmp, long long tmplen);
typedef long long (__cdecl *fOodleLZ_Decompress)(void *in, long long inlen, void *out, long long outlen, int crc, int p5, long long verb, void *dic, long long diclen, void *p9, long long p10, void *p11=0, long long p12=0, int p13=0);
typedef struct OodleLZ_CompressOptions *(__cdecl *fOodleLZ_CompressOptions_GetDefault)(int compid, int level);
static fOodleLZ_Compress                   OodleLZ_Compress_;
static fOodleLZ_Decompress                 OodleLZ_Decompress_;
static fOodleLZ_CompressOptions_GetDefault OodleLZ_CompressOptions_GetDefault_;
  #endif  
  #if _SNAPPY_C
#include "snappy-c/snappy.h"
struct snappy_env env;
  #endif

  #if _SMALLZ4
#include "smallz4/smallz4.h"
#include "smallz4/smallz4cat.c"
  #endif

  #if _SMAZ
#include "smaz/smaz.h"
  #endif

  #if _UNISHOX2
int unishox2_compressx(  const char *in, int inlen, char *out, int lev);
int unishox2_decompressx(const char *in, int inlen, char *out, int lev);
  #endif

  #if _UNISHOX3
#include "unishox2/Unishox3_Alpha/unishox3.h"
  #endif

  #if _ZLIB_NG
#include "zlib-ng/zconf-ng.h"
//#include "zlib-ng/zlib-ng.h"     
ZEXTERN ZEXPORT const char *zlibng_version(void);
ZEXTERN ZEXPORT int zng_compress2( unsigned char *dest, size_t *destLen, const unsigned char *source, size_t sourceLen, int level);
ZEXTERN ZEXPORT int zng_uncompress(unsigned char *dest, size_t *destLen, const unsigned char *source, size_t sourceLen);
  #endif

  #if _ZOPFLI
#include "zopfli/src/zopfli/zopfli.h"
  #endif

  #if __cplusplus
}
  #endif
  //------------------------------------ Encoding -----------------------------------
  #if _TURBORLE
#include "Turbo-Run-Length-Encoding/trle.h"
  #endif

  #if _MRLE
#include "Turbo-Run-Length-Encoding/ext/mrle.h"
  #endif
  
  #if _TB64
#include "TurboBase64/turbob64.h"
  #endif

  #if _SB64
//#include "base64simd/encode/lookup.sse.cpp"
//#include "base64simd/decode/decode.sse.cpp"
  #endif

  #if _B64
#include "base64/include/libbase64.h"
  #endif

  #if _FB64
#include "fastbase64/include/chromiumbase64.h"
#include "fastbase64/include/scalarbase64.h"
#include "fastbase64/include/linuxbase64.h"
   #ifdef AVX2_ON
#include "fastbase64/include/fastavxbase64.h"
#include "fastbase64/include/klompavxbase64.h"
   #endif
#ifdef HAVE_AVX512BW
#include "fastbase64/include/fastavxbase64.h"
#endif // HAVE_AVX512BW
  #endif
  #if __ARM_NEON
int neon_base64_decode(char *out, const char *src, size_t srclen);
  #endif
  #if __cplusplus
extern "C" {
  #endif

  #if _LZ4ULTRA
#define LZ4ULTRA_FLAG_FAVOR_RATIO    (1<<0)           /**< 1 to compress with the best ratio, 0 to trade some compression ratio for extra decompression speed */
#define LZ4ULTRA_FLAG_RAW_BLOCK      (1<<1)           /**< 1 to emit raw block */
#define LZ4ULTRA_FLAG_INDEP_BLOCKS   (1<<2)           /**< 1 if blocks are independent, 0 if using inter-block back references */
#define LZ4ULTRA_FLAG_LEGACY_FRAMES  (1<<3)           /**< 1 if using the legacy frames format, 0 if using the modern lz4 frame format */
//#include "lz4ultra/src/lib.h"
//#include "lz4ultra/src/shrink_inmem.h"
//#include "lz4ultra/src/expand_inmem.h"

size_t lz4ultra_compress_inmem(const unsigned char *pInputData, unsigned char *pOutBuffer, size_t nInputSize, size_t nMaxOutBufferSize, unsigned int nFlags,  int nBlockMaxCode);
size_t lz4ultra_decompress_inmem(const unsigned char *pFileData, unsigned char *pOutBuffer, size_t nFileSize, size_t nMaxOutBufferSize, unsigned int nFlags);
  #endif

  #if __cplusplus
}
  #endif


  //------------------------------------ Transform ----------------------------------
  #if _LIBBSC
#include "libbsc/libbsc/bwt/bwt.h"
#include "libbsc/libbsc/coder/coder.h"
  #endif
  
  #if _DIVBWT
#include "Turbo-Range-Coder/libdivsufsort/include/divsufsort.h"
 #endif
  
  //------------------------------------ Entropy Coder ------------------------------
  #if _FASTAC
#include "EC/fastac/arithmetic_codec.h"
  #endif

  #if _FASTHF
#include "EC/fasthf/binary_codec.h"
  #endif

  #if _FPC
#include "EC/FPC/fpc.h"
  #endif

  #if _FQZ
#include "EC/fqz0/f_o0.h"
  #endif

/*  #if _JAC
#include "EC/rans_static_/arith_static.h"
  #endif*/

  #if _HTSCODECS
#include "EC/htscodecs/htscodecs/config.h"  
#include "EC/htscodecs/htscodecs/rANS_static32x16pr.h"
  #endif

  #if _FPAQC
#include "EC/fpaqc/fpaqc.h"
  #endif

  #if _FREQTAB
#include "EC/freqtab/freqtab.h"
  #endif

  #if _MMRC
#include "EC/fpaq0f2/fpaq0f2.h"
#include "EC/fpaq0p/fpaq0p_mm.h"
#include "EC/o0zp/o0zp.h"
  #endif

  #if _NANS
#include "EC/ans_nania/narans.h"
  #endif

  #if _MARLIN
#include "marlin/inc/marlin.h"
/*struct MarlinDictionary;
//#include "marlin/inc/marlin.h"
ssize_t Marlin_compress(const MarlinDictionary *dict, uint8_t* dst, size_t dstCapacity, const uint8_t* src, size_t srcSize);
ssize_t Marlin_decompress(const MarlinDictionary *dict, uint8_t* dst, size_t dstSize, const uint8_t* src, size_t srcSize);
MarlinDictionary *Marlin_build_dictionary(const char *name, const double hist[256]);
void Marlin_free_dictionary(MarlinDictionary *dict);
const MarlinDictionary **Marlin_get_prebuilt_dictionaries();
const MarlinDictionary * Marlin_estimate_best_dictionary(const MarlinDictionary **dict, const uint8_t* src, size_t srcSize);*/
  #endif

  #if _POLHF
#include "EC/polar/polar.h"
  #endif

  #if _PPMDEC
#include "EC/ppmdec/ppmdec.h"
  #endif

  #if _RECIPARITH
#include "EC/recip_arith_/reciparith.h"
  #endif

  #if _SUBOTIN
#include "EC/subotin_/subotin.h"
  #endif

  #if _SHRC
#include "EC/fpaq0p/fpaq0p_sh.h"
#include "EC/vecrc/vector_rc.h"
  #endif

  #if _MYCODEC
//#include "my_header.h"
  #endif

  #if __cplusplus
extern "C" {
  #endif

  #if _FASTARI
#include "EC/FastARI/FastAri.h"
  #endif

  #if _FSC
#include "EC/fsc/fsc.h"
  #endif

  #if _NIBRANS
#define NIBRANS_STATIC
#include "EC/nibrans/nibrans.h"
  #endif

/*  #if _RANS_S
#include "EC/rans_static/rANS_static.h"
unsigned char *rans_compress_to_32x16(unsigned char *in,  unsigned int in_size,
                     unsigned char *out, unsigned int *out_size,
                     int order);
unsigned char *rans_uncompress_to_32x16(unsigned char *in,  unsigned int in_size,
                       unsigned char *out, unsigned int *out_size,
                       int order);
#define X_PACK 0x80
#define X_RLE  0x40
  #endif*/

  #if __cplusplus
}
  #endif

  #if _ZSTD
#include "zstd/lib/zstd.h"
#include "zstd/examples/common.h"
#include "zstd/lib/common/fse.h"
#include "zstd/lib/common/huf.h"

static ZSTD_CDict* createCDict_orDie(const char* dictFileName, int cLevel) {
  size_t dictSize;
  void* const dictBuffer = mallocAndLoadFile_orDie(dictFileName, &dictSize);
  ZSTD_CDict* const cdict = ZSTD_createCDict(dictBuffer, dictSize, cLevel);
  free(dictBuffer);
  return cdict;
}

static ZSTD_DDict* createDDict_orDie(const char* dictFileName) {
  size_t dictSize;
  void* const dictBuffer = mallocAndLoadFile_orDie(dictFileName, &dictSize);
  ZSTD_DDict* const ddict = ZSTD_createDDict(dictBuffer, dictSize);
  free(dictBuffer);
  return ddict;
}
  #endif

//------------------------------------------------- registry -------------------------------------------------------------------------------------------------
struct plugs plugs[] = {
  { P_C_BLOSC2,  "blosc",       _C_BLOSC2,  "Blosc",                   "0,1,2,3,4,5,6,7,8,9", 64*1024},
  { P_BRIEFLZ,   "brieflz",     _BRIEFLZ,   "BriefLz",                 "1,3,6,9" },
  { P_BROTLI,    "brotli",      _BROTLI,    "Brotli",                  "0,1,2,3,4,5,6,7,8,9,10,11/d#:V"},
  { P_BZIP2,     "bzip2",       _BZIP2,     "Bzip2",                   "" },
  { P_BZIP3,     "bzip3",       _BZIP3,     "Bzip3",                   "" },
  { P_CHAMELEON, "chameleon",   _CHAMELEON, "Chameleon",               "1,2" },
  { P_CSC,       "csc",         _CSC,       "CSC",                     "1,2,3,4,5" },
  { P_DENSITY,   "density",     _DENSITY,   "Density",                 "1,2,3" },
  { P_DOBOZ,     "doboz",       _DOBOZ,     "Doboz",                   "" },  //crash on windows
  { P_FASTLZ,    "fastlz",      _FASTLZ,    "FastLz",                  "1,2" },
  { P_FLZMA2,    "flzma2",      _FLZMA2,    "Fast-lzma2",              "0,1,2,3,4,5,6,7,8,9,10,11/mt#" },
  { P_GIPFELI,   "gipfeli",     _GIPFELI,   "Gipfeli",                 "" },
  { P_GLZA,      "glza",        _GLZA,      "glza",                    "" },
  { P_HEATSHRINK,"heatshrink",  _HEATSHRINK,"heatshrink",              "" },
  { P_IGZIP,     "igzip",       _IGZIP,     "igzip",                   "0,1,2,3" },
  { P_LIBBSC,    "bsc",         _LIBBSC,    "bsc",                     "0,3,4,5,6,7,8/p:e#"},
  { P_LIBBSCC,   "bscqlfc",     _LIBBSC,    "bsc",                     "1,2"},
  { P_LIBDEFLATE,"libdeflate",  _LIBDEFLATE,"libdeflate",              "1,2,3,4,5,6,7,8,9,12/dg"},
  { P_LIBLZF,    "lzf",         _LIBLZF,    "LibLZF",                  "" },
  { P_LIBLZG,    "lzg",         _LIBLZG,    "LibLzg",                  "1,2,3,4,5,6,7,8,9" }, //"https://gitorious.org/liblzg" BLOCKSIZE must be < 64MB
  { P_ZPAQ,      "zpaq",        _ZPAQ,      "Libzpaq",                 "0,1,2,3,4,5" },
  { P_SLZ,       "slz",         _SLZ,       "libslz",                  "0,1,2,3,4,5,6,7,8,9" },
  { P_LZ4,       "lz4",         _LZ4,       "Lz4",                     "0,1,9,10,11,12,16/MfsB#" },
  { P_LZ4ULTRA,  "lz4ultra",    _LZ4ULTRA,  "Lz4ultra",                "9,10,11,12/z" },
  { P_LIZARD,    "lizard",      _LIZARD,    "Lizard",                  "10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49" },
  { P_LZFSE,     "lzfse",       _LZFSE,     "lzfse",                   "" },  
  { P_LZFSEA,    "lzfsea",      _LZFSEA,    "lzfsea",                  "" },
  { P_LZHAM,     "lzham",       _LZHAM,     "Lzham",                   "1,2,3,4/t#:fb#:x#" },
  { P_LZLIB,     "lzlib",       _LZLIB,     "Lzlib",                   "1,2,3,4,5,6,7,8,9/d#:fb#" },
  { P_LZMAT,     "lzmat",       _LZMAT,     "Lzmat",                   "" },
  { P_LZMA,      "lzma",        _LZMA,      "Lzma",                    "0,1,2,3,4,5,6,7,8,9/d#:fb#:lp#:lc#:pb#:a#:mt#" },

  { P_LZO1b,     "lzo1b",       _LZO,       "Lzo",                     "1,9,99,999" },
  { P_LZO1c,     "lzo1c",       _LZO,       "Lzo",                     "1,9,99,999" },
  { P_LZO1f,     "lzo1f",       _LZO,       "Lzo",                     "1,999" },
  { P_LZO1x,     "lzo1x",       _LZO,       "Lzo",                     "1,11,12,15,999" },
  { P_LZO1y,     "lzo1y",       _LZO,       "Lzo",                     "1,999" },
  { P_LZO1z,     "lzo1z",       _LZO,       "Lzo",                     "999" },
  { P_LZO2a,     "lzo2a",       _LZO,       "Lzo",                     "999" },
  { P_LZOMA,     "lzoma",       _LZOMA,     "lzoma",                   "1,2,3,4,5,6,7,8,9" },
  { P_LZSA,      "lzsa",        _LZSA,      "lzsa",                    "9/f#cr" },
  { P_LZSSE2,    "lzsse2",      _LZSSE,     "lzsse",                   "1,2,3,4,5,6,7,8,9,12,16,17"},
  { P_LZSSE4,    "lzsse4",      _LZSSE,     "lzsse",                   "0,1,2,3,4,5,6,7,8,9,12,16,17"},
  { P_LZSSE8,    "lzsse8",      _LZSSE,     "lzsse",                   "0,1,2,3,4,5,6,7,8,9,12,16,17"},
  { P_MINIZ,     "miniz",       _MINIZ,     "miniz zlib-replace",      "1,2,3,4,5,6,7,8,9" },
  { P_MSCOMPRESS,"mscompress",  _MSCOMPRESS,"ms-compress",             "2,3,4" },
  { P_NAKA,      "naka",        _NAKA,      "Nakamichi Washigan",      "" },
  { P_PITHY,     "pithy",       _PITHY,     "Pithy",                   "0,1,2,3,4,5,6,7,8,9" },
  { P_QUICKLZ,   "quicklz",     _QUICKLZ,   "Quicklz",                 "1,2,3" },
  { P_PYSAP,     "sap",         _PYSAP,     "sap",                     "0,1,2" },
  { P_SHRINKER,  "shrinker",    _SHRINKER,  "Shrinker",                "", 0, (1<<26) },
  { P_SHOCO,     "shoco",       _SHOCO,     "Shoco",                   "" },
  { P_SMAZ,      "smaz",        _SMAZ,      "smaz",                    "" },
  { P_SNAPPY,    "snappy",      _SNAPPY,    "Snappy",                  ""  },
  { P_SNAPPY_C,  "snappy_c",    _SNAPPY_C,  "Snappy-c",                "" },
  { P_SMALLZ4,   "smallz4",     _SMALLZ4,   "SmalLz4",                 "6,7,8,9,10,11,12/z" },
  { P_TORNADO,   "tornado",     _TORNADO,   "Tornado",                 "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16" },
  { P_UNISHOX2,  "unishox2",    _UNISHOX2,  "unishox2",                "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16" },
  { P_UNISHOX3,  "unishox3",    _UNISHOX3,  "unishox3",                "" },
  { P_WFLZ,      "wflz",        _WFLZ,      "wfLZ",                    "1,2" },
  { P_XPACK,     "xpack",       _XPACK,     "xpack",                   "1,2,3,4,5,6,7,8,9" },
  { P_YALZ77,    "yalz77",      _YALZ77,    "Yalz77",                  "1,6,12" },
  { P_YAPPY,     "yappy",       _YAPPY,     "Yappy",                   "" },//crash windows
  { P_ZLIB,      "zlib",        _ZLIB,      "zlib",                    "1,2,3,4,5,6,7,8,9" },
  { P_ZLIB_NG,   "zlib_ng",     _ZLIB_NG,   "zlib-ng",                 "1,2,3,4,5,6,7,8,9" },
  { P_ZLING,     "zling",       _ZLING,     "Libzling",                "0,1,2,3,4" },
  { P_ZOPFLI,    "zopfli",      _ZOPFLI,    "Zopfli",                  ""},
  { P_ZSTD,      "zstd",        _ZSTD,      "ZSTD",                    "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15,-16,-17,-18,-19,-20,-21,-22/d#" },
//------------------------------------------------------------------
  { P_MCPY,      "imemcpy",     _MEMCPY,    "inline memcpy",           "" },
  { P_LMCPY,     "memcpy",      _MEMCPY,    "library memcpy",          "" },
  { P_AOM,       "AOM",         _AOM,       "AV1 Entropy coder",       ""},
  { P_DAALA,     "Daala",       _DAALA,     "DAALA Entropy Coder",     ""},
  { P_FPC,       "fpc",         _FPC,       "Fast Prefix Coder",       "0,8,9,10,11,12,16,32,48,63" },
  { P_FREQTAB,   "freqtab",     _FREQTAB,   "FreqTable v2.Eugene shelwien", "" },
  { P_FSC,       "fsc",         _FSC,       "Finite State Coder",      "", E_ANS },
  { P_FSE,       "fse",         _FSE,       "Finite State Entropy",    "", E_ANS },
  { P_FSEH,      "fsehuf",      _FSEHUF,    "Zstd Huffman Coding",     "", E_HUF },
  { P_FPAQC,     "fpaqc",       _FPAQC,     "Asymmetric Binary Coder", "" },
  { P_SHRC,      "fpaq0p_sh",   _SHRC,      "Bitwise RC",              "" },
  { P_SHRCV,     "vecrc_sh",    _VECRC,     "Bitwise vector RC",       "" },
  { P_FASTHF,    "FastHF",      _FASTHF,    "Fast HF",                 "" },
  { P_FASTARI,   "FastAri",     _FASTARI,   "FastAri",                 "" },
  { P_FASTAC,    "FastAC",      _FASTAC,    "Fast AC",                 "" },
  { P_JAC,       "arith_static",_JAC,       "Range Coder/J.Bonfield",  "", E_ANS},
  { P_FQZ0,      "fqz0",        _FQZ0,      "FQZ/PPMD Range Coder",    ""},
  { P_MARLIN,    "Marlin",      _MARLIN,    "Marlin Entropy coder",    ""},
  { P_NIBRANS,   "nibrans",     _NIBRANS,   "nibrans",                 ""},
  { P_OODLE, 	 "oodle", 		_OODLE, 	"Oodle 8:Kraken 9:Mermaid 11:Selkie 12:Hydra 13:Leviathan", "01,02,03,04,05,06,07,08,09,11,12,13,14,15,16,17,18,19,21,22,23,24,25,26,27,28,29,41,42,43,44,45,46,47,48,49,51,52,53,54,55,56,57,58,59,61,62,63,64,65,66,67,68,69,71,72,73,74,75,76,77,78,79,81,82,83,84,85,86,87,88,89,-81,-82,-83,91,92,93,94,95,96,97,98,99,-91,-92,-93,101,102,103,104,105,106,107,108,109,111,112,113,114,115,116,117,118,119,-111,-112,-113,121,122,123,124,125,126,127,128,129,131,132,133,134,135,136,137,138,139" },
  { P_POLHF,     "polar",       _POLHF,     "Polar Codes",             "" },
  { P_PPMDEC,    "ppmdec",      _PPMDEC,    "PPMD Range Coder",        ""},

  { P_RANS_S8,   "rans_s8",     _HTSCODECS,    "ANS/J.Bonfield",          "0,1", E_ANS},
  { P_RANS_S16,  "rans_s16",    _HTSCODECS,    "ANS/J.Bonfield",          "0,1", E_ANS},
  { P_RANS_S32,  "rans_s32",    _HTSCODECS,    "ANS/J.Bonfield",          "0,1", E_ANS},
  { P_RECIPARITH,"recip_arith", _RECIPARITH,"recip arith",			   "" },
  { P_SUBOTIN,   "subotin",     _SUBOTIN,   "subotin RC",              "" },
  { P_TORNADOHF, "tornado_huff",_TORNADO,   "Tornado Huf",             "" },
  { P_TURBORC,   "TurboRC",     _TURBORC,   "Turbo Range Coder",       "1,2,3,4,9,10,12,14,17,20,21,90/e#" }, 
  { P_ZLIBH,     "zlibh",       _ZLIB,      "zlib Huffmann",           "8,9,10,11,12,13,14,15,16,32" },
  { P_ZRLE,      "zlibrle",     _ZLIB,      "zlib rle",                "" },
  //---- Encoding ------
  { P_RLES,      "srle",        _TURBORLE,  "TurboRLE ESC",            "0,8,16,32,64" },
  { P_RLET,      "trle",        _TURBORLE,  "TurboRLE",                "" },
  { P_RLEM,      "mrle",        _MRLE,      "Mespostine RLE",          "" },
  { P_RLE8,      "rle8",        _RLE8,      "8 bit RLE",               "1,2,8,16,24,32,48,64/S#s (S:Subsection, s:single)" },
  //----- Transform -----
  { P_DIVBWT,    "divbwt",      _DIVBWT,    "bwt libdivsufsort","" },
  { P_LIBBSCBWT, "bscbwt",      _LIBBSC,    "bwt libbsc",              "" },
  { P_ST,        "st",          _LIBBSC,    "st  libbsc",              "3,4,5,6,7,8" },
  { P_BRC,       "brc",         _BRC,       "Behemoth-Rank-Coding",    "0,1" },
//{ P_MYCODEC,   "mycodec",     _MYCODEC,  "0",        "My codec",             "           ",      "",                                                                                     "" },
    #ifdef _LZTURBO
  #include "../dev/x/beplugr.h"
    #endif
  { -1 }
};
#undef DF
//---------------------------------------------- plugins --------------------------------------------------------
  #ifndef max
#define max(x,y) (((x)>(y)) ? (x) : (y))
#define min(x,y) (((x)<(y)) ? (x) : (y))
  #endif

#define Kb (1u<<10)
#define Mb (1u<<20)
#define Gb (1u<<30)
#define KB 1000
#define MB 1000000
#define GB 1000000000

unsigned argtoi(char *s, unsigned def) {
  char *p;
  unsigned n = strtol(s, &p, 10),f = 1;
  switch(*p) {
    case 'B': f = 1; break;
    case 'K': f = KB; break;
    case 'M': f = MB; break;
    case 'G': f = GB; break;
    case 'k': f = Kb; break;
    case 'm': f = Mb; break;
    case 'g': f = Gb; break;
    case 'b': def = 0;
    default: if(!def) return n>=32?0xffffffffu:(1u << n); f = def;
  }
  return n*f;
}

NOINLINE void libmemcpy(unsigned char *dst, unsigned char *src, int len) {
  void *(*memcpy_ptr)(void *, const void *, size_t) = memcpy;
  if (time(NULL) == 1)
    memcpy_ptr = NULL;
  memcpy_ptr(dst, src, len);
}

static int dicsize;
int coddicsize(int _dicsize) { dicsize = _dicsize; }

  #if _ZSTD
static ZSTD_CDict *cdictPtr;
static ZSTD_DDict *ddictPtr;
  #endif

static char _workmem[1<<16],*workmem=_workmem;
static int state_size,dstate_size;
static size_t workmemsize;

int codini(size_t insize, int codec, int lev, char *prm) {
  workmemsize = 0;

  switch(codec) {
      #if _C_BLOSC2
    case P_C_BLOSC2: blosc2_init(); blosc2_set_nthreads(1);break;
      #endif

      #if _FASTARI
    case P_FASTARI: workmemsize = FA_WORKMEM; break;
      #endif

      #if _LZFSE
    case P_LZFSE:
      { size_t lzfse_size = lzfse_encode_scratch_size();
        workmemsize = max(lzfse_size, workmemsize);
        lzfse_size = lzfse_decode_scratch_size();
        workmemsize = max(lzfse_size, workmemsize);
      }
      break;
      #endif

      #if _LZFSEA
    case P_LZFSEA:
      { size_t lzfsea_size = compression_encode_scratch_buffer_size(COMPRESSION_LZFSE);
        workmemsize = max(lzfsea_size, workmemsize);
        lzfsea_size = compression_decode_scratch_buffer_size(COMPRESSION_LZFSE);
        workmemsize = max(lzfsea_size, workmemsize);
      }
      break;
      #endif

      #if _LZO
    case P_LZO1b: lzo_init(); workmemsize = LZO1B_999_MEM_COMPRESS; break;
    case P_LZO1c: P_LZO1f: P_LZO1x: P_LZO1y: P_LZO1z: P_LZO2a: lzo_init(); workmemsize = LZO1X_MEM_COMPRESS; break;
      #endif

      #if _CHAMELEON
    case P_CHAMELEON: workmemsize = sizeof(struct Chameleon); break;
      #endif

      #if _OODLE
    case P_OODLE: 
      char oodle[65];
      if(!OodleLZ_Compress_) 
        #if _WIN32
      { HINSTANCE hdll; int i;  
	for(i = 9; i >= 4; i--) {
	  sprintf(oodle, "oo2core_%d_win64.dll", i);
	  if(hdll = LoadLibrary(oodle)) break;
        }
  	if(!hdll) { printf("oo2core_?_win64.dll not found\n"); exit(-1); }
	if(!(OodleLZ_Compress_   = (fOodleLZ_Compress  )GetProcAddress(hdll, "OodleLZ_Compress"  ))) { printf("OodleLZ_Compress not found\n");   exit(-1); }
	if(!(OodleLZ_Decompress_ = (fOodleLZ_Decompress)GetProcAddress(hdll, "OodleLZ_Decompress"))) { printf("OodleLZ_Decompress not found\n"); exit(-1); }
	if(!(OodleLZ_CompressOptions_GetDefault_ = (fOodleLZ_CompressOptions_GetDefault)GetProcAddress(hdll, "OodleLZ_CompressOptions_GetDefault"))) { printf("OodleLZ_CompressOptions_GetDefault not found\n"); exit(-1); }
      } 
        #else
      { 
          #ifdef __aarch64__
        strcpy(oodle, "./liboo2corelinuxarm64.so.9");
          #else
        strcpy(oodle, "./liboo2corelinux64.so.9");
          #endif
        void *hdll = dlopen(oodle, RTLD_LAZY);
        if(!hdll) die("oodle shared library '%s' not found.'%s'\n", oodle, dlerror());
        if(!(OodleLZ_Compress_   = (fOodleLZ_Compress  )dlsym(hdll, "OodleLZ_Compress"  ))) { printf("OodleLZ_Compress not found\n");   exit(-1); }
	if(!(OodleLZ_Decompress_ = (fOodleLZ_Decompress)dlsym(hdll, "OodleLZ_Decompress"))) { printf("OodleLZ_Decompress not found\n"); exit(-1); }
	if(!(OodleLZ_CompressOptions_GetDefault_ = (fOodleLZ_CompressOptions_GetDefault)dlsym(hdll, "OodleLZ_CompressOptions_GetDefault"))) { 
	 printf("OodleLZ_CompressOptions_GetDefault not found\n"); exit(-1); }
      }
      #endif 
      break;
      #endif

      #if _QUICKLZ
    case P_QUICKLZ:
      state_size  = max(qlz_get_setting1(1), max(qlz_get_setting2(1), qlz_get_setting3(1))); workmemsize = max(state_size, workmemsize);
      dstate_size = max(qlz_get_setting1(2), max(qlz_get_setting2(2), qlz_get_setting3(2))); workmemsize = max(dstate_size, workmemsize);
      break;
      #endif

      #if _BRIEFLZ
    case P_BRIEFLZ: workmemsize = blz_workmem_size_level(insize, lev); break;
      #endif

      #if _SNAPPY_C
    case P_SNAPPY_C: snappy_init_env(&env); break;
      #endif

      #if _WFLZ
    case P_WFLZ: workmemsize = wfLZ_GetWorkMemSize(); break;
      #endif

      #if _YAPPY
    case P_YAPPY: YappyFillTables(); break;
      #endif

      #if _LIBBSC
    case P_LIBBSC: case P_LIBBSCC: bsc_init(LIBBSC_FEATURE_FASTMODE); bsc_st_init(LIBBSC_FEATURE_FASTMODE); break;
      #endif

      #if _SLZ
         slz_make_crc_table();
         slz_prepare_dist_table();
      #endif

      #if _FSEHUF
//    case P_FSEH: workmemsize = max(4096*sizeof(unsigned), workmemsize); break;
      #endif

      #ifdef _LZTURBO
    #include "../dev/x/beplug0.h"
      #endif
  }
  if(!workmemsize) return 0;
  if(workmemsize > sizeof(_workmem) && !(workmem = (char *)malloc(workmemsize)) ) {
    fprintf(stderr, "Codini: malloc error. workmemsize=%d\n", workmemsize);
    exit(0);
  }
  return 0;
}

int codstart(size_t insize, int codec, int lev, char *prm, int mode) {
  switch(codec) {
      #if _ZSTD
    case P_ZSTD:
      if(strchr(prm, 'D')) {
        if(mode) { if(!ddictPtr) { char *q; ddictPtr = NULL; if(q = strchr(prm,'D')) { q=q+(q[1]=='='?2:1);  if(!(ddictPtr = createDDict_orDie(q     ))) die("zstd:createDDict '%s' failed\n", q); } } }
        else     { if(!cdictPtr) { char *q; cdictPtr = NULL; if(q = strchr(prm,'D')) { q=q+(q[1]=='='?2:1);  if(!(cdictPtr = createCDict_orDie(q, lev))) die("zstd:createCDict '%s' failed\n", q); } } }
      }
      break;
      #endif
    default:;
  }
  return 0;
}

int codend(size_t insize, int codec, int lev, char *prm, int mode) {
  switch(codec) {
      #if _ZSTD
    case P_ZSTD:
        if(cdictPtr) { ZSTD_freeCDict(cdictPtr); cdictPtr = NULL; }
        if(ddictPtr) { ZSTD_freeDDict(ddictPtr); ddictPtr = NULL; }
      break;
      #endif
  }
  return 0;
}

void codexit(int codec) {
  if(workmem != _workmem) {
      #if _MARLIN
    if(codec == P_MARLIN) Marlin_free_dictionary((Marlin *)workmem);
    else
      #endif
    free(workmem/*, workmemsize*/);
    workmem = _workmem;
  }
    #if _SNAPPY_C
  if(codec == P_SNAPPY_C)
    snappy_free_env(&env);
    #endif
}

int brotlidic,brotlictx,brotlirep;
#define powof2(n) !((n)&((n)-1))
static ALWAYS_INLINE unsigned pow2next(unsigned x) { return x<2?1:(1ull << (__bsr32((x)-1)+1)); }

  #if _SMALLZ4
static unsigned char *gop,*gip,*giend;

static size_t getbytes(void *data, size_t n) { n = min(giend-gip,n); memcpy(data, gip, n); gip+=n; return n; }
static void sendbytes(const void *data, size_t n) { memcpy(gop, data, n); gop += n; }

static unsigned char getbyte() { return *gip++; }
  #endif

int codcomp(unsigned char *in, int inlen, unsigned char *out, int outsize, int codec, int lev, char *prm) { int outlen; unsigned char *oend=out+outsize; //printf("#(%d), inlen=%d,outsize=%d\n", codec, inlen, outsize);fflush(stdout);
  unsigned dsize = dicsize; char *q;
  if(q = strchr(prm,'d')) dsize = argtoi(q+(q[1]=='='?2:1),0);

  switch(codec) {
      #if _BALZ
    case P_BALZ: return balzcompress(in, inlen, out,lev);
      #endif

      #if _BCM
    case P_BCM: return bcmcompress(in, inlen, out); break;
      #endif

      #if _C_BLOSC2
    case P_C_BLOSC2:
        #if _C_BLOSC2LZ
      return blosclz_compress(lev, in, inlen, out, outsize);
        #else
      return blosc1_compress(lev, strchr(prm,'s')?1:0/*doshuffle*/, (q=strchr(prm,'t'))?atoi(q+(q[1]=='='?2:1)):1/*typesize*/, inlen, in, out, outsize/*inlen+BLOSC_MAX_OVERHEAD*/);
        #endif
      #endif

      #if _BRIEFLZ
    case P_BRIEFLZ: return blz_pack_level(in, out, inlen, workmem, lev);
      #endif

      #if _BROTLI
    case P_BROTLI: { unsigned lgwin, mode = BROTLI_DEFAULT_MODE; size_t esize = outsize;
      if(q = strchr(prm,'w'))              lgwin = atoi(q+(q[1]=='='?2:1));     // window specified by local parameter w
      else if(dsize)                       lgwin = bsr32(dsize)-powof2(dsize);  // window specified by global option -d
      else if(lev < 10 || strchr(prm,'W')) lgwin = BROTLI_DEFAULT_WINDOW;       // set default=24 for lev<10
      else                                 lgwin = BROTLI_LARGE_MAX_WINDOW_BITS;// set large window brotli
      if(q = strchr(prm,'m')) mode = atoi(q+(q[1]=='='?2:1));
                                                                            // Only for modified brotli by powturbo -------------------------------------
                                                                            brotlidic = brotlictx = brotlirep = 0;
                                                                            if(strchr(prm,'V'))      brotlidic = 1; // Disable builtin dictionary
                                                                            if(strchr(prm,'r'))      brotlirep = 1; // Disable extended reps -1,1,-2,2,-3,3
                                                                            else if(strchr(prm,'R')) brotlirep = 2; // disable all reps
                                                                            if(strchr(prm,'x'))      brotlictx = 1; // disable order-2 lit context
                                                                            if(strchr(prm,'X'))      brotlictx = 2; // disable all lit contexts
                                                                            //-----------------------------------------------------------------------
      int rc = BrotliEncoderCompress(lev, lgwin, (BrotliEncoderMode)mode, (size_t)inlen, (uint8_t*)in, &esize, (uint8_t*)out);
      return rc?esize:0;
    }
      #endif

      #if _LIBBSC
    #define BSC_MODE LIBBSC_FEATURE_FASTMODE|(strchr(prm,'P')?LIBBSC_FEATURE_LARGEPAGES:0)|(strchr(prm,'t')?0:LIBBSC_FEATURE_MULTITHREADING)
    case P_LIBBSC: return bsc_compress(      in, out, inlen,strchr(prm,'p')?0:15,strchr(prm,'p')?0:128, lev<3?1:lev, (q=strchr(prm,'e'))?atoi(q+(q[1]=='='?2:1)):1, BSC_MODE);
    case P_LIBBSCC:return bsc_coder_compress(in, out, inlen, lev, BSC_MODE);
    case P_LIBBSCBWT: { int bwtidx; memcpy(out+sizeof(bwtidx), in, inlen); bwtidx = bsc_bwt_encode(out+sizeof(bwtidx), inlen, 0, NULL, 0); *(unsigned *)out = bwtidx; return inlen+4; }
    case P_ST: { memcpy(out+4,in, inlen); *(unsigned *)(out) = bsc_st_encode(out+4, inlen, lev, 0); return inlen+4; }
      #endif
    int bsc_coder_compress(const unsigned char * input, unsigned char * output, int n, int coder, int features);

      #if _LIBDEFLATE
    case P_LIBDEFLATE:  {
       struct libdeflate_compressor *dc = libdeflate_alloc_compressor(lev);
            if(strchr(prm,'d')) outlen = libdeflate_deflate_compress(dc,in, inlen,out, outsize);
       else if(strchr(prm,'g')) outlen = libdeflate_gzip_compress(   dc,in, inlen,out, outsize);
       else                     outlen = libdeflate_zlib_compress(  dc,in, inlen,out, outsize);
       libdeflate_free_compressor(dc); return outlen;
      }
      #endif

      #if _BZIP2
    case P_BZIP2:    { unsigned outlen = outsize; return BZ2_bzBuffToBuffCompress((char *)out, &outlen, (char *)in, inlen, 9, 0, 0)==BZ_OK?outlen:-1; }
      #endif

      #if _BZIP3
    case P_BZIP3:    { 
	  #define BZIP3_SIZE 511*MB
	  struct bz3_state *st = bz3_new(BZIP3_SIZE);
	  unsigned char *ip,*op = out;
	  for(ip = in; ip < in+inlen;) { 
		unsigned iplen = (in+inlen) - ip; iplen = min(iplen, BZIP3_SIZE);
		op+=4; memcpy(op, ip, iplen);
		int rc = bz3_encode_block(st, op, iplen);
		if(rc == -1) die("bzip3 encode failed");
		ctou32(op-4) = rc; op += rc;
		ip += iplen;		
	  }
	  bz3_free(st);
	  return op - out;
	}
      #endif

      #if _CHAMELEON
    case P_CHAMELEON:  { Chameleon_Reset((Chameleon *)workmem); return lev<2?Chameleon_Encode((Chameleon *)workmem,out,in, inlen):Chameleon2_Encode((Chameleon *)workmem,out,in, inlen); }
      #endif

      #if _CSC
    case P_CSC: {
        CSCProps prop; CSCEncProps_Init(&prop, dsize?bsr32(dsize)-powof2(dsize):(1<<29), lev); CSCEnc_WriteProperties(&prop, (uint8_t*)out, 0);
        MemISeqInStream  si; si.s.Read  = (int(*)(const ISeqInStream *, void *, size_t *))cscread;  si.in  = in;                  si.inlen  = inlen;
        MemISeqOutStream so; so.s.Write = (size_t(*)(const ISeqOutStream *, const void *, size_t  ))cscwrite; so.out = out + CSC_PROP_SIZE; so.outlen = CSC_PROP_SIZE;
        CSCEncHandle eh = CSCEnc_Create(&prop, (ISeqOutStream*)&so, NULL); CSCEnc_Encode(eh, (ISeqInStream*)&si, NULL); CSCEnc_Encode_Flush(eh); CSCEnc_Destroy(eh);
        return so.outlen;
      }
      #endif

      #if _CRUSH
    case P_CRUSH: return crush_compress(lev, in, inlen, out);
      #endif

      #if _DENSITY
    case P_DENSITY: {
        density_processing_result rs = density_compress((const uint8_t *)in, inlen, (uint8_t*)out, outsize, (const DENSITY_ALGORITHM)lev/*, DENSITY_BLOCK_TYPE_DEFAULT, NULL, NULL*/);
        return rs.state?0:rs.bytesWritten;
      }
      #endif       

      #if _DOBOZ
    case P_DOBOZ:   { doboz::Compressor c; size_t rc; return c.compress(in,inlen, out, c.getMaxCompressedSize(inlen), rc) == doboz::RESULT_OK ? rc : -1; }
      #endif

      #if _FASTLZ
    case P_FASTLZ:  return fastlz_compress_level(lev, in, inlen, out);
      #endif

      #if _GIPFELI
    case P_GIPFELI: {
        util::compression::Compressor *c = util::compression::NewGipfeliCompressor();
        util::compression::UncheckedByteArraySink sink((char*) out);
        util::compression::ByteArraySource        src((const char*)in, inlen);
        outlen = c->CompressStream(&src, &sink); delete c; return outlen;
      }
      #endif

      #if _GLZA
    case P_GLZA:  { size_t outsize; return GLZAcomp(inlen, (uint8_t *)in, &outsize, (uint8_t *)out, (FILE *)0, 0)?outsize:0; }
      #endif

      #if _HEATSHRINK
    case P_HEATSHRINK:   return hscompress(in, inlen, out);
      #endif


      #if _IGZIP
    case P_IGZIP: struct isal_zstream s;
      isal_deflate_stateless_init(&s);
      static unsigned bs_default[] = { ISAL_DEF_LVL0_DEFAULT,     ISAL_DEF_LVL1_DEFAULT,     ISAL_DEF_LVL2_DEFAULT,     ISAL_DEF_LVL3_DEFAULT };
      static unsigned bs_xlarge[]  = { ISAL_DEF_LVL0_EXTRA_LARGE, ISAL_DEF_LVL1_EXTRA_LARGE, ISAL_DEF_LVL2_EXTRA_LARGE, ISAL_DEF_LVL3_EXTRA_LARGE };
      s.level_buf_size = strchr(prm,'x')?bs_xlarge[lev]:bs_default[lev];
      if(lev && !(s.level_buf = malloc(s.level_buf_size))) die("igzip:malloc error\n");
      s.next_in       = in;  s.avail_in  = inlen;
      s.next_out      = out; s.avail_out = outsize;
      s.level         = lev;
      s.gzip_flag     = IGZIP_ZLIB_NO_HDR;  // Decompression error with gzip_flag=IGZIP_ZLIB or gzip_flag=IGZIP_GZIP
      if(strchr(prm,'d')) s.gzip_flag = IGZIP_DEFLATE;
      if(strchr(prm,'g')) s.gzip_flag = IGZIP_GZIP_NO_HDR;
      isal_deflate_stateless(&s);
      if(s.level_buf) free(s.level_buf);
      return s.total_out;
      #endif

      #if _LIBLZF
    case P_LIBLZF:    return lzf_compress(in, inlen, out, outsize);
      #endif

      #if _LIBZPAQ
    case P_ZPAQ: { zin = in; zin_ = in+inlen; zout = out; char s[3]; s[0]=lev+'0'; s[1]=0; libzpaq::compress(&zmemin, &zmemout, s); return zout - out; }
      #endif

      #if _LZ4ULTRA
    case P_LZ4ULTRA:
      if(!strchr(prm,'Z')) {
        unsigned nFlags = 0;
        if(strchr(prm,'c')) nFlags |= LZ4ULTRA_FLAG_FAVOR_RATIO;
        if(strchr(prm,'r')) nFlags |= LZ4ULTRA_FLAG_RAW_BLOCK;
        if(strchr(prm,'i')) nFlags |= LZ4ULTRA_FLAG_INDEP_BLOCKS;
        if(strchr(prm,'l')) nFlags |= LZ4ULTRA_FLAG_LEGACY_FRAMES;
        if(lev>9) lev = 9; lev -= 2; lev = max(lev,7); //lz4 levels 9..12 -> lzultra 4..7
        return lz4ultra_compress_inmem(in, out, inlen, outsize, nFlags, lev);
      } // else fall through to compression with lz4
      #endif

      #if _SMALLZ4
    case P_SMALLZ4:
      if(!strchr(prm,'Z')) { gip = in; giend = in+inlen; gop = out; if(lev>9) lev=9; if(lev<4) lev=4; smallz4::lz4(getbytes,sendbytes,((1<<(7+lev))-1),false); return gop-out; }
      #endif

      #if _LZ4
    case P_LZ4:
      if(strchr(prm,'M')) { return !lev?LZ4_compress_fast((char *)in, (char *)out, inlen, outsize, 4):(lev<9?LZ4_compress_default((char *)in, (char *)out, inlen, outsize):LZ4_compress_HC((char *)in, (char *)out, inlen, outsize, lev)); }
      else { char *q;
         LZ4F_preferences_t opts = LZ4F_INIT_PREFERENCES;
                              opts.compressionLevel      = lev;
        if(strchr(prm,'s'))   opts.favorDecSpeed         = 1;
        if(strchr(prm,'f'))   opts.autoFlush             = 1;
                              opts.frameInfo.blockSizeID = LZ4F_max4MB;
        if(q=strchr(prm,'B')) opts.frameInfo.blockSizeID = (LZ4F_blockSizeID_t)atoi(q+(q[1]=='='?2:1)); if(opts.frameInfo.blockSizeID>LZ4F_max4MB) opts.frameInfo.blockSizeID=LZ4F_max4MB;else if(opts.frameInfo.blockSizeID && opts.frameInfo.blockSizeID<LZ4F_default) opts.frameInfo.blockSizeID=LZ4F_default;
        return LZ4F_compressFrame(out, outsize, in, inlen, &opts);
      }
      #endif

      #if _LIZARD
    case P_LIZARD: return Lizard_compress((const char*)in, (char*)out, inlen, outsize, lev);
      #endif

      #if _LZFSE
    case P_LZFSE : return lzfse_encode_buffer(out, outsize, in, inlen, workmem);
      #endif

      #if _LZFSEA
    case P_LZFSEA : return compression_encode_buffer(out, outsize, in, inlen, workmem, COMPRESSION_LZFSE);
      #endif

      #if _LZHAM
    case P_LZHAM: { static int dicbits[] = { 24, 24, 24, 26, 29, 29 }; lzham_compress_params p; memset(&p, 0, sizeof(p)); p.m_struct_size = sizeof(p);
                               p.m_dict_size_log2                    = out[0] = dsize?(bsr32(dsize)-powof2(dsize)):dicbits[lev]; //printf("[%u,%u] ", dsize, p.m_dict_size_log2);
                               p.m_level                             = (lzham_compress_level)lev;
                               p.m_compress_flags                   |= LZHAM_COMP_FLAG_FORCE_SINGLE_THREADED_PARSING;
                               p.m_compress_flags                   |= strchr(prm,'x')?LZHAM_COMP_FLAG_EXTREME_PARSING:0;
        if(q=strchr(prm,'t'))  p.m_max_helper_threads                = atoi(q+(q[1]=='='?2:1));
        if(q=strstr(prm,"fb")) p.m_fast_bytes                        = atoi(q+(q[2]=='='?3:2));
        if(q=strchr(prm,'x'))  { unsigned x = atoi(q+(q[1]=='='?2:1)); p.m_extreme_parsing_max_best_arrivals = x<4?4:x; }
                               p.m_table_update_rate                 = LZHAM_DEFAULT_TABLE_UPDATE_RATE;
        size_t outlen        = outsize;
        lzham_uint32 adler32 = 0;
        lzham_compress_status_t rc = lzham_compress_memory(&p, (lzham_uint8*)out+1, &outlen, (const lzham_uint8 *)in, inlen, &adler32);
        return rc == LZHAM_COMP_STATUS_SUCCESS?outlen:-rc;
    }
      #endif

      #if _FLZMA2
    case P_FLZMA2: { unsigned nbThreads = 1; char *q; if(q=strstr(prm,"mt")) nbThreads = atoi(q+(q[2]=='='?3:2)); return FL2_compressMt(out, outsize, in, inlen, lev, nbThreads); }
      #endif
      #if _LZMA
        #if __x86_64__
           #define DICSIZE (1<<30)
        #else
           #define DICSIZE (1<<27)
        #endif
    case P_LZMA: { CLzmaEncProps p; LzmaEncProps_Init(&p); p.level = lev; p.numThreads = 1; char *q;
      if(q=strstr(prm,"lc")) p.lc         = atoi(q+(q[2]=='='?3:2));
      if(q=strstr(prm,"lp")) p.lp         = atoi(q+(q[2]=='='?3:2));
      if(q=strstr(prm,"pb")) p.pb         = atoi(q+(q[2]=='='?3:2));
      if(q=strstr(prm,"fb")) p.fb         = atoi(q+(q[2]=='='?3:2));else if(q=strstr(prm,"nice=")) p.fb = atoi(q+5);
      if(q=strstr(prm,"mc")) p.mc         = atoi(q+(q[2]=='='?3:2));
      if(q=strstr(prm,"mt")) p.numThreads = atoi(q+(q[2]=='='?3:2));
      if(q=strchr(prm,'a'))  p.algo       = atoi(q+(q[1]=='='?2:1));
      if(q=strstr(prm,"mf=bt")) p.btMode  = 1, p.numHashBytes = atoi(q+5);
      if(q=strstr(prm,"mf=hc")) p.btMode  = 0, p.numHashBytes = atoi(q+5);
      if(dsize) p.dictSize = dsize; if(p.dictSize>inlen) p.dictSize=inlen; if(p.dictSize>DICSIZE) p.dictSize=DICSIZE; //printf("dsize=%u, %d,%d,%d:%d, %d,%d, %d,%d\n ", p.dictSize, p.lc,p.lp,p.pb,p.fb, p.mc,p.algo, p.btMode,p.numHashBytes);
      LzmaEncProps_Normalize(&p);
      SizeT psize = LZMA_PROPS_SIZE, outlen = outsize - LZMA_PROPS_SIZE;
      return LzmaEncode(out+LZMA_PROPS_SIZE, &outlen, in, inlen, &p, out, &psize, 0, NULL, &g_Alloc, &g_Alloc) == SZ_OK?outlen+LZMA_PROPS_SIZE:0;
    }
      #endif

      #if _LZLIB
    case P_LZLIB:  { long outlen=outsize; unsigned ds = dsize?dsize:option_mapping[lev].dictionary_size, mlen=option_mapping[lev].match_len_limit;
      if(q=strstr(prm,"fb")) mlen = atoi(q+(q[2]=='='?3:2)); if(ds > inlen) ds = inlen; //printf("(%u,%d,%u)", ds, mlen, inlen);
      bbcompress( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen,  ds, mlen); return outlen;
    }
      #endif

      #if _LIBLZG
    case P_LIBLZG: { lzg_encoder_config_t cfg; LZG_InitEncoderConfig(&cfg); cfg.fast = LZG_TRUE; cfg.level = lev; return LZG_Encode(in, inlen, out, outsize, &cfg); }
      #endif

      #if _SLZ
    case P_SLZ: { if(lev > 7) lev = 7;
      unsigned blk = 16384 << lev;
      struct slz_stream strm;
            if(strchr(prm,'d')) slz_init(&strm, !!lev, SLZ_FMT_DEFLATE);
       else if(strchr(prm,'g')) slz_init(&strm, !!lev, SLZ_FMT_GZIP);
       else                     slz_init(&strm, !!lev, SLZ_FMT_ZLIB);
      unsigned char *ip, *op=out;
      for(ip = in, in += inlen; ip < in; ) {
        unsigned iplen = in - ip; iplen = iplen>blk?blk:iplen;
        op += slz_encode(&strm, op, ip, iplen, in-ip > blk);
        ip += iplen;
      }
      op += slz_finish(&strm, op);
      return op - out;
    }
      #endif
      #if _LZMAT
    case P_LZMAT:   { MP_U32 outs=outsize; int rc = lzmat_encode(out, &outs, in, inlen); return rc == LZMAT_STATUS_OK ? outs : 0; }
      #endif

      #if _LZOMA
    case P_LZOMA:   return lzomapack(in, inlen, out, lev);
      #endif

      #if _LZO
    case P_LZO1b:   { lzo_uint out_len;
         lev<  9?  lzo1b_1_compress(in, inlen, out, &out_len, workmem):
        (lev< 99?  lzo1b_9_compress(in, inlen, out, &out_len, workmem):
        (lev<999? lzo1b_99_compress(in, inlen, out, &out_len, workmem):
        (        lzo1b_999_compress(in, inlen, out, &out_len, workmem))));
        return out_len;
      }
    case P_LZO1c:   { lzo_uint out_len;
         lev<  9?  lzo1c_1_compress(in, inlen, out, &out_len, workmem):
        (lev< 99?  lzo1c_9_compress(in, inlen, out, &out_len, workmem):
        (lev<999? lzo1c_99_compress(in, inlen, out, &out_len, workmem):
        (        lzo1c_999_compress(in, inlen, out, &out_len, workmem))));
        return out_len;
      }
    case P_LZO1f:   { lzo_uint out_len; lev<999?  lzo1f_1_compress(in, inlen, out, &out_len, workmem):lzo1f_999_compress(in, inlen, out, &out_len, workmem); return out_len; }
    case P_LZO1x: { lzo_uint out_len;
         lev< 11?  lzo1x_1_compress(in, inlen, out, &out_len, workmem):
        (lev< 12? lzo1x_1_11_compress(in, inlen, out, &out_len, workmem):
        (lev< 15? lzo1x_1_12_compress(in, inlen, out, &out_len, workmem):
        (lev<999? lzo1x_1_15_compress(in, inlen, out, &out_len, workmem):
        (        lzo1x_999_compress(in, inlen, out, &out_len, workmem)))));
        return out_len;
      }
    case P_LZO1y:   { lzo_uint out_len; lev<999?  lzo1y_1_compress(in, inlen, out, &out_len, workmem):lzo1y_999_compress(in, inlen, out, &out_len, workmem); return out_len; }
    case P_LZO1z:   { lzo_uint out_len; lzo1z_999_compress(in, inlen, out, &out_len, workmem); return out_len; }
    case P_LZO2a:   { lzo_uint out_len; lzo2a_999_compress(in, inlen, out, &out_len, workmem); return out_len; }
      #endif

      #if _LZSA
    case P_LZSA:  {
      unsigned nFlags = 0, nMinMatchSize=4, nFormatVersion=1; char *q;
      if(strchr(prm,'c')) nFlags |= LZSA_FLAG_FAVOR_RATIO;
      if(strchr(prm,'r')) nFlags |= LZSA_FLAG_RAW_BLOCK;
      if(q=strchr(prm,'f')) nFormatVersion = atoi(q+(q[1]=='='?2:1)); if(nFormatVersion < 1) nFormatVersion=1; if(nFormatVersion > 2) nFormatVersion=2;
      if(q=strchr(prm,'m')) nMinMatchSize = atoi(q+(q[1]=='='?2:1));
      return lzsa_compress_inmem(in, out, inlen, outsize, nFlags, nMinMatchSize, nFormatVersion);
    }
     #endif

      #if _LZSS
    case P_LZSS1:   { return lzss_encode(in, inlen, out, 0); }
    case P_LZSS2:   { return lzss_encode(in, inlen, out, 1); }
      #endif

      #if _LZSSE
    case P_LZSSE2: {            LZSSE2_OptimalParseState *s = LZSSE2_MakeOptimalParseState(inlen); if(!s) return 0; outlen = LZSSE2_CompressOptimalParse( s, in, inlen, out, outsize, lev ); LZSSE2_FreeOptimalParseState(s); return outlen; }
    case P_LZSSE4: if(lev==0) { LZSSE4_FastParseState    *s = LZSSE4_MakeFastParseState();                          outlen = LZSSE4_CompressFast(         s, in, inlen, out, outsize      ); LZSSE4_FreeFastParseState(s);    return outlen; }
                   else {       LZSSE4_OptimalParseState *s = LZSSE4_MakeOptimalParseState(inlen); if(!s) return 0; outlen = LZSSE4_CompressOptimalParse( s, in, inlen, out, outsize, lev ); LZSSE4_FreeOptimalParseState(s); return outlen; }
    case P_LZSSE8: if(lev==0) { LZSSE8_FastParseState    *s = LZSSE8_MakeFastParseState();                          outlen = LZSSE8_CompressFast(         s, in, inlen, out, outsize      ); LZSSE8_FreeFastParseState(s);    return outlen; }
                   else {       LZSSE8_OptimalParseState *s = LZSSE8_MakeOptimalParseState(inlen); if(!s) return 0; outlen = LZSSE8_CompressOptimalParse( s, in, inlen, out, outsize, lev ); LZSSE8_FreeOptimalParseState(s); return outlen; }

      #endif

      #if _MSCOMPRESS
    case P_MSCOMPRESS: { size_t outlen=outsize; return ms_compress((MSCompFormat)lev, in, inlen, out, &outlen)?0:outlen; }
      #endif

      #if _MINIZ
    case P_MINIZ:   { uLongf outlen = outsize; int rc = mz_compress2(out, &outlen, in, inlen, lev); if (rc != Z_OK) printf("miniz compress2 rc=%d\n", rc); return outlen; }
      #endif

      #if _NAKA
    case P_NAKA:    return NakaCompress( (char *)out, (char *)in, inlen);
       #endif

      #if _PITHY
    case P_PITHY: return pithy_Compress((const char *)in, inlen, (char *)out, outsize, lev);
      #endif

      #if _QUICKLZ
    case P_QUICKLZ: { memset(workmem,0,workmemsize); return lev<=1?qlz_compress1((char *)in, (char *)out, inlen, workmem):(lev<=2?qlz_compress2((char *)in, (char *)out, inlen, workmem):qlz_compress3((char *)in, (char *)out, inlen, workmem)); }
      #endif

      #if _OODLE
    case P_OODLE: {
      int nodll = strchr(prm,'c'), level = abs(lev), comp = level/10; level = (level>99?level-100:level)%10; if(lev<0) level = -level;    
      if(!nodll) {
	OodleLZ_CompressOptions copts = *OodleLZ_CompressOptions_GetDefault_(comp, level);
        return OodleLZ_Compress_(comp, in, inlen, out, level, &copts, 0, 0, 0, 0);
      } 
        #if _OODLESRC
      else {
	 OodleLZ_CompressOptions copts = *OodleLZ_CompressOptions_GetDefault(comp, level);
        return OodleLZ_Compress(comp, in, inlen, out, level, &copts, 0, 0, 0, 0);
      }
	#endif
    }
      #endif

      #if _PYSAP
    case P_PYSAP: { CsObjectInt c; SAP_INT bytes_read, bytes_written; int rc = c.CsInitCompr((SAP_BYTE *)out, inlen, lev); out += CS_HEAD_SIZE; outsize -= CS_HEAD_SIZE;
        c.CsCompr(inlen, in, inlen, out, outsize, lev, &bytes_read, &bytes_written); return bytes_written+CS_HEAD_SIZE;
      }
      #endif

      #if _SHOCO
    case P_SHOCO:     return shoco_compress((const char *)in, inlen, (char *)out, outsize);
      #endif

      #if _SMAZ
    case P_SMAZ:     return smaz_compress((const char *)in, inlen, (char *)out, outsize);
      #endif

      #if _SHRINKER
    case P_SHRINKER:     return shrinker_compress((char *)in, (char *)out, inlen);
      #endif

      #if _SNAPPY
        #if __cplusplus
    case P_SNAPPY:    { size_t outlen=outsize; snappy::RawCompress((char *)in, inlen, (char*)out, &outlen); return outlen;}
        #else
    case P_SNAPPY:    { size_t outlen=outsize; int rc = snappy_compress(in, inlen, out, &outlen); return outlen;}
        #endif
      #endif

      #if _SNAPPY_C
    case P_SNAPPY_C:   { size_t outlen=outsize; int rc = snappy_compress(&env, (const char *)in, inlen, (char *)out, &outlen); return outlen;}
      #endif

      #if _TORNADO
    case P_TORNADO:   return torcompress(lev, in, out, inlen);
      #endif

      #if _UNISHOX2
    case P_UNISHOX2:  return unishox2_compressx(in, inlen, out, lev);
      #endif
      #if _UNISHOX3
    case P_UNISHOX3:  { unishox3 usx3; return usx3.compress(in, inlen, USX3_API_OUT_AND_LEN(out,outsize) ); }
      #endif

      #if _WFLZ
    case P_WFLZ:    return lev<=1?wfLZ_CompressFast( (const uint8_t* WF_RESTRICT const)in, inlen, (uint8_t* WF_RESTRICT const)out, (const uint8_t* WF_RESTRICT)workmem, 0 ):
                                      wfLZ_Compress( (const uint8_t* WF_RESTRICT const)in, inlen, (uint8_t* WF_RESTRICT const)out, (const uint8_t* WF_RESTRICT)workmem, 0 );
      #endif

      #if _WIMLIB
    case P_WIMLIB: { struct wimlib_compressor *compressor; if(wimlib_create_compressor(lev, 32*1024/*inlen*/, 0, &compressor)) return 0;
        outlen = wimlib_compress(in, inlen, out, outsize /*inlen- 1*/, compressor);  wimlib_free_compressor(compressor); return outlen;
      }
      #endif

      #if _WKDM
    case P_WKDM:    return WKdm_compress ((WK_word*)in, (WK_word*)out, inlen);
      #endif

      #if _XPACK
    case P_XPACK:  { struct xpack_compressor *dc = xpack_alloc_compressor(inlen,lev);
       outlen = xpack_compress(dc,in, inlen,out, outsize);
       xpack_free_compressor(dc); return outlen;
      }
      #endif

      #if _YALZ77
    case P_YALZ77: { lz77::compress_t c(lev, lz77::DEFAULT_BLOCKSIZE); std::string os = c.feed(in,in+inlen); memcpy(out, os.c_str(), os.size()); return os.size(); }
      #endif

      #if _YAPPY
    case P_YAPPY:    return YappyCompress(in, out, inlen, 10)-out;
      #endif

      #if _ZLIB
    case P_ZLIB:     { uLongf outlen = outsize; int rc = compress2(out, &outlen, in, inlen, lev); if(rc != Z_OK) printf("zlib compress2 rc=%d\n", rc);  return outlen; }
      #endif

      #if _ZLIB_NG
    case P_ZLIB_NG:   { size_t outlen = outsize; int rc = zng_compress2(out, &outlen, in, inlen, lev); if(rc) printf("zlib compress2 rc=%d\n", rc);  return outlen; }
      #endif

      #if _ZLING
    case P_ZLING:    return zling_compress(lev, in, inlen, out, outsize);
      #endif

      #if _ZOPFLI
    case P_ZOPFLI:  {
        ZopfliOptions opts; ZopfliInitOptions(&opts); size_t outlen = 0; unsigned char *pout = out;
        ZopfliCompress(&opts, ZOPFLI_FORMAT_ZLIB, in, inlen, &pout, &outlen);
        if(pout != out)
          memcpy(out, pout, outlen);
        return outlen;
     }
      #endif

      #if _ZSTD
    case P_ZSTD: { 
	  ZSTD_CStream *z = ZSTD_createCStream(); if(!z) return -1;
      if(dsize) {
	    int windowLog = bsr32(dsize)-powof2(dsize); 
		ZSTD_CCtx_setParameter(z, ZSTD_c_enableLongDistanceMatching, 1); 
	    ZSTD_CCtx_setParameter(z, ZSTD_c_windowLog, windowLog);
      }
	  ZSTD_initCStream(z, lev);
	  ZSTD_inBuffer  ip = { in, (size_t)inlen,   0 };
	  ZSTD_outBuffer op = { out,(size_t)outsize, 0 };
	  ZSTD_compressStream(z, &op, &ip);
	  ZSTD_endStream(z, &op);
	  ZSTD_freeCStream(z);
      return op.pos;
    }
      #endif
    //------------------------- Encoding
     #if _TURBORLE
          #define _ESC8  0x5 //0xda
          #define _ESC16 0xdad5
          #define _ESC32 0xdad5abad
          #define _ESC64 0xdad5abada9d6d3aeull
    case P_RLES:
      switch(lev) {
        case  0: return srlec(  in, inlen, out);
        case  8: return srlec8( in, inlen, out, _ESC8);
        case 16: return srlec16(in, inlen, out, _ESC16);
        case 32: return srlec32(in, inlen, out, _ESC32);
        case 64: return srlec64(in, inlen, out, _ESC64);
      } break;
    case P_RLET:  return trlec(in, inlen, out);
      #endif
      #if _MRLE
    case P_RLEM:  return mrlec(in, inlen, out);
      #endif

      #if _RLE8
    case P_RLE8:
      switch(lev) {
        case 1 : { int subSections = 0; if(q=strchr(prm,'S')) subSections = atoi(q+(q[1]=='='?2:1));
          return subSections?rle8m_compress(subSections, in, inlen, out, outsize):(strchr(prm,'s')?rle8_compress_only_max_frequency(in, inlen, out, outsize):rle8_compress(in, inlen, out, outsize));
        }
        case 2 : return strchr(prm,'s')?rle8_ultra_compress_only_max_frequency(in, inlen, out, outsize):rle8_ultra_compress(in, inlen, out, outsize);

        case  8 : return strchr(prm,'s')?rle8_extreme_single_compress(in, inlen, out, outlen):rle8_extreme_multi_compress( in, inlen, out, outsize);
        case 16 : return rle16_extreme_compress(in, inlen, out, outsize);
        case 24 : return rle24_extreme_compress(in, inlen, out, outsize);
        case 32 : return rle32_extreme_compress(in, inlen, out, outsize);
        case 48 : return rle48_extreme_compress(in, inlen, out, outsize);
        case 64 : return rle64_extreme_compress(in, inlen, out, outsize);
      }
      #endif

      #if _B64
    case P_B64_PLAIN:  { size_t outlen=0; base64_encode(in, inlen, out, &outlen, BASE64_FORCE_PLAIN); return outlen; }
    case P_B64_SSSE3:  { size_t outlen=0; base64_encode(in, inlen, out, &outlen, BASE64_FORCE_SSSE3); return outlen; }
    case P_B64_SSE41:  { size_t outlen=0; base64_encode(in, inlen, out, &outlen, BASE64_FORCE_SSE41); return outlen; }
    case P_B64_AVX:    { size_t outlen=0; base64_encode(in, inlen, out, &outlen, BASE64_FORCE_AVX); return outlen; }
    case P_B64_AVX2:   { size_t outlen=0; base64_encode(in, inlen, out, &outlen, BASE64_FORCE_AVX2);  return outlen; }
        #if __ARM_NEON
    case P_B64_NEON64: { size_t outlen=0; base64_encode(in, inlen, out, &outlen, BASE64_FORCE_NEON64);      return outlen; }
        #endif
      #endif

      #if _FB64
        #ifdef AVX2_ON
    case P_FB64AVX2:     return fast_avx2_base64_encode(  out,   in,inlen);
    case P_FB64KLOMP:    { size_t outlen = outsize; klomp_avx2_base64_encode((const char*)in, inlen, out, &outlen); return outlen; }
        #endif
        #if __ARM_NEON
    case P_FB64NEON:
        #endif
    case P_FB64CHROM:  return chromium_base64_encode( (char*)out, (const char*)in, inlen);
    case P_FB64LINUX:     return linux_base64_encode(    (char*)out, (const char*)in, (const char*)(in+inlen));
    case P_FB64PLAIN:   { size_t outlen = outsize; scalar_base64_encode(   (const char*)in,inlen,(char*)out,&outlen);return outlen; }
      #endif

      #if _SB64
    case P_SB64SSE:  base64::sse::encode(base64::sse::lookup_naive, in, inlen, out); return ((inlen+2)/3)*4;
      #endif

      #if _TB64
    case P_TB64:    { int u; if(q=strchr(prm,'q')) cpuini(u=atoi(q+(q[1]=='='?2:1))); return tb64enc(in, inlen, out); }
    case P_TB64S:   return tb64senc(  in, inlen, out);
    case P_TB64X:   return tb64xenc(  in, inlen, out);
        #if __ARM_NEON
    case P_TB64NEON:
        #else
    case P_TB64AVX: return tb64avxenc(in, inlen, out);
          #ifdef AVX2_ON
    case P_TB64AVX2: return tb64avx2enc(in, inlen, out);
          #endif
        #endif
    case P_TB64SSE: return tb64sseenc(in, inlen, out);
     #endif

    //------------------------- Transform -----------------------------
      #if _DIVBWT
    case P_DIVBWT: { int *sa = (int *)malloc((inlen + 1) * sizeof(int)); if(!sa) return -1;
      unsigned bwtidx = divbwt(in, out+sizeof(bwtidx), sa, inlen, NULL, NULL, 0); free(sa); *(unsigned *)out = bwtidx; return inlen+4; }
      #endif
      #if _BRC
    case P_BRC:
      if(lev) { struct brc_cxt_s brc_cxt; brc_init_cxt(&brc_cxt, inlen); int rc = brc_encode(&brc_cxt, in, inlen); memcpy(out, brc_cxt.block, brc_cxt.size); outlen = brc_cxt.size; brc_free_cxt(&brc_cxt); return rc?0:outlen; }
      else return vsrc_forwards(in, out, inlen);
      #endif
    //------------------------- Entropy Coders -------------------------
      #if _MEMCPY
    case P_MCPY:     memcpy(out, in, inlen); return inlen;
    case P_LMCPY: libmemcpy(out, in, inlen); return inlen;
      #endif

      #if _AOM
    case P_AOM:     return aomenc(in, inlen, out, outsize);
      #endif

      #if _DAALA
    case P_DAALA:   return daalaenc(in, inlen, out, outsize);
      #endif

      #if _BCMEC
    case P_BCMEC:  return bcmenc(in, inlen, out);
      #endif

    #define SYMNUM 0x100
      #if _FASTAC
    case P_FASTAC: {
        Adaptive_Data_Model adaptive_model(SYMNUM);
        adaptive_model.set_alphabet(SYMNUM);
        adaptive_model.reset();
        Arithmetic_Codec encoder(outsize, out);
        encoder.start_encoder();
        for(int i = 0; i < inlen; i++)
          encoder.encode(unsigned(in[i]), adaptive_model);
        return encoder.stop_encoder();
      }
      #endif

      #if _FASTARI
    case P_FASTARI:  { size_t outlen = outsize; return fa_compress((const unsigned char *)in, out, inlen, &outlen, workmem)?0:outlen; }
      #endif

      #if _FASTHF
    case P_FASTHF: {
        Adaptive_Huffman_Code adaptive_model(SYMNUM);
        adaptive_model.set_alphabet(SYMNUM);
        adaptive_model.reset();
        Binary_Codec encoder(outsize, out);
        encoder.start_encoder();
        for(int i = 0; i < inlen; i++)
          encoder.encode(unsigned(in[i]), adaptive_model);
        return encoder.stop_encoder();
      }
      #endif

      #if _FPAQC
    case P_FPAQC:   return absc(in, inlen, out, outsize);
      #endif

      #if _FPC
    case P_FPC:  return FPC_compress(out,in,inlen,lev*1024);
      #endif

      #if _FQZ
    case P_FQZ0:  { unsigned int outlen; compress_block(in, inlen, out, &outlen); return outlen; }
      #endif

      #if _FREQTAB
    case P_FREQTAB: return freqtabenc(in, inlen, out, outsize);
      #endif

      #if _FSC
    case P_FSC:     { size_t outlen = 0; uint8_t *op = NULL; int ok = FSCEncode(in, inlen, &op, &outlen, 12, CODING_METHOD_DEFAULT); if(ok) { memcpy(out, op, outlen); free(op); } return outlen; }
      #endif

      #if _FSE
    case P_FSE:     { size_t o = FSE_compress(out, outsize, in, inlen); if(o == 1) { out[0] = in[0]; return 1; } if(!o || o >= inlen) { memcpy(out, in, inlen); o = inlen; } return o; }
	  #endif
	  
      #if _FSEHUF
    case P_FSEH:    { size_t o = HUF_compress(out, outsize, in, inlen); if(o == 1) { out[0] = in[0]; return 1; } if(!o || o >= inlen) { memcpy(out, in, inlen); o = inlen; } return o;    }
      #endif

      #if _MARLIN
    case P_MARLIN:  {
      double hist[256]={}; Marlin *dict;
      if(!workmem == _workmem) {
        unsigned char *p; for(p=in; p != in+inlen; p++) hist[*p]++;
        dict = Marlin_build_dictionary("marlin", hist);
        workmem = (char *)dict;
      }
      dict = (Marlin *)workmem;
      return Marlin_compress(dict, (uint8_t*)out, outsize, (const uint8_t*)in, inlen);
    }
      #endif

      #if _NANS
    case P_NANS:   return nansenc(in, inlen, out, oend);
      #endif

      #if _NIBRANS
    case P_NIBRANS:  { struct nibrans nbra; nibransInit(&nbra); return nibransEncode(&nbra, out, outsize, in, inlen); }
      #endif

      #if _JAC
    case P_JAC:  { unsigned outlen; arith_compress_O0(in, inlen, &outlen, out); return outlen; }
      #endif

      #if _HTSCODECS
    //case P_RANS_S8:     { unsigned outlen = outsize; return rans_compress_to(   8,in, inlen, out, &outlen, lev) ? outlen : 0;}
    //case P_RANS_S16:    { unsigned outlen = outsize; if(strchr(prm,'r')) lev |= X_RLE; if(strchr(prm,'p')) lev |= X_PACK;
    //                                                 return rans_compress_to(  16,in, inlen, out, &outlen, lev) ? outlen : 0;
    //}
        #ifdef __AVX2__
    case P_RANS_S32:   { unsigned outlen = outsize; return rans_compress_O0_32x16_avx2(in, inlen, out, &outlen) ? outlen : 0;}
        #endif
      #endif

      #if _PPMDEC
    case P_PPMDEC:  return ppmdenc(in, inlen, out);
      #endif

      #if _POLHF
    case P_POLHF:   return polarenc(in, inlen, out);
      #endif

      #if _RECIPARITH
    case P_RECIPARITH:     return raenc(in, inlen, out, outsize);
      #endif

      #if _SHRC
    case P_SHRC:    return rcshc(in, inlen, out);
    case P_SHRCV:   return vecrcenc(in, inlen, out);
      #endif

      #if _SUBOTIN
    case P_SUBOTIN:     return subenc(in, inlen, out, inlen, SYMNUM);
      #endif

      #if _TORNADOHF
    case P_TORNADOHF:     return torhenc(in, inlen, out, outsize);
      #endif

      #if _TURBORC
    case P_TURBORC: { //int ec = 0; 
      //unsigned prm1 = 5,prm2 = 6; //if(q=strchr(prm,'r')) { prm1 = atoi(q+(q[1]=='='?2:1)); prm2 = prm1%10; prm1 = prm1/10; if(prm1>9)prm1=9;if(!prm1) prm1=1; if(prm2>9)prm2=9;if(!prm2) prm2=1; }
	  char *q;
	  unsigned bwtlev = 9, xprep8=0, forcelzp=0, verbose=0, xsort=0, itmax=0, lenmin=1;
	  if(q = strchr(prm,'e')) bwtlev = atoi(q+(q[1]=='='?2:1));  
	  if(q = strchr(prm,'m')) lenmin = atoi(q+(q[1]=='='?2:1));  
      #define bwtflag(z) (z==2?BWT_BWT16:0) | (xprep8?BWT_PREP8:0) | forcelzp | (verbose?BWT_VERBOSE:0) | xsort <<14 | itmax <<10 | lenmin
      switch(lev) {
        case  1: return rcsenc(    in, inlen, out);
        case  2: return rccsenc(   in, inlen, out); 
        case  3: return rcc2senc(  in, inlen, out);
        case  4: return rcxsenc(   in, inlen, out);
        //case  5: mbcset(15); clen = rcx2enc(  in, inlen, out, prdid); break;
        //case  9: return rcmsenc(  in, inlen, out);       
        //case 10: return rcm2senc( in, inlen, out);        
        case 12: return rcrlesenc( in, inlen, out);
        case 14: return rcrle1senc(in, inlen, out);        //case 17: return rcqlfcsenc(in, inlen, out);
        //case 20: return rcbwtenc( in, inlen, out, bwtlev, 0, bwtflag(1));
	//case 21: return utf8enc( in, inlen, out, bwtflag(1)|BWT_COPY|BWT_RATIO);
	//case 90: return lzpenc( in, inlen, out, 1, 0);
      }
    }
      #endif

      #if _ZLIB
    case P_ZLIBH: { z_stream z; unsigned char *in_ = in+inlen, *ip, *op = out; unsigned iplen; //return ZLIBH_compress((char* )out, (const char*)in, inlen);
      for(ip = in; ip != in_; ip += iplen) { iplen = min(in_-ip, lev*1024);
        memset(&z, 0, sizeof(z));
        if(deflateInit2(&z, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_HUFFMAN_ONLY) == Z_OK) {
          z.next_in   = ip;
          z.avail_in  = iplen;
          z.next_out  = op+2;
          z.avail_out = outsize;
          if(deflate(&z, Z_FINISH) != Z_STREAM_END) break;
          unsigned oplen = outsize - z.avail_out; ctou16(op) = oplen; op += 2+oplen;
          deflateEnd(&z);
        }
      } return op-out;
    }
      #endif

      #if _ZLIB
    case P_ZRLE: { z_stream z; memset(&z, 0, sizeof(z));
      if(deflateInit2(&z, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_RLE) == Z_OK) {
        z.next_in   = in;
        z.avail_in  = inlen;
        z.next_out  = out;
        z.avail_out = outsize;
        if(deflate(&z, Z_FINISH) != Z_STREAM_END) break;
        deflateEnd(&z);
      }
      return outsize - z.avail_out;
    }
      #endif

      #ifdef _LZTURBO
    #include "../dev/x/beplugc.c"
      #endif

      #if _MYCODEC
//    case P_MYCODEC:   return mycomp(in, inlen, out, outsize);
      #endif

    defaulf: fprintf(stderr, "library '%d' not included\n", codec);
  }
} 

int coddecomp(unsigned char *in, int inlen, unsigned char *out, int outlen, int codec, int lev, char *prm) { 
  switch(codec) {
      #if _AOM
    case P_AOM:     aomdec(in, inlen, out, outlen); return outlen;
      #endif

      #if _BALZ
    case P_BALZ: return balzdecompress(in, inlen, out, outlen);
      #endif

      #if _BCM
    case P_BCM: return bcmdecompress(in, inlen, out, outlen);
      #endif

      #if _C_BLOSC2
    case P_C_BLOSC2: return blosc1_decompress(in, out, outlen);
      #endif

      #if _C_BLOSC2LZ
    case P_C_BLOSC2LZ: return blosclz_decompress(in, inlen, out, outlen);
      #endif

      #if _BRIEFLZ
    case P_BRIEFLZ:     return blz_depack(in, out, outlen);
      #endif

      #if _BROTLI
    case P_BROTLI: { BrotliDecoderState* s = BrotliDecoderCreateInstance(NULL, NULL, NULL); if(!s) return -1;
    BrotliDecoderSetParameter(s, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1u);
    size_t total_out, available_in=inlen, available_out=outlen; uint8_t *next_in=in, *next_out=out;
    BrotliDecoderResult rc = BrotliDecoderDecompressStream(s, &available_in, (const uint8_t **)&next_in, &available_out, (uint8_t **)&next_out, &total_out);
        BrotliDecoderDestroyInstance(s);
        return rc?total_out:0;
      }
      #endif

      #if _LIBBSC
    case P_LIBBSC:     return bsc_decompress(in, inlen, out, outlen, BSC_MODE);
    case P_LIBBSCC:    return bsc_coder_decompress(in, out, lev, BSC_MODE);
      #endif
 
      #if _LIBDEFLATE
    case P_LIBDEFLATE:  { size_t rc; struct libdeflate_decompressor *dd = libdeflate_alloc_decompressor();
            if(prm && *prm=='d') outlen = libdeflate_deflate_decompress(dd, in, inlen,out, outlen, &rc);
       else if(prm && *prm=='g') outlen = libdeflate_gzip_decompress(   dd, in, inlen,out, outlen, &rc);
       else                      outlen = libdeflate_zlib_decompress(   dd, in, inlen,out, outlen, &rc);
       libdeflate_free_decompressor(dd);
       return outlen;
      }
      #endif

      #if _BZIP2
    case P_BZIP2: { unsigned outsize = outlen; return BZ2_bzBuffToBuffDecompress((char *)out, &outsize, (char *)in, inlen, 0, 0)==BZ_OK?outlen:-1; }
      #endif

      #if _BZIP3
    case P_BZIP3: { //size_t outsize = outlen; return bz3_decompress(in, out, inlen, &outsize)==BZ3_OK?outlen:-1; 
	    struct bz3_state *st = bz3_new(BZIP3_SIZE);
		unsigned char *ip = in, *op;
		for(op = out; op < out+outlen;) { 
		  unsigned iplen = ctou32(ip), oplen = (out+outlen) - op; oplen = min(oplen, BZIP3_SIZE);
		  memcpy(op, ip+4, iplen);
		  if(bz3_decode_block(st, op, iplen, oplen) == -1) die("bzip3 decode failed");
		  op += oplen;	
          ip += 4+iplen;		  
		}
		bz3_free(st);
		return op-out;
	  }
      #endif

      #if _CHAMELEON
    case P_CHAMELEON:  { Chameleon_Reset((Chameleon *)workmem); lev<2?Chameleon_Decode((Chameleon *)workmem, out, outlen, in):Chameleon2_Decode((Chameleon *)workmem, out, outlen, in); return inlen; }
      #endif

      #if _CSC
    case P_CSC: {
        CSCProps prop; CSCDec_ReadProperties(&prop, (uint8_t*)in);
        MemISeqInStream  si; si.s.Read  = (int(*)(const ISeqInStream *, void *, size_t *))cscread;  si.in  = in + CSC_PROP_SIZE; si.inlen  = inlen - CSC_PROP_SIZE;
        MemISeqOutStream so; so.s.Write = (size_t(*)(const ISeqOutStream *, const void *, size_t  ))cscwrite; so.out = out;                so.outlen = 0;
        CSCDecHandle dh = CSCDec_Create(&prop, (ISeqInStream*)&si, NULL); CSCDec_Decode(dh, (ISeqOutStream*)&so, NULL); CSCDec_Destroy(dh);
        return si.inlen;
      }
      #endif

     #if _CRUSH
    case P_CRUSH: crush_decompress(in, out, outlen); break;
      #endif

      #if _DAALA
    case P_DAALA:   daaladec(in, inlen, out, outlen); return outlen;
      #endif

      #if _DENSITY
    case P_DENSITY: { density_processing_result rs = density_decompress((uint8_t *)in, inlen, (uint8_t*)out, outlen/*+DENSITY_MINIMUM_OUTPUT_BUFFER_SIZE*/);  return rs.state?0:rs.bytesWritten; }
      #endif

      #if _FASTARI
    case P_FASTARI:  { size_t outsize = outlen; return fa_decompress((const unsigned char *)in, out, inlen, &outsize, workmem)?0:inlen; }
      #endif

      #if _DOBOZ
    case P_DOBOZ:   { doboz::Decompressor d; return d.decompress(in, inlen, out, outlen) == doboz::RESULT_OK ? outlen : -1; }
      #endif

      #if _FASTLZ
    case P_FASTLZ: fastlz_decompress(in, inlen, out, outlen); break;
      #endif

      #if _GIPFELI
    case P_GIPFELI: {
        util::compression::Compressor *c = util::compression::NewGipfeliCompressor();
        util::compression::UncheckedByteArraySink sink((char*) out);
        util::compression::ByteArraySource         src((const char*)in, inlen);
        outlen = c->UncompressStream(&src, &sink); delete c; return outlen;
      }
      break;
      #endif

      #if _GLZA
    case P_GLZA: { size_t outsize; GLZAdecode(inlen, (uint8_t *)in, &outsize, (uint8_t *)out, (FILE *)0, 0); break; }
      #endif

      #if _HEATSHRINK
    case P_HEATSHRINK: return hsdecompress(in, inlen, out, outlen);
      #endif

      #if _IGZIP
    case P_IGZIP: { struct inflate_state s; int rc; isal_inflate_init(&s);
         /*    if(prm && *prm == 'd') { s.crc_flag = ISAL_DEFLATE; }
        else if(prm && *prm == 'g') { s.crc_flag = ISAL_GZIP; }
        else s.crc_flag = ISAL_ZLIB;*/
        s.next_in  = in;  s.avail_in  = inlen;
        s.next_out = out; s.avail_out = outlen;
        if((rc = isal_inflate_stateless(&s)) != ISAL_DECOMP_OK) die("igzip error. rc=%d\n", rc);
      } break;
      #endif

      #if _LIBLZF
    case P_LIBLZF: lzf_decompress(in, inlen, out, outlen); break;
      #endif

      #if _SMALLZ4
    case P_SMALLZ4:
      if(!strchr(prm,'z')) { gip = in; giend = in+inlen; gop = out; unlz4(getbyte,sendbytes,NULL); break; }
      #endif

      #if _LZ4ULTRA
    case P_LZ4ULTRA:                                                                //if(strchr(prm,'z')) LZ4_decompress_safe((const char *)in, (char *)out, inlen, outlen);
      if(!strchr(prm,'z')) {
        unsigned nFlags = 0;
        if(strchr(prm,'c')) nFlags |= LZ4ULTRA_FLAG_FAVOR_RATIO;
        if(strchr(prm,'r')) nFlags |= LZ4ULTRA_FLAG_RAW_BLOCK;
        if(strchr(prm,'i')) nFlags |= LZ4ULTRA_FLAG_INDEP_BLOCKS;
        if(strchr(prm,'l')) nFlags |= LZ4ULTRA_FLAG_LEGACY_FRAMES;
        lz4ultra_decompress_inmem(in, out, inlen, outlen, nFlags);//lz4ultra_expand_block(in, inlen, out, 0, outlen);
        break;
      } // else fall throught to decompression with lz4
      #endif

      #if _LZ4
    case P_LZ4:
      if(strchr(prm,'M')) { LZ4_decompress_safe((const char *)in, (char *)out, inlen, outlen); break; }
      else {
        lz4: LZ4F_dctx *ctx; LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION);
        size_t ilen = inlen, olen = outlen, rc = LZ4F_decompress(ctx, out, &olen, in, &ilen, NULL);
        LZ4F_freeDecompressionContext(ctx);
        return rc;
      }
      #endif

      #if _LIZARD
    case P_LIZARD: return Lizard_decompress_safe((const char *)in, (char *)out, inlen, outlen);
      #endif

      #if _LZFSE
    case P_LZFSE : return lzfse_decode_buffer(out, outlen, in, inlen, workmem);
      #endif

      #if _LZFSEA
    case P_LZFSEA : return compression_decode_buffer(out, outlen, in, inlen, workmem, COMPRESSION_LZFSE);
      #endif

      #if _LIBLZG
    case P_LIBLZG: LZG_Decode(in, inlen, out, outlen); break;
      #endif

      #if _LIBZPAQ
    case P_ZPAQ: { zin = in; zin_ = in+inlen; zout = out; libzpaq::decompress(&zmemin, &zmemout); return zin - in; }
      #endif

      #if _LZHAM
    case P_LZHAM: { lzham_decompress_params p; memset(&p, 0, sizeof(p)); p.m_struct_size = sizeof(p);
      p.m_dict_size_log2    = in[0];
      size_t outl           = outlen;
      lzham_uint32 adler32  = 0;
      return lzham_decompress_memory(&p, (lzham_uint8*)out, &outl, in+1, inlen, &adler32) == LZHAM_COMP_STATUS_SUCCESS?outlen:-1;
    }
      #endif

      #if _LZLIB
    case P_LZLIB: { long out_len = outlen; bbdecompress( in, outlen, out, &out_len ); } break;
      #endif

      #if _FLZMA2
    case P_FLZMA2: { return FL2_decompress(out, outlen,  in, inlen); }
      #endif

      #if _LZMA
    case P_LZMA: {
        SizeT ol = outlen, il = inlen - LZMA_PROPS_SIZE; ELzmaStatus sts;
        return LzmaDecode(out, &ol, in+LZMA_PROPS_SIZE, &il, in, LZMA_PROPS_SIZE, LZMA_FINISH_END, &sts, &g_Alloc)?0:inlen;
      }
      #endif

      #if _LZMAT
    case P_LZMAT:  { MP_U32 rc = outlen; lzmat_decode(out, &rc, in, inlen); return rc; }
      #endif

      #if _LZO
    case P_LZO1x: { lzo_uint out_len;   lzo1x_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1b: { lzo_uint out_len;   lzo1b_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1c: { lzo_uint out_len;   lzo1c_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1f: { lzo_uint out_len;   lzo1f_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1y: { lzo_uint out_len;   lzo1y_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1z: { lzo_uint out_len;   lzo1z_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO2a: { lzo_uint out_len;   lzo2a_decompress(in, inlen, out, &out_len, NULL); break; }
      #endif

      #if _LZOMA
    case P_LZOMA:   lzomaunpack(in, inlen, out, outlen);break;
      #endif

      #if _MINIZ
    case P_MINIZ: { uLongf outsize = outlen; int rc = mz_uncompress(out, &outsize, in, inlen); } break;
      #endif

      #if _NAKA
    case P_NAKA:    return NakaDecompress((char *)out, (char *)in, inlen);
      #endif
      #if _PITHY
    case P_PITHY: return pithy_Decompress((const char *)in, inlen, (char *)out, outlen);
      #endif

      #if _QUICKLZ
    case P_QUICKLZ: { lev= (in[0]>>2)&3; outlen = lev<=1?qlz_decompress1((char*)in, out, workmem):(lev<=2?qlz_decompress2((char*)in, out, workmem):qlz_decompress3((char*)in, out, workmem)); } break;
      #endif

      #if _PYSAP
    case P_PYSAP: { CsObjectInt d; SAP_INT bytes_read, bytes_written;
        d.CsInitDecompr((SAP_BYTE *)in); in += CS_HEAD_SIZE; d.CsDecompr( (SAP_BYTE *)in, inlen-CS_HEAD_SIZE, (SAP_BYTE *)out, outlen, lev, &bytes_read, &bytes_written );
        return inlen-CS_HEAD_SIZE;
      }
      #endif

      #if _ZLING
    case P_ZLING: zling_decompress(in, inlen, out, outlen); break;
      #endif

      #if _LZSA
    case P_LZSA:  {
      unsigned nFlags = 0; int nFormatVersion=1;
      if(strchr(prm,'c')) nFlags |= LZSA_FLAG_FAVOR_RATIO;
      if(strchr(prm,'r')) nFlags |= LZSA_FLAG_RAW_BLOCK;
      if(strchr(prm,'f')) nFormatVersion = 2;
      lzsa_decompress_inmem(in, out, inlen, outlen, nFlags, &nFormatVersion);
      break;
    }
      #endif

      #if _LZSS
    case P_LZSS: lzss_decode(in, out, outlen); break;
      #endif

      #if _LZSSE
        #if __x86_64__
    case P_LZSSE2: return LZSSE2_Decompress(in,inlen,out,outlen);
    case P_LZSSE4: return LZSSE4_Decompress(in,inlen,out,outlen);
    case P_LZSSE8: return LZSSE8_Decompress(in,inlen,out,outlen);
        #endif
      #endif

      #if _MSCOMPRESS
     case P_MSCOMPRESS: { size_t _outlen=outlen; return ms_decompress((MSCompFormat)lev, in, inlen, out, &_outlen)==MSCOMP_OK?inlen:0; }
      #endif

      #if _OODLE
    case P_OODLE: { 
      int nodll = strchr(prm,'d');  
      if(!nodll) {
        int rc = OodleLZ_Decompress_(in, inlen, out, outlen, 0,0,0,0,0,0,0,0,0,0);
        return outlen;
      } 
	#if _OODLESRC
      else {
        int rc = OodleLZ_Decompress(in, inlen, out, outlen, 0,0,0,0,0,0,0,0,0,0);
        return outlen;
      }
	#endif
    } 
      #endif

      #if _SHOCO
    case P_SHOCO:     shoco_decompress((const char *)in, inlen, (char *)out, outlen); return inlen;
      #endif

      #if _SMAZ
    case P_SMAZ:       smaz_decompress((const char *)in, inlen, (char *)out, outlen); return inlen;
      #endif

      #if _SHRINKER
    case P_SHRINKER:    shrinker_decompress(in, out, outlen); break;
      #endif

      #if _SNAPPY
        #if __cplusplus
    case P_SNAPPY: snappy::RawUncompress((char*)in, inlen, (char*)out);  break;
        #else
    case P_SNAPPY: snappy_uncompress(in, inlen, out, &outlen);
        #endif
      #endif

      #if _SNAPPY_C
    case P_SNAPPY_C: return snappy_uncompress((const char *)in, inlen, (char *)out);
      #endif

      #if _TORNADO
    case P_TORNADO: return tordecompress(in, out, inlen, outlen);
      #endif

      #if _UNISHOX2
    case P_UNISHOX2: return unishox2_decompressx(in, inlen, out, lev);
      #endif
      #if _UNISHOX3
    case P_UNISHOX3: { unishox3 usx3; return usx3.decompress(in, inlen, USX3_API_OUT_AND_LEN(out,outlen) ); }
      #endif

      #if _WFLZ
    case P_WFLZ:    wfLZ_Decompress( in, out); return inlen;
      #endif

      #if _WIMLIB
    case P_WIMLIB: { struct wimlib_decompressor *decompressor; if(wimlib_create_decompressor(lev, 32*1024/*inlen*/, &decompressor)) return 0;
        outlen = wimlib_decompress(in, inlen, out, outlen, decompressor);  wimlib_free_decompressor(decompressor); return inlen;
      }
      #endif

      #if _WKDM
    case P_WKDM:    WKdm_decompress ((WK_word*)in, (WK_word*)out, outlen);
      #endif

      #if _XPACK
    case P_XPACK:  { size_t rc; struct xpack_decompressor *dd = xpack_alloc_decompressor();
        outlen = xpack_decompress(dd, in, inlen,out, outlen, &rc);
        xpack_free_decompressor(dd); return outlen;
      }
      #endif

      #if _YALZ77
    case P_YALZ77: { lz77::decompress_t d; std::string extra; if(!d.feed(in,in+inlen,extra) || extra.size() > 0) return 0;
        const std::string& os = d.result(); memcpy(out, os.c_str(), os.size()); return os.size();
      }
      #endif

      #if _YAPPY
    case P_YAPPY:    return YappyUnCompress(in, in+inlen, out)-out;
      #endif

      #if _SLZ
    case P_SLZ: {
      struct slz_stream strm;
      int fmt=15;
            if(prm && *prm == 'd') fmt=-15;
       else if(prm && *prm == 'g') fmt=15|32;
       z_stream z;
       memset(&z, 0, sizeof(z));
       z.next_in   = in;
       z.avail_in  = inlen;
       z.next_out  = out;
       z.avail_out = outlen;
       if(inflateInit2(&z, fmt) == Z_OK) {
         if(inflate(&z, Z_SYNC_FLUSH) == Z_STREAM_END)
           inflateEnd(&z);
       }
    } break;
      #endif

      #if _ZLIB
    //case P_IGZIP: case P_LIBDEFLATE:
        #if _ZLIB_NG == 0
    case P_ZOPFLI:
        #endif
    case P_ZLIB: { uLongf outsize = outlen; int rc = uncompress(out, &outsize, in, inlen); } break;
      #endif

      #if _ZLIB_NG
    case P_ZOPFLI:
    case P_ZLIB_NG: { size_t outsize = outlen; int rc = zng_uncompress(out, &outsize, in, inlen); } break;
      #endif

      #if _ZSTD
    case P_ZSTD: if(strchr(prm,'D')) {
                   unsigned const expectedDictID = ZSTD_getDictID_fromDDict(ddictPtr);
                   unsigned const actualDictID   = ZSTD_getDictID_fromFrame(in, inlen);
                   if(actualDictID != expectedDictID) { printf("ZSTD: DictID mismatch: expected %u got %u", expectedDictID, actualDictID); return 0; }
                   ZSTD_DCtx *ctx = ZSTD_createDCtx();
                   inlen = ZSTD_decompress_usingDDict(ctx, out, outlen, in, inlen, ddictPtr);
                   ZSTD_freeDCtx(ctx);
                } else ZSTD_decompress( out, outlen, in, inlen);
      break;
      #endif
      //------------ Encoding -----------------------------------------------------------------------
      #if _TURBORLE
    case P_RLES:
      switch(lev) {
        case  0: return  srled( in, inlen, out, outlen);
        case  8: return  srled8( in, inlen, out, outlen, _ESC8);
        case 16: return  srled16(in, inlen, out, outlen, _ESC16);
        case 32: return  srled32(in, inlen, out, outlen, _ESC32);
        case 64: return  srled64(in, inlen, out, outlen, _ESC64);
      } break;
    case P_RLET: return trled(in, inlen, out, outlen);
      #endif
      #if _MRLE
    case P_RLEM: return mrled(in, out, outlen);
      #endif

      #if _RLE8
    case P_RLE8:
      switch(lev) {
        case  1 : { int subSections = 0; char *q; if(q=strchr(prm,'S')) subSections = atoi(q+(q[1]=='='?2:1)); return subSections?rle8m_decompress(in, inlen, out, outlen):rle8_decompress(in, inlen, out, outlen); }
        case  2 : return    rle8_ultra_decompress(in, inlen, out, outlen);
        case  8 : return  rle8_extreme_decompress(in, inlen, out, outlen);
        case 16 : return rle16_extreme_decompress(in, inlen, out, outlen);
        case 24 : return rle24_extreme_decompress(in, inlen, out, outlen);
        case 32 : return rle32_extreme_decompress(in, inlen, out, outlen);
        case 48 : return rle48_extreme_decompress(in, inlen, out, outlen);
        case 64 : return rle64_extreme_decompress(in, inlen, out, outlen);
     }
      #endif

      #if _B64
    case P_B64_PLAIN:  { size_t outlen; base64_decode(in, inlen, out, &outlen, BASE64_FORCE_PLAIN);  return inlen; }
    case P_B64_SSSE3:  { size_t outlen; base64_decode(in, inlen, out, &outlen, BASE64_FORCE_SSSE3);  return inlen; }
    case P_B64_SSE41:  { size_t outlen; base64_decode(in, inlen, out, &outlen, BASE64_FORCE_SSE41);  return inlen; }
    case P_B64_AVX:    { size_t outlen; base64_decode(in, inlen, out, &outlen, BASE64_FORCE_AVX);    return inlen; }
    case P_B64_AVX2:   { size_t outlen; base64_decode(in, inlen, out, &outlen, BASE64_FORCE_AVX2);   return inlen; }
    case P_B64_NEON64: { size_t outlen; base64_decode(in, inlen, out, &outlen, BASE64_FORCE_NEON64); return inlen; }
      #endif

      //------------ Transform -----------------------------------------------------------------------
      #if _TB64
    case P_TB64:    return tb64dec(     in, inlen, out);
    case P_TB64S:   return tb64sdec(    in, inlen, out);
    case P_TB64X:   return tb64xdec(    in, inlen, out);
        #if __ARM_NEON
    case P_TB64NEON:
        #else
    case P_TB64AVX: return tb64avxdec(  in, inlen, out);
          #ifdef AVX2_ON
    case P_TB64AVX2: return tb64avx2dec(in, inlen, out);
          #endif
        #endif
    case P_TB64SSE: return tb64ssedec(  in, inlen, out);
      #endif

      #if _DIVBWT
    case P_DIVBWT: memcpy(out, in+4, outlen); bsc_bwt_decode(out, outlen, *(unsigned *)in, 0, NULL, 0); return inlen;
      #endif
      #if _LIBBSC
    case P_LIBBSCBWT: memcpy(out, in+4, outlen); bsc_bwt_decode(out, outlen, *(unsigned *)in, 1, NULL, 0); break;
    case P_ST: {   memcpy(out, in+4, inlen-4); bsc_st_decode(out, inlen-4, lev, *(unsigned *)(in), 0); break; }
      #endif

      #if _BRC
    case P_BRC: if(lev) { struct brc_cxt_s brc_cxt; size_t o; brc_init_cxt(&brc_cxt, outlen); brc_cxt.size = inlen; memcpy(brc_cxt.block, in, inlen); brc_decode(&brc_cxt, out, &o); brc_free_cxt(&brc_cxt); return inlen; }
                else return vsrc_reverse(in, out, inlen);
      #endif
      #if _FB64
        #ifdef AVX2_ON
    case P_FB64AVX2:      fast_avx2_base64_decode(out, in,inlen);return inlen;
    case P_FB64KLOMP:    { size_t _outlen = outlen; klomp_avx2_base64_decode( (const char *)in, inlen, (char *)out, &_outlen); return inlen; }
        #endif

    case P_FB64CHROM:  chromium_base64_decode( (char*)out,(const char*)in,inlen); return inlen;
    case P_FB64LINUX:     linux_base64_decode(    (char*)out,(const char*)in,(const char*)in+inlen); return inlen;
    case P_FB64PLAIN:   { size_t _outlen = outlen; scalar_base64_decode(     (const char*)in,inlen,(char*)out,&_outlen);return inlen; }
        #if __ARM_NEON
    case P_FB64NEON:      neon_base64_decode(out, in,inlen);return inlen;
        #endif
      #endif

      #if _SB64
    case P_SB64SSE:  base64::sse::decode(base64::sse::lookup_base, in, inlen, out); return inlen;
      #endif

      //------------ Entropy Coders ------------------------------------------------------------------
      #if _MEMCPY
    case P_MCPY:     memcpy(out, in, inlen);  break;
    case P_LMCPY: libmemcpy(out, in, outlen); break;
      #endif

      #if _BCMEC
    case P_BCMEC:   return bcmdec(in, inlen, out, outlen);
      #endif

      #if _FASTAC
    case P_FASTAC: {
        Adaptive_Data_Model adaptive_model(SYMNUM);
        adaptive_model.set_alphabet(SYMNUM);
        adaptive_model.reset();
        Arithmetic_Codec decoder(outlen, in);
        decoder.start_decoder();
        for(int i = 0; i < outlen; i++)
          out[i] = (unsigned char)(decoder.decode(adaptive_model));
        decoder.stop_decoder();
        return outlen;
      }
      #endif

      #if _FASTHF
    case P_FASTHF: {
        Adaptive_Huffman_Code adaptive_model(SYMNUM);
        adaptive_model.set_alphabet(SYMNUM); adaptive_model.reset();
        Binary_Codec decoder(outlen, in);
        decoder.start_decoder();
        for(int i = 0; i < outlen; i++)
          out[i] = (unsigned char)(decoder.decode(adaptive_model));
        decoder.stop_decoder();
      } break;
      #endif

      #if _FPC
    case P_FPC:  return FPC_decompress(out,outlen,in,inlen);
      #endif

      #if _FSC
    case P_FSC:     { size_t outsize = 0; uint8_t *op = NULL; int ok = FSCDecode(in, inlen, &op, &outsize); if(ok) { memcpy(out,op,outlen); free(op); } } break;
      #endif

      #if _FSE
    case P_FSE:  if(inlen == outlen) memcpy(out, in, outlen); else if(inlen == 1) memset(out,in[0],outlen); else FSE_decompress(out, outlen, in, inlen); break;
      #endif
	  
      #if _FSEHUF
    case P_FSEH: if(inlen == outlen) memcpy(out, in, outlen); else if(inlen == 1) memset(out,in[0],outlen); else HUF_decompress(out, outlen, in, inlen); break;
      #endif

      #if _FQZ
    case P_FQZ0: uncompress_block(in, out, outlen); break;
      #endif

      #if _JAC
    case P_JAC:  { unsigned outlen; arith_uncompress_O0(in, inlen, &outlen, out); } break;
      #endif

      #if _HTSCODECS
    //case P_RANS_S8   : rans_uncompress_to(    8,in, inlen, out, (unsigned *)&outlen, lev); break;
    //case P_RANS_S16  : rans_uncompress_to(   16,in, inlen, out, (unsigned *)&outlen, lev); break;
        #ifdef __AVX2__
    case P_RANS_S32  : rans_uncompress_O0_32x16_avx2(in, inlen, out, outlen); break;
        #endif
      #endif

      #if _FPAQC
    case P_FPAQC:   absd(in, outlen, out); break;
      #endif

      #if _FREQTAB
    case P_FREQTAB: return freqtabdec(in, inlen, out, outlen);
      #endif

      #if _MMRC
    case P_O0ZP:    rczmmd(in, outlen, out); break;
    case P_FPAQ0F2: rcmm2d(in, outlen, out); break;
    case P_MMRC:    rcmmd( in, outlen, out); break;
      #endif

      #if _MARLIN
    case P_MARLIN:  Marlin_decompress((Marlin *)workmem, (uint8_t*)out, outlen, in, inlen); /*Marlin_free_dictionary((Marlin *)workmem);*/ break;
      #endif

      #if _NANS
    case P_NANS:    nansdec(in, inlen, out, outlen); break;
      #endif

       #if _NIBRANS
    case P_NIBRANS: { struct nibrans nbra; nibransInit(&nbra); return nibransDecode(&nbra, out, outlen, in, inlen); }
      #endif

     #if _PPMDEC
    case P_PPMDEC:  return ppmddec(in, inlen, out, outlen);
      #endif

      #if _POLHF
    case P_POLHF:   polardec(in, out, outlen); break;
      #endif

      #if _RECIPARITH
    case P_RECIPARITH:     return radec(in, inlen, out, outlen);
      #endif

      #if _SHRC
    case P_SHRCV:   vecrcdec(in, outlen, out); break;
    case P_SHRC:    rcshd(in, out, outlen); break;
      #endif

      #if _SUBOTIN
    case P_SUBOTIN:     subdec(in, outlen, out, outlen, SYMNUM); break;
      #endif

      #if _TORNADOHF
    case P_TORNADOHF:    torhdec(in, inlen, out, outlen); break;
      #endif

      #if _TURBORC
    case P_TURBORC: { //unsigned prm1 = 5,prm2 = 6; char *q; //if(q=strchr(prm,'r')) { prm1 = atoi(q+(q[1]=='='?2:1)); prm2 = prm1%10; prm1 = prm1/10; if(prm1>9)prm1=9;if(!prm1) prm1=1; if(prm2>9)prm2=9;if(!prm2) prm2=1; }
	  unsigned bwtlev = 9;
	  char *q;
	  if(q=strchr(prm,'e')) bwtlev = atoi(q+(q[1]=='='?2:1)); 
      switch(lev) {
        case  1 : return rcsdec(    in, outlen, out);
        case  2 : return rccsdec(   in, outlen, out);
        case  3 : return rcc2sdec(  in, outlen, out);
        case  4 : return rcxsdec(   in, outlen, out);

        //case  9 : return rcmsdec(   in, outlen, out);
        //case 10 : return rcm2sdec(  in, outlen, out);
        case 12 : return rcrlesdec( in, outlen, out);
        case 14 : return rcrle1sdec(in, outlen, out);        //case 17 : return rcqlfcsdec( in, outlen, out);
        //case 20 : return rcbwtdec( in, outlen, out, bwtlev, 0);
        //case 21 : if(inlen==outlen) memcpy(out,in,outlen); else utf8dec( in, outlen, out); return outlen;
        //case 90 : if(inlen==outlen) memcpy(out,in,outlen); else lzpdec(  in, outlen, out, 1, 0); return outlen;
      }	  
    }
      #endif

      #if _ZLIB
    case P_ZLIBH: { unsigned char *out_=out+outlen,*ip=in,*op; unsigned oplen;//return ZLIBH_decompress((char* )out, (const char*)in);
      for(op = out; op != out+outlen; op+=oplen) { oplen = min(out_-op,lev*1024);
        z_stream z; memset(&z, 0, sizeof(z));
        if(inflateInit2(&z, -15) == Z_OK) {
          z.avail_in  = ctou16(ip);   ip+=2;
          z.next_in   = ip;           ip+=z.avail_in;
          z.next_out  = op;
          z.avail_out = outlen;
          if(inflate(&z, Z_SYNC_FLUSH) != Z_STREAM_END) break; //op += outlen - z.avail_out;
          inflateEnd(&z);
        }
      }
    } break;
      #endif

      #if _ZLIB
    case P_ZRLE: { z_stream z; memset(&z, 0, sizeof(z));
      if(inflateInit2(&z, -15) == Z_OK) {
        z.next_in   = in;
        z.avail_in  = inlen;
        z.next_out  = out;
        z.avail_out = outlen;
        if(inflate(&z, Z_SYNC_FLUSH) != Z_STREAM_END) break;
        inflateEnd(&z);
      }
    } break;
      #endif

      #ifdef _LZTURBO
    #include "../dev/x/beplugd.c"
      #endif

      #if _MYCODEC
//   case P_MYCODEC:   return mydecomp(in, inlen, out, outlen);
      #endif
  }
  return NULL;
}

char *codver(int codec, char *v, char *s) {
  switch(codec) {
      #if _C_BLOSC2
    return BLOSC2_VERSION_STRING;
      #endif
      #if _BRIEFLZ
    case P_BRIEFLZ: sprintf(s,"%d.%d.%d", BLZ_VER_MAJOR, BLZ_VER_MINOR, BLZ_VER_PATCH); break;
      #endif

      #if _LZ4
    case P_LZ4:     sprintf(s,"%d.%d.%d", LZ4_VERSION_MAJOR, LZ4_VERSION_MINOR, LZ4_VERSION_RELEASE); break;
      #endif

      #if _LIZARD
    case P_LIZARD:     sprintf(s,"%d.%d.%d", LIZARD_VERSION_MAJOR, LIZARD_VERSION_MINOR, LIZARD_VERSION_RELEASE); break;
      #endif

      #if _ZSTD
    case P_ZSTD:    sprintf(s,"%d.%d.%d", ZSTD_VERSION_MAJOR, ZSTD_VERSION_MINOR, ZSTD_VERSION_RELEASE); break;
      #endif

      #if _DENSITY
    case P_DENSITY: sprintf(s,"%d.%d.%d", density_version_major(), density_version_minor(), density_version_revision()); break;
      #endif

      #if _HEATSHRINK
    case P_HEATSHRINK: sprintf(s,"%d.%d.%d", HEATSHRINK_VERSION_MAJOR, HEATSHRINK_VERSION_MINOR, HEATSHRINK_VERSION_PATCH); break;
      #endif

      #if _SNAPPY
    case P_SNAPPY:  sprintf(s,"%d.%d.%d", SNAPPY_MAJOR, SNAPPY_MINOR, SNAPPY_PATCHLEVEL); break;
      #endif
      #if _ZLIB
    case P_ZLIB:
      sprintf(s,zlib_version); break;
      #endif
      #if _ZLIB_NG
    case P_ZLIB_NG:
      sprintf(s,zlibng_version()); break;
      #endif
    default:        strcpy(s,v);
  }
  return s;
}

