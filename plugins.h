/**
    Copyright (C) powturbo 2013-2015
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
//	    TurboBench: plugins.h - settings 
#define _COMPRESS1
#define _COMPRESS2
#define _ECODEC 
//#define _APPLE
//#define _GPL

//#define _PLUGIN2 // Additional plugins not released
#define _CPP
//-------------------------------------------------------------------------------------
  #ifdef _COMPRESS1   
#define _BROTLI	 		"15-11\tBrotli\tApache license\thttps://github.com/google/brotli"
#define _LZ4 			"15-10\tLz4\tBSD license\thttps://github.com/Cyan4973/lz4"
#define _LZMA 			"9.35\tLzma\tPublic Domain\thttp://7-zip.org\thttps://github.com/jljusten/LZMA-SDK"
#define _LZTURBO    	"1.3\tLzTurbo\t        \thttps://sites.google.com/site/powturbo\t"
//#define _ZLIB			"14.06\tZlib Intel\tZlib license\thttps://github.com/jtkukunas/zlib"
//#define _ZLIB			"\tzlib-ng\tzlib license\thttps://github.com/Dead2/zlib-ng"
//#define _ZLIB			"\tsystem library zlib\t\tzlib license\thttp://zlib.net"
#define _ZLIB			"1.2.8\tzlib\tzlib license\thttp://zlib.net\thttps://github.com/madler/zlib"
#define _ZSTD			"15-10\tZSTD\tBSD license\thttps://github.com/Cyan4973/zstd"
  #endif

  #ifdef _COMPRESS2
#define _BALZ 			"1.20\tbalz\tPublic Domain\thttp://sourceforge.net/projects/balz"
#define _BCM 			"1.0\tbcm\tPublic Domain\thttp://sourceforge.net/projects/bcm" 
#define _BRIEFLZ 		"1.1.0\tBriefLz\tBSD like\thttps://github.com/jibsen/brieflz"
#define _BZIP2 		    "1.06\tBzip2\tBSD like\thttp://www.bzip.org/downloads.html\thttps://github.com/asimonov-im/bzip2"
#define _C_BLOSC2		"1.7.1\tBlosc\tBSD license\thttps://github.com/Blosc/c-blosc2" 
#define _CHAMELEON      "15-03\tChameleon\tPublic Domain\thttp://cbloomrants.blogspot.de/2015/03/03-25-15-my-chameleon.html"
#define _CRUSH	 		"1.0.0\tCrush\tPublic Domain\thttp://sourceforge.net/projects/crush"
#define _CSC            "15-06\tCSC\tPublic domain\thttps://github.com/fusiyuan2010/CSC"
#define _DENSITY    	"0.12.0\tDensity\tBSD license\thttps://github.com/centaurean/density"
#define _DOBOZ			"14-01-14\tDoboz\tBSD Like\thttps://bitbucket.org/attila_afra\thttps://github.com/nemequ/doboz" //crash
#define _FASTLZ 		"0.1.0\tFastLz\tBSD like\thttp://fastlz.org\thttps://github.com/ariya/FastLZ"
#define _GIPFELI    	"15.10\tGipfeli\tApache license\thttps://github.com/google/gipfeli"
#define _GIPFELI0
#define _HEATSHRINK     "0.4.1\theatshrink\tBSD license\thttps://github.com/atomicobject/heatshrink"
#define _LIBBSC         "3.1.0\tbsc\tApache license\thttps://github.com/IlyaGrebnov/libbsc"
#define _LIBLZF        	"1.06\tLibLZF\tBSD license\thttp://oldhome.schmorp.de/marc/liblzf.html\thttps://github.com/nemequ/liblzf"
#define _LIBLZG			"1.0.8\tLibLzg\tzlib-license\thttps://github.com/mbitsnbites/liblzg\thttp://liblzg.bitsnbites.eu/e" //"https://gitorious.org/liblzg" BLOCKSIZE must be < 64MB 
#define _LZ5 			"15-10\tLz5\tBSD license\thttps://github.com/inikep/lz5"
#define _LZHAM			"1.1\tLzham\tMIT license\thttps://github.com/richgel999/lzham_codec_devel"
//#define _LZSS      	" \tLZSS\t "//crashing
#define _PITHY   	    "2011\tPithy\tBSD license\thttps://github.com/johnezang/pithy"
#define _SNAPPY			"1.1.2/14-02\tSnappy\tApache license\thttps://github.com/google/snappy" 
#define _SNAPPY_C		"1.1.2/14.04\tSnappy-c\tBSD Like\thttps://github.com/andikleen/snappy-c"
#define _SHRINKER   	"0.1/r9\tShrinker\tBSD license\thttps://code.google.com/p/data-shrinker"
#define _WFLZ 		    "15-04\twfLZ\tCC0 license\thttps://github.com/ShaneWF/wflz"
//#define _WIMLIB	    	"15-11\tWimLib\tBSD license\thttp://wimlib.net"  //git clone git://wimlib.net/wimlib
//#define _WKDM 		    "2003\tWKdm\tApple PS License\thttp://www.opensource.apple.com/source/xnu/xnu-1456.1.26/iokit/Kernel/\thttps://github.com/berkus/wkdm" // crash
#define _YALZ77         "15-09\tYalz77\tPublic domain\thttps://github.com/ivan-tkatchev/yalz77" 
#define _YAPPY          ".\tYappy\t\t" //crash on windows
#define _ZOPFLI			"15-05\tZopfli\tApache license\thttps://code.google.com/p/zopfli"
#define _ZLING	    	"15-09\tLibzling\tBSD license\thttps://github.com/richox/libzling"
  #endif

  #ifdef _APPLE
#define _LZFSE          "2015\tlzfse\thttps://developer.apple.com/library/prerelease/mac/documentation/Performance/Reference/Compression/\tiOS and OS X"
  #endif

//------------------ Entropy Coders ---------------
#define _IMEMCPY		".\tinline memcpy\t--------------------------------------------------\t" 
#define _MEMCPY		    ".\tlibrary memcpy\t\t"

  #ifdef _ECODEC
#define _BCMEC 			"1.0\tbcm range coder\tPublic Domain\thttp://sourceforge.net/projects/bcm" 
#define _LZTANS		    "1.3\tTurboANX-ANS\t        \thttps://sites.google.com/site/powturbo\t"  
#define _LZTHF 		    "1.3\tTurboHF-Huffmann\t        \thttps://sites.google.com/site/powturbo\t"  
#define _LZTRC 		    "1.3\tTurboRC-Range Coder\t        \thttps://sites.google.com/site/powturbo\t" 
#define _FASTARI	    "15-10\tFastAri\tMIT license\thttps://github.com/davidcatt/FastARI"
#define _FASTAC		    "2006\tFast AC\tBSD like\thttp://www.cipr.rpi.edu/research/SPIHT/"
#define _FASTHF		    "2006\tFast HF\tBSD like\thttp://www.cipr.rpi.edu/research/SPIHT/"
#define _FSC 		    "15-05\tFinite State Coder\tApache license\thttps://github.com/skal65535/fsc"
#define _FSE 		    "15-11\tFinite State Entropy\tBSD license\thttps://github.com/Cyan4973/FiniteStateEntropy"
#define _JAC 	        "15-07\tRange Coder/J.Bonfield\tBSD license\tftp://ftp.sanger.ac.uk/pub/users/jkb"
#define _JRANS 	        "15-08\tANS/J.Bonfield\tPublic Domain\tftp://ftp.sanger.ac.uk/pub/users/jkb" //"15-07\trANS - samtools\thttps://github.com/samtools/htslib/tree/develop/cram"
#define _SUBOTIN        "2000\tsubotin\tPublic Domain\thttp://ezcodesample.com/ralpha/Subbotin.txt"
#define _ZLIBH		    "1.2.8\tzlib Huffmann\tBSD license\thttps://github.com/Cyan4973/FiniteStateEntropy"
  #endif

  #ifndef _CPP  
#undef _BALZ 			
#undef _BCM 			
#undef _BROTLI	
#undef _CSC            
#undef _DOBOZ
#undef _GIPFELI
#undef _SNAPPY			
#undef _YALZ77          
#undef _ZOPFLI			
#undef _ZLING	    	
#undef _TORNADO
#undef _YAPPY 

#undef _BCMEC
#undef _FASTAC
#undef _FASTHF		    
#undef _TORNADOHF	 
  #endif
	
  #ifdef _GPL
#define _LZLIB      	"1.7\tLzlib\tGPL license\thttp://www.nongnu.org/lzip\thttps://github.com/daniel-baumann/lzlib"   
#define _LZMAT      	"1.0\tLzmat\tGPL license\thttps://github.com/nemequ/lzmat\thttp://www.matcode.com/lzmat.htm"
#define _LZO			"2.09\tLzo\tGPL license\thttp://www.oberhumer.com/opensource/lzo\thttps://github.com/nemequ/lzo"
#define _MSCOMPRESS	    "15.09\tms-compress\tGPL license\thttps://github.com/coderforlife/ms-compress"
#define _QUICKLZ		"1.5.1\tQuicklz\tGPL license\thttp://www.quicklz.com\thttps://github.com/robottwo/quicklz"
#define _SAP		    "15-11\tsap\tGPL license\thttps://github.com/CoreSecurity/pysap"
#define _TORNADO		"0.6a\tTornado\tGPL license\thttp://freearc.org\thttps://github.com/nemequ/tornado" 

//#define _MMABS 		    "2007\tAsymmetric Binary Systems/ Matt Mahonney\tGPL 2.0"
//#define _MMRC 		  " \tRange Coder / Matt Mahonney"
#define _POLHF          "10-07\tPolar Codes\tGPL license\thttp://www.ezcodesample.com/prefixer/prefixer_article.html"
#define _TORNADOHF		 "0.6a\tTornado Huf\tGPL license\thttp://freearc.org/Research.aspx\thttps://github.com/nemequ/tornado" 
  #endif

  #ifdef _WIN32
#undef _FSC
#undef _GIPFELI
#undef _C_BLOSC2
  #endif

//------------------------------------------------------------------------------------------------------------------
  #ifdef _PLUGIN2
#include "../beplug.h"
  #else
#undef _LZTURBO
#undef _HEATSHRINK

#undef _LZTANS		    
#undef _LZTHF 		      
#undef _LZTRC 		    
#define _BETRANS BETRANS
  #endif

enum { 
  _BETRANS,
 P_BALZ,
 P_BCM,
 P_C_BLOSC2,
 P_BRIEFLZ, 
 P_BROTLI,    
 P_LIBBSC, P_LIBBSC_ST,  
 P_BZIP2, 
 P_CHAMELEON,
 P_CSC, 
 P_CRUSH, 
 P_DENSITY, 
 P_DOBOZ, 
 P_FASTLZ, 
 P_GIPFELI,
 P_HEATSHRINK,
 P_LZ4,
 P_LZ5,    
 P_LIBLZF,  
 P_LIBLZG,
 P_LZHAM,
 P_LZLIB, 
 P_LZMA, 
 P_LZMAT,
 P_LZO1b, P_LZO1c, P_LZO1f, P_LZO1x, P_LZO1y, P_LZO1z, P_LZO2a,
 P_LZFSE,
 P_LZSS, 
 P_MINZ, 
 P_MSCOMPRESS, 
 P_PITHY, 
 P_QUICKLZ, 
 P_SHRINKER, 
 P_SAP, 
 P_SNAPPY, 
 P_SNAPPYC, 
 P_TORNADO,  
 P_WFLZ,
 P_WIMLIB,
 P_YALZ77,
 P_YAPPY,
 P_ZLIB, 
 P_ZLING,  
 P_ZOPFLI, 
 P_ZSTD,
  // --------- Entropy coders -------------
 P_ABSMM, 
 P_BCMEC,
 P_JAC, 
 P_FASTAC,
 P_FASTARI,
 P_FASTHF,
 P_FSC,
 P_FSE, 
 P_FSEH, 
 P_JRANS,P_JRANS1,P_JRANS64,P_JRANS641,
 P_LMCPY, 
 P_MCPY,
 P_SUB,
 P_SHRCV, 
 P_O0ZP, 
 P_POLHF, 
 P_SHRC, P_MMRC, 
 P_TORNADOHF,
 P_ZLIBH,
 P_LAST
};    

  #ifdef __cplusplus
extern "C" {
  #endif
int  codini(unsigned _inlen);
void codexit();
int  codstart( unsigned char *in, int inlen, int codec);
int  codcomp(  unsigned char *in, int inlen, unsigned char *out, int outsize, int codec, int lev, char *prm);
int  coddecomp(unsigned char *in, int inlen, unsigned char *out, int outlen,  int codec, int lev);
char *codver(int codec, char *v, char *s);
char *imemcpy(char *out, char *in, size_t n);
  #ifdef __cplusplus
}
  #endif
