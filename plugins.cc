/**
    Copyright (C) powturbo 2013-2016
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
#define _COMP1 0
  #else
#define _COMP1 1
  #endif

  #ifdef NCOMP2
#define _COMP2 0
  #else
#define _COMP2 1
  #endif

  #ifdef NECODER
#define _ECODER 0
  #else
#define _ECODER 1
  #endif

  #ifdef NENCOD
#define _ENCOD 0
  #else
#define _ENCOD 1
  #endif

  #ifdef NTRANSFORM
#define _TRANFORM 0
  #else
#define _TRANSFORM 1
  #endif

  #ifdef GPL
#define _GPL 1
  #else
#define _GPL 0
  #endif

enum { 
#define _MEMCPY		1
 P_LMCPY,   // must be 0
 P_MCPY,	// must be 1

#define _BALZ 		_COMP2		
 P_BALZ,
#define _BCM 		_COMP2	
 P_BCM,
#define _BRIEFLZ 	_COMP2	
 P_BRIEFLZ, 
#define _BROTLI		_COMP1
 P_BROTLI,    
#define _BZIP2 		_COMP2
 P_BZIP2, 
#define _CHAMELEON  _COMP2    
 P_CHAMELEON,
#define _C_BLOSC2	_COMP2
 P_C_BLOSC2,
#define _CRUSH	 	_COMP2	
 P_CRUSH, 
#define _CSC        _COMP2    
 P_CSC, 
#define _DENSITY    _COMP2	
 P_DENSITY, 
#define _DOBOZ		_COMP2	 //crash
 P_DOBOZ, 
#define _FASTLZ 	_COMP2	
 P_FASTLZ, 
#define _GIPFELI    _COMP2	 
 P_GIPFELI,
#define _HEATSHRINK _COMP2    
 P_HEATSHRINK,
#define _LIBBSC     _COMP2    
 P_LIBBSC, P_LIBBSC_ST,  
#define _LIBLZF     _COMP2   	
 P_LIBLZF,  
#define _LIBLZG		_COMP2	  
 P_LIBLZG,
#define _LIBZPAQ	_COMP2	
 P_LIBZPAQ, 
#define _LZ4		_COMP1 			
 P_LZ4,
#define _LZ5 		_COMP2	
 P_LZ5,    
#define _LZHAM		_COMP1
 P_LZHAM,
#define _LZFSE      0 // disabled
 P_LZFSE,
#define _LZLIB 		_GPL
 P_LZLIB, 
#define _LZMA		_COMP1 			
 P_LZMA, 
#define _LZMAT 		_GPL
 P_LZMAT,
#define _LZO		_GPL		
 P_LZO1b, P_LZO1c, P_LZO1f, P_LZO1x, P_LZO1y, P_LZO1z, P_LZO2a,
#define _LZOMA		0 //_GPL
 P_LZOMA, 
#define _MINIZ   	_COMP2
 P_MINIZ,
#define _MSCOMPRESS	_GPL  
 P_MSCOMPRESS, 
#define _NAKA	    _COMP2
 P_NAKA,
#define _PITHY   	_COMP2    
 P_PITHY, 
#define _QUICKLZ	_GPL
 P_QUICKLZ, 
#define _SHRINKER	_COMP2   	
 P_SHRINKER, 
#define _SAP		_GPL
 P_SAP, 
#define _SHOCO      _COMP2    
 P_SHOCO,
  #ifdef SNAPPY
#define _SNAPPY		_COMP2
  #else		
#define _SNAPPY		0
  #endif
 P_SNAPPY, 
#define _SNAPPY_C	_COMP2		
 P_SNAPPY_C, 
#define _TORNADO	_GPL	 
 P_TORNADO,  
#define _WFLZ 		_COMP2    		    
 P_WFLZ,
#define _YALZ77     _COMP2     
 P_YALZ77,
#define _YAPPY      _COMP2   
 P_YAPPY,
#define _ZLIB		_COMP1	
 P_ZLIB, 
#define _ZLING	    _COMP2
 P_ZLING,  
#define _ZOPFLI		_COMP2	
 P_ZOPFLI, 
#define _ZSTD		_COMP1	
 P_ZSTD,
  // --------- Encoding -------------------
#define _RLE        _ENCOD
 P_RLES,
 P_RLET,
  //---------- Transform ------------------
#define _DIVBWT     _LIBBSC //_TRANSFORM
 P_DIVBWT,
  // --------- Entropy coders -------------
 #if _BCM  
#define _BCMEC      _ECODER 
 #else
#define _BCMEC      0
 #endif
 P_BCMEC,
#define _FASTAC		_ECODER    
 P_FASTAC,
#define _FASTARI	_ECODER    
 P_FASTARI,
#define _FASTHF		_ECODER    
 P_FASTHF,
#define _FSC 		_ECODER    
 P_FSC,
#define _FSE 		_ECODER    
 P_FSE, 
 P_FSEH, 
#define _FQZ 	 	_ECODER    
 P_FQZ0,
#define _JAC 	    _ECODER    
 P_JAC, 
#define _JRANS		_ECODER
 P_JRANS,
 P_JRANS1,
 P_JRANS64,
 P_JRANS641,
#define _FPAQC 		_GPL
 P_FPAQC,
#define _NANS       _ECODER
 P_NANS,      
#define _PPMDEC	 	_ECODER       
 P_PPMDEC,
#define _SHRC       _ECODER
 P_SHRC,
 P_SHRCV,
#define _SUBOTIN    _ECODER    
 P_SUB,
#define _POLHF      _GPL
 P_POLHF, 
#define _TORNADOHF	_GPL	 
 P_TORNADOHF,
#define _ZLIBH		_ECODER    
 P_ZLIBH,
#define _MYCODEC    _COMP2	// Include your codec into TurboBench :search and modify "_MYCODEC" in the following files: plugins.h, plugreg.c. Insert your function calls like mycomp/mydecomp in plugins.cc 
 P_MYCODEC, // User plugin
  #ifdef LZTURBO
#include "../beplug.h"
  #endif
}; 
   
  #ifdef NCPP  
#define _BALZ 		0
#define _BCM		0			
#define _BROTLI		0
#define _CSC 		0
#define _DOBOZ		0
#define _GIPFELI	0
#define _SNAPPY		0	
#define _YALZ77     0     
#define _ZOPFLI		0	
#define _ZLING	    0	
#define _TORNADO	0
#define _YAPPY 		0

#define _BCMEC		0
#define _FASTAC		0
#define _FASTHF		0    
#define _TORNADOHF	0 
  #endif

  #ifdef _WIN32
#define _FSC 		0
#define _GIPFELI	0
#define _C_BLOSC2 	0
  #endif
//-------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <time.h>
#include "plugins.h"

  #if _BALZ
#include "balz/balz.h"
  #endif

  #if _BCM
#include "bcm/bcm.h"
  #endif

  #if _C_BLOSC2
    #ifdef _C_BLOSC2LZ
#include "c-blosc2/blosc/blosclz.h"
    #else
#include "c-blosc2/blosc/blosc.h"
    #endif
  #endif
  
  #if _BRIEFLZ 
#include "brieflz/brieflz.h"
  #endif
  
  #if _LIBBSC
#include "libbsc/libbsc/libbsc.h"
#include "libbsc/libbsc/st/st.h"
#include "libbsc/libbsc/lzp/lzp.h"
  #endif

  #if _BZIP2 
#include "bzip2/bzlib.h"
  #endif
  
  #if _BROTLI
#include "brotli/enc/encode.h"
#include "brotli/dec/decode.h"
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

  #if _CRUSH
#include <stdint.h>
#include "crush/crush.hpp"
  #endif

  #if _DOBOZ
#include "doboz/Source/Doboz/Compressor.h"
#include "doboz/Source/Doboz/Decompressor.h"
  #endif

  #if _GIPFELI
#include "gipfeli/gipfeli.h"
  #endif

  #if _HEATSHRINK
#include "heatshrink_/heatshrink.h"
  #endif

  #if _LIBLZG
#include "liblzg/src/include/lzg.h"
  #endif

  #if _LIBZPAQ
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
  #endif
    
  #if _LZ5
#include "lz5/lib/lz5.h"
#include "lz5/lib/lz5hc.h"
  #endif
    
  #if _LZFSE
#include <compression.h>
  #endif  

  #if _LZHAM
#include "lzham_codec_devel/include/lzham.h"
  #endif
 
  #if _LZLIB
#include "lzlib_/bbexample.h"
struct Lzma_options  {
  int dictionary_size;		// 4 KiB .. 512 MiB
  int match_len_limit;		// 5 .. 273
};
static const Lzma_options option_mapping[] =  {
  { 1 << 16,  16 },		// -0 entry values not used
  { 1 << 20,   5 },		// -1
  { 3 << 19,   6 },		// -2
  { 1 << 21,   8 },		// -3
  { 3 << 20,  12 },		// -4
  { 1 << 22,  20 },		// -5
  { 1 << 23,  36 },		// -6
  { 1 << 24,  68 },		// -7
  { 3 << 23, 132 },		// -8
  { 1 << 25, 273 } };	// -9
  #endif
  
  #if _LZMA
#include "LZMA-SDK/C/Alloc.h"
#include "LZMA-SDK/C/LzmaEnc.h"
#include "LZMA-SDK/C/LzmaDec.h"
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

  #if _MSCOMPRESS
#include "ms-compress/include/mscomp.h"
  #endif  

  #if _LZSS
#include "lzss/lzss.h"
  #endif  

  #ifdef _NAKA
#include "nakamichi/nakamichi.h"
  #endif

  #if _PITHY
#include "pithy/pithy.h"
  #endif

  #if _QUICKLZ
#include "quicklz_/quicklz-c.h"
  #endif

  #if _SAP
#include "pysap/pysapcompress/hpa101saptype.h"
#include "pysap/pysapcompress/hpa104CsObject.h"
#include "pysap/pysapcompress/hpa106cslzc.h"
#include "pysap/pysapcompress/hpa107cslzh.h"
#include "pysap/pysapcompress/hpa105CsObjInt.h"
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

  #if _WKDM
#include "wkdm/WKdm.h"
  #endif
  
  #if _WFLZ
#include "wflz/wfLZ.h"
  #endif
  
  #if _WIMLIB
#include "../wimlib/include/wimlib.h"
  #endif
  
  #if _YALZ77
#include "yalz77/lz77.h"
  #endif

  #if _YAPPY
#include "yappy/yappy.hpp"
  #endif

  #if _ZLIB
     #if _ZLIBLIB
#include <zlib.h>
     #elif defined(_ZLIB_INTEL)
#include "zlib_intel/zlib.h"
     #else
#include "zlib/zlib.h"
     #endif
  #endif
  
  #if _ZLING
#include "libzling/src/libzling.h"
#include "libzling_/libzling_utils_mem.h"
  #endif  

  #if _ZSTD
#include "zstd/lib/zstd.h"
  #endif

  #ifdef LZTURBO
#include "../beplugi.h"
  #endif  
//-----------------------------
  #if __cplusplus  
extern "C" {
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

  #if _MINIZ
typedef unsigned long mz_ulong;
int mz_compress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len, int level);
int mz_uncompress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len);
  #endif

  #if _SNAPPY_C
#include "snappy-c/snappy.h"
struct snappy_env env; 
  #endif
 
  #if _ZOPFLI
#include "zopfli/src/zopfli/zopfli.h"  
  #endif

  #if __cplusplus
}
  #endif
  //------------------------------------ Encoding -----------------------------------
  #ifdef _RLE
#include "TurboRLE/trle.h"
  #endif
  //------------------------------------ Transform ----------------------------------
  #ifdef _DIVBWT 
#include "libbsc/libbsc/bwt/divsufsort/divsufsort.h"
#include "libbsc/libbsc/bwt/bwt.h"
  #endif
  //------------------------------------ Entropy Coder ------------------------------
  #if _FASTAC
#include "fastac/arithmetic_codec.h"
  #endif
  
  #if _FASTHF
#include "fasthf/binary_codec.h"
  #endif
  
  #if _FSE
#include "zstd/lib/fse.h"
#include "zstd/lib/huff0.h"
  #endif

  #if _FQZ
#include "fqz0/f_o0.h"
  #endif

  #if _JAC
#include "ans_jb/arith_static.h"
  #endif
  
  #if _JRANS
#include "ans_jb/rANS_static4c.h"
    #ifdef __x86_64__
#include "ans_jb/rANS_static64c.h"
    #endif
  #endif

  #if _FPAQC
#include "fpaqc/fpaqc.h"
  #endif
  
  #if _MMRC
#include "fpaq0f2/fpaq0f2.h"
#include "fpaq0p/fpaq0p_mm.h"
#include "o0zp/o0zp.h"
  #endif

  #if _NANS
#include "ans_nania/narans.h"
  #endif

#if _POLHF
  #include "polar/polar.h"
#endif

  #if _PPMDEC
#include "ppmdec/ppmdec.h"
  #endif

  #if _SUBOTIN
#include "subotin_/subotin.h"
  #endif

  #if _SHRC
#include "fpaq0p/fpaq0p_sh.h"
#include "vecrc/vector_rc.h"
  #endif  

  #if _ZLIBH
#include "zlibh/zlibh.h"
  #endif
  
  #if _MYCODEC
//#include "my_header.h"
  #endif

  #if __cplusplus  
extern "C" {
  #endif

  #if _FASTARI
#include "FastARI/FastAri.h"
  #endif

  #if _FSC
#include "fsc/fsc.h"
  #endif

  #if __cplusplus
}
  #endif
//------------------------------------------------- registry -------------------------------------------------------------------------------------------------
struct plugs plugs[] = {
  { P_BALZ, 	"balz", 			_BALZ, 		"1.20",		"balz",					"Public Domain",	"http://sourceforge.net/projects/balz", 												"0,1" }, 
  { P_BCM, 		"bcm", 				_BCM, 		"1.0",		"bcm",					"Public Domain",	"http://sourceforge.net/projects/bcm", 													"" }, 
  { P_C_BLOSC2, "blosc",			_C_BLOSC2, 	"2.0",		"Blosc",				"BSD license",		"https://github.com/Blosc/c-blosc2", 													"0,1,2,3,4,5,6,7,8,9", 64*1024},
  { P_BRIEFLZ,	"brieflz", 		    _BRIEFLZ, 	"1.1.0",	"BriefLz",				"BSD like",			"https://github.com/jibsen/brieflz", 													"" }, 
  { P_BROTLI,	"brotli", 			_BROTLI, 	"16-01",	"Brotli",				"Apache license",	"https://github.com/google/brotli", 													"0,1,2,3,4,5,6,7,8,9,11/DOWX"},
  { P_BZIP2,	"bzip2", 			_BZIP2, 	"1.06",		"Bzip2",				"BSD like",			"http://www.bzip.org/downloads.html\thttps://github.com/asimonov-im/bzip2", 			"" }, 
  { P_CHAMELEON,"chameleon",		_CHAMELEON, "15-03",	"Chameleon",			"Public Domain",	"http://cbloomrants.blogspot.de/2015/03/03-25-15-my-chameleon.html", 					"1,2" },
  { P_CRUSH,	"crush", 			_CRUSH, 	"1.0.0",	"Crush",				"Public Domain",	"http://sourceforge.net/projects/crush", 												"0,1,2" },
  { P_CSC,	    "csc", 				_CSC, 		"16-01",	"CSC",					"Public domain",	"https://github.com/fusiyuan2010/CSC", 													"1,2,3,4,5" },
  { P_DENSITY, 	"density",        	_DENSITY,	"0.12.0",	"Density",				"BSD license",		"https://github.com/centaurean/density",												"1,2,3" },
  { P_DOBOZ,	"doboz",			_DOBOZ, 	"14-01-14",	"Doboz",				"BSD Like",			"https://bitbucket.org/attila_afra\thttps://github.com/nemequ/doboz", 					"" },  //crash on windows
  { P_FASTLZ,	"fastlz", 			_FASTLZ,	"0.1.0",	"FastLz",				"BSD like",			"http://fastlz.org\thttps://github.com/ariya/FastLZ",									"1,2" },
  { P_GIPFELI, 	"gipfeli", 			_GIPFELI, 	"15.12",	"Gipfeli",				"Apache license",	"https://github.com/google/gipfeli",													"" }, 
  { P_HEATSHRINK,"heatshrink",		_HEATSHRINK,"0.4.1",	"heatshrink",			"BSD license",		"https://github.com/atomicobject/heatshrink",											"" },
  { P_LIBBSC_ST,"bsc_st", 			_LIBBSC, 	"3.1.0",	"bsc",					"Apache license",	"https://github.com/IlyaGrebnov/libbsc",												"3,4,5,6,7,8" }, 
  { P_LIBBSC, 	"bsc", 				_LIBBSC, 	"3.1.0",	"bsc",					"Apache license",	"https://github.com/IlyaGrebnov/libbsc",												"1,2"}, 
  { P_LIBLZF, 	"lzf", 				_LIBLZF, 	"1.06",		"LibLZF",				"BSD license",		"http://oldhome.schmorp.de/marc/liblzf.html\thttps://github.com/nemequ/liblzf",			"" },
  { P_LIBLZG,  	"lzg", 				_LIBLZG,    "1.0.8",	"LibLzg",				"zlib-license",		"https://github.com/mbitsnbites/liblzg\thttp://liblzg.bitsnbites.eu/e",					"1,2,3,4,5,6,7,8,9" }, //"https://gitorious.org/liblzg" BLOCKSIZE must be < 64MB
  { P_LIBZPAQ,  "zpaq", 			_LIBZPAQ, 	"7.05",		"Libzpaq",				"Public Domain",	"https://github.com/zpaq/zpaq",															"0,1,2,3,4,5" }, 
  { P_LZ4,  	"lz4",				_LZ4, 		"15-10",	"Lz4",					"BSD license",		"https://github.com/Cyan4973/lz4", 														"0,1,9,12,16" }, 
  { P_LZ5,  	"lz5",				_LZ5, 		"1.3.3",	"Lz5",					"BSD license",		"https://github.com/inikep/lz5",														"0,1,2,3,4,5,6,7,8,9,12,15" }, 
  { P_LZFSE, 	"lzfse", 			_LZFSE, 	"2015",		"lzfse",				"iOS and OS X",		"https://developer.apple.com/library/prerelease/mac/documentation/Performance/Reference/Compression/","" },
  { P_LZHAM, 	"lzham", 			_LZHAM,		"1.1",		"Lzham",				"MIT license",		"https://github.com/richgel999/lzham_codec_devel",										"1,2,3,4/x" }, 
  { P_LZLIB, 	"lzlib", 			_LZLIB, 	"1.7",		"Lzlib",				"GPL license",		"http://www.nongnu.org/lzip\thttps://github.com/daniel-baumann/lzlib",					"1,2,3,4,5,6,7,8,9" },
  { P_LZMAT, 	"lzmat", 			_LZMAT, 	"1.0",		"Lzmat",				"GPL license",		"https://github.com/nemequ/lzmat\thttp://www.matcode.com/lzmat.htm",					"" },
  { P_LZMA,  	"lzma", 			_LZMA, 		"9.35",		"Lzma",					"Public Domain",	"http://7-zip.org\thttps://github.com/jljusten/LZMA-SDK", 								"0,1,2,3,4,5,6,7,8,9" }, 

  { P_LZO1b, 	"lzo1b", 			_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"1,9,99,999" },  
  { P_LZO1c, 	"lzo1c",			_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"1,9,99,999" },
  { P_LZO1f, 	"lzo1f",			_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"1,999" },
  { P_LZO1x, 	"lzo1x", 			_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"1,11,12,15,999" },
  { P_LZO1y,  	"lzo1y",			_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"1,999" }, 
  { P_LZO1z, 	"lzo1z", 			_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"999" }, 
  { P_LZO2a, 	"lzo2a", 			_LZO, 		"2.09",		"Lzo",					"GPL license",		"http://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo",				"999" }, 
  { P_LZOMA, 	"lzoma", 			_LZOMA,		"15-06",	"lzoma",				"GPL license",		"https://github.com/alef78/lzoma", 														"1,2,3",0,(1<<16) },
  { P_MINIZ, 	"miniz", 			_MINIZ,		"15-06",	"miniz zlib-replacement","Public domain",	"https://github.com/richgel999/miniz", 													"1,2,3,4,5,6,7,8,9" },
  { P_MSCOMPRESS,"mscompress", 		_MSCOMPRESS,"15.09",	"ms-compress",			"GPL license",		"https://github.com/coderforlife/ms-compress", 											"2,3,4" }, 
  { P_NAKA, 	"Naka", 			_NAKA,		"15-10",	"Nakamichi Kintaro",	"           ",	    "http://www.overclock.net/t/1577282/fastest-open-source-decompressors-benchmark#post_24538188",	"" },
  { P_PITHY, 	"pithy",			_PITHY, 	"2011",		"Pithy",	  			"BSD license",		"https://github.com/johnezang/pithy",													"0,1,2,3,4,5,6,7,8,9" },
  { P_QUICKLZ, 	"quicklz",			_QUICKLZ, 	"1.5.1",	"Quicklz",	  			"GPL license",		"http://www.quicklz.com\thttps://github.com/robottwo/quicklz",							"1,2,3" },
  { P_SAP, 	    "sap",				_SAP, 		"15-11",	"sap",		  			"GPL license",		"https://github.com/CoreSecurity/pysap",												"0,1,2"	},
  { P_SHRINKER, "shrinker",			_SHRINKER, 	"0.1/r9",	"Shrinker",				"BSD license",		"https://code.google.com/p/data-shrinker",												"", 0, (1<<26) },
  { P_SHOCO,    "shoco",			_SHOCO, 	"2015",		"Shoco",				"MIT license",		"https://github.com/Ed-von-Schleck/shoco",												"" },
  { P_SNAPPY, 	"snappy",			_SNAPPY, 	"1.1.2",	"Snappy",				"Apache license",	"https://github.com/google/snappy"														""	},
  { P_SNAPPY_C, "snappy_c",			_SNAPPY_C,	"1.1.2",	"Snappy-c",				"BSD Like",			"https://github.com/andikleen/snappy-c",												"" },
  { P_TORNADO, 	"tornado", 			_TORNADO, 	"0.6a",		"Tornado",				"GPL license",		"http://freearc.org\thttps://github.com/nemequ/tornado",								"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16" }, 
  { P_WFLZ,	    "wflz", 			_WFLZ, 		"15-04",	"wfLZ",					"CC0 license",		"https://github.com/ShaneWF/wflz",														"1,2" },
//{ P_WKDM, 	"WKdm",				_WKDM, 		"2003",		"WKdm",					"Apple PS License",	"http://www.opensource.apple.com/source/xnu/xnu-1456.1.26/iokit/Kernel/\thttps://github.com/berkus/wkdm", "" }, // crash
  { P_YALZ77, 	"yalz77", 			_YALZ77, 	"15-09",	"Yalz77",				"Public domain",	"https://github.com/ivan-tkatchev/yalz77",												"1,6,12" },
  { P_YAPPY, 	"yappy",			_YAPPY, 	"2011",		"Yappy",				"",					"" ,																					"" },//crash windows
  { P_ZLIB, 	"zlib", 			_ZLIB, 		"1.2.8",	"zlib",					"zlib license",		"http://zlib.net\thttps://github.com/madler/zlib", 										"1,2,3,4,5,6,7,8,9" },
  { P_ZLING, 	"zling", 	   		_ZLING, 	"16-01",	"Libzling",				"BSD license",		"https://github.com/richox/libzling",													"0,1,2,3,4" }, 
  { P_ZOPFLI, 	"zopfli",			_ZOPFLI, 	"15-05",	"Zopfli",				"Apache license",	"https://code.google.com/p/zopfli",														""}, 
  { P_ZSTD, 	"zstd", 			_ZSTD,		"15-12",	"ZSTD",					"BSD license",		"https://github.com/Cyan4973/zstd", 													"1,2,3,4,5,6,7,8,9,12,16,20" },
//-----------------------------------------------------------------------------------	  
  { P_MCPY, 	"imemcpy", 			_MEMCPY, 	".",		"inline memcpy",		"------------",		"--------------------------------------",												"" },
  { P_LMCPY, 	"memcpy",			_MEMCPY,  	".",		"library memcpy",		"",					"",																						"" },
  { P_BCMEC, 	"bcmec", 			_BCMEC, 	"1.0",		"bcm range coder",		"Public Domain",	"http://sourceforge.net/projects/bcm",													"" },
  { P_FSC, 		"fsc", 				_FSC, 		"15-05",	"Finite State Coder",	"Apache license",	"https://github.com/skal65535/fsc",														"", E_ANS },
  { P_FSE, 		"fse", 				_FSE, 		"15-12",	"Finite State Entropy",	"BSD license",		"https://github.com/Cyan4973/FiniteStateEntropy",										"", E_ANS },
  { P_FSEH,		"fsehuf", 			_FSE, 		"15-12",	"Finite State Entropy",	"BSD license",		"https://github.com/Cyan4973/FiniteStateEntropy",										"", E_HUF },
  { P_FPAQC,	"fpaqc", 			_FPAQC, 	"07-12",	"Fpaqc:Asymmetric Binary Coder","GPL license",	"http://www.mattmahoney.net/dc/",														"" },
  { P_SHRC,	    "fpaq0p_sh",		_SHRC, 	    "2010",	    "Bitwise RC",			"           ",		"http://encode.ru/threads/1153-Simple-binary-rangecoder-demo",							"" },
  { P_SHRCV,	"vecrc_sh",			_SHRC, 	    "2012",	    "Bitwise vector RC",	"           ",		"http://encode.ru/threads/1200-Vectorized-rangecoder",									"" },
  { P_FASTHF,  	"FastHF", 			_FASTHF, 	"2006",		"Fast HF",				"BSD like",			"http://www.cipr.rpi.edu/research/SPIHT/",												"" },
  { P_FASTARI,  "FastAri", 			_FASTARI, 	"15-10",	"FastAri",				"MIT license",		"https://github.com/davidcatt/FastARI", 												"" },
  { P_FASTAC,  	"FastAC", 			_FASTAC, 	"2006",		"Fast AC",				"BSD like",			"http://www.cipr.rpi.edu/research/SPIHT/",												"" },
  { P_JAC, 		"arith_static",		_JAC, 		"15-07",	"Range Coder/J.Bonfield","BSD license",		"ftp://ftp.sanger.ac.uk/pub/users/jkb",													"", E_ANS},
  { P_FQZ0, 	"fqz0",				_FQZ, 		"15-03",	"FQZ/PPMD Range Coder",	"           ",		"http://encode.ru/threads/2149-ao0ec-Bytewise-adaptive-order-0-entropy-coder",			""},
  { P_PPMDEC, 	"ppmdec", 			_PPMDEC,	"15-03",	"PPMD Range Coder",		"           ",		"http://encode.ru/threads/2149-ao0ec-Bytewise-adaptive-order-0-entropy-coder",  		""},
  { P_JRANS, 	"rans_static4c",	_JRANS,		"15-08",	"ANS/J.Bonfield",		"Public Domain",	"ftp://ftp.sanger.ac.uk/pub/users/jkb",													"", E_ANS},
  { P_JRANS1, 	"rans_static4c_o1", _JRANS, 	"15-08",	"ANS/J.Bonfield",		"Public Domain",	"ftp://ftp.sanger.ac.uk/pub/users/jkb",													"", E_ANS},
  { P_JRANS64, 	"rans_static64c",	_JRANS, 	"15-08",	"ANS 64/J.Bonfield",	"Public Domain",	"ftp://ftp.sanger.ac.uk/pub/users/jkb",													"", E_ANS},
  { P_JRANS641,	"rans_static64c_o1",_JRANS, 	"15-08",	"ANS 64/J.Bonfield",	"Public Domain",	"ftp://ftp.sanger.ac.uk/pub/users/jkb",													"", E_ANS},
  { P_NANS,	    "naniarans",		_NANS, 	    "2015",	    "Nania Adaptive rANS",	"           ",		"http://encode.ru/threads/2079-nARANS-(Nania-Adaptive-Range-Variant-of-ANS)",			"", E_ANS},
  { P_POLHF,    "polar", 			_POLHF, 	"10-07",	"Polar Codes",			"GPL license",		"http://www.ezcodesample.com/prefixer/prefixer_article.html",							"" },
  { P_SUB, 		"subotin", 			_SUBOTIN, 	"2000",		"subotin RC",			"Public Domain",	"http://ezcodesample.com/ralpha/Subbotin.txt",											"" },
  { P_TORNADOHF,"tornado_huff", 	_TORNADOHF, "0.6a",		"Tornado Huf",			"GPL license",		"http://freearc.org/Research.aspx\thttps://github.com/nemequ/tornado" ,					"" },
  { P_ZLIBH, 	"zlibh",			_ZLIBH, 	"1.2.8",	"zlib Huffmann",		"BSD license",		"https://github.com/Cyan4973/FiniteStateEntropy",										"", E_HUF },
  //---- Encoding ------
  { P_RLES, 	"srle",	    		_RLE, 	    "16-01", 	"TurboRLE ESC",			"            ",		"https://sites.google.com/site/powturbo",  												"0,8,16,32,64" },
  { P_RLET, 	"trle",	    		_RLE, 	    "16-01", 	"TurboRLE",			    "            ",		"https://sites.google.com/site/powturbo",  												"" },
  //----- Transform -----
  { P_DIVBWT, 	"divbwt",    		_DIVBWT,    "",			"bwt libdivsufsort/libbsc",	"        ",		"https://github.com/y-256/libdivsufsort",  												"" },

//{ P_MYCODEC, 	"mycodec",			_MYCODEC, 	"0",		"My codec",				"           ",		"",																						"" },
    #ifdef LZTURBO
  #include "../beplugr.h"
    #endif  
  { -1 }
};

//---------------------------------------------- plugins --------------------------------------------------------
  #ifndef max
#define max(x,y) (((x)>(y)) ? (x) : (y))
  #endif

void libmemcpy(unsigned char *dst, unsigned char *src, int len) {
  void *(*memcpy_ptr)(void *, const void *, size_t) = memcpy;
  if (time(NULL) == 1) 
    memcpy_ptr = NULL;
  memcpy_ptr(dst, src, len);
}

static char _workmem[1<<16],*workmem=_workmem;
static int state_size,dstate_size;
static size_t workmemsize;

int codini(size_t insize, int codec) {
  workmemsize = 0;

  switch(codec) {
      #if _C_BLOSC2
    case P_C_BLOSC2: blosc_init(); blosc_set_nthreads(1);break;
      #endif
      
      #if _FASTARI
    case P_FASTARI: workmemsize = FA_WORKMEM; break;
      #endif

      #if _LZFSE
    case P_LZFSE:
      size_t lzfse_size = compression_encode_scratch_buffer_size(COMPRESSION_LZFSE);
      workmemsize = max(lzfse_size, workmemsize);
      lzfse_size = compression_decode_scratch_buffer_size(COMPRESSION_LZFSE);
      workmemsize = max(lzfse_size, workmemsize);
      break;
      #endif

      #if _LZO
    case P_LZO1b: lzo_init(); workmemsize = LZO1B_999_MEM_COMPRESS; break;
    case P_LZO1c: P_LZO1f: P_LZO1x: P_LZO1y: P_LZO1z: P_LZO2a: lzo_init(); workmemsize = LZO1X_MEM_COMPRESS; break;
      #endif
	
      #if _CHAMELEON
    case P_CHAMELEON: workmemsize = sizeof(struct Chameleon); break;
	  #endif
	
      #if _QUICKLZ
    case P_QUICKLZ:
      state_size  = max(qlz_get_setting1(1), max(qlz_get_setting2(1), qlz_get_setting3(1))); workmemsize = max(state_size, workmemsize);
      dstate_size = max(qlz_get_setting1(2), max(qlz_get_setting2(2), qlz_get_setting3(2))); workmemsize = max(dstate_size, workmemsize);
      break;
      #endif
 
      #if _BRIEFLZ
    case P_BRIEFLZ: workmemsize = blz_workmem_size(insize); break;
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
    case P_LIBBSC: case P_LIBBSC_ST: bsc_init(LIBBSC_FEATURE_FASTMODE); bsc_st_init(LIBBSC_FEATURE_FASTMODE); break;
    #endif
  }
  if(!workmemsize) return 0;
  if(workmemsize > sizeof(_workmem) && !(workmem = (char *)malloc(workmemsize)) ) { 
    fprintf(stderr, "Malloc error: %d\n", workmemsize); 
    exit(0);
  }
}  

void codexit(int codec) { 
  if(workmem != _workmem) {
    free(workmem/*, workmemsize*/); 
    workmem = NULL;
  }
    #if _SNAPPY_C
  if(codec == P_SNAPPY_C)
    snappy_free_env(&env);
    #endif
} 

int brotlidic,brotlictx,brotlirep;

int codcomp(unsigned char *in, int inlen, unsigned char *out, int outsize, int codec, int lev, char *prm) {  int outlen; unsigned char *oend=out+outsize;
  switch(codec) { 
      #ifdef LZTURBO  
    #include "../beplugc.c"
      #endif
	  
	  #if _BALZ
	case P_BALZ: return balzcompress(in, inlen, out,lev);
      #endif 
 
	  #if _BCM
    case P_BCM: return bcmcompress(in, inlen, out); break;
      #endif 

      #if _C_BLOSC2
    case P_C_BLOSC2:
        #ifdef _C_BLOSC2LZ
      return blosclz_compress(lev, in, inlen, out, outsize);
        #else
      return blosc_compress(lev/*clevel*/, 0/*doshuffle*/, 1/*typesize*/, inlen, in, out, outsize /*inlen+BLOSC_MAX_OVERHEAD*/);
        #endif 
      #endif 

      #if _BRIEFLZ
	case P_BRIEFLZ: return blz_pack(in, out, inlen, workmem);
	  #endif
	  
      #if _BROTLI
    case P_BROTLI: { brotli::BrotliParams params;params.quality=lev; 
         if(params.quality==11) params.lgwin = 24; if(*prm=='w') { prm++; params.lgwin=22;} else if(strchr(prm,'W')) { prm++; params.lgwin=24;} if(strchr(prm,'D')) brotlidic++; if(strchr(prm,'R')) brotlirep++; if(strchr(prm,'X')) brotlictx++;
         size_t esize=outsize; int rc = brotli::BrotliCompressBuffer(params, inlen,in,&esize,out); brotlidic = brotlictx = brotlirep = 0; return rc?esize:0; 
      }
	  #endif    

      #if _LIBBSC
	    #define LZPHASHSIZE 18 //LIBBSC_DEFAULT_LZPHASHSIZE
	case P_LIBBSC_ST: return bsc_compress(in, out, inlen, 0/*LZPHASHSIZE*/, 0/*LIBBSC_DEFAULT_LZPMINLEN*/, lev/*codec-_BSC_ST3+LIBBSC_BLOCKSORTER_ST3*/, lev>4?LIBBSC_CODER_QLFC_ADAPTIVE:LIBBSC_CODER_QLFC_STATIC, 0/*LIBBSC_FEATURE_FASTMODE /*| LIBBSC_FEATURE_MULTITHREADING*/);
	case P_LIBBSC:    return bsc_compress(in, out, inlen, LZPHASHSIZE,     32, LIBBSC_BLOCKSORTER_BWT,     lev,                                          LIBBSC_DEFAULT_FEATURES/*LIBBSC_FEATURE_FASTMODE | LIBBSC_FEATURE_MULTITHREADING*/);
	  #endif

      #if _BZIP2
	case P_BZIP2:    { unsigned outlen = outsize; return BZ2_bzBuffToBuffCompress((char *)out, &outlen, (char *)in, inlen, 9, 0, 0)==BZ_OK?outlen:-1; }
	  #endif
	  
	  #if _CHAMELEON
    case P_CHAMELEON:  { Chameleon_Reset((Chameleon *)workmem); return lev<2?Chameleon_Encode((Chameleon *)workmem,out,in, inlen):Chameleon2_Encode((Chameleon *)workmem,out,in, inlen); }
	  #endif

      #if _CSC
    case P_CSC: { 
        CSCProps prop; CSCEncProps_Init(&prop, inlen<(1<<30)?inlen:(1<<30), lev); CSCEnc_WriteProperties(&prop, (uint8_t*)out, 0);
        MemISeqInStream  si; si.s.Read  = (int(*)(void *, void *, size_t *))cscread;  si.in  = in;                  si.inlen  = inlen;
	    MemISeqOutStream so; so.s.Write = (size_t(*)(void *, const void *, size_t  ))cscwrite; so.out = out + CSC_PROP_SIZE; so.outlen = CSC_PROP_SIZE;
	    CSCEncHandle eh = CSCEnc_Create(&prop, (ISeqOutStream*)&so, NULL); CSCEnc_Encode(eh, (ISeqInStream*)&si, NULL); CSCEnc_Encode_Flush(eh); CSCEnc_Destroy(eh);
        return so.outlen;
      }
      #endif

      #if _CRUSH
    case P_CRUSH: return crush::compress(lev, in, inlen, out);
      #endif
	  
      #if _DENSITY
    case P_DENSITY: { 
	    density_buffer_processing_result rs = density_buffer_compress((const uint8_t *)in, inlen,                                 (uint8_t*)out,      outsize, (const DENSITY_COMPRESSION_MODE)lev/*, DENSITY_BLOCK_TYPE_DEFAULT, NULL, NULL*/);
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
 
      #if _HEATSHRINK
    case P_HEATSHRINK:   return hscompress(in, inlen, out);
      #endif

	  #if _LIBLZF  
    case P_LIBLZF:    return lzf_compress(in, inlen, out, outsize);
	  #endif
	
      #if _LIBZPAQ
    case P_LIBZPAQ: { zin = in; zin_ = in+inlen; zout = out; char s[3]; s[0]=lev+'0'; s[1]=0; libzpaq::compress(&zmemin, &zmemout, s); return zout - out; }
      #endif

	  #if _LZ4
    case P_LZ4: return !lev?LZ4_compress_fast((char *)in, (char *)out, inlen, outsize, 4):(lev<9?LZ4_compress_default((char *)in, (char *)out, inlen, outsize):LZ4_compress_HC((char *)in, (char *)out, inlen, outsize, lev));
	  #endif

	  #if _LZ5
    case P_LZ5: return !lev?LZ5_compress_fast((char *)in, (char *)out, inlen, outsize, 4):(lev<2?LZ5_compress_default((char *)in, (char *)out, inlen, outsize):LZ5_compress_HC((char *)in, (char *)out, inlen, outsize, lev));
	  #endif

      #if _LZFSE
    case P_LZFSE_ : return compression_encode_buffer(out, outsize, in, inlen, workmem, COMPRESSION_LZFSE); break;
      #endif

      #if _LZHAM
    case P_LZHAM:    
		lzham_uint32 m_table_max_update_interval;
		lzham_uint32 m_table_update_interval_slow_rate;


	  { static int dicbits[] = { 24, 24, 24, 26, 29, 29 }; if(lev > 4) lev = 4;
	    lzham_compress_params hprm; memset(&hprm, 0, sizeof(hprm)); 
		hprm.m_struct_size 						      = sizeof(hprm);
        hprm.m_dict_size_log2           			  = dicbits[lev];
        hprm.m_level                    			  = (lzham_compress_level)lev; if(hprm.m_level > LZHAM_COMP_LEVEL_UBER) hprm.m_level = LZHAM_COMP_LEVEL_UBER;
        hprm.m_compress_flags   					 |= LZHAM_COMP_FLAG_FORCE_SINGLE_THREADED_PARSING;
        hprm.m_max_helper_threads                     = 0;
		if(lev == 4 && *prm == 'x') {
          hprm.m_compress_flags   				     |= LZHAM_COMP_FLAG_EXTREME_PARSING;
          hprm.m_fast_bytes        				      = LZHAM_MAX_FAST_BYTES;
		  hprm.m_extreme_parsing_max_best_arrivals    = 4;										
		}
		hprm.m_table_update_rate   				      = LZHAM_DEFAULT_TABLE_UPDATE_RATE;		
        size_t outlen        = outsize;		
		lzham_uint32 adler32 = 0;
        lzham_compress_status_t rc = lzham_compress_memory(&hprm, (lzham_uint8*)out, &outlen, (const lzham_uint8 *)in, inlen, &adler32); 
        return rc == LZHAM_COMP_STATUS_SUCCESS?outlen:-rc;
      }
      #endif
		
        #ifdef __x86_64__
           #define DICSIZE (1<<30)
        #else
           #define DICSIZE (1<<27)
        #endif
      #if _LZMA
	case P_LZMA: { CLzmaEncProps p;	LzmaEncProps_Init(&p); p.level = lev; p.numThreads = 1; if(lev==9) p.fb = 273,p.dictSize=inlen<DICSIZE?inlen:DICSIZE; LzmaEncProps_Normalize(&p);
        SizeT psize = LZMA_PROPS_SIZE, outlen = outsize - LZMA_PROPS_SIZE;
  	    return LzmaEncode(out+LZMA_PROPS_SIZE, &outlen, in, inlen, &p, out, &psize, 0, NULL, &g_Alloc, &g_Alloc) == SZ_OK?outlen+LZMA_PROPS_SIZE:0;
	  }
      #endif
	
      #if _LZLIB
	case P_LZLIB:  { unsigned outlen; bbcompress( (const uint8_t *)in, inlen, (uint8_t *)out, (int * const)&outlen,  option_mapping[lev].dictionary_size, option_mapping[lev].match_len_limit); return outlen; }
      #endif
	    
	  #if _LIBLZG
	case P_LIBLZG: { lzg_encoder_config_t cfg; LZG_InitEncoderConfig(&cfg); cfg.fast = LZG_TRUE; cfg.level = lev; return LZG_Encode(in, inlen, out, outsize, &cfg); }
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
 
      #if _LZSS
	case P_LZSS1:   { return lzss_encode(in, inlen, out, 0); }
    case P_LZSS2:   { return lzss_encode(in, inlen, out, 1); }
	  #endif

      #if _MSCOMPRESS
    case P_MSCOMPRESS: { size_t outlen=outsize; return ms_compress((MSCompFormat)lev, in, inlen, out, &outlen)?0:outlen; }
      #endif

 	  #if _MINIZ
    case P_MINIZ:   { uLongf outlen = outsize; int rc = mz_compress2(out, &outlen, in, inlen, lev); if (rc != Z_OK) printf("miniz compress2 rc=%d\n", rc); return outlen; }
      #endif

      #if _NAKA
    case P_NAKA:    return NakaCompress( out, in, inlen); 
       #endif 

	  #if _PITHY
    case P_PITHY: return pithy_Compress((const char *)in, inlen, (char *)out, outsize, lev);
      #endif  	

      #if _QUICKLZ
    case P_QUICKLZ: { memset(workmem,0,workmemsize); return lev<=1?qlz_compress1((char *)in, (char *)out, inlen, workmem):(lev<=2?qlz_compress2((char *)in, (char *)out, inlen, workmem):qlz_compress3((char *)in, (char *)out, inlen, workmem)); }
      #endif  	

	  #if _SAP
    case P_SAP: { CsObjectInt c; SAP_INT bytes_read, bytes_written; int rc = c.CsInitCompr((SAP_BYTE *)out, inlen, lev); out += CS_HEAD_SIZE; outsize -= CS_HEAD_SIZE;
        c.CsCompr(inlen, in, inlen, out, outsize, lev, &bytes_read, &bytes_written); return bytes_written+CS_HEAD_SIZE; 
      }
      #endif  	

	  #if _SHOCO
    case P_SHOCO:     return shoco_compress((const char *)in, inlen, (char *)out, outsize);
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
    case P_TORNADO:      return torcompress(lev, in, out, inlen);
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

      #if _YALZ77
    case P_YALZ77: { lz77::compress_t c(lev, lz77::DEFAULT_BLOCKSIZE); std::string os = c.feed(in,in+inlen); memcpy(out, os.c_str(), os.size()); return os.size(); }
	  #endif

      #if _YAPPY
    case P_YAPPY:    return YappyCompress(in, out, inlen, 10)-out;
	  #endif

      #if _ZLIB
    case P_ZLIB:     { uLongf outlen = outsize; int rc = compress2(out, &outlen, in, inlen, lev); if(rc != Z_OK) printf("zlib compress2 rc=%d\n", rc);	return outlen; }
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
    case P_ZSTD: ZSTD_compress( out, outsize, in, inlen, lev); break;
      #endif   
    //------------------------- Encoding
     #if _RLE 
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
    case P_RLET: return trlec(in, inlen, out); 
      #endif 
    //------------------------- Transform -----------------------------
      #if _DIVBWT
    case P_DIVBWT: { int *sa = (int *)malloc((inlen + 1) * sizeof(int)); if(!sa) return -1; 
	  unsigned bwtidx = divbwt(in, out+sizeof(bwtidx), sa, inlen, NULL, NULL, 0); free(sa); *(unsigned *)out = bwtidx; return inlen+4; }
      #endif	
    //------------------------- Entropy Coders -------------------------
      #if _MEMCPY 
    case P_MCPY:   memcpy(out, in, inlen);    return inlen;
    case P_LMCPY:   libmemcpy(out, in, inlen); return inlen;
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

      #if _FQZ
    case P_FQZ0:  { unsigned int outlen; compress_block(in, inlen, out, &outlen); return outlen; }
      #endif

	  #if _FSC
    case P_FSC:     { size_t outlen = 0; uint8_t *op = NULL; int ok = FSCEncode(in, inlen, &op, &outlen, 12, CODING_METHOD_DEFAULT); if(ok) { memcpy(out, op, outlen); free(op); } return outlen; }
	  #endif 

	  #if _FSE
    case P_FSE:     { size_t o = FSE_compress(out, outsize, in, inlen); if(o == 1) { out[0] = in[0]; return 1; } if(!o || o >= inlen) { memcpy(out, in, inlen); return inlen; } return o; }
    case P_FSEH:    { size_t o = HUF_compress(out, outsize, in, inlen); if(o == 1) { out[0] = in[0]; return 1; } if(!o || o >= inlen) { memcpy(out, in, inlen); return inlen; } return o; }
      #endif 

	  #if _NANS
    case P_NANS:   return nansenc(in, inlen, out, oend);
      #endif

      #if _JAC
    case P_JAC:  { unsigned outlen; arith_compress_O0(in, inlen, &outlen, out); return outlen; } 
      #endif

	  #if _JRANS
    case P_JRANS:   return rans_compress_O0(in, inlen, out, oend);
    case P_JRANS1:  return rans_compress_O1(in, inlen, out, oend); 
        #ifdef __x86_64__
    case P_JRANS64: return rans64_compress_O0(in, inlen, out, oend);
    case P_JRANS641:return rans64_compress_O1(in, inlen, out, oend);
        #endif
	  #endif

      #if _PPMDEC
	case P_PPMDEC:  return ppmdenc(in, inlen, out);
	  #endif
       
      #if _POLHF
    case P_POLHF:   return polarenc(in, inlen, out); 
      #endif

      #if _SHRC
    case P_SHRC:    return rcshc(in, inlen, out); 
    case P_SHRCV:   return vecrcenc(in, inlen, out);
      #endif

      #if _SUBOTIN 
    case P_SUB:     return subenc(in, inlen, out, inlen, SYMNUM);
      #endif
  
      #if _TORNADOHF 
    case P_TORNADOHF:     return torhenc(in, inlen, out, outsize); 
	  #endif
      
	  #if _ZLIBH
    case P_ZLIBH:   return ZLIBH_compress((char* )out, (const char*)in, inlen); 
	  #endif

      #if _MYCODEC 
//   case P_MYCODEC:   return mycomp(in, inlen, out, outsize);
	  #endif	
 
    defaulf: fprintf(stderr, "library '%d' not included\n", codec);
  } 
} 
  
int coddecomp(unsigned char *in, int inlen, unsigned char *out, int outlen, int codec, int lev) {	
  switch(codec) {
      #ifdef LZTURBO  
    #include "../beplugd.c"
      #endif

  	  #if _BALZ
	case P_BALZ: return balzdecompress(in, inlen, out, outlen);
      #endif

	  #if _BCM
    case P_BCM: return bcmdecompress(in, inlen, out, outlen);
      #endif 

      #if _C_BLOSC2
    case P_C_BLOSC2: return blosc_decompress(in, out, outlen);
      #endif

      #if _C_BLOSC2LZ
    case P_C_BLOSC2LZ: return blosclz_decompress(in, inlen, out, outlen);
      #endif
	  
	  #if _BRIEFLZ
	case P_BRIEFLZ:     return blz_depack(in, out, outlen);
	  #endif
	  
      #if _BROTLI
    case P_BROTLI: { size_t dsize = outlen; int rc = BrotliDecompressBuffer(inlen,in,&dsize,out); return rc?dsize:0; }
	  #endif

      #if _LIBBSC
	case P_LIBBSC_ST: 
	case P_LIBBSC:	   return bsc_decompress(in, inlen, out, outlen, 0);
	  #endif
	
      #if _BZIP2 	  
 	case P_BZIP2: { unsigned outsize = outlen; return BZ2_bzBuffToBuffDecompress((char *)out, &outsize, (char *)in, inlen, 0, 0)==BZ_OK?outlen:-1; }
      #endif
	  
	  #if _CHAMELEON
    case P_CHAMELEON:  { Chameleon_Reset((Chameleon *)workmem); lev<2?Chameleon_Decode((Chameleon *)workmem, out, outlen, in):Chameleon2_Decode((Chameleon *)workmem, out, outlen, in); return inlen; }
	  #endif

      #if _CSC
    case P_CSC: { 
        CSCProps prop; CSCDec_ReadProperties(&prop, (uint8_t*)in);
        MemISeqInStream  si; si.s.Read  = (int(*)(void *, void *, size_t *))cscread;  si.in  = in + CSC_PROP_SIZE; si.inlen  = inlen - CSC_PROP_SIZE;
	    MemISeqOutStream so; so.s.Write = (size_t(*)(void *, const void *, size_t  ))cscwrite; so.out = out;                so.outlen = 0;
	    CSCDecHandle dh = CSCDec_Create(&prop, (ISeqInStream*)&si, NULL); CSCDec_Decode(dh, (ISeqOutStream*)&so, NULL); CSCDec_Destroy(dh);
        return si.inlen;
      }
      #endif

     #if _CRUSH
    case P_CRUSH: crush::decompress(in, out, outlen); break;
	  #endif
	  
      #if _DENSITY
    case P_DENSITY: { density_buffer_processing_result rs = density_buffer_decompress((uint8_t *)in, inlen, (uint8_t*)out, outlen+DENSITY_MINIMUM_OUTPUT_BUFFER_SIZE);  return rs.state?0:rs.bytesWritten; }
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

      #if _HEATSHRINK
    case P_HEATSHRINK: return hsdecompress(in, inlen, out, outlen); 
      #endif

	  #if _LIBLZF 
    case P_LIBLZF: lzf_decompress(in, inlen, out, outlen); break;
	  #endif

	  #if _LZ4
    case P_LZ4: LZ4_decompress_safe((const char *)in, (char *)out, inlen, outlen); break;
      #endif
	  
	  #if _LZ5
    case P_LZ5: LZ5_decompress_safe((const char *)in, (char *)out, inlen, outlen); break;
      #endif
	  
      #if _LZFSE
    case P_LZFSE_ : return compression_decode_buffer(out, outlen, in, inlen, workmem, COMPRESSION_LZFSE); break;
      #endif

	  #if _LIBLZG
	case P_LIBLZG: LZG_Decode(in, inlen, out, outlen); break;
	  #endif
	  
      #if _LIBZPAQ
    case P_LIBZPAQ: { zin = in; zin_ = in+inlen; zout = out; libzpaq::decompress(&zmemin, &zmemout); return zin - in; }
      #endif

      #if _LZHAM
    case P_LZHAM: { static int dicbits[]={ 24, 24, 24, 26, 29, 29 };
	    lzham_decompress_params prm; memset(&prm, 0, sizeof(prm));
        prm.m_struct_size    = sizeof(prm);
        prm.m_dict_size_log2 = dicbits[lev];
        size_t outl          = outlen;
		lzham_uint32 adler32 = 0;																
        lzham_decompress_status_t rc = lzham_decompress_memory(&prm, (lzham_uint8*)out, &outl, in, inlen, &adler32);	//if(rc!=LZHAM_COM_STATUS_SUCCESS) die("rc=%d ", rc);
        return rc == LZHAM_COMP_STATUS_SUCCESS?outlen:-1;	
	  }
      #endif
	  
      #if _LZLIB
	case P_LZLIB: { int out_len = outlen; bbdecompress( in, outlen, out, &out_len ); } break;
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
    case P_NAKA:    return NakaDecompress(out, in, inlen); 
      #endif
	  #if _PITHY
    case P_PITHY: return pithy_Decompress((const char *)in, inlen, (char *)out, outlen);
      #endif  	

      #if _QUICKLZ
    case P_QUICKLZ: { lev= (in[0]>>2)&3; outlen = lev<=1?qlz_decompress1((char*)in, out, workmem):(lev<=2?qlz_decompress2((char*)in, out, workmem):qlz_decompress3((char*)in, out, workmem)); } break;
      #endif
	  
	  #if _SAP
    case P_SAP: { CsObjectInt d; SAP_INT bytes_read, bytes_written; 
	    d.CsInitDecompr((SAP_BYTE *)in); in += CS_HEAD_SIZE; d.CsDecompr( (SAP_BYTE *)in, inlen-CS_HEAD_SIZE, (SAP_BYTE *)out, outlen, lev, &bytes_read, &bytes_written );			 
	    return inlen-CS_HEAD_SIZE; 
	  }
      #endif  	

      #if _ZLING
    case P_ZLING: zling_decompress(in, inlen, out, outlen); break;
      #endif

	  #if _LZSS
    case P_LZSS: lzss_decode(in, out, outlen); break;
	  #endif

      #if _MSCOMPRESS
     case P_MSCOMPRESS: { size_t _outlen=outlen; return ms_decompress((MSCompFormat)lev, in, inlen, out, &_outlen)==MSCOMP_OK?inlen:0; }
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
	  
      #if _TORNADO
    case P_TORNADO:     return tordecompress(in, out, inlen, outlen);
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

      #if _YALZ77
    case P_YALZ77: { lz77::decompress_t d; std::string extra; if(!d.feed(in,in+inlen,extra) || extra.size() > 0) return 0;
        const std::string& os = d.result(); memcpy(out, os.c_str(), os.size()); return os.size(); 
	  }
	  #endif

	  #if _YAPPY
	case P_YAPPY:    return YappyUnCompress(in, in+inlen, out)-out; 
      #endif
		 
      #if _ZLIB
    case P_ZLIB: case P_ZOPFLI: { uLongf outsize = outlen; int rc = uncompress(out, &outsize, in, inlen); } break;
      #endif

      #if _ZSTD
    case P_ZSTD: ZSTD_decompress( out, outlen, in, inlen); break;
      #endif
      //------------ Encoding -----------------------------------------------------------------------
      #if _RLE
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
      //------------ Transform -----------------------------------------------------------------------
      #if _DIVBWT
    case P_DIVBWT: memcpy(out, in+4, outlen); bsc_bwt_decode(out, outlen, *(unsigned *)in, 0, NULL, 0); return inlen;
      #endif	
      //------------ Entropy Coders ------------------------------------------------------------------
      #if _MEMCPY 
    case P_MCPY:    memcpy(out, in, inlen); 	break;
    case P_LMCPY:   libmemcpy(out, in, outlen); break;
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
	  
	  #if _FSC
    case P_FSC:     { size_t outsize = 0; uint8_t *op = NULL; int ok = FSCDecode(in, inlen, &op, &outsize); if(ok) { memcpy(out,op,outlen); free(op); } } break;
	  #endif
 
	  #if _FSE
    case P_FSE:  if(inlen == outlen) memcpy(out, in, outlen); else if(inlen == 1) memset(out,in[0],outlen); else FSE_decompress(out, outlen, in, inlen); break;
    case P_FSEH: if(inlen == outlen) memcpy(out, in, outlen); else if(inlen == 1) memset(out,in[0],outlen); else HUF_decompress(out, outlen, in, inlen); break;
	  #endif
		
      #if _FQZ
	case P_FQZ0: uncompress_block(in, out, outlen); break;
	  #endif

      #if _JAC
    case P_JAC:  { unsigned outlen; arith_uncompress_O0(in, inlen, &outlen, out); } break;
      #endif

	  #if _JRANS
    case P_JRANS:   rans_uncompress_O0(  in, inlen, out, outlen); break;
    case P_JRANS1:  rans_uncompress_O1(  in, inlen, out, outlen); break;
        #ifdef __x86_64__
    case P_JRANS64: rans64_uncompress_O0(in, inlen, out, outlen); break;
    case P_JRANS641:rans64_uncompress_O1(in, inlen, out, outlen); break;
        #endif
	  #endif

      #if _FPAQC
    case P_FPAQC:   absd(in, outlen, out); break;
      #endif

      #if _MMRC     
    case P_O0ZP:    rczmmd(in, outlen, out); break;
    case P_FPAQ0F2: rcmm2d(in, outlen, out); break;
    case P_MMRC:    rcmmd( in, outlen, out); break;
      #endif

	  #if _NANS
    case P_NANS:    nansdec(in, inlen, out, outlen); break;
	  #endif

      #if _PPMDEC
	case P_PPMDEC:  return ppmddec(in, inlen, out, outlen);
	  #endif

      #if _POLHF
    case P_POLHF:   polardec(in, out, outlen); break;
      #endif

      #if _SHRC
    case P_SHRCV:   vecrcdec(in, outlen, out); break;
    case P_SHRC:    rcshd(in, out, outlen); break;
      #endif

      #if _SUBOTIN
    case P_SUB:     subdec(in, outlen, out, outlen, SYMNUM); break;
      #endif

      #if _TORNADOHF
    case P_TORNADOHF:    torhdec(in, inlen, out, outlen); break;
      #endif 

	  #if _ZLIBH		
    case P_ZLIBH:  return ZLIBH_decompress((char* )out, (const char*)in);
	  #endif	

      #if _MYCODEC 
//   case P_MYCODEC:   return mydecomp(in, inlen, out, outlen);
	  #endif	
  }
}

char *codver(int codec, char *v, char *s) {
  switch(codec) {  
      #if _C_BLOSC2
    return BLOSC_VERSION_STRING;
      #endif 
	  #if _BRIEFLZ
    case P_BRIEFLZ: sprintf(s,"%d.%d.%d", BLZ_VER_MAJOR, BLZ_VER_MINOR, BLZ_VER_PATCH); return s;
	  #endif

	  #if _LZ4
    case P_LZ4:     sprintf(s,"%d.%d.%d", LZ4_VERSION_MAJOR, LZ4_VERSION_MINOR, LZ4_VERSION_RELEASE); return s;
	  #endif

	  #if _LZ5
    case P_LZ5:     sprintf(s,"%d.%d.%d", LZ5_VERSION_MAJOR, LZ5_VERSION_MINOR, LZ5_VERSION_RELEASE); return s;
	  #endif

	  #if _ZSTD
    case P_ZSTD:    sprintf(s,"%d.%d.%d", ZSTD_VERSION_MAJOR, ZSTD_VERSION_MINOR, ZSTD_VERSION_RELEASE); return s;
      #endif

	  #if _DENSITY
    case P_DENSITY: sprintf(s,"%d.%d.%d", density_version_major(), density_version_minor(), density_version_revision()); return s;
	  #endif

      #if _HEATSHRINK
    case P_HEATSHRINK: sprintf(s,"%d.%d.%d", HEATSHRINK_VERSION_MAJOR, HEATSHRINK_VERSION_MINOR, HEATSHRINK_VERSION_PATCH); return s;
      #endif

	  #if _SNAPPY
    case P_SNAPPY:  sprintf(s,"%d.%d.%d", SNAPPY_MAJOR, SNAPPY_MINOR, SNAPPY_PATCHLEVEL); return s;
	  #endif
	default:        strcpy(s,v);
  }
  return s;
}

