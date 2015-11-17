# powturbo  (c) Copyright 2013-2015
# Linux: "export CC=clang" windows mingw: "set CC=gcc" or uncomment one of following lines
#CC=clang
#CXX=clang++
CC=gcc
CXX=g++

COMPRESS1=1
COMPRESS2=1
ECODEC=1
GPL=0
#PLUGIN2=0

CPP=1

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
LDFLAGS=-lpthread -lm -static
HAVE_ZLIB=1
else
LDFLAGS=-static 
#-libgcc
endif

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
ARCH=64
else
ARCH=64
#CFLAGS=-fomit-frame-pointer
endif

DDEBUG=-DNDEBUG -s
CFLAGS+=-w -fpermissive -Wall $(DDEBUG) -Ilz4/lib -Ilz5/lib -D_7ZIP_ST -Ilzo/include -DZSTD_LEGACY_SUPPORT=0 
CXXFLAGS=-w -fpermissive -Wall $(DDEBUG) -std=c++11 -fno-rtti -Ilzham_codec_devel/include -Ilzham_codec_devel/lzhamcomp -Ilzham_codec_devel/lzhamdecomp -Ilzo/include -ICSC/src/libcsc -D_7Z_TYPES_ -DLIBBSC_SORT_TRANSFORM_SUPPORT -DSHUFFLE_SSE2_ENABLED -DHAVE_CPU_FEAT_INTRIN

all:  turbobench

ifeq ($(PLUGIN2),1)
include ../plugin2.mk
endif

tornado_/tormem.o: tornado_/tormem.cpp
	$(CXX) -O3 $(TORDEF) -D__x86_$(ARCH)__ -DFREEARC_$(ARCH)BIT -pipe -fforce-addr -fno-exceptions -fno-rtti -c tornado_/tormem.cpp -o tornado_/tormem.o

BRIEFLZ=brieflz/brieflz.o brieflz/depack.o

BROTLI=brotli/enc/backward_references.o brotli/enc/block_splitter.o brotli/enc/encode.o brotli/enc/entropy_encode.o brotli/enc/histogram.o brotli/enc/literal_cost.o brotli/enc/brotli_bit_stream.o brotli/enc/metablock.o \
	brotli/enc/static_dict.o brotli/enc/streams.o brotli/dec/bit_reader.o brotli/dec/decode.o brotli/dec/dictionary.o brotli/dec/huffman.o brotli/dec/state.o brotli/dec/streams.o brotli/enc/utf8_util.o

BZIP2=bzip2/blocksort.o bzip2/huffman.o bzip2/crctable.o bzip2/randtable.o bzip2/compress.o bzip2/decompress.o bzip2/bzlib.o

C_BLOSC2=c-blosc2/blosc/blosc.o c-blosc2/blosc/blosclz.o c-blosc2/blosc/shuffle.o c-blosc2/blosc/shuffle-generic.o c-blosc2/blosc/shuffle-sse2.o c-blosc2/blosc/bitshuffle-generic.o

DENSITY=density/src/buffers/buffer.o density/src/core/algorithms.o density/src/structure/header.o density/src/globals.o density/src/streams/stream.o \
density/src/core/chameleon/chameleon_decode.o density/src/core/chameleon/chameleon_dictionary.o density/src/core/chameleon/chameleon_encode.o \
density/src/core/lion/lion_decode.o density/src/core/lion/lion_dictionary.o density/src/core/lion/lion_encode.o density/src/core/lion/lion_form_model.o \
density/src/core/cheetah/cheetah_decode.o density/src/core/cheetah/cheetah_dictionary.o density/src/core/cheetah/cheetah_encode.o 

DOBOZ=doboz/Source/Doboz/Compressor.o doboz/Source/Doboz/Decompressor.o doboz/Source/Doboz/Dictionary.o

FASTLZ=FastLZ/fastlz.o 

HEATSHRINK=heatshrink_/heatshrink.o heatshrink/heatshrink_encoder.o heatshrink/heatshrink_decoder.o

LIBBSC=libbsc/libbsc/libbsc/libbsc.o libbsc/libbsc/coder/coder.o libbsc/libbsc/coder/qlfc/qlfc.o libbsc/libbsc/coder/qlfc/qlfc_model.o libbsc/libbsc/platform/platform.o \
	libbsc/libbsc/filters/detectors.o libbsc/libbsc/filters/preprocessing.o libbsc/libbsc/adler32/adler32.o libbsc/libbsc/bwt/bwt.o libbsc/libbsc/bwt/divsufsort/divsufsort.o libbsc/libbsc/st/st.o libbsc/libbsc/lzp/lzp.o

LIBCSC=CSC/src/libcsc/csc_analyzer.o CSC/src/libcsc/csc_coder.o CSC/src/libcsc/csc_dec.o CSC/src/libcsc/csc_enc.o CSC/src/libcsc/csc_encoder_main.o CSC/src/libcsc/csc_filters.o \
CSC/src/libcsc/csc_lz.o CSC/src/libcsc/csc_memio.o CSC/src/libcsc/csc_mf.o CSC/src/libcsc/csc_model.o CSC/src/libcsc/csc_profiler.o 

GIPFELI=gipfeli/lz77.o gipfeli/entropy.o gipfeli/entropy_code_builder.o gipfeli/decompress.o gipfeli/gipfeli-internal.o
	
LIBLZG=liblzg/src/lib/encode.o liblzg/src/lib/decode.o liblzg/src/lib/checksum.o 

LIBLZF=liblzf/lzf_c.o liblzf/lzf_c_best.o liblzf/lzf_d.o 

LIBZLING=libzling/src/libzling.o libzling/src/libzling_huffman.o libzling/src/libzling_utils.o libzling/src/libzling_lz.o libzling_/libzling_utils_mem.o 

LZ4=lz4/lib/lz4.o lz4/lib/lz4hc.o 

LZ5=lz5/lib/lz5.o lz5/lib/lz5hc.o 

LZHAM = lzham_codec_devel/lzhamcomp/lzham_lzbase.o lzham_codec_devel/lzhamcomp/lzham_lzcomp.o lzham_codec_devel/lzhamcomp/lzham_lzcomp_internal.o \
lzham_codec_devel/lzhamcomp/lzham_lzcomp_state.o lzham_codec_devel/lzhamcomp/lzham_match_accel.o lzham_codec_devel/lzhamcomp/lzham_pthreads_threading.o \
lzham_codec_devel/lzhamdecomp/lzham_assert.o lzham_codec_devel/lzhamdecomp/lzham_checksum.o lzham_codec_devel/lzhamdecomp/lzham_huffman_codes.o \
lzham_codec_devel/lzhamdecomp/lzham_lzdecomp.o lzham_codec_devel/lzhamdecomp/lzham_lzdecompbase.o lzham_codec_devel_/lzhamdecomp/lzham_mem.o \
lzham_codec_devel/lzhamdecomp/lzham_platform.o lzham_codec_devel/lzhamdecomp/lzham_prefix_coding.o \
lzham_codec_devel/lzhamdecomp/lzham_symbol_codec.o lzham_codec_devel/lzhamdecomp/lzham_timer.o lzham_codec_devel/lzhamdecomp/lzham_vector.o \
lzham_codec_devel/lzhamlib/lzham_lib.o 
ifneq ($(UNAME), Linux)
LZHAM+=lzham_codec_devel/lzhamcomp/lzham_win32_threading.o
endif

LZMA=LZMA-SDK/C/LzFind.o LZMA-SDK/C/LzmaDec.o LZMA-SDK/C/LzmaEnc.o LZMA-SDK/C/LzmaLib.o LZMA-SDK/C/Alloc.o 

LZMAT=lzmat/lzmat_enc.o lzmat/lzmat_dec.o

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
#LZO = minilzo/minilzo.o

MSCOMPRESS=ms-compress/src/mscomp.o ms-compress/src/lznt1_compress.o ms-compress/src/lznt1_decompress.o ms-compress/src/xpress_compress.o ms-compress/src/xpress_decompress.o ms-compress/src/xpress_huff_compress.o ms-compress/src/xpress_huff_decompress.o

PITHY=pithy/pithy.o

LZLIB=lzlib/lzlib.o lzlib_/bbexample.o 

QUICKLZ=quicklz_/quicklz1.o quicklz_/quicklz2.o quicklz_/quicklz3.o 

SHRINKER=shrinker/Shrinker.o

SNAPPY=snappy/snappy-sinksource.o snappy/snappy-stubs-internal.o snappy/snappy.o

SNAPPY_C=snappy-c/snappy.o snappy-c/util.o

TORNADO=tornado_/tormem.o 	
ifeq ($(UNAME), Linux)
TORDEF=-DFREEARC_UNIX -DFREEARC_INTEL_BYTE_ORDER
else
TORDEF=-DFREEARC_WIN -DFREEARC_INTEL_BYTE_ORDER -D_UNICODE -DUNICODE 
endif

WFLZ=wflz/wfLZ.o

YAPPY=yappy/yappy.o 

ifeq ($(HAVE_ZLIB), 1)
#ZLIB = -lz
ZLIB=/usr/lib/x86_64-linux-gnu/libz.a
else
ZLIBDIR=zlib/
#ZLIBDIR=zlib-ng/
#ZLIBDIR=zlib_intel/
ZLIB=$(ZLIBDIR)compress.o $(ZLIBDIR)deflate.o $(ZLIBDIR)infback.o $(ZLIBDIR)inffast.o $(ZLIBDIR)inflate.o $(ZLIBDIR)inftrees.o $(ZLIBDIR)trees.o $(ZLIBDIR)adler32.o $(ZLIBDIR)crc32.o $(ZLIBDIR)uncompr.o $(ZLIBDIR)zutil.o
#ZLIB+=$(ZLIBDIR)match.o
endif

ZSTD=zstd/lib/zstd.o zstd/lib/fse.o zstd/lib/huff0.o zstd/lib/zstdhc.o

Z=zopfli/src/zopfli/
ZOPFLI=$(Z)blocksplitter.o $(Z)cache.o $(Z)deflate.o $(Z)gzip_container.o $(Z)hash.o $(Z)util.o $(Z)lz77.o $(Z)tree.o $(Z)squeeze.o $(Z)katajainen.o $(Z)zlib_container.o $(Z)zopfli_lib.o
#-------------------------------------------------------------
OBJB+=plugins.o 

ifeq ($(COMPRESS1), 1)
OBJB+=$(LZ4) $(LZMA) $(ZLIB) $(ZSTD)
ifeq ($(CPP), 1)
OBJB+=$(BROTLI) 
endif
endif

ifeq ($(COMPRESS2), 1)
OBJB+= $(BRIEFLZ) $(BZIP2) $(DENSITY) $(FASTLZ) $(HEATSHRINK) $(LIBBSC) $(LIBLZF) $(LZ5) $(LIBLZG) $(SHRINKER) $(PITHY) $(LZSS) $(SNAPPY_C) $(MINIZ) $(YAPPY) $(WFLZ) $(WIMLIB) $(ZOPFLI)
OBJB+=chameleon/chameleon.o
OBJB+=crush/crush.o
ifeq ($(CPP), 1)
OBJB+=$(LIBZLING) $(DOBOZ) $(LIBCSC) $(LZHAM) $(SNAPPY) 
OBJB+=balz/balz.o
OBJB+=bcm/bcm.o
ifeq ($(UNAME), Linux)
OBJB+=$(C_BLOSC2) $(GIPFELI)
endif
endif	   
endif

ifeq ($(GPL), 1)
OBJB+=$(LZMAT) $(LZO) $(QUICKLZ) $(ECGPL) $(LZLIB) $(MSCOMPRESS) 
ifeq ($(CPP), 1)
OBJB+=$(TORNADO)
endif
endif

ifeq ($(ECODEC), 1)
OBJB+=FastARI/FastAri.o ans_jb/rANS_static4c.o ans_jb/arith_static.o zlibh/zlibh.o subotin_/subotin.o  

ifeq ($(COMPRESS1), 0)
OBJB+=FiniteStateEntropy/lib/fse.o FiniteStateEntropy/lib/huff0.o
endif

ifeq ($(UNAME), Linux)
OBJB+=fsc/fsc_enc.o fsc/fsc_dec.o fsc/fsc_utils.o fsc/bits.o fsc/histo.o fsc/alias.o 
endif

ifeq ($(CPP), 1)
OBJB+=fastac/arithmetic_codec.o
OBJB+=fasthf/binary_codec.o
endif

ifeq ($(ARCH),64)
OBJB+=ans_jb/rANS_static64c.o
endif
endif

ifeq ($(GPL), 1)
OBJB+=polar/polar.o
endif

turbobench.o: plugreg.c plugins.h
plugins.o: plugins.h

turbobench: $(OBJB) turbobench.o  
	$(CXX) $^ $(LDFLAGS) -o turbobench

.c.o:
	$(CC) -O3 $(MARCH) $(CFLAGS) $< -c -o $@  

.cc.o:
	$(CXX) -O3 $(MARCH) $(CXXFLAGS) -DNDEBUG -std=c++11 $< -c -o $@ 

.cpp.o:
	$(CXX) -O3 $(MARCH) $(CXXFLAGS) -DNDEBUG -std=c++11 $< -c -o $@ 

clean:
	find . -name "turbobench" -type f -delete
	find . -name "*.o" -type f -delete
	find . -name "*~" -type f -delete
	find . -name "core" -type f -delete

cleanw:
	del /S *.o 
	del /S *~
	del /S *.exe

# cop< snappy_ to snappy 
