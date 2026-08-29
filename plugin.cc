/**
    Copyright (C) powturbo 2013-2026
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
//     TurboBench: plugin.cc - compressor plugins
#include <cstdlib>
#include <cstddef>
#include <atomic>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "conf.h"
#include "plugin.h"

enum {
#define _MEMCPY 1
 P_LMCPY,   // must be 0
 P_MCPY,    // must be 1
#ifndef _AOCL
#define _AOCL 0
#endif
 P_AOCL_LZ4,
 P_AOCL_LZ4HC,
 P_AOCL_LZMA,
 P_AOCL_BZIP2,
 P_AOCL_SNAPPY,
 P_AOCL_ZLIB,
 P_AOCL_ZSTD,

#ifndef _AOM
#define _AOM 0
#endif
 P_AOM,
 
#ifndef _BPC
#define _BPC 0
#endif
 P_BPC,
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
#ifndef _CLICKHOUSE
#define _CLICKHOUSE 0
#endif
 P_CLICKHOUSE,
 
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
#ifndef _ISA_L
#define _ISA_L 0
#endif

 P_ISA_L,
#ifndef _KANZI
#define _KANZI 0
#endif
 P_KANZI,
 
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
 
#ifndef _MEMLZ
#define _MEMLZ 0
#endif
 P_MEMLZ,
 
#ifndef _LZ4
#define _LZ4 0
#endif
 P_LZ4,
#ifndef _LZ4ULTRA
#define _LZ4ULTRA 0
#endif
 P_LZ4ULTRA,
#ifndef _LZAV
#define _LZAV 0
#endif
 P_LZAV,
#ifndef _LZFSE
#define _LZFSE 0
#endif
 P_LZFSE,
#ifndef _LZFSEA
#define _LZFSEA 0
#endif
 P_LZFSEA,
#ifndef _LZHAM
#define _LZHAM 0
#endif
 P_LZHAM,
#ifndef _LIZARD
#define _LIZARD 0
#endif
 P_LIZARD,
#ifndef _LZJODY
#define _LZJODY 0
#endif
 P_LZJODY,
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
#ifndef _MISA77
#define _MISA77 0
#endif
 P_MISA77,
 P_MISA77S,
#ifndef _MSCOMPRESS
#define _MSCOMPRESS 0
#endif
 P_MSCOMPRESS,

#ifndef _OPENZL
#define _OPENZL 0
#endif
 P_OPENZL_U8,
 P_OPENZL_I8,
 P_OPENZL_U16,
 P_OPENZL_I16,
 P_OPENZL_U32,
 P_OPENZL_I32,
 P_OPENZL_U64,
 P_OPENZL_I64,
 P_OPENZL_SERIAL,
 P_OPENZL_GENERIC,
 P_OPENZL_ZSTD,
 P_OPENZL_LZ4,
 P_OPENZL_TP,

#ifndef _PCODEC
#define _PCODEC 0
#endif
 P_PCODECI8,
 P_PCODECU8,
 P_PCODECI16,
 P_PCODECU16,
 P_PCODECF16,
 P_PCODECI32,
 P_PCODECU32,
 P_PCODECF32,
 P_PCODECI64,
 P_PCODECU64,
 P_PCODECF64,
#ifndef _PITHY
#define _PITHY 0
#endif
 P_PITHY,
#ifndef _PYSAP
#define _PYSAP 0
#endif
 P_PYSAP,
 
#ifndef _QUICKLZ
#define _QUICKLZ 0
#endif
 P_QUICKLZ,
 
#ifndef _SHRINKER
#define _SHRINKER 0
#endif
 P_SHRINKER,
#ifndef _FIRETRAIL
#define _FIRETRAIL 0
#endif
 P_FIRETRAIL,
#ifndef _LIBSLZ
#define _LIBSLZ 0
#endif
 P_LIBSLZ,
 
#ifndef _OODLE 
#define _OODLE 0
#endif
#ifndef _OODLE_EC 
#define _OODLE_EC 0
#endif
 P_OODLE_HUF,
 P_OODLE_ANS,
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

#ifndef _TAMP
#define _TAMP 0
#endif
 P_TAMP,
#ifndef _TCOBS
#define _TCOBS 0
#endif
 P_TCOBS,
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
#ifndef _ZPAQ
#define _ZPAQ 0
#endif
 P_ZPAQ,
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
#ifndef _XZ
#define _XZ 0
#endif
 P_XZ,
#ifndef _ZXC
#define _ZXC 0
#endif
 P_ZXC,
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
 
#ifndef _HRLE
#define _HRLE 0
#endif
 P_HRLE,
 P_HRLESH,
 P_HRLEMMTF,
 P_HRLEXMMTF,
 P_HRLELE,
 P_HRLEM,
 P_HRLEU,
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
#ifndef _SSERC
#define _SSERC 0
#endif
 P_SSERC,
#ifndef _GANS
#define _GANS 0
#endif
 P_GANSR,
 P_GANSW,
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
#ifndef _PIVCOHUF
#define _PIVCOHUF 0
#endif
 P_PIVCOHUF,          // PivCo-Huffman (ph/pha)
#ifndef _PHAZ
#define _PHAZ 0
#endif
 P_PHAZ,              // PHAZ: PivCo-Huffman entropy transplant onto zstd
 P_MYCODEC, // User plugin
  #ifdef _LZ
#include "../lz/x/beplug.h"
  #endif
};

//-------------------------------------------------------------------------------------------------------------------
  #if _AOCL
#include "aocl-compression/api/aocl_compression.h"
static aocl_compression_desc aocl;
#define AOCL_CODEC(codec, lev) (aocl_compression_type)(((codec == P_AOCL_LZ4 && lev>0)?P_AOCL_LZ4HC:codec) - P_AOCL_LZ4)
  #endif

  #if _AOM
#include "EC/aom_/aom.h"
  #endif

  #if _BPC
#include "BitPlaneComp/src/BPCompressor.hh"
  #endif

  #if _BRIEFLZ
#include "brieflz/include/brieflz.h"
  #endif

  #if _BZIP2
#include "bzip2/bzlib.h"
  #endif

  #if _BROTLI
#include "brotli/c/include/brotli/encode.h"
#include "brotli/c/include/brotli/decode.h"
#include "brotli/c/common/version.h"
  #endif

  #if _C_BLOSC2
    #ifdef C_C_BLOSC2LZ
#include "c-blosc2/blosclz.h"
    #else
#include "c-blosc2/include/blosc2.h"
    #endif
    
#include "c-blosc2/include/blosc2.h"
#include "c-blosc2/include/blosc2/filters-registry.h"

#define ICC_LZ4  1
#define ICC_ZSTD 2

unsigned blosccomp(unsigned char *in, size_t inlen, unsigned char *out, unsigned outsize, unsigned codid, int codlev, unsigned esize, int filter0, int filter1, int filter2) {
  unsigned clevel   = codid==ICC_ZSTD?((codlev+1)/2):codlev;
  unsigned compcode = codid==ICC_LZ4?(clevel>9?BLOSC_LZ4HC:BLOSC_LZ4):BLOSC_ZSTD;
  blosc2_schunk schunk;
  schunk.typesize   = esize?esize:1;
  blosc2_cparams cp = BLOSC2_CPARAMS_DEFAULTS;
        cp.typesize = esize?esize:1;
        cp.compcode = compcode;                                                        //BLOSC_LZ4HC, BLOSC_LZ4, BLOSC_ZSTD, BLOSC_LZ4, BLOSC_BLOSCLZ
        cp.clevel   = clevel<1?1:(clevel<9?clevel:9);                                  //blocksize=[1,32768[2,65536][3,131072][4,262144][5,262144][6,524288][7,524288][8,524288][9,1048576]
        cp.nthreads = 1;
        cp.schunk   = &schunk;
        cp.filters[BLOSC2_MAX_FILTERS - 1] = filter0; //BLOSC_NOFILTER, BLOSC_SHUFFLE, BLOSC_BITSHUFFLE
        cp.filters[BLOSC2_MAX_FILTERS - 2] = filter1; //BLOSC_DELTA, BLOSC_FILTER_BYTEDELTA
        cp.filters[BLOSC2_MAX_FILTERS - 3] = filter2; //BLOSC_TRUNC_PREC
        //cp.filters_meta[BLOSC2_MAX_FILTERS - 1] = 0;  // 0 means typesize when using schunks

  blosc2_context *ctx = blosc2_create_cctx(cp);
  int rc = blosc2_compress_ctx(ctx, in, (int)inlen, out, (int)outsize);
  blosc2_free_ctx(ctx);
  if(rc>inlen) { memcpy(out,in,inlen); rc = inlen; }
  return rc;
}

unsigned bloscdecomp(unsigned char *in, size_t inlen, unsigned char *out, unsigned outlen, unsigned esize) {
  blosc2_schunk schunk;
  schunk.typesize     = esize?esize:1;
  blosc2_dparams   dp = BLOSC2_DPARAMS_DEFAULTS;
                   dp.schunk = &schunk;
  blosc2_context *ctx = blosc2_create_dctx(dp);
  int rc = blosc2_decompress_ctx(ctx, in, inlen, out, outlen);
  blosc2_free_ctx(ctx);
  return rc;
}   
  #endif

  #if _CLICKHOUSE
#include "Clickhouse/src/Compression/LZ4_decompress_faster.h"  
  #endif
  
  #if _CSC
#define __7Z_TYPES_H  
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
#define __LZMA_ENC_H  
#define __LZMA_DEC_H  
#include "fast-lzma2/lzma2_enc.h"
#include "fast-lzma2/lzma2_dec.h"
  #endif

  #if _GIPFELI
#include "gipfeli/gipfeli.h"
  #endif

  #if _GLZA
#include "GLZA/GLZAcomp.h"
#include "GLZA/GLZAdecode.h"
  #endif

  #if _HEATSHRINK
#include "heatshrink_/heatshrink.h"
  #endif

  #if _ISA_L
    #ifdef HAVE_IGZIP
#include <isa-l.h>
    #else
#include "isa-l/include/isal_api.h"
#include "isa-l/include/igzip_lib.h"
    #endif
  #endif

  #if _KANZI
#include "kanzi-cpp/src/types.hpp"
#include "kanzi-cpp/src/InputStream.hpp"
#include "kanzi-cpp/src/OutputStream.hpp"
#include "kanzi-cpp/src/io/CompressedInputStream.hpp"
#include "kanzi-cpp/src/io/CompressedOutputStream.hpp"
#include "kanzi-cpp/src/util/fixedbuf.hpp"
#include "kanzi-cpp/src/api/Decompressor.hpp"

// derived from lzbench
int64_t kanzi_compress(char *inbuf, size_t insize, char *outbuf, size_t outsize, int threadnum, int lev) {
  std::string entropy;
  std::string transform;
  kanzi::uint szBlock;
  switch (lev) {
    case 0: transform = "NONE";                      entropy = "NONE";    szBlock =  4 * 1024 * 1024; break;
    case 1: transform = "LZX";                       entropy = "NONE";    szBlock =  4 * 1024 * 1024; break;
    case 2: transform = "DNA+LZ";                    entropy = "HUFFMAN"; szBlock =  4 * 1024 * 1024; break;
    case 3: transform = "TEXT+UTF+PACK+MM+LZX";      entropy = "HUFFMAN"; szBlock =  4 * 1024 * 1024; break;
    case 4: transform = "TEXT+UTF+EXE+PACK+MM+ROLZ"; entropy = "NONE";    szBlock =  4 * 1024 * 1024; break;
    case 5: transform = "TEXT+UTF+BWT+RANK+ZRLT";    entropy = "ANS0";    szBlock =  4 * 1024 * 1024; break;
    case 6: transform = "TEXT+UTF+BWT+SRT+ZRLT";     entropy = "FPAQ";    szBlock =  8 * 1024 * 1024; break;
    case 7: transform = "LZP+TEXT+UTF+BWT+LZP";      entropy = "CM";      szBlock = 16 * 1024 * 1024; break;
    case 8: transform = "EXE+RLT+TEXT+UTF+DNA";      entropy = "TPAQ";    szBlock = 16 * 1024 * 1024; break;
    case 9: transform = "EXE+RLT+TEXT+UTF+DNA";      entropy = "TPAQX";   szBlock = 32 * 1024 * 1024; break;
    default:  return -1;
  }
  ofixedbuf buf(outbuf, outsize);
  std::iostream os(&buf);
  kanzi::CompressedOutputStream cos(os, threadnum, entropy, transform, szBlock);
  const size_t max_io_size = size_t(1) << 30;
        size_t remaining = insize;
          char *next = inbuf;

  while (remaining > 0) {
    const size_t chunk = std::min(remaining, max_io_size);
     cos.write(next, static_cast<std::streamsize>(chunk));
     next += chunk;
     remaining -= chunk;
  }
  cos.close();
  return cos.getWritten();
}

int64_t kanzi_decompress(char *inbuf, size_t insize, char *outbuf, size_t outsize, int threadnum) {
  ifixedbuf buf(inbuf, insize);
  std::iostream is(&buf);
  kanzi::CompressedInputStream cis(is, threadnum);
  const size_t max_io_size = size_t(1) << 30;
        size_t total = 0;

  while (total < outsize) {
    const size_t chunk = std::min(outsize - total, max_io_size);
    cis.read(outbuf + total, static_cast<std::streamsize>(chunk));
    const size_t decoded = static_cast<size_t>(cis.gcount());
    total += decoded;
    if (decoded != chunk)
      break;
  }
  cis.close();
  return total;
}
  #endif
  
  #if _LIBBSC
#include "libbsc/libbsc/libbsc.h"
#include "libbsc/libbsc/st/st.h"
#include "libbsc/libbsc/lzp/lzp.h"
  #endif

  #if _LIBDEFLATE
#include "libdeflate/libdeflate.h"
  #endif

  #if _LIBLZG
#include "liblzg/src/include/lzg.h"
  #endif

  #if _LZLIB
#include "lzlib-1.16/lzlib.h"
  #endif

  #if _LIZARD
#include "lizard/lib/lizard_compress.h"    //v2.0
#include "lizard/lib/lizard_decompress.h"
  #endif

  #if _LZMA
#include "lzma/C/Alloc.h"
#include "lzma/C/LzmaEnc.h"
#include "lzma/C/LzmaDec.h"
#include "lzma/C/7zVersion.h"
  #endif

  #if _MEMLZ
#include "memlz/memlz.h"
  #endif

  #if _ZPAQ
#include "zpaq/libzpaq.h"
void libzpaq::error(const char* msg) {
  fprintf(stderr, "zpaq error: %s\n", msg);
  exit(1);
}

static unsigned char *zin,*zin_,*zout;
#define _putc(_ch_, _out_) *_out_++ = (_ch_)
#define _getc(_in_, _ep_) (_in_<_ep_?*_in_++:-1)

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

  #if _LZAV
#include "lzav/lzav.h"  
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

  #if _MISA77
#include "misa77/include/misa77/misa77.h"
  #endif

  #if _MSCOMPRESS
#include "ms-compress/include/mscomp.h"
  #endif

  #if _LZSS
#include "lzss/lzss.h"
  #endif

  #if _PCODEC
#include "pcodec_/cpcodec.h" // https://github.com/pcodec/pcodec --------------------------------------------
  #if defined(_WIN32)
#include <windows.h>
#define __off64_t   _off64_t
  #else
#include <dlfcn.h> // dlopen
  #endif
  
typedef enum PcoError(*fpco_compress  )(const void *nums,       size_t n,              unsigned char dtype, const struct PcoChunkConfig *config, void *dst, size_t dst_cap, size_t *n_written);
typedef enum PcoError(*fpco_decompress)(const void *compressed, size_t compressed_len, unsigned char dtype,                                      void *dst, size_t dst_cap, size_t *n_written);
static fpco_compress   pco_compress;
static fpco_decompress pco_decompress;
static int pco = 0;
void pco_ini() {
  if (pco)
    return;
  pco++;
    #if _WIN32
  HINSTANCE hdll;
  if(hdll = LoadLibrary("libcpcodec.dll")) {
    if(!(pco_compress   = (fpco_compress  )GetProcAddress(hdll, "pco_standalone_simple_compress_into")))   die("fpco_simpler_compress not found\n");
    if(!(pco_decompress = (fpco_decompress)GetProcAddress(hdll, "pco_standalone_simple_decompress_into"))) die("pco_simple_decompress not found\n");
  }
  else fprintf(stderr, "libcpcodec.dll not found\n");
    #elif !defined(_STATIC)
  void* hdll = dlopen("./libcpcodec.so", RTLD_LAZY);
  if (hdll) {
    if (!(pco_compress   = (fpco_compress  )dlsym(hdll, "pco_standalone_simple_compress_into"  ))) die("fpco_simpler_compress not found\n");
    if (!(pco_decompress = (fpco_decompress)dlsym(hdll, "pco_standalone_simple_decompress_into"))) die("pco_simple_decompress not found\n");
  }
  else fprintf(stderr, "libcpcodec.so shared library not found. '%s'\n", dlerror());
    #endif
}
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

  #if _TAMP
#include "tamp/tamp/_c_src/tamp/compressor.h"
#include "tamp/tamp/_c_src/tamp/decompressor.h"
  #endif

  #if _TCOBS
#include "tcobs/v2/tcobs.h"
  #endif

  #if _TORNADO
#include "tornado_/tormem.h"
  #endif

  #if _TURBORC
#include "Turbo-Range-Coder/include/turborc.h"
#include "Turbo-Range-Coder/include/anscdf.h"
#include "Turbo-Range-Coder/include_/tp.h"
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

  #if _XZ
#include "xz/src/liblzma/api/lzma.h" //derived from lzbench
int64_t _xz_compress(char *in, size_t insize, char *out, size_t outsize, int lev, int threadnum) {
  lzma_stream strm = LZMA_STREAM_INIT;
  lzma_ret ret;
  lzma_mt mt_options    = {0};
  mt_options.preset     = (lev >= 0 && lev <= 9)  ? (uint32_t)lev  : LZMA_PRESET_DEFAULT;
  mt_options.check      = LZMA_CHECK_NONE; // Check type (CRC64 is default and common)  //mt_options.check = LZMA_CHECK_CRC32;
  mt_options.threads    = threadnum;
  mt_options.block_size = 0;
  ret = lzma_stream_encoder_mt(&strm, &mt_options);
  if (ret != LZMA_OK) return -1;
  strm.next_in = (const uint8_t *)in;
  strm.avail_in = insize;
  strm.next_out = (uint8_t *)out;
  strm.avail_out = outsize;
  ret = lzma_code(&strm, LZMA_FINISH);
  if (ret != LZMA_STREAM_END) { lzma_end(&strm);  return -2;  }
  size_t compressed_size = strm.total_out;
  lzma_end(&strm);
  return (int64_t)compressed_size;
}

int64_t _xz_decompress(char *in, size_t insize, char *out, size_t outsize, int threadnum) {
  lzma_stream strm = LZMA_STREAM_INIT;
  lzma_ret ret;
  lzma_mt mt_options = {0};
  mt_options.threads = threadnum;
  mt_options.memlimit_stop = UINT64_MAX;
  mt_options.flags = LZMA_CONCATENATED | LZMA_IGNORE_CHECK;

  ret = lzma_stream_decoder_mt(&strm, &mt_options);
  if (ret != LZMA_OK) { lzma_end(&strm); return -1; }
  strm.next_in = (const uint8_t *)in;
  strm.avail_in = insize;
  strm.next_out = (uint8_t *)out;
  strm.avail_out = outsize;
  ret = lzma_code(&strm, LZMA_FINISH);  if (ret != LZMA_STREAM_END) { lzma_end(&strm); return -2;  }
  size_t decompressed_size = strm.total_out;
  lzma_end(&strm);
  return (int64_t)decompressed_size;
}
  #endif

  #if _ZLIBLIB
#include <zlib.h>
//     #elif defined(ZLIB_NG)          // zlib-ng.a compatible mode : "./configure --zlib-compat" (see zlib-ng/INSTALL)
//#include "zlib/zlib.h"
  #elif defined(ZLIB_INTEL)
#include "zlib_intel/zlib.h"
  #else
#include "zlib/zlib.h"
  #endif

  #if _ZLING
#include "libzling/src/libzling.h"
#include "libzling_/libzling_utils_mem.h"
  #endif

  #ifdef _LZ
#include "../lz/x/beplugi.h"
  #endif
//-----------------------------
  #if _BRC
#include "Behemoth-Rank-Coding/brc.hpp"
int vsrc_forwards(unsigned char * src, unsigned char * dst, size_t src_size);
int vsrc_reverse(unsigned char * src, unsigned char * dst, size_t src_size);
  #endif

  #if _OPENZL  // Adapted from lzbench
#include "openzl/include/openzl/openzl.h"
#include "openzl/include/openzl/codecs/zl_segmenters.h"
#include "openzl/include/openzl/openzl.h"
#include "openzl/src/openzl/codecs/transpose/decode_transpose_kernel.h"
#include "openzl/src/openzl/codecs/transpose/encode_transpose_kernel.h"
#include "openzl/src/openzl/shared/portability.h"
#define OPENZL_FORMAT_VERSION 24
#define WINDOWLOG_OPENZL      27
typedef struct {
  ZL_Compressor* cgraph;
  ZL_CCtx *cctx;
  ZL_DCtx *dctx;
} openzl_params_s;

static openzl_params_s *_openzl_init_base(size_t insize, size_t level, size_t windowLog) {
  openzl_params_s *params = (openzl_params_s*)malloc(sizeof(openzl_params_s));
  params->cgraph = ZL_Compressor_create();  assert(params->cgraph);
  params->cctx   = ZL_CCtx_create();        assert(params->cctx);
  params->dctx   = ZL_DCtx_create();        assert(params->dctx);
  ZL_Report report = ZL_Compressor_setParameter(params->cgraph, ZL_CParam_formatVersion, OPENZL_FORMAT_VERSION);
  if(ZL_isError(report)) die("OpenZL initialisation error: %s\n", ZL_Compressor_getErrorContextString(params->cgraph, report));
  return params;
}

static openzl_params_s *_openzl_init_serial(size_t insize, size_t level, size_t windowLog) {
  openzl_params_s *params = _openzl_init_base(insize, level, windowLog);  
  ZL_Report report = ZL_Compressor_selectStartingGraphID(params->cgraph, ZL_GRAPH_LZ);// ZL_GRAPH_LZ: standard graph for LZ compression, offer performance similar to Zstd. Used for serial data (aka raw bytes).
  if (ZL_isError(report)) printf("OpenZL initialisation error: %s\n", ZL_Compressor_getErrorContextString(params->cgraph, report));
  return params;
}

template <typename TInteger>
static openzl_params_s *_openzl_init_integer_t(size_t insize, size_t level, size_t windowLog) {
  openzl_params_s *params = _openzl_init_base(insize, level, windowLog);
  ZL_GraphID graph = ZL_GRAPH_FIELD_LZ; // Build a graph to compress signed or unsigned integers (Little Endian). Adapted from OpenZL buildIntProfile() source code in cli/utils/compress_profiles.cpp .
  if (std::is_signed<TInteger>::value) 
    graph = ZL_Compressor_registerStaticGraph_fromNode1o(params->cgraph, ZL_NODE_ZIGZAG, graph);
  
  graph = ZL_Compressor_registerStaticGraph_fromNode1o(params->cgraph, ZL_Node_interpretAsLE(8*sizeof(TInteger)), graph);
  graph = ZL_Compressor_buildNumFromSerialSegmenter(params->cgraph, sizeof(TInteger), 0, graph);

  ZL_Report report = ZL_Compressor_selectStartingGraphID(params->cgraph, graph);
  if (ZL_isError(report)) die("OpenZL initialisation error: %s\n", ZL_Compressor_getErrorContextString(params->cgraph, report));
  return params;
}

template openzl_params_s *_openzl_init_integer_t<uint8_t >(size_t insize, size_t level, size_t windowLog);
template openzl_params_s *_openzl_init_integer_t<int8_t  >(size_t insize, size_t level, size_t windowLog);
template openzl_params_s *_openzl_init_integer_t<uint16_t>(size_t insize, size_t level, size_t windowLog);
template openzl_params_s *_openzl_init_integer_t<int16_t >(size_t insize, size_t level, size_t windowLog);
template openzl_params_s *_openzl_init_integer_t<uint32_t>(size_t insize, size_t level, size_t windowLog);
template openzl_params_s *_openzl_init_integer_t<int32_t >(size_t insize, size_t level, size_t windowLog);
template openzl_params_s *_openzl_init_integer_t<uint64_t>(size_t insize, size_t level, size_t windowLog);
template openzl_params_s *_openzl_init_integer_t<int64_t >(size_t insize, size_t level, size_t windowLog);

openzl_params_s *_openzl_init_generic(size_t insize, size_t level, size_t windowLog) {
  openzl_params_s *params = _openzl_init_base(insize, level, windowLog);
  // ZL_GRAPH_COMPRESS_GENERIC: "default" generic compression suitable for any stream type. Used as a fallback if a compressor does not match the characteristics of the data. Currently corresponds to Zstd level 6.
  ZL_Report report = ZL_Compressor_selectStartingGraphID(params->cgraph, ZL_GRAPH_COMPRESS_GENERIC);
  if (ZL_isError(report)) die("OpenZL initialisation error: %s\n", ZL_Compressor_getErrorContextString(params->cgraph, report));
  return params;
}

openzl_params_s *_openzl_init_zstd(size_t insize, size_t level, size_t windowLog) {
  openzl_params_s *params = _openzl_init_base(insize, level, windowLog);  
  ZL_Report report = ZL_Compressor_selectStartingGraphID(params->cgraph, ZL_GRAPH_ZSTD); // ZL_GRAPH_ZSTD: Zstd compression.
  if (ZL_isError(report)) die("OpenZL initialisation error: %s\n", ZL_Compressor_getErrorContextString(params->cgraph, report));
   // Valid compression levels range from -99 (?) to -1 and from 1 to 22. Level 0 requests the default behaviour, which corresponds to level 6.
  report = ZL_Compressor_setParameter(params->cgraph, ZL_CParam_compressionLevel, level);
  if (ZL_isError(report)) {
    printf("OpenZL initialisation error: %s\n", ZL_Compressor_getErrorContextString(params->cgraph, report));
    abort();
  }
  return params;
}

static openzl_params_s *_openzl_init_lz4(size_t insize, size_t level, size_t windowLog) {
  openzl_params_s *params = _openzl_init_base(insize, level, windowLog);
  ZL_Report report = ZL_Compressor_selectStartingGraphID(params->cgraph, ZL_GRAPH_LZ4); // ZL_GRAPH_LZ4: LZ4 compression.
  if (ZL_isError(report)) die("OpenZL initialisation error: %s\n", ZL_Compressor_getErrorContextString(params->cgraph, report)); 
  report = ZL_Compressor_setParameter(params->cgraph, ZL_CParam_compressionLevel, level); // Valid compression levels range from -99 (?) to -1 and from 1 to 12. Level 0 requests the default behaviour, which corresponds to level 6.
  if (ZL_isError(report)) die("OpenZL initialisation error: %s\n", ZL_Compressor_getErrorContextString(params->cgraph, report));
  return params;
}

static void _openzl_deinit(openzl_params_s *params) {
  if (!params) return;
  if (params->dctx) ZL_DCtx_free(params->dctx);
  if (params->cctx) ZL_CCtx_free(params->cctx);
  if (params->cgraph) ZL_Compressor_free(params->cgraph);
  free(params);
}

static int64_t _openzl_compress(char *inbuf, size_t insize, char *outbuf, size_t outsize, openzl_params_s *params) {
  if(!params || !params->cctx || !params->cgraph) return 0;
  ZL_Report report = ZL_CCtx_refCompressor(params->cctx, params->cgraph);
  if (ZL_isError(report)) die("OpenZL compression error: %s\n", ZL_CCtx_getErrorContextString(params->cctx, report));
  report = ZL_CCtx_compress(params->cctx, outbuf, outsize, inbuf, insize);
  if(ZL_isError(report)) die("OpenZL compression error: %s\n", ZL_CCtx_getErrorContextString(params->cctx, report));
  return (int64_t) ZL_validResult(report);
}

static int64_t _openzl_decompress(char *inbuf, size_t insize, char *outbuf, size_t outsize, openzl_params_s *params) {
  if(!params || !params->dctx) return 0;
  ZL_Report report = ZL_DCtx_decompress(params->dctx, outbuf, outsize, inbuf, insize);
  if (ZL_isError(report)) die("OpenZL decompression error: %s\n", ZL_DCtx_getErrorContextString(params->dctx, report));
  return (int64_t) ZL_validResult(report);
}
  #endif
//=============================================================================================================================

  #if __cplusplus
extern "C" {
  #endif

  #if _FIRETRAIL
#include "firetrail/firetrail.h"
static firetrail_encoder_t *firetrail_encoder;
static firetrail_decoder_t *firetrail_decoder;
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

  #if _LIBSLZ
#include "libslz/src/slz.h"
  #endif

  #if _LZFSE
#include "lzfse/src/lzfse.h"
  #endif

  #if _LZJODY
#include "lzjody/lzjody.h"
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
typedef long long (__cdecl *fOodleLZ_Decompress)(void *in, long long inlen, void *out, long long outlen, int crc, int p5, long long verb, void *dic, long long diclen, void *p9, long long p10, void *p11, long long p12, int p13);
typedef struct OodleLZ_CompressOptions *(__cdecl *fOodleLZ_CompressOptions_GetDefault)(int compid, int level);
static fOodleLZ_Compress                   OodleLZ_Compress_;
static fOodleLZ_Decompress                 OodleLZ_Decompress_;
static fOodleLZ_CompressOptions_GetDefault OodleLZ_CompressOptions_GetDefault_;
  #endif  

  #if _OODLE_EC
#include "pivco-huffman/extras/bench/bench_oodle_wrapper.h"  
  #endif
  #if _SMALLZ4
#include "smallz4/smallz4.h"
#include "smallz4/smallz4cat.c"
  #endif

  #if _SMAZ
#include "smaz/smaz.h"
  #endif

  #if _SNAPPY_C
#include "snappy-c/snappy.h"
struct snappy_env env;
  #endif

  #if _UNISHOX2
int unishox2_compressx(  const char *in, int inlen, char *out, int lev);
int unishox2_decompressx(const char *in, int inlen, char *out, int lev);
  #endif

  #if _UNISHOX3
#include "unishox2/Unishox3_Alpha/unishox3.h"
  #endif

  #if _ZLIB_NG
#include "zlib-ng_/zconf-ng.h"
#define Z_EXTERN
#define Z_EXPORT   
Z_EXTERN Z_EXPORT const char *zlibng_version(void);
Z_EXTERN Z_EXPORT int32_t zng_compress2(uint8_t *dest, size_t *destLen, const uint8_t *source, size_t sourceLen, int32_t level);
Z_EXTERN Z_EXPORT int32_t zng_uncompress(uint8_t *dest, size_t *destLen, const uint8_t *source, size_t sourceLen);
  #endif

  #if _ZOPFLI
#include "zopfli/src/zopfli/zopfli.h"
  #endif

  #if _ZXC
#define ZXC_STATIC_DEFINE
#include "zxc/include/zxc.h"
/*static ZXC_NOINLINE ZXC_COLD int zxc_ensure_entropy_scratch(const zxc_cctx_t* RESTRICT ctx) {
    if (LIKELY(ctx->pivco_scratch != NULL)) return ZXC_OK;
    return zxc_cctx_alloc_entropy_scratch((zxc_cctx_t*)(uintptr_t)ctx);
}
static ZXC_NOINLINE ZXC_COLD int zxc_decode_lit_pivco(const zxc_cctx_t* RESTRICT ctx, const uint8_t* RESTRICT payload,  const size_t psize, const size_t required_size) {
  const int arc = zxc_ensure_entropy_scratch(ctx);
  if (UNLIKELY(arc != ZXC_OK)) return arc;
  if (UNLIKELY(ctx->lit_buffer_cap < required_size + ZXC_PAD_SIZE || ctx->pivco_scratch_cap < required_size + ZXC_PIVCO_SCRATCH_PAD))
    return ZXC_ERROR_CORRUPT_DATA;
  return zxc_huf_decode_section(payload, psize, ctx->lit_buffer, required_size,  ctx->pivco_scratch);
}*/
  #endif

  #if __cplusplus
}
  #endif
// ======================================== Encoding =====================================================
  #if _GANS
#include "EC/rans.h"
  #endif

  #if _SSERC
#include "EC/sserangecoding/sserangecoder.h"
#define SSE_BITS 12
static int ssercini;
unsigned ssercenc(unsigned char *_in, unsigned inlen, unsigned char *_out) {
  sserangecoder::uint8_vec  in(inlen), out;
  sserangecoder::uint32_vec sym_freq(256);
  memcpy(&in[0], _in, inlen);                 
  if(!ssercini) sserangecoder::vrange_init(), ssercini++;
  
  for(uint32_t i = 0; i < inlen; i++) sym_freq[_in[i]]++;
  unsigned a = 256; while(a > 1 && !sym_freq[a-1]) a--;  
  sserangecoder::uint32_vec scaled_cum_prob(a+1);
  if(!sserangecoder::vrange_create_cum_probs(scaled_cum_prob, sym_freq)) return -1; 

  unsigned char *op = _out; 
  *op++ = a - 1; 
  for(int i = 0; i < a; i++) ctou16(op) = scaled_cum_prob[i], op +=2;  
  sserangecoder::vrange_encode(in, out, scaled_cum_prob); 
  memcpy(op, &out[0], out.size()); 
  op += out.size();
  return op - _out;
}

unsigned ssercdec(unsigned char *in, unsigned inlen, unsigned char *out, unsigned outlen) { 
  unsigned char *ip = in;
  unsigned      a = 1 + (*ip++);                                                
  if(!ssercini) sserangecoder::vrange_init(),ssercini++;

  sserangecoder::uint32_vec scaled_cum_prob(a+1);
  
  unsigned cum = 0,i;
  for(i = 0; i < a; i++) scaled_cum_prob[i] = ctou16(ip), ip+=2;    
  scaled_cum_prob[a] = (1<<SSE_BITS);   
  sserangecoder::uint32_vec dec_table(a);
  sserangecoder::vrange_init_table(a, scaled_cum_prob, dec_table);
  if(!sserangecoder::vrange_decode(ip, (in+inlen) - ip, out, outlen, &dec_table[0])) return -1;  //for(int i=0; i < 100; i++)  printf("%c", out[i]);
  return outlen;
}
  #endif
  
  #if _TURBORLE
#include "Turbo-Run-Length-Encoding/include/trle.h"
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
  
  #if _HRLE
#include "hypersonic-rle-kit/src/rle.h"
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

  #if _PIVCOHUF
#include "pivcohuf_file.h"
/*static size_t pivcoenc(char *in, size_t n, char *out, int csize, int lev) {
  char *ip = in, *op = out, *in_ = in+n, *out_ = out+n;
  size_t olen, ilen;
  for(; ip < in_; ip += ilen, op += olen) {
    ilen  = (in_ - ip); ilen = ilen > csize ? csize : ilen;    olen = ilen;  
    if(pivcohuf_compress_ex(ip, ilen, op, &olen, lev) != PIVCOHUF_OK) { op = out_; break; }  //{ op[-1] = 0xff; memcpy(op, ip, ilen); olen = ilen; }
    if(op+olen >= out_) { op = out_; break; }
  }
  return op - out;
}

static size_t pivcodec(char *in, size_t n, char *out, int csize) {
  char *ip = in, *op = out, *out_ = out+n;
  size_t olen, ilen; 
  for(; op < out_; ip += ilen, op += olen) {                                                 
    olen = (out_ - op); olen = olen > csize ? csize: olen;
    ilen = olen; 
    if(pivcohuf_decompress(ip, olen, op, &ilen) != PIVCOHUF_OK) return 0; 
  }
  return ip - in;
}*/
  #endif

  #if _PHAZ
// phaz_local.o exports only these (zstd + pivco localized inside the blob).
// Last arg is a phaz_stats* (passed 0 here); declared void* to avoid the header.
extern "C" size_t phaz_compress(const void *src, size_t n, void *dst, size_t cap, int level, void *st);
extern "C" size_t phaz_decompress(const void *src, size_t n, void *dst, size_t cap, void *st);
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

  #if __cplusplus
extern "C" {
  #endif
  #if _FSE
#include "EC/fse/fse.h"  
  #endif
  #if _FSEHUF
//#include "fse/huf.h"  
#define HUF_PUBLIC_API 
HUF_PUBLIC_API size_t HUF_compress(void* dst, size_t dstCapacity, const void* src, size_t srcSize);
HUF_PUBLIC_API size_t HUF_decompress(void* dst,  size_t originalSize, const void* cSrc, size_t cSrcSize);
  #endif
  #if __cplusplus
}
  #endif
  
//------------------------------------------------- registry -------------------------------------------------------------------------------------------------
struct plugs plugs[] = {
  { P_AOCL_LZ4,      "aocl-lz4",      _AOCL,      "AMD aocl-compression lz4",    "0,1,2,3,4,5,6,7,8,9,10,11,12" },
  { P_AOCL_LZMA,     "aocl-lzma",     _AOCL,      "AMD aocl-compression lzma",   "1,2,3,4,5,6,7,8,9" },
  { P_AOCL_BZIP2,    "aocl-bzip2",    _AOCL,      "AMD aocl-compression bzip2",  "" },
  { P_AOCL_SNAPPY,   "aocl-snappy",   _AOCL,      "AMD aocl-compression snappy", "" },
  { P_AOCL_ZLIB,     "aocl-zlib",     _AOCL,      "AMD aocl-compression zlib",   "1,2,3,4,5,6,7,8,9" },
  { P_AOCL_ZSTD,     "aocl-zstd",     _AOCL,      "AMD aocl-compression zstd",   "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-20,-30,-40,-50.-60,-70,-80,-90,-99/d#" },
  { P_BPC,           "bpc",           _BPC,       "bit plane compression",   "" },
  { P_BRIEFLZ,       "brieflz",       _BRIEFLZ,   "BriefLz",                 "1,3,6,9" },
  { P_BROTLI,        "brotli",        _BROTLI,    "Brotli",                  "0,1,2,3,4,5,6,7,8,9,10,11/d#:V"},
  { P_BZIP2,         "bzip2",         _BZIP2,     "Bzip2",                   "" },
  { P_BZIP3,         "bzip3",         _BZIP3,     "Bzip3",                   "" },
  
  { P_C_BLOSC2,      "blosc",         _C_BLOSC2,  "c-blosc2",                "0,1,2,3,4,5,6,7,8,9,100/SBDsd", 64*1024},
  { P_CHAMELEON,     "chameleon",     _CHAMELEON, "Chameleon",               "1,2" },
  { P_CSC,           "csc",           _CSC,       "CSC",                     "1,2,3,4,5" },
  { P_CLICKHOUSE,    "lz4_ch",        _CLICKHOUSE,"lz4 Clickhouse",          "1,2,3,4,5,6,7,8,9,10,11,12,-1,-2,-3,-4,-5,-6,-7,-8,-10,-20,-30,-40,-50.-60,-70,-80,-90,-99/MfsB#" },
  
  { P_DENSITY,       "density",       _DENSITY,   "Density",                 "1,2,3" },
  { P_DOBOZ,         "doboz",         _DOBOZ,     "Doboz",                   "" },  //crash on windows
  
  { P_FASTLZ,        "fastlz",        _FASTLZ,    "FastLz",                  "1,2" },
  { P_FLZMA2,        "flzma2",        _FLZMA2,    "Fast-lzma2",              "0,1,2,3,4,5,6,7,8,9,10,11/mt#" },
  
  { P_GIPFELI,       "gipfeli",       _GIPFELI,   "Gipfeli",                 "" },
  { P_GLZA,          "glza",          _GLZA,      "glza",                    "" },
  
  { P_HEATSHRINK,    "heatshrink",    _HEATSHRINK,"heatshrink",              "" },
  
  { P_ISA_L,         "igzip",         _ISA_L,     "igzip",                   "0,1,2,3" },
  
  { P_KANZI,         "kanzi",         _KANZI,     "kanzi",                   "0,1,2,3,4,5,6,7,8,9/T#" },
  
  { P_LIBBSC,        "bsc",           _LIBBSC,    "bsc",                     "0,3,4,5,6,7,8/P:t:e#"}, // Multithreading w. parameter t
  { P_LIBBSCC,       "bscqlfc",       _LIBBSC,    "bsc",                     "1,2"},
  { P_LIBDEFLATE,    "libdeflate",    _LIBDEFLATE,"libdeflate",              "1,2,3,4,5,6,7,8,9,12/dg"},
  { P_LIBLZF,        "lzf",           _LIBLZF,    "LibLZF",                  "" },
  { P_LIBLZG,        "lzg",           _LIBLZG,    "LibLzg",                  "1,2,3,4,5,6,7,8,9" }, //"https://gitorious.org/liblzg" BLOCKSIZE must be < 64MB
  { P_LIZARD,        "lizard",        _LIZARD,    "Lizard",                  "10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49" },
  { P_LZ4,           "lz4",           _LZ4,       "Lz4",                     "0,1,2,3,4,5,6,7,8,9,10,11,12,-1,-2,-3,-4,-5,-6,-7,-8,-10,-20,-30,-40,-50.-60,-70,-80,-90,-99/MfsB#" },
  { P_LZ4ULTRA,      "lz4ultra",      _LZ4ULTRA,  "Lz4ultra",                "9,10,11,12/z" },
  { P_LZAV,          "lzav",          _LZAV,      "lzav",                    "1,2" },
  { P_LZFSE,         "lzfse",         _LZFSE,     "lzfse",                   "" },  
  { P_LZFSEA,        "lzfsea",        _LZFSEA,    "lzfsea",                  "" },
  { P_LZJODY,        "lzjody",        _LZJODY,    "lzjody",                  "" },
  { P_LZHAM,         "lzham",         _LZHAM,     "Lzham",                   "1,2,3,4/t#:fb#:x#" },
  { P_LZLIB,         "lzlib",         _LZLIB,     "Lzlib",                   "1,2,3,4,5,6,7,8,9/d#:fb#" },
  { P_LZMAT,         "lzmat",         _LZMAT,     "Lzmat",                   "" },
  { P_LZMA,          "lzma",          _LZMA,      "Lzma",                    "0,1,2,3,4,5,6,7,8,9/d#:fb#:lp#:lc#:pb#:a#:mt#" },
  { P_LZO1b,         "lzo1b",         _LZO,       "Lzo",                     "1,9,99,999" },
  { P_LZO1c,         "lzo1c",         _LZO,       "Lzo",                     "1,9,99,999" },
  { P_LZO1f,         "lzo1f",         _LZO,       "Lzo",                     "1,999" },
  { P_LZO1x,         "lzo1x",         _LZO,       "Lzo",                     "1,11,12,15,999" },
  { P_LZO1y,         "lzo1y",         _LZO,       "Lzo",                     "1,999" },
  { P_LZO1z,         "lzo1z",         _LZO,       "Lzo",                     "999" },
  { P_LZO2a,         "lzo2a",         _LZO,       "Lzo",                     "999" },
  { P_LZOMA,         "lzoma",         _LZOMA,     "lzoma",                   "1,2,3,4,5,6,7,8,9" },
  { P_LZSA,          "lzsa",          _LZSA,      "lzsa",                    "9/f#cr" },
  { P_LZSSE2,        "lzsse2",        _LZSSE,     "lzsse",                   "1,2,3,4,5,6,7,8,9,12,16,17"},
  { P_LZSSE4,        "lzsse4",        _LZSSE,     "lzsse",                   "0,1,2,3,4,5,6,7,8,9,12,16,17"},
  { P_LZSSE8,        "lzsse8",        _LZSSE,     "lzsse",                   "0,1,2,3,4,5,6,7,8,9,12,16,17"},

  { P_MEMLZ,         "memlz",         _MEMLZ,     "memlz",                   "" },
  { P_MINIZ,         "miniz",         _MINIZ,     "miniz",                   "1,2,3,4,5,6,7,8,9" },
  { P_MISA77,        "misa77",        _MISA77,    "misa77",                  "0,1,2,3,4,-1" },
  { P_MISA77S,       "misa77_safe",   _MISA77,    "misa77 safe",             "0,1" },
  { P_MSCOMPRESS,    "mscompress",    _MSCOMPRESS,"ms-compress",             "2,3,4" },
 
  { P_OPENZL_U8,     "openzl_u8",     _OPENZL,    "openzl u8",               "" },
  { P_OPENZL_I8,     "openzl_i8",     _OPENZL,    "openzl i8",               "" },
  { P_OPENZL_U16,    "openzl_le_u16", _OPENZL,    "openzl u16",              "" },
  { P_OPENZL_I16,    "openzl_le_i16", _OPENZL,    "openzl i16",              "" },
  { P_OPENZL_U32,    "openzl_le_u32", _OPENZL,    "openzl u32",              "" },
  { P_OPENZL_I32,    "openzl_le_i32", _OPENZL,    "openzl i32",              "" },
  { P_OPENZL_U64,    "openzl_le_u64", _OPENZL,    "openzl u64",              "" },
  { P_OPENZL_I64,    "openzl_le_i64", _OPENZL,    "openzl i64",              "" },
  { P_OPENZL_SERIAL, "openzl_serial", _OPENZL,    "openzl serial",           "" },
  { P_OPENZL_GENERIC,"openzl_generic",_OPENZL,    "openzl generic",          "" },
  { P_OPENZL_ZSTD,   "openzl_zstd",   _OPENZL,    "openzl zstd",             "1,2,3,4,5,6,8,10,12,14,16,18,20,22,-1,-2,-3,-4,-5,-6,-7,-8,-10,-20,-30,-40,-50.-60,-70,-80,-90,-99" },
  { P_OPENZL_LZ4,    "openzl_lz4",    _OPENZL,    "openzl lz4",              "1,2,3,4,5,6,7,8,9,10,11,12,-1,-2,-3,-4,-5,-6,-7,-8,-10,-20,-30,-40,-50.-60,-70,-80,-90,-99" }, 
  { P_OPENZL_TP,     "openzl_tp",     _OPENZL,    "openzl transpose",        "2,4,8" },

  { P_PCODECI8,      "pcodec_i8",     _PCODEC,    "pcodec_i8",                "0,1,2,3,4,5,6,7,8,9" },
  { P_PCODECU8,      "pcodec_u8",     _PCODEC,    "pcodec_u8",                "0,1,2,3,4,5,6,7,8,9" },
  { P_PCODECI16,     "pcodec_i16",    _PCODEC,    "pcodec_i16",               "0,1,2,3,4,5,6,7,8,9" },
  { P_PCODECU16,     "pcodec_u16",    _PCODEC,    "pcodec_u16",               "0,1,2,3,4,5,6,7,8,9" },
  { P_PCODECF16,     "pcodec_f16",    _PCODEC,    "pcodec_f16",               "0,1,2,3,4,5,6,7,8,9" },
  { P_PCODECI32,     "pcodec_i32",    _PCODEC,    "pcodec_i32",               "0,1,2,3,4,5,6,7,8,9" },
  { P_PCODECU32,     "pcodec_u32",    _PCODEC,    "pcodec_u32",               "0,1,2,3,4,5,6,7,8,9" },
  { P_PCODECF32,     "pcodec_f32",    _PCODEC,    "pcodec_f32",               "0,1,2,3,4,5,6,7,8,9" },
  { P_PCODECI64,     "pcodec_i64",    _PCODEC,    "pcodec_i64",               "0,1,2,3,4,5,6,7,8,9" },
  { P_PCODECU64,     "pcodec_u64",    _PCODEC,    "pcodec_u64",               "0,1,2,3,4,5,6,7,8,9" },
  { P_PCODECF64,     "pcodec_f64",    _PCODEC,    "pcodec_f64",               "0,1,2,3,4,5,6,7,8,9" },
  { P_PHAZ,          "phaz",          _PHAZ,      "PivCo-Huffman/zstd",       "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22" },  // level = zstd level
  { P_PITHY,         "pithy",         _PITHY,     "Pithy",                    "0,1,2,3,4,5,6,7,8,9" }, 
  { P_PYSAP,         "sap",           _PYSAP,     "sap",                      "0,1,2" },
  
  { P_QUICKLZ,       "quicklz",       _QUICKLZ,   "Quicklz",                  "1,2,3" },
  
  { P_FIRETRAIL,          "firetrail",          _FIRETRAIL,      "firetrail",                     ""},
  { P_SHRINKER,      "shrinker",      _SHRINKER,  "Shrinker",                 "", 0, (1<<26) },
  { P_SHOCO,         "shoco",         _SHOCO,     "Shoco",                    "" },
  { P_LIBSLZ,        "slz",           _LIBSLZ,    "libslz",                   "0,1,2,3,4,5,6,7,8,9" },
  { P_SMAZ,          "smaz",          _SMAZ,      "smaz",                     "" },
  { P_SNAPPY,        "snappy",        _SNAPPY,    "Snappy",                   ""  },
  { P_SNAPPY_C,      "snappy_c",      _SNAPPY_C,  "Snappy-c",                 "" },
  { P_SMALLZ4,       "smallz4",       _SMALLZ4,   "SmalLz4",                  "6,7,8,9,10,11,12/z" },
  
  { P_TAMP,          "tamp",          _TAMP,      "tamp",                     "8,9,10,11,12,13,14,15" },
  { P_TCOBS,         "tcobs",         _TCOBS,     "tcobs",                    "" },
  { P_TORNADO,       "tornado",       _TORNADO,   "Tornado",                  "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16" },
  
  { P_UNISHOX2,      "unishox2",      _UNISHOX2,  "unishox2",                 "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16" },
  { P_UNISHOX3,      "unishox3",      _UNISHOX3,  "unishox3",                 "" },
  
  { P_WFLZ,          "wflz",          _WFLZ,      "wfLZ",                     "1,2" },
  
  { P_XPACK,         "xpack",         _XPACK,     "xpack",                    "1,2,3,4,5,6,7,8,9" },
  { P_XZ,            "xz",            _XZ,        "xz",                       "0,1,2,3,4,5,6,7,8,9/d#:fb#:lp#:lc#:pb#:a#:mt#" },
  
  { P_YALZ77,        "yalz77",        _YALZ77,    "Yalz77",                   "1,6,12" },
  { P_YAPPY,         "yappy",         _YAPPY,     "Yappy",                    "" },//crash windows

  
  { P_ZLIB,          "zlib",          _ZLIB,      "zlib",                     "1,2,3,4,5,6,7,8,9" },
  { P_ZLIB_NG,       "zlib_ng",       _ZLIB_NG,   "zlib-ng",                  "1,2,3,4,5,6,7,8,9" },
  { P_ZLING,         "zling",         _ZLING,     "Libzling",                 "0,1,2,3,4" },
  { P_ZOPFLI,        "zopfli",        _ZOPFLI,    "zopfli",                   ""},
  { P_ZSTD,          "zstd",          _ZSTD,      "zstd",                     "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-20,-30,-40,-50.-60,-70,-80,-90,-99/d#" },
  { P_ZXC,           "zxc",           _ZXC,       "zxc",                      "1,2,3,4,5,6,7" },
  { P_ZPAQ,          "zpaq",          _ZPAQ,      "libzpaq",                  "0,1,2,3,4,5" }, // multithreading with lev 2 or 3

//------------------------------------------------------------------
  { P_MCPY,          "imemcpy",     _MEMCPY,    "inline memcpy",              "" },
  { P_LMCPY,         "memcpy",      _MEMCPY,    "library memcpy",             "" },
  { P_AOM,           "AOM",         _AOM,       "AV1 Entropy coder",          ""},
  { P_DAALA,         "Daala",       _DAALA,     "DAALA Entropy Coder",        ""},
  { P_FPC,           "fpc",         _FPC,       "Fast Prefix Coder",          "0,8,9,10,11,12,16,32,48,63" },
  { P_FREQTAB,       "freqtab",     _FREQTAB,   "FreqTable v2.E. shelwien",   "" },
  { P_FSC,           "fsc",         _FSC,       "Finite State Coder",         "", E_ANS },
  { P_FSE,           "fse",         _FSE,       "Finite State Entropy",       "", E_ANS },
  { P_FSEH,          "fsehuf",      _FSEHUF,    "Zstd Huffman Coding",        "", E_HUF },
  { P_FPAQC,         "fpaqc",       _FPAQC,     "Asymmetric Binary Coder",    "" },
  { P_SHRC,          "fpaq0p_sh",   _SHRC,      "Bitwise RC",                 "" },
  { P_SHRCV,         "vecrc_sh",    _VECRC,     "Bitwise vector RC",          "" },
  { P_FASTHF,        "FastHF",      _FASTHF,    "Fast HF",                    "" },
  { P_FASTARI,       "FastAri",     _FASTARI,   "FastAri",                    "" },
  { P_FASTAC,        "FastAC",      _FASTAC,    "Fast AC",                    "" },
  { P_GANSR,         "rygrans",     _GANS,      "Ryg rANS",                   "", E_ANS },
  { P_GANSW,         "rygranssse",  _GANS,      "Ryg rANS",                   "", E_ANS },
  { P_JAC,           "arith_static",_JAC,       "Range Coder/J.Bonfield",     "", E_ANS},
  { P_FQZ0,          "fqz0",        _FQZ0,      "FQZ/PPMD Range Coder",       ""},
  { P_MARLIN,        "Marlin",      _MARLIN,    "Marlin Entropy coder",       ""},
  { P_NIBRANS,       "nibrans",     _NIBRANS,   "nibrans",                    ""},
  { P_OODLE,         "oodle",       _OODLE,     "Oodle 8:Kraken 9:Mermaid 11:Selkie 12:Hydra 13:Leviathan", "01,02,03,04,05,06,07,08,09,11,12,13,14,15,16,17,18,19,21,22,23,24,25,26,27,28,29,41,42,43,44,45,46,47,48,49,51,52,53,54,55,56,57,58,59,61,62,63,64,65,66,67,68,69,71,72,73,74,75,76,77,78,79,81,82,83,84,85,86,87,88,89,-81,-82,-83,91,92,93,94,95,96,97,98,99,-91,-92,-93,101,102,103,104,105,106,107,108,109,111,112,113,114,115,116,117,118,119,-111,-112,-113,121,122,123,124,125,126,127,128,129,131,132,133,134,135,136,137,138,139" },
  { P_OODLE_HUF,     "oodle_huf",   _OODLE_EC,  "Oodle Huffman/tans",         "", E_HUF },
  { P_OODLE_ANS,     "oodle_tans",  _OODLE_EC,  "Oodle tans",                 "", E_ANS },
  { P_PIVCOHUF,      "pivco",       _PIVCOHUF,  "PivCo-Huffman",              "0,1", E_HUF },  // 0=PH, 1=PHA (ANS-gated bitmaps)
  { P_POLHF,         "polar",       _POLHF,     "Polar Codes",                "" },
  { P_PPMDEC,        "ppmdec",      _PPMDEC,    "PPMD Range Coder",           ""},
  { P_RECIPARITH,   "recip_arith", _RECIPARITH,"recip arith",                 "" },
  { P_SSERC,        "sserc",       _SSERC,     "sserangecoder",               "", E_ANS },
  { P_SUBOTIN,      "subotin",     _SUBOTIN,   "subotin RC",                  "" },
  { P_TORNADOHF,    "tornado_huff",_TORNADO,   "Tornado Huf",                 "" },
  { P_TURBORC,      "TurboRC",     _TURBORC,   "Turbo Range Coder",           "1,2,3,4,5,6,7,8,9,10,11,12,13,14,17,20,56,100,101/e#s" }, 
  { P_ZLIBH,        "zlibh",       _ZLIB,      "zlib Huffmann",               "8,9,10,11,12,13,14,15,16,32" },
  { P_ZRLE,         "zlibrle",     _ZLIB,      "zlib rle",                    "" },

  //---- Encoding ------
  { P_RLES,         "srle",        _TURBORLE,  "TurboRLE ESC",                "0,8,16,32,64" },
  { P_RLET,         "trle",        _TURBORLE,  "TurboRLE",                    "" },
  { P_RLEM,         "mrle",        _MRLE,      "Mespostine RLE",              "" },

  { P_HRLELE,       "hrlele",      _HRLE,      "Hypersonic-RLE rle8_low_entropy", "8" },
  { P_HRLEM,        "hrlem",       _HRLE,      "Hypersonic-RLE rle8m",            "8/S#:subsection" },
  { P_HRLE,         "hrle",        _HRLE,      "Hypersonic-RLE rle8_single/rle8_multi", "8,16,24,32,48,64/s:single" },
  { P_HRLESH,       "hrlesh",      _HRLE,      "Hypersonic-RLE rle8_sh",          "8" },
  { P_HRLEMMTF,     "hrlemmtf",    _HRLE,      "Hypersonic-RLE rle8_mmtf128",     "128" },
  /*{ P_HRLEXMMTF,    "hrlexmmtf",   _HRLE,      "8 bit RLE",               "128,256" },
  { P_HRLEU,        "hrleu",       _HRLE,      "8 bit RLE",               "8/s:single)" },*/

  //----- Transform -----
  { P_DIVBWT,       "divbwt",      _DIVBWT,    "bwt libdivsufsort","" },
  { P_LIBBSCBWT,    "bscbwt",      _LIBBSC,    "bwt libbsc",              "" },
  { P_ST,           "st",          _LIBBSC,    "st  libbsc",              "3,4,5,6,7,8" },
  { P_BRC,          "brc",         _BRC,       "Behemoth-Rank-Coding",    "0,1" },
//{ P_MYCODEC,   "mycodec",     _MYCODEC,  "0",        "My codec",             "           ",      "",        "" },   // Example for your codec
    #ifdef _LZ
  #include "../lz/x/beplugr.h"
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

NOINLINE void libmemcpy(unsigned char *dst, unsigned char *src, size_t len) {
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
      #if _AOCL
    case P_AOCL_LZ4: case P_AOCL_LZ4HC: case P_AOCL_LZMA: case P_AOCL_BZIP2: case P_AOCL_SNAPPY: case P_AOCL_ZLIB: case P_AOCL_ZSTD: 
      { char *q;  memset(&aocl, 0, sizeof(aocl));  aocl.inSize = insize; aocl.level = lev; aocl.numThreads = (q = strchr(prm,'t'))?atoi(q+(q[2]=='='?3:2)):1;  
        if(aocl_llc_setup(&aocl, AOCL_CODEC(codec,lev) )) die("aocl_llc_setup failed\n");
      } break;
      #endif

      #if _BRIEFLZ
    case P_BRIEFLZ: workmemsize = blz_workmem_size_level(insize, lev); break;
      #endif

      #if _CHAMELEON
    case P_CHAMELEON: workmemsize = sizeof(struct Chameleon); break;
      #endif

      #if _C_BLOSC2
    case P_C_BLOSC2: blosc2_init(); blosc2_set_nthreads(1);break;
      #endif

      #if _FASTARI
    case P_FASTARI: workmemsize = FA_WORKMEM; break;
      #endif

      #if _LIBBSC
    #define BSC_MODE LIBBSC_FEATURE_FASTMODE | (strchr(prm,'P')?LIBBSC_FEATURE_LARGEPAGES:0) | (strchr(prm,'t')?LIBBSC_FEATURE_MULTITHREADING:0)      
    case P_LIBBSC: case P_LIBBSCC: bsc_init(BSC_MODE); bsc_st_init(BSC_MODE); break;
      #endif

      #if _LIBSLZ
    case P_LIBSLZ:
      slz_make_crc_table();
      slz_prepare_dist_table();
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

      #if _MEMLZ
    case P_MEMLZ: workmemsize = sizeof(memlz_state); break;
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
    if(hdll) {
      if(!(OodleLZ_Compress_   = (fOodleLZ_Compress  )GetProcAddress(hdll, "OodleLZ_Compress"  ))) { printf("OodleLZ_Compress not found\n");   exit(-1); }
      if(!(OodleLZ_Decompress_ = (fOodleLZ_Decompress)GetProcAddress(hdll, "OodleLZ_Decompress"))) { printf("OodleLZ_Decompress not found\n"); exit(-1); }
      if(!(OodleLZ_CompressOptions_GetDefault_ = (fOodleLZ_CompressOptions_GetDefault)GetProcAddress(hdll, "OodleLZ_CompressOptions_GetDefault"))) { printf("OodleLZ_CompressOptions_GetDefault not found\n"); exit(-1); }
        } else fprintf(stderr,"oo2core_9_win64.dll not found\n");
      } 
        #else
      {
          #ifdef __aarch64__
            #ifdef __APPLE__
        strcpy(oodle, "./liboo2coremac64.2.9.16.dylib");
            #else
        strcpy(oodle, "./liboo2corelinuxarm64.so.9");
            #endif
          #else
        strcpy(oodle, "./liboo2corelinux64.so.9");
          #endif
        void *hdll = dlopen(oodle, RTLD_LAZY);
        if(hdll) { 
          if(!(OodleLZ_Compress_   = (fOodleLZ_Compress  )dlsym(hdll, "OodleLZ_Compress"  ))) 
            die("OodleLZ_Compress not found\n");
      if(!(OodleLZ_Decompress_ = (fOodleLZ_Decompress)dlsym(hdll, "OodleLZ_Decompress"))) 
        die("OodleLZ_Decompress not found\n");
      if(!(OodleLZ_CompressOptions_GetDefault_ = (fOodleLZ_CompressOptions_GetDefault)dlsym(hdll, "OodleLZ_CompressOptions_GetDefault")))  
        die("OodleLZ_CompressOptions_GetDefault not found\n");
        } else fprintf(stderr,"oodle shared library '%s' not found.'%s'\n", oodle, dlerror());   
      }
      #endif 
      break;
      #endif

      #if _PCODEC
    case P_PCODECI8:
    case P_PCODECU8:
    case P_PCODECI16:
    case P_PCODECU16:
    case P_PCODECF16:
    case P_PCODECI32:
    case P_PCODECU32:
    case P_PCODECF32:
    case P_PCODECI64:
    case P_PCODECU64:
    case P_PCODECF64: pco_ini(); break;  
      #endif
        
      #if _QUICKLZ
    case P_QUICKLZ:
      state_size  = max(qlz_get_setting1(1), max(qlz_get_setting2(1), qlz_get_setting3(1))); workmemsize = max(state_size, workmemsize);
      dstate_size = max(qlz_get_setting1(2), max(qlz_get_setting2(2), qlz_get_setting3(2))); workmemsize = max(dstate_size, workmemsize);
      break;
      #endif

      #if _FIRETRAIL
    firetrail_encoder = firetrail_encoder_create();
    firetrail_decoder = firetrail_decoder_create();
      #endif

      #if _SNAPPY_C
    case P_SNAPPY_C: snappy_init_env(&env); break;
      #endif

      #if _TAMP
    case P_TAMP: workmemsize = 1 << lev; break;
      #endif

      #if _SSERC
    case P_SSERC: if(!ssercini) sserangecoder::vrange_init(),ssercini++; break;
      #endif
      
      #if _WFLZ
    case P_WFLZ: workmemsize = wfLZ_GetWorkMemSize(); break;
      #endif

      #if _YAPPY
    case P_YAPPY: YappyFillTables(); break;
      #endif

      #ifdef _LZ
    #include "../lz/x/beplug0.h"
      #endif
       #if _FSEHUF
//    case P_FSEH: workmemsize = max(4096*sizeof(unsigned), workmemsize); break;
      #endif
 }
  if(!workmemsize) return 0;
  if(workmemsize > sizeof(_workmem) && !(workmem = (char *)calloc(1,workmemsize)) ) {
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

void codexit(int codec, int lev) {
  if(workmem != _workmem) {
      #if _MARLIN
    if(codec == P_MARLIN) Marlin_free_dictionary((Marlin *)workmem);
    else
      #endif
    free(workmem/*, workmemsize*/);
    workmem = _workmem;
    return;
  } 
  switch(codec) {
      #if _AOCL
    case P_AOCL_LZ4: case P_AOCL_LZ4HC: case P_AOCL_LZMA: case P_AOCL_BZIP2: case P_AOCL_SNAPPY: case P_AOCL_ZLIB: case P_AOCL_ZSTD: aocl_llc_destroy(&aocl, AOCL_CODEC(codec,lev) ); break;
      #endif
  
      #if _SNAPPY_C
    case P_SNAPPY_C: snappy_free_env(&env);
      #endif
      #if _FIRETRAIL
    case P_FIRETRAIL: {
      if(firetrail_encoder) firetrail_encoder_destroy(firetrail_encoder); firetrail_encoder = NULL;
      if(firetrail_decoder) firetrail_decoder_destroy(firetrail_decoder); firetrail_decoder = NULL;
    }  
      #endif    
  }
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

unsigned codcomp(unsigned char *in, unsigned inlen, unsigned char *out, unsigned outsize, int codec, int lev, char *prm) { unsigned outlen; unsigned char *oend=out+outsize; //printf("#(%d), inlen=%d,outsize=%d\n", codec, inlen, outsize);fflush(stdout);
  char     *q        = strchr(prm,'d');
  unsigned dsize     = q?argtoi(q+(q[1]=='='?2:1),0):dicsize; 
  int      threadnum = (q = strchr(prm,'t'))?atoi(q+(q[2]=='='?3:2)):1;
  
  switch(codec) {
      #if _AOCL
    case P_AOCL_LZ4: case P_AOCL_LZ4HC: case P_AOCL_LZMA: case P_AOCL_BZIP2: case P_AOCL_SNAPPY: case P_AOCL_ZLIB: case P_AOCL_ZSTD: {
      aocl.inBuf = (char *)in; aocl.inSize = inlen; aocl.outBuf = (char *)out; aocl.outSize = inlen; aocl.level = lev; return aocl_llc_compress(&aocl, AOCL_CODEC(codec,lev));  
    }
      #endif

      #if _BALZ
    case P_BALZ: return balzcompress(in, inlen, out,lev);
      #endif

      #if _BCM
    case P_BCM: return bcmcompress(in, inlen, out); break;
      #endif

      #if _C_BLOSC2
    case P_C_BLOSC2: { char *q;
      //  #if _C_BLOSC2LZ
      //return blosclz_compress(lev, in, inlen, out, outsize);
      //  #else
      blosc2_set_nthreads(threadnum);
      int codid = ICC_ZSTD;
      if((q=strchr(prm,'E')) && strcasecmp(q+(q[1]=='='?2:1), "lz4")) codid = ICC_LZ4;
      int filter0 = strchr(prm,'B')?BLOSC_BITSHUFFLE : strchr(prm,'S')?BLOSC_SHUFFLE : strchr(prm,'D')?BLOSC_FILTER_BYTEDELTA : 0;
      int filter1 = strchr(prm,'d')?BLOSC_DELTA : strchr(prm,'b')?BLOSC_FILTER_BYTEDELTA : strchr(prm,'s')?BLOSC_SHUFFLE:0;
      int typesize = (q=strchr(prm,'u'))?atoi(q+(q[1]=='='?2:1)):1;
      if(lev == 100) return (filter0 & BLOSC_BITSHUFFLE) ? blosc2_bitshuffle(typesize, inlen, in, out) : blosc2_shuffle(typesize, inlen, in, out);
      return blosccomp(in, inlen, out, outsize, codid, lev, typesize, filter0, filter1, 0);
    }
      #endif

      #if _BPC
    case P_BPC: {
      unsigned char *ip, *op=out;
      for(ip = in, in += inlen; ip < in; ) {
        unsigned iplen = in - ip; iplen = iplen>_MAX_BYTES_PER_LINE?_MAX_BYTES_PER_LINE:iplen;
        op += BPCompressor::compressLine((CACHELINE_DATA*)ip, 0);
        ip += iplen;
      }
      return op - out;
    }
      #endif
      
      #if _BRIEFLZ
    case P_BRIEFLZ: return blz_pack_level(in, out, inlen, workmem, lev);
      #endif

      #if _BROTLI
    case P_BROTLI: { 
      unsigned lgwin = BROTLI_DEFAULT_WINDOW, mode = BROTLI_DEFAULT_MODE; size_t esize = outsize;
      if(q = strchr(prm,'w'))              lgwin = atoi(q+(q[1]=='='?2:1));     // window specified by local parameter w
      else if(dsize)                       lgwin = bsr32(dsize)-powof2(dsize);  // window specified by global option -d
      else if(lev < 10 || strchr(prm,'W')) lgwin = BROTLI_DEFAULT_WINDOW;       // set default=24 for lev<10
      else                               { lgwin = bsr32(inlen)-powof2(inlen); lgwin = min(lgwin,BROTLI_LARGE_MAX_WINDOW_BITS); }// set large window brotli
      if(q = strchr(prm,'m')) mode = atoi(q+(q[1]=='='?2:1));         
                                                                            // Only for modified brotli by powturbo -------------------------------------
                                                                            brotlidic = brotlictx = brotlirep = 0;
                                                                            if(strchr(prm,'V'))      brotlidic = 1; // Disable builtin dictionary
                                                                            if(strchr(prm,'r'))      brotlirep = 1; // Disable extended reps -1,1,-2,2,-3,3
                                                                            else if(strchr(prm,'R')) brotlirep = 2; // disable all reps
                                                                            if(strchr(prm,'x'))      brotlictx = 1; // disable order-2 lit context
                                                                            if(strchr(prm,'X'))      brotlictx = 2; // disable all lit contexts
                                                                            //-----------------------------------------------------------------------
                                                                           //printf("BROTLI lev=%d lgwin=%d mode=%d l=%d,%d \n", lev, lgwin, mode, inlen, outsize);                                                                         
      int rc = BrotliEncoderCompress(lev, lgwin, (BrotliEncoderMode)mode, (size_t)inlen, (uint8_t*)in, &esize, (uint8_t*)out); // printf("rc=%d ", rc);
      return rc?esize:0;
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
        //BZIP3_API int bz3_compress_mt(u32 block_size, const u8 * const in, u8 * out, size_t in_size, size_t * out_size, int threads);
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


      #if _ISA_L
    case P_ISA_L: struct isal_zstream s;
      isal_deflate_stateless_init(&s);
      static unsigned bs_default[] = { ISAL_DEF_LVL0_DEFAULT,     ISAL_DEF_LVL1_DEFAULT,     ISAL_DEF_LVL2_DEFAULT,     ISAL_DEF_LVL3_DEFAULT };
      static unsigned bs_xlarge[]  = { ISAL_DEF_LVL0_EXTRA_LARGE, ISAL_DEF_LVL1_EXTRA_LARGE, ISAL_DEF_LVL2_EXTRA_LARGE, ISAL_DEF_LVL3_EXTRA_LARGE };
      s.level_buf_size = strchr(prm,'x')?bs_xlarge[lev]:bs_default[lev];
      if(lev && !(s.level_buf = (uint8_t *)malloc(s.level_buf_size))) die("igzip:malloc error\n");
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

      #if _KANZI
    case P_KANZI: return kanzi_compress((char *)in, inlen, (char *)out, outsize, threadnum, lev);
      #endif
      
      #if _LIBBSC
    #define BSC_MODE LIBBSC_FEATURE_FASTMODE | (strchr(prm,'P')?LIBBSC_FEATURE_LARGEPAGES:0) | (strchr(prm,'t')?LIBBSC_FEATURE_MULTITHREADING:0)      
    case P_LIBBSC: { int ec = (q=strchr(prm,'e'))?atoi(q+(q[1]=='='?2:1)):1; ec = ec==0?3:(ec>3?3:ec);  
      return bsc_compress(      in, out, inlen,strchr(prm,'p')?0:15,strchr(prm,'p')?0:128, lev<3?1:lev, ec, BSC_MODE);
      //int bsc_compress_mt(const unsigned char * input, unsigned char * output, int n, int lzpHashSize, int lzpMinLen, int blockSorter, int coder, int features, int threads);
    }
    case P_LIBBSCC: return bsc_coder_compress(in, out, inlen, lev, BSC_MODE);
    case P_LIBBSCBWT: { int bwtidx; memcpy(out+sizeof(bwtidx), in, inlen); bwtidx = bsc_bwt_encode(out+sizeof(bwtidx), inlen, 0, NULL, 0); *(unsigned *)out = bwtidx; return inlen+4; }
    case P_ST: { memcpy(out+4,in, inlen); *(unsigned *)(out) = bsc_st_encode(out+4, inlen, lev, 0); return inlen+4; }
      #endif

      #if _LIBDEFLATE
    case P_LIBDEFLATE:  {
       struct libdeflate_compressor *dc = libdeflate_alloc_compressor(lev);
            if(strchr(prm,'d')) outlen = libdeflate_deflate_compress(dc,in, inlen,out, outsize);
       else if(strchr(prm,'g')) outlen = libdeflate_gzip_compress(   dc,in, inlen,out, outsize);
       else                     outlen = libdeflate_zlib_compress(  dc,in, inlen,out, outsize);
       libdeflate_free_compressor(dc); 
       return outlen;
      }
      #endif

      #if _LIBLZF
    case P_LIBLZF:    return lzf_compress(in, inlen, out, outsize);
      #endif

      #if _ZPAQ
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
        #if _CLICKHOUSE
    case P_CLICKHOUSE:
        #endif
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

      #if _LZAV
    case P_LZAV: 
          return (lev == 1)?lzav_compress_default((char *)in, (char *)out, inlen, outsize):lzav_compress_hi((char *)in, (char *)out, inlen, outsize);
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
                               p.m_max_helper_threads                = threadnum; 
        if(q=strstr(prm,"fb")) p.m_fast_bytes                        = atoi(q+(q[2]=='='?3:2));
        if(q=strchr(prm,'x'))  { unsigned x = atoi(q+(q[1]=='='?2:1)); p.m_extreme_parsing_max_best_arrivals = x<4?4:x; }
                               p.m_table_update_rate                 = LZHAM_DEFAULT_TABLE_UPDATE_RATE;
        size_t outlen        = outsize;
        lzham_uint32 adler32 = 0;
        lzham_compress_status_t rc = lzham_compress_memory(&p, (lzham_uint8*)out+1, &outlen, (const lzham_uint8 *)in, inlen, &adler32);
        return rc == LZHAM_COMP_STATUS_SUCCESS?outlen:-rc;
    }
      #endif

      #if _LZJODY
    case P_LZJODY : return lzjody_compress(in, out, 0, inlen);
      #endif

      #if _LIZARD
    case P_LIZARD: return Lizard_compress((const char*)in, (char*)out, inlen, outsize, lev);
      #endif

      #if _FLZMA2
    case P_FLZMA2: return FL2_compressMt(out, outsize, in, inlen, lev, threadnum);
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
                             p.numThreads = (q=strstr(prm,"mt"))?atoi(q+(q[2]=='='?3:2)):threadnum;
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

      #if _MEMLZ
    case P_MEMLZ: memlz_reset((memlz_state*)workmem); return memlz_stream_compress(out, in, inlen, (memlz_state*)workmem);
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

      #if _MISA77
    case P_MISA77:
    case P_MISA77S: return misa77::compress(in, inlen, out, outsize, misa77::config(lev));
      #endif

      #if _OPENZL
    case P_OPENZL_U8:     { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint8_t >(inlen, lev, windowLog); int64_t rc = _openzl_compress((char *)in, inlen, (char *)out, outsize, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_I8:     { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint8_t >(inlen, lev, windowLog); int64_t rc = _openzl_compress((char *)in, inlen, (char *)out, outsize, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_U16:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint16_t>(inlen, lev, windowLog); int64_t rc = _openzl_compress((char *)in, inlen, (char *)out, outsize, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_I16:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint16_t>(inlen, lev, windowLog); int64_t rc = _openzl_compress((char *)in, inlen, (char *)out, outsize, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_U32:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint32_t>(inlen, lev, windowLog); int64_t rc = _openzl_compress((char *)in, inlen, (char *)out, outsize, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_I32:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint32_t>(inlen, lev, windowLog); int64_t rc = _openzl_compress((char *)in, inlen, (char *)out, outsize, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_U64:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint64_t>(inlen, lev, windowLog); int64_t rc = _openzl_compress((char *)in, inlen, (char *)out, outsize, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_I64:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint64_t>(inlen, lev, windowLog); int64_t rc = _openzl_compress((char *)in, inlen, (char *)out, outsize, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_SERIAL: { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_serial(             inlen, lev, windowLog); int64_t rc = _openzl_compress((char *)in, inlen, (char *)out, outsize, p); _openzl_deinit(p); return rc;} break; 
    case P_OPENZL_GENERIC:{ char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_generic(            inlen, lev, windowLog); int64_t rc = _openzl_compress((char *)in, inlen, (char *)out, outsize, p); _openzl_deinit(p); return rc;} break;
    case P_OPENZL_ZSTD:   { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_zstd(               inlen, lev, windowLog); int64_t rc = _openzl_compress((char *)in, inlen, (char *)out, outsize, p); _openzl_deinit(p); return rc;} break;
    case P_OPENZL_LZ4:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_lz4(                inlen, lev, windowLog); int64_t rc = _openzl_compress((char *)in, inlen, (char *)out, outsize, p); _openzl_deinit(p); return rc;} break;
    case P_OPENZL_TP :    ZS_transposeEncode(out, in, inlen / lev, lev);  return inlen;  
      #endif

      #if _OODLE
    case P_OODLE: {
      int nodll = strchr(prm,'c')?1:0, level = abs(lev), comp = level/10; level = (level>99?level-100:level)%10; if(lev<0) level = -level;    
      if(!nodll) {
    OodleLZ_CompressOptions copts = *OodleLZ_CompressOptions_GetDefault_(comp, level);
        return OodleLZ_Compress_?OodleLZ_Compress_(comp, in, inlen, out, level, &copts, 0, 0, 0, 0):0;
      } 
        #if _OODLESRC
      else {
     OodleLZ_CompressOptions copts = *OodleLZ_CompressOptions_GetDefault(comp, level);
        return OodleLZ_Compress(comp, in, inlen, out, level, &copts, 0, 0, 0, 0);
      }
    #endif
    }
      #endif

      #if _OODLE_EC
    case P_OODLE_HUF: {int  huff_type = 0;/*strchr(prm,'t')?3:6;*/ unsigned rc = oodle_huff_encode((const unsigned char *)in, inlen, out+1, outsize, &huff_type); out[0]=huff_type; return rc+1; } 
    case P_OODLE_ANS: return oodle_tans_encode((const unsigned char *)in, inlen, out, outsize);
      #endif     

      #if _PCODEC
    case P_PCODECI8:  if(pco_compress) { size_t w=0; struct PcoChunkConfig config; memset(&config,0, sizeof(config)); config.compression_level = lev; pco_compress(in, inlen,   PCO_TYPE_I8,  &config, out, outsize, &w); return w; } break;
    case P_PCODECU8:  if(pco_compress) { size_t w=0; struct PcoChunkConfig config; memset(&config,0, sizeof(config)); config.compression_level = lev; pco_compress(in, inlen,   PCO_TYPE_U8,  &config, out, outsize, &w); return w; } break;
    
    case P_PCODECI16: if(pco_compress) { size_t w=0; struct PcoChunkConfig config; memset(&config,0, sizeof(config)); config.compression_level = lev; pco_compress(in, inlen/2, PCO_TYPE_I16, &config, out, outsize, &w); return w; } break;
    case P_PCODECU16: if(pco_compress) { size_t w=0; struct PcoChunkConfig config; memset(&config,0, sizeof(config)); config.compression_level = lev; pco_compress(in, inlen/2, PCO_TYPE_U16, &config, out, outsize, &w); return w; } break;
    case P_PCODECF16: if(pco_compress) { size_t w=0; struct PcoChunkConfig config; memset(&config,0, sizeof(config)); config.compression_level = lev; pco_compress(in, inlen/2, PCO_TYPE_F16, &config, out, outsize, &w); return w; } break;
    case P_PCODECI32: if(pco_compress) { size_t w=0; struct PcoChunkConfig config; memset(&config,0, sizeof(config)); config.compression_level = lev; pco_compress(in, inlen/4, PCO_TYPE_I32, &config, out, outsize, &w); return w; } break;
    case P_PCODECU32: if(pco_compress) { size_t w=0; struct PcoChunkConfig config; memset(&config,0, sizeof(config)); config.compression_level = lev; pco_compress(in, inlen/4, PCO_TYPE_U32, &config, out, outsize, &w); return w; } break;
    case P_PCODECF32: if(pco_compress) { size_t w=0; struct PcoChunkConfig config; memset(&config,0, sizeof(config)); config.compression_level = lev; pco_compress(in, inlen/4, PCO_TYPE_F32, &config, out, outsize, &w); return w; } break;
    case P_PCODECI64: if(pco_compress) { size_t w=0; struct PcoChunkConfig config; memset(&config,0, sizeof(config)); config.compression_level = lev; pco_compress(in, inlen/8, PCO_TYPE_I64, &config, out, outsize, &w); return w; } break;
    case P_PCODECU64: if(pco_compress) { size_t w=0; struct PcoChunkConfig config; memset(&config,0, sizeof(config)); config.compression_level = lev; pco_compress(in, inlen/8, PCO_TYPE_U64, &config, out, outsize, &w); return w; } break;
    case P_PCODECF64: if(pco_compress) { size_t w=0; struct PcoChunkConfig config; memset(&config,0, sizeof(config)); config.compression_level = lev; pco_compress(in, inlen/8, PCO_TYPE_F64, &config, out, outsize, &w); return w; } break;
      #endif

      #if _PITHY
    case P_PITHY: return pithy_Compress((const char *)in, inlen, (char *)out, outsize, lev);
      #endif

      #if _PYSAP
    case P_PYSAP: { CsObjectInt c; SAP_INT bytes_read, bytes_written; int rc = c.CsInitCompr((SAP_BYTE *)out, inlen, lev); out += CS_HEAD_SIZE; outsize -= CS_HEAD_SIZE;
        c.CsCompr(inlen, in, inlen, out, outsize, lev, &bytes_read, &bytes_written); return bytes_written+CS_HEAD_SIZE;
      }
      #endif

      #if _QUICKLZ
    case P_QUICKLZ: { memset(workmem,0,workmemsize); return lev<=1?qlz_compress1((char *)in, (char *)out, inlen, workmem):(lev<=2?qlz_compress2((char *)in, (char *)out, inlen, workmem):qlz_compress3((char *)in, (char *)out, inlen, workmem)); }
      #endif

      #if _LIBSLZ
    case P_LIBSLZ: { if(lev > 7) lev = 7;
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
      
      #if _SHOCO
    case P_SHOCO:    return shoco_compress((const char *)in, inlen, (char *)out, outsize);
      #endif

      #if _SMAZ
    case P_SMAZ:     return smaz_compress((const char *)in, inlen, (char *)out, outsize);
      #endif

      #if _SHRINKER
    case P_SHRINKER: return shrinker_compress((char *)in, (char *)out, inlen);
      #endif

      #if _FIRETRAIL
    case P_FIRETRAIL:  firetrail_encoder_reset(firetrail_encoder);  return firetrail_encoder_compress(firetrail_encoder, (const uint8_t*)in, inlen, (uint8_t*)out);
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

      #if _TAMP
    case P_TAMP: { int64_t rc = 0;
      TampConf conf = {
         .window  = (uint16_t)lev, // Describes the size of the decompression buffer in bits. A 10-bit window represents a 1024-byte buffer. Must be in range [8, 15], representing [256, 32678] byte windows.
         .literal = 8,
         .use_custom_dictionary = false
      };
      TampCompressor compressor;
      tamp_compressor_init(&compressor, &conf, (unsigned char *)workmem);
      tamp_compressor_compress_and_flush(&compressor, (unsigned char*)out, outsize, (size_t *)&rc, (unsigned char *)in, inlen, NULL, false);
      return rc;
    } break;
      #endif

      #if _TCOBS
    case P_TCOBS:     return TCOBSEncode( out, in, inlen);
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

      #if _XZ
        #if __x86_64__
           #define DICSIZE (1<<30)
        #else
           #define DICSIZE (1<<27)
        #endif
    case P_XZ: if(q=strstr(prm,"mt")) threadnum = atoi(q+(q[2]=='='?3:2)); return _xz_compress((char *)in, inlen, (char *)out, outsize, lev, threadnum);
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
    case P_ZSTD: { ZSTD_CStream *z = ZSTD_createCStream(); if(!z) return -1;
      if(dsize) {
        unsigned windowLog = bsr32(dsize) - powof2(dsize); 
        ZSTD_CCtx_setParameter(z, ZSTD_c_enableLongDistanceMatching, 1); 
        ZSTD_CCtx_setParameter(z, ZSTD_c_windowLog, windowLog);
        ZSTD_CCtx_setParameter(z, ZSTD_c_nbWorkers, threadnum);
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

      #if _ZXC
    /* ZXC block_size must be a power of 2 in [4KB, 2MB].
     * Valid values:  4096  (4KB)    1 << 12
     *                8192  (8KB)    1 << 13
     *               16384  (16KB)   1 << 14
     *               32768  (32KB)   1 << 15
     *               65536  (64KB)   1 << 16
     *              131072  (128KB)  1 << 17
     *              262144  (256KB)  1 << 18
     *              524288  (512KB)  1 << 19  (default)
     *             1048576  (1MB)    1 << 20
     *             2097152  (2MB)    1 << 21
     * Set to 0 to use the default (512KB). */
    case P_ZXC: { zxc_compress_opts_t opts = {0}; opts.level = lev; opts.n_threads = threadnum; if(q = strchr(prm,'B')) opts.block_size = 1 << atoi(q+(q[1] == '='?2:1)); 
      zxc_cctx *zxc_cctx = zxc_create_cctx(&opts);
      int64_t rc = zxc_compress_cctx(zxc_cctx, in, inlen, out, outsize, NULL);
      zxc_free_cctx(zxc_cctx); 
      return rc > 0?rc:0;
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
        case  8: return srlec8( (uint8_t  *)in, inlen, out, _ESC8);
        case 16: return srlec16((uint8_t *)in, inlen, out, _ESC16);
        case 32: return srlec32((uint8_t *)in, inlen, out, _ESC32);
        case 64: return srlec64((uint8_t *)in, inlen, out, _ESC64);
      } break;
    case P_RLET:  return trlec(in, inlen, out);
      #endif
      #if _MRLE
    case P_RLEM:  return mrlec(in, inlen, out);
      #endif

      #if _HRLE
    case P_HRLELE : return rle8_low_entropy_compress(in, inlen, out, outsize);
    case P_HRLEM: { char *q; int subSections = 0; if(q = strchr(prm,'S')) subSections = atoi(q+(q[1] == '='?2:1)); return rle8m_compress(subSections, in, inlen, out, outsize); }
      
    case P_HRLE:
      switch(lev) {
        case  8 : return strchr(prm,'s')?rle8_single_compress(in, inlen, out, outlen):rle8_multi_compress( in, inlen, out, outsize);
        case 16 : return rle16_sym_compress(in, inlen, out, outsize);
        case 24 : return rle24_sym_compress(in, inlen, out, outsize);
        case 32 : return rle32_sym_compress(in, inlen, out, outsize);
        case 48 : return rle48_sym_compress(in, inlen, out, outsize);
        case 64 : return rle64_sym_compress(in, inlen, out, outsize);
      }
      break;
      
    case P_HRLESH: return rle8_sh_compress(in, inlen, out, outsize);
    case P_HRLEMMTF:
      switch(lev) {
        case 128: return rle8_mmtf128_compress(in, inlen, out, outsize);
        //case 256: return rle8_mmtf256_compress(in, inlen, out, outsize);
      }
      break;
    /*case P_HRLEXMMTF:
      switch(lev) {
        case 128: return rle8_extreme_mmtf128_compress(in, inlen, out, outsize);
        case 256: return rle8_extreme_mmtf128_compress(in, inlen, out, outsize); // no avx2
      }
      break;
    case P_HRLEU: return strchr(prm,'s')?rle8_low_entropy_compress_only_max_frequency(in, inlen, out, outsize):rle8_ultra_compress(in, inlen, out, outsize);*/
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
    case P_FQZ0:  { unsigned outlen; compress_block(in, inlen, out, &outlen); return outlen; }
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

      #if _GANS
        #ifdef __x86_64__
    case P_GANSR:   return ransrcompress(in, inlen, out);
    case P_GANSW:   return ranswcompress(in, inlen, out);
        #endif
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

      #if _SSERC
    case P_SSERC: return ssercenc(in, inlen, out);
      #endif

      #if _SUBOTIN
    case P_SUBOTIN:     return subenc(in, inlen, out, inlen, SYMNUM);
      #endif

      #if _TORNADOHF
    case P_TORNADOHF:     return torhenc(in, inlen, out, outsize);
      #endif

      #if _TURBORC
    case P_TURBORC: { //int ec = 0; 
      char *q;
      unsigned bwtlev = 9, xprep8=0, forcelzp=0, verbose=0, xsort=0, itmax=0, lenmin=1, nutf8=0, z=0;
      if(q = strchr(prm,'e')) bwtlev = atoi(q+(q[1]=='='?2:1));  
      if(q = strchr(prm,'m')) lenmin = atoi(q+(q[1]=='='?2:1));  
      if(q = strchr(prm,'U')) nutf8  = 1;
      if(q = strchr(prm,'s')) z = 2; else if(q = strchr(prm,'u')) z = 4;

      #define bwtflag(z) (z==2?BWT_BWT16:0) | (xprep8?BWT_PREP8:0) | forcelzp | (verbose?BWT_VERBOSE:0) | (nutf8?BWT_NUTF8:0) | xsort <<14 | itmax <<10 | lenmin
      switch(lev) {
        case  1: return rcsenc(    in, inlen, out);
        case  2: return rccsenc(   in, inlen, out); 
        case  3: return rcc2senc(  in, inlen, out);
        case  4: return rcxsenc(   in, inlen, out);
        case  5: return rcx2senc(   in, inlen, out);
        case  6: return z==2?rcsenc16(in,inlen,out)  :rcsenc32(in,inlen,out);
        case  7: return z==2?rccsenc16(in,inlen,out) :rccsenc32(in,inlen,out);
        case  8: rcc2senc32(in,inlen,out);
        case  9: return rcmsenc(    in, inlen, out);
        case 10: return rcm2senc(   in, inlen, out);
        case 11: return rcmrsenc(   in, inlen, out);
        case 12: return rcmrrsenc(  in, inlen, out);
        case 13: return z==2?rcrlesenc16( in, inlen, out):rcrlesenc(in,inlen,out);
        case 14: return z==2?rcrle1senc16(in, inlen, out):rcrle1senc(in,inlen,out);
        case 17: return rcu3senc(   in, inlen, out);
        case 20: return rcbwtenc( in, inlen, out, bwtlev, 0, bwtflag(1));
        case 56: return anscdfenc(    in, inlen, out);
        //case 100: { unsigned esize = (q=strchr(prm,'u'))?atoi(q+1):4; tpenc( in, inlen, out, esize?esize:4); return inlen; }  
        //case 101: { unsigned esize = (q=strchr(prm,'u'))?atoi(q+1):4; tp4enc(in, inlen, out, esize?esize:4); return inlen; }       
    default: return 0;
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

      #ifdef _LZ
    #include "../lz/x/beplugc.c"
      #endif

      #if _PIVCOHUF
    case P_PIVCOHUF: { size_t olen = outsize; if(pivcohuf_compress_ex(in, inlen, out, &olen, lev) != PIVCOHUF_OK) return 0; return (unsigned)olen; }
      #endif
      
      #if _PHAZ
    case P_PHAZ: return (unsigned)phaz_compress(in, inlen, out, outsize, lev, 0);  /* lev = zstd level */
      #endif
      #if _MYCODEC
//    case P_MYCODEC:   return mycomp(in, inlen, out, outsize);
      #endif

    defaulf: fprintf(stderr, "library '%d' not included\n", codec);
  }
  return 0;
} 

unsigned coddecomp(unsigned char *in, unsigned inlen, unsigned char *out, unsigned outlen, int codec, int lev, char *prm) {
  char *q;
  int  threadnum = (q = strchr(prm,'t'))?atoi(q+(q[2]=='='?3:2)):1;

  switch(codec) {
      #if _AOCL
    case P_AOCL_LZ4: case P_AOCL_LZ4HC: case P_AOCL_LZMA: case P_AOCL_BZIP2: case P_AOCL_SNAPPY: case P_AOCL_ZLIB: case P_AOCL_ZSTD: 
      aocl.inBuf = (char *)in; aocl.outBuf = (char *)out; aocl.inSize = inlen; aocl.outSize = outlen; aocl.level = lev; return aocl_llc_decompress(&aocl, AOCL_CODEC(codec,lev) );
      #endif

      #if _AOM
    case P_AOM: aomdec(in, inlen, out, outlen); return outlen;
      #endif

      #if _BALZ
    case P_BALZ: return balzdecompress(in, inlen, out, outlen);
      #endif

      #if _BCM
    case P_BCM: return bcmdecompress(in, inlen, out, outlen);
      #endif

      #if _BPC
    case P_BPC: {
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
          if(bz3_decode_block(st, op, iplen, oplen, iplen) == -1) die("bzip3 decode failed");
          op += oplen;  
          ip += 4+iplen;          
        }
        bz3_free(st);
        return op-out;
      }
      #endif

      #if _C_BLOSC2
    case P_C_BLOSC2: { //return blosc1_decompress(in, out, outlen);
      int filter0 = strchr(prm,'B')?BLOSC_BITSHUFFLE : strchr(prm,'S')?BLOSC_SHUFFLE : strchr(prm,'D')?BLOSC_FILTER_BYTEDELTA : 0;
      int typesize = (q=strchr(prm,'u'))?atoi(q+(q[1]=='='?2:1)):1;
      if(lev == 100) return (filter0 & BLOSC_BITSHUFFLE) ? blosc2_bitunshuffle(typesize, inlen, in, out) : blosc2_unshuffle(typesize, inlen, in, out);
      char *q; return bloscdecomp(in, inlen, out, outlen, typesize);
    }
      #endif

      #if _C_BLOSC2LZ
    case P_C_BLOSC2LZ: return blosclz_decompress(in, inlen, out, outlen);
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

      #if _CLICKHOUSE
    case P_CLICKHOUSE: return LZ4::decompress((const char *)in, (char * const)out, inlen, outlen);//, 0/*PerformanceStatistics & statistics*/);
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

      #if _DOBOZ
    case P_DOBOZ:   { doboz::Decompressor d; return d.decompress(in, inlen, out, outlen) == doboz::RESULT_OK ? outlen : -1; }
      #endif

      #if _FASTARI
    case P_FASTARI:  { size_t outsize = outlen; return fa_decompress((const unsigned char *)in, out, inlen, &outsize, workmem)?0:inlen; }
      #endif

      #if _FASTLZ
    case P_FASTLZ: fastlz_decompress(in, inlen, out, outlen); break;
      #endif

      #if _FLZMA2
    case P_FLZMA2: { return FL2_decompress(out, outlen,  in, inlen); }
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

      #if _ISA_L
    case P_ISA_L: { struct inflate_state s; int rc; isal_inflate_init(&s);
         /*    if(prm && *prm == 'd') { s.crc_flag = ISAL_DEFLATE; }
        else if(prm && *prm == 'g') { s.crc_flag = ISAL_GZIP; }
        else s.crc_flag = ISAL_ZLIB;*/
        s.next_in  = in;  s.avail_in  = inlen;
        s.next_out = out; s.avail_out = outlen;
        if((rc = isal_inflate_stateless(&s)) != ISAL_DECOMP_OK) die("igzip error. rc=%d\n", rc);
      } break;
      #endif

     #if _KANZI
    case P_KANZI: return kanzi_decompress((char *)in, inlen, (char *)out, outlen, threadnum);
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

      #if _LIBLZG
    case P_LIBLZG: LZG_Decode(in, inlen, out, outlen); break;
      #endif

      #if _LIBLZF
    case P_LIBLZF: lzf_decompress(in, inlen, out, outlen); break;
      #endif

      #if _LIZARD
    case P_LIZARD: return Lizard_decompress_safe((const char *)in, (char *)out, inlen, outlen);
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

      #if _LZAV
    case P_LZAV: return lzav_decompress(in, out, inlen, outlen);
      #endif

      #if _LZFSE
    case P_LZFSE : return lzfse_decode_buffer(out, outlen, in, inlen, workmem);
      #endif

      #if _LZFSEA
    case P_LZFSEA : return compression_decode_buffer(out, outlen, in, inlen, workmem, COMPRESSION_LZFSE);
      #endif

      #if _LZJODY
    case P_LZJODY : {  
      unsigned options = *in & 0xc0, l = in[1],rc; l |= ((*in & 0x1f) << 8);
      if(l > (LZJODY_BSIZE + 4)) die("lzjody decompression header error\n" );
      if (options & O_NOCOMPRESS) { memcpy(out, in, outlen); return inlen; }
      rc = lzjody_decompress(in+2, out, inlen-2, 0);
      if(rc > LZJODY_BSIZE) die("lzjody decompression error.rc=%d\n", rc ); 
      return rc; }
      #endif

      #if _MEMLZ
    case P_MEMLZ: memlz_reset((memlz_state*)workmem); return (int64_t)memlz_stream_decompress(out, in, (memlz_state*)workmem);
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
     case P_MSCOMPRESS: { size_t _outlen = outlen; return ms_decompress((MSCompFormat)lev, in, inlen, out, &_outlen)==MSCOMP_OK?inlen:0; }
      #endif

      #if _MINIZ
    case P_MINIZ: { uLongf outsize = outlen; int rc = mz_uncompress(out, &outsize, in, inlen); } break;
      #endif

      #if _MISA77
    case P_MISA77:  return misa77::decompress(in, inlen, out, outlen);
    case P_MISA77S: return misa77::decompress(in, inlen, out, outlen, misa77::dconfig(true));
      #endif
      
      #if _OODLE
    case P_OODLE: { 
      int nodll = strchr(prm,'d')?1:0;  
      if(!nodll) {
        int rc = OodleLZ_Decompress_?OodleLZ_Decompress_(in, inlen, out, outlen, 0,0,0,0,0,0,0,0,0,0):0;
        return rc;
      } 
    #if _OODLESRC
      else {
        int rc = OodleLZ_Decompress(in, inlen, out, outlen, 0,0,0,0,0,0,0,0,0,0);
        return rc;
      }
    #endif
    }
      #endif

      #if _OODLE_EC
    case P_OODLE_HUF: { /*huff_type = strchr(prm,'t')?3:6;*/ return oodle_huff_decode((const unsigned char *)(in+1), inlen-1, out, outlen, in[0]); }
    case P_OODLE_ANS: return oodle_tans_decode((const unsigned char *)in, inlen, out, outlen);
      #endif

      #if _OPENZL
    case P_OPENZL_U8:     { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint8_t >( inlen, lev, windowLog); int64_t rc = _openzl_decompress((char *)in, inlen, (char *)out, outlen, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_I8:     { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint8_t >( inlen, lev, windowLog); int64_t rc = _openzl_decompress((char *)in, inlen, (char *)out, outlen, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_U16:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint8_t >( inlen, lev, windowLog); int64_t rc = _openzl_decompress((char *)in, inlen, (char *)out, outlen, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_I16:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint8_t >( inlen, lev, windowLog); int64_t rc = _openzl_decompress((char *)in, inlen, (char *)out, outlen, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_U32:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint8_t >( inlen, lev, windowLog); int64_t rc = _openzl_decompress((char *)in, inlen, (char *)out, outlen, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_I32:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint8_t >( inlen, lev, windowLog); int64_t rc = _openzl_decompress((char *)in, inlen, (char *)out, outlen, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_U64:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint8_t >( inlen, lev, windowLog); int64_t rc = _openzl_decompress((char *)in, inlen, (char *)out, outlen, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_I64:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_integer_t<uint8_t >( inlen, lev, windowLog); int64_t rc = _openzl_decompress((char *)in, inlen, (char *)out, outlen, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_SERIAL: { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_serial(              inlen, lev, windowLog); int64_t rc = _openzl_decompress((char *)in, inlen, (char *)out, outlen, p); _openzl_deinit(p); return rc;} 
    case P_OPENZL_GENERIC:{ char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_generic(             inlen, lev, windowLog); int64_t rc = _openzl_decompress((char *)in, inlen, (char *)out, outlen, p); _openzl_deinit(p); return rc;} 
    case P_OPENZL_ZSTD:   { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_zstd(                inlen, lev, windowLog); int64_t rc = _openzl_decompress((char *)in, inlen, (char *)out, outlen, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_LZ4:    { char *q; size_t windowLog = (q=strchr(prm,'w'))?atoi(q+(q[1]=='='?2:1)):WINDOWLOG_OPENZL; openzl_params_s *p = _openzl_init_lz4(                 inlen, lev, windowLog); int64_t rc = _openzl_decompress((char *)in, inlen, (char *)out, outlen, p); _openzl_deinit(p); return rc;}
    case P_OPENZL_TP :    ZS_transposeDecode(out, in, inlen / lev, lev);  return inlen;
       #endif

      #if _PCODEC
    case P_PCODECI8:  if(pco_decompress) { size_t w=0; pco_decompress(in, inlen, PCO_TYPE_I8,  out, outlen, &w); return w; } break;
    case P_PCODECU8:  if(pco_decompress) { size_t w=0; pco_decompress(in, inlen, PCO_TYPE_U8,  out, outlen, &w); return w; } break;
    case P_PCODECI16: if(pco_decompress) { size_t w=0; pco_decompress(in, inlen, PCO_TYPE_I16, out, outlen, &w); return w; } break;
    case P_PCODECU16: if(pco_decompress) { size_t w=0; pco_decompress(in, inlen, PCO_TYPE_U16, out, outlen, &w); return w; } break;
    case P_PCODECF16: if(pco_decompress) { size_t w=0; pco_decompress(in, inlen, PCO_TYPE_F16, out, outlen, &w); return w; } break;
    case P_PCODECI32: if(pco_decompress) { size_t w=0; pco_decompress(in, inlen, PCO_TYPE_I32, out, outlen, &w); return w; } break;
    case P_PCODECU32: if(pco_decompress) { size_t w=0; pco_decompress(in, inlen, PCO_TYPE_U32, out, outlen, &w); return w; } break;
    case P_PCODECF32: if(pco_decompress) { size_t w=0; pco_decompress(in, inlen, PCO_TYPE_F32, out, outlen, &w); return w; } break;
    case P_PCODECI64: if(pco_decompress) { size_t w=0; pco_decompress(in, inlen, PCO_TYPE_I64, out, outlen, &w); return w; } break;
    case P_PCODECU64: if(pco_decompress) { size_t w=0; pco_decompress(in, inlen, PCO_TYPE_U64, out, outlen, &w); return w; } break;
    case P_PCODECF64: if(pco_decompress) { size_t w=0; pco_decompress(in, inlen, PCO_TYPE_F64, out, outlen, &w); return w; } break;
      #endif

      #if _PITHY
    case P_PITHY: return pithy_Decompress((const char *)in, inlen, (char *)out, outlen);
      #endif

      #if _PYSAP
    case P_PYSAP: { CsObjectInt d; SAP_INT bytes_read, bytes_written;
        d.CsInitDecompr((SAP_BYTE *)in); in += CS_HEAD_SIZE; d.CsDecompr( (SAP_BYTE *)in, inlen-CS_HEAD_SIZE, (SAP_BYTE *)out, outlen, lev, &bytes_read, &bytes_written );
        return inlen-CS_HEAD_SIZE;
      }
      #endif

      #if _QUICKLZ
    case P_QUICKLZ: { lev= (in[0]>>2)&3; outlen = lev<=1?qlz_decompress1((char*)in, out, workmem):(lev<=2?qlz_decompress2((char*)in, out, workmem):qlz_decompress3((char*)in, out, workmem)); } break;
      #endif

      #if _FIRETRAIL
    case P_FIRETRAIL: {
      firetrail_decoder_reset(firetrail_decoder); 
      size_t rc = firetrail_decoder_decompress(firetrail_decoder, (const uint8_t*)in, inlen, (uint8_t*)out, outlen);
      if(!rc) return 0;
      return firetrail_decoder_exact_output_length((const uint8_t*)in, inlen);
      #endif

      #if _LIBSLZ
    case P_LIBSLZ: {
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

      #if _SMALLZ4
    case P_SMALLZ4:
      if(!strchr(prm,'z')) { gip = in; giend = in+inlen; gop = out; unlz4(getbyte,sendbytes,NULL); break; }
      #endif

      #if _SMAZ
    case P_SMAZ:       smaz_decompress((const char *)in, inlen, (char *)out, outlen); return inlen;
      #endif

      #if _SHOCO
    case P_SHOCO:     shoco_decompress((const char *)in, inlen, (char *)out, outlen); return inlen;
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

      #if _TAMP
    case P_TAMP: { int64_t rc = 0;
      TampConf conf;
      TampDecompressor decompressor;
      size_t compressed_consumed_size;
      tamp_decompressor_init(&decompressor, NULL, (unsigned char *)workmem, lev);
      tamp_decompressor_decompress(&decompressor, (unsigned char *)out, outlen, (size_t *)&rc, (unsigned char *)in, inlen, NULL );
      return rc;
    } break;
      #endif
      
      #if _TCOBS
    case P_TCOBS:     return TCOBSDecode( out, outlen, in, inlen);
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

      #if _XZ
    case P_XZ: if(q=strstr(prm,"mt")) threadnum = atoi(q+(q[2]=='='?3:2)); return _xz_decompress((char *)in, inlen, (char *)out, outlen, threadnum);
      #endif

      #if _YAPPY
    case P_YAPPY:    return YappyUnCompress(in, in+inlen, out)-out;
      #endif

      #if _ZLIB
    //case P_ISA_L: case P_LIBDEFLATE:
        #if _ZLIB_NG == 0
    case P_ZOPFLI:
        #endif
    case P_ZLIB: { uLongf outsize = outlen; int rc = uncompress(out, &outsize, in, inlen); } break;
      #endif

      #if _ZLIB_NG
    case P_ZOPFLI:
    case P_ZLIB_NG: { size_t outsize = outlen; int rc = zng_uncompress(out, &outsize, in, inlen); } break;
      #endif

      #if _ZLING
    case P_ZLING: zling_decompress(in, inlen, out, outlen); break;
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
      
      #if _ZXC
    case P_ZXC: {
      zxc_decompress_opts_t opts = {0}; opts.n_threads = threadnum; 
      zxc_dctx *zxc_dctx = zxc_create_dctx(); 
      size_t rc = zxc_decompress_dctx(zxc_dctx, in, inlen, out, outlen, &opts); 
      zxc_free_dctx(zxc_dctx); 
      return rc;    
    }
      #endif

      #if _ZPAQ
    case P_ZPAQ: { zin = in; zin_ = in+inlen; zout = out; libzpaq::decompress(&zmemin, &zmemout); return zin - in; }
      #endif

      //------------ Encoding -----------------------------------------------------------------------
      #if _TURBORLE
    case P_RLES:
      switch(lev) {
        case  0: return  srled(  in, inlen, out, outlen);
        case  8: return  srled8( in, inlen, (uint8_t *)out, outlen, _ESC8);
        case 16: return  srled16(in, inlen, (uint8_t *)out, outlen, _ESC16);
        case 32: return  srled32(in, inlen, (uint8_t *)out, outlen, _ESC32);
        case 64: return  srled64(in, inlen, (uint8_t *)out, outlen, _ESC64);
      } break;
    case P_RLET: return trled(in, inlen, out, outlen);
      #endif
      #if _MRLE
    case P_RLEM: return mrled(in, out, outlen);
      #endif

      #if _HRLE
    case P_HRLELE : return rle8_low_entropy_decompress(in, inlen, out, outlen);
    case P_HRLEM: return rle8m_decompress(in, inlen, out, outlen);
    case P_HRLE:
      switch(lev) {
        case  8 : return rle8_decompress(in, inlen, out, outlen);
        case 16 : return rle16_sym_decompress(in, inlen, out, outlen);
        case 24 : return rle24_sym_decompress(in, inlen, out, outlen);
        case 32 : return rle32_sym_decompress(in, inlen, out, outlen);
        case 48 : return rle48_sym_decompress(in, inlen, out, outlen);
        case 64 : return rle64_sym_decompress(in, inlen, out, outlen);
      }
      break;
    case P_HRLESH: return rle8_sh_decompress(in, inlen, out, outlen);
    case P_HRLEMMTF:
      switch(lev) {
        case 128 : return rle8_mmtf128_decompress(in, inlen, out, outlen);
        //case 256 : return rle8_mmtf256_decompress(in, inlen, out, outlen);
      }
      break;
    /*case P_HRLEXMMTF:
      switch(lev) {
        case 128 : return rle8_mmtf128_decompress(in, inlen, out, outlen);
        case 256 : return rle8_mmtf128_decompress(in, inlen, out, outlen);
      }
      break;
    //case P_HRLEU: return rle8_ultra_decompress(in, inlen, out, outlen);*/
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

      #if _GANS
        #ifdef __x86_64__
    case P_GANSR:   ransrdecompress(in, outlen, out); return outlen;
    case P_GANSW:   ranswdecompress(in, outlen, out); return outlen;
        #endif
      #endif

      #if _SSERC
    case P_SSERC: return ssercdec(in, inlen, out, outlen);
      #endif

      #if _SUBOTIN
    case P_SUBOTIN:     subdec(in, outlen, out, outlen, SYMNUM); break;
      #endif

      #if _TORNADOHF
    case P_TORNADOHF:    torhdec(in, inlen, out, outlen); break;
      #endif

      #if _TURBORC
    case P_TURBORC: { //unsigned prm1 = 5,prm2 = 6; char *q; //if(q=strchr(prm,'r')) { prm1 = atoi(q+(q[1]=='='?2:1)); prm2 = prm1%10; prm1 = prm1/10; if(prm1>9)prm1=9;if(!prm1) prm1=1; if(prm2>9)prm2=9;if(!prm2) prm2=1; }
      unsigned bwtlev = 9,z=0;
      char *q;
      if(inlen >= outlen && lev < 100) { memcpy(out,in, outlen); return inlen; }
      if(q=strchr(prm,'e')) bwtlev = atoi(q+(q[1]=='='?2:1)); 
      if(q=strchr(prm,'s')) z = 2;
      switch(lev) {
        case  1 : return rcsdec(    in, outlen, out);
        case  2 : return rccsdec(   in, outlen, out);
        case  3 : return rcc2sdec(  in, outlen, out);
        case  4 : return rcxsdec(   in, outlen, out);
        case  5 : return rcx2sdec(   in, outlen, out);
        case  6 : return z==2?rcsdec16(  in, outlen, out):rcsdec32(  in, outlen, out);
        case  7 : return z==2?rccsdec16( in, outlen, out):rccsdec32( in, outlen, out);
        case  8 : return rcc2sdec32(in, outlen, out);
        case  9 : return rcmsdec(   in, outlen, out);
        case 10 : return rcm2sdec(  in, outlen, out);
        case 11 : return rcmrsdec(  in, outlen, out);
        case 12 : return rcmrrsdec( in, outlen, out);       
        case 13 : return z==2?rcrlesdec16(in, outlen, out):rcrlesdec(in, outlen, out);
        case 14 : return z==2?rcrle1sdec16(in, outlen, out):rcrle1sdec(in, outlen, out);
        case 17 : return rcu3sdec( in, outlen, out);        
        case 20 : return rcbwtdec( in, outlen, out, bwtlev, 0);
        case 56 : return anscdfdec(in, outlen, out);
        //case 100: { unsigned esize = (q=strchr(prm,'u'))?atoi(q+1):4; tpdec( in, outlen, out, esize?esize:4); return inlen; }  
        //case 101: { unsigned esize = (q=strchr(prm,'u'))?atoi(q+1):4; tp4dec(in, outlen, out, esize?esize:4); return inlen; }       
        default: return 0;
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

      #ifdef _LZ
    #include "../lz/x/beplugd.c"
      #endif

      #if _PIVCOHUF
    case P_PIVCOHUF: { size_t ilen = outlen; if(pivcohuf_decompress(in, inlen, out, &ilen) != PIVCOHUF_OK) return 0;  return inlen;  }
      #endif
      #if _PHAZ
    case P_PHAZ: return (unsigned)phaz_decompress(in, inlen, out, outlen, 0);
      #endif
      #if _MYCODEC
//   case P_MYCODEC:   return mydecomp(in, inlen, out, outlen);
      #endif
  }
  return NULL;
}

char *codver(int codec, char *v, char *s) {
  switch(codec) { 
      #if _AOCL
    case P_AOCL_LZ4: case P_AOCL_LZ4HC: case P_AOCL_LZMA: case P_AOCL_BZIP2: case P_AOCL_SNAPPY: case P_AOCL_ZLIB: case P_AOCL_ZSTD: return aocl_llc_version();
      #endif
      #if _BZIP2
    case P_BZIP2: return (char *)BZ2_bzlibVersion();
      #endif
      #if _BZIP3
    case P_BZIP3: return "v1.5.3";
      #endif
      #if _BRIEFLZ
    case P_BRIEFLZ:  sprintf(s,"v%d.%d.%d", BLZ_VER_MAJOR, BLZ_VER_MINOR, BLZ_VER_PATCH); break;
      #endif
      #if _BROTLI
    case P_BROTLI:   sprintf(s,"v%d.%d.%d", BROTLI_VERSION_MAJOR, BROTLI_VERSION_MINOR, BROTLI_VERSION_PATCH); break;
      #endif

      #if _C_BLOSC2
    case P_C_BLOSC2: sprintf(s, "v%s MT", BLOSC2_VERSION_STRING);
      #endif

      #if _DENSITY
    case P_DENSITY: sprintf(s,"v%d.%d.%d", density_version_major(), density_version_minor(), density_version_revision()); break;
      #endif

      #if _FLZMA2
    case P_FLZMA2 : strcpy(s, "v1.0.1 MT");
      #endif
      #if _FASTLZ
    case P_FASTLZ: return FASTLZ_VERSION_STRING;
      #endif
      #if _FPC
    case P_FPC : return "v2023.03.08";
      #endif
   
      #if _GLZA
    case P_GLZA:  return "v0.12";
      #endif

      #if _LZFSE
    case P_LZFSE:  sprintf(s, "v2017.03.07", LIBBSC_VERSION_STRING); break;
      #endif

      #if _LZHAM
    case P_LZHAM:  return "v2015.11.22 MT"; break;
      #endif

      #if _HEATSHRINK
    case P_HEATSHRINK: sprintf(s,"v%d.%d.%d", HEATSHRINK_VERSION_MAJOR, HEATSHRINK_VERSION_MINOR, HEATSHRINK_VERSION_PATCH); break;
      #endif

      #if _ISA_L
    case P_ISA_L:  sprintf(s,"intel ISA-L v%d.%d.%d", ISAL_MAJOR_VERSION, ISAL_MINOR_VERSION, ISAL_PATCH_VERSION); break;
      #endif

      #if _KANZI
    case P_KANZI:  sprintf(s,"v%d.%d.%d MT", KANZI_DECOMP_VERSION_MAJOR, KANZI_DECOMP_VERSION_MINOR, KANZI_DECOMP_VERSION_PATCH); break;
      #endif

      #if _LIBBSC
    case P_LIBBSC:  sprintf(s, "%s MT", LIBBSC_VERSION_STRING); break;
      #endif
      #if _LIBDEFLATE
    case P_LIBDEFLATE:  strcpy(s, LIBDEFLATE_VERSION_STRING); break;
      #endif   
      #if _LIBSLZ
    case P_LIBSLZ:  return "v1.3.0";
      #endif
 
      #if _ZPAQ
    case P_ZPAQ:  return "7.12 2016.04.19";
      #endif
 
      #if _LIZARD
    case P_LIZARD:     sprintf(s,"v%d.%d.%d", LIZARD_VERSION_MAJOR, LIZARD_VERSION_MINOR, LIZARD_VERSION_RELEASE); break;
      #endif
      #if _LZ4
    case P_LZ4:     sprintf(s,"v%d.%d.%d", LZ4_VERSION_MAJOR, LZ4_VERSION_MINOR, LZ4_VERSION_RELEASE); break;
      #endif
      #if _LZLIB
    case P_LZLIB:  sprintf(s,"v%d", LZ_API_VERSION); break;
      #endif
      #if _LZAV
    case P_LZAV:  strcpy(s, LZAV_VER_STR); break;
      #endif
      #if _LZMA
    case P_LZMA:  sprintf(s, "v%s MT", MY_VERSION_NUMBERS); break;
      #endif
      #if _LZSA
    case P_LZSA : return "v1.4.1"; // file lzsa.c
      #endif
      #if _LZO
     case P_LZO1b:case P_LZO1c: case P_LZO1f: case P_LZO1x: case P_LZO1y: case P_LZO1z: case P_LZO2a: strcpy(s, "v2.10 2017.03.01"); break;
      #endif

      #if _LZSSE
    case P_LZSSE2: case P_LZSSE4: case P_LZSSE8: return "v2018.10.24"; break;
      #endif

      #if _MINIZ
    case P_MINIZ : return "v11.3.2";
      #endif
      #if _MEMLZ
    case P_MEMLZ : return "v2025.12.10"; 
      #endif
      #if _MISA77
    case P_MISA77:
    case P_MISA77S:    sprintf(s,"v%d.%d.%d", MISA77_VERSION_MAJOR, MISA77_VERSION_MINOR, MISA77_VERSION_PATCH); break;
      #endif
      #if _MSCOMPRESS
    case P_MSCOMPRESS: return "v2020.01.05";
      #endif

      #if _OPENZL
    case P_OPENZL_U8:  case P_OPENZL_I8:
    case P_OPENZL_U16: case P_OPENZL_I16: 
    case P_OPENZL_U32: case P_OPENZL_I32:
    case P_OPENZL_U64: case P_OPENZL_I64:
    case P_OPENZL_SERIAL: case P_OPENZL_GENERIC:  case P_OPENZL_ZSTD:  case P_OPENZL_LZ4:   
    case P_OPENZL_TP: sprintf(s,"v%d.%d.%d", ZL_LIBRARY_VERSION_MAJOR, ZL_LIBRARY_VERSION_MINOR, ZL_LIBRARY_VERSION_PATCH); break;
      #endif

      #if _PCODEC
    case P_PCODECI8:
    case P_PCODECU8:
    case P_PCODECI16:
    case P_PCODECU16:
    case P_PCODECF16:
    case P_PCODECI32:
    case P_PCODECU32:
    case P_PCODECF32:
    case P_PCODECI64:
    case P_PCODECU64:
    case P_PCODECF64: return "v1.0.2";  
      #endif
      #if _PIVCOHUF
    case P_PIVCOHUF:  sprintf(s,"v%d.%d", PIVCOHUF_VERSION_MAJOR, PIVCOHUF_VERSION_MINOR); break;
      #endif
      #if _PHAZ
    case P_PHAZ:      sprintf(s,"v%d.%d", PIVCOHUF_VERSION_MAJOR, PIVCOHUF_VERSION_MINOR); break;
      #endif

      #if _QUICKLZ
    case P_QUICKLZ: return "v2012.01.13";
      #endif

      #if _SNAPPY
    case P_SNAPPY:  sprintf(s,"v%d.%d.%d", SNAPPY_MAJOR, SNAPPY_MINOR, SNAPPY_PATCHLEVEL); break;
      #endif
      
      #if _TAMP
    case P_TAMP:  return "v2.3.0";
      #endif
      #if _TURBORC
    case P_TURBORC: return turborc_version();
      #endif

      #if _XZ
    case P_XZ:  sprintf(s,"v%d.%d", LZMA_VERSION_MAJOR, LZMA_VERSION_MINOR, LZMA_VERSION_PATCH); break;
      #endif

      #if _ZLIB
    case P_ZLIB:  strcpy(s,zlib_version); break;
      #endif     
      #if _ZLIB_NG
    case P_ZLIB_NG:  strcpy(s,zlibng_version()); break;
      #endif
      #if _ZOPFLI
    case P_ZOPFLI:  return "v1.0.3";
      #endif
      #if _ZSTD
    case P_ZSTD:    sprintf(s,"v%d.%d.%d MT", ZSTD_VERSION_MAJOR, ZSTD_VERSION_MINOR, ZSTD_VERSION_RELEASE); break;
      #endif

      #if _ZXC
    case P_ZXC:  sprintf(s, "v%s MT", ZXC_LIB_VERSION_STR); break;
      #endif
    default:        strcpy(s,v);
  }
  return s;
}
/*
void* operator new(std::size_t size) {
    void* p = mem_malloc(size);       // or call the real operator
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete(void* p) noexcept { mem_free(p); }
// + new[], nothrow, aligned (C++17), sized-delete, etc.

#include <new>          // for std::nothrow, etc.
#include <cstddef>
#include <cstdlib>      // optional, if you want to fall back

extern "C" {
void* cpp_new(std::size_t size) { return ::operator new(size); }             // throws std::bad_alloc on failure
void* cpp_new_nothrow(std::size_t size) { return ::operator new(size, std::nothrow); }
void cpp_delete(void* p) { ::operator delete(p); }
void* cpp_new_array(std::size_t size) { return ::operator new[](size);}
void cpp_delete_array(void* p) { ::operator delete[](p); }
}
*/
