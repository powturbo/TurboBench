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
//	    TurboBench: plugins.cc - compressor plugins 
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <time.h>
#include "plugins.h"

  #ifndef __cplusplus
#undef _BROTLI
#undef _CRUSH
#undef _DOBOZ
#undef _GIPFELI
#undef _ZLING						
#undef _LZHAM									
#undef _MINILZ
#undef _MINIZIP 
#undef _SNAPPY									
#undef _TORNADO									
#undef _FLZP									
  #endif

  #ifdef _BALZ
#include "balz/balz.h"
  #endif

  #if defined(_BCM) || defined(_BCMEC)
#include "bcm/bcm.h"
  #endif

  #ifdef _C_BLOSC2
    #ifdef _C_BLOSC2LZ
#include "c-blosc2/blosc/blosclz.h"
    #else
#include "c-blosc2/blosc/blosc.h"
    #endif
  #endif
  
  #ifdef _BRIEFLZ 
#include "brieflz/brieflz.h"
  #endif
  
  #ifdef _LIBBSC
#include "libbsc/libbsc/libbsc.h"
#include "libbsc/libbsc/lzp/lzp.h"
  #endif

  #ifdef _BZIP2 
#include "bzip2/bzlib.h"
  #endif
  
  #ifdef _BROTLI
#include "brotli/enc/encode.h"
#include "brotli/dec/decode.h"
  #endif

  #ifdef _CSC
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
  return 0;//*inlen;
}

static size_t cscwrite(MemISeqOutStream *so, const void *out, size_t outlen) {
  memcpy(so->out, out, outlen);
  so->out    += outlen;
  so->outlen += outlen;
  return outlen;
}
  #endif

  #ifdef _CRUSH
#include <stdint.h>
#include "crush/crush.hpp"
  #endif

  #ifdef _DOBOZ
#include "doboz/Source/Doboz/Compressor.h"
#include "doboz/Source/Doboz/Decompressor.h"
  #endif

  #ifdef _GIPFELI
    #ifdef _GIPFELI0
#include "gipfeli0/gipfeli.h"
    #else
#include "gipfeli/gipfeli.h"
    #endif 
  #endif

  #ifdef _HEATSHRINK
#include "heatshrink_/heatshrink.h"
  #endif

    #ifdef _LIBLZG
#include "liblzg/src/include/lzg.h"
  #endif

  #ifdef _LZ4
#include "lz4/lib/lz4.h"
#include "lz4/lib/lz4hc.h"
  #endif
    
  #ifdef _LZ5
#include "lz5/lib/lz5.h"
#include "lz5/lib/lz5hc.h"
  #endif
    
  #ifdef _LZFSE
#include <compression.h>
  #endif  

  #ifdef _LZHAM
#include "lzham_codec_devel/include/lzham.h"
  #endif
 
  #ifdef _LZLIB
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
  
  #ifdef _LZMA
#include "LZMA-SDK/C/Alloc.h"
#include "LZMA-SDK/C/LzmaEnc.h"
#include "LZMA-SDK/C/LzmaDec.h"
  #endif

  #ifdef _LZMAT
#include "lzmat/lzmat.h"
  #endif
  
  #ifdef _LZO
    #ifdef _MINILZO
#include "minilzo/minilzo.h"
    #else
#include "lzo/include/lzo/lzo1b.h"
#include "lzo/include/lzo/lzo1c.h"
#include "lzo/include/lzo/lzo1f.h"
#include "lzo/include/lzo/lzo1x.h"
#include "lzo/include/lzo/lzo1y.h"
#include "lzo/include/lzo/lzo1z.h"
#include "lzo/include/lzo/lzo2a.h"
    #endif
  #endif
 
  #ifdef _MINIZIP
typedef unsigned long mz_ulong;
int mz_compress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len, int level);
int mz_uncompress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len);
  #endif

  #ifdef _MSCOMPRESS
#include "ms-compress/include/mscomp.h"
  #endif  

  #ifdef _LZSS
#include "lzss/lzss.h"
  #endif  

  #ifdef _PITHY
#include "pithy/pithy.h"
  #endif

  #ifdef _QUICKLZ
#include "quicklz_/quicklz-c.h"
  #endif

  #ifdef _SAP
//#include "pysap/src/hpa101saptype.h"        /* Common SAP Header Files ...............*/
//#include "pysap/src/hpa106cslzc.h"          /* Internal Definitions for LZC algorithm */
//#include "pysap/src/hpa107cslzh.h"

//#include "../sap/src/hpa104CsObject.h"
//#include "../sap/src/hpa105CsObjInt.h"

#include "pysap/pysapcompress/hpa101saptype.h"
#include "pysap/pysapcompress/hpa104CsObject.h"
#include "pysap/pysapcompress/hpa106cslzc.h"
#include "pysap/pysapcompress/hpa107cslzh.h"
#include "pysap/pysapcompress/hpa105CsObjInt.h"
  #endif
  #ifdef _SHRINKER
#include "shrinker/Shrinker.h"
  #endif
   
  #ifdef _SNAPPY
#include "snappy/snappy.h"
  #endif
   
  #ifdef _TORNADO
#include "tornado_/tormem.h"
  #endif

  #ifdef _WKDM
#include "wkdm/WKdm.h"
  #endif
  
  #ifdef _WFLZ
#include "wflz/wfLZ.h"
  #endif
  
  #ifdef _WIMLIB
#include "../wimlib/include/wimlib.h"
  #endif
  
  #ifdef _YALZ77
#include "yalz77/lz77.h"
  #endif

  #ifdef _YAPPY
#include "yappy/yappy.hpp"
  #endif

  #ifdef _ZLIB
     #ifdef _ZLIBLIB
#include <zlib.h>
     #elif defined(_ZLIB_INTEL)
#include "zlib_intel/zlib.h"
     #else
#include "zlib/zlib.h"
     #endif
  #endif
  
  #ifdef _ZLING
#include "libzling/src/libzling.h"
#include "libzling_/libzling_utils_mem.h"
  #endif  

  #ifdef _ZSTD
#include "zstd/lib/zstd.h"
#include "zstd/lib/zstdhc.h"
  #endif

  #ifdef _PLUGIN2
#include "../beplugi.h"
  #endif  
//-----------------------------
  #ifdef __cplusplus  
extern "C" {
  #endif

  #ifdef _CHAMELEON
#include "chameleon/Chameleon2.h"
  #endif

  #ifdef _DENSITY
#include "density/src/density_api.h"
  #endif

  #ifdef _FASTLZ
#include "FastLZ/fastlz.h"
  #endif

  #ifdef _LIBLZF
#include "liblzf/lzf.h"
  #endif

  #ifdef _SNAPPY_C
#include "snappy-c/snappy.h"
struct snappy_env env; 
  #endif
 
  #ifdef _ZOPFLI
#include "zopfli/src/zopfli/zopfli.h"  
  #endif

  #ifdef __cplusplus
}
  #endif

  //------------------------------------ Entropy Coder ------------------------------
  #ifdef _FASTAC
#include "fastac/arithmetic_codec.h"
  #endif
  
  #ifdef _FASTHF
#include "fasthf/binary_codec.h"
  #endif
  
  #ifdef _FSE
    #ifdef _ZSTD
#include "zstd/lib/fse.h"
#include "zstd/lib/huff0.h"
    #else
#include "FiniteStateEntropy/lib/fse.h"
#include "FiniteStateEntropy/lib/huff0.h"
     #endif
  #endif

  #ifdef _JAC
#include "ans_jb/arith_static.h"
  #endif
  
  #ifdef _JRANS
#include "ans_jb/rANS_static4c.h"
    #ifdef __x86_64__
#include "ans_jb/rANS_static64c.h"
    #endif
  #endif

#ifdef _POLHF
  #include "polar/polar.h"
#endif

  #ifdef _MMABS
#include "ans_mm/fpaqc.h"
  #endif
  
  #ifdef _MMRC
#include "fpaq0f2/fpaq0f2.h"
#include "fpaq0p/fpaq0p_mm.h"
#include "o0zp/o0zp.h"
  #endif

  #ifdef _NANS
#include "ans_nania/narans.h"
  #endif

  #ifdef _SUBOTIN
#include "subotin_/subotin.h"
  #endif

  #ifdef _SHRC
#include "fpaq0p/fpaq0p_sh.h"
#include "vecrc/vector_rc.h"
  #endif  

  #ifdef _ZLIBH
#include "zlibh/zlibh.h"
  #endif
  
  #ifdef __cplusplus  
extern "C" {
  #endif

  #ifdef _FASTARI
#include "FastARI/FastAri.h"
  #endif

  #ifdef _FSC
#include "fsc/fsc.h"
  #endif

  #ifdef __cplusplus
}
  #endif
//---------------------------------------------------------------------------------------
  #ifndef max
#define max(x,y) (((x)>(y)) ? (x) : (y))
  #endif

void libmemcpy(unsigned char *dst, unsigned char *src, int len) {
  void *(*memcpy_ptr)(void *, const void *, size_t) = memcpy;
  if (time(NULL) == 1) 
    memcpy_ptr = NULL;
  memcpy_ptr(dst, src, len);
}

static char *workmem;
static int state_size,dstate_size,lzosize,blzsize;

int codini(unsigned insize) {
  size_t workmemsize = 0;
    #ifdef _C_BLOSC2
  blosc_init();
  blosc_set_nthreads(1);  //blosc_set_compressor("lz4");
    #endif

    #ifdef _LZFSE
  size_t lzfse_size = compression_encode_scratch_buffer_size(COMPRESSION_LZFSE);
  workmemsize = max(lzfse_size, workmemsize);
         lzfse_size = compression_decode_scratch_buffer_size(COMPRESSION_LZFSE);
  workmemsize = max(lzfse_size, workmemsize);
    #endif

    #ifdef LZO_VERSION
	   #if 1
  lzo_init(); lzosize = LZO1X_MEM_COMPRESS;
	   #else
  lzo_init(); lzosize = LZO1B_999_MEM_COMPRESS; 
       #endif
  workmemsize = max(lzosize, workmemsize);
    #endif
	
    #ifdef _CHAMELEON
  workmemsize = max(sizeof(struct Chameleon), workmemsize);	
	#endif
	
    #ifdef _QUICKLZ
  state_size  = max(qlz_get_setting1(1), max(qlz_get_setting2(1), qlz_get_setting3(1))); workmemsize = max(state_size, workmemsize);
  dstate_size = max(qlz_get_setting1(2), max(qlz_get_setting2(2), qlz_get_setting3(2))); workmemsize = max(dstate_size, workmemsize);
    #endif
 
    #ifdef _BRIEFLZ
  blzsize     = blz_workmem_size(insize); 												 
  workmemsize = max(blzsize, workmemsize); 
    #endif  
    #ifdef _LINUX_SNAPPY_H
  snappy_init_env(&env);
    #endif
    
    #ifdef _WFLZ	
  unsigned wflzsize = wfLZ_GetWorkMemSize();
  workmemsize = max(wflzsize, workmemsize);
    #endif

  if(workmemsize && !(workmem = (char *)malloc(workmemsize))) { printf("Malloc error: %d\n", workmemsize); exit(0); }

    #ifdef _YAPPY
  YappyFillTables(); 	
    #endif
    #ifdef _LIBBSC
  bsc_init(LIBBSC_FEATURE_FASTMODE);
    #endif
}  

void codexit() { 
  if(workmem) free(workmem); workmem = NULL;
    #ifdef _LINUX_SNAPPY_H
  snappy_free_env(&env);
    #endif
} 

int brotlidic,brotlictx,brotlirep;

int codcomp(unsigned char *in, int inlen, unsigned char *out, int outsize, int codec, int lev, char *prm) {  int outlen; unsigned char *oend=out+outsize;
  switch(codec) { 
      #ifdef _PLUGIN2  
    #include "../beplugc.c"
      #endif
	  
	  #ifdef _BALZ
	case P_BALZ: return balzcompress(in, inlen, out,lev);
      #endif 
 
	  #ifdef _BCM
    case P_BCM: return bcmcompress(in, inlen, out); break;
      #endif 

      #ifdef _C_BLOSC2
    case P_C_BLOSC2:
        #ifdef _C_BLOSC2LZ
      return blosclz_compress(lev, in, inlen, out, outsize);
        #else
      return blosc_compress(lev/*clevel*/, 0/*doshuffle*/, 1/*typesize*/, inlen, in, out, outsize /*inlen+BLOSC_MAX_OVERHEAD*/);
        #endif 
      #endif 

      #ifdef _BRIEFLZ
	case P_BRIEFLZ: return blz_pack(in, out, inlen, workmem);
	  #endif
	  
      #ifdef _BROTLI
    case P_BROTLI: { brotli::BrotliParams params;params.quality=lev; 
         if(params.quality==11) params.lgwin = 24; if(*prm=='w') { prm++; params.lgwin=22;} else if(strchr(prm,'W')) { prm++; params.lgwin=24;} if(strchr(prm,'D')) brotlidic++; if(strchr(prm,'R')) brotlirep++; if(strchr(prm,'X')) brotlictx++;
         size_t esize=outsize; int rc = brotli::BrotliCompressBuffer(params, inlen,in,&esize,out); brotlidic = brotlictx = brotlirep = 0; return rc?esize:0; 
      }
	  #endif    

      #ifdef _LIBBSC
	    #define LZPHASHSIZE 18 //LIBBSC_DEFAULT_LZPHASHSIZE
	case P_LIBBSC_ST: return bsc_compress(in, out, inlen, 0/*LZPHASHSIZE*/, 0/*LIBBSC_DEFAULT_LZPMINLEN*/, lev/*codec-_BSC_ST3+LIBBSC_BLOCKSORTER_ST3*/, lev>4?LIBBSC_CODER_QLFC_ADAPTIVE:LIBBSC_CODER_QLFC_STATIC, 0/*LIBBSC_FEATURE_FASTMODE /*| LIBBSC_FEATURE_MULTITHREADING*/);
	case P_LIBBSC: return bsc_compress(in, out, inlen, LZPHASHSIZE, 32, LIBBSC_BLOCKSORTER_BWT, lev, LIBBSC_DEFAULT_FEATURES/*LIBBSC_FEATURE_FASTMODE | LIBBSC_FEATURE_MULTITHREADING*/);
	  #endif

      #ifdef _BZIP2
	case P_BZIP2:    { unsigned outlen = outsize; return BZ2_bzBuffToBuffCompress((char *)out, &outlen, (char *)in, inlen, 9, 0, 0)==BZ_OK?outlen:-1; }
	  #endif
	  
	  #ifdef _CHAMELEON
    case P_CHAMELEON:  { Chameleon_Reset((Chameleon *)workmem); return lev<2?Chameleon_Encode((Chameleon *)workmem,out,in, inlen):Chameleon2_Encode((Chameleon *)workmem,out,in, inlen); }
	  #endif

      #ifdef _CSC
    case P_CSC: { 
        CSCProps prop; CSCEncProps_Init(&prop, inlen<(1<<30)?inlen:(1<<30), lev); CSCEnc_WriteProperties(&prop, (uint8_t*)out, 0);
        MemISeqInStream  si; si.s.Read  = (int(*)(void *, void *, size_t *))cscread;  si.in  = in;                  si.inlen  = inlen;
	    MemISeqOutStream so; so.s.Write = (size_t(*)(void *, const void *, size_t  ))cscwrite; so.out = out + CSC_PROP_SIZE; so.outlen = CSC_PROP_SIZE;
	    CSCEncHandle eh = CSCEnc_Create(&prop, (ISeqOutStream*)&so); CSCEnc_Encode(eh, (ISeqInStream*)&si, NULL); CSCEnc_Encode_Flush(eh); CSCEnc_Destroy(eh);
        return so.outlen;
      }
      #endif
      #ifdef _CRUSH
    case P_CRUSH: return crush::compress(lev, in, inlen, out);
      #endif
	  
      #ifdef _DENSITY
    case P_DENSITY: { 
	    density_buffer_processing_result rs = density_buffer_compress((const uint8_t *)in, inlen,                                 (uint8_t*)out,      outsize, (const DENSITY_COMPRESSION_MODE)lev/*, DENSITY_BLOCK_TYPE_DEFAULT, NULL, NULL*/);
        return rs.state?0:rs.bytesWritten;
	  }
      #endif        
       	  
      #ifdef _DOBOZ
	case P_DOBOZ:   { doboz::Compressor c; size_t rc; return c.compress(in,inlen, out, c.getMaxCompressedSize(inlen), rc) == doboz::RESULT_OK ? rc : -1; }  
	  #endif
	  
      #ifdef _FASTLZ
    case P_FASTLZ:  return fastlz_compress_level(lev, in, inlen, out);
	  #endif
	  
 	  #ifdef _GIPFELI 
    case P_GIPFELI:
 	    #ifdef _GIPFELI0
	                { gipfeli::Compressor c; c.Init(); outlen = c.Compress((char *)in, inlen, (char *)out); return outlen; }
        #else
    case P_GIPFELI: { 
        util::compression::Compressor *c = util::compression::NewGipfeliCompressor();
        util::compression::UncheckedByteArraySink sink((char*) out);
        util::compression::ByteArraySource        src((const char*)in, inlen);
        outlen = c->CompressStream(&src, &sink); delete c; return outlen;
	  }	
        #endif
	  #endif
 
      #ifdef _HEATSHRINK
    case P_HEATSHRINK:   return hscompress(in, inlen, out); 
      #endif

	  #ifdef _LIBLZF  
    case P_LIBLZF:    return lzf_compress(in, inlen, out, outsize);
	  #endif
	
	  #ifdef _LZ4
    case P_LZ4: return !lev?LZ4_compress_fast((char *)in, (char *)out, inlen, outsize, 4):(lev<9?LZ4_compress_default((char *)in, (char *)out, inlen, outsize):LZ4_compress_HC((char *)in, (char *)out, inlen, outsize, lev));
	  #endif

	  #ifdef _LZ5
    case P_LZ5: return !lev?LZ5_compress_fast((char *)in, (char *)out, inlen, outsize, 4):(lev<9?LZ5_compress_default((char *)in, (char *)out, inlen, outsize):LZ5_compress_HC((char *)in, (char *)out, inlen, outsize, lev));
	  #endif

      #ifdef _LZFSE
    case P_LZFSE_ : return compression_encode_buffer(out, outsize, in, inlen, workmem, COMPRESSION_LZFSE); break;
      #endif

      #ifdef _LZHAM
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
      #ifdef _LZMA
	case P_LZMA: { CLzmaEncProps p;	LzmaEncProps_Init(&p); p.level = lev; p.numThreads = 1; if(lev==9) p.fb = 273,p.dictSize=inlen<DICSIZE?inlen:DICSIZE; LzmaEncProps_Normalize(&p);
        SizeT psize = LZMA_PROPS_SIZE, outlen = outsize - LZMA_PROPS_SIZE;
  	    return LzmaEncode(out+LZMA_PROPS_SIZE, &outlen, in, inlen, &p, out, &psize, 0, NULL, &g_Alloc, &g_Alloc) == SZ_OK?outlen+LZMA_PROPS_SIZE:0;
	  }
      #endif
	
      #ifdef _LZLIB
	case P_LZLIB:  { unsigned outlen; bbcompress( (const uint8_t *)in, inlen, (uint8_t *)out, (int * const)&outlen,  option_mapping[lev].dictionary_size, option_mapping[lev].match_len_limit); return outlen; }
      #endif
	    
	  #ifdef _LIBLZG
	case P_LIBLZG: { lzg_encoder_config_t cfg; LZG_InitEncoderConfig(&cfg); cfg.fast = LZG_TRUE; cfg.level = lev; return LZG_Encode(in, inlen, out, outsize, &cfg); }
	  #endif

	  #ifdef _LZMAT
	case P_LZMAT:   { MP_U32 outs=outsize; int rc = lzmat_encode(out, &outs, in, inlen); return rc == LZMAT_STATUS_OK ? outs : 0; }
      #endif

	  #ifdef _LZO
	    #ifdef _MINILZO
    case P_LZO1x: { lzo_uint out_len; lzo1x_1_compress(in, inlen, out, &out_len, workmem); return out_len; }
	    #else
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
      #endif 
 
      #ifdef _LZSS
	case P_LZSS1:   { return lzss_encode(in, inlen, out, 0); }
    case P_LZSS2:   { return lzss_encode(in, inlen, out, 1); }
	  #endif

      #ifdef _MSCOMPRESS
    case P_MSCOMPRESS: { size_t outlen=outsize; return ms_compress(lev, in, inlen, out, &outlen)?0:outlen; }
      #endif

 	  #ifdef _MINIZIP
    case P_MINZ:   { uLongf outlen = inlen+16; int rc = mz_compress2(out, &outlen, in, inlen, lev); if (rc != Z_OK) printf("miniz compress2 rc=%d\n", rc);	return outlen; }
      #endif

	  #ifdef _PITHY
    case P_PITHY: return pithy_Compress((const char *)in, inlen, (char *)out, outsize, lev);
      #endif  	

      #ifdef _QUICKLZ
    case P_QUICKLZ: { memset(workmem,0,state_size); return lev<=1?qlz_compress1((char *)in, (char *)out, inlen, workmem):(lev<=2?qlz_compress2((char *)in, (char *)out, inlen, workmem):qlz_compress3((char *)in, (char *)out, inlen, workmem)); }
      #endif  	

	  #ifdef _SAP
    case P_SAP: { CsObjectInt c; SAP_INT bytes_read, bytes_written; int rc = c.CsInitCompr((SAP_BYTE *)out, inlen, lev); out += CS_HEAD_SIZE; outsize -= CS_HEAD_SIZE;
        c.CsCompr(inlen, in, inlen, out, outsize, lev, &bytes_read, &bytes_written); return bytes_written+CS_HEAD_SIZE; 
      }
      #endif  	

	  #ifdef _SHRINKER
    case P_SHRINKER:     return shrinker_compress((char *)in, (char *)out, inlen);
      #endif  	
    
	  #ifdef _SNAPPY 
	    #ifdef __cplusplus
    case P_SNAPPY:    { size_t outlen=outsize; snappy::RawCompress((char *)in, inlen, (char*)out, &outlen); return outlen;}
        #else
    case P_SNAPPY:    { size_t outlen=outsize; int rc = snappy_compress(in, inlen, out, &outlen); return outlen;}
	    #endif 
	  #endif	

	  #ifdef _SNAPPY_C
    case P_SNAPPYC:   { size_t outlen=outsize; int rc = snappy_compress(&env, (const char *)in, inlen, (char *)out, &outlen); return outlen;}
	  #endif 
	  
      #ifdef _TORNADO
    case P_TORNADO:      return torcompress(lev, in, out, inlen);
      #endif	

      #ifdef _WFLZ
    case P_WFLZ:    return lev<=1?wfLZ_CompressFast( (const uint8_t* WF_RESTRICT const)in, inlen, (uint8_t* WF_RESTRICT const)out, (const uint8_t* WF_RESTRICT)workmem, 0 ):
                                      wfLZ_Compress( (const uint8_t* WF_RESTRICT const)in, inlen, (uint8_t* WF_RESTRICT const)out, (const uint8_t* WF_RESTRICT)workmem, 0 );
	  #endif

      #ifdef _WIMLIB
    case P_WIMLIB: { struct wimlib_compressor *compressor; if(wimlib_create_compressor(lev, 32*1024/*inlen*/, 0, &compressor)) return 0;
        outlen = wimlib_compress(in, inlen, out, outsize /*inlen- 1*/, compressor);  wimlib_free_compressor(compressor); return outlen; 
      }
	  #endif

      #ifdef _WKDM
    case P_WKDM:    return WKdm_compress ((WK_word*)in, (WK_word*)out, inlen);
	  #endif

      #ifdef _YALZ77
    case P_YALZ77: { lz77::compress_t c(lev, lz77::DEFAULT_BLOCKSIZE); std::string os = c.feed(in,in+inlen); memcpy(out, os.c_str(), os.size()); return os.size(); }
	  #endif

      #ifdef _YAPPY
    case P_YAPPY:    return YappyCompress(in, out, inlen, 10)-out;
	  #endif

      #ifdef _ZLIB
    case P_ZLIB:     { uLongf outlen = outsize; int rc = compress2(out, &outlen, in, inlen, lev); if(rc != Z_OK) printf("zlib compress2 rc=%d\n", rc);	return outlen; }
      #endif
	  
      #ifdef _ZLING
    case P_ZLING:    return zling_compress(lev, in, inlen, out, outsize);
      #endif     
	  
	  #ifdef _ZOPFLI
	case P_ZOPFLI:  { 
        ZopfliOptions opts; ZopfliInitOptions(&opts); size_t outlen = 0; unsigned char *pout = out;
        ZopfliCompress(&opts, ZOPFLI_FORMAT_ZLIB, in, inlen, &pout, &outlen); 
        if(pout != out) 
          memcpy(out, pout, outlen);
	    return outlen; 
	 }
      #endif	    

	  #ifdef _ZSTD
    case P_ZSTD: ZSTD_HC_compress( out, outsize, in, inlen, lev); break;
      #endif   

    //------------------------- Entropy Coders -------------------------
      #ifdef _IMEMCPY 
    case P_MCPY:   imemcpy(out, in, inlen);    return inlen;
	  #endif	

      #ifdef _MEMCPY 
    case P_LMCPY:   libmemcpy(out, in, inlen); return inlen;
	  #endif	

      #ifdef _BCMEC
	case P_BCMEC:  return bcmenc(in, inlen, out);
	  #endif

    #define SYMNUM 0x100	
	  #ifdef _FASTAC
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
 
      #ifdef _FASTARI
    case P_FASTARI:  { size_t outlen = outsize; return fa_compress((const unsigned char *)in, out, inlen, &outlen)?0:outlen; }
      #endif

	  #ifdef _FASTHF
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

	  #ifdef _FSC
    case P_FSC:     { size_t outlen = 0; uint8_t *op = NULL; int ok = FSCEncode(in, inlen, &op, &outlen, 12, CODING_METHOD_DEFAULT); if(ok) { memcpy(out, op, outlen); free(op); } return outlen; }
	  #endif 

	  #ifdef _FSE
    case P_FSE:     { size_t o = FSE_compress(out, outsize, in, inlen); if(o == 1) { out[0] = in[0]; return 1; } if(!o || o >= inlen) { memcpy(out, in, inlen); return inlen; } return o; }
    case P_FSEH:    { size_t o = HUF_compress(out, outsize, in, inlen); if(o == 1) { out[0] = in[0]; return 1; } if(!o || o >= inlen) { memcpy(out, in, inlen); return inlen; } return o; }
      #endif 
	
	  #ifdef _NANS
    case P_NANS:   return nansenc(in, inlen, out, oend);
      #endif

      #ifdef _JAC
    case P_JAC:  { unsigned outlen; arith_compress_O0(in, inlen, &outlen, out); return outlen; } 
      #endif

	  #ifdef _JRANS
    case P_JRANS:   return rans_compress_O0(in, inlen, out, oend);
    case P_JRANS1:  return rans_compress_O1(in, inlen, out, oend); 
        #ifdef __x86_64__
    case P_JRANS64: return rans64_compress_O0(in, inlen, out, oend);
    case P_JRANS641:return rans64_compress_O1(in, inlen, out, oend);
        #endif
	  #endif

      #ifdef _POLHF
    case P_POLHF:   return polarenc(in, inlen, out); 
      #endif

      #ifdef _SUBOTIN 
    case P_SUB:     return subenc(in, inlen, out, inlen, SYMNUM);
      #endif
  
      #ifdef _TORNADOHF 
    case P_TORNADOHF:     return torhenc(in, inlen, out, outsize); 
	  #endif
      
	  #ifdef _ZLIBH
    case P_ZLIBH:   return ZLIBH_compress((char* )out, (const char*)in, inlen); 
	  #endif  
    defaulf: fprintf(stderr, "library '%d' not included\n", codec);
  } 
} 
  
int coddecomp(unsigned char *in, int inlen, unsigned char *out, int outlen, int codec, int lev) {	
  switch(codec) {
      #ifdef _PLUGIN2  
    #include "../beplugd.c"
      #endif

  	  #ifdef _BALZ
	case P_BALZ: return balzdecompress(in, inlen, out, outlen);
      #endif

	  #ifdef _BCM
    case P_BCM: return bcmdecompress(in, inlen, out, outlen);
      #endif 

      #ifdef _C_BLOSC2
    case P_C_BLOSC2: return blosc_decompress(in, out, outlen);
      #endif

      #ifdef _C_BLOSC2LZ
    case P_C_BLOSC2LZ: return blosclz_decompress(in, inlen, out, outlen);
      #endif
	  
	  #ifdef _BRIEFLZ
	case P_BRIEFLZ:     return blz_depack(in, out, outlen);
	  #endif
	  
      #ifdef _BROTLI
    case P_BROTLI: { size_t dsize = outlen; int rc = BrotliDecompressBuffer(inlen,in,&dsize,out); return rc?dsize:0; }
	  #endif

      #ifdef _LIBBSC
	case P_LIBBSC_ST: 
	case P_LIBBSC:	   return bsc_decompress(in, inlen, out, outlen, 0);
	  #endif
	
      #ifdef _BZIP2 	  
 	case P_BZIP2: { unsigned outsize = outlen; return BZ2_bzBuffToBuffDecompress((char *)out, &outsize, (char *)in, inlen, 0, 0)==BZ_OK?outlen:-1; }
      #endif
	  
	  #ifdef _CHAMELEON
    case P_CHAMELEON:  { Chameleon_Reset((Chameleon *)workmem); lev<2?Chameleon_Decode((Chameleon *)workmem, out, outlen, in):Chameleon2_Decode((Chameleon *)workmem, out, outlen, in); return inlen; }
	  #endif

      #ifdef _CSC
    case P_CSC: { 
        CSCProps prop; CSCDec_ReadProperties(&prop, (uint8_t*)in);
        MemISeqInStream  si; si.s.Read  = (int(*)(void *, void *, size_t *))cscread;  si.in  = in + CSC_PROP_SIZE; si.inlen  = inlen - CSC_PROP_SIZE;
	    MemISeqOutStream so; so.s.Write = (size_t(*)(void *, const void *, size_t  ))cscwrite; so.out = out;                so.outlen = 0;
	    CSCDecHandle dh = CSCDec_Create(&prop, (ISeqInStream*)&si); CSCDec_Decode(dh, (ISeqOutStream*)&so, NULL); CSCDec_Destroy(dh);
        return si.inlen;
      }
      #endif

     #ifdef _CRUSH
    case P_CRUSH: crush::decompress(in, out, outlen); break;
	  #endif
	  
      #ifdef _DENSITY
    case P_DENSITY: { density_buffer_processing_result rs = density_buffer_decompress((uint8_t *)in, inlen, (uint8_t*)out, outlen+DENSITY_MINIMUM_OUTPUT_BUFFER_SIZE);  return rs.state?0:rs.bytesWritten; }
      #endif

      #ifdef _FASTARI
    case P_FASTARI:  { size_t outsize = outlen; return fa_decompress((const unsigned char *)in, out, inlen, &outsize)?0:inlen; }
      #endif
      #ifdef _DOBOZ
	case P_DOBOZ:   { doboz::Decompressor d; return d.decompress(in, inlen, out, outlen) == doboz::RESULT_OK ? outlen : -1; }
      #endif

	  #ifdef _FASTLZ
    case P_FASTLZ: fastlz_decompress(in, inlen, out, outlen); break;
	  #endif
	   
 	  #ifdef _GIPFELI
    case P_GIPFELI:
 	    #ifdef _GIPFELI0
                    { gipfeli::Uncompressor uncompressor; uncompressor.Init(); int s = uncompressor.Uncompress((const char *)in, inlen, (char *)out, outlen); } break;
	    #else
      { 
	    util::compression::Compressor *c = util::compression::NewGipfeliCompressor();
        util::compression::UncheckedByteArraySink sink((char*) out);
        util::compression::ByteArraySource         src((const char*)in, inlen);
        outlen = c->UncompressStream(&src, &sink); delete c; return outlen;
	  }  
        #endif
      break;
	  #endif

      #ifdef _HEATSHRINK
    case P_HEATSHRINK: return hsdecompress(in, inlen, out, outlen); 
      #endif

	  #ifdef _LIBLZF 
    case P_LIBLZF: lzf_decompress(in, inlen, out, outlen); break;
	  #endif

	  #ifdef _LZ4
    case P_LZ4: LZ4_decompress_safe((const char *)in, (char *)out, inlen, outlen); break;
      #endif
	  
	  #ifdef _LZ5
    case P_LZ5: LZ5_decompress_safe((const char *)in, (char *)out, inlen, outlen); break;
      #endif
	  
      #ifdef _LZFSE
    case P_LZFSE_ : return compression_decode_buffer(out, outlen, in, inlen, workmem, COMPRESSION_LZFSE); break;
      #endif

	  #ifdef _LIBLZG
	case P_LIBLZG: LZG_Decode(in, inlen, out, outlen); break;
	  #endif
	  
      #ifdef _LZHAM
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
	  
      #ifdef _LZLIB
	case P_LZLIB: { int out_len = outlen; bbdecompress( in, outlen, out, &out_len ); } break;
      #endif 
	  
	  #ifdef _LZMA
	case P_LZMA: {  
	    SizeT ol = outlen, il = inlen - LZMA_PROPS_SIZE; ELzmaStatus sts;
	    return LzmaDecode(out, &ol, in+LZMA_PROPS_SIZE, &il, in, LZMA_PROPS_SIZE, LZMA_FINISH_END, &sts, &g_Alloc)?0:inlen;
      }
      #endif

	  #ifdef _LZMAT
	case P_LZMAT:  { MP_U32 rc = outlen; lzmat_decode(out, &rc, in, inlen); return rc; }  
	  #endif
	    
	  #ifdef _LZO
    case P_LZO1x: { lzo_uint out_len;   lzo1x_decompress(in, inlen, out, &out_len, NULL); break; }
        #ifndef _MINILZO
    case P_LZO1b: { lzo_uint out_len;   lzo1b_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1c: { lzo_uint out_len;   lzo1c_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1f: { lzo_uint out_len;   lzo1f_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1y: { lzo_uint out_len;   lzo1y_decompress(in, inlen, out, &out_len, NULL); break; }
    case P_LZO1z: { lzo_uint out_len;   lzo1z_decompress(in, inlen, out, &out_len, NULL); break; }    
    case P_LZO2a: { lzo_uint out_len;   lzo2a_decompress(in, inlen, out, &out_len, NULL); break; }
        #endif 
      #endif 

      #ifdef _MINIZIP
    case P_MINZ: { uLongf outsize = outlen; int rc = mz_uncompress(out, &outsize, in, inlen); } break;
      #endif

	  #ifdef _PITHY
    case P_PITHY: return pithy_Decompress((const char *)in, inlen, (char *)out, outlen);
      #endif  	

      #ifdef _QUICKLZ
    case P_QUICKLZ: { lev= (in[0]>>2)&3; outlen = lev<=1?qlz_decompress1((char*)in, out, workmem):(lev<=2?qlz_decompress2((char*)in, out, workmem):qlz_decompress3((char*)in, out, workmem)); } break;
      #endif
	  
	  #ifdef _SAP
    case P_SAP: { CsObjectInt d; SAP_INT bytes_read, bytes_written; 
	    d.CsInitDecompr((SAP_BYTE *)in); in += CS_HEAD_SIZE; d.CsDecompr( (SAP_BYTE *)in, inlen-CS_HEAD_SIZE, (SAP_BYTE *)out, outlen, lev, &bytes_read, &bytes_written );			 
	    return inlen-CS_HEAD_SIZE; 
	  }
      #endif  	

	  #ifdef _SHRINKER
    case P_SHRINKER:    shrinker_decompress(in, out, outlen); break;
	  #endif
	   
      #ifdef _ZLING
    case P_ZLING: zling_decompress(in, inlen, out, outlen); break;
      #endif

	  #ifdef _LZSS
    case P_LZSS: lzss_decode(in, out, outlen); break;
	  #endif

      #ifdef _MSCOMPRESS
     case P_MSCOMPRESS: { size_t _outlen=outlen; return ms_decompress(lev, in, inlen, out, &_outlen)==MSCOMP_OK?inlen:0; }
      #endif	 
	  #ifdef _SNAPPY
        #ifdef __cplusplus
    case P_SNAPPY: snappy::RawUncompress((char*)in, inlen, (char*)out);  break;
        #else 
    case P_SNAPPY: snappy_uncompress(in, inlen, out, &outlen);	
        #endif
	  #endif	
	  
	  #ifdef _SNAPPY_C 
	case P_SNAPPYC: return snappy_uncompress((const char *)in, inlen, (char *)out);
	  #endif  
	  
      #ifdef _TORNADO
    case P_TORNADO:     return tordecompress(in, out, inlen, outlen);
      #endif
	  
      #ifdef _WFLZ
    case P_WFLZ:    wfLZ_Decompress( in, out); return inlen;
	  #endif

      #ifdef _WIMLIB
    case P_WIMLIB: { struct wimlib_decompressor *decompressor; if(wimlib_create_decompressor(lev, 32*1024/*inlen*/, &decompressor)) return 0;
        outlen = wimlib_decompress(in, inlen, out, outlen, decompressor);  wimlib_free_decompressor(decompressor); return inlen;
      }
	  #endif
 
      #ifdef _WKDM
    case P_WKDM:    WKdm_decompress ((WK_word*)in, (WK_word*)out, outlen);
	  #endif 

      #ifdef _YALZ77
    case P_YALZ77: { lz77::decompress_t d; std::string extra; if(!d.feed(in,in+inlen,extra) || extra.size() > 0) return 0;
        const std::string& os = d.result(); memcpy(out, os.c_str(), os.size()); return os.size(); 
	  }
	  #endif

	  #ifdef _YAPPY
	case P_YAPPY:    return YappyUnCompress(in, in+inlen, out)-out; 
      #endif
		 
      #ifdef _ZLIB
    case P_ZLIB: case P_ZOPFLI: { uLongf outsize = outlen; int rc = uncompress(out, &outsize, in, inlen); } break;
      #endif

      #ifdef _ZSTD
    case P_ZSTD: ZSTD_decompress( out, outlen, in, inlen); break;
      #endif
      //------------ Entropy Coders ------------------------------------------------------------------
      #ifdef _IMEMCPY 
    case P_MCPY:    imemcpy(out, in, outlen); 	break;
      #endif
      #ifdef _MEMCPY 
    case P_LMCPY:   libmemcpy(out, in, outlen); break;
      #endif

      #ifdef _BCMEC
	case P_BCMEC:   return bcmdec(in, inlen, out, outlen);
	  #endif

	  #ifdef _FASTAC
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

	  #ifdef _FASTHF
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
	  
	  #ifdef _FSC
    case P_FSC:     { size_t outsize = 0; uint8_t *op = NULL; int ok = FSCDecode(in, inlen, &op, &outsize); if(ok) { memcpy(out,op,outlen); free(op); } } break;
	  #endif
 
	  #ifdef _FSE
    case P_FSE:  if(inlen == outlen) memcpy(out, in, outlen); else if(inlen == 1) memset(out,in[0],outlen); else FSE_decompress(out, outlen, in, inlen); break;
    case P_FSEH: if(inlen == outlen) memcpy(out, in, outlen); else if(inlen == 1) memset(out,in[0],outlen); else HUF_decompress(out, outlen, in, inlen); break;
	  #endif
		
      #ifdef _JAC
    case P_JAC:  { unsigned outlen; arith_uncompress_O0(in, inlen, &outlen, out); } break;
      #endif

	  #ifdef _JRANS
    case P_JRANS:   rans_uncompress_O0(  in, inlen, out, outlen); break;
    case P_JRANS1:  rans_uncompress_O1(  in, inlen, out, outlen); break;
        #ifdef __x86_64__
    case P_JRANS64: rans64_uncompress_O0(in, inlen, out, outlen); break;
    case P_JRANS641:rans64_uncompress_O1(in, inlen, out, outlen); break;
        #endif
	  #endif

      #ifdef _MMABS
    case P_MMABS:   absd(in, outlen, out); break;
      #endif

      #ifdef _MMRC     
    case P_O0ZP:    rczmmd(in, outlen, out); break;
    case P_FPAQ0F2: rcmm2d(in, outlen, out); break;
    case P_MMRC:    rcmmd( in, outlen, out); break;
      #endif

	  #ifdef _NANS
    case P_NANS:    nansdec(in, inlen, out, outlen); break;
	  #endif

      #ifdef _POLHF
    case P_POLHF:   polardec(in, out, outlen); break;
      #endif

      #ifdef _SUBOTIN
    case P_SUB:     subdec(in, outlen, out, outlen, SYMNUM); break;
      #endif

      #ifdef _TORNADOHF
    case P_TORNADOHF:    torhdec(in, inlen, out, outlen); break;
      #endif 

	  #ifdef _ZLIBH		
    case P_ZLIBH:  return ZLIBH_decompress((char* )out, (const char*)in);
	  #endif	
  }
}

char *codver(int codec, char *v, char *s) {
  switch(codec) {  
      #ifdef _C_BLOSC2
    return BLOSC_VERSION_STRING;
      #endif 
	  #ifdef _BRIEFLZ
    case P_BRIEFLZ: sprintf(s,"%d.%d.%d", BLZ_VER_MAJOR, BLZ_VER_MINOR, BLZ_VER_PATCH); return s;
	  #endif

	  #ifdef _LZ4
    case P_LZ4:     sprintf(s,"%d.%d.%d", LZ4_VERSION_MAJOR, LZ4_VERSION_MINOR, LZ4_VERSION_RELEASE); return s;
	  #endif

	  #ifdef _LZ5
    case P_LZ5:     sprintf(s,"%d.%d.%d", LZ5_VERSION_MAJOR, LZ5_VERSION_MINOR, LZ5_VERSION_RELEASE); return s;
	  #endif

	  #ifdef _ZSTD
    case P_ZSTD:    sprintf(s,"%d.%d.%d", ZSTD_VERSION_MAJOR, ZSTD_VERSION_MINOR, ZSTD_VERSION_RELEASE); return s;
      #endif

	  #ifdef _DENSITY
    case P_DENSITY: sprintf(s,"%d.%d.%d", density_version_major(), density_version_minor(), density_version_revision()); return s;
	  #endif

      #ifdef _HEATSHRINK
    case P_HEATSHRINK: sprintf(s,"%d.%d.%d", HEATSHRINK_VERSION_MAJOR, HEATSHRINK_VERSION_MINOR, HEATSHRINK_VERSION_PATCH); return s;
      #endif

	  #ifdef _SNAPPY
    case P_SNAPPY:  sprintf(s,"%d.%d.%d", SNAPPY_MAJOR, SNAPPY_MINOR, SNAPPY_PATCHLEVEL); return s;
	  #endif
	default:        strcpy(s,v);
  }
  return s;
}

