/**
    Copyright (C) powturbo 2013-2017
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
//	    TurboBench: plugins.cc - compressor plugins 
  #ifdef NCOMP1
#define COMP1 0
  #else
#define COMP1 1
  #endif

  #ifdef NCOMP2
#define COMP2 0
  #else
#define COMP2 1
  #endif

  #ifdef NECODER
#define ECODER 0
  #else
#define ECODER 1
  #endif

  #ifdef NENCOD
#define ENCOD 0
  #else
#define ENCOD 1
  #endif

  #ifdef NTRANSFORM
#define TRANSFORM 0
  #else
#define TRANSFORM 1
  #endif

  #ifdef GPL
#define GPL 1
  #else
#define GPL 0
  #endif

enum { 
#define C_MEMCPY		1
 P_LMCPY,   // must be 0
 P_MCPY,	// must be 1

   #ifdef AOM
#define C_AOM	COMP2
   #else
#define C_AOM	0
   #endif
 P_AOM,
#define C_BALZ 		COMP2		
 P_BALZ,
#define C_BCM 		COMP2	
 P_BCM,
#define C_BRIEFLZ 	COMP2	
 P_BRIEFLZ, 
#define C_BROTLI	COMP1
 P_BROTLI,    
#define C_BZIP2 	COMP2
 P_BZIP2, 
#define C_CHAMELEON COMP2    
 P_CHAMELEON,
   #ifdef BLOSC
#define C_C_BLOSC2	COMP2
   #else
#define C_C_BLOSC2	0
   #endif
 P_C_BLOSC2,
#define C_CRUSH	 	 COMP2	
 P_CRUSH, 
#define C_CSC        COMP2    
 P_CSC, 
   #ifdef DAALA
#define C_DAALA	COMP2
   #else
#define C_DAALA	0
   #endif
 P_DAALA,
#define C_DENSITY    COMP2	
 P_DENSITY, 
#define C_DOBOZ		 COMP2	 //crash
 P_DOBOZ, 
#define C_FASTLZ 	 COMP2	
 P_FASTLZ, 
#define C_FLZMA2	 0 //COMP2 	Disabled because of Compile error
 P_FLZMA2, 
#define C_GIPFELI    COMP2	 
 P_GIPFELI,
   #ifdef GLZA
#define C_GLZA       COMP2
   #else
#define C_GLZA       0
   #endif
 P_GLZA,
#define C_HEATSHRINK COMP2
 P_HEATSHRINK,

   #ifdef IGZIP
#define C_IGZIP		 COMP2
   #else
#define C_IGZIP		 0
   #endif	
 P_IGZIP,

#define C_LIBBSC     COMP2    
 P_LIBBSC,   
 P_LIBBSCC,   
#define C_LIBDEFLATE COMP1	
 P_LIBDEFLATE,
#define C_LIBLZF     COMP2 
 P_LIBLZF,  
#define C_LIBLZG	COMP2	  
 P_LIBLZG,
#define C_LIBSLZ	COMP1	  
 P_LIBSLZ,
#define C_LIBZPAQ	COMP2	
 P_LIBZPAQ, 
#define C_LZ4		COMP1 			
 P_LZ4,
#define C_LIZARD 	COMP2
 P_LIZARD,    
#define C_LZHAM		COMP2
 P_LZHAM,
#define C_LZFSE     COMP2 
 P_LZFSE,
#define C_LZFSEA    0 // apple api on Osx. disabled 
 P_LZFSEA,
#define C_LZLIB 	COMP2
 P_LZLIB, 
#define C_LZMA		COMP1 			
 P_LZMA, 
#define C_LZMAT 	GPL
 P_LZMAT,
#define C_LZO		GPL		
 P_LZO1b, P_LZO1c, P_LZO1f, P_LZO1x, P_LZO1y, P_LZO1z, P_LZO2a,
#define C_LZOMA		GPL
 P_LZOMA, 
   #ifndef NSIMD
#define C_LZSSE     COMP2
   #else
#define C_LZSSE     0
   #endif
 P_LZSSE2,
 P_LZSSE4,
 P_LZSSE8,
#define C_MARLIN   	0 //COMP2
 P_MARLIN,
#define C_MINIZ   	COMP2
 P_MINIZ,
#define C_MSCOMPRESS	GPL  
 P_MSCOMPRESS, 
#define C_NAKA	    COMP2
   #ifndef NSIMD
#define C_NAKA	    COMP2
   #else
#define C_NAKA	    0
   #endif
 P_NAKA,
#define C_PITHY   	COMP2    
 P_PITHY, 
#define C_QUICKLZ	GPL
 P_QUICKLZ, 
#define C_SHRINKER	COMP2   	
 P_SHRINKER, 
#define C_SAP		GPL
 P_SAP, 
#define C_SHOCO      COMP2    
 P_SHOCO,
  #ifdef SNAPPY
#define C_SNAPPY		COMP2
  #else		
#define C_SNAPPY		0
  #endif
 P_SNAPPY, 
#define C_SNAPPY_C	COMP2		
 P_SNAPPY_C, 
#define C_TORNADO	GPL	 
 P_TORNADO,  
#define C_WFLZ 		COMP2    		    
 P_WFLZ,
#define C_YALZ77     COMP2     
 P_YALZ77,
#define C_YAPPY      COMP2   
 P_YAPPY,
#define C_ZLIB		COMP1	
 P_ZLIB, 
#define C_ZLING	    0 //COMP2
 P_ZLING,  
#define C_ZOPFLI	COMP2	
 P_ZOPFLI, 
#define C_ZSTD		COMP1	
 P_ZSTD,
  // --------- Encoding -------------------
  #ifdef BASE64 
#define C_B64 		ENCOD
  #else
#define C_B64 		0
  #endif
 P_B64_AVX2,
 P_B64_NEON32,
 P_B64_NEON64,
 P_B64_PLAIN,
 P_B64_SSSE3,
 P_B64_SSE41,
 P_B64_SSE42,
 P_B64_AVX,

#define C_SB64 		0 //ENCOD
 P_SB64SSE,
#define C_FB64 		ENCOD
 P_FB64AVX,
 P_FB64CHROMIUM,
 P_FB64LINUX,
 P_FB64QUICKTIME,
 P_FB64SCALAR,
#define C_TB64		ENCOD
 P_TB64,
 P_TB64S,
#define C_RLE		ENCOD
 P_RLES,
 P_RLET,
 P_RLEM,
  //---------- Transform ------------------
#define C_DIVBWT     C_LIBBSC //_TRANSFORM
 P_DIVBWT,
#define C_ST         C_LIBBSC //_TRANSFORM
 P_ST,
  // --------- Entropy coders -------------
 #if C_BCM  
#define C_BCMEC     ECODER 
 #else
#define C_BCMEC     0
 #endif
 P_BCMEC,
#define C_FASTAC	ECODER    
 P_FASTAC,
#define C_FASTARI	ECODER    
 P_FASTARI,
#define C_FASTHF	ECODER    
 P_FASTHF,
#define C_FSC 		ECODER    
 P_FSC,
#define C_FPC 		ECODER    
 P_FPC,
#define C_FSE 		COMP1    
 P_FSE, 
 P_FSEH, 
#define C_FQZ 	 	ECODER    
 P_FQZ0,
 #if 1 //ndef NSIMD
#define C_JRANS		ECODER 
#define C_JAC 	    	ECODER    
 #else
#define C_JRANS		0
#define C_JAC 	    	0
 #endif
 P_JAC, 
 P_JRANS4_8o0,
 P_JRANS4_8o1,
 P_JRANS4_16o0,
 P_JRANS4_16o1,
 P_JRANS4_32o0,
 P_JRANS4_32o1,
  
#define C_FPAQC 	GPL
 P_FPAQC,
#define C_NANS		ECODER 
 P_NANS,      
#define C_PPMDEC	ECODER       
 P_PPMDEC,
#define C_SHRC		ECODER 
 P_SHRC,
 P_SHRCV,
#define C_SUBOTIN   ECODER    
 P_SUB,
#define C_POLHF     GPL
 P_POLHF, 
#define C_TORNADOHF	GPL	 
 P_TORNADOHF,
#define C_XPACK		COMP2    
 P_XPACK,
#define C_ZLIBH		ECODER    
 P_ZLIBH,
#define C_MYCODEC   COMP2	// Include your codec into TurboBench :search and modify "_MYCODEC" in the following files: plugins.h, plugreg.c. Insert your function calls like mycomp/mydecomp in plugins.cc 
 P_MYCODEC, // User plugin
  #ifdef LZTURBO
#include "../dev/x/beplug.h"
  #endif
}; 
   
  #ifdef NCPP  
#define C_BALZ 		0
#define C_BCM		0			
#define C_CSC 		0
#define C_DOBOZ		0
#define C_GIPFELI	0
#define C_SNAPPY	0	
#define C_YALZ77    0     
#define C_ZOPFLI	0	
#define C_ZLING	    0	
#define C_TORNADO	0
#define C_YAPPY 	0

#define C_BCMEC		0
#define C_FASTAC	0
#define C_FASTHF	0    
#define C_TORNADOHF	0 
  #endif

  #ifdef _WIN32
#define C_FSC 		0
#define C_GIPFELI	0
#define C_C_BLOSC2 	0
  #endif
//-------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <time.h>
#include "conf.h"
#include "plugins.h"

  #if C_AOM
#include "aom_/aom.h"
  #endif

  #if C_BALZ
#include "balz/balz.h"
  #endif

  #if C_BCM
#include "bcm_/bcm.h"
  #endif

  #if C_C_BLOSC2
    #ifdef C_C_BLOSC2LZ
#include "c-blosc2/blosc/blosclz.h"
    #else
#include "c-blosc2/blosc/blosc.h"
    #endif
  #endif
  
  #if C_BRIEFLZ 
#include "brieflz/include/brieflz.h"
  #endif
  
  #if C_LIBBSC
#include "libbsc/libbsc/libbsc.h"
#include "libbsc/libbsc/st/st.h"
#include "libbsc/libbsc/lzp/lzp.h"
  #endif

  #if C_LIBDEFLATE
#include "libdeflate/libdeflate.h"
  #endif

  #if C_BZIP2 
#include "bzip2/bzlib.h"
  #endif
  
  #if C_BROTLI
#include "brotli/c/include/brotli/encode.h"
#include "brotli/c/include/brotli/decode.h"
  #endif

  #if C_LZMA
#include "lzma/C/Alloc.h"
#include "lzma/C/LzmaEnc.h"
#include "lzma/C/LzmaDec.h"
  #endif

  #if C_CSC
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

  #if C_CRUSH
#include <stdint.h>
#include "crush/crush.h"
  #endif

  #if C_DAALA
#include "daala_/daala.h"
  #endif

  #if C_DOBOZ
#include "doboz/Source/Doboz/Compressor.h"
#include "doboz/Source/Doboz/Decompressor.h"
  #endif

  #if C_FLZMA2
#include "fast-lzma2/lzma2_enc.h"
#include "fast-lzma2/lzma2_dec.h"
  #endif

  #if C_GIPFELI
#include "gipfeli/gipfeli.h"
  #endif

  #if C_GLZA
#include "glza/GLZAcomp.h"
#include "glza/GLZAdecode.h"
  #endif

  #if C_HEATSHRINK
#include "heatshrink_/heatshrink.h"
  #endif

  #if C_IGZIP
#include "isa-l/include/igzip_lib.h"
  #endif

  #if C_LIBLZG
#include "liblzg/src/include/lzg.h"
  #endif

  #if C_LIBZPAQ
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

  #if C_LZ4
#include "lz4/lib/lz4.h"
#include "lz4/lib/lz4hc.h"
  #endif
    
  #if C_LIZARD
#include "lizard/lib/lizard_compress.h"    //v2.0
#include "lizard/lib/lizard_decompress.h"
  #endif
    
  #if C_LZFSEA
#include <compression.h>
  #endif  

  #if C_LZHAM
#include "lzham_codec_devel/include/lzham.h"
  #endif
 
  #if C_LZLIB
#include "lzlib_/bbexample.h"
  struct Lzma_options
    {
    int dictionary_size;		/* 4 KiB .. 512 MiB */
    int match_len_limit;		/* 5 .. 273 */
    };
  /* Mapping from gzip/bzip2 style 1..9 compression modes
     to the corresponding LZMA compression modes. */
  const struct Lzma_options option_mapping[] =
    {
    {   65535,  16 },		/* -0 (65535,16 chooses fast encoder) */
    { 1 << 20,   5 },		/* -1 */
    { 3 << 19,   6 },		/* -2 */
    { 1 << 21,   8 },		/* -3 */
    { 3 << 20,  12 },		/* -4 */
    { 1 << 22,  20 },		/* -5 */
    { 1 << 23,  36 },		/* -6 */
    { 1 << 24,  68 },		/* -7 */
    { 3 << 23, 132 },		/* -8 */
    { 1 << 25, 273 } };		/* -9 */
  struct Lzma_options encoder_options;
  #endif
  
  #if C_LZMAT
#include "lzmat/lzmat.h"
  #endif
  
  #if C_LZO
#include "lzo/include/lzo/lzo1b.h"
#include "lzo/include/lzo/lzo1c.h"
#include "lzo/include/lzo/lzo1f.h"
#include "lzo/include/lzo/lzo1x.h"
#include "lzo/include/lzo/lzo1y.h"
#include "lzo/include/lzo/lzo1z.h"
#include "lzo/include/lzo/lzo2a.h"
  #endif
 
  #if C_LZOMA
#include "lzoma_/lzoma.h"
  #endif

  #if C_LZSSE
#include "LZSSE/lzsse2/lzsse2.h"
#include "LZSSE/lzsse4/lzsse4.h"
#include "LZSSE/lzsse8/lzsse8.h"
  #endif
 
  #if C_MSCOMPRESS
#include "ms-compress/include/mscomp.h"
  #endif  

  #if C_LZSS
#include "lzss/lzss.h"
  #endif  

  #if C_NAKA
#include "nakamichi/nakamichi.h"
  #endif

  #if C_PITHY
#include "pithy/pithy.h"
  #endif

  #if C_QUICKLZ
#include "quicklz_/quicklz-c.h"
  #endif

  #if C_SAP
#include "pysap/pysapcompress/hpa101saptype.h"
#include "pysap/pysapcompress/hpa104CsObject.h"
#include "pysap/pysapcompress/hpa106cslzc.h"
#include "pysap/pysapcompress/hpa107cslzh.h"
#include "pysap/pysapcompress/hpa105CsObjInt.h"
#undef max
#undef min
  #endif

  #if C_SHRINKER
#include "shrinker/Shrinker.h"
  #endif
   
  #if C_SHOCO
#include "shoco/shoco.h"
  #endif

  #if C_SNAPPY
#include "snappy/snappy.h"
  #endif
   
  #if C_TORNADO
#include "tornado_/tormem.h"
  #endif

  #if C_WKDM
#include "wkdm/WKdm.h"
  #endif
  
  #if C_WFLZ
#include "wflz/wfLZ.h"
  #endif
  
  #if C_WIMLIB
#include "../wimlib/include/wimlib.h"
  #endif
  
  #if C_XPACK
#include "xpack/libxpack.h"
  #endif

  #if C_YALZ77
#include "yalz77/lz77.h"
  #endif

  #if C_YAPPY
#include "yappy/yappy.hpp"
  #endif

  #if C_ZLIB
     #if C_ZLIBLIB
#include <zlib.h>
     #elif defined(ZLIB_NG)
#include "zlib/zlib.h"
     #elif defined(ZLIB_INTEL)
#include "zlib_intel/zlib.h"
     #else
#include "zlib/zlib.h"
     #endif
  #endif
  
  #if C_ZLING
#include "libzling/src/libzling.h"
#include "libzling_/libzling_utils_mem.h"
  #endif  

  #if C_ZSTD
#define ZSTD_STATIC_LINKING_ONLY
#include "zstd/lib/zstd.h"
  #endif

  #ifdef LZTURBO
#include "../dev/x/beplugi.h"
  #endif  
//-----------------------------
  #if C_FB64
//#include "fastbase64/include/chromiumbase64.h"
  #endif

  #if __cplusplus  
extern "C" {
  #endif

  #if C_CHAMELEON
#include "chameleon/Chameleon2.h"
  #endif

  #if C_DENSITY
#include "density/src/density_api.h"
  #endif

  #if C_FASTLZ
#include "FastLZ/fastlz.h"
  #endif

  #if C_LIBLZF
#include "liblzf/lzf.h"
  #endif

  #if C_LIBSLZ
#include "libslz/src/slz.h"
  #endif

  #if C_LZFSE
#include "lzfse/src/lzfse.h"
  #endif  

  #if C_MINIZ
typedef unsigned long mz_ulong;
int mz_compress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len, int level);
int mz_uncompress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len);
  #endif

  #if C_SNAPPY_C
#include "snappy-c/snappy.h"
struct snappy_env env; 
  #endif
 
  #if C_ZOPFLI
#include "zopfli/src/zopfli/zopfli.h"  
  #endif

  #if __cplusplus
}
  #endif
  //------------------------------------ Encoding -----------------------------------
  #if C_RLE
#include "TurboRLE/trle.h"
#include "TurboRLE/ext/mrle.h"
  #endif
  #if C_TB64
#include "TurboBase64/turbob64.h"
  #endif
  #if C_SB64
//#include "base64simd/encode/lookup.sse.cpp"
//#include "base64simd/decode/decode.sse.cpp"
  #endif

  #if C_B64
#include "base64/include/libbase64.h"
  #endif

  #if __cplusplus  
extern "C" {
  #endif
  #if C_FB64
   #ifdef AVX2_ON
//#include "fastbase64/include/fastavxbase64.h"
size_t fast_avx2_base64_decode(char *out, const char *src, size_t len);
size_t fast_avx2_base64_encode(char* dest, const char* str, size_t len);
   #endif

//#include "fastbase64/include/chromiumbase64.h"
size_t chromium_base64_encode(char* dest, const char* str, size_t len);
size_t chromium_base64_decode(char* dest, const char* src, size_t len);
#include "fastbase64/include/scalarbase64.h"
#include "fastbase64/include/quicktimebase64.h"
#include "fastbase64/include/linuxbase64.h"

  #endif
  #if __cplusplus
}
  #endif

  //------------------------------------ Transform ----------------------------------
  #if C_DIVBWT || C_LIBBSC
#include "libbsc/libbsc/bwt/divsufsort/divsufsort.h"
#include "libbsc/libbsc/bwt/bwt.h"
#include "libbsc/libbsc/coder/coder.h"
  #endif
  //------------------------------------ Entropy Coder ------------------------------
  #if C_FASTAC
#include "fastac/arithmetic_codec.h"
  #endif
  
  #if C_FASTHF
#include "fasthf/binary_codec.h"
  #endif
  
  #if C_FPC
#include "FPC/fpc.h"
  #endif

  #if C_FSE
#include "zstd/lib/common/fse.h"
#include "zstd/lib/common/huf.h"
  #endif

  #if C_FQZ
#include "fqz0/f_o0.h"
  #endif

  #if C_JAC
#include "rans_static_/arith_static.h"
  #endif
  
  #if C_JRANS
#include "rans_static/rANS_static.h"
  #endif

  #if C_FPAQC
#include "fpaqc/fpaqc.h"
  #endif
  
  #if C_MMRC
#include "fpaq0f2/fpaq0f2.h"
#include "fpaq0p/fpaq0p_mm.h"
#include "o0zp/o0zp.h"
  #endif

  #if C_NANS
#include "ans_nania/narans.h"
  #endif

  #if C_POLHF
#include "polar/polar.h"
  #endif

  #if C_PPMDEC
#include "ppmdec/ppmdec.h"
  #endif

  #if C_SUBOTIN
#include "subotin_/subotin.h"
  #endif

  #if C_SHRC
#include "fpaq0p/fpaq0p_sh.h"
#include "vecrc/vector_rc.h"
  #endif  

  #if C_ZLIBH
#include "zlibh/zlibh.h"
  #endif
  
  #if C_MYCODEC
//#include "my_header.h"
  #endif

  #if __cplusplus  
extern "C" {
  #endif

  #if C_FASTARI
#include "FastARI/FastAri.h"
  #endif

  #if C_FSC
#include "fsc/fsc.h"
  #endif

  #if C_JRANS
#include "rans_static/rANS_static.h"
unsigned char *rans_compress_to_32x16(unsigned char *in,  unsigned int in_size,
				     unsigned char *out, unsigned int *out_size,
				     int order);
unsigned char *rans_uncompress_to_32x16(unsigned char *in,  unsigned int in_size,
				       unsigned char *out, unsigned int *out_size,
				       int order);
  #endif

  #if __cplusplus
}
  #endif
//------------------------------------------------- registry -------------------------------------------------------------------------------------------------
struct plugs plugs[] = {
  { P_BALZ, 	"balz", 			C_BALZ, 	"1.20",		"balz",					"Public Domain",	"http://sourceforge.net/projects/balz", 												"0,1" }, 
  { P_BCM, 		"bcm", 				C_BCM, 		"1.25",		"bcm",					"Public Domain",	"https://github.com/encode84/bcm", 													"" }, 
  { P_C_BLOSC2, "blosc",			C_C_BLOSC2, "2.0",		"Blosc",				"BSD license",		"https://github.com/Blosc/c-blosc2", 													"0,1,2,3,4,5,6,7,8,9", 64*1024},
  { P_BRIEFLZ,	"brieflz", 		    C_BRIEFLZ, 	"1.2.0",	"BriefLz",				"BSD like",			"https://github.com/jibsen/brieflz", 													"1,3,6,9" }, 
  { P_BROTLI,	"brotli", 			C_BROTLI, 	"17-04",	"Brotli",				"Apache license",	"https://github.com/google/brotli", 													"0,1,2,3,4,5,6,7,8,9,10,11/d#:V"},
  { P_BZIP2,	"bzip2", 			C_BZIP2, 	"1.06",		"Bzip2",				"BSD like",			"http://www.bzip.org/downloads.html\thttps://github.com/asimonov-im/bzip2", 			"" }, 
  { P_CHAMELEON,"chameleon",		C_CHAMELEON, "15-03",	"Chameleon",			"Public Domain",	"http://cbloomrants.blogspot.de/2015/03/03-25-15-my-chameleon.html", 					"1,2" },
  { P_CRUSH,	"crush", 			C_CRUSH, 	"1.0.0",	"Crush",				"Public Domain",	"http://sourceforge.net/projects/crush", 												"0,1,2" },
  { P_CSC,	    "csc", 				C_CSC, 		"16-01",	"CSC",					"Public domain",	"https://github.com/fusiyuan2010/CSC", 													"1,2,3,4,5" },
  { P_DENSITY, 	"density",        	C_DENSITY,	"0.12.0",	"Density",				"BSD license",		"https://github.com/centaurean/density",												"1,2,3" },
  { P_DOBOZ,	"doboz",			C_DOBOZ, 	"14-01-14",	"Doboz",				"BSD Like",			"https://bitbucket.org/attila_afra\thttps://github.com/nemequ/doboz", 					"" },  //crash on windows
  { P_FASTLZ,	"fastlz", 			C_FASTLZ,	"0.1.0",	"FastLz",				"BSD like",			"http://fastlz.org\thttps://github.com/ariya/FastLZ",									"1,2" },
  { P_FLZMA2,  	"flzma2", 			C_LZMA, 	"2018.3",	"Fast-lzma2",			"BSD",				"https://github.com/conor42/fast-lzma2", 												"0,1,2,3,4,5,6,7,8,9,10,11/mt#" }, 
  { P_GIPFELI, 	"gipfeli", 			C_GIPFELI, 	"16-08",	"Gipfeli",				"Apache license",	"https://github.com/google/gipfeli",													"" }, 
  { P_GLZA, 	"glza", 			C_GLZA, 	"16-08",	"glza",					"Apache license",	"https://github.com/jrmuizel/GLZA",													    "" }, 
  { P_HEATSHRINK,"heatshrink",		C_HEATSHRINK,"0.4.1",	"heatshrink",			"BSD license",		"https://github.com/atomicobject/heatshrink",											"" },
  { P_IGZIP,  	"igzip",			C_IGZIP, 	 "",	"igzip",					"BSD3",				"https://github.com/01org/isa-l",														"0,1,2,3" }, 
  { P_LIBBSC, 	"bsc", 				C_LIBBSC, 	"3.1.0",	"bsc",					"Apache license",	"https://github.com/IlyaGrebnov/libbsc",												"0,3,4,5,6,7,8/p:e#"}, 
  { P_LIBBSCC, 	"bscqlfc", 			C_LIBBSC, 	"3.1.0",	"bsc",					"Apache license",	"https://github.com/IlyaGrebnov/libbsc",												"1,2"}, 
  { P_LIBDEFLATE,"libdeflate", 	    C_LIBDEFLATE,"17-04",	"libdeflate",			"CC0 license",		"https://github.com/ebiggers/libdeflate",												"1,2,3,4,5,6,7,8,9,12/dg"}, 
  { P_LIBLZF, 	"lzf", 				C_LIBLZF, 	"1.06",		"LibLZF",				"BSD license",		"http://oldhome.schmorp.de/marc/liblzf.html\thttps://github.com/nemequ/liblzf",			"" },
  { P_LIBLZG,  	"lzg", 				C_LIBLZG,   "1.0.8",	"LibLzg",				"zlib-license",		"https://github.com/mbitsnbites/liblzg\thttp://liblzg.bitsnbites.eu/e",					"1,2,3,4,5,6,7,8,9" }, //"https://gitorious.org/liblzg" BLOCKSIZE must be < 64MB
  { P_LIBZPAQ,  "zpaq", 			C_LIBZPAQ, 	"7.10",		"Libzpaq",				"Public Domain",	"https://github.com/zpaq/zpaq",															"0,1,2,3,4,5" }, 
  { P_LIBSLZ, 	"slz",				C_LIBSLZ, 	"1.0.0",	"libslz",				"BSD license",	    "http://git.1wt.eu/web/libslz.git/",													"0,1,2,3,4,5,6,7,8,9" },
  { P_LZ4,  	"lz4",				C_LZ4, 		"",			"Lz4",					"BSD license",		"https://github.com/Cyan4973/lz4", 														"0,1,9,12,16" }, 
  { P_LIZARD,  	"lizard",			C_LIZARD, 	"2.0",	    "Lizard",				"BSD license",		"https://github.com/inikep/lizard",														"10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49" }, 
  { P_LZFSE, 	"lzfse", 			C_LZFSE, 	"17-03",	"lzfse",				"BSD licence",		"https://github.com/lzfse/lzfse","" },
  { P_LZFSEA, 	"lzfsea", 			C_LZFSEA, 	"2015",		"lzfsea",				"iOS and OS X",		"https://developer.apple.com/library/ios/documentation/Performance/Reference/Compression/index.html","" },
  { P_LZHAM, 	"lzham", 			C_LZHAM,	"1.1",		"Lzham",				"MIT license",		"https://github.com/richgel999/lzham_codec_devel",										"1,2,3,4/t#:fb#:x#" },
  { P_LZLIB, 	"lzlib", 			C_LZLIB, 	"1.9",		"Lzlib",				"BSD license",		"http://www.nongnu.org/lzip\thttps://github.com/daniel-baumann/lzlib",					"1,2,3,4,5,6,7,8,9/d#:fb#" },
  { P_LZMAT, 	"lzmat", 			C_LZMAT, 	"1.0",		"Lzmat",				"GPL license",		"https://github.com/nemequ/lzmat\thttp://www.matcode.com/lzmat.htm",					"" },
  { P_LZMA,  	"lzma", 			C_LZMA, 	"18.01",	"Lzma",					"Public Domain",	"http://7-zip.org\thttps://github.com/sisong/lzma", 								"0,1,2,3,4,5,6,7,8,9/d#:fb#:lp#:lc#:pb#:a#:mt#" }, 

  { P_LZO1b, 	"lzo1b", 			C_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"1,9,99,999" },  
  { P_LZO1c, 	"lzo1c",			C_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"1,9,99,999" },
  { P_LZO1f, 	"lzo1f",			C_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"1,999" },
  { P_LZO1x, 	"lzo1x", 			C_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"1,11,12,15,999" },
  { P_LZO1y,  	"lzo1y",			C_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"1,999" }, 
  { P_LZO1z, 	"lzo1z", 			C_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"999" }, 
  { P_LZO2a, 	"lzo2a", 			C_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"999" }, 
  { P_LZOMA, 	"lzoma", 			C_LZOMA,	"16-03",	"lzoma",				"GPL license",		"https://github.com/alef78/lzoma", 														"1,2,3,4,5,6,7,8,9" },
  { P_LZSSE2,	"lzsse2",   	    C_LZSSE,	"16-04",	"lzsse",				"BSD license",		"https://github.com/ConorStokes/LZSSE",													"1,2,3,4,5,6,7,8,9,12,16,17"}, 
  { P_LZSSE4,	"lzsse4",   	    C_LZSSE,	"16-04",	"lzsse",				"BSD license",		"https://github.com/ConorStokes/LZSSE",													"0,1,2,3,4,5,6,7,8,9,12,16,17"}, 
  { P_LZSSE8,	"lzsse8",   	    C_LZSSE,	"16-04",	"lzsse",				"BSD license",		"https://github.com/ConorStokes/LZSSE",													"0,1,2,3,4,5,6,7,8,9,12,16,17"}, 
  { P_MINIZ, 	"miniz", 			C_MINIZ,	"17-03",	"miniz zlib-replacement","Public domain",	"https://github.com/richgel999/miniz", 													"1,2,3,4,5,6,7,8,9" },
  { P_MSCOMPRESS,"mscompress", 		C_MSCOMPRESS,"16.06",	"ms-compress",			"GPL license",		"https://github.com/coderforlife/ms-compress", 											"2,3,4" }, 
  { P_NAKA, 	"naka", 			C_NAKA,		"17-03",	"Nakamichi Washigan",	"Public Domain",    "http://www.overclock.net/t/1577282/fastest-open-source-decompressors-benchmark#post_24538188",	"" },
  { P_PITHY, 	"pithy",			C_PITHY, 	"2011",		"Pithy",	  			"BSD license",		"https://github.com/johnezang/pithy",													"0,1,2,3,4,5,6,7,8,9" },
  { P_QUICKLZ, 	"quicklz",			C_QUICKLZ, 	"1.5.1",	"Quicklz",	  			"GPL license",		"http://www.quicklz.com\thttps://github.com/robottwo/quicklz",							"1,2,3" },
  { P_SAP, 	    "sap",				C_SAP, 		"17-03",	"sap",		  			"GPL license",		"https://github.com/CoreSecurity/pysap",												"0,1,2"	},
  { P_SHRINKER, "shrinker",			C_SHRINKER, "0.1/r9",	"Shrinker",				"BSD license",		"https://code.google.com/p/data-shrinker",												"", 0, (1<<26) },
  { P_SHOCO,    "shoco",			C_SHOCO, 	"2015",		"Shoco",				"MIT license",		"https://github.com/Ed-von-Schleck/shoco",												"" },
  { P_SNAPPY, 	"snappy",			C_SNAPPY, 	"1.1.2",	"Snappy",				"Apache license",	"https://github.com/google/snappy"														""	},
  { P_SNAPPY_C, "snappy_c",			C_SNAPPY_C,	"1.1.2",	"Snappy-c",				"BSD Like",			"https://github.com/andikleen/snappy-c",												"" },
  { P_TORNADO, 	"tornado", 			C_TORNADO, 	"0.6a",		"Tornado",				"GPL license",		"http://freearc.org\thttps://github.com/nemequ/tornado",								"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16" }, 
  { P_WFLZ,	    "wflz", 			C_WFLZ, 	"15-04",	"wfLZ",					"CC0 license",		"https://github.com/ShaneWF/wflz",														"1,2" },
//{ P_WKDM, 	"WKdm",				C_WKDM, 		"2003",		"WKdm",					"Apple PS License",	"http://www.opensource.apple.com/source/xnu/xnu-1456.1.26/iokit/Kernel/\thttps://github.com/berkus/wkdm", "" }, // crash
  { P_XPACK, 	"xpack", 			C_XPACK,	"16-08",	"xpack",				"BSD license",		"https://github.com/ebiggers/xpack", 													"1,2,3,4,5,6,7,8,9" },
  { P_YALZ77, 	"yalz77", 			C_YALZ77, 	"15-09",	"Yalz77",				"Public domain",	"https://github.com/ivan-tkatchev/yalz77",												"1,6,12" },
  { P_YAPPY, 	"yappy",			C_YAPPY, 	"2011",		"Yappy",				"",					"" ,																					"" },//crash windows
  { P_ZLIB, 	"zlib", 			C_ZLIB, 	"1.2.9",	"zlib",					"zlib license",		"http://zlib.net\thttps://github.com/madler/zlib", 										"1,2,3,4,5,6,7,8,9" },
  { P_ZLING, 	"zling", 	   		C_ZLING, 	"16-01",	"Libzling",				"BSD license",		"https://github.com/richox/libzling",													"0,1,2,3,4" }, 
  { P_ZOPFLI, 	"zopfli",			C_ZOPFLI, 	"16-04",	"Zopfli",				"Apache license",	"https://code.google.com/p/zopfli",														""}, 
  { P_ZSTD, 	"zstd", 			C_ZSTD,		"1.0.0",	"ZSTD",					"BSD license+Patents","https://github.com/facebook/zstd", 													"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15,-16,-17,-18,-19,-20,-21,-22/d#" },
//-----------------------------------------------------------------------------------	  
  { P_MCPY, 	"imemcpy", 			C_MEMCPY, 	".",		"inline memcpy",		"------------",		"--------------------------------------",												"" },
  { P_LMCPY, 	"memcpy",			C_MEMCPY,  	".",		"library memcpy",		"",					"",																						"" },
  { P_AOM, 	    "AOM",				C_AOM, 	    "",		    "AOMEDIA AV1 Entropy coder","        ",		"https://aomedia.googlesource.com/aom/",												""},
  { P_BCMEC, 	"bcmec", 			C_BCMEC, 	"1.0",		"bcm range coder",		"Public Domain",	"http://sourceforge.net/projects/bcm",													"" },
  { P_DAALA,    "Daala",			C_DAALA, 	"",		     "DAALA Entropy Coder", "        ",			"https://github.com/xiph/daala",												""},
  { P_FPC, 		"fpc", 				C_FPC, 		"",	"Fast Prefix Coder",	"BSD license",		"https://github.com/algorithm314/FPC",															"0,8,9,10,11,12,16,32,48,63" },
  { P_FSC, 		"fsc", 				C_FSC, 		"15-05",	"Finite State Coder",	"Apache license",	"https://github.com/skal65535/fsc",														"", E_ANS },
  { P_FSE, 		"fse", 				C_FSE, 		"",	"Finite State Entropy",	"BSD license",		"https://github.com/Cyan4973/FiniteStateEntropy",										"", E_ANS },
  { P_FSEH,		"fsehuf", 			C_FSE, 		"",	"Finite State Entropy",	"BSD license",		"https://github.com/Cyan4973/FiniteStateEntropy",										"", E_HUF },
  { P_FPAQC,	"fpaqc", 			C_FPAQC, 	"07-12",	"Fpaqc:Asymmetric Binary Coder","GPL license",	"http://www.mattmahoney.net/dc/",														"" },
  { P_SHRC,	    "fpaq0p_sh",		C_SHRC, 	"2010",	    "Bitwise RC",			"           ",		"http://encode.ru/threads/1153-Simple-binary-rangecoder-demo",							"" },
  { P_SHRCV,	"vecrc_sh",			C_SHRC, 	"2012",	    "Bitwise vector RC",	"           ",		"http://encode.ru/threads/1200-Vectorized-rangecoder",									"" },
  { P_FASTHF,  	"FastHF", 			C_FASTHF, 	"2006",		"Fast HF",				"BSD like",			"http://www.cipr.rpi.edu/research/SPIHT/",												"" },
  { P_FASTARI,  "FastAri", 			C_FASTARI, 	"15-10",	"FastAri",				"MIT license",		"https://github.com/davidcatt/FastARI", 												"" },
  { P_FASTAC,  	"FastAC", 			C_FASTAC, 	"2006",		"Fast AC",				"BSD like",			"http://www.cipr.rpi.edu/research/SPIHT/",												"" },
  { P_JAC, 		"arith_static",		C_JAC, 		"17-03",	"Range Coder/J.Bonfield","BSD license",		"https://github.com/jkbonfield/rans_static",													"", E_ANS},
  { P_FQZ0, 	"fqz0",				C_FQZ, 		"15-03",	"FQZ/PPMD Range Coder",	"Public Domain",	"http://encode.ru/threads/2149-ao0ec-Bytewise-adaptive-order-0-entropy-coder",			""},
  { P_PPMDEC, 	"ppmdec", 			C_PPMDEC,	"15-03",	"PPMD Range Coder",		"Public Domain",	"http://encode.ru/threads/2149-ao0ec-Bytewise-adaptive-order-0-entropy-coder",  		""},

  { P_JRANS4_8o0, "rans_static8",	C_JRANS, 	"",	"ANS/J.Bonfield",		"Public Domain",	"https://github.com/jkbonfield/rans_static",											"", E_ANS},
  { P_JRANS4_8o1, "rans_static8o1", C_JRANS, 	"",	"ANS/J.Bonfield",		"Public Domain",	"https://github.com/jkbonfield/rans_static",											"", E_ANS},
  { P_JRANS4_16o0,"rans_static16",  C_JRANS,	"",	"ANS/J.Bonfield",		"Public Domain",	"https://github.com/jkbonfield/rans_static",											"", E_ANS},
  { P_JRANS4_16o1,"rans_static16o1",C_JRANS,    "",	"ANS/J.Bonfield",		"Public Domain",	"https://github.com/jkbonfield/rans_static",											"", E_ANS},
  { P_JRANS4_32o0,"rans_static32",  C_JRANS,	"",	"ANS/J.Bonfield",		"Public Domain",	"https://github.com/jkbonfield/rans_static",											"", E_ANS},
  { P_JRANS4_32o1,"rans_static32o1",C_JRANS,    "",	"ANS/J.Bonfield",		"Public Domain",	"https://github.com/jkbonfield/rans_static",											"", E_ANS},
  
  { P_NANS,	    "naniarans",		C_NANS, 	"2015",	    "Nania Adaptive rANS",	"           ",		"http://encode.ru/threads/2079-nARANS-(Nania-Adaptive-Range-Variant-of-ANS)",			"", E_ANS},
  { P_POLHF,    "polar", 			C_POLHF, 	"10-07",	"Polar Codes",			"GPL license",		"http://www.ezcodesample.com/prefixer/prefixer_article.html",							"" },
  { P_SUB, 		"subotin", 			C_SUBOTIN, 	"2000",		"subotin RC",			"Public Domain",	"http://ezcodesample.com/ralpha/Subbotin.txt",											"" },
  { P_TORNADOHF,"tornado_huff", 	C_TORNADOHF,"0.6a",		"Tornado Huf",			"GPL license",		"http://freearc.org/Research.aspx\thttps://github.com/nemequ/tornado" ,					"" },
  { P_ZLIBH, 	"zlibh",			C_ZLIBH, 	"1.2.8",	"zlib Huffmann",		"BSD license",		"https://github.com/Cyan4973/FiniteStateEntropy",										"", E_HUF },
  //---- Encoding ------
  { P_RLES, 	"srle",	    		C_RLE, 	    "", 	"TurboRLE ESC",			"            ",		"https://github.com/powturbo/TurboRLE",  												"0,8,16,32,64" },
  { P_RLET, 	"trle",	    		C_RLE, 	    "", 	"TurboRLE",			    "            ",		"https://github.com/powturbo/TurboRLE",  												"" },
  { P_RLEM, 	"mrle",	    		C_RLE, 	    "", 	"Mespostine RLE",	    "            ",		"https://github.com/powturbo/TurboRLE",  												"" },
  
  { P_FB64AVX,		"fb64_avx2",    C_FB64,		 "    ",	"FastBase64",			"BSD license",		"https://github.com/lemire/fastbase64",  						"" },
  { P_FB64CHROMIUM, "fb64chromium", C_FB64,		 "    ",	"FastBase64",			"BSD license",		"https://github.com/lemire/fastbase64",  						"" },
  { P_FB64LINUX,	"fb64linux",    C_FB64,		 "    ",	"FastBase64",			"BSD license",		"https://github.com/lemire/fastbase64",  						"" },
  { P_FB64QUICKTIME,"fb64quicktime",C_FB64,		 "    ",	"FastBase64",			"BSD license",		"https://github.com/lemire/fastbase64",  						"" },
  { P_FB64SCALAR,	"fb64scalar",   C_FB64,		 "    ",	"FastBase64",			"BSD license",		"https://github.com/lemire/fastbase64",  						"" },
  { P_TB64,		    "TurboB64",    	C_FB64,		 "    ",	"TurboBase64",			"BSD license",		"https://github.com/powturbo/TurboBase64",  						"" },
  { P_TB64S,	    "TurboB64s",    C_FB64,		 "    ",	"TurboBase64",			"BSD license",		"https://github.com/powturbo/TurboBase64",  						"" },
//  { P_SB64SSE,		"base64simd",   C_SB64,		 "    ",	"Base64simd",			"BSD license",		"https://github.com/lemire/fastbase64",  						"" },
  { P_B64_AVX2,		"b64_avx2",     C_B64,		 "    ",	"Base64",				"BSD license",		"https://github.com/aklomp/base64",  						"" },
  { P_B64_PLAIN,	"b64_plain",    C_B64,		 "    ",	"Base64",				"BSD license",		"https://github.com/aklomp/base64",  						"" },
  { P_B64_SSSE3,	"b64_ssse3",    C_B64,		 "    ",	"Base64",				"BSD license",		"https://github.com/aklomp/base64",  						"" },
  { P_B64_SSE41,	"b64_sse41",    C_B64,		 "    ",	"Base64",				"BSD license",		"https://github.com/aklomp/base64",  						"" },
  //----- Transform -----
  { P_DIVBWT, 	"divbwt",    		C_DIVBWT,    "    ",	"bwt libdivsufsort/libbsc",	"        ",		"https://github.com/y-256/libdivsufsort",  												"" },
  { P_ST, 	    "st",    			C_ST,   	 "    ",	"st  libbsc",			"Apache license",	"https://github.com/IlyaGrebnov/libbsc",  						"3,4,5,6,7,8" },

//{ P_MYCODEC, 	"mycodec",			C_MYCODEC, 	"0",		"My codec",				"           ",		"",																						"" },
    #ifdef LZTURBO
  #include "../dev/x/beplugr.h"
    #endif  
  { -1 }
};

//---------------------------------------------- plugins --------------------------------------------------------
  #ifndef max
#define max(x,y) (((x)>(y)) ? (x) : (y))
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
void coddicsize(int _dicsize) { dicsize = _dicsize; }

static char _workmem[1<<16],*workmem=_workmem;
static int state_size,dstate_size;
static size_t workmemsize;

int codini(size_t insize, int codec, int lev) {
  workmemsize = 0;

  switch(codec) {
      #if C_C_BLOSC2
    case P_C_BLOSC2: blosc_init(); blosc_set_nthreads(1);break;
      #endif
      
      #if C_FASTARI
    case P_FASTARI: workmemsize = FA_WORKMEM; break;
      #endif

      #if C_LZFSE
    case P_LZFSE:
      { size_t lzfse_size = lzfse_encode_scratch_size();
        workmemsize = max(lzfse_size, workmemsize);
        lzfse_size = lzfse_decode_scratch_size();
        workmemsize = max(lzfse_size, workmemsize);
      }
      break;
      #endif

      #if C_LZFSEA
    case P_LZFSEA:
      { size_t lzfsea_size = compression_encode_scratch_buffer_size(COMPRESSION_LZFSE);
        workmemsize = max(lzfsea_size, workmemsize);
        lzfsea_size = compression_decode_scratch_buffer_size(COMPRESSION_LZFSE);
        workmemsize = max(lzfsea_size, workmemsize);
      }
      break;
      #endif

      #if C_LZO
    case P_LZO1b: lzo_init(); workmemsize = LZO1B_999_MEM_COMPRESS; break;
    case P_LZO1c: P_LZO1f: P_LZO1x: P_LZO1y: P_LZO1z: P_LZO2a: lzo_init(); workmemsize = LZO1X_MEM_COMPRESS; break;
      #endif
	
      #if C_CHAMELEON
    case P_CHAMELEON: workmemsize = sizeof(struct Chameleon); break;
	  #endif
	
      #if C_QUICKLZ
    case P_QUICKLZ:
      state_size  = max(qlz_get_setting1(1), max(qlz_get_setting2(1), qlz_get_setting3(1))); workmemsize = max(state_size, workmemsize);
      dstate_size = max(qlz_get_setting1(2), max(qlz_get_setting2(2), qlz_get_setting3(2))); workmemsize = max(dstate_size, workmemsize);
      break;
      #endif
 
      #if C_BRIEFLZ
    case P_BRIEFLZ: workmemsize = blz_workmem_size_level(insize, lev); break;
      #endif  

      #if C_SNAPPY_C
    case P_SNAPPY_C: snappy_init_env(&env); break; 
      #endif
    
      #if C_WFLZ	
    case P_WFLZ: workmemsize = wfLZ_GetWorkMemSize(); break;
      #endif

      #if C_YAPPY
    case P_YAPPY: YappyFillTables(); break;
      #endif

      #if C_LIBBSC
    case P_LIBBSC: case P_LIBBSCC: bsc_init(LIBBSC_FEATURE_FASTMODE); bsc_st_init(LIBBSC_FEATURE_FASTMODE); break;
      #endif

      #if C_LIBSLZ
         slz_make_crc_table();
         slz_prepare_dist_table();
      #endif

	  #if C_FSE
    case P_FSEH: workmemsize = max(4096*sizeof(unsigned), workmemsize); break;
      #endif
      #ifdef LZTURBO  
    #include "../dev/x/beplug0.h"
      #endif
  }
  if(!workmemsize) return 0;
  if(workmemsize > sizeof(_workmem) && !(workmem = (char *)malloc(workmemsize)) ) { 
    fprintf(stderr, "Malloc error: %d\n", workmemsize); 
    exit(0);
  }
  return 1;
}  

void codexit(int codec) { 
  if(workmem != _workmem) {
    free(workmem/*, workmemsize*/); 
    workmem = NULL;
  }
    #if C_SNAPPY_C
  if(codec == P_SNAPPY_C)
    snappy_free_env(&env);
    #endif
} 

int brotlidic,brotlictx,brotlirep;
#define powof2(n) !((n)&((n)-1))

int codcomp(unsigned char *in, int inlen, unsigned char *out, int outsize, int codec, int lev, char *prm) { int outlen; unsigned char *oend=out+outsize;
  unsigned dsize = dicsize; char *q;
  if(q = strchr(prm,'d')) dsize = argtoi(q+(q[1]=='='?2:1),0); 

  switch(codec) { 
	  #if C_BALZ
	case P_BALZ: return balzcompress(in, inlen, out,lev);
      #endif 
 
	  #if C_BCM
    case P_BCM: return bcmcompress(in, inlen, out); break;
      #endif 

      #if C_C_BLOSC2
    case P_C_BLOSC2:
        #ifdef _C_BLOSC2LZ
      return blosclz_compress(lev, in, inlen, out, outsize);
        #else
      return blosc_compress(lev, strchr(prm,'s')?1:0/*doshuffle*/, (q=strchr(prm,'t'))?atoi(q+(q[1]=='='?2:1)):1/*typesize*/, inlen, in, out, outsize/*inlen+BLOSC_MAX_OVERHEAD*/);
        #endif 
      #endif 

      #if C_BRIEFLZ
	case P_BRIEFLZ: return blz_pack_level(in, out, inlen, workmem, lev);
	  #endif
	  
      #if C_BROTLI
    case P_BROTLI: { int lgwin, mode = BROTLI_DEFAULT_MODE; brotlidic = brotlictx = brotlirep = 0; 
        lgwin = dsize?(bsr32(dsize)-powof2(dsize)):BROTLI_DEFAULT_WINDOW;
        if(q = strchr(prm,'m')) mode = atoi(q+(q[1]=='='?2:1));										if(strchr(prm,'V')) brotlidic++; //	if(strchr(prm,'R')) brotlirep++; if(strchr(prm,'X')) brotlictx++;
        size_t esize = outsize;
        int rc = BrotliEncoderCompress(lev, lgwin, (BrotliEncoderMode)mode, (size_t)inlen, (uint8_t*)in, &esize, (uint8_t*)out); 		
        return rc?esize:0; 
      }
	  #endif    

      #if C_LIBBSC
	#define BSC_MODE LIBBSC_FEATURE_FASTMODE|(strchr(prm,'P')?LIBBSC_FEATURE_LARGEPAGES:0)|(strchr(prm,'t')?0:LIBBSC_FEATURE_MULTITHREADING)
	case P_LIBBSC: return bsc_compress(      in, out, inlen,/*18*/strchr(prm,'p')?0:LIBBSC_DEFAULT_LZPHASHSIZE,/*32*/ strchr(prm,'p')?0:LIBBSC_DEFAULT_LZPMINLEN, lev<3?1:lev, (q=strchr(prm,'e'))?atoi(q+(q[1]=='='?2:1)):1, BSC_MODE);
	case P_LIBBSCC:return bsc_coder_compress(in, out, inlen, lev, BSC_MODE);
	  #endif
    int bsc_coder_compress(const unsigned char * input, unsigned char * output, int n, int coder, int features);

	  #if C_LIBDEFLATE
	case P_LIBDEFLATE:  { 
       struct libdeflate_compressor *dc = libdeflate_alloc_compressor(lev); 
            if(strchr(prm,'d')) outlen = libdeflate_deflate_compress(dc,in, inlen,out, outsize);
       else if(strchr(prm,'g')) outlen = libdeflate_gzip_compress(   dc,in, inlen,out, outsize);
       else                     outlen = libdeflate_zlib_compress(  dc,in, inlen,out, outsize); 
	   libdeflate_free_compressor(dc); return outlen;
	  } 
	  #endif

      #if C_BZIP2
	case P_BZIP2:    { unsigned outlen = outsize; return BZ2_bzBuffToBuffCompress((char *)out, &outlen, (char *)in, inlen, 9, 0, 0)==BZ_OK?outlen:-1; }
	  #endif
	  
	  #if C_CHAMELEON
    case P_CHAMELEON:  { Chameleon_Reset((Chameleon *)workmem); return lev<2?Chameleon_Encode((Chameleon *)workmem,out,in, inlen):Chameleon2_Encode((Chameleon *)workmem,out,in, inlen); }
	  #endif

      #if C_CSC
    case P_CSC: { 
        CSCProps prop; CSCEncProps_Init(&prop, dsize?bsr32(dsize)-powof2(dsize):(1<<29), lev); CSCEnc_WriteProperties(&prop, (uint8_t*)out, 0);
        MemISeqInStream  si; si.s.Read  = (int(*)(void *, void *, size_t *))cscread;  si.in  = in;                  si.inlen  = inlen;
	    MemISeqOutStream so; so.s.Write = (size_t(*)(void *, const void *, size_t  ))cscwrite; so.out = out + CSC_PROP_SIZE; so.outlen = CSC_PROP_SIZE;
	    CSCEncHandle eh = CSCEnc_Create(&prop, (ISeqOutStream*)&so, NULL); CSCEnc_Encode(eh, (ISeqInStream*)&si, NULL); CSCEnc_Encode_Flush(eh); CSCEnc_Destroy(eh);
        return so.outlen;
      }
      #endif

      #if C_CRUSH
    case P_CRUSH: return crush_compress(lev, in, inlen, out);
      #endif
	  
      #if C_DENSITY
    case P_DENSITY: { 
	    density_processing_result rs = density_compress((const uint8_t *)in, inlen, (uint8_t*)out, outsize, (const DENSITY_ALGORITHM)lev/*, DENSITY_BLOCK_TYPE_DEFAULT, NULL, NULL*/);
        return rs.state?0:rs.bytesWritten;
	  }
      #endif        (
       	  
      #if C_DOBOZ
	case P_DOBOZ:   { doboz::Compressor c; size_t rc; return c.compress(in,inlen, out, c.getMaxCompressedSize(inlen), rc) == doboz::RESULT_OK ? rc : -1; }  
	  #endif
	  
      #if C_FASTLZ
    case P_FASTLZ:  return fastlz_compress_level(lev, in, inlen, out);
	  #endif
	  
 	  #if C_GIPFELI 
    case P_GIPFELI: { 
        util::compression::Compressor *c = util::compression::NewGipfeliCompressor();
        util::compression::UncheckedByteArraySink sink((char*) out);
        util::compression::ByteArraySource        src((const char*)in, inlen);
        outlen = c->CompressStream(&src, &sink); delete c; return outlen;
	  }	
	  #endif
 
      #if C_GLZA
    case P_GLZA:  { size_t outsize; return GLZAcomp(inlen, (uint8_t *)in, &outsize, (uint8_t *)out, (FILE *)0)?outsize:0; }
	  #endif

      #if C_HEATSHRINK
    case P_HEATSHRINK:   return hscompress(in, inlen, out);
      #endif


      #if C_IGZIP
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

	  #if C_LIBLZF  
    case P_LIBLZF:    return lzf_compress(in, inlen, out, outsize);
	  #endif
	
      #if C_LIBZPAQ
    case P_LIBZPAQ: { zin = in; zin_ = in+inlen; zout = out; char s[3]; s[0]=lev+'0'; s[1]=0; libzpaq::compress(&zmemin, &zmemout, s); return zout - out; }
      #endif

	  #if C_LZ4
    case P_LZ4: return !lev?LZ4_compress_fast((char *)in, (char *)out, inlen, outsize, 4):(lev<9?LZ4_compress_default((char *)in, (char *)out, inlen, outsize):LZ4_compress_HC((char *)in, (char *)out, inlen, outsize, lev));
	  #endif

	  #if C_LIZARD
    case P_LIZARD: return Lizard_compress((const char*)in, (char*)out, inlen, outsize, lev); 
	  #endif

      #if C_LZFSE
    case P_LZFSE : return lzfse_encode_buffer(out, outsize, in, inlen, workmem);
      #endif

      #if C_LZFSEA
    case P_LZFSEA : return compression_encode_buffer(out, outsize, in, inlen, workmem, COMPRESSION_LZFSE);
      #endif

      #if C_LZHAM
    case P_LZHAM: { static int dicbits[] = { 24, 24, 24, 26, 29, 29 }; lzham_compress_params p; memset(&p, 0, sizeof(p)); p.m_struct_size = sizeof(p);
                               p.m_dict_size_log2                    = out[0] = dsize?(bsr32(dsize)-powof2(dsize)):dicbits[lev]; //printf("[%u,%u] ", dsize, p.m_dict_size_log2);
                               p.m_level                             = (lzham_compress_level)lev;
                               p.m_compress_flags                   |= LZHAM_COMP_FLAG_FORCE_SINGLE_THREADED_PARSING;
                               p.m_compress_flags                   |= strchr(prm,'x')?LZHAM_COMP_FLAG_EXTREME_PARSING:0;
        if(q=strchr(prm,'t'))  p.m_max_helper_threads                = atoi(q+(q[1]=='='?2:1));
        if(q=strstr(prm,"fb")) p.m_fast_bytes                        = atoi(q+(q[2]=='='?3:2));
		if(q=strchr(prm,'x'))  { unsigned x = atoi(q+(q[1]=='='?2:1)); p.m_extreme_parsing_max_best_arrivals = x<4?4:x; }
		                       p.m_table_update_rate   				 = LZHAM_DEFAULT_TABLE_UPDATE_RATE;		
        size_t outlen        = outsize;		
		lzham_uint32 adler32 = 0;
        lzham_compress_status_t rc = lzham_compress_memory(&p, (lzham_uint8*)out+1, &outlen, (const lzham_uint8 *)in, inlen, &adler32);
        return rc == LZHAM_COMP_STATUS_SUCCESS?outlen:-rc;
    }
      #endif
		
      #if C_FLZMA2
    case P_FLZMA2: { unsigned nbThreads = 1; char *q; if(q=strstr(prm,"mt")) nbThreads = atoi(q+(q[2]=='='?3:2)); return FL2_compressMt(out, outsize, in, inlen, lev, nbThreads); }
      #endif
      #if C_LZMA
        #ifdef __x86_64__
           #define DICSIZE (1<<30)
        #else
           #define DICSIZE (1<<27)
        #endif
	case P_LZMA: { CLzmaEncProps p;	LzmaEncProps_Init(&p); p.level = lev; p.numThreads = 1; char *q; 
	  if(q=strstr(prm,"lc")) p.lc         = atoi(q+(q[2]=='='?3:2)); 
	  if(q=strstr(prm,"lp")) p.lp         = atoi(q+(q[2]=='='?3:2)); 
	  if(q=strstr(prm,"pb")) p.pb         = atoi(q+(q[2]=='='?3:2)); 
	  if(q=strstr(prm,"fb")) p.fb         = atoi(q+(q[2]=='='?3:2));else if(q=strstr(prm,"nice=")) p.fb = atoi(q+5); 
	  if(q=strstr(prm,"mc")) p.mc         = atoi(q+(q[2]=='='?3:2));  
	  if(q=strstr(prm,"mt")) p.numThreads = atoi(q+(q[2]=='='?3:2));
	  if(q=strchr(prm,'a'))  p.algo       = atoi(q+(q[1]=='='?2:1));
	  if(q=strstr(prm,"mf=bt")) p.btMode  = 1, p.numHashBytes = atoi(q+5);
	  if(q=strstr(prm,"mf=hc")) p.btMode  = 0, p.numHashBytes = atoi(q+5);
	  if(dsize) p.dictSize = dsize; if(p.dictSize>inlen)  p.dictSize=inlen; if(p.dictSize>DICSIZE)  p.dictSize=DICSIZE; //printf("dsize=%u, %d,%d,%d:%d, %d,%d, %d,%d\n ", p.dictSize, p.lc,p.lp,p.pb,p.fb, p.mc,p.algo, p.btMode,p.numHashBytes);
	  LzmaEncProps_Normalize(&p);
      SizeT psize = LZMA_PROPS_SIZE, outlen = outsize - LZMA_PROPS_SIZE; 
  	  return LzmaEncode(out+LZMA_PROPS_SIZE, &outlen, in, inlen, &p, out, &psize, 0, NULL, &g_Alloc, &g_Alloc) == SZ_OK?outlen+LZMA_PROPS_SIZE:0;
	}
      #endif
	
      #if C_LZLIB
	case P_LZLIB:  { long outlen=outsize; unsigned ds = dsize?dsize:option_mapping[lev].dictionary_size, mlen=option_mapping[lev].match_len_limit; 
      if(q=strstr(prm,"fb")) mlen = atoi(q+(q[2]=='='?3:2)); if(ds > inlen) ds = inlen; //printf("(%u,%d,%u)", ds, mlen, inlen);
	  bbcompress( (const uint8_t *)in, inlen, (uint8_t *)out, &outlen,  ds, mlen); return outlen; 
	}
      #endif
	    
	  #if C_LIBLZG
	case P_LIBLZG: { lzg_encoder_config_t cfg; LZG_InitEncoderConfig(&cfg); cfg.fast = LZG_TRUE; cfg.level = lev; return LZG_Encode(in, inlen, out, outsize, &cfg); }
	  #endif

	  #if C_LIBSLZ
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
	  #if C_LZMAT
	case P_LZMAT:   { MP_U32 outs=outsize; int rc = lzmat_encode(out, &outs, in, inlen); return rc == LZMAT_STATUS_OK ? outs : 0; }
      #endif

	  #if C_LZOMA
	case P_LZOMA:   return lzomapack(in, inlen, out, lev);
      #endif

	  #if C_LZO
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
 
      #if C_LZSS
	case P_LZSS1:   { return lzss_encode(in, inlen, out, 0); }
    case P_LZSS2:   { return lzss_encode(in, inlen, out, 1); }
	  #endif

	  #if C_LZSSE
	case P_LZSSE2: {            LZSSE2_OptimalParseState *s = LZSSE2_MakeOptimalParseState(inlen); if(!s) return 0; outlen = LZSSE2_CompressOptimalParse( s, in, inlen, out, outsize, lev ); LZSSE2_FreeOptimalParseState(s); return outlen; }
	case P_LZSSE4: if(lev==0) { LZSSE4_FastParseState    *s = LZSSE4_MakeFastParseState();                          outlen = LZSSE4_CompressFast(         s, in, inlen, out, outsize      ); LZSSE4_FreeFastParseState(s);    return outlen; }
                   else {       LZSSE4_OptimalParseState *s = LZSSE4_MakeOptimalParseState(inlen); if(!s) return 0; outlen = LZSSE4_CompressOptimalParse( s, in, inlen, out, outsize, lev ); LZSSE4_FreeOptimalParseState(s); return outlen; }
	case P_LZSSE8: if(lev==0) { LZSSE8_FastParseState    *s = LZSSE8_MakeFastParseState();                          outlen = LZSSE8_CompressFast(         s, in, inlen, out, outsize      ); LZSSE8_FreeFastParseState(s);    return outlen; }
                   else {       LZSSE8_OptimalParseState *s = LZSSE8_MakeOptimalParseState(inlen); if(!s) return 0; outlen = LZSSE8_CompressOptimalParse( s, in, inlen, out, outsize, lev ); LZSSE8_FreeOptimalParseState(s); return outlen; }

	  #endif

      #if C_MSCOMPRESS
    case P_MSCOMPRESS: { size_t outlen=outsize; return ms_compress((MSCompFormat)lev, in, inlen, out, &outlen)?0:outlen; }
      #endif

 	  #if C_MINIZ
    case P_MINIZ:   { uLongf outlen = outsize; int rc = mz_compress2(out, &outlen, in, inlen, lev); if (rc != Z_OK) printf("miniz compress2 rc=%d\n", rc); return outlen; }
      #endif

      #if C_NAKA
    case P_NAKA:    return NakaCompress( (char *)out, (char *)in, inlen); 
       #endif 

	  #if C_PITHY
    case P_PITHY: return pithy_Compress((const char *)in, inlen, (char *)out, outsize, lev);
      #endif  	

      #if C_QUICKLZ
    case P_QUICKLZ: { memset(workmem,0,workmemsize); return lev<=1?qlz_compress1((char *)in, (char *)out, inlen, workmem):(lev<=2?qlz_compress2((char *)in, (char *)out, inlen, workmem):qlz_compress3((char *)in, (char *)out, inlen, workmem)); }
      #endif  	

	  #if C_SAP
    case P_SAP: { CsObjectInt c; SAP_INT bytes_read, bytes_written; int rc = c.CsInitCompr((SAP_BYTE *)out, inlen, lev); out += CS_HEAD_SIZE; outsize -= CS_HEAD_SIZE;
        c.CsCompr(inlen, in, inlen, out, outsize, lev, &bytes_read, &bytes_written); return bytes_written+CS_HEAD_SIZE; 
      }
      #endif  	

	  #if C_SHOCO
    case P_SHOCO:     return shoco_compress((const char *)in, inlen, (char *)out, outsize);
      #endif  	

	  #if C_SHRINKER
    case P_SHRINKER:     return shrinker_compress((char *)in, (char *)out, inlen);
      #endif  	
    
	  #if C_SNAPPY 
	    #if __cplusplus
    case P_SNAPPY:    { size_t outlen=outsize; snappy::RawCompress((char *)in, inlen, (char*)out, &outlen); return outlen;}
        #else
    case P_SNAPPY:    { size_t outlen=outsize; int rc = snappy_compress(in, inlen, out, &outlen); return outlen;}
	    #endif 
	  #endif	

	  #if C_SNAPPY_C
    case P_SNAPPY_C:   { size_t outlen=outsize; int rc = snappy_compress(&env, (const char *)in, inlen, (char *)out, &outlen); return outlen;}
	  #endif 
	  
      #if C_TORNADO
    case P_TORNADO:      return torcompress(lev, in, out, inlen);
      #endif	

      #if C_WFLZ
    case P_WFLZ:    return lev<=1?wfLZ_CompressFast( (const uint8_t* WF_RESTRICT const)in, inlen, (uint8_t* WF_RESTRICT const)out, (const uint8_t* WF_RESTRICT)workmem, 0 ):
                                      wfLZ_Compress( (const uint8_t* WF_RESTRICT const)in, inlen, (uint8_t* WF_RESTRICT const)out, (const uint8_t* WF_RESTRICT)workmem, 0 );
	  #endif

      #if C_WIMLIB
    case P_WIMLIB: { struct wimlib_compressor *compressor; if(wimlib_create_compressor(lev, 32*1024/*inlen*/, 0, &compressor)) return 0;
        outlen = wimlib_compress(in, inlen, out, outsize /*inlen- 1*/, compressor);  wimlib_free_compressor(compressor); return outlen; 
      }
	  #endif

      #if C_WKDM
    case P_WKDM:    return WKdm_compress ((WK_word*)in, (WK_word*)out, inlen);
	  #endif

	  #if C_XPACK
	case P_XPACK:  { struct xpack_compressor *dc = xpack_alloc_compressor(inlen,lev); 
	   outlen = xpack_compress(dc,in, inlen,out, outsize); 
	   xpack_free_compressor(dc); return outlen;
	  } 
	  #endif

      #if C_YALZ77
    case P_YALZ77: { lz77::compress_t c(lev, lz77::DEFAULT_BLOCKSIZE); std::string os = c.feed(in,in+inlen); memcpy(out, os.c_str(), os.size()); return os.size(); }
	  #endif

      #if C_YAPPY
    case P_YAPPY:    return YappyCompress(in, out, inlen, 10)-out;
	  #endif

      #if C_ZLIB
    case P_ZLIB:     { uLongf outlen = outsize; int rc = compress2(out, &outlen, in, inlen, lev); if(rc != Z_OK) printf("zlib compress2 rc=%d\n", rc);	return outlen; }
      #endif
	  
      #if C_ZLING
    case P_ZLING:    return zling_compress(lev, in, inlen, out, outsize);
      #endif     
	  
	  #if C_ZOPFLI
	case P_ZOPFLI:  { 
        ZopfliOptions opts; ZopfliInitOptions(&opts); size_t outlen = 0; unsigned char *pout = out;
        ZopfliCompress(&opts, ZOPFLI_FORMAT_ZLIB, in, inlen, &pout, &outlen); 
        if(pout != out) 
          memcpy(out, pout, outlen);
	    return outlen; 
	 }
      #endif	    

	  #if C_ZSTD
    case P_ZSTD: { ZSTD_CCtx *ctx = ZSTD_createCCtx(); //ZSTD_compress( out, outsize, in, inlen, lev); 
      ZSTD_parameters p = ZSTD_getParams(lev, inlen, 0); p.fParams.contentSizeFlag = 1;
      if(dsize) { p.cParams.windowLog = bsr32(dsize)-powof2(dsize);  
        p.cParams.chainLog = p.cParams.windowLog + ((p.cParams.strategy == ZSTD_btlazy2) | (p.cParams.strategy == ZSTD_btopt));
      }
      unsigned rc = ZSTD_compress_advanced(ctx, out, outsize, in, inlen, NULL, 0, p);
      ZSTD_freeCCtx(ctx); 
      return rc; 
	}
      #endif   
    //------------------------- Encoding
     #if C_RLE 	 
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
    case P_RLET: return trlec(in, inlen, out);//return brlec8(in, inlen, out, out+inlen);
    case P_RLEM: return mrlec(in, inlen, out);
      #endif 

	  #if C_B64
    case P_B64_AVX2:  { size_t outlen=0; base64_encode(in, inlen, out, &outlen, BASE64_FORCE_AVX2); return outlen; }
    case P_B64_PLAIN: { size_t outlen=0; base64_encode(in, inlen, out, &outlen, BASE64_FORCE_PLAIN); return outlen; }
    case P_B64_SSSE3: { size_t outlen=0; base64_encode(in, inlen, out, &outlen, BASE64_FORCE_SSSE3); return outlen; } 
    case P_B64_SSE41: { size_t outlen; base64_encode(in, inlen, out, &outlen, BASE64_FORCE_SSE41); return outlen; }
	  #endif

	  #if C_FB64
        #ifdef AVX2_ON
	case P_FB64AVX: 	 { size_t outlen = outsize; fast_avx2_base64_encode(  out,   in,inlen);return outlen; }
	    #endif
	case P_FB64CHROMIUM:  return chromium_base64_encode( (char*)out, (const char*)in, inlen);
	case P_FB64LINUX:     return linux_base64_encode(    (char*)out, (const char*)in, (const char*)(in+inlen));
	case P_FB64QUICKTIME: return quicktime_base64_encode((char*)out, (const char*)in, inlen);
	case P_FB64SCALAR:   { size_t outlen = outsize; scalar_base64_encode(   (const char*)in,inlen,(char*)out,&outlen);return outlen; }
	  #endif

	  #if C_SB64
	case P_SB64SSE:  base64::sse::encode(base64::sse::lookup_naive, in, inlen, out); return ((inlen+2)/3)*4;
	  #endif

	  #if C_TB64 
	case P_TB64:  return turbob64enc(in, inlen, out); 
	case P_TB64S: return turbob64encs(in, inlen, out); 
     #endif

    //------------------------- Transform -----------------------------
      #if C_DIVBWT
    case P_DIVBWT: { int *sa = (int *)malloc((inlen + 1) * sizeof(int)); if(!sa) return -1; 
	  unsigned bwtidx = divbwt(in, out+sizeof(bwtidx), sa, inlen, NULL, NULL, 0); free(sa); *(unsigned *)out = bwtidx; return inlen+4; }
    case P_ST: { memcpy(out+4,in, inlen); *(unsigned *)(out) = bsc_st_encode(out+4, inlen, lev, 0); return inlen+4; }
      #endif	
    //------------------------- Entropy Coders -------------------------
      #if C_MEMCPY 
    case P_MCPY:     memcpy(out, in, inlen); return inlen;
    case P_LMCPY: libmemcpy(out, in, inlen); return inlen;
	  #endif	

      #if C_AOM
    case P_AOM:     return aomenc(in, inlen, out, outsize); 
      #endif

      #if C_DAALA
    case P_DAALA:   return daalaenc(in, inlen, out, outsize); 
      #endif

      #if C_BCMEC
	case P_BCMEC:  return bcmenc(in, inlen, out);
	  #endif

    #define SYMNUM 0x100	
	  #if C_FASTAC
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
 
      #if C_FASTARI
    case P_FASTARI:  { size_t outlen = outsize; return fa_compress((const unsigned char *)in, out, inlen, &outlen, workmem)?0:outlen; }
      #endif

	  #if C_FASTHF
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

      #if C_FPAQC
    case P_FPAQC:   return absc(in, inlen, out, outsize); 
      #endif

      #if C_FPC
    case P_FPC:  return FPC_compress(out,in,inlen,lev*1024); 
      #endif

      #if C_FQZ
    case P_FQZ0:  { unsigned int outlen; compress_block(in, inlen, out, &outlen); return outlen; }
      #endif

	  #if C_FSC
    case P_FSC:     { size_t outlen = 0; uint8_t *op = NULL; int ok = FSCEncode(in, inlen, &op, &outlen, 12, CODING_METHOD_DEFAULT); if(ok) { memcpy(out, op, outlen); free(op); } return outlen; }
	  #endif 

	  #if C_FSE
    case P_FSE:     { size_t o = FSE_compress(out, outsize, in, inlen); if(o == 1) { out[0] = in[0]; return 1; } if(!o || o >= inlen) { memcpy(out, in, inlen); return inlen; } return o; }
    case P_FSEH:    { size_t o = HUF_compress(out, outsize, in, inlen); //HUF_compress4X_wksp(out, outsize, in, inlen, 255, 11, workmem, workmemsize); //; 
      if(o == 1) { out[0] = in[0]; return 1; } if(!o || o >= inlen) { memcpy(out, in, inlen); return inlen; } return o; 
    }
      #endif 

	  #if C_NANS
    case P_NANS:   return nansenc(in, inlen, out, oend);
      #endif

      #if C_JAC
    case P_JAC:  { unsigned outlen; arith_compress_O0(in, inlen, &outlen, out); return outlen; } 
      #endif

	  #if C_JRANS
    case P_JRANS4_16o0: { unsigned int outlen = outsize; return rans_compress_to(16,in, inlen, out, &outlen,0) ? outlen : 0;}
    case P_JRANS4_16o1: { unsigned int outlen = outsize; return rans_compress_to(16,in, inlen, out, &outlen,1) ? outlen : 0;} 
    case P_JRANS4_8o0:  { unsigned int outlen = outsize; return rans_compress_to( 8,in, inlen, out, &outlen,0) ? outlen : 0;}
    case P_JRANS4_8o1:  { unsigned int outlen = outsize; return rans_compress_to( 8,in, inlen, out, &outlen,1) ? outlen : 0;}
	    #ifdef AVX2_ON
    case P_JRANS4_32o0: { unsigned int outlen = outsize; return rans_compress_to_32x16(in, inlen, out, &outlen,0) ? outlen : 0;}  //rans_compress_to_4x16(in, in_size, out, out_size, order)
    case P_JRANS4_32o1: { unsigned int outlen = outsize; return rans_compress_to_32x16(in, inlen, out, &outlen,1) ? outlen : 0;} 
	    #endif
	  #endif

      #if C_PPMDEC
	case P_PPMDEC:  return ppmdenc(in, inlen, out);
	  #endif
       
      #if C_POLHF
    case P_POLHF:   return polarenc(in, inlen, out); 
      #endif

      #if C_SHRC
    case P_SHRC:    return rcshc(in, inlen, out); 
    case P_SHRCV:   return vecrcenc(in, inlen, out);
      #endif

      #if C_SUBOTIN 
    case P_SUB:     return subenc(in, inlen, out, inlen, SYMNUM);
      #endif
  
      #if C_TORNADOHF 
    case P_TORNADOHF:     return torhenc(in, inlen, out, outsize); 
	  #endif
      
	  #if C_ZLIBH
    case P_ZLIBH:   return ZLIBH_compress((char* )out, (const char*)in, inlen); 
	  #endif

      #ifdef LZTURBO  
    #include "../dev/x/beplugc.c"
      #endif
	  
      #if C_MYCODEC 
//   case P_MYCODEC:   return mycomp(in, inlen, out, outsize);
	  #endif	
 
    defaulf: fprintf(stderr, "library '%d' not included\n", codec);
  } 
  return 0;
} 
  
int coddecomp(unsigned char *in, int inlen, unsigned char *out, int outlen, int codec, int lev, char *prm) {	
  switch(codec) {
      #if C_AOM   	
    case P_AOM:     aomdec(in, inlen, out, outlen); return outlen;
	  #endif

  	  #if C_BALZ
	case P_BALZ: return balzdecompress(in, inlen, out, outlen);
      #endif

	  #if C_BCM
    case P_BCM: return bcmdecompress(in, inlen, out, outlen);
      #endif 

      #if C_C_BLOSC2
    case P_C_BLOSC2: return blosc_decompress(in, out, outlen);
      #endif

      #if C_C_BLOSC2LZ
    case P_C_BLOSC2LZ: return blosclz_decompress(in, inlen, out, outlen);
      #endif
	  
	  #if C_BRIEFLZ
	case P_BRIEFLZ:     return blz_depack(in, out, outlen);
	  #endif
	  
      #if C_BROTLI
    case P_BROTLI: { BrotliDecoderState* s = BrotliDecoderCreateInstance(NULL, NULL, NULL); if(!s) return -1;
		  BrotliDecoderSetParameter(s, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1u); 
		  size_t total_out, available_in=inlen,available_out=outlen;
		  int rc = BrotliDecoderDecompressStream(s, &available_in, &in, &available_out, &out, &total_out); 
          BrotliDecoderDestroyInstance(s);
          return rc?total_out:0;//size_t dsize = outlen; int rc = BrotliDecoderDecompress(inlen,in,&dsize,out); 
	  }
	  #endif

      #if C_LIBBSC
	case P_LIBBSC:	   return bsc_decompress(in, inlen, out, outlen, BSC_MODE);
	case P_LIBBSCC:	   return bsc_coder_decompress(in, out, lev, BSC_MODE);
	  #endif
	
	  #if C_LIBDEFLATE
	case P_LIBDEFLATE:  { size_t rc; struct libdeflate_decompressor *dd = libdeflate_alloc_decompressor(); 
            if(prm && *prm=='d') outlen = libdeflate_deflate_decompress(dd, in, inlen,out, outlen, &rc);
       else if(prm && *prm=='g') outlen = libdeflate_gzip_decompress(   dd, in, inlen,out, outlen, &rc);
       else                      outlen = libdeflate_zlib_decompress(   dd, in, inlen,out, outlen, &rc);
	   libdeflate_free_decompressor(dd); 
       return outlen;
	  }
	  #endif

      #if C_BZIP2 	  
 	case P_BZIP2: { unsigned outsize = outlen; return BZ2_bzBuffToBuffDecompress((char *)out, &outsize, (char *)in, inlen, 0, 0)==BZ_OK?outlen:-1; }
      #endif
	  
	  #if C_CHAMELEON
    case P_CHAMELEON:  { Chameleon_Reset((Chameleon *)workmem); lev<2?Chameleon_Decode((Chameleon *)workmem, out, outlen, in):Chameleon2_Decode((Chameleon *)workmem, out, outlen, in); return inlen; }
	  #endif

      #if C_CSC
    case P_CSC: { 
        CSCProps prop; CSCDec_ReadProperties(&prop, (uint8_t*)in);
        MemISeqInStream  si; si.s.Read  = (int(*)(void *, void *, size_t *))cscread;  si.in  = in + CSC_PROP_SIZE; si.inlen  = inlen - CSC_PROP_SIZE;
	    MemISeqOutStream so; so.s.Write = (size_t(*)(void *, const void *, size_t  ))cscwrite; so.out = out;                so.outlen = 0;
	    CSCDecHandle dh = CSCDec_Create(&prop, (ISeqInStream*)&si, NULL); CSCDec_Decode(dh, (ISeqOutStream*)&so, NULL); CSCDec_Destroy(dh);
        return si.inlen;
      }
      #endif

     #if C_CRUSH
    case P_CRUSH: crush_decompress(in, out, outlen); break;
	  #endif
	  
      #if C_DAALA
    case P_DAALA:   daaladec(in, inlen, out, outlen); return outlen;
	  #endif

      #if C_DENSITY
    case P_DENSITY: { density_processing_result rs = density_decompress((uint8_t *)in, inlen, (uint8_t*)out, outlen/*+DENSITY_MINIMUM_OUTPUT_BUFFER_SIZE*/);  return rs.state?0:rs.bytesWritten; }
      #endif

      #if C_FASTARI
    case P_FASTARI:  { size_t outsize = outlen; return fa_decompress((const unsigned char *)in, out, inlen, &outsize, workmem)?0:inlen; }
      #endif

      #if C_DOBOZ
	case P_DOBOZ:   { doboz::Decompressor d; return d.decompress(in, inlen, out, outlen) == doboz::RESULT_OK ? outlen : -1; }
      #endif

	  #if C_FASTLZ
    case P_FASTLZ: fastlz_decompress(in, inlen, out, outlen); break;
	  #endif
	   
 	  #if C_GIPFELI
    case P_GIPFELI: { 
	    util::compression::Compressor *c = util::compression::NewGipfeliCompressor();
        util::compression::UncheckedByteArraySink sink((char*) out);
        util::compression::ByteArraySource         src((const char*)in, inlen);
        outlen = c->UncompressStream(&src, &sink); delete c; return outlen;
	  }  
      break;
	  #endif

	  #if C_GLZA
    case P_GLZA: { size_t outsize; GLZAdecode(inlen, (uint8_t *)in, &outsize, (uint8_t *)out, (FILE *)0); break; }
	  #endif

      #if C_HEATSHRINK
    case P_HEATSHRINK: return hsdecompress(in, inlen, out, outlen); 
      #endif

      #if C_IGZIP
    case P_IGZIP: { struct inflate_state s; int rc; isal_inflate_init(&s);
         /*    if(prm && *prm == 'd') { s.crc_flag = ISAL_DEFLATE; }
        else if(prm && *prm == 'g') { s.crc_flag = ISAL_GZIP; }
        else s.crc_flag = ISAL_ZLIB;*/
	    s.next_in  = in;  s.avail_in  = inlen;
	    s.next_out = out; s.avail_out = outlen;
	    if((rc = isal_inflate_stateless(&s)) != ISAL_DECOMP_OK) die("igzip error. rc=%d\n", rc);
      } break;
      #endif

	  #if C_LIBLZF 
    case P_LIBLZF: lzf_decompress(in, inlen, out, outlen); break;
	  #endif

	  #if C_LZ4
    case P_LZ4: LZ4_decompress_safe((const char *)in, (char *)out, inlen, outlen); break;
      #endif
	  
	  #if C_LIZARD
    case P_LIZARD: return Lizard_decompress_safe((const char *)in, (char *)out, inlen, outlen); 
      #endif
	  
      #if C_LZFSE
    case P_LZFSE : return lzfse_decode_buffer(out, outlen, in, inlen, workmem); 
      #endif

      #if C_LZFSEA
    case P_LZFSEA : return compression_decode_buffer(out, outlen, in, inlen, workmem, COMPRESSION_LZFSE);
      #endif

	  #if C_LIBLZG
	case P_LIBLZG: LZG_Decode(in, inlen, out, outlen); break;
	  #endif
	  
      #if C_LIBZPAQ
    case P_LIBZPAQ: { zin = in; zin_ = in+inlen; zout = out; libzpaq::decompress(&zmemin, &zmemout); return zin - in; }
      #endif

      #if C_LZHAM
    case P_LZHAM: { lzham_decompress_params p; memset(&p, 0, sizeof(p)); p.m_struct_size = sizeof(p);
      p.m_dict_size_log2    = in[0];
      size_t outl           = outlen;
	  lzham_uint32 adler32  = 0;																
      return lzham_decompress_memory(&p, (lzham_uint8*)out, &outl, in+1, inlen, &adler32) == LZHAM_COMP_STATUS_SUCCESS?outlen:-1;	
	}
      #endif
	  
      #if C_LZLIB
	case P_LZLIB: { long out_len = outlen; bbdecompress( in, outlen, out, &out_len ); } break;
      #endif 
	  
      #if C_FLZMA2
    case P_FLZMA2: { return FL2_decompress(out, outlen,  in, inlen); }
      #endif

	  #if C_LZMA
	case P_LZMA: {  
	    SizeT ol = outlen, il = inlen - LZMA_PROPS_SIZE; ELzmaStatus sts;
	    return LzmaDecode(out, &ol, in+LZMA_PROPS_SIZE, &il, in, LZMA_PROPS_SIZE, LZMA_FINISH_END, &sts, &g_Alloc)?0:inlen;
      }
      #endif

	  #if C_LZMAT
	case P_LZMAT:  { MP_U32 rc = outlen; lzmat_decode(out, &rc, in, inlen); return rc; }  
	  #endif
	    
	  #if C_LZO
    case P_LZO1x: { lzo_uint out_len;   lzo1x_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1b: { lzo_uint out_len;   lzo1b_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1c: { lzo_uint out_len;   lzo1c_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1f: { lzo_uint out_len;   lzo1f_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1y: { lzo_uint out_len;   lzo1y_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1z: { lzo_uint out_len;   lzo1z_decompress(in, inlen, out, &out_len, NULL); break; }    
    case P_LZO2a: { lzo_uint out_len;   lzo2a_decompress(in, inlen, out, &out_len, NULL); break; }
      #endif 

	  #if C_LZOMA
	case P_LZOMA:   lzomaunpack(in, inlen, out, outlen);break;
      #endif

      #if _MINIZ
    case P_MINIZ: { uLongf outsize = outlen; int rc = mz_uncompress(out, &outsize, in, inlen); } break;
      #endif

      #if C_NAKA
    case P_NAKA:    return NakaDecompress((char *)out, (char *)in, inlen); 
      #endif
	  #if C_PITHY
    case P_PITHY: return pithy_Decompress((const char *)in, inlen, (char *)out, outlen);
      #endif  	

      #if C_QUICKLZ
    case P_QUICKLZ: { lev= (in[0]>>2)&3; outlen = lev<=1?qlz_decompress1((char*)in, out, workmem):(lev<=2?qlz_decompress2((char*)in, out, workmem):qlz_decompress3((char*)in, out, workmem)); } break;
      #endif
	  
	  #if C_SAP
    case P_SAP: { CsObjectInt d; SAP_INT bytes_read, bytes_written; 
	    d.CsInitDecompr((SAP_BYTE *)in); in += CS_HEAD_SIZE; d.CsDecompr( (SAP_BYTE *)in, inlen-CS_HEAD_SIZE, (SAP_BYTE *)out, outlen, lev, &bytes_read, &bytes_written );			 
	    return inlen-CS_HEAD_SIZE; 
	  }
      #endif  	

      #if C_ZLING
    case P_ZLING: zling_decompress(in, inlen, out, outlen); break;
      #endif

	  #if C_LZSS
    case P_LZSS: lzss_decode(in, out, outlen); break;
	  #endif

	  #if C_LZSSE
        #ifdef __x86_64__
	case P_LZSSE2: return LZSSE2_Decompress(in,inlen,out,outlen);
	case P_LZSSE4: return LZSSE4_Decompress(in,inlen,out,outlen);
	case P_LZSSE8: return LZSSE8_Decompress(in,inlen,out,outlen);
	    #endif
	  #endif

      #if C_MSCOMPRESS
     case P_MSCOMPRESS: { size_t _outlen=outlen; return ms_decompress((MSCompFormat)lev, in, inlen, out, &_outlen)==MSCOMP_OK?inlen:0; }
      #endif	 

	  #if C_SHOCO
    case P_SHOCO:     shoco_decompress((const char *)in, inlen, (char *)out, outlen); return inlen;
      #endif  	

	  #if C_SHRINKER
    case P_SHRINKER:    shrinker_decompress(in, out, outlen); break;
	  #endif
	   
	  #if C_SNAPPY
        #if __cplusplus
    case P_SNAPPY: snappy::RawUncompress((char*)in, inlen, (char*)out);  break;
        #else 
    case P_SNAPPY: snappy_uncompress(in, inlen, out, &outlen);	
        #endif
	  #endif	
	  
	  #if C_SNAPPY_C 
	case P_SNAPPY_C: return snappy_uncompress((const char *)in, inlen, (char *)out);
	  #endif  
	  
      #if C_TORNADO
    case P_TORNADO:     return tordecompress(in, out, inlen, outlen);
      #endif
	  
      #if C_WFLZ
    case P_WFLZ:    wfLZ_Decompress( in, out); return inlen;
	  #endif

      #if C_WIMLIB
    case P_WIMLIB: { struct wimlib_decompressor *decompressor; if(wimlib_create_decompressor(lev, 32*1024/*inlen*/, &decompressor)) return 0;
        outlen = wimlib_decompress(in, inlen, out, outlen, decompressor);  wimlib_free_decompressor(decompressor); return inlen;
      }
	  #endif
 
      #if C_WKDM
    case P_WKDM:    WKdm_decompress ((WK_word*)in, (WK_word*)out, outlen);
	  #endif 

	  #if C_XPACK
	case P_XPACK:  { size_t rc; struct xpack_decompressor *dd = xpack_alloc_decompressor(); 
	    outlen = xpack_decompress(dd, in, inlen,out, outlen, &rc); 
	    xpack_free_decompressor(dd); return outlen;
	  } 
	  #endif

      #if C_YALZ77
    case P_YALZ77: { lz77::decompress_t d; std::string extra; if(!d.feed(in,in+inlen,extra) || extra.size() > 0) return 0;
        const std::string& os = d.result(); memcpy(out, os.c_str(), os.size()); return os.size(); 
	  }
	  #endif

	  #if C_YAPPY
	case P_YAPPY:    return YappyUnCompress(in, in+inlen, out)-out; 
      #endif
	  
	  #if C_LIBSLZ
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

      #if C_ZLIB
    //case P_IGZIP: case P_LIBDEFLATE:
    case P_ZLIB: case P_ZOPFLI: { uLongf outsize = outlen; int rc = uncompress(out, &outsize, in, inlen); } break;
      #endif

      #if C_ZSTD
    case P_ZSTD: ZSTD_decompress( out, outlen, in, inlen); break;
      #endif
      //------------ Encoding -----------------------------------------------------------------------
      #if C_RLE
    case P_RLES:
      switch(lev) {
        case  0: return  srled( in, inlen, out, outlen); 
        case  8: return  srled8( in, inlen, out, outlen, _ESC8); 
        case 16: return  srled16(in, inlen, out, outlen, _ESC16); 
        case 32: return  srled32(in, inlen, out, outlen, _ESC32);
        case 64: return  srled64(in, inlen, out, outlen, _ESC64);
      } break;
    case P_RLET: return trled(in, inlen, out, outlen); 
    case P_RLEM: return mrled(in, out, outlen); 
      #endif
	  #if C_B64
    case P_B64_AVX2:  { size_t outlen; base64_decode(in, inlen, out, &outlen, BASE64_FORCE_AVX2);  return inlen; }
    case P_B64_PLAIN: { size_t outlen; base64_decode(in, inlen, out, &outlen, BASE64_FORCE_PLAIN); return inlen; }
    case P_B64_SSSE3: { size_t outlen; base64_decode(in, inlen, out, &outlen, BASE64_FORCE_SSSE3); return inlen; } 
    case P_B64_SSE41: { size_t outlen; base64_decode(in, inlen, out, &outlen, BASE64_FORCE_SSE41); return inlen; }
	  #endif

      //------------ Transform -----------------------------------------------------------------------
	  #if C_TB64 
	case P_TB64: return turbob64dec(in, inlen, out);
	case P_TB64S: return turbob64decs(in, inlen, out); 
      #endif

      #if C_DIVBWT
    case P_DIVBWT: memcpy(out, in+4, outlen); bsc_bwt_decode(out, outlen, *(unsigned *)in, 0, NULL, 0); return inlen;
    case P_ST: {   memcpy(out, in+4, inlen-4); bsc_st_decode(out, inlen-4, lev, *(unsigned *)(in), 0); break; }
      #endif
	  #if C_FB64
        #ifdef AVX2_ON
	case P_FB64AVX:      { size_t _outlen = outlen; fast_avx2_base64_decode(out, in,inlen);return inlen; }
	    #endif

	case P_FB64CHROMIUM:  chromium_base64_decode( (char*)out,(const char*)in,inlen); return inlen;
	case P_FB64LINUX:     linux_base64_decode(    (char*)out,(const char*)in,(const char*)in+inlen); return inlen;
	case P_FB64QUICKTIME: quicktime_base64_decode((char*)out,(const char*)in);         return inlen;
	case P_FB64SCALAR:   { size_t _outlen = outlen; scalar_base64_decode(	 (const char*)in,inlen,(char*)out,&_outlen);return inlen; }
	  #endif

	  #if C_SB64
	case P_SB64SSE:  base64::sse::decode(base64::sse::lookup_base, in, inlen, out); return inlen;
	  #endif
  
      //------------ Entropy Coders ------------------------------------------------------------------
      #if C_MEMCPY 
    case P_MCPY:     memcpy(out, in, inlen);  break;
    case P_LMCPY: libmemcpy(out, in, outlen); break;
      #endif

      #if C_BCMEC
	case P_BCMEC:   return bcmdec(in, inlen, out, outlen);
	  #endif

	  #if C_FASTAC
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

	  #if C_FASTHF
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
	  
      #if C_FPC
    case P_FPC:  return FPC_decompress(out,outlen,in,inlen); 
      #endif

	  #if C_FSC
    case P_FSC:     { size_t outsize = 0; uint8_t *op = NULL; int ok = FSCDecode(in, inlen, &op, &outsize); if(ok) { memcpy(out,op,outlen); free(op); } } break;
	  #endif
 
	  #if C_FSE
    case P_FSE:  if(inlen == outlen) memcpy(out, in, outlen); else if(inlen == 1) memset(out,in[0],outlen); else FSE_decompress(out, outlen, in, inlen); break;
    case P_FSEH: if(inlen == outlen) memcpy(out, in, outlen); else if(inlen == 1) memset(out,in[0],outlen); else HUF_decompress(out, outlen, in, inlen); break;
	  #endif
		
      #if C_FQZ
	case P_FQZ0: uncompress_block(in, out, outlen); break;
	  #endif

      #if C_JAC
    case P_JAC:  { unsigned outlen; arith_uncompress_O0(in, inlen, &outlen, out); } break;
      #endif

	  #if C_JRANS
    case P_JRANS4_16o0: rans_uncompress_to(16,in, inlen, out, (unsigned int *)&outlen,0); break;
    case P_JRANS4_16o1: rans_uncompress_to(16,in, inlen, out, (unsigned int *)&outlen,1); break;
    case P_JRANS4_8o0:  rans_uncompress_to( 8,in, inlen, out, (unsigned int *)&outlen,0); break;
    case P_JRANS4_8o1:  rans_uncompress_to( 8,in, inlen, out, (unsigned int *)&outlen,1); break; 	
	    #ifdef AVX2_ON	 
    case P_JRANS4_32o0: rans_uncompress_to_32x16(in, inlen, out, (unsigned int *)&outlen,0); break;
    case P_JRANS4_32o1: rans_uncompress_to_32x16(in, inlen, out, (unsigned int *)&outlen,1); break;
	    #endif
	  #endif

      #if C_FPAQC
    case P_FPAQC:   absd(in, outlen, out); break;
      #endif

      #if C_MMRC     
    case P_O0ZP:    rczmmd(in, outlen, out); break;
    case P_FPAQ0F2: rcmm2d(in, outlen, out); break;
    case P_MMRC:    rcmmd( in, outlen, out); break;
      #endif

	  #if C_NANS
    case P_NANS:    nansdec(in, inlen, out, outlen); break;
	  #endif

      #if C_PPMDEC
	case P_PPMDEC:  return ppmddec(in, inlen, out, outlen);
	  #endif

      #if C_POLHF
    case P_POLHF:   polardec(in, out, outlen); break;
      #endif

      #if C_SHRC
    case P_SHRCV:   vecrcdec(in, outlen, out); break;
    case P_SHRC:    rcshd(in, out, outlen); break;
      #endif

      #if C_SUBOTIN
    case P_SUB:     subdec(in, outlen, out, outlen, SYMNUM); break;
      #endif

      #if C_TORNADOHF
    case P_TORNADOHF:    torhdec(in, inlen, out, outlen); break;
      #endif 

	  #if C_ZLIBH		
    case P_ZLIBH:  return ZLIBH_decompress((char* )out, (const char*)in);
	  #endif	

      #ifdef LZTURBO  
    #include "../dev/x/beplugd.c"
      #endif

      #if C_MYCODEC 
//   case P_MYCODEC:   return mydecomp(in, inlen, out, outlen);
	  #endif	
  }
  return 0;
}

char *codver(int codec, char *v, char *s) {
  switch(codec) {  
      #if C_C_BLOSC2
    return BLOSC_VERSION_STRING;
      #endif 
	  #if C_BRIEFLZ
    case P_BRIEFLZ: sprintf(s,"%d.%d.%d", BLZ_VER_MAJOR, BLZ_VER_MINOR, BLZ_VER_PATCH); return s;
	  #endif

	  #if C_LZ4
    case P_LZ4:     sprintf(s,"%d.%d.%d", LZ4_VERSION_MAJOR, LZ4_VERSION_MINOR, LZ4_VERSION_RELEASE); return s;
	  #endif

	  #if C_LIZARD
    case P_LIZARD:     sprintf(s,"%d.%d.%d", LIZARD_VERSION_MAJOR, LIZARD_VERSION_MINOR, LIZARD_VERSION_RELEASE); return s;
	  #endif

	  #if C_ZSTD
    case P_ZSTD:    sprintf(s,"%d.%d.%d", ZSTD_VERSION_MAJOR, ZSTD_VERSION_MINOR, ZSTD_VERSION_RELEASE); return s;
      #endif

	  #if C_DENSITY
    case P_DENSITY: sprintf(s,"%d.%d.%d", density_version_major(), density_version_minor(), density_version_revision()); return s;
	  #endif

      #if C_HEATSHRINK
    case P_HEATSHRINK: sprintf(s,"%d.%d.%d", HEATSHRINK_VERSION_MAJOR, HEATSHRINK_VERSION_MINOR, HEATSHRINK_VERSION_PATCH); return s;
      #endif

	  #if C_SNAPPY
    case P_SNAPPY:  sprintf(s,"%d.%d.%d", SNAPPY_MAJOR, SNAPPY_MINOR, SNAPPY_PATCHLEVEL); return s;
	  #endif
	default:        strcpy(s,v);
  }
  return s;
}

