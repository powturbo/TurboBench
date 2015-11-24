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
//	    TurboBench: plugreg.c - registry 
#define C_EC   0x0001 
#define C_EC1  0x0002 

#define E_FLAG 0x0f00
#define E_ANS  0x0100
#define E_HUF  0x0200

struct plugs { int id; char *s,*v,*lev; unsigned flag,blksize; };
  
struct plugs plugs[] = {
    #ifdef _PLUGIN2
  #include "../beplugr.h"
    #endif

    #ifdef _BALZ
  { P_BALZ, 	"balz", 			_BALZ, "0,1" }, 
    #endif
	
    #ifdef _BCM
  { P_BCM, 		"bcm", 				_BCM, "" }, 
    #endif

    #ifdef _C_BLOSC2
  { P_C_BLOSC2, "blosc",			_C_BLOSC2, "0,1,2,3,4,5,6,7,8,9", 64*1024},
    #endif
	   
    #ifdef _BRIEFLZ
  { P_BRIEFLZ,	"brieflz", 		    _BRIEFLZ, "" }, 
    #endif
	
    #ifdef _BROTLI
  { P_BROTLI,	"brotli", 			_BROTLI, "0,1,2,3,4,5,6,7,8,9,11/DOWX" },
    #endif
	 
    #ifdef _BZIP2
  { P_BZIP2,	"bzip2", 			_BZIP2, "" }, 
    #endif

    #ifdef _CHAMELEON
  { P_CHAMELEON,"chameleon",		_CHAMELEON, "1,2" },
    #endif

    #ifdef _CRUSH
  { P_CRUSH,	"crush", 			_CRUSH, "0,1,2" },
    #endif
	
    #ifdef _CSC
  { P_CSC,	    "csc", 				_CSC, "1,2,3,4,5" },
    #endif
	
    #ifdef _DENSITY
  { P_DENSITY, 	"density",        	_DENSITY,"1,2,3" },
    #endif

    #ifdef _DOBOZ
  { P_DOBOZ,	"doboz",			_DOBOZ, "" },  //crash on windows
    #endif

    #ifdef _FASTLZ
  { P_FASTLZ,	"fastlz", 			_FASTLZ,"1,2" },
    #endif
	
	#ifdef _GIPFELI
  { P_GIPFELI, 	"gipfeli", 			_GIPFELI, "" }, 
    #endif
    
    #ifdef _HEATSHRINK 
  { P_HEATSHRINK,"heatshrink",		_HEATSHRINK, "" },
    #endif
	
    #ifdef _LIBBSC
  { P_LIBBSC_ST,"bsc_st", 			_LIBBSC, "3,4,5,6,7,8" }, 
  { P_LIBBSC, 	"bsc", 				_LIBBSC, "1,2"}, 
    #endif

    #ifdef _LIBLZF
  { P_LIBLZF, 	"lzf", 				_LIBLZF, "" },
    #endif

    #ifdef _LIBLZG
  { P_LIBLZG,  	"lzg", 				_LIBLZG, "1,2,3,4,5,6,7,8,9" }, 
    #endif
	 
    #ifdef _LZ4
  { P_LZ4,  	"lz4",				_LZ4, "0,1,9" }, 
    #endif
	
    #ifdef _LZ5
  { P_LZ5,  	"lz5",				_LZ5, "0,1,9" }, 
    #endif
	
    #ifdef _LZFSE
  { P_LZFSE, 	"lzfse", 			_LZFSE, "" },
    #endif

    #ifdef _LZHAM
  { P_LZHAM, 	"lzham", 			_LZHAM,"1,2,3,4/x" }, 
    #endif

    #ifdef _LZLIB 
  { P_LZLIB, 	"lzlib", 			_LZLIB, "1,2,3,4,5,6,7,8,9" },
   #endif
	
    #ifdef _LZMAT
  { P_LZMAT, 	"lzmat", 			_LZMAT, "" },
    #endif
  
    #ifdef _LZMA
  { P_LZMA,  	"lzma", 			_LZMA, "0,1,2,3,4,5,6,7,8,9" }, 
    #endif

    #ifdef _LZO
	  #ifdef _MINILZO
  { P_LZO1x, 	"lzo1x",			_LZO, "1" },
      #else
  { P_LZO1b, 	"lzo1b", 			_LZO, "1,9,99,999" },  
  { P_LZO1c, 	"lzo1c",			_LZO, "1,9,99,999" },
  { P_LZO1f, 	"lzo1f",			_LZO, "1,999" },
  { P_LZO1x, 	"lzo1x", 			_LZO, "1,11,12,15,999" },
  { P_LZO1y,  	"lzo1y",			_LZO, "1,999" }, 
  { P_LZO1z, 	"lzo1z", 			_LZO, "999" }, 
  { P_LZO2a, 	"lzo2a", 			_LZO, "999" }, 
      #endif
    #endif
   
    #ifdef _LZSS
  { P_LZSS,  	"lzss", 			_LZSS, "1,2" }, 
    #endif

    #ifdef _LZTURBO
  { P_LZT,		"lzturbo", 			_LZTURBO, "10,11,12,19,20,21,22,29,30,31,32,39,49/a" },
    #endif

    #ifdef _MINIZIP
  { P_MINZ,    	"miniz", 			_MINIZIP, "1,2,3" }, 
    #endif

    #ifdef _MSCOMPRESS
  { P_MSCOMPRESS,"mscompress", 		_MSCOMPRESS, "2,3,4" }, 
    #endif

    #ifdef _PITHY
  { P_PITHY, 	"pithy",			_PITHY, "0,1,2,3,4,5,6,7,8,9" },
    #endif

    #ifdef _QUICKLZ
  { P_QUICKLZ, 	"quicklz",			_QUICKLZ, "1,2,3" },
    #endif

    #ifdef _SAP
  { P_SAP, 	    "sap",				_SAP, "0,1,2"	},
    #endif
  
    #ifdef _SHRINKER
  { P_SHRINKER, "shrinker",			_SHRINKER, "", 0, (1<<26) },
    #endif

    #ifdef _SNAPPY
  { P_SNAPPY, 	"snappy",			_SNAPPY, ""	},
    #endif
  
    #ifdef _SNAPPY_C
  { P_SNAPPYC, 	"snappy_c",			_SNAPPY_C,"" },
    #endif
   
    #ifdef _TORNADO
  { P_TORNADO, 	"tornado", 			_TORNADO, "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16" }, 
    #endif
   
    #ifdef _WFLZ
  { P_WFLZ,	    "wflz", 			_WFLZ, "1,2" },
    #endif
	
    #ifdef _WIMLIB
  { P_WIMLIB,	"wimlib", 			_WIMLIB, "1,2,3" },//LZX,XPRESS,LZMS
    #endif
	
    #ifdef _WKDM
  { P_WKDM, 	"WKdm",				_WKDM, "" }, // crash
    #endif	

    #ifdef _YALZ77
  { P_YALZ77, 	"yalz77", 			_YALZ77, "1,6,12" },
    #endif

    #ifdef _YAPPY
  { P_YAPPY, 	"yappy",			_YAPPY, "" },//crash windows
    #endif

    #ifdef _ZLIB
  { P_ZLIB, 	"zlib", 			_ZLIB, "1,2,3,4,5,6,7,8,9" },
    #endif 			
  
    #ifdef _ZLING
  { P_ZLING, 	"zling", 	   		_ZLING, "0,1,2,3,4" }, 
    #endif

	#ifdef _ZOPFLI
  { P_ZOPFLI, 	"zopfli",			_ZOPFLI, ""}, 
    #endif
	
    #ifdef _ZSTD
  { P_ZSTD, 	"zstd", 			_ZSTD, "1,2,3,4,5,6,7,8,9,15,16,20" },
    #endif
//-----------------------------------------------------------------------------------	  
  { P_MCPY, 	"imemcpy", 			_IMEMCPY, "" },
  { P_LMCPY, 	"memcpy",			_MEMCPY,  "" },

    #ifdef _LZTANS
	   #ifdef __x86_64__
  { P_LZTANS64, "TurboANS",			_LZTANS, "" },
       #endif
  { P_LZTANS, 	"TurboANX",	    	_LZTANS, "",C_EC },
//{ P_LZTANSX, 	"TurboANXX",	  	_LZTANS, "",C_EC1},
    #endif
	
    #ifdef _LZTHF
  { P_LZT3, 	"TurboHF",		    _LZTHF, "",C_EC},  
    #endif
    #ifdef _LZTRC
  { P_LZT5, 	"TurboAC_byte", 	_LZTRC, "",C_EC}, 
  { P_LZT6, 	"TurboRC", 			_LZTRC, "",C_EC}, 
  { P_LZT7, 	"TurboRC_o1",   	_LZTRC, "",C_EC1}, 
    #endif

    #ifdef _BCMEC
  { P_BCMEC, 	"bcmec", 			_BCMEC, "", C_EC},
    #endif

    #ifdef _FSC
  { P_FSC, 		"fsc", 				_FSC, "", C_EC|E_ANS },
    #endif

    #ifdef _FSE
  { P_FSE, 		"fse", 				_FSE, "", C_EC|E_ANS },
  { P_FSEH,		"fsehuf", 			_FSE, "", C_EC|E_HUF },
    #endif

    #ifdef _MMABS
  { P_MMABS, 	"fpaqc",	 		_MMABS, "", C_EC},
    #endif

    #ifdef _FASTHF
  { P_FASTHF,  	"FastHF", 			_FASTHF, "",C_EC },
    #endif

    #ifdef _FASTARI
  { P_FASTARI,  "FastAri", 			_FASTARI, "",C_EC },
    #endif

    #ifdef _FASTAC
  { P_FASTAC,  	"FastAC", 			_FASTAC, "",C_EC },
    #endif

    #ifdef _JAC
  { P_JAC, 		"arith_static",		_JAC, "",C_EC|E_ANS },
    #endif
  
	#ifdef _JRANS
  { P_JRANS, 	"rans_static4c",	_JRANS, "",C_EC |E_ANS},
  { P_JRANS1, 	"rans_static4c_o1", _JRANS, "",C_EC1|E_ANS},
  { P_JRANS64, 	"rans_static64c",	_JRANS, "",C_EC |E_ANS},
  { P_JRANS641,	"rans_static64c_o1",_JRANS, "",C_EC1|E_ANS},
    #endif
 
    #ifdef _MMRC
  { P_FPAQ0F2, 	"fpaq0f2", 		    _MMRC, "",C_EC},
  { P_MMRC, 	"fpaq0p_mm", 		_MMRC, "",C_EC},
  { P_O0ZP,     "o0zp",             _MMRC, "",C_EC},
    #endif

	#ifdef _NANS
  { P_NANS, 	"naniarans",		_NANS, "",C_EC|E_ANS},
    #endif
 
	#ifdef _POLHF
  { P_POLHF,    "polar", 			_POLHF, "",C_EC},
    #endif

    #ifdef _SUBOTIN
  { P_SUB, 		"subotin", 			_SUBOTIN, "",C_EC},
    #endif

    #ifdef _TORNADOHF
  { P_TORNADOHF,"tornado_huff", 	_TORNADOHF, "", C_EC},
    #endif

    #ifdef _ZLIBH
  { P_ZLIBH, 	"zlibh",			_ZLIBH, "", C_EC|E_HUF },
    #endif
	
  { -1, "", ""} 
  };
