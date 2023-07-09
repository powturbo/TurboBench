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


unsigned coddecomp(unsigned char *in, unsigned inlen, unsigned char *out, unsigned outlen, int codec, int lev, char *prm) { 
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

      #if _LIBLZF
    case P_LIBLZF: lzf_decompress(in, inlen, out, outlen); break;
      #endif

if _LIZARD
    case P_LIZARD: return Lizard_decompress_safe((const char *)in, (char *)out, inlen, outlen);
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

      #if _LIBLZG
    case P_LIBLZG: LZG_Decode(in, inlen, out, outlen); break;
      #endif

      #if _ZPAQ
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

      #if _QCOMPRESS
    case P_QCOMPRESS32: { FfiVec v = auto_decompress_i32_(in, inlen); memcpy(out, v.ptr, outlen); free_i32_(v); return outlen;}
    case P_QCOMPRESS64: { FfiVec v = auto_decompress_i64_(in, inlen); memcpy(out, v.ptr, outlen); free_i64_(v); return outlen;}
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
     case P_MSCOMPRESS: { size_t _outlen = outlen; return ms_decompress((MSCompFormat)lev, in, inlen, out, &_outlen)==MSCOMP_OK?inlen:0; }
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
      if(inlen >= outlen) { memcpy(out,in, outlen); return inlen; }
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


