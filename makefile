# powturbo  (c) Copyright 2013-2016
# ----------- Downloading + Compiling ----------------------
# git clone --recursive git://github.com/powturbo/TurboBench.git 
# make
#
# snappy:    "cp snappy_/* snappy" (or configure snappy) & type make
# GPL: "make GPL=1" to include GPL libraries
# Minimum make: "make NCOMP2=1 NECODER=1 NSIMD=1" to compile only lz4,brotli,lzma,zlib and zstd

# Linux: "export CC=clang" "export CXX=clang". windows mingw: "set CC=gcc" "set CXX=g++" or uncomment the CC,CXX lines
CC ?= gcc
CXX ?= g++
#CC=clang
#CXX=clang++
#----- Compile for 'American Fuzzy Lop (http://lcamtuf.coredump.cx/afl/) 
# add '-static' to LDFLAG
# Run turbobench w/o output : afl-fuzz -i testcase_dir -o finding_dir -- ./turbobench -elzturbo,12 -v0 -gg -k0
#CC=~/b/afl/afl-gcc 
#CXX=~/b/afl/afl-g++

#----------------------------------------------
ifeq ($(OS),Windows_NT)
  UNAME := Windows
CC=gcc
CXX=g++
else
  UNAME := $(shell uname -s)
ifeq ($(UNAME),$(filter $(UNAME),Linux Darwin FreeBSD GNU/kFreeBSD))
LDFLAGS+=-lpthread -lrt
endif
endif

ifeq ($(STATIC),1)
LDFLAGS+=-static
NMEMSIZE=1
endif

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
ARCH=64
else
ifeq ($(ARCH),32)
CFLAGS=-fomit-frame-pointer
else
ARCH=64
endif
endif

#---------------------- make args --------------------------
# best or popular compressors
ifeq ($(NCOMP1),1)
DEFS+=-DNCOMP1
else
NCOMP1=0
endif

# other compressors
ifeq ($(NCOMP2),1)
DEFS+=-DNCOMP2
else
NCOMP2=0
endif

# Entropy coders
ifeq ($(NECODER),1)
DEFS+=-DNECODER
else
NECODER=0
endif

# Encoding: RLE
ifeq ($(NENCOD),1)
DEFS+=-DNENCOD
else
NENCOD=0
endif

ifeq ($(GPL),1)
DEFS+=-DGPL
DEFS+=-Ilzo/include 
endif

# disable peak memory calculation
ifeq ($(NMEMSIZE),1)
DEFS+=-DNMEMSIZE
else
ifeq ($(UNAME),$(filter $(UNAME),Linux Darwin FreeBSD GNU/kFreeBSD))
LDFLAGS += -ldl
endif
endif

# disable SIMD compressors (LZSSE)
ifeq ($(NSIMD),1)
DEFS+=-DNSIMD
else
NSIMD=0
endif

# blosc
ifeq ($(BLOSC),1)
DEFS+=-DBLOSC
endif

# GLZA
ifeq ($(GLZA),1)
DEFS+=-DGLZA
endif

# Lzfse
ifeq ($(APPLE),1)
DEFS+=-DAPPLE
endif

# No c++ codec
NCPP=0

# configure or copy directory "snappy_/*" to "snappy"
ifneq (,$(wildcard snappy/snappy-stubs-public.h))
SNAPPY=1
DEFS+=-DSNAPPY
endif

ifeq ($(LZTURBO),1)
DEFS+=-DLZTURBO
endif

#------------- 
DDEBUG=-DNDEBUG -s

CFLAGS+=$(DDEBUG) -w -std=gnu99 -fpermissive -Wall -Izstd/lib -Izstd/lib/common -D_7ZIP_ST $(DEFS) -Ilibdeflate -Ilibdeflate/common -Ilz4/lib -Ilz5/lib -Ibrotli/include  
CXXFLAGS+=$(DDEBUG) -w -fpermissive -Wall -fno-rtti -Ilzham_codec_devel/include -Ilzham_codec_devel/lzhamcomp -Ilzham_codec_devel/lzhamdecomp -D"UINT64_MAX=-1ull" -ICSC/src/libcsc -D_7Z_TYPES_ -Ibrotli/include -DLIBBSC_SORT_TRANSFORM_SUPPORT $(DEFS)

all:  turbobench

ifeq ($(LZTURBO),1)
include ../lzturbo.mk
endif

OB+=plugins.o 
#----------------------- COMP1 -----------------------------------------
ifeq ($(NCOMP1), 0)
OB+=lz4/lib/lz4hc.o lz4/lib/lz4.o  
OB+=LZMA-SDK/C/LzFind.o LZMA-SDK/C/LzmaDec.o LZMA-SDK/C/LzmaEnc.o LZMA-SDK/C/LzmaLib.o LZMA-SDK/C/Alloc.o 
OB+=zstd/lib/common/xxhash.o zstd/lib/compress/zstd_compress.o zstd/lib/decompress/zstd_decompress.o zstd/lib/compress/fse_compress.o zstd/lib/common/fse_decompress.o zstd/lib/compress/huf_compress.o zstd/lib/decompress/huf_decompress.o zstd/lib/common/zstd_common.o zstd/lib/common/entropy_common.o

OB+=brotli_/enc/backward_references.o brotli/enc/bit_cost.o brotli/enc/cluster.o brotli/enc/block_splitter.o brotli/enc/encode.o brotli/enc/entropy_encode.o brotli/enc/compress_fragment.o brotli/enc/compress_fragment_two_pass.o brotli/enc/histogram.o \
	brotli/enc/literal_cost.o brotli/enc/brotli_bit_stream.o brotli/enc/memory.o brotli/enc/metablock.o brotli_/enc/static_dict.o brotli/enc/streams.o brotli/dec/bit_reader.o brotli/dec/decode.o brotli/common/dictionary.o \
	brotli/dec/huffman.o brotli/dec/state.o brotli/enc/utf8_util.o

ifeq ($(HAVE_ZLIB), 1)
ifeq ($(STATIC),1)
OB+=/usr/lib/x86_64-linux-gnu/libz.a
else
ifeq ($(ZLIB_NG), 1)
OB+=zlib-ng/libz.a
else
OB+=-lz
endif
endif
else
ifeq ($(ZLIB_NG), 1)
ZD=zlib-ng/
#type "./configure" in zlib_ng directory to create zconf.h
OB+=$(ZD)deflate_fast.o $(ZD)deflate_slow.o $(ZD)match.o
else
ifeq ($(ZLIB_INTEL), 1)
ZD=zlib_intel/
OB+=$(ZD)match.o
else
ZD=zlib/
endif
endif

OB+=$(ZD)adler32.o $(ZD)crc32.o $(ZD)compress.o $(ZD)deflate.o $(ZD)infback.o $(ZD)inffast.o $(ZD)inflate.o $(ZD)inftrees.o $(ZD)trees.o $(ZD)uncompr.o $(ZD)zutil.o
endif
endif

#----------------------- COMP2 -----------------------
xpack/lib/xpack_common.o: xpack/lib/xpack_common.c
	$(CC) -O3 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@ 

xpack/lib/xpack_compress.o: xpack/lib/xpack_compress.c
	$(CC) -O3 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@ 

xpack/lib/xpack_decompress.o: xpack/lib/xpack_decompress.c
	$(CC) -O3 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@ 

xpack/lib/x86_cpu_features.o: xpack/lib/x86_cpu_features.c
	$(CC) -O3 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@ 

glza/GLZAmodel.o: glza/GLZAmodel.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@ 

glza/GLZAcomp.o: glza/GLZAcomp.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@ 

glza/GLZAencode.o: glza/GLZAencode.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@ 

glza/GLZAcompress.o: glza/GLZAcompress.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@ 

glza/GLZAformat.o: glza/GLZAformat.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@ 

glza/GLZAdecode.o: glza/GLZAdecode.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@ 

pithy/pithy.o: pithy/pithy.c
	$(CC) -O2 $(MARCH) $(CFLAGS)  $< -c -o $@  

shrinker/shrinker.o: shrinker/shrinker.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@  

wflz/wfLZ.o: wflz/wfLZ.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@ 

nakamichi/Nakamichi_Nin.o: nakamichi/Nakamichi_Nin.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@ 

LZSSE/lzsse2/lzsse2.o: LZSSE/lzsse2/lzsse2.cpp
	$(CC) -O3 -msse4.1 -std=c++0x $(MARCH) $< -c -o $@ 

LZSSE/lzsse4/lzsse4.o: LZSSE/lzsse4/lzsse4.cpp
	$(CC) -O3 -msse4.1 -std=c++0x $(MARCH) $< -c -o $@ 

LZSSE/lzsse8/lzsse8.o: LZSSE/lzsse8/lzsse8.cpp
	$(CC) -O3 -msse4.1 -std=c++0x $(MARCH) $< -c -o $@ 

#WKDM=wkdm/WKdmCompress.o wkdm/WKdmDecompress.o
ifeq ($(NCOMP2), 0)
DIVSUFSORT=libbsc/libbsc/bwt/divsufsort/divsufsort.o
OB+=balz/balz.o
OB+=bcm_/bcm.o
OB+=brieflz/brieflz.o brieflz/depack.o
OB+=libbsc/libbsc/libbsc/libbsc.o libbsc/libbsc/coder/coder.o libbsc/libbsc/coder/qlfc/qlfc.o libbsc/libbsc/coder/qlfc/qlfc_model.o libbsc/libbsc/platform/platform.o libbsc/libbsc/filters/detectors.o \
	libbsc/libbsc/filters/preprocessing.o libbsc/libbsc/adler32/adler32.o libbsc/libbsc/bwt/bwt.o $(DIVSUFSORT) libbsc/libbsc/st/st.o libbsc/libbsc/lzp/lzp.o
OB+=libdeflate/lib/adler32.o libdeflate/lib/aligned_malloc.o libdeflate/lib/crc32.o libdeflate/lib/x86_cpu_features.o \
    libdeflate/lib/deflate_compress.o libdeflate/lib/deflate_decompress.o libdeflate/lib/gzip_compress.o libdeflate/lib/gzip_decompress.o libdeflate/lib/zlib_compress.o libdeflate/lib/zlib_decompress.o 
OB+=bzip2/blocksort.o bzip2/huffman.o bzip2/crctable.o bzip2/randtable.o bzip2/compress.o bzip2/decompress.o bzip2/bzlib.o
OB+=density/src/buffers/buffer.o density/src/core/algorithms.o density/src/structure/header.o density/src/globals.o density/src/streams/stream.o \
	density/src/core/chameleon/chameleon_decode.o density/src/core/chameleon/chameleon_dictionary.o density/src/core/chameleon/chameleon_encode.o \
	density/src/core/lion/lion_decode.o density/src/core/lion/lion_dictionary.o density/src/core/lion/lion_encode.o density/src/core/lion/lion_form_model.o \
	density/src/core/cheetah/cheetah_decode.o density/src/core/cheetah/cheetah_dictionary.o density/src/core/cheetah/cheetah_encode.o 
OB+=FastLZ/fastlz.o 
ifeq ($(GLZA),1)
OB+=glza/GLZAcomp.o glza/GLZAformat.o glza/GLZAcompress.o glza/GLZAencode.o glza/GLZAdecode.o glza/GLZAmodel.o 
endif
OB+=heatshrink_/heatshrink.o heatshrink/heatshrink_encoder.o heatshrink/heatshrink_decoder.o
#OB+=kraken/kraken.o 
OB+=chameleon/chameleon.o
OB+=crush/crush.o
OB+=liblzf/lzf_c.o liblzf/lzf_c_best.o liblzf/lzf_d.o 
OB+=liblzg/src/lib/encode.o liblzg/src/lib/decode.o liblzg/src/lib/checksum.o 
OB+=libslz/src/slz.o 
OB+=lz5/lib/lz5.o lz5/lib/lz5hc.o
OB+=lzfse/src/lzfse_decode_base.o lzfse/src/lzfse_decode.o lzfse/src/lzfse_encode_base.o lzfse/src/lzfse_encode.o lzfse/src/lzfse_fse.o lzfse/src/lzvn_decode_base.o lzfse/src/lzvn_encode_base.o
OB+=lzham_codec_devel/lzhamcomp/lzham_lzbase.o lzham_codec_devel/lzhamcomp/lzham_lzcomp.o lzham_codec_devel/lzhamcomp/lzham_lzcomp_internal.o \
	lzham_codec_devel/lzhamcomp/lzham_lzcomp_state.o lzham_codec_devel/lzhamcomp/lzham_match_accel.o lzham_codec_devel/lzhamcomp/lzham_pthreads_threading.o \
	lzham_codec_devel/lzhamdecomp/lzham_assert.o lzham_codec_devel/lzhamdecomp/lzham_checksum.o lzham_codec_devel/lzhamdecomp/lzham_huffman_codes.o \
	lzham_codec_devel/lzhamdecomp/lzham_lzdecomp.o lzham_codec_devel/lzhamdecomp/lzham_lzdecompbase.o lzham_codec_devel/lzhamdecomp/lzham_mem.o \
	lzham_codec_devel/lzhamdecomp/lzham_platform.o lzham_codec_devel/lzhamdecomp/lzham_prefix_coding.o \
	lzham_codec_devel/lzhamdecomp/lzham_symbol_codec.o lzham_codec_devel/lzhamdecomp/lzham_timer.o lzham_codec_devel/lzhamdecomp/lzham_vector.o \
	lzham_codec_devel/lzhamlib/lzham_lib.o 
ifeq ($(UNAME), Windows)
OB+=lzham_codec_devel/lzhamcomp/lzham_win32_threading.o
endif

ifeq ($(NSIMD),0)
OB+=LZSSE/lzsse2/lzsse2.o LZSSE/lzsse4/lzsse4.o LZSSE/lzsse8/lzsse8.o 
endif
OB+=miniz/miniz.o
OB+=nakamichi/Nakamichi_Kintaro.o
OB+=pithy/pithy.o
OB+=shoco/shoco.o
OB+=shrinker/Shrinker.o
OB+=wflz/wfLZ.o 
OB+=xpack/lib/xpack_common.o xpack/lib/xpack_compress.o xpack/lib/xpack_decompress.o xpack/lib/x86_cpu_features.o
OB+=yappy/yappy.o 
ifeq ($(NCPP), 0)
OB+=CSC/src/libcsc/csc_analyzer.o CSC/src/libcsc/csc_coder.o CSC/src/libcsc/csc_dec.o CSC/src/libcsc/csc_default_alloc.o CSC/src/libcsc/csc_enc.o CSC/src/libcsc/csc_encoder_main.o CSC/src/libcsc/csc_filters.o CSC/src/libcsc/csc_lz.o CSC/src/libcsc/csc_memio.o \
	CSC/src/libcsc/csc_mf.o CSC/src/libcsc/csc_model.o CSC/src/libcsc/csc_profiler.o 
OB+=doboz/Source/Doboz/Compressor.o doboz/Source/Doboz/Decompressor.o doboz/Source/Doboz/Dictionary.o
OB+=libzling/src/libzling.o libzling/src/libzling_huffman.o libzling/src/libzling_utils.o libzling/src/libzling_lz.o libzling_/libzling_utils_mem.o 
OB+=zpaq/libzpaq.o
ifeq ($(SNAPPY), 1)
OB+=snappy/snappy-sinksource.o snappy/snappy-stubs-internal.o snappy/snappy.o
endif
OB+=snappy-c/snappy.o snappy-c/util.o
OB+=zopfli/src/zopfli/blocksplitter.o zopfli/src/zopfli/cache.o zopfli/src/zopfli/deflate.o zopfli/src/zopfli/gzip_container.o zopfli/src/zopfli/hash.o zopfli/src/zopfli/util.o zopfli/src/zopfli/lz77.o zopfli/src/zopfli/tree.o zopfli/src/zopfli/squeeze.o zopfli/src/zopfli/katajainen.o zopfli/src/zopfli/zlib_container.o zopfli/src/zopfli/zopfli_lib.o

ifeq ($(UNAME), Linux)
OB+=gipfeli/lz77.o gipfeli/entropy.o gipfeli/entropy_code_builder.o gipfeli/decompress.o gipfeli/gipfeli-internal.o
ifeq ($(BLOSC), 1)
OB+=c-blosc2/blosc/blosc.o c-blosc2/blosc/blosclz.o c-blosc2/blosc/schunk.o c-blosc2/blosc/delta.o c-blosc2/blosc/shuffle.o c-blosc2/blosc/shuffle-generic.o c-blosc2/blosc/shuffle-sse2.o c-blosc2/blosc/bitshuffle-generic.o
endif
endif
endif

endif
#----------------------- Apple -----------------------
ifeq ($(APPLE), 1)
LZFSE=
endif
#----------------------- GPL -------------------------
lzmat/lzmat_dec.o: lzmat/lzmat_dec.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@
lzmat/lzmat_enc.o: lzmat/lzmat_enc.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@

LZO = lzo/src/lzo1.o lzo/src/lzo1a.o lzo/src/lzo1a_99.o lzo/src/lzo1b_1.o lzo/src/lzo1b_2.o lzo/src/lzo1b_3.o lzo/src/lzo1b_4.o lzo/src/lzo1b_5.o \
lzo/src/lzo1b_6.o lzo/src/lzo1b_7.o lzo/src/lzo1b_8.o lzo/src/lzo1b_9.o lzo/src/lzo1b_99.o lzo/src/lzo1b_9x.o lzo/src/lzo1b_cc.o \
lzo/src/lzo1b_d1.o lzo/src/lzo1b_d2.o lzo/src/lzo1b_rr.o lzo/src/lzo1b_xx.o lzo/src/lzo1c_1.o lzo/src/lzo1c_2.o lzo/src/lzo1c_3.o \
lzo/src/lzo1c_4.o lzo/src/lzo1c_5.o lzo/src/lzo1c_6.o lzo/src/lzo1c_7.o lzo/src/lzo1c_8.o lzo/src/lzo1c_9.o lzo/src/lzo1c_99.o \
lzo/src/lzo1c_9x.o lzo/src/lzo1c_cc.o lzo/src/lzo1c_d1.o lzo/src/lzo1c_d2.o lzo/src/lzo1c_rr.o lzo/src/lzo1c_xx.o lzo/src/lzo1f_1.o \
lzo/src/lzo1f_9x.o lzo/src/lzo1f_d1.o lzo/src/lzo1f_d2.o lzo/src/lzo1x_1.o lzo/src/lzo1x_1k.o lzo/src/lzo1x_1l.o lzo/src/lzo1x_1o.o \
lzo/src/lzo1x_9x.o lzo/src/lzo1x_d1.o lzo/src/lzo1x_d2.o lzo/src/lzo1x_d3.o lzo/src/lzo1x_o.o lzo/src/lzo1y_1.o lzo/src/lzo1y_9x.o \
lzo/src/lzo1y_d1.o lzo/src/lzo1y_d2.o lzo/src/lzo1y_d3.o lzo/src/lzo1y_o.o lzo/src/lzo1z_9x.o lzo/src/lzo1z_d1.o lzo/src/lzo1z_d2.o \
lzo/src/lzo1z_d3.o lzo/src/lzo1_99.o lzo/src/lzo2a_9x.o lzo/src/lzo2a_d1.o lzo/src/lzo2a_d2.o lzo/src/lzo_crc.o lzo/src/lzo_init.o \
lzo/src/lzo_ptr.o lzo/src/lzo_str.o lzo/src/lzo_util.o 

tornado_/tormem.o: tornado_/tormem.cpp
	$(CXX) -O3 $(TORDEF) -D__x86_$(ARCH)__ -DFREEARC_$(ARCH)BIT -pipe -fforce-addr -fno-exceptions -fno-rtti -c tornado_/tormem.cpp -o tornado_/tormem.o

ifeq ($(UNAME), Linux)
TORDEF=-DFREEARC_UNIX -DFREEARC_INTEL_BYTE_ORDER
else
TORDEF=-DFREEARC_WIN -DFREEARC_INTEL_BYTE_ORDER -D_UNICODE -DUNICODE 
endif

ifeq ($(GPL), 1)
OB+=$(LZMAT) $(LZO) $(ECGPL) $(MSCOMPRESS)
OB+=lzlib-1.8/lzlib.o lzlib_/bbexample.o 
OB+=lzmat/lzmat_enc.o lzmat/lzmat_dec.o
OB+=lzoma_/pack.o lzoma_/unpack.o lzoma_/divsufsort.o
OB+=ms-compress/src/mscomp.o ms-compress/src/lznt1_compress.o ms-compress/src/lznt1_decompress.o ms-compress/src/xpress_compress.o ms-compress/src/xpress_decompress.o ms-compress/src/xpress_huff_compress.o ms-compress/src/xpress_huff_decompress.o
OB+=quicklz_/quicklz1.o quicklz_/quicklz2.o quicklz_/quicklz3.o 
OB+=pysap/pysapcompress/vpa105CsObjInt.o pysap/pysapcompress/vpa106cslzc.o pysap/pysapcompress/vpa107cslzh.o pysap/pysapcompress/vpa108csulzh.o
ifeq ($(NCPP), 0)
OB+=tornado_/tormem.o
endif
endif
#-------------------- Encoding ------------------------
ifeq ($(NENCOD),0)
OB+=TurboRLE/trlec.o TurboRLE/trled.o
endif
#-------------------- Entropy Coder -------------------
ifeq ($(NECODER), 0)
OB+=FastARI/FastAri.o 
OB+=rans_static/rANS_static4x8.o rans_static/rANS_static4x16.o rans_static/rANS_static.o rans_static_/arith_static.o
OB+=zlibh/zlibh.o
OB+=subotin_/subotin.o 
OB+=fqz0/f_o0.o
OB+=ppmdec/ppmdec.o
OB+=ans_nania/narans.o 
OB+=fpaq0p/fpaq0p_sh.o 
OB+=vecrc/vector_rc.o

#ifeq ($(NCOMP1), 0)
#OB+=FiniteStateEntropy/lib/fse.o FiniteStateEntropy/lib/huff0.o
#endif

ifeq ($(UNAME), Linux)
OB+=fsc/fsc_enc.o fsc/fsc_dec.o fsc/fsc_utils.o fsc/bits.o fsc/histo.o fsc/alias.o 
endif

ifeq ($(NCPP), 0)
OB+=fastac/arithmetic_codec.o
OB+=fasthf/binary_codec.o
endif

endif

ifeq ($(GPL), 1)
OB+=polar/polar.o fpaqc/fpaqc.o
endif
#--------------------------------------------------------------------
turbobench: $(OB) turbobench.o  
	$(CXX) $^ $(LDFLAGS) -o turbobench

.c.o:
	$(CC) -O3 $(MARCH) $(CFLAGS) $< -c -o $@  

.cc.o:
	$(CXX) -O3 $(MARCH) $(CXXFLAGS)  $< -c -o $@ 

.cpp.o:
	$(CXX) -O3 $(MARCH) $(CXXFLAGS) $< -c -o $@ 

clean:
	find . -name "turbobench" -type f -delete
	find . -name "*.o" -type f -delete
	find . -name "*~" -type f -delete
	find . -name "core" -type f -delete

cleanw:
	del /S *.o 
	del /S *~
	del /S *.exe

