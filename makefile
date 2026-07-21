# powturbo  (c) Copyright 2013-2026
# ----------- Downloading + Compiling -----------------------
# git clone --recursive git://github.com/powturbo/TurboBench.git
# make
#
#----------------
# Cross compile: export CROSS to aarch64 riscv64 loongarch64 or powerpc64le. Ex.:
# export CROSS=aarch64
# Testing with qemu
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./turbobench .l2
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

BUILDIR := build
obj = $(addprefix $(BUILDIR)/,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(patsubst %.cpp,%.o,$(patsubst %.S,%.o,$(1))))))

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
#CROSS:=$(CC)
endif
CHOST=$(CROSS)

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
  CFLAGS=$(_SSE)
else ifeq ($(ARCH),riscv64)
#  CFLAGS=-march=rv64gc_zba_zbb_zbs
  CFLAGS=-march=rv64gcv_zvbb 
  CFLAGS+=-mabi=lp64d -mno-strict-align
else ifeq ($(ARCH),ppc64le)
  _SSE=-D__SSE4_1__
  CFLAGS=-mcpu=power9 -mtune=power9 $(_SSE)
  CHOST=powerpc64le
else ifeq ($(ARCH),loongarch64)
  _SSE=-mlsx
  CFLAGS=$(_SSE)
else ifeq ($(ARCH),x86_64)
  _SSE=-mavx -mpopcnt
  _AVX2=-march=haswell
endif

ifeq ($(OS),Windows)
  LDFLAGS=-Wl,--stack,33554432 -lpowrprof
endif

CFLAGS+=-w -Wall $(DDEBUG) -std=gnu99 -fpermissive -Wimplicit-function-declaration
CXXFLAGS+=$(DDEBUG) -w -Wall -fpermissive  -fno-rtti

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
else ifeq ($(OS),$(filter $(OS),Darwin FreeBSD GNU/kFreeBSD Linux NetBSD SunOS))
LDFLAGS += -ldl
endif

ifdef OPENMP
CFLAGS+=-fopenmp -DLIBBSC_OPENMP_SUPPORT
LDFLAGS+=-fopenmp
endif

all: turbobench 
 
ifdef LZTURBO
CXXFLAGS+=-D_LZTURBO
include ../dev/x/lzturbo.mk
endif

# ***************************************************************** codecs *****************************************************************************
ifneq ($(wildcard brotli/.),)
CXXFLAGS+=-D_BROTLI -Ibrotli/c/include 
CFLAGS+=-Ibrotli/c/include 
BROTLI_SRCS := $(wildcard brotli/c/common/*.c) $(wildcard brotli/c/dec/*.c) $(wildcard brotli/*.c) $(wildcard brotli/c/enc/*.c)
OB += $(call obj,$(BROTLI_SRCS))
endif

ifneq ($(wildcard bzip2/.),)
CXXFLAGS+=-D_BZIP2
OB += $(call obj,bzip2/blocksort.o bzip2/huffman.o bzip2/crctable.o bzip2/randtable.o bzip2/compress.o bzip2/decompress.o bzip2/bzlib.o)
endif

ifneq ($(wildcard bzip3/.),)
CXXFLAGS+=-D_BZIP3
CFLAGS+=-DVERSION=1 -Ibzip3/include -Wno-int-conversion
OB+=$(call obj,bzip3/src/libbz3.o)
endif

C_BLOSC2_LIB :=
ifneq ($(wildcard c-blosc2/.),)
#ifneq ($(OS), Windows)  # not working under windows
C_BLOSC2_SRCS := $(shell find c-blosc2 -type f -name '*.[c]' -o -name '*.cpp' -o -name '*.cc')
ifdef CROSS # no cross compile
#c-blosc2/blosc/libblosc2.a: $(C_BLOSC2_SRCS)
#	(export CC=$(CROSS)-linux-gnu-gcc && cd c-blosc2 && cmake . -DBLOSC_ZSTD_SOURCE_DIR=../zstd && $(MAKE))
else
CXXFLAGS += -D_C_BLOSC2
C_BLOSC2_LIB = $(BUILDIR)/c-blosc2/blosc/libblosc2.a
$(C_BLOSC2_LIB): $(C_BLOSC2_SRCS)
	cmake -S c-blosc2 -B $(BUILDIR)/c-blosc2 -DBLOSC_ZSTD_SOURCE_DIR=zstd -DBUILD_TESTS=OFF -DBUILD_BENCHMARKS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_FUZZERS=OFF -DBUILD_SHARED=OFF
	cmake --build $(BUILDIR)/c-blosc2
LDFLAGS += $(C_BLOSC2_LIB)
endif
#endif
endif

ISAL_LIB :=
ifneq ($(wildcard isa-l/.),)
NASM ?= $(shell command -v nasm)
ifeq ($(NASM),)  # nasm not installed
  ifneq ($(wildcard isa-l_/$(OS)-$(ARCH)/isa-l.a),)
    CXXFLAGS += -D_ISA_L
    ISAL_LIB := isa-l_/$(OS)-$(ARCH)/isa-l.a    
  endif     
else
  CXXFLAGS += -D_ISA_L
  ISAL_SRCS := $(shell find isa-l -type f -name '*.[c]' -o -name '*.cpp' -o -name '*.cc')
  ISAL_LIB := isa-l/bin/isa-l.a 
  ifdef CROSS
$(ISAL_LIB): $(ISAL_SRCS)
	export CC=$(CROSS)-linux-gnu-gcc && cd isa-l && $(MAKE) -f Makefile.unx host_cpu=$(CHOST)
  else
$(ISAL_LIB): $(ISAL_SRCS)
	mkdir -p $(BUILDIR)/isa-l
	mkdir -p isa-l/bin
	$(MAKE) -C isa-l -f Makefile.unx O=$(abspath $(BUILDIR)/isa-l)
  endif
endif
LDFLAGS += $(ISAL_LIB)
endif

ifneq ($(wildcard GLZA/.),)
CXXFLAGS+=-D_GLZA
GLZA_OBJS := $(call obj,GLZA/GLZAmodel.o GLZA/GLZAcomp.o GLZA/GLZAencode.o GLZA/GLZAcompress.o GLZA/GLZAformat.o GLZA/GLZAdecode.o)
GLZA_BUILD = $(CC) -O2 $(CFLAGS) $< -c -o $@
$(GLZA_DIR)/%.o: GLZA/%.c
	$(GLZA_BUILD)
OB += $(GLZA_OBJS) 
endif

ifneq ($(wildcard kanzi-cpp/.),)
CXXFLAGS+=-D_KANZI
KANZI_DIR = kanzi-cpp/src
KANZI_SRCS := $(wildcard $(KANZI_DIR)/io/*.cpp) $(wildcard $(KANZI_DIR)/entropy/*.cpp) $(wildcard $(KANZI_DIR)/bitstream/*.cpp) $(wildcard $(KANZI_DIR)/*.cpp) $(wildcard $(KANZI_DIR)/transform/*.cpp)
OB += $(call obj,$(KANZI_SRCS))
endif

ifneq ($(wildcard libbsc/.),)
CXXFLAGS+=-D_LIBBSC -DLIBBSC_SORT_TRANSFORM_SUPPORT -ICSC/src/libcsc
OB+=$(call obj,libbsc/libbsc/libbsc/libbsc.o libbsc/libbsc/coder/coder.o libbsc/libbsc/coder/qlfc/qlfc.o libbsc/libbsc/coder/qlfc/qlfc_model.o libbsc/libbsc/filters/detectors.o \
	libbsc/libbsc/filters/preprocessing.o libbsc/libbsc/adler32/adler32.o libbsc/libbsc/bwt/bwt.o libbsc/libbsc/st/st.o libbsc/libbsc/lzp/lzp.o)
OB+=$(call obj,libbsc/libbsc/platform/platform.o libbsc/libbsc/bwt/libsais/libsais.o libbsc/libbsc/bwt/libsais/libsais.o)
LIBSAIS=1
endif

ifneq ($(wildcard libdeflate/.),)
CXXFLAGS+=-D_LIBDEFLATE
CFLAGS+=-Ilibdeflate -Ilibdeflate/common
LIBDEFLATE_SRCS := $(wildcard libdeflate/lib/*.c) libdeflate/lib/arm/cpu_features.c libdeflate/lib/x86/cpu_features.c 
OB += $(call obj,$(LIBDEFLATE_SRCS))
endif

ifneq ($(wildcard libslz/.),)
CXXFLAGS+=-D_LIBSLZ
OB+=$(call obj,libslz/src/slz.o)
endif

ifneq ($(wildcard lizard/.),)
CFLAGS+=-Ilizard/lib
LIZARD_SRCS := $(wildcard lizard/lib/*.c) $(wildcard lizard/lib/entropy/*.c) 
LIZARD_SRCS := $(filter-out %/debug.c, $(LIZARD_SRCS))
OB += $(call obj,$(LIZARD_SRCS))
endif

ifneq ($(wildcard lz4/.),)
CXXFLAGS+=-D_LZ4
CFLAGS+=-Ilz4/lib
OB+=$(call obj,lz4/lib/lz4hc.o lz4/lib/lz4.o lz4/lib/lz4frame.o lz4/lib/xxhash.o)
endif

ifneq ($(wildcard lzav/.),)
CXXFLAGS+=-D_LZAV
endif

ifneq ($(wildcard lzfse/.),)
CXXFLAGS+=-D_LZFSE
OB+=$(call obj,lzfse/src/lzfse_decode_base.o lzfse/src/lzfse_decode.o lzfse/src/lzfse_encode_base.o lzfse/src/lzfse_encode.o lzfse/src/lzfse_fse.o lzfse/src/lzvn_decode_base.o lzfse/src/lzvn_encode_base.o)
endif

#ifneq ($(and $(wildcard lzham_codec_devel/.),$(filter x86_64,$(ARCH))),)
ifneq ($(wildcard lzham_codec_devel/.),)
ifneq ($(OS),$(filter $(OS),Darwin))
CXXFLAGS+=-D_LZHAM -D"UINT64_MAX=-1ull" -Ilzham_codec_devel/include -Ilzham_codec_devel/lzhamcomp -Ilzham_codec_devel/lzhamdecomp
LZHAM_SRCS := $(wildcard lzham_codec_devel/lzhamcomp/*.cpp) $(wildcard lzham_codec_devel/lzhamdecomp/*.cpp) $(wildcard lzham_codec_devel/lzhamlib/*.cpp)
LZHAM_SRCS := $(filter-out %/lzham_win32_threading.cpp, $(LZHAM_SRCS))
OB += $(call obj,$(LZHAM_SRCS))
ifeq ($(OS), Windows)
OB += $(call obj,lzham_codec_devel/lzhamcomp/lzham_win32_threading.o)
else
OB += += lz/lzham/lzhamcomp/lzham_pthreads_threading.o
CXXFLAGS+=-DTHREAD_MODEL_POSIX
endif
endif
endif

ifneq ($(wildcard lzlib-1.16/.),)
CXXFLAGS+=-D_LZLIB
OB+=$(call obj,lzlib-1.16/lzlib.o lzlib_/bbexample.o)
endif

ifneq ($(wildcard lzma/.),)
CXXFLAGS+=-D_LZMA -D_7Z_TYPES_
CFLAGS+=-D_7ZIP_ST
OB+=$(call obj,lzma/C/Alloc.o lzma/C/CpuArch.o lzma/C/LzFind.o lzma/C/LzmaDec.o lzma/C/LzmaEnc.o lzma/C/LzmaLib.o lzma/C/Threads.o lzma/C/LzFindMt.o lzma/C/LzFindOpt.o)
endif

ifneq ($(wildcard lzo/.),)
CXXFLAGS+=-D_LZO -Ilzo/include
CFLAGS+=-Ilzo/include
LZO_SRCS := $(wildcard lzo/src/*.c)
OB += $(call obj,$(LZO_SRCS))
endif

ifneq ($(and $(wildcard LZSSE/.),$(filter x86_64,$(ARCH))),)
CXXFLAGS += -D_LZSSE
OB += $(addprefix $(BUILDIR)/LZSSE/, lzsse2/lzsse2.o lzsse4/lzsse4.o lzsse8/lzsse8.o)
$(BUILDIR)/LZSSE/%.o: LZSSE/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -O2 -msse4.1 -std=c++11 $< -c -o $@
endif

ifneq ($(wildcard memlz/.),)
CXXFLAGS+=-D_MEMLZ
endif

ifneq ($(wildcard miniz/.),)
miniz/miniz_export.h: miniz_/miniz_export.h
	cp miniz_/miniz_export.h miniz/miniz_export.h
$(BUILDIR)/miniz/miniz.o: miniz/miniz.c miniz/miniz_export.h
	@mkdir -p $(dir $@)
	$(CC) -O3 $(MARCH) $(CFLAGS) $< -c -o $@
CXXFLAGS+=-D_MINIZ
OB+=$(BUILDIR)/miniz/miniz.o $(call obj,miniz/miniz_tdef.o miniz/miniz_tinfl.o)
endif

ifneq ($(wildcard misa77/.),)
CXXFLAGS+=-D_MISA77
MISA77_DIR = misa77
MISA77_SRCS := $(wildcard $(MISA77_DIR)/src/*.cpp) $(wildcard $(MISA77_DIR)/src/experimental/*.cpp)  $(MISA77_DIR)/src/isa/target_portable.o $(MISA77_DIR)/src/experimental/isa/etarget_portable.o
MMISA77_BUILD = $(CXX) -O3 $(CXXFLAGS) -std=c++20 -I$(MISA77_DIR)/include -I$(MISA77_DIR)/src $(MISA77_FLAGS) $< -c -o $@
$(BUILDIR)/$(MISA77_DIR)/src/%_sse2.o: CXXFLAGS += $(_SSE)
$(BUILDIR)/$(MISA77_DIR)/src/%_avx2.o: CXXFLAGS += $(_AVX2)
$(BUILDIR)/$(MISA77_DIR)/src/%.o: $(MISA77_DIR)/src/%.cpp
	@mkdir -p $(dir $@)
	$(MISA77_BUILD)
$(BUILDIR)/$(MISA77_DIR)/src/experimental/isa/%.o: $(MISA77_DIR)/src/experimental/%.cpp
	@mkdir -p $(dir $@)
	$(MISA77_BUILD)

OB += $(call obj,$(MISA77_SRCS))
ifeq ($(ARCH),x86_64)
  OB += $(BUILDIR)/$(MISA77_DIR)/src/isa/target_sse2.o  $(BUILDIR)/$(MISA77_DIR)/src/experimental/isa/etarget_sse2.o 
  OB += $(BUILDIR)/$(MISA77_DIR)/src/isa/target_avx2.o $(BUILDIR)/$(MISA77_DIR)/src/experimental/isa/etarget_avx2.o
else ifeq ($(ARCH),aarch64)
  OB += $(BUILDIR)/$(MISA77_DIR)/src/isa/target_neon.o
endif
endif

OPENZL_LIB :=
ifneq ($(wildcard openzl/.),)
OPENZL_SRCS := $(shell find openzl -type f -name '*.[c]' -o -name '*.cpp' -o -name '*.cc')
ifdef CROSS #not working
#openzl/libopenzl.a: $(OPENZL_SRCS)
#	export CC=$(CROSS)-linux-gnu-gcc && cd openzl && $(MAKE) lib
else
CXXFLAGS += -D_OPENZL -Iopenzl/include
ifeq ($(OS), Windows)
OPENZL_LIB = openzl/libopenzl.a
$(OPENZL_LIB): $(OPENZL_SRCS)
	cd openzl && $(MAKE) lib
#	mkdir -p openzl && $(MAKE) -C $(BUILDIR)/openzl lib
else
OPENZL_LIB = $(BUILDIR)/openzl/libopenzl.a
$(OPENZL_LIB): $(OPENZL_SRCS)
	cmake -S openzl -B $(BUILDIR)/openzl
	cmake --build $(BUILDIR)/openzl --config Release
endif
LDFLAGS += $(OPENZL_LIB)
endif
endif

# 'oo2core_9_win64.dll', 'liboo2corelinuxarm64.so.9' or 'liboo2corelinux64.so.9' must be in the same directory as turbobench[.exe]
# download corresponding library from https://github.com/WorkingRobot/OodleUE
CXXFLAGS+=-D_OODLE

ifneq ($(wildcard pcodec_/.),)
  CXXFLAGS += -D_PCODEC
endif

ifneq ($(wildcard skim/.),)
CXXFLAGS+=-D_MEMLZ
LDFLAGS += skim/libskim.a
endif

ifneq ($(wildcard snappy/.),)
# configure or copy directory "snappy_/*" to "snappy"
ifneq (,$(wildcard snappy/snappy-stubs-public.h))
CXXFLAGS+=-D_SNAPPY
OB+=$(call obj,snappy/snappy-sinksource.o snappy/snappy-stubs-internal.o snappy/snappy.o)
endif
endif

ifneq ($(wildcard tamp/.),)
CXXFLAGS+=-D_TAMP
TAMP_DIR = tamp/tamp/_c_src/tamp
OB += $(call obj,$(TAMP_DIR)/common.o $(TAMP_DIR)/compressor.o $(TAMP_DIR)/decompressor.o)
endif

ZLIB_NG_LIB :=
ifneq ($(wildcard zlib-ng/.),)
CXXFLAGS += -D_ZLIB_NG
ZLIB_NG_SRCS := $(shell find zlib-ng -type f -name '*.[c]' -o -name '*.cpp' -o -name '*.cc')
ZLIB_NG_LIB = $(BUILDIR)/zlib-ng/libz-ng.a
ifdef CROSS
$(ZLIB_NG_LIB): $(ZLIB_NG_SRCS)
	export CC=$(CROSS)-linux-gnu-gcc && cd zlib-ng && ./configure && $(MAKE)
else
$(ZLIB_NG_LIB): $(ZLIB_NG_SRCS)
	cmake -S zlib-ng -B $(BUILDIR)/zlib-ng
	cmake --build $(BUILDIR)/zlib-ng --config Release
	cp $(BUILDIR)/zlib-ng/zconf-ng.h zlib-ng_
endif
LDFLAGS += $(ZLIB_NG_LIB)
endif

ifneq ($(wildcard zopfli/.),)
CXXFLAGS+=-D_ZOPFLI
ZOPFLI_SRCS := $(wildcard zopfli/src/zopfli/*.c) 
ZOPFLI_SRCS := $(filter-out %/zopfli_bin.c, $(ZOPFLI_SRCS))
OB += $(call obj,$(ZOPFLI_SRCS))
endif

ifneq ($(wildcard zstd/.),)
CXXFLAGS+=-D_ZSTD -Izstd/lib -Izstd/lib/common
CFLAGS+=-Izstd/lib -Izstd/lib/common
ZSTD_SRCS := $(wildcard zstd/lib/common/*.c) $(wildcard zstd/lib/compress/*.c) $(wildcard zstd/lib/decompress/*.c) $(wildcard zstd/lib/decompress/*.S)
# .S sources are passed directly to the linker (assembled at link time), so only the .c sources are compiled into $(BUILDIR) objects.
ZSTD_C_SRCS := $(filter %.c,$(ZSTD_SRCS))
ZSTD_S_SRCS := $(filter %.S,$(ZSTD_SRCS))
ZSTD_OBJS := $(call obj,$(ZSTD_C_SRCS)) $(ZSTD_S_SRCS)
OB += $(ZSTD_OBJS)
endif

FSE := EC/fse
ifneq ($(wildcard FSE/.),)
CXXFLAGS+=-D_FSE
OB+=$(call obj,$(LB)EC/fse/fse_compress_.o $(LB)EC/fse/fse_decompress_.o)
endif

XZ_LIB :=
ifneq ($(wildcard xz/.),)
CXXFLAGS += -D_XZ
XZ_SRCS := $(shell find xz/src/liblzma -type f -name '*.[c]' -o -name '*.cpp' -o -name '*.cc')
ifdef CROSS
$(BUILDIR)/xz/liblzma.a: $(XZ_SRCS)
	export CC=$(CROSS)-linux-gnu-gcc && cd xz && cmake ../build && $(MAKE) 
else
$(BUILDIR)/xz/liblzma.a: $(XZ_SRCS)
	cmake -S xz -B $(BUILDIR)/xz && $(MAKE) -C $(BUILDIR)/xz
endif
XZ_LIB = $(BUILDIR)/xz/liblzma.a
LDFLAGS += $(XZ_LIB)
endif

ifneq ($(wildcard zlib/.),)
CXXFLAGS+=-D_ZLIB
ZD=zlib/
OB+=$(call obj,$(ZD)adler32.o $(ZD)crc32.o $(ZD)compress.o $(ZD)deflate.o $(ZD)infback.o $(ZD)inffast.o $(ZD)inflate.o $(ZD)inftrees.o $(ZD)trees.o $(ZD)uncompr.o $(ZD)zutil.o)
endif

ifneq ($(wildcard zpaq/.),)
ifeq ($(OS),$(filter $(OS),Darwin))
else
CXXFLAGS+=-D_ZPAQ
OB+=$(call obj,zpaq/libzpaq.o)
endif
endif

ifneq ($(wildcard zxc/.),)
#ifneq (,$(filter $(ARCH),x86_64 aarch64))
CXXFLAGS += -D_ZXC -DZXC_STATIC_DEFINE
CFLAGS += -Izxc/src/lib/vendors -DZXC_STATIC_DEFINE -fPIC 
ZXCDIR = zxc/src/lib
ZXC_BUILD = $(CC) -O3 $(CFLAGS) -I$(ZXCDIR)/vendors $(ZXC_FLAGS) $< -c -o $@

define ZXC_RULE
$$(BUILDIR)/$$(ZXCDIR)/%$(1).o: ZXC_FLAGS = $(2)
$$(BUILDIR)/$$(ZXCDIR)/%$(1).o: $$(ZXCDIR)/%.c
	@mkdir -p $$(dir $$@)
	$$(ZXC_BUILD)
endef
$(eval $(call ZXC_RULE,,))
$(eval $(call ZXC_RULE,_default,-DZXC_FUNCTION_SUFFIX=_default))
$(eval $(call ZXC_RULE,_sse2,-msse2 -DZXC_FUNCTION_SUFFIX=_sse2 -DZXC_USE_SSE2))
$(eval $(call ZXC_RULE,_avx2,-mavx2 -mbmi2 -DZXC_FUNCTION_SUFFIX=_avx2 -DZXC_USE_AVX2))
$(eval $(call ZXC_RULE,_avx512,-mavx512f -mavx512bw -mbmi2 -DZXC_FUNCTION_SUFFIX=_avx512 -DZXC_USE_AVX512))
$(eval $(call ZXC_RULE,_neon,$(_SSE) -DZXC_FUNCTION_SUFFIX=_neon -DZXC_USE_NEON64))
ZXC_OBJS = common driver dispatch compress_default decompress_default huffman_default pivco_tables seekable

ifeq ($(ARCH),x86_64)
  ZXC_OBJS += $(foreach e,sse2 avx2 avx512,compress_$(e) decompress_$(e) huffman_$(e)) dict_sse2
else ifeq ($(ARCH),aarch64)
  ZXC_OBJS += $(foreach e,neon,compress_$(e) decompress_$(e) huffman_$(e) dict_$(e))
endif
OB += $(call obj,$(patsubst %,$(ZXCDIR)/zxc_%.o,$(ZXC_OBJS)))
#endif
endif
#------------------------------------ Notable codecs ---------------------------------------------------------------------------
ifneq ($(wildcard brieflz/.),)
CXXFLAGS+=-D_BRIEFLZ
CFLAGS+=-Ibrieflz/include
OB+=$(call obj,brieflz/src/brieflz.o brieflz/src/depack.o)
endif

ifneq ($(wildcard fast-lzma2/.),)
CXXFLAGS+=-D_FLZMA2
FLZMA2_SRCS := $(wildcard fast-lzma2/*.c) 
FLZMA2_SRCS := $(filter-out %/xxhash.c, $(FLZMA2_SRCS))
FLZMA2_OBJS := $(call obj,$(FLZMA2_SRCS))
OB += $(FLZMA2_OBJS)
endif

ifneq ($(wildcard xzz/.),)
XZ_DIR = xz/src/liblzma
CXXFLAGS+=-D_XZ
CFLAGS+=-Ixz/src -Ixz/src/common  -I$(XZ_DIR) -I$(XZ_DIR)/check -I$(XZ_DIR)/common -I$(XZ_DIR)/delta -I$(XZ_DIR)/simple -I$(XZ_DIR)/api -I$(XZ_DIR)/common -I$(XZ_DIR)/lzma -I$(XZ_DIR)/lz -I$(XZ_DIR)/check -I$(XZ_DIR)/rangecoder -DHAVE_CHECK_CRC32 -DMYTHREAD_POSIX
XZ_SRCS := $(wildcard $(XZ_DIR)/check/*.c) $(wildcard $(XZ_DIR)/common/*.c) $(wildcard $(XZ_DIR)/lz/*.c) $(wildcard $(XZ_DIR)/lzma/*.c) $(wildcard $(XZ_DIR)/rangecoder/*.c)
XZ_OBJS := $(call obj,$(XZ_SRCS))
OB += $(XZ_OBJS)
endif

ifneq ($(wildcard lz4ultra/.),)
CXXFLAGS+=-D_LZ4ULTRA -Ilz4ultra/src -Ilz4ultra/src/libdivsufsort/include
OB+=$(call obj,lz4ultra/src/shrink_inmem.o lz4ultra/src/expand_inmem.o lz4ultra/src/shrink_block.o lz4ultra/src/expand_block.o lz4ultra/src/shrink_context.o lz4ultra/src/matchfinder.o lz4ultra/src/frame.o)
ifeq ($(DIVSORT), 1)
else
OB+=$(call obj,lz4ultra/src/libdivsufsort/lib/divsufsort.o lz4ultra/src/libdivsufsort/lib/sssort.o lz4ultra/src/libdivsufsort/lib/trsort.o)
DIVSORT=1
endif
endif

ifneq ($(wildcard lzsa/.),)
CXXFLAGS+=-D_LZSA
CFLAGS+=-Ilzsa/src -Ilzsa/src/libdivsufsort/include
OB+=$(call obj,lzsa/src/expand_block_v1.o lzsa/src/expand_block_v2.o lzsa/src/expand_context.o lzsa/src/expand_inmem.o lzsa/src/shrink_block_v1.o lzsa/src/shrink_block_v2.o lzsa/src/shrink_inmem.o lzsa/src/shrink_context.o \
    lzsa/src/matchfinder.o lzsa/src/frame.o)
ifeq ($(DIVSORT), 1)
else
OB+=$(call obj,lzsa/src/libdivsufsort/lib/divsufsort.o lzsa/src/libdivsufsort/lib/sssort.o lzsa/src/libdivsufsort/lib/trsort.o)
DIVSORT=1
endif
endif

ifneq ($(wildcard snappy-c/.),)
CXXFLAGS+=-D_SNAPPY_C
OB+=$(call obj,snappy-c/snappy.o snappy-c/util.o)
endif

ifneq ($(wildcard gipfeli/.),)
CXXFLAGS+=-D_GIPFELI
OB+=$(call obj,gipfeli/lz77.o gipfeli/entropy.o gipfeli/entropy_code_builder.o gipfeli/decompress.o gipfeli/gipfeli-internal.o)
endif

ifneq ($(wildcard tcobs/.),)
CXXFLAGS+=-D_TCOBS -Drestrict=__restrict
OB+=$(call obj,tcobs/v2/tcobsEncode.o tcobs/v2/tcobsDecode.o)
endif

ifdef SMALLZ4
CXXFLAGS+=-DSMALLZ4
endif

ifneq ($(wildcard Unishox2/.),)
CXXFLAGS+=-D_UNISHOX2
OB+=$(call obj,Unishox2/unishox2.o turbobench_/unishox.o)
CXXFLAGS+=-D_UNISHOX3 -Imarisa-trie/include
OB+=$(call obj,Unishox2/Unishox3_Alpha/unishox3.o)
endif
#------------------------- Entropy coder -----------------------------------------
# First download or clone aomedia (git clone https://aomedia.googlesource.com/aom) into TurboBench directory
# after cmake, put the generated "aom_config.h" into the aom directory
# or copy aom_/aom_config.h to aom
ifneq ($(wildcard EC/aom/.),)
CXXFLAGS+=-D_AOM
OB+=$(call obj,EC/aom_/aom.o EC/aom/aom_dsp/entenc.o EC/aom/aom_dsp/entdec.o EC/aom/aom_dsp/entcode.o)
#OB+=daala_/daala.o
endif

# First download or clone daala (https://github.com/xiph/daala) into TurboBench directory
ifneq ($(wildcard EC/daala/.),)
CXXFLAGS+=-D_DAALA
OB+=$(call obj,EC/daala_/daala.o)
endif

ifneq ($(wildcard EC/fastac/.),)
CXXFLAGS+=-D_FASTAC
OB+=$(call obj,EC/fastac/arithmetic_codec.o)
endif

ifneq ($(wildcard EC/fasthf/.),)
CXXFLAGS+=-D_FASTHF
OB+=$(call obj,EC/fasthf/binary_codec.o)
endif

ifneq ($(wildcard EC/fpaq0p/.),)
CXXFLAGS+=-D_FPAQ0P
OB+=$(call obj,EC/fpaq0p/fpaq0p_sh.o)
endif

ifneq ($(wildcard EC/FPC/.),)
CXXFLAGS+=-D_FPC
OB+=$(call obj,EC/FPC/fpc.o)
endif

ifdef FREQTAB
CXXFLAGS+=-D_FREQTAB

ifdef FREQTABO
FREQOPT=-march=skylake -fwhole-program -fpermissive -fstrict-aliasing -fomit-frame-pointer -I../Lib3 -I../Lib -fno-stack-protector -fno-stack-check -fno-check-new -fno-exceptions \
  -fno-rtti -fno-operator-names -flto -ffat-lto-objects -Wl,-flto -fuse-linker-plugin -Wl,-O -Wl,--sort-common -Wl,--as-needed -ffunction-sections

$(BUILDIR)/EC/freqtab/src/c_mem.o: EC/freqtab/src/c_mem.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(FREQOPT) -O3 -std=c++11 $< -c -o $@

$(BUILDIR)/EC/freqtab/src/model.o: EC/freqtab/src/model.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(FREQOPT) -O3 -march=skylake -std=c++11 $< -c -o $@
endif

OB+=$(call obj,EC/freqtab/src/c_mem.o EC/freqtab/src/coder/model.o)
endif

ifdef GANS
CXXFLAGS+=-D_GANS
OB+=$(call obj,EC/rans.o EC/head_cbloom.o)
endif

# PivCo-Huffman (https://github.com/MarcinZukowski/pivco-huffman): SIMD tree-walk Huffman, levels 1=PH, 2=PHA.  Submodule built via its own CMake; we link the
# pre-localized object (libpivco_huffman_local.o) whose vendored FSE_*/HUF_* symbols are localized so they don't clash with the zstd TurboBench bundles.
# The submodule has its own submodule (ext/fse), so init recursively:  git submodule update --init --recursive pivco-huffman
ifneq ($(wildcard pivco-huffman/.),)
ifndef CROSS
PIVCODIR = pivco-huffman
CXXFLAGS+=-D_PIVCOHUF -I$(PIVCODIR)/include
$(BUILDIR)/$(PIVCODIR)/libpivco_huffman_local.o:
	cmake -S $(PIVCODIR) -B $(BUILDIR)/$(PIVCODIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILDIR)/$(PIVCODIR) --target pivco_huffman_local -j
OB+=$(BUILDIR)/$(PIVCODIR)/libpivco_huffman_local.o

# PHAZ: PivCo-Huffman entropy transplant onto zstd (full LZ+entropy compressor;
# level = zstd level).  Built from the pivco-huffman submodule's extras/phaz via its own build.sh: it patches a *private* copy of zstd source (pointed at
# TurboBench's own zstd/ submodule, same pinned SHA 5233c58e) and merges it + pivco into one blob (phaz_local.o) that exports only phaz_compress /
# phaz_decompress -- everything else (all of zstd, FSE/HUF, pivco) is localized, so it coexists with the vanilla zstd TurboBench links.  Requires:
#   git submodule update --init --recursive pivco-huffman zstd
PHAZDIR=$(PIVCODIR)/extras/phaz
CXXFLAGS+=-D_PHAZ
$(PHAZDIR)/build/phaz_local.o:
	cmake -S $(PIVCODIR) -B $(PIVCODIR)/build -DCMAKE_BUILD_TYPE=Release
	cmake --build $(PIVCODIR)/build --target pivco_huffman_local -j
	ZSTD_SRC=$(abspath zstd) MARCH="$(MARCH)" CC=$(CC) bash $(PHAZDIR)/tools/build.sh
OB += $(PHAZDIR)/build/phaz_local.o
LDFLAGS+=-lm
endif
endif

ifdef RECIPARITH
CXXFLAGS+=-D_RECIPARITH
OB+=$(call obj,EC/recip_arith_/reciparith.o)
endif

ifneq ($(and $(wildcard EC/sserangecoding/.),$(filter x86_64,$(ARCH))),)
$(BUILDIR)/EC/sserangecoding/sserangecoder.o: EC/sserangecoding/sserangecoder.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -O3 $(CFLAGS) -march=corei7-avx -mtune=corei7-avx -mno-aes EC/sserangecoding/sserangecoder.cpp -o $@

CXXFLAGS+=-D_SSERC
OB+=$(BUILDIR)/EC/sserangecoding/sserangecoder.o
endif

ifneq ($(wildcard EC/subotin/.),)
CXXFLAGS+=-D_SUBOTIN
OB+=$(call obj,EC/subotin_/subotin.o)
endif

ifneq ($(wildcard Turbo-Range-Coder/.),)
ifneq ($(ARCH),loongarch64)
CXXFLAGS+=-D_TURBORC
CFLAGS+=-D_ANS -D_BWT -ITurbo-Range-Coder/libsais/include 
TRC=Turbo-Range-Coder/
#$(BUILDIR)/$(TRC)anscdf0.o: $(TRC)anscdf.c $(TRC)anscdf_.h
#	@mkdir -p $(dir $@)
#	$(CC) -c -O3 $(CFLAGS) -falign-loops=32 $(TRC)anscdf.c -o $@
$(BUILDIR)/$(TRC)anscdfs.o: $(TRC)anscdf.c $(TRC)anscdf_.h
	@mkdir -p $(dir $@)
	$(CC) -c -O3 $(CFLAGS) $(_SSE) -falign-loops=32 $(TRC)anscdf.c -o $@
OB+=$(BUILDIR)/$(TRC)anscdfs.o 
ifeq ($(ARCH), x86_64)
$(BUILDIR)/$(TRC)anscdfx.o: $(TRC)anscdf.c $(TRC)anscdf_.h
	@mkdir -p $(dir $@)
	$(CC) -c -O3 $(CFLAGS) $(_AVX2) -falign-loops=32 $(TRC)anscdf.c -o $@
OB+=$(BUILDIR)/$(TRC)anscdfx.o 
endif
OB+=$(call obj,Turbo-Range-Coder/rc_ss.o Turbo-Range-Coder/rc_s.o Turbo-Range-Coder/rccdf.o Turbo-Range-Coder/rcutil.o Turbo-Range-Coder/bec_b.o Turbo-Range-Coder/rccm_s.o Turbo-Range-Coder/rccm_ss.o \
  Turbo-Range-Coder/rcqlfc_s.o Turbo-Range-Coder/rcqlfc_ss.o Turbo-Range-Coder/rcqlfc_sf.o Turbo-Range-Coder/rcbwt.o Turbo-Range-Coder/libsais/src/libsais16.o)

LIBSAIS=1
ifdef LZTURBO
CFLAGS+=-D_NCPUISA -D_NQUANT
endif
endif
endif

ifneq ($(wildcard EC/vecrc/.),)
CXXFLAGS+=-D_VECRC
OB+=$(call obj,EC/vecrc/vector_rc.o)
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
ifneq ($(ARCH),loongarch64)
CXXFLAGS+=-D_TURBORLE

TRLEDIR = Turbo-Run-Length-Encoding
BUILD_TRLE = $(CC) -O3 $(TRLE_FLAGS) $< -c -o $@

$(BUILDIR)/$(TRLEDIR)/%.o: TRLE_FLAGS = $(_AVX2) -w -fstrict-aliasing -falign-loops=32 $(DEBUG)
$(BUILDIR)/$(TRLEDIR)/%.o: $(TRLEDIR)/%.c
	@mkdir -p $(dir $@)
	$(BUILD_TRLE)

OB+=$(BUILDIR)/$(TRLEDIR)/trlec.o $(BUILDIR)/$(TRLEDIR)/trled.o

CXXFLAGS+=-D_MRLE
OB+=$(call obj,Turbo-Run-Length-Encoding/ext/mrle.o)
endif
endif

ifneq ($(wildcard hypersonic-rle-kit/.),)
CXXFLAGS+=-D_HRLE
HRLE=hypersonic-rle-kit

$(BUILDIR)/$(HRLE)/src/simd_platform.o: $(HRLE)/src/simd_platform.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -mxsave $(MARCH) $(CFLAGS) $< -c -o $@

OB+=$(call obj,$(HRLE)/src/rle_sh.o $(HRLE)/src/rle8_extreme_cpu.o $(HRLE)/src/rle8_low_entropy_cpu.o $(HRLE)/src/rle8_low_entropy_short_cpu.o $(HRLE)/src/rle8_mmtf.o \
  $(HRLE)/src/rle24_extreme_cpu.o $(HRLE)/src/rle48_extreme_cpu.o $(HRLE)/src/rle128_extreme_cpu.o \
  $(HRLE)/src/rleX_extreme_cpu.o $(HRLE)/src/rle8_mmtf.o)
OB+=$(BUILDIR)/$(HRLE)/src/simd_platform.o
endif

#-------------------------------------- Archived ----------------------------------
ifneq ($(wildcard chameleon/.),)
CXXFLAGS+=-D_CHAMELEON
OB+=$(call obj,chameleon/chameleon.o)
endif

ifneq ($(wildcard density/.),)
CXXFLAGS+=-D_DENSITY
OB+=$(call obj,density/src/buffers/buffer.o density/src/algorithms/algorithms.o density/src/algorithms/dictionaries.o density/src/structure/header.o density/src/globals.o density/src/buffers/buffer.o \
	density/src/algorithms/chameleon/core/chameleon_decode.o density/src/algorithms/chameleon/core/chameleon_encode.o \
	density/src/algorithms/lion/core/lion_decode.o density/src/algorithms/lion/core/lion_encode.o density/src/algorithms/lion/forms/lion_form_model.o \
	density/src/algorithms/cheetah/core/cheetah_decode.o density/src/algorithms/cheetah/core/cheetah_encode.o)
endif

ifneq ($(wildcard lzjody/.),)
CXXFLAGS+=-D_LZJODY
OB+=$(call obj,lzjody/lzjody.o lzjody/byteplane_xfrm.o)
endif

ifneq ($(wildcard lzoma_/.),)
CXXFLAGS+=-D_LZOMA
OB+=$(call obj,lzoma_/pack.o lzoma_/unpack.o lzoma_/divsufsort.o)
endif

ifneq ($(wildcard xpack/.),)
CXXFLAGS+=-D_XPACK
# O2 instead of O3 because of error gcc 7
$(BUILDIR)/xpack/lib/xpack_common.o: xpack/lib/xpack_common.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@

$(BUILDIR)/xpack/lib/xpack_compress.o: xpack/lib/xpack_compress.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@

$(BUILDIR)/xpack/lib/xpack_decompress.o: xpack/lib/xpack_decompress.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@

$(BUILDIR)/xpack/lib/x86_cpu_features.o: xpack/lib/x86_cpu_features.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@

OB+=$(BUILDIR)/xpack/lib/xpack_common.o $(BUILDIR)/xpack/lib/xpack_compress.o $(BUILDIR)/xpack/lib/xpack_decompress.o $(BUILDIR)/xpack/lib/x86_cpu_features.o
endif

ifneq ($(wildcard pithy/.),)
CXXFLAGS+=-D_PITHY
$(BUILDIR)/pithy/pithy.o: pithy/pithy.c
	@mkdir -p $(dir $@)
	$(CC) -O2 $(MARCH) $(CFLAGS)  $< -c -o $@
OB+=$(BUILDIR)/pithy/pithy.o
endif

ifneq ($(wildcard shrinker/.),)
CXXFLAGS+=-D_SHRINKER
$(BUILDIR)/shrinker/shrinker.o: shrinker/shrinker.c
	@mkdir -p $(dir $@)
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@
OB+=$(call obj,shrinker/Shrinker.o)
endif

ifneq ($(wildcard wlfz/.),)
CXXFLAGS+=-D_WFLZ
$(BUILDIR)/wflz/wfLZ.o: wflz/wfLZ.c
	@mkdir -p $(dir $@)
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@
OB+=$(BUILDIR)/wflz/wfLZ.o
endif

ifneq ($(wildcard nakamichi/.),)
CXXFLAGS+=-D_NAKAMICHI
$(BUILDIR)/nakamichi/Nakamichi_Okamigan.o: nakamichi/Nakamichi_Okamigan.c
	@mkdir -p $(dir $@)
	$(CC) -O3 -msse4.1 $(MARCH) $< -c -o $@
OB+=$(call obj,nakamichi/Nakamichi_Washigan.o)
endif

ifneq ($(wildcard FastLZ/.),)
CXXFLAGS+=-D_FASTLZ
OB+=$(call obj,FastLZ/fastlz.o)
endif

ifneq ($(wildcard heatshrink_/.),)
CXXFLAGS+=-D_HEATSHRINK
OB+=$(call obj,heatshrink_/heatshrink.o heatshrink/heatshrink_encoder.o heatshrink/heatshrink_decoder.o)
endif

ifneq ($(wildcard liblzf/.),)
CXXFLAGS+=-D_LIBLZF
OB+=$(call obj,liblzf/lzf_c.o liblzf/lzf_c_best.o liblzf/lzf_d.o)
endif

ifneq ($(wildcard liblzg/.),)
CXXFLAGS+=-D_LIBLZG
OB+=$(call obj,liblzg/src/lib/encode.o liblzg/src/lib/decode.o liblzg/src/lib/checksum.o)
endif

ifneq ($(wildcard shoco/.),)
CXXFLAGS+=-D_SHOCO
OB+=$(call obj,shoco/shoco.o)
endif

ifneq ($(wildcard smaz/.),)
CXXFLAGS+=-D_SMAZ
OB+=$(call obj,smaz/smaz.o)
endif

ifneq ($(wildcard yappy/.),)
CXXFLAGS+=-D_YAPPY
OB+=$(call obj,yappy/yappy.o)
endif


ifneq ($(wildcard CSC/.),)
CXXFLAGS+=-D_CSC
OB+=$(call obj,CSC/src/libcsc/csc_analyzer.o CSC/src/libcsc/csc_coder.o CSC/src/libcsc/csc_dec.o CSC/src/libcsc/csc_default_alloc.o CSC/src/libcsc/csc_enc.o CSC/src/libcsc/csc_encoder_main.o CSC/src/libcsc/csc_filters.o CSC/src/libcsc/csc_lz.o CSC/src/libcsc/csc_memio.o \
	CSC/src/libcsc/csc_mf.o CSC/src/libcsc/csc_model.o CSC/src/libcsc/csc_profiler.o)
endif

ifneq ($(wildcard doboz/.),)
CXXFLAGS+=-D_DOBOZ
OB+=$(call obj,doboz/Source/Doboz/Compressor.o doboz/Source/Doboz/Decompressor.o doboz/Source/Doboz/Dictionary.o)
endif

ifneq ($(wildcard libzling/.),)
CXXFLAGS+=-D_LIBZLING
# Disabled : compile error in gcc 7.2
#OB+=libzling/src/libzling.o libzling/src/libzling_huffman.o libzling/src/libzling_utils.o libzling/src/libzling_lz.o libzling_/libzling_utils_mem.o
endif

ifneq ($(wildcard Behemoth-Rank-Coding/.),)
CXXFLAGS+=-D_BRC
OB+=$(call obj,Behemoth-Rank-Coding/brc.o)
endif
#----------------------- GPL -------------------------
ifneq ($(wildcard lzmat/.),)
CXXFLAGS+=-DLZMAT
$(BUILDIR)/lzmat/lzmat_dec.o: lzmat/lzmat_dec.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -D"__int64=long long" $(MARCH) $(CFLAGS) $< -c -o $@
$(BUILDIR)/lzmat/lzmat_enc.o: lzmat/lzmat_enc.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -D"__int64=long long" $(MARCH) $(CFLAGS) $< -c -o $@

OB+=$(BUILDIR)/lzmat/lzmat_enc.o $(BUILDIR)/lzmat/lzmat_dec.o
endif

ifneq ($(wildcard tornado/.),)
CXXFLAGS+=-D_TORNADO
$(BUILDIR)/tornado_/tormem.o: tornado_/tormem.cpp
	@mkdir -p $(dir $@)
	$(CXX) -O3 $(TORDEF) -D__x86_$(ARCH)__ -DFREEARC_$(ARCH)BIT -pipe -fforce-addr -fno-exceptions -fno-rtti -c tornado_/tormem.cpp -o $@
ifeq ($(OS), Linux)
TORDEF=-DFREEARC_UNIX -DFREEARC_INTEL_BYTE_ORDER
else
TORDEF=-DFREEARC_WIN -DFREEARC_INTEL_BYTE_ORDER -D_UNICODE -DUNICODE
endif
OB+=$(BUILDIR)/tornado_/tormem.o
endif

ifneq ($(wildcard ms-compress/.),)
CXXFLAGS+=-D_MSCOMPRESS
OB+=$(call obj,ms-compress/src/mscomp.o ms-compress/src/lznt1_compress.o ms-compress/src/lznt1_decompress.o ms-compress/src/xpress_compress.o ms-compress/src/xpress_decompress.o ms-compress/src/xpress_huff_compress.o ms-compress/src/xpress_huff_decompress.o)
endif

ifneq ($(wildcard quicklz_/.),)
CXXFLAGS+=-D_QUICKLZ
OB+=$(call obj,quicklz_/quicklz1.o quicklz_/quicklz2.o quicklz_/quicklz3.o)
endif

ifneq ($(wildcard pysap/.),)
CXXFLAGS+=-D_PYSAP
OB+=$(call obj,pysap/pysapcompress/vpa105CsObjInt.o pysap/pysapcompress/vpa106cslzc.o pysap/pysapcompress/vpa107cslzh.o pysap/pysapcompress/vpa108csulzh.o)
endif
#--------------------------------------------------------------------

OB+=$(ICL) $(HUF) $(ANX) $(LZ) $(BUILDIR)/plugin.o

$(BUILDIR)/plugin.o: plugin.cc $(C_BLOSC2_LIB) $(ISAL_LIB) $(OPENZL_LIB) $(ZLIB_NG_LIB) $(XZ_LIB) 
	@mkdir -p $(dir $@)
	$(CXX) -O3 $(MARCH) $(CXXFLAGS)  $< -c -o $@


turbobench: $(OB) $(BUILDIR)/turbobench.o $(BUILDIR)/plugin.o
	$(CXX) $^ $(LDFLAGS) -o turbobench

$(BUILDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -O3 $(MARCH) $(CFLAGS) $< -c -o $@

$(BUILDIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) -O3 $(MARCH) $(CXXFLAGS)  $< -c -o $@

$(BUILDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -O3 $(MARCH) $(CXXFLAGS) $< -c -o $@

$(BUILDIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) -O3 $(MARCH) $(CFLAGS) $< -c -o $@


ifeq ($(OS),Windows)
clean:
	rmdir /S /Q $(BUILDIR)
	del /S *~
	del /S *.exe
else
clean:
	rm -rf $(BUILDIR)
	find . -name "turbobench" -type f -delete
	find . -name "*.o" -type f -delete
	find . -name "*~" -type f -delete
	find . -name "core" -type f -delete

cleana:
	rm -rf $(BUILDIR)
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

