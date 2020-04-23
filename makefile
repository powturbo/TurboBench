# powturbo  (c) Copyright 2013-2020
# ----------- Downloading + Compiling ----------------------
# git clone --recursive git://github.com/powturbo/TurboBench.git 
# make
#
# Minimum make: "make NCODEC2=1" to compile only brotli,lz4,lzma,zlib and zstd
#
# snappy:    "cp snappy_/* snappy" (or configure snappy) & type make
ifeq ($(NCODEC1),1) # Popular codecs
else
BROTLI=1
LZ4=1
LZMA=1
ZLIB=1
ZSTD=1
endif

ifeq ($(NCODEC2),1) # Notable codecs
else
#CSC=1
#BRIEFLZ=1
BZIP2=1
#CHAMELEON=1
#DENSITY=1
FASTLZ=1
FLZMA2=1
LIBDEFLATE=1
#GIPFELI=1
GLZA=1
#HEATSHRINK=1
#IGZIP=1
LIBBSC=1
#LIBZLING=1
LIZARD=1
LZFSE=1
LZHAM=1
LZLIB=1
LZO=1
LZSA=1
LZSSE=1
#MINIZ=1
#MSCOMPRESS=1
PYSAP=1
QUICKLZ=1
#SCHOCO=1
SNAPPY=1
#SNAPPY_C=1
TURBORC=1
TURBORLE=1
#ZLIB_NG=1
ZOPFLI=1
ZPAQ=1
endif

ifeq ($(EC),1) # Encoding Entropy coders / RLE 
#AOM=1
FASTAC=1
FASTHF=1
FASTARI=1
FPAQ0P=1
FPAQC=1
#FREQTAB=1
FPC=1
FQZ0=1
RANS_S=1
#RECIPARITH=1
SUBOTIN=1
VECRC=1
endif

ifeq ($(TR),1) # Transform 
#BRC=1
endif
# Archived codecs and other codecs (manual download)
#BLOSC=1
#DAALA=1
#DOBOZ=1
#LZMAT=1
#NAKAMICHI=1
#PITHY=1
#POLAR
#PPMDEC
#LZ4ULTRA=1
#LZOMA=1
#SMALLZ4=1
#SLZ=1
#SHRINKER=1
#TORNADO=1
#WFLZ=1
#XPACK=1
#YALZ77
#XPACK=1
#------------------------------------------------------------------------------------------------------------
CC ?= gcc
CXX ?= g++
#CC=clang
#CXX=clang++
#CC=icc
#CXX=icc

#CC=powerpc64le-linux-gnu-gcc
#CL = $(CC)

#DEBUG=-DDEBUG -g
#DDEBUG=-g
DDEBUG=-DNDEBUG -s

#------- OS/ARCH -------------------
ifneq (,$(filter Windows%,$(OS)))
  OS := Windows
  CC=gcc
  CXX=g++
  ARCH=x86_64
else
  OS := $(shell uname -s)
  ARCH := $(shell uname -m)

ifneq (,$(findstring aarch64,$(CC)))
  ARCH = aarch64
else ifneq (,$(findstring powerpc64le,$(CC)))
  ARCH = ppc64le
endif
endif

ifeq ($(ARCH),ppc64le)
#  SSE=-D__SSSE3__
  CFLAGS=-mcpu=power9 -mtune=power9 $(SSE)
else ifeq ($(ARCH),aarch64)
  CFLAGS+=-march=armv8-a 
ifneq (,$(findstring clang, $(CC)))
  CFLAGS+=-march=armv8-a 
  OPT+=-fomit-frame-pointer
else
  CFLAGS+=-march=armv8-a 
endif
  SSE=-march=armv8-a
else ifeq ($(ARCH),$(filter $(ARCH),x86_64))
# set minimum arch sandy bridge SSE4.1 + AVX
  SSE=-march=corei7-avx -mtune=corei7-avx 
# SSE+=-mno-avx -mno-aes
  AVX2=-march=haswell
#  CFLAGS=$(SSE)
#  CFLAGS=$(AVX2)
else
SNAPPY_C=0
endif

CFLAGS+=-w -Wall $(DEBUG) $(OPT) 

ifeq ($(OS),$(filter $(OS),Linux GNU/kFreeBSD GNU OpenBSD FreeBSD DragonFly NetBSD MSYS_NT Haiku))
LDFLAGS+=-lrt -lpthread
endif

ifeq ($(STATIC),1)
LDFLAGS+=-static
NMEMSIZE=1
endif

# disable peak memory calculation
ifeq ($(OS),$(filter $(OS),Darwin))
NMEMSIZE=1
endif

ifeq ($(NMEMSIZE),1)
CFLAGS+=-DNMEMSIZE
else
ifeq ($(OS),$(filter $(OS),Darwin FreeBSD GNU/kFreeBSD Linux NetBSD SunOS))
LDFLAGS += -ldl
endif
endif

ifeq ($(OPENMP),1)
CFLAGS+=-fopenmp -DLIBBSC_OPENMP_SUPPORT
LDFLAGS+=-fopenmp 
endif

CFLAGS+=$(DDEBUG) -w -std=gnu99 -fpermissive -Wall $(INC)  
CXXFLAGS+=$(DDEBUG) -w -fpermissive -Wall -fno-rtti $(INC)
#CXXFLAGS+=-Imarlin/inc
#CXXFLAGS+=$(DDEBUG) -std=gnu++14 -Wall -Wextra -Wcast-qual -Wcast-align -Wstrict-aliasing -Wswitch-enum -Wundef -pedantic  -Wfatal-errors -Wshadow 

all:  turbobench

ifeq ($(LZTURBO),1)
CXXFLAGS+=-D_LZTURBO
CFLAGS+=-D_LZTURBO
include ../dev/x/lzturbo.mk
endif

#--------------------------------- Default codecs --------------------------------------------------------------------------------------------
ifeq ($(BROTLI), 1)
CXXFLAGS+=-D_BROTLI 
INC+=-Ibrotli/c/include -Ibrotli/c/enc 
ifeq ($(BROTLILIB), 1)  # Use the libraries created by the brotli own builds
OB+=brotli/out/libbrotlicommon.so brotli/out/libbrotlidec.so brotli/out/libbrotlienc.so
else
OB+=brotli/c/common/dictionary.o brotli/c/common/transform.o brotli/c/dec/huffman.o brotli/c/dec/state.o brotli/c/dec/bit_reader.o brotli/c/dec/decode.o\
    brotli/c/enc/backward_references.o brotli/c/enc/bit_cost.o brotli/c/enc/brotli_bit_stream.o brotli/c/enc/block_splitter.o brotli/c/enc/cluster.o brotli/c/enc/encode.o brotli/c/enc/compress_fragment.o brotli/c/enc/compress_fragment_two_pass.o \
    brotli/c/enc/encoder_dict.o brotli/c/enc/entropy_encode.o brotli/c/enc/histogram.o brotli/c/enc/literal_cost.o  brotli/c/enc/memory.o brotli/c/enc/metablock.o brotli/c/enc/utf8_util.o brotli/c/enc/backward_references_hq.o \
    brotli/c/enc/dictionary_hash.o brotli/c/enc/static_dict.o
endif
endif

DIVSUFSORT=libbsc/libbsc/bwt/divsufsort/divsufsort.o
ifeq ($(LIBBSC),1)
CXXFLAGS+=-D_LIBBSC -ICSC/src/libcsc -DLIBBSC_SORT_TRANSFORM_SUPPORT 
OB+=libbsc/libbsc/libbsc/libbsc.o libbsc/libbsc/coder/coder.o libbsc/libbsc/coder/qlfc/qlfc.o libbsc/libbsc/coder/qlfc/qlfc_model.o libbsc/libbsc/platform/platform.o libbsc/libbsc/filters/detectors.o \
	libbsc/libbsc/filters/preprocessing.o libbsc/libbsc/adler32/adler32.o libbsc/libbsc/bwt/bwt.o $(DIVSUFSORT) libbsc/libbsc/st/st.o libbsc/libbsc/lzp/lzp.o
endif

ifeq ($(BZIP2),1)
CXXFLAGS+=-D_BZIP2
OB+=bzip2/blocksort.o bzip2/huffman.o bzip2/crctable.o bzip2/randtable.o bzip2/compress.o bzip2/decompress.o bzip2/bzlib.o
endif

ifeq ($(LIBDEFLATE), 1)
ifneq ($(CC), icc)
CXXFLAGS+=-D_LIBDEFLATE 
INC+=-Ilibdeflate -Ilibdeflate/common
OB+=libdeflate/lib/adler32.o libdeflate/lib/crc32.o libdeflate/lib/arm/cpu_features.o libdeflate/lib/x86/cpu_features.o \
    libdeflate/lib/deflate_compress.o libdeflate/lib/deflate_decompress.o libdeflate/lib/gzip_compress.o libdeflate/lib/gzip_decompress.o libdeflate/lib/zlib_compress.o libdeflate/lib/zlib_decompress.o libdeflate/lib/utils.o
endif
endif

ifeq ($(LIZARD), 1)
CXXFLAGS+=-D_LIZARD
INC+=-Ilizard/lib
OB+=lizard/lib/lizard_compress.o lizard/lib/lizard_decompress.o
endif

ifeq ($(LZFSE), 1)
CXXFLAGS+=-D_LZFSE
OB+=lzfse/src/lzfse_decode_base.o lzfse/src/lzfse_decode.o lzfse/src/lzfse_encode_base.o lzfse/src/lzfse_encode.o lzfse/src/lzfse_fse.o lzfse/src/lzvn_decode_base.o lzfse/src/lzvn_encode_base.o
endif

ifeq ($(LZHAM), 1)
CXXFLAGS+=-D_LZHAM -Ilzham_codec_devel/include -Ilzham_codec_devel/lzhamcomp -Ilzham_codec_devel/lzhamdecomp -D"UINT64_MAX=-1ull" 
OB+=lzham_codec_devel/lzhamcomp/lzham_lzbase.o lzham_codec_devel/lzhamcomp/lzham_lzcomp.o lzham_codec_devel/lzhamcomp/lzham_lzcomp_internal.o \
	lzham_codec_devel/lzhamcomp/lzham_lzcomp_state.o lzham_codec_devel/lzhamcomp/lzham_match_accel.o lzham_codec_devel/lzhamcomp/lzham_pthreads_threading.o \
	lzham_codec_devel/lzhamdecomp/lzham_assert.o lzham_codec_devel/lzhamdecomp/lzham_checksum.o lzham_codec_devel/lzhamdecomp/lzham_huffman_codes.o \
	lzham_codec_devel/lzhamdecomp/lzham_lzdecomp.o lzham_codec_devel/lzhamdecomp/lzham_lzdecompbase.o lzham_codec_devel/lzhamdecomp/lzham_mem.o \
	lzham_codec_devel/lzhamdecomp/lzham_platform.o lzham_codec_devel/lzhamdecomp/lzham_prefix_coding.o \
	lzham_codec_devel/lzhamdecomp/lzham_symbol_codec.o lzham_codec_devel/lzhamdecomp/lzham_timer.o lzham_codec_devel/lzhamdecomp/lzham_vector.o \
	lzham_codec_devel/lzhamlib/lzham_lib.o 
ifeq ($(OS), Windows)
OB+=lzham_codec_devel/lzhamcomp/lzham_win32_threading.o
endif
endif

ifeq ($(LZ4), 1)
CXXFLAGS+=-D_LZ4
INC+=-Ilz4/lib
OB+=lz4/lib/lz4hc.o lz4/lib/lz4.o lz4/lib/lz4frame.o
endif

ifeq ($(LZMA), 1)
CXXFLAGS+=-D_LZMA
CFLAGS+=-D_7ZIP_ST
CXXFLAGS+=-D_7Z_TYPES_ 
OB+=lzma/C/Alloc.o lzma/C/LzFind.o lzma/C/LzmaDec.o lzma/C/LzmaEnc.o lzma/C/LzmaLib.o 
ifeq ($(OS),Windows_NT)
OB+=lzma/C/Threads.o lzma/C/LzFindMt.o 
endif
endif

ifeq ($(LZOMA), 1)
CXXFLAGS+=-D_LZOMA
OB+=lzoma_/pack.o lzoma_/unpack.o lzoma_/divsufsort.o 
endif

ifeq ($(LZSSE), 1) # SSE4.1 
ifeq ($(ARCH),$(filter $(ARCH),x86_))
CXXFLAGS+=-D_LZSSE
LZSSE/lzsse2/lzsse2.o: LZSSE/lzsse2/lzsse2.cpp
	$(CXX) -O2 -msse4.1 -std=c++11 $< -c -o $@ 

LZSSE/lzsse4/lzsse4.o: LZSSE/lzsse4/lzsse4.cpp
	$(CXX) -O2 -msse4.1 -std=c++11  $< -c -o $@ 

LZSSE/lzsse8/lzsse8.o: LZSSE/lzsse8/lzsse8.cpp
	$(CXX) -O2 -msse4.1 -std=c++11  $< -c -o $@

OB+=LZSSE/lzsse2/lzsse2.o LZSSE/lzsse4/lzsse4.o LZSSE/lzsse8/lzsse8.o 
endif
endif

ifeq ($(ZSTD), 1)
CXXFLAGS+=-D_ZSTD 
INC+=-Izstd/lib -Izstd/lib/common
OB+=zstd/lib/common/pool.o zstd/lib/common/xxhash.o zstd/lib/common/error_private.o \
    zstd/lib/compress/hist.o zstd/lib/compress/zstd_compress.o zstd/lib/compress/zstd_compress_literals.o zstd/lib/compress/zstd_compress_sequences.o zstd/lib/compress/zstd_double_fast.o zstd/lib/compress/zstd_fast.o zstd/lib/compress/zstd_lazy.o zstd/lib/compress/zstd_ldm.o zstd/lib/compress/zstdmt_compress.o zstd/lib/compress/zstd_opt.o \
    zstd/lib/decompress/zstd_decompress.o zstd/lib/decompress/zstd_decompress_block.o zstd/lib/decompress/zstd_ddict.o zstd/lib/compress/fse_compress.o zstd/lib/common/fse_decompress.o zstd/lib/compress/huf_compress.o zstd/lib/decompress/huf_decompress.o zstd/lib/common/zstd_common.o zstd/lib/common/entropy_common.o zstd/lib/compress/zstd_compress_superblock.o
endif

ifeq ($(ZLIB), 1)
CXXFLAGS+=-D_ZLIB
ZD=zlib/
OB+=$(ZD)adler32.o $(ZD)crc32.o $(ZD)compress.o $(ZD)deflate.o $(ZD)infback.o $(ZD)inffast.o $(ZD)inflate.o $(ZD)inftrees.o $(ZD)trees.o $(ZD)uncompr.o $(ZD)zutil.o
endif

ifeq ($(ZOPFLI), 1)
CXXFLAGS+=-D_ZOPFLI
OB+=zopfli/src/zopfli/blocksplitter.o zopfli/src/zopfli/cache.o zopfli/src/zopfli/deflate.o zopfli/src/zopfli/gzip_container.o zopfli/src/zopfli/hash.o zopfli/src/zopfli/util.o zopfli/src/zopfli/lz77.o zopfli/src/zopfli/tree.o zopfli/src/zopfli/squeeze.o zopfli/src/zopfli/katajainen.o zopfli/src/zopfli/zlib_container.o zopfli/src/zopfli/zopfli_lib.o
endif

#------------------------------------ Notable codecs ---------------------------------------------------------------------------
ifeq ($(BLOSC),1)
CXXFLAGS+=-D_BLOSC
OB+=c-blosc2/blosc/blosc.o c-blosc2/blosc/blosclz.o c-blosc2/blosc/schunk.o c-blosc2/blosc/delta.o c-blosc2/blosc/shuffle.o c-blosc2/blosc/shuffle-generic.o c-blosc2/blosc/shuffle-sse2.o c-blosc2/blosc/bitshuffle-generic.o
endif

ifeq ($(BRIEFLZ),1)
CXXFLAGS+=-D_BRIEFLZ
INC+=-Ibrieflz/include
OB+=brieflz/src/brieflz.o brieflz/src/depack.o
endif

ifeq ($(FLZMA2),1)
CXXFLAGS+=-D_FLZMA2
#Compile error: Disabled
OB+=fast-lzma2/dict_buffer.o fast-lzma2/fl2_common.o fast-lzma2/fl2_compress.o fast-lzma2/fl2_decompress.o fast-lzma2/lzma2_dec.o fast-lzma2/lzma2_enc.o fast-lzma2/radix_bitpack.o fast-lzma2/radix_mf.o fast-lzma2/radix_struct.o \
fast-lzma2/range_enc.o fast-lzma2/fl2_threading.o fast-lzma2/fl2_pool.o fast-lzma2/util.o
#fast-lzma2/xxhash.o fast-lzma2/fl2_error_private.o  
endif

ifeq ($(GLZA), 1)
CXXFLAGS+=-D_GLZA
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

OB+=glza/GLZAcomp.o glza/GLZAformat.o glza/GLZAcompress.o glza/GLZAencode.o glza/GLZAdecode.o glza/GLZAmodel.o 
endif

ifeq ($(IGZIP),1)
CXXFLAGS+=-DIGZIP
#OB+=isa-l/igzip/igzip.o isa-l/igzip/hufftables_c.o isa-l/igzip/igzip_base.o isa-l/igzip/igzip_icf_base.o isa-l/igzip/crc32_gzip_base.o isa-l/igzip/flatten_ll.o isa-l/igzip/encode_df.o
#OB+=isa-l/.libs/libisal.a
OB+=/usr/lib/libisal.so
endif

ifeq ($(LZ4ULTRA), 1)
CXXFLAGS+=-D_LZ4ULTRA
CXXFLAGS+=-Ilz4ultra/src -Ilz4ultra/src/libdivsufsort/include
OB+=lz4ultra/src/shrink_inmem.o lz4ultra/src/expand_inmem.o lz4ultra/src/shrink_block.o lz4ultra/src/expand_block.o lz4ultra/src/shrink_context.o lz4ultra/src/matchfinder.o lz4ultra/src/frame.o 
ifeq ($(DIVSORT), 1)
else
OB+=lz4ultra/src/libdivsufsort/lib/divsufsort.o lz4ultra/src/libdivsufsort/lib/sssort.o lz4ultra/src/libdivsufsort/lib/trsort.o
DIVSORT=1
endif
endif

ifeq ($(LZLIB), 1)
CXXFLAGS+=-D_LZLIB
OB+=lzlib-1.11/lzlib.o lzlib_/bbexample.o 
endif

ifeq ($(ZLIB_NG), 1)
CMD:= $(shell cd zlib-ng && ./configure && make && cd ..)
CXXFLAGS+=-D_ZLIB_NG
OB+=zlib-ng/libz-ng.a
endif

ifeq ($(LZO),1)
CXXFLAGS+=-D_LZO
INC+=-Ilzo/include
OB+= lzo/src/lzo1.o lzo/src/lzo1a.o lzo/src/lzo1a_99.o lzo/src/lzo1b_1.o lzo/src/lzo1b_2.o lzo/src/lzo1b_3.o lzo/src/lzo1b_4.o lzo/src/lzo1b_5.o \
lzo/src/lzo1b_6.o lzo/src/lzo1b_7.o lzo/src/lzo1b_8.o lzo/src/lzo1b_9.o lzo/src/lzo1b_99.o lzo/src/lzo1b_9x.o lzo/src/lzo1b_cc.o \
lzo/src/lzo1b_d1.o lzo/src/lzo1b_d2.o lzo/src/lzo1b_rr.o lzo/src/lzo1b_xx.o lzo/src/lzo1c_1.o lzo/src/lzo1c_2.o lzo/src/lzo1c_3.o \
lzo/src/lzo1c_4.o lzo/src/lzo1c_5.o lzo/src/lzo1c_6.o lzo/src/lzo1c_7.o lzo/src/lzo1c_8.o lzo/src/lzo1c_9.o lzo/src/lzo1c_99.o \
lzo/src/lzo1c_9x.o lzo/src/lzo1c_cc.o lzo/src/lzo1c_d1.o lzo/src/lzo1c_d2.o lzo/src/lzo1c_rr.o lzo/src/lzo1c_xx.o lzo/src/lzo1f_1.o \
lzo/src/lzo1f_9x.o lzo/src/lzo1f_d1.o lzo/src/lzo1f_d2.o lzo/src/lzo1x_1.o lzo/src/lzo1x_1k.o lzo/src/lzo1x_1l.o lzo/src/lzo1x_1o.o \
lzo/src/lzo1x_9x.o lzo/src/lzo1x_d1.o lzo/src/lzo1x_d2.o lzo/src/lzo1x_d3.o lzo/src/lzo1x_o.o lzo/src/lzo1y_1.o lzo/src/lzo1y_9x.o \
lzo/src/lzo1y_d1.o lzo/src/lzo1y_d2.o lzo/src/lzo1y_d3.o lzo/src/lzo1y_o.o lzo/src/lzo1z_9x.o lzo/src/lzo1z_d1.o lzo/src/lzo1z_d2.o \
lzo/src/lzo1z_d3.o lzo/src/lzo1_99.o lzo/src/lzo2a_9x.o lzo/src/lzo2a_d1.o lzo/src/lzo2a_d2.o lzo/src/lzo_crc.o lzo/src/lzo_init.o \
lzo/src/lzo_ptr.o lzo/src/lzo_str.o lzo/src/lzo_util.o 
endif

ifeq ($(LZSA), 1)
CXXFLAGS+=-D_LZSA
CFLAGS+=-Ilzsa/src -Ilzsa/src/libdivsufsort/include
OB+=lzsa/src/expand_block_v1.o lzsa/src/expand_block_v2.o lzsa/src/expand_context.o lzsa/src/expand_inmem.o lzsa/src/shrink_block_v1.o lzsa/src/shrink_block_v2.o lzsa/src/shrink_inmem.o lzsa/src/shrink_context.o \
    lzsa/src/matchfinder.o lzsa/src/frame.o 
ifeq ($(DIVSORT), 1)
else
OB+=lzsa/src/libdivsufsort/lib/divsufsort.o lzsa/src/libdivsufsort/lib/sssort.o lzsa/src/libdivsufsort/lib/trsort.o
DIVSORT=1
endif
endif

ifeq ($(MINIZ), 1)
CXXFLAGS+=-D_MINIZ
OB+=miniz/miniz.o miniz/miniz_tdef.o miniz/miniz_tinfl.o
endif

ifeq ($(SNAPPY), 1)
# configure or copy directory "snappy_/*" to "snappy"
ifneq (,$(wildcard snappy/snappy-stubs-public.h))
CXXFLAGS+=-DSNAPPY
OB+=snappy/snappy-sinksource.o snappy/snappy-stubs-internal.o snappy/snappy.o
endif
endif

ifeq ($(SNAPPY_C), 1)
CXXFLAGS+=-D_SNAPPY_C
OB+=snappy-c/snappy.o snappy-c/util.o
endif

ifeq ($(GIPFELI), 1)
CXXFLAGS+=-D_GIPFELI
ifeq ($(OS),$(filter $(OS),Linux GNU/kFreeBSD GNU OpenBSD FreeBSD DragonFly NetBSD MSYS_NT Haiku))
OB+=gipfeli/lz77.o gipfeli/entropy.o gipfeli/entropy_code_builder.o gipfeli/decompress.o gipfeli/gipfeli-internal.o
endif
endif

ifeq ($(SLZ), 1)
CXXFLAGS+=-D_SLZ
OB+=libslz/src/slz.o 
endif

ifeq ($(SMALLZ4), 1)
CXXFLAGS+=-DSMALLZ4
endif

#------------------------- Entropy coder -----------------------------------------
# First download or clone aomedia (git clone https://aomedia.googlesource.com/aom) into TurboBench directory
# after cmake, put the generated "aom_config.h" into the aom directory
# or copy aom_/aom_config.h to aom
ifeq ($(AOM),1)
CXXFLAGS+=-D_AOM
OB+=EC/aom_/aom.o EC/aom/aom_dsp/entenc.o EC/aom/aom_dsp/entdec.o EC/aom/aom_dsp/entcode.o 
#OB+=daala_/daala.o 
endif

# First download or clone daala (https://github.com/xiph/daala) into TurboBench directory
ifeq ($(DAALA),1)
CXXFLAGS+=-D_DAALA
OB+=EC/daala_/daala.o 
endif

ifeq ($(FASTAC),1)
CXXFLAGS+=-D_FASTAC
OB+=EC/fastac/arithmetic_codec.o
endif

ifeq ($(FASTHF),1) 
CXXFLAGS+=-D_FASTHF
OB+=EC/fasthf/binary_codec.o
endif

ifeq ($(FPAQ0P),1) 
CXXFLAGS+=-D_FPAQ0P
OB+=EC/fpaq0p/fpaq0p_sh.o 
endif

ifeq ($(FPC), 1)
CXXFLAGS+=-D_FPC
OB+=EC/FPC/fpc.o
endif

ifeq ($(FREQTAB),1)
CXXFLAGS+=-D_FREQTAB

ifeq ($(FREQTABO),1)
FREQOPT=-march=skylake -fwhole-program -fpermissive -fstrict-aliasing -fomit-frame-pointer -I../Lib3 -I../Lib \
-fno-stack-protector -fno-stack-check -fno-check-new \
-fno-exceptions -fno-rtti -fno-operator-names \
-flto -ffat-lto-objects -Wl,-flto -fuse-linker-plugin -Wl,-O -Wl,--sort-common -Wl,--as-needed -ffunction-sections

EC/freqtab/src/c_mem.o: EC/freqtab/src/c_mem.cpp
	$(CXX) $(FREQOPT) -O3 -std=c++11 $< -c -o $@ 

EC/freqtab/src/model.o: EC/freqtab/src/model.cpp
	$(CXX) $(FREQOPT) -O3 -march=skylake -std=c++11 $< -c -o $@ 
endif

OB+=EC/freqtab/src/c_mem.o EC/freqtab/src/coder/model.o
endif


ifeq ($(RANS_S),1) 
CXXFLAGS+=-D_RANS_S
EC/rans_static/r32x16b_avx2.o: EC/rans_static/r32x16b_avx2.c
	$(CC) -O3 -mavx2 $(MARCH) $< -c -o $@ 
OB+=EC/rans_static/rANS_static4x8.o EC/rans_static/rANS_static4x16pr.o EC/rans_static/rANS_static.o EC/rans_static/arith_static.o
ifeq ($(AVX2),1)
OB+=EC/rans_static/r32x16b_avx2.o
endif
endif

ifeq ($(RECIPARITH),1) 
CXXFLAGS+=-D_RECIPARITH
OB+=EC/recip_arith_/reciparith.o
endif

ifeq ($(SUBOTIN),1) 
CXXFLAGS+=-D_SUBOTIN
OB+=EC/subotin_/subotin.o 
endif

ifeq ($(TURBORC),1) 
CXXFLAGS+=-D_TURBORC
OB+=Turbo-Range-Coder/turborcs.o Turbo-Range-Coder/turborcss.o #Turbo-Range-Coder/turborcn.o
endif

ifeq ($(VECRC),1) 
CXXFLAGS+=-D_VECRC
OB+=EC/vecrc/vector_rc.o
endif

#OB+=FastARI/FastAri.o 
#OB+=nibrans/tenc.o nibrans/tdec.o
#OB+=fqz0/f_o0.o
#OB+=ppmdec/ppmdec.o
#OB+=ans_nania/narans.o 
#OB+=marlin/src/compress.o marlin/src/configuration.o marlin/src/decompress.o marlin/src/dictionary.o marlin/src/marlin.o
#endif
#ifeq ($(NCOMP1), 0)
#OB+=FiniteStateEntropy/lib/fse.o FiniteStateEntropy/lib/huff0.o
#endif
#OB+=fsc/fsc_enc.o fsc/fsc_dec.o fsc/fsc_utils.o fsc/bits.o fsc/histo.o fsc/alias.o 
#OB+=polar/polar.o fpaqc/fpaqc.o
#-------------------- Encoding ------------------------
ifeq ($(TURBORLE), 1)
CXXFLAGS+=-D_TURBORLE
OB+=Turbo-Run-Length-Encoding/trlec.o Turbo-Run-Length-Encoding/trled.o 
endif

ifeq ($(MRLE),1)
CXXFLAGS+=-D_MRLE
OB+=Turbo-Run-Length-Encoding/ext/mrle.o
endif

ifeq ($(RLE8),1)
CXXFLAGS+=-D_RLE8
OB+=rle8/src/rle8_cpu.o rle8/src/rle8_ultra_cpu.o rle8/src/rle8_extreme_cpu.o rle8/src/rleX_extreme_cpu.o rle8/src/rle24_extreme_cpu.o rle8/src/rle48_extreme_cpu.o
endif

#-------------------------------------- Archived ----------------------------------
ifeq ($(CHAMELEON),1)
CXXFLAGS+=-D_CHAMELEON
OB+=chameleon/chameleon.o
endif

ifeq ($(DENSITY),1)
CXXFLAGS+=-D_DENSITY
OB+=density/src/buffers/buffer.o density/src/algorithms/algorithms.o density/src/algorithms/dictionaries.o density/src/structure/header.o density/src/globals.o density/src/buffers/buffer.o \
	density/src/algorithms/chameleon/core/chameleon_decode.o density/src/algorithms/chameleon/core/chameleon_encode.o \
	density/src/algorithms/lion/core/lion_decode.o density/src/algorithms/lion/core/lion_encode.o density/src/algorithms/lion/forms/lion_form_model.o \
	density/src/algorithms/cheetah/core/cheetah_decode.o density/src/algorithms/cheetah/core/cheetah_encode.o 
endif

ifeq ($(XPACK), 1)
CXXFLAGS+=-D_XPACK
# O2 instead of O3 because of error gcc 7
xpack/lib/xpack_common.o: xpack/lib/xpack_common.c
	$(CC) -O2 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@ 

xpack/lib/xpack_compress.o: xpack/lib/xpack_compress.c
	$(CC) -O2 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@ 

xpack/lib/xpack_decompress.o: xpack/lib/xpack_decompress.c
	$(CC) -O2 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@ 

xpack/lib/x86_cpu_features.o: xpack/lib/x86_cpu_features.c
	$(CC) -O2 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@

OB+=xpack/lib/xpack_common.o xpack/lib/xpack_compress.o xpack/lib/xpack_decompress.o xpack/lib/x86_cpu_features.o
endif

ifeq ($(PITHY), 1)
CXXFLAGS+=-D_PITHY
pithy/pithy.o: pithy/pithy.c
	$(CC) -O2 $(MARCH) $(CFLAGS)  $< -c -o $@
OB+=pithy/pithy.o
endif

ifeq ($(SHRINKER), 1)
CXXFLAGS+=-D_SHRINKER
shrinker/shrinker.o: shrinker/shrinker.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@  
OB+=shrinker/Shrinker.o
endif

ifeq ($(WFLZ), 1)
CXXFLAGS+=-D_WFLZ
wflz/wfLZ.o: wflz/wfLZ.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@
OB+=wflz/wfLZ.o 
endif

ifeq ($(NAKAMICHI),1)
CXXFLAGS+=-D_NAKAMICHI
nakamichi/Nakamichi_Okamigan.o: nakamichi/Nakamichi_Okamigan.c
	$(CC) -O3 -msse4.1 $(MARCH) $< -c -o $@ 
OB+=nakamichi/Nakamichi_Washigan.o
endif

ifeq ($(FASTLZ),1)
CXXFLAGS+=-D_FASTLZ
OB+=FastLZ/fastlz.o 
endif

ifeq ($(HEATSHRINK),1)
CXXFLAGS+=-D_HEATSHRINK
OB+=heatshrink_/heatshrink.o heatshrink/heatshrink_encoder.o heatshrink/heatshrink_decoder.o
endif

ifeq ($(LIBLZF),1)
CXXFLAGS+=-D_LIBLZF
OB+=liblzf/lzf_c.o liblzf/lzf_c_best.o liblzf/lzf_d.o 
endif

ifeq ($(LIBLZG),1)
CXXFLAGS+=-D_LIBLZG
OB+=liblzg/src/lib/encode.o liblzg/src/lib/decode.o liblzg/src/lib/checksum.o 
endif

ifeq ($(SHOCO),1)
CXXFLAGS+=-D_SHOCO
OB+=shoco/shoco.o
endif

ifeq ($(YAPPY),1)
CXXFLAGS+=-D_YAPPY
OB+=yappy/yappy.o 
endif

ifeq ($(ZPAQ),1)
ifeq ($(OS),$(filter $(OS),Darwin))
else
CXXFLAGS+=-D_ZPAQ
OB+=zpaq/libzpaq.o
endif
endif

ifeq ($(CSC),1)
CXXFLAGS+=-D_CSC
OB+=CSC/src/libcsc/csc_analyzer.o CSC/src/libcsc/csc_coder.o CSC/src/libcsc/csc_dec.o CSC/src/libcsc/csc_default_alloc.o CSC/src/libcsc/csc_enc.o CSC/src/libcsc/csc_encoder_main.o CSC/src/libcsc/csc_filters.o CSC/src/libcsc/csc_lz.o CSC/src/libcsc/csc_memio.o \
	CSC/src/libcsc/csc_mf.o CSC/src/libcsc/csc_model.o CSC/src/libcsc/csc_profiler.o 
endif

ifeq ($(DOBOZ),1)
CXXFLAGS+=-D_DOBOZ
OB+=doboz/Source/Doboz/Compressor.o doboz/Source/Doboz/Decompressor.o doboz/Source/Doboz/Dictionary.o
endif

ifeq ($(LIBZLING),1)
CXXFLAGS+=-D_LIBZLING
# Disabled : compile error in gcc 7.2
#OB+=libzling/src/libzling.o libzling/src/libzling_huffman.o libzling/src/libzling_utils.o libzling/src/libzling_lz.o libzling_/libzling_utils_mem.o 
endif

ifeq ($(BRC), 1)
CXXFLAGS+=-D_BRC
OB+=Behemoth-Rank-Coding/brc.o
endif
#----------------------- GPL -------------------------
ifeq ($(LZMAT), 1)
CXXFLAGS+=-DLZMAT
lzmat/lzmat_dec.o: lzmat/lzmat_dec.c
	$(CC) -O2 -D"__int64=long long" $(MARCH) $(CFLAGS) $< -c -o $@
lzmat/lzmat_enc.o: lzmat/lzmat_enc.c
	$(CC) -O2 -D"__int64=long long" $(MARCH) $(CFLAGS) $< -c -o $@

OB+=lzmat/lzmat_enc.o lzmat/lzmat_dec.o
endif

ifeq ($(TORNADO), 1)
CXXFLAGS+=-D_TORNADO
tornado_/tormem.o: tornado_/tormem.cpp
	$(CXX) -O3 $(TORDEF) -D__x86_$(ARCH)__ -DFREEARC_$(ARCH)BIT -pipe -fforce-addr -fno-exceptions -fno-rtti -c tornado_/tormem.cpp -o tornado_/tormem.o
ifeq ($(OS), Linux)
TORDEF=-DFREEARC_UNIX -DFREEARC_INTEL_BYTE_ORDER
else
TORDEF=-DFREEARC_WIN -DFREEARC_INTEL_BYTE_ORDER -D_UNICODE -DUNICODE 
endif
OB+=tornado_/tormem.o
endif

ifeq ($(MSCOMPRESS), 1)
CXXFLAGS+=-D_MSCOMPRESS
OB+=ms-compress/src/mscomp.o ms-compress/src/lznt1_compress.o ms-compress/src/lznt1_decompress.o ms-compress/src/xpress_compress.o ms-compress/src/xpress_decompress.o ms-compress/src/xpress_huff_compress.o ms-compress/src/xpress_huff_decompress.o
endif

ifeq ($(QUICKLZ), 1)
CXXFLAGS+=-D_QUICKLZ
OB+=quicklz_/quicklz1.o quicklz_/quicklz2.o quicklz_/quicklz3.o 
endif

ifeq ($(PYSAP), 1)
CXXFLAGS+=-D_PYSAP
OB+=pysap/pysapcompress/vpa105CsObjInt.o pysap/pysapcompress/vpa106cslzc.o pysap/pysapcompress/vpa107cslzh.o pysap/pysapcompress/vpa108csulzh.o
endif

#--------------------------------------------------------------------
OB+=$(ICL) $(HUF) $(ANS) $(LZ) plugins.o

turbobench: $(OB) turbobench.o  
	$(CXX) $^ $(LDFLAGS) -o turbobench
 
.c.o:
	$(CC) -O3 $(MARCH) $(CFLAGS) $< -c -o $@  

.cc.o:
	$(CXX) -O3 $(MARCH) $(CXXFLAGS)  $< -c -o $@ 

.cpp.o:
	$(CXX) -O3 $(MARCH) $(CXXFLAGS) $< -c -o $@ 

ifeq ($(OS),Windows_NT)
clean:
	del /S *.o 
	del /S *~
	del /S *.exe
else
clean:
	find . -name "turbobench" -type f -delete
	find . -name "*.o" -type f -delete
	find . -name "*~" -type f -delete
	find . -name "core" -type f -delete

cleana:
	find . -name "turbobench" -type f -delete
	find . -name "*.o" -type f -delete
	find . -name "*~" -type f -delete
	find . -name "core" -type f -delete
	rm -rf --interactive=never .git
	rm -rf pysap/docs
	rm -rf brotli/tests
	rm -rf brotli/java
	rm -rf brotli/research
	rm -rf isa-l/erasure_code
	rm -rf isa-l/crc
	rm -rf lzma/bin
	rm -rf lzma/CPP
	rm -rf snappy/testdata
	rm -rf zstd/contrib
	rm -rf zstd/lib/legacy
	rm -rf zstd/tests
	rm -rf zstd/doc
	rm -rf zlib/contrib
endif


