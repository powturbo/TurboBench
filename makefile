# powturbo  (c) Copyright 2013-2026
# ----------- Downloading + Compiling ----------------------
# git clone --recursive git://github.com/powturbo/TurboBench.git
# make
#
#----------------
# Cross compile: export CROSS to aarch64 riscv64 loongarch64 or powerpc64le. Ex.:
# export CROSS=aarch64
# Testing with qemu
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./icapp ZIPF
#LZTURBO=1

CC ?= gcc
#CC ?= clang
CXX ?= g++
CX ?= clang
#CX ?= gcc
#CC = clang

#DEBUG=-DDEBUG -g
DEBUG=-DNDEBUG
JAVA_HOME ?= /usr/lib/jvm/java-8-openjdk-amd64
PREFIX ?= /usr/local
DIRBIN ?= $(PREFIX)/bin
DIRINC ?= $(PREFIX)/include
DIRLIB ?= $(PREFIX)/lib
SRC ?= lib/

#------- OS/ARCH -------------------
ifneq (,$(filter Windows%,$(OS)))
  OS := Windows
  CC=gcc
# CC=clang
# CX=gcc
  CX=clang
  CXX=g++
  ARCH=x86_64
else
  OS := $(shell uname -s)
  ARCH := $(shell uname -m)
endif
#$(info OS="$(OS)")

ifndef CROSS
else
ifeq ($(OS), Windows)
CP=$(CROSS)-unknown-elf
else
CP=$(CROSS)-linux-gnu
endif

CXX:=$(CP)-g++
ifeq ($(CX),clang)
CX=clang --target=$(CP) --sysroot=/usr/$(CP) -fuse-ld=lld
ifeq ($(CC),clang)
CC=$(CX)
else
CC:=$(CP)-gcc
endif
else
CC:=$(CP)-gcc
CX=$(CC)
endif

endif

ifneq (,$(or $(findstring aarch64,$(CC) $(ARCH)),$(findstring arm64,$(CC) $(ARCH))))
  ARCH = aarch64
else ifneq (,$(findstring riscv64,$(CC) $(ARCH)))
  ARCH = riscv64
else ifneq (,$(findstring iPhone,$(ARCH)))
  ARCH = aarch64
  CFLAGS=-DHAVE_MALLOC_MALLOC
else ifneq (,$(findstring powerpc64le,$(CC) $(ARCH)))
  ARCH = ppc64le
else ifneq (,$(findstring loongarch64,$(CC) $(ARCH)))
  ARCH = loongarch64
else ifneq (,$(findstring x86_64,$(CC) $(ARCH)))
  ARCH = x86_64
endif

ifeq ($(ARCH),aarch64)
  _SSE=-march=armv8-a
  CFLAGS+=$(_SSE)
else ifeq ($(ARCH),riscv64)
#  CFLAGS=-march=rv64gcv -mabi=lp64d
  CFLAGS=-march=rv64gcv_zvbb -mabi=lp64d
else ifeq ($(ARCH),ppc64le)
  _SSE=-D__SSE4_1__
  CFLAGS=-mcpu=power9 -mtune=power9 $(_SSE)
else ifeq ($(ARCH),loongarch64)
  _SSE=-mlsx
  CFLAGS=$(_SSE)
else ifeq ($(ARCH),x86_64)
# _SSE=-mssse3 
# _SSE+=-mno-avx -mno-aes
# _SSE=-march=corei7-avx -mtune=corei7-avx
# _SSE=-march=ivybridge -mavx
  _SSE=-mavx -mpopcnt

# _AVX2=-march=skylake-avx512 -mavx512vbmi -mavx512f -mavx512vl
  _AVX2=-march=haswell
endif

ifeq ($(OS),Windows)
  LDFLAGS=-Wl,--stack,33554432
endif

ifneq ($(ARCH),x86_64)
TURBORC=0
SNAPPY_C=0
LZHAM=0
endif

CFLAGS+=-w -Wall $(DEBUG) -fpermissive -Wimplicit-function-declaration

ifeq ($(OS),$(filter $(OS),Linux GNU/kFreeBSD GNU OpenBSD FreeBSD DragonFly NetBSD MSYS_NT Haiku))
LDFLAGS+=-lrt -lpthread
endif

ifdef STATIC
LDFLAGS+=-static
NMEMSIZE=1
endif

# disable peak memory calculation
ifeq ($(OS),$(filter $(OS),Darwin))
NMEMSIZE=1
endif

ifdef NMEMSIZE
CFLAGS+=-DNMEMSIZE
else
ifeq ($(OS),$(filter $(OS),Darwin FreeBSD GNU/kFreeBSD Linux NetBSD SunOS))
LDFLAGS += -ldl
endif
endif

ifdef OPENMP
CFLAGS+=-fopenmp -DLIBBSC_OPENMP_SUPPORT
LDFLAGS+=-fopenmp
endif

all: copy-miniz  turbobench 

ifdef LZTURBO
CXXFLAGS+=-D_LZTURBO
CFLAGS+=-D_LZTURBO
include ../dev/x/lzturbo.mk
endif

#--------------------------------- codecs --------------------------------------------------------------------------------------------
ifneq ($(wildcard brotli/.),)
CXXFLAGS+=-D_BROTLI -Ibrotli/c/include 
CFLAGS+=-Ibrotli/c/include 
#-Ibrotli/c/enc
ifdef BROTLILIB  # Use the libraries created by the brotli own builds
OB+=brotli/out/libbrotlicommon.so brotli/out/libbrotlidec.so brotli/out/libbrotlienc.so
else
OB+=brotli/c/common/constants.o brotli/c/common/context.o brotli/c/common/dictionary.o brotli/c/common/platform.o brotli/c/common/transform.o brotli/c/common/shared_dictionary.o brotli/c/dec/huffman.o brotli/c/dec/prefix.o\
    brotli/c/dec/state.o brotli/c/dec/static_init.o brotli/c/dec/bit_reader.o brotli/c/dec/decode.o \
    brotli/c/enc/backward_references.o brotli/c/enc/bit_cost.o brotli/c/enc/brotli_bit_stream.o brotli/c/enc/block_splitter.o brotli/c/enc/cluster.o brotli/c/enc/command.o brotli/c/enc/compound_dictionary.o brotli/c/enc/encode.o brotli/c/enc/encoder_dict.o brotli/c/enc/compress_fragment.o brotli/c/enc/compress_fragment_two_pass.o \
    brotli/c/enc/encoder_dict.o brotli/c/enc/entropy_encode.o brotli/c/enc/histogram.o brotli/c/enc/fast_log.o brotli/c/enc/literal_cost.o  brotli/c/enc/memory.o brotli/c/enc/metablock.o brotli/c/enc/utf8_util.o brotli/c/enc/backward_references_hq.o \
    brotli/c/enc/dictionary_hash.o brotli/c/enc/static_dict.o brotli/c/enc/static_init.o brotli/c/enc/static_dict_lut.o
endif
endif

ifneq ($(wildcard libbsc/.),)
CXXFLAGS+=-D_LIBBSC -DLIBBSC_SORT_TRANSFORM_SUPPORT -ICSC/src/libcsc
OB+=libbsc/libbsc/libbsc/libbsc.o libbsc/libbsc/coder/coder.o libbsc/libbsc/coder/qlfc/qlfc.o libbsc/libbsc/coder/qlfc/qlfc_model.o libbsc/libbsc/filters/detectors.o \
	libbsc/libbsc/filters/preprocessing.o libbsc/libbsc/adler32/adler32.o libbsc/libbsc/bwt/bwt.o libbsc/libbsc/st/st.o libbsc/libbsc/lzp/lzp.o
OB+=libbsc/libbsc/platform/platform.o libbsc/libbsc/bwt/libsais/libsais.o libbsc/libbsc/bwt/libsais/libsais.o
LIBSAIS=1
endif

ifneq ($(wildcard bzip2/.),)
CXXFLAGS+=-D_BZIP2
OB+=bzip2/blocksort.o bzip2/huffman.o bzip2/crctable.o bzip2/randtable.o bzip2/compress.o bzip2/decompress.o bzip2/bzlib.o
endif

ifneq ($(wildcard bzip3/.),)
CXXFLAGS+=-D_BZIP3
CFLAGS+=-DVERSION=1 -Ibzip3/include -Wno-int-conversion
OB+=bzip3/src/libbz3.o
endif

ifneq ($(wildcard libdeflate/.),)
CXXFLAGS+=-D_LIBDEFLATE
CFLAGS+=-Ilibdeflate -Ilibdeflate/common
OB+=libdeflate/lib/adler32.o libdeflate/lib/crc32.o libdeflate/lib/arm/cpu_features.o libdeflate/lib/x86/cpu_features.o \
    libdeflate/lib/deflate_compress.o libdeflate/lib/deflate_decompress.o libdeflate/lib/gzip_compress.o libdeflate/lib/gzip_decompress.o libdeflate/lib/zlib_compress.o libdeflate/lib/zlib_decompress.o libdeflate/lib/utils.o
endif

ifneq ($(wildcard lizard/.),)
CXXFLAGS+=-D_LIZARD
CFLAGS+=-Ilizard/lib
OB+=lizard/lib/entropy/entropy_common.o lizard/lib/entropy/hist.o lizard/lib/lizard_compress.o lizard/lib/lizard_decompress.o lizard/lib/entropy/huf_decompress.o lizard/lib/entropy/huf_compress.o lizard/lib/entropy/fse_compress.o lizard/lib/entropy/fse_decompress.o
endif

ifneq ($(wildcard lz4/.),)
CXXFLAGS+=-D_LZ4
CFLAGS+=-Ilz4/lib
OB+=lz4/lib/lz4hc.o lz4/lib/lz4.o lz4/lib/lz4frame.o lz4/lib/xxhash.o
endif

ifneq ($(wildcard lzav/.),)
CXXFLAGS+=-D_LZAV
endif

ifneq ($(wildcard lzfse/.),)
CXXFLAGS+=-D_LZFSE
OB+=lzfse/src/lzfse_decode_base.o lzfse/src/lzfse_decode.o lzfse/src/lzfse_encode_base.o lzfse/src/lzfse_encode.o lzfse/src/lzfse_fse.o lzfse/src/lzvn_decode_base.o lzfse/src/lzvn_encode_base.o
endif

ifneq ($(wildcard lzham/.),)
CXXFLAGS+=-D_LZHAM -D"UINT64_MAX=-1ull" -Ilzham_codec_devel/include -Ilzham_codec_devel/lzhamcomp -Ilzham_codec_devel/lzhamdecomp
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

ifneq ($(wildcard lzjody/.),)
CXXFLAGS+=-D_LZJODY
OB+=lzjody/lzjody.o lzjody/byteplane_xfrm.o
endif

ifneq ($(wildcard lzma/.),)
CXXFLAGS+=-D_LZMA
CFLAGS+=-D_7ZIP_ST
CXXFLAGS+=-D_7Z_TYPES_
OB+=lzma/C/Alloc.o lzma/C/CpuArch.o lzma/C/LzFind.o lzma/C/LzmaDec.o lzma/C/LzmaEnc.o lzma/C/LzmaLib.o
#ifeq ($(OS),Windows_NT)
OB+=lzma/C/Threads.o lzma/C/LzFindMt.o lzma/C/LzFindOpt.o
#endif
endif

ifneq ($(wildcard lzoma_/.),)
CXXFLAGS+=-D_LZOMA
OB+=lzoma_/pack.o lzoma_/unpack.o lzoma_/divsufsort.o
endif

ifneq ($(and $(wildcard LZSSE/.),$(filter x86_64,$(ARCH))),)
CXXFLAGS+=-D_LZSSE
LZSSE/lzsse2/lzsse2.o: LZSSE/lzsse2/lzsse2.cpp
	$(CXX) -O2 -msse4.1 -std=c++11 $< -c -o $@
LZSSE/lzsse4/lzsse4.o: LZSSE/lzsse4/lzsse4.cpp
	$(CXX) -O2 -msse4.1 -std=c++11  $< -c -o $@
LZSSE/lzsse8/lzsse8.o: LZSSE/lzsse8/lzsse8.cpp
	$(CXX) -O2 -msse4.1 -std=c++11  $< -c -o $@
OB+=LZSSE/lzsse2/lzsse2.o LZSSE/lzsse4/lzsse4.o LZSSE/lzsse8/lzsse8.o
endif

ifneq ($(wildcard zstd/.),)
CXXFLAGS+=-D_ZSTD -Izstd/lib -Izstd/lib/common
ifeq ($(ZSTDLIB), 1)
# compile zstd: "cd zstd" and then "make"
#LDFLAGS+=-lzstd
#LDFLAGS+=zstd/lib/libzstd.a
LDFLAGS+=zstd-v1.5.4-win64/static/libzstd_static.lib
#LDFLAGS+=zstd/lib/dll/libzstd.dll.a
#LDFLAGS+=libzstd_static.lib
else
#CFLAGS+=-DZSTD_BUILD_STATIC -DZSTD_STATIC_LINKING_ONLY -DFSE_STATIC_LINKING_ONLY -DHUF_STATIC_LINKING_ONLY
#CFLAGS+=-DZSTD_DISABLE_ASM
CFLAGS+=-Izstd/lib -Izstd/lib/common
ZT0=zstd/lib/common/
ZTC=zstd/lib/compress/
ZTD=zstd/lib/decompress/
OB+=$(ZT0)pool.o $(ZT0)xxhash.o $(ZT0)error_private.o $(ZT0)fse_decompress.o $(ZT0)zstd_common.o $(ZT0)entropy_common.o \
    $(ZTC)hist.o $(ZTC)zstd_compress.o $(ZTC)zstd_compress_literals.o $(ZTC)zstd_compress_sequences.o $(ZTC)zstd_double_fast.o $(ZTC)zstd_fast.o $(ZTC)zstd_lazy.o $(ZTC)/zstd_preSplit.o\
	$(ZTC)zstd_ldm.o $(ZTC)zstdmt_compress.o $(ZTC)zstd_opt.o $(ZTC)fse_compress.o $(ZTC)zstd_compress_superblock.o \
    $(ZTD)zstd_decompress.o $(ZTD)zstd_decompress_block.o $(ZTD)zstd_ddict.o $(ZTD)huf_decompress_amd64.o $(ZTC)huf_compress.o $(ZTD)huf_decompress.o  
endif
endif

FSE := EC/fse
ifneq ($(wildcard FSE/.),)
CXXFLAGS+=-D_FSE
OB+=$(LB)EC/fse/fse_compress_.o $(LB)EC/fse/fse_decompress_.o 
endif

#ifdef FSEHUF
#CXXFLAGS+=-D_FSEHUF
#OB+=$(LB)EC/fse/huf_compress_.o $(LB)EC/fse/huf_decompress_.o 
#else
#OB+=$(LB)zstd/lib/compress/huf_compress.o $(LB)zstd/lib/decompress/huf_decompress.o 
#endif

ifneq ($(wildcard zlib/.),)
CXXFLAGS+=-D_ZLIB
ZD=zlib/
OB+=$(ZD)adler32.o $(ZD)crc32.o $(ZD)compress.o $(ZD)deflate.o $(ZD)infback.o $(ZD)inffast.o $(ZD)inflate.o $(ZD)inftrees.o $(ZD)trees.o $(ZD)uncompr.o $(ZD)zutil.o
endif

ifneq ($(wildcard zopfli/.),)
CXXFLAGS+=-D_ZOPFLI
OB+=zopfli/src/zopfli/blocksplitter.o zopfli/src/zopfli/cache.o zopfli/src/zopfli/deflate.o zopfli/src/zopfli/gzip_container.o zopfli/src/zopfli/hash.o zopfli/src/zopfli/util.o zopfli/src/zopfli/lz77.o zopfli/src/zopfli/tree.o zopfli/src/zopfli/squeeze.o zopfli/src/zopfli/katajainen.o zopfli/src/zopfli/zlib_container.o zopfli/src/zopfli/zopfli_lib.o
endif

#------------------------------------ Notable codecs ---------------------------------------------------------------------------
ifneq ($(wildcard c-blosc2/.),)
CXXFLAGS+=-D_C_BLOSC2
CFLAGS+=-Ic-blosc2/blosc -Ic-blosc2/include -Ic-blosc2/include/blosc2 -DHAVE_ZSTD
OB+=c-blosc2/blosc/blosc2.o c-blosc2/blosc/blosclz.o c-blosc2/blosc/frame.o c-blosc2/blosc/sframe.o c-blosc2/blosc/schunk.o c-blosc2/blosc/blosc2-stdio.o c-blosc2/blosc/fastcopy.o c-blosc2/blosc/stune.o \
    c-blosc2/blosc/delta.o c-blosc2/blosc/shuffle.o c-blosc2/blosc/shuffle-generic.o c-blosc2/blosc/shuffle-sse2.o c-blosc2/blosc/timestamp.o c-blosc2/blosc/trunc-prec.o c-blosc2/blosc/bitshuffle-generic.o
endif

ifneq ($(wildcard brieflz/.),)
CXXFLAGS+=-D_BRIEFLZ
CFLAGS+=-Ibrieflz/include
OB+=brieflz/src/brieflz.o brieflz/src/depack.o
endif

ifneq ($(wildcard fast-lzma2/.),)
CXXFLAGS+=-D_FLZMA2
OB+=fast-lzma2/dict_buffer.o fast-lzma2/fl2_common.o fast-lzma2/fl2_compress.o fast-lzma2/fl2_decompress.o fast-lzma2/lzma2_dec.o fast-lzma2/lzma2_enc.o fast-lzma2/radix_bitpack.o fast-lzma2/radix_mf.o fast-lzma2/radix_struct.o \
fast-lzma2/range_enc.o fast-lzma2/fl2_threading.o fast-lzma2/fl2_pool.o fast-lzma2/util.o
#fast-lzma2/xxhash.o fast-lzma2/fl2_error_private.o
endif

ifneq ($(wildcard glza/.),)
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

ifneq ($(wildcard isa-l_NOP/.),)
CXXFLAGS+=-D_ISA_L
ifeq ($(OS),Windows)
# msys2 build: install nasm and type:
# mingw32-make -f Makefile.unx  arch=mingw  host_cpu=x86_64  have_as_w_avx512= CC=gcc AS=yasm AR=ar STRIP=strip LDFLAGS=  CFLAGS_mingw=-m64
LDFLAGS+=isa-l_/win64/isa-l.a
else
#ISA-L library needs to be installed before use
CXXFLAGS+=-DHAVE_IGZIP -D_ISA_L
#LDFLAGS+=-lisa-l
LDFLAGS+=isa-l_/linux/libisal.a
endif
endif

ifneq ($(wildcard lz4ultra/.),)
CXXFLAGS+=-D_LZ4ULTRA -Ilz4ultra/src -Ilz4ultra/src/libdivsufsort/include
OB+=lz4ultra/src/shrink_inmem.o lz4ultra/src/expand_inmem.o lz4ultra/src/shrink_block.o lz4ultra/src/expand_block.o lz4ultra/src/shrink_context.o lz4ultra/src/matchfinder.o lz4ultra/src/frame.o
ifeq ($(DIVSORT), 1)
else
OB+=lz4ultra/src/libdivsufsort/lib/divsufsort.o lz4ultra/src/libdivsufsort/lib/sssort.o lz4ultra/src/libdivsufsort/lib/trsort.o
DIVSORT=1
endif
endif

ifneq ($(wildcard lzlib-1.13/.),)
CXXFLAGS+=-D_LZLIB
OB+=lzlib-1.13/lzlib.o lzlib_/bbexample.o
endif

ifneq ($(wildcard zlib-ng/.),)
CMD:= $(shell cd zlib-ng && ./configure && make && cd ..)
CXXFLAGS+=-D_ZLIB_NG
LDFLAGS+=zlib-ng/libz-ng.a
#ifeq ($(OS),Windows)
#OB+=msys-z-ng.dll
#LDFLAGS+=zlib-ng_/win64/libz-ng.a
#else
#LDFLAGS+=-lz-ng
#LDFLAGS+=zlib-ng_/linux/libz-ng.a
#endif
endif

ifneq ($(wildcard lzo/.),)
CXXFLAGS+=-D_LZO -Ilzo/include
CFLAGS+=-Ilzo/include
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

ifneq ($(wildcard lzsa/.),)
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

.PHONY: copy-miniz
ifneq ($(wildcard miniz/.),)
copy-miniz:
	cp miniz_/miniz_export.h miniz/miniz_export.h
CXXFLAGS+=-D_MINIZ
OB+=miniz/miniz.o miniz/miniz_tdef.o miniz/miniz_tinfl.o
endif

ifneq ($(wildcard misa77/.),)
CXXFLAGS+=-D_MISA77
MISA77_DIR = misa77
MISA77_INC = -I$(MISA77_DIR)/include -I$(MISA77_DIR)/src
MISA77_SRCS := $(wildcard $(MISA77_DIR)/src/*.cpp) $(wildcard $(MISA77_DIR)/src/experimental/*.cpp)
MISA77_OBJS := $(patsubst %.cpp, %.o, $(MISA77_SRCS))
MISA77_BUILD = $(CXX) -O3 $(CXXFLAGS) -std=c++20 $(MISA77_INC) $(MISA77_FLAGS) $< -c -o $@
$(MISA77_DIR)/src/%_sse2.o: CXXFLAGS += $(_SSE)
$(MISA77_DIR)/src/%_avx2.o: CXXFLAGS += $(_AVX2)
$(MISA77_DIR)/src/%.o: $(MISA77_DIR)/src/%.cpp
	$(MISA77_BUILD)
$(MISA77_DIR)/src/experimental/%.o: $(MISA77_DIR)/src/experimental/%.cpp
	$(MISA77_BUILD)

OB += $(MISA77_OBJS)
ifeq ($(ARCH),x86_64)
  OB += $(MISA77_DIR)/src/isa/target_sse2.o $(MISA77_DIR)/src/isa/target_avx2.o
  OB += $(MISA77_DIR)/src/experimental/isa/etarget_sse2.o $(MISA77_DIR)/src/experimental/isa/etarget_avx2.o
endif

endif

ifneq ($(wildcard snappy/.),)
# configure or copy directory "snappy_/*" to "snappy"
ifneq (,$(wildcard snappy/snappy-stubs-public.h))
CXXFLAGS+=-D_SNAPPY
OB+=snappy/snappy-sinksource.o snappy/snappy-stubs-internal.o snappy/snappy.o
endif
endif

ifneq ($(wildcard snappy-c/.),)
CXXFLAGS+=-D_SNAPPY_C
OB+=snappy-c/snappy.o snappy-c/util.o
endif

ifneq ($(wildcard gipfeli/.),)
#ifeq ($(OS),$(filter $(OS),Linux GNU/kFreeBSD GNU OpenBSD FreeBSD DragonFly NetBSD MSYS_NT Haiku))
CXXFLAGS+=-D_GIPFELI
OB+=gipfeli/lz77.o gipfeli/entropy.o gipfeli/entropy_code_builder.o gipfeli/decompress.o gipfeli/gipfeli-internal.o
#endif
endif

#ifdef OODLE
CXXFLAGS+=-D_OODLE
# oodle dll 'oo2core_9_win64.dll', 'liboo2corelinuxarm64.so.9' or 'liboo2corelinux64.so.9' must be in the same directory as turbobench[.exe]
#endif

ifneq ($(wildcard libslz/.),)
CXXFLAGS+=-D_SLZ
OB+=libslz/src/slz.o
endif

ifneq ($(wildcard tcobs/.),)
CXXFLAGS+=-D_TCOBS -Drestrict=__restrict
OB+=tcobs/v2/tcobsEncode.o tcobs/v2/tcobsDecode.o
endif

ifdef SMALLZ4
CXXFLAGS+=-DSMALLZ4
endif

ifneq ($(wildcard Unishox2/.),)
CXXFLAGS+=-D_UNISHOX2
OB+=Unishox2/unishox2.o turbobench_/unishox.o
CXXFLAGS+=-D_UNISHOX3 -Imarisa-trie/include
OB+=Unishox2/Unishox3_Alpha/unishox3.o
endif

#------------------------- Entropy coder -----------------------------------------

# First download or clone aomedia (git clone https://aomedia.googlesource.com/aom) into TurboBench directory
# after cmake, put the generated "aom_config.h" into the aom directory
# or copy aom_/aom_config.h to aom
ifneq ($(wildcard EC/aom/.),)
CXXFLAGS+=-D_AOM
OB+=EC/aom_/aom.o EC/aom/aom_dsp/entenc.o EC/aom/aom_dsp/entdec.o EC/aom/aom_dsp/entcode.o
#OB+=daala_/daala.o
endif

# First download or clone daala (https://github.com/xiph/daala) into TurboBench directory
ifneq ($(wildcard EC/daala/.),)
CXXFLAGS+=-D_DAALA
OB+=EC/daala_/daala.o
endif

ifneq ($(wildcard EC/fastac/.),)
CXXFLAGS+=-D_FASTAC
OB+=EC/fastac/arithmetic_codec.o
endif

ifneq ($(wildcard EC/fasthf/.),)
CXXFLAGS+=-D_FASTHF
OB+=EC/fasthf/binary_codec.o
endif

ifneq ($(wildcard EC/fpaq0p/.),)
CXXFLAGS+=-D_FPAQ0P
OB+=EC/fpaq0p/fpaq0p_sh.o
endif

ifneq ($(wildcard EC/FPC/.),)
CXXFLAGS+=-D_FPC
OB+=EC/FPC/fpc.o
endif

ifdef FREQTAB
CXXFLAGS+=-D_FREQTAB

ifdef FREQTABO
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

ifdef GANS
CXXFLAGS+=-D_GANS
OB+=EC/rans.o EC/head_cbloom.o
endif

ifdef RECIPARITH
CXXFLAGS+=-D_RECIPARITH
OB+=EC/recip_arith_/reciparith.o
endif

ifneq ($(wildcard EC/sserangecoding/.),)
EC/sserangecoding/sserangecoder.o: EC/sserangecoding/sserangecoder.cpp
	$(CXX) -c -O3 $(CFLAGS) -march=corei7-avx -mtune=corei7-avx -mno-aes EC/sserangecoding/sserangecoder.cpp -o EC/sserangecoding/sserangecoder.o 

CXXFLAGS+=-D_SSERC
OB+=EC/sserangecoding/sserangecoder.o
endif

ifneq ($(wildcard EC/subotin/.),)
CXXFLAGS+=-D_SUBOTIN
OB+=EC/subotin_/subotin.o
endif

ifneq ($(wildcard Turbo-Range-Coder/.),)
#ifeq ($(ANS), 1)
CFLAGS+=-D_ANS
TRC=Turbo-Range-Coder/
$(TRC)anscdf0.o: $(TRC)anscdf.c $(TRC)anscdf_.h
	$(CC) -c -O3 $(CFLAGS) $(_SCALAR) -falign-loops=32 $(TRC)anscdf.c -o $(TRC)anscdf0.o  

$(TRC)anscdfs.o: $(TRC)anscdf.c $(TRC)anscdf_.h
	$(CC) -c -O3 $(CFLAGS) $(SSE) -falign-loops=32 $(TRC)anscdf.c -o $(TRC)anscdfs.o  

OB+=$(TRC)anscdfs.o 
ifeq ($(ARCH), x86_64)
$(TRC)anscdfx.o: $(TRC)anscdf.c $(TRC)anscdf_.h
	$(CC) -c -O3 $(CFLAGS) -march=haswell -falign-loops=32 $(TRC)anscdf.c -o $(TRC)anscdfx.o

OB+=$(TRC)anscdfx.o 
#$(TRC)anscdf0.o
#endif
endif


CXXFLAGS+=-D_TURBORC
#-D_ANS
CFLAGS+=-D_BWT -ITurbo-Range-Coder/libsais/include
ifdef LZTURBO
CFLAGS+=-D_NCPUISA -D_NQUANT
endif
OB+=Turbo-Range-Coder/rc_ss.o Turbo-Range-Coder/rc_s.o Turbo-Range-Coder/rccdf.o Turbo-Range-Coder/rcutil.o Turbo-Range-Coder/bec_b.o Turbo-Range-Coder/rccm_s.o Turbo-Range-Coder/rccm_ss.o \
  Turbo-Range-Coder/rcqlfc_s.o Turbo-Range-Coder/rcqlfc_ss.o Turbo-Range-Coder/rcqlfc_sf.o Turbo-Range-Coder/rcbwt.o Turbo-Range-Coder/libsais/src/libsais16.o 
#Turbo-Range-Coder/anscdf0.o   
LIBSAIS=1
endif

ifneq ($(wildcard EC/vecrc/.),)
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
ifneq ($(wildcard Turbo-Run-Length-Encoding/.),)
CXXFLAGS+=-D_TURBORLE

TRLEDIR = Turbo-Run-Length-Encoding
BUILD_TRLE = $(CC) -O3 $(TRLE_FLAGS) $< -c -o $@

$(TRLEDIR)/%.o: TRLE_FLAGS = $(_AVX2) -w -fstrict-aliasing -falign-loops=32 $(DEBUG)
$(TRLEDIR)/%.o: $(TRLEDIR)/%.c ; $(BUILD_TRLE)

OB+=$(TRLEDIR)/trlec.o $(TRLEDIR)/trled.o

CXXFLAGS+=-D_MRLE
OB+=Turbo-Run-Length-Encoding/ext/mrle.o
endif

ifneq ($(wildcard hypersonic-rle-kit/.),)
CXXFLAGS+=-D_HRLE
HRLE=hypersonic-rle-kit

$(HRLE)/src/simd_platform.o: $(HRLE)/src/simd_platform.c
	$(CC) -O2 -mxsave $(MARCH) $(CFLAGS) $< -c -o $@

OB+=$(HRLE)/src/rle_sh.o $(HRLE)/src/rle8_extreme_cpu.o $(HRLE)/src/rle8_low_entropy_cpu.o $(HRLE)/src/rle8_low_entropy_short_cpu.o $(HRLE)/src/rle8_mmtf.o \
  $(HRLE)/src/rle24_extreme_cpu.o $(HRLE)/src/rle48_extreme_cpu.o $(HRLE)/src/rle128_extreme_cpu.o \
  $(HRLE)/src/rleX_extreme_cpu.o $(HRLE)/src/simd_platform.o $(HRLE)/src/rle8_mmtf.o
endif

#-------------------------------------- Archived ----------------------------------
ifneq ($(wildcard chameleon/.),)
CXXFLAGS+=-D_CHAMELEON
OB+=chameleon/chameleon.o
endif

ifneq ($(wildcard density/.),)
CXXFLAGS+=-D_DENSITY
OB+=density/src/buffers/buffer.o density/src/algorithms/algorithms.o density/src/algorithms/dictionaries.o density/src/structure/header.o density/src/globals.o density/src/buffers/buffer.o \
	density/src/algorithms/chameleon/core/chameleon_decode.o density/src/algorithms/chameleon/core/chameleon_encode.o \
	density/src/algorithms/lion/core/lion_decode.o density/src/algorithms/lion/core/lion_encode.o density/src/algorithms/lion/forms/lion_form_model.o \
	density/src/algorithms/cheetah/core/cheetah_decode.o density/src/algorithms/cheetah/core/cheetah_encode.o
endif

ifneq ($(wildcard xpack/.),)
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

ifneq ($(wildcard pithy/.),)
CXXFLAGS+=-D_PITHY
pithy/pithy.o: pithy/pithy.c
	$(CC) -O2 $(MARCH) $(CFLAGS)  $< -c -o $@
OB+=pithy/pithy.o
endif

ifneq ($(wildcard shrinker/.),)
CXXFLAGS+=-D_SHRINKER
shrinker/shrinker.o: shrinker/shrinker.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@
OB+=shrinker/Shrinker.o
endif

ifneq ($(wildcard wlfz/.),)
CXXFLAGS+=-D_WFLZ
wflz/wfLZ.o: wflz/wfLZ.c
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@
OB+=wflz/wfLZ.o
endif

ifneq ($(wildcard nakamichi/.),)
CXXFLAGS+=-D_NAKAMICHI
nakamichi/Nakamichi_Okamigan.o: nakamichi/Nakamichi_Okamigan.c
	$(CC) -O3 -msse4.1 $(MARCH) $< -c -o $@
OB+=nakamichi/Nakamichi_Washigan.o
endif

ifneq ($(wildcard FastLZ/.),)
CXXFLAGS+=-D_FASTLZ
OB+=FastLZ/fastlz.o
endif

ifneq ($(wildcard heatshrink_/.),)
CXXFLAGS+=-D_HEATSHRINK
OB+=heatshrink_/heatshrink.o heatshrink/heatshrink_encoder.o heatshrink/heatshrink_decoder.o
endif

ifneq ($(wildcard liblzf/.),)
CXXFLAGS+=-D_LIBLZF
OB+=liblzf/lzf_c.o liblzf/lzf_c_best.o liblzf/lzf_d.o
endif

ifneq ($(wildcard liblzg/.),)
CXXFLAGS+=-D_LIBLZG
OB+=liblzg/src/lib/encode.o liblzg/src/lib/decode.o liblzg/src/lib/checksum.o
endif

ifneq ($(wildcard shoco/.),)
CXXFLAGS+=-D_SHOCO
OB+=shoco/shoco.o
endif

ifneq ($(wildcard smaz/.),)
CXXFLAGS+=-D_SMAZ
OB+=smaz/smaz.o
endif

ifneq ($(wildcard yappy/.),)
CXXFLAGS+=-D_YAPPY
OB+=yappy/yappy.o
endif

ifneq ($(wildcard zpaq/.),)
ifeq ($(OS),$(filter $(OS),Darwin))
else
CXXFLAGS+=-D_ZPAQ
OB+=zpaq/libzpaq.o
endif
endif

ifneq ($(wildcard zxc/.),)
CXXFLAGS+=-D_ZXC -DZXC_STATIC_DEFINE
CFLAGS+=-Izxc/src/lib/vendors -DZXC_STATIC_DEFINE

OB+= $(ZXCDIR)/zxc_common.o $(ZXCDIR)/zxc_driver.o $(ZXCDIR)/zxc_dispatch.o $(ZXCDIR)/zxc_compress_default.o $(ZXCDIR)/zxc_decompress_default.o $(ZXCDIR)/zxc_huffman_default.o $(ZXCDIR)/zxc_pstream.o

#from lzbench 
ZXCDIR = zxc/src/lib
OB+= $(ZXCDIR)/zxc_common.o $(ZXCDIR)/zxc_driver.o $(ZXCDIR)/zxc_dispatch.o $(ZXCDIR)/zxc_compress_default.o  $(ZXCDIR)/zxc_seekable.o $(ZXCDIR)/zxc_decompress_default.o $(ZXCDIR)/zxc_pivco_tables.o 

  ifneq (,$(filter x86_64% amd64% i%86%,$(ARCH)))
    ifneq (,$(filter x86_64% amd64%,$(ARCH)))
    OB += $(ZXCDIR)/zxc_compress_sse2.o $(ZXCDIR)/zxc_decompress_sse2.o
    OB += $(ZXCDIR)/zxc_compress_avx2.o $(ZXCDIR)/zxc_decompress_avx2.o
    OB += $(ZXCDIR)/zxc_compress_avx512.o $(ZXCDIR)/zxc_decompress_avx512.o
    OB += $(ZXCDIR)/zxc_huffman_sse2.o $(ZXCDIR)/zxc_huffman_sse2.o
    OB += $(ZXCDIR)/zxc_huffman_avx2.o $(ZXCDIR)/zxc_huffman_avx2.o
    OB += $(ZXCDIR)/zxc_huffman_avx512.o $(ZXCDIR)/zxc_huffman_avx512.o
    OB += $(ZXCDIR)/zxc_dict_sse2.o $(ZXCDIR)/zxc_dict_sse2.o
    endif
  else
    ifneq (,$(filter arm% aarch64%,$(ARCH)))
    OB += $(ZXCDIR)/zxc_compress_neon.o $(ZXCDIR)/zxc_decompress_neon.o
    OB += $(ZXCDIR)/zxc_huffman_neon.o $(ZXCDIR)/zxc_huffman_neon.o
    OB += $(ZXCDIR)/zxc_dict_neon.o $(ZXCDIR)/zxc_dict_neon.o
        
      ifneq (,$(filter arm64% aarch64%,$(ARCH)))
      NEON_FLAGS = -DZXC_USE_NEON64
      else
      NEON_FLAGS = -march=armv7-a -mfloat-abi=softfp -mfpu=neon -DZXC_USE_NEON32
      endif
    endif  
  endif

CMD_BUILD_ZXC = $(CC) -O3 $(CFLAGS) -I$(ZXCDIR)/vendors $(ZXC_FLAGS) $< -c -o $@

$(ZXCDIR)/%.o: $(ZXCDIR)/%.c ; $(CMD_BUILD_ZXC)

$(ZXCDIR)/%_default.o: ZXC_FLAGS = -DZXC_FUNCTION_SUFFIX=_default
$(ZXCDIR)/%_default.o: $(ZXCDIR)/%.c ; $(CMD_BUILD_ZXC)

$(ZXCDIR)/%_sse2.o: ZXC_FLAGS = -msse2 -DZXC_FUNCTION_SUFFIX=_sse2 -DZXC_USE_SSE2
$(ZXCDIR)/%_sse2.o: $(ZXCDIR)/%.c ; $(CMD_BUILD_ZXC)

$(ZXCDIR)/%_avx2.o: ZXC_FLAGS = -mavx2 -mbmi2 -DZXC_FUNCTION_SUFFIX=_avx2 -DZXC_USE_AVX2
$(ZXCDIR)/%_avx2.o: $(ZXCDIR)/%.c ; $(CMD_BUILD_ZXC)

$(ZXCDIR)/%_avx512.o: ZXC_FLAGS = -mavx512f -mavx512bw -mbmi2 -DZXC_FUNCTION_SUFFIX=_avx512 -DZXC_USE_AVX512
$(ZXCDIR)/%_avx512.o: $(ZXCDIR)/%.c ; $(CMD_BUILD_ZXC)

$(ZXCDIR)/%_default.o: ZXC_FLAGS = -DZXC_FUNCTION_SUFFIX=_default
$(ZXCDIR)/%_default.o: $(ZXCDIR)/%.c ; $(CMD_BUILD_ZXC)

$(ZXCDIR)/%_avx2.o: ZXC_FLAGS = -mavx2 -mbmi2 -DZXC_FUNCTION_SUFFIX=_avx2 -DZXC_USE_AVX2
$(ZXCDIR)/%_avx2.o: $(ZXCDIR)/%.c ; $(CMD_BUILD_ZXC)

$(ZXCDIR)/%_avx512.o: ZXC_FLAGS = -mavx512f -mavx512bw -mbmi2 -DZXC_FUNCTION_SUFFIX=_avx512 -DZXC_USE_AVX512
$(ZXCDIR)/%_avx512.o: $(ZXCDIR)/%.c ; $(CMD_BUILD_ZXC)


$(ZXCDIR)/%_neon.o: ZXC_FLAGS = $(NEON_FLAGS) -DZXC_FUNCTION_SUFFIX=_neon
$(ZXCDIR)/%_neon.o: $(ZXCDIR)/%.c ; $(CMD_BUILD_ZXC)
endif

ifneq ($(wildcard CSC/.),)
CXXFLAGS+=-D_CSC
OB+=CSC/src/libcsc/csc_analyzer.o CSC/src/libcsc/csc_coder.o CSC/src/libcsc/csc_dec.o CSC/src/libcsc/csc_default_alloc.o CSC/src/libcsc/csc_enc.o CSC/src/libcsc/csc_encoder_main.o CSC/src/libcsc/csc_filters.o CSC/src/libcsc/csc_lz.o CSC/src/libcsc/csc_memio.o \
	CSC/src/libcsc/csc_mf.o CSC/src/libcsc/csc_model.o CSC/src/libcsc/csc_profiler.o
endif

ifneq ($(wildcard doboz/.),)
CXXFLAGS+=-D_DOBOZ
OB+=doboz/Source/Doboz/Compressor.o doboz/Source/Doboz/Decompressor.o doboz/Source/Doboz/Dictionary.o
endif

ifneq ($(wildcard libzling/.),)
CXXFLAGS+=-D_LIBZLING
# Disabled : compile error in gcc 7.2
#OB+=libzling/src/libzling.o libzling/src/libzling_huffman.o libzling/src/libzling_utils.o libzling/src/libzling_lz.o libzling_/libzling_utils_mem.o
endif

ifneq ($(wildcard Behemoth-Rank-Coding/.),)
CXXFLAGS+=-D_BRC
OB+=Behemoth-Rank-Coding/brc.o
endif
#----------------------- GPL -------------------------
ifneq ($(wildcard lzmat/.),)
CXXFLAGS+=-DLZMAT
lzmat/lzmat_dec.o: lzmat/lzmat_dec.c
	$(CC) -O2 -D"__int64=long long" $(MARCH) $(CFLAGS) $< -c -o $@
lzmat/lzmat_enc.o: lzmat/lzmat_enc.c
	$(CC) -O2 -D"__int64=long long" $(MARCH) $(CFLAGS) $< -c -o $@

OB+=lzmat/lzmat_enc.o lzmat/lzmat_dec.o
endif

ifneq ($(wildcard tornado/.),)
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

ifneq ($(wildcard ms-compress/.),)
CXXFLAGS+=-D_MSCOMPRESS
OB+=ms-compress/src/mscomp.o ms-compress/src/lznt1_compress.o ms-compress/src/lznt1_decompress.o ms-compress/src/xpress_compress.o ms-compress/src/xpress_decompress.o ms-compress/src/xpress_huff_compress.o ms-compress/src/xpress_huff_decompress.o
endif

ifneq ($(wildcard quicklz_/.),)
CXXFLAGS+=-D_QUICKLZ
OB+=quicklz_/quicklz1.o quicklz_/quicklz2.o quicklz_/quicklz3.o
endif

ifneq ($(wildcard pysap/.),)
CXXFLAGS+=-D_PYSAP
OB+=pysap/pysapcompress/vpa105CsObjInt.o pysap/pysapcompress/vpa106cslzc.o pysap/pysapcompress/vpa107cslzh.o pysap/pysapcompress/vpa108csulzh.o
endif

# PivCo-Huffman (https://github.com/MarcinZukowski/pivco-huffman): SIMD tree-walk
# Huffman, levels 1=PH, 2=PHA.  Submodule built via its own CMake; we link the
# pre-localized object (libpivco_huffman_local.o) whose vendored FSE_*/HUF_*
# symbols are localized so they don't clash with the zstd TurboBench bundles.
# The submodule has its own submodule (ext/fse), so init recursively:
#   git submodule update --init --recursive pivco-huffman
ifneq ($(wildcard pivco-huffman/.),)
PIVCOHUFDIR=pivco-huffman
CXXFLAGS+=-D_PIVCOHUF=1 -I$(PIVCOHUFDIR)/include
$(PIVCOHUFDIR)/build/libpivco_huffman_local.o:
	cmake -S $(PIVCOHUFDIR) -B $(PIVCOHUFDIR)/build -DCMAKE_BUILD_TYPE=Release
	cmake --build $(PIVCOHUFDIR)/build --target pivco_huffman_local -j
OB+=$(PIVCOHUFDIR)/build/libpivco_huffman_local.o

# PHAZ: PivCo-Huffman entropy transplant onto zstd (full LZ+entropy compressor;
# level = zstd level).  Built from the pivco-huffman submodule's extras/phaz via
# its own build.sh: it patches a *private* copy of zstd source (pointed at
# TurboBench's own zstd/ submodule, same pinned SHA 5233c58e) and merges it +
# pivco into one blob (phaz_local.o) that exports only phaz_compress /
# phaz_decompress -- everything else (all of zstd, FSE/HUF, pivco) is localized,
# so it coexists with the vanilla zstd TurboBench links.  Requires:
#   git submodule update --init --recursive pivco-huffman zstd
ifeq ($(PHAZ), 1)
PIVCOHUFDIR=pivco-huffman
PHAZDIR=$(PIVCOHUFDIR)/extras/phaz
CXXFLAGS+=-D_PHAZ=1
$(PHAZDIR)/build/phaz_local.o:
	cmake -S $(PIVCOHUFDIR) -B $(PIVCOHUFDIR)/build -DCMAKE_BUILD_TYPE=Release
	cmake --build $(PIVCOHUFDIR)/build --target pivco_huffman_local -j
	ZSTD_SRC=$(abspath zstd) MARCH="$(MARCH)" CC=$(CC) bash $(PHAZDIR)/tools/build.sh
OB+=$(PHAZDIR)/build/phaz_local.o
endif
endif
#--------------------------------------------------------------------
CFLAGS+=$(DDEBUG) -w -std=gnu99 -fpermissive -Wall
CXXFLAGS+=$(DDEBUG) -w -fpermissive -Wall -fno-rtti

OB+=$(ICL) $(HUF) $(ANS) $(LZ) plugin.o

turbobench: $(OB) turbobench.o
	$(CXX) $^ $(LDFLAGS) -o turbobench

.c.o:
	$(CC) -O3 $(MARCH) $(CFLAGS) $< -c -o $@

.cc.o:
	$(CXX) -O3 $(MARCH) $(CXXFLAGS)  $< -c -o $@

.cpp.o:
	$(CXX) -O3 $(MARCH) $(CXXFLAGS) $< -c -o $@

.S.o:
	$(CC) -O3 $(MARCH) $(CFLAGS) $< -c -o $@


ifeq ($(OS),Windows)
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

