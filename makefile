# powturbo  (c) Copyright 2013-2026
# ----------- Downloading + Compiling -----------------------
# git clone --recursive git://github.com/powturbo/TurboBench.git
# make
#
#----------------
# Cross compile: export CROSS to aarch64 riscv64 loongarch64 or powerpc64le. Ex.:
# export CROSS=aarch64
# Testing with qemu
# qemu-aarch64 -L /usr/aarch64-linux-gnu ./turbobench -l2
# qemu-riscv64 -L /usr/riscv64-linux-gnu ./turbobench -l2
# qemu-ppc64le -L /usr/powerpc64le-linux-gnu

CC ?= gcc
#CC ?= clang
CXX ?= g++
CX ?= clang
#CX ?= gcc
#CC = clang

BUILD ?= build
obj = $(addprefix $(BUILD)/,$(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(patsubst %.cpp,%.o,$(patsubst %.S,%.o,$(1))))))

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

CFLAGS+=-w -Wall $(DDEBUG) -fpermissive 
#-Wimplicit-function-declaration -std=gnu99 
CXXFLAGS+=$(DDEBUG) -w -Wall -fpermissive  -fno-rtti

ifeq ($(OS),$(filter $(OS),Linux GNU/kFreeBSD GNU OpenBSD FreeBSD DragonFly NetBSD MSYS_NT Haiku))
LDFLAGS+=-lrt -lpthread
endif

ifdef STATIC
LDFLAGS+=-static
NMEMSIZE=1
endif

ifdef NMEMSIZE
CFLAGS+=-DNMEMSIZE
else ifeq ($(OS),$(filter $(OS),Darwin FreeBSD GNU/kFreeBSD Linux NetBSD SunOS))
LDFLAGS += -ldl
endif

HAVE_OPENMP := $(shell echo 'int main(){return 0;}' | $(CC) -fopenmp -x c - -o /dev/null 2>/dev/null && echo yes || echo no)
ifeq ($(HAVE_OPENMP),no)
  $(warning OpenMP not available)
endif

all: turbobench 
 
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
ifneq ($(OS), Windows)  # not compiling for windows in CI. ar.exe ERROR
C_BLOSC2_SRCS := $(shell find c-blosc2 -type f -name '*.[c]' -o -name '*.cpp' -o -name '*.cc')
ifdef CROSS #ERROR IN C_BLOSC BUILD  
#CXXFLAGS+=-D_C_BLOSC2
#C_BLOSC2_LIB = $(BUILD)/c-blosc2/blosc/libblosc2.a
#$(C_BLOSC2_LIB): $(C_BLOSC2_SRCS)
#	export CC=$(CROSS)-linux-gnu-gcc
#	export CXX=$(CROSS)-linux-gnu-g++
#	cmake -S c-blosc2 -B $(BUILD)/c-blosc2 -DBLOSC_ZSTD_SOURCE_DIR=zstd -DBUILD_TESTS=OFF -DBUILD_BENCHMARKS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_FUZZERS=OFF \
#	          -DPREFER_EXTERNAL_LZ4=ON -DPREFER_EXTERNAL_ZLIB=ON -DPREFER_EXTERNAL_ZSTD=ON  -DBUILD_SHARED=OFF -DBUILD_SHARED_LIBS=OFF
#	cmake --build $(BUILD)/c-blosc2
else
CXXFLAGS+=-D_C_BLOSC2
C_BLOSC2_LIB = $(BUILD)/c-blosc2/blosc/libblosc2.a
$(C_BLOSC2_LIB): $(C_BLOSC2_SRCS)
	cmake -S c-blosc2 -B $(BUILD)/c-blosc2 -DBLOSC_ZSTD_SOURCE_DIR=zstd -DBUILD_TESTS=OFF -DBUILD_BENCHMARKS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_FUZZERS=OFF \
	          -DPREFER_EXTERNAL_LZ4=ON -DPREFER_EXTERNAL_ZLIB=ON -DPREFER_EXTERNAL_ZSTD=ON  -DBUILD_SHARED=OFF -DBUILD_SHARED_LIBS=OFF 
	cmake --build $(BUILD)/c-blosc2 --parallel 4
endif
LDFLAGS += $(C_BLOSC2_LIB)
endif
endif

ifneq ($(wildcard ClickhouseXXX/.),)
CXXFLAGS+=-D_CLICKHOUSE -IClickhouse/src -IClickhouse	#-IClickhouse/base/pcg_random -IContrib/abseil-cpp
OB+=$(call obj,Clickhouse/src/Compression/LZ4_decompress_faster.o)
endif

ISAL_LIB :=
ifneq ($(wildcard isa-l/.),)
ifndef CROSS
NASM ?= $(shell command -v nasm)
ifeq ($(NASM),)
  ifneq ($(wildcard isa-l_/$(OS)-$(ARCH)/isa-l.a),)
    CXXFLAGS += -D_ISA_L
    ISAL_LIB := isa-l_/$(OS)-$(ARCH)/isa-l.a    
  endif     
else
CXXFLAGS += -D_ISA_L
ISAL_SRCS := $(shell find isa-l -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.asm' \))
ISAL_LIB := $(BUILD)/bin/isa-l.a 
$(ISAL_LIB): $(ISAL_SRCS)
	@mkdir -p $(BUILD)/isa-l
	@mkdir -p $(BUILD)/bin
	@mkdir -p isa-l/bin
	$(MAKE) -C isa-l -f Makefile.unx O=$(abspath $(BUILD)/isa-l)
	@mv isa-l/bin/isa-l.a $@
endif
LDFLAGS += $(ISAL_LIB)
endif
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
CXXFLAGS+=-D_LIBBSC
LIBBSC_CFLAGS = -O3 -D_LIBBSC -DLIBBSC_SORT_TRANSFORM_SUPPORT -ICSC/src/libcsc 
LIBBSC_LDFLAGS =
ifeq ($(HAVE_OPENMP),yes)
  LIBBSC_CFLAGS  += -fopenmp -DLIBBSC_OPENMP_SUPPORT -DLIBSAIS_OPENMP
  LDFLAGS += -fopenmp
  $(info OpenMP enabled for libbsc)
endif
OB += $(BUILD)/libbsc/libbsc/libbsc/libbsc.o $(BUILD)/libbsc/libbsc/coder/coder.o $(BUILD)/libbsc/libbsc/coder/qlfc/qlfc.o $(BUILD)/libbsc/libbsc/coder/qlfc/qlfc_model.o $(BUILD)/libbsc/libbsc/filters/detectors.o \
	$(BUILD)/libbsc/libbsc/filters/preprocessing.o $(BUILD)/libbsc/libbsc/adler32/adler32.o $(BUILD)/libbsc/libbsc/bwt/bwt.o $(BUILD)/libbsc/libbsc/st/st.o $(BUILD)/libbsc/libbsc/lzp/lzp.o \
	$(BUILD)/libbsc/libbsc/platform/platform.o $(BUILD)/libbsc/libbsc/bwt/libsais/libsais.o

$(BUILD)/libbsc/%.o: libbsc/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(LIBBSC_CFLAGS) -c $< -o $@

$(BUILD)/libbsc/%.o: libbsc/%.c
	@mkdir -p $(dir $@)
	$(CC) $(LIBBSC_CFLAGS) -c $< -o $@
LIBSAIS = 1
endif

ifneq ($(wildcard libdeflate/.),)
CXXFLAGS+=-D_LIBDEFLATE
CFLAGS+=-Ilibdeflate -Ilibdeflate/common
LIBDEFLATE_SRCS := $(wildcard libdeflate/lib/*.c) libdeflate/lib/arm/cpu_features.c libdeflate/lib/x86/cpu_features.c 
OB += $(call obj,$(LIBDEFLATE_SRCS))
endif

ifneq ($(wildcard libslz/.),)
CXXFLAGS+=-D_LIBSLZ
LIBSLZ_SRCS := libslz/src/slz.c libslz/src/uslz.c libslz/src/slz_common.c
OB+=$(call obj,$(LIBSLZ_SRCS))
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

ifneq ($(wildcard lzham_codec_devel/.),)
ifneq ($(OS),$(filter $(OS),Darwin))
CXXFLAGS+=-D_LZHAM -D"UINT64_MAX=-1ull" -Ilzham_codec_devel/include -Ilzham_codec_devel/lzhamcomp -Ilzham_codec_devel/lzhamdecomp
LZHAM_SRCS := $(wildcard lzham_codec_devel/lzhamcomp/*.cpp) $(wildcard lzham_codec_devel/lzhamdecomp/*.cpp) $(wildcard lzham_codec_devel/lzhamlib/*.cpp)
LZHAM_SRCS := $(filter-out %/lzham_win32_threading.cpp, $(LZHAM_SRCS))
OB += $(call obj,$(LZHAM_SRCS))
ifeq ($(OS), Windows)
OB += $(call obj,lzham_codec_devel/lzhamcomp/lzham_win32_threading.o)
else
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
LZMA_SRCS := lzma/C/Alloc.c lzma/C/CpuArch.c lzma/C/LzFind.c lzma/C/LzmaDec.c lzma/C/LzmaEnc.c lzma/C/LzmaLib.c lzma/C/Threads.c lzma/C/LzFindMt.c lzma/C/LzFindOpt.c
OB += $(call obj,$(LZMA_SRCS))
endif

ifneq ($(wildcard lzo/.),)
CXXFLAGS+=-D_LZO -Ilzo/include
CFLAGS+=-Ilzo/include
LZO_SRCS := $(wildcard lzo/src/*.c)
OB += $(call obj,$(LZO_SRCS))
endif

ifneq ($(and $(wildcard LZSSE/.),$(filter x86_64,$(ARCH))),)
CXXFLAGS += -D_LZSSE
OB += $(addprefix $(BUILD)/LZSSE/, lzsse2/lzsse2.o lzsse4/lzsse4.o lzsse8/lzsse8.o)
$(BUILD)/LZSSE/%.o: LZSSE/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -O2 -msse4.1 -std=c++11 $< -c -o $@
endif

ifneq ($(wildcard memlz/.),)
CXXFLAGS+=-D_MEMLZ
endif

ifneq ($(wildcard miniz/.),)
miniz/miniz_export.h: miniz_/miniz_export.h
	cp miniz_/miniz_export.h miniz/miniz_export.h
$(BUILD)/miniz/miniz.o: miniz/miniz.c miniz/miniz_export.h
	@mkdir -p $(dir $@)
	$(CC) -O3 $(MARCH) $(CFLAGS) $< -c -o $@
CXXFLAGS+=-D_MINIZ
OB+=$(BUILD)/miniz/miniz.o $(call obj,miniz/miniz_tdef.o miniz/miniz_tinfl.o)
endif

ifneq ($(wildcard misa77/.),)
CXXFLAGS += -D_MISA77
MISA77_DIR  := misa77
MISA77_SRCS := $(wildcard $(MISA77_DIR)/src/*.cpp)
MISA77_OBJS := $(MISA77_DIR)/src/isa/target_portable.o
MISA77_BUILD = $(CXX) -O3 $(CXXFLAGS) -std=c++20 -I$(MISA77_DIR)/include -I$(MISA77_DIR)/src $< -c -o $@
ifeq ($(ARCH),x86_64)
MISA77_BASE := -march=x86-64
MISA77_AVX2 := -mavx2
endif
$(BUILD)/$(MISA77_DIR)/src/%_portable.o: CXXFLAGS += $(MISA77_BASE)
$(BUILD)/$(MISA77_DIR)/src/%_sse2.o: CXXFLAGS += $(MISA77_BASE)
$(BUILD)/$(MISA77_DIR)/src/%_avx2.o: CXXFLAGS += $(MISA77_AVX2)

$(BUILD)/$(MISA77_DIR)/src/%.o: $(MISA77_DIR)/src/%.cpp
	@mkdir -p $(dir $@)
	$(MISA77_BUILD)

OB += $(call obj,$(MISA77_SRCS) $(MISA77_OBJS))
MISA77_ARCH_OBJS.x86_64  := isa/target_sse2.o isa/target_avx2.o
MISA77_ARCH_OBJS.aarch64 := isa/target_neon.o
OB += $(addprefix $(BUILD)/$(MISA77_DIR)/src/,$(MISA77_ARCH_OBJS.$(ARCH)))
endif

ifneq ($(wildcard misa77),)
CXXFLAGS += -D_MISA77
MISA77_SRC := misa77/src
ifeq ($(ARCH),x86_64)
  $(BUILD)/$(MISA77_SRC)/%_sse2.o: CXXFLAGS += -march=x86-64
  $(BUILD)/$(MISA77_SRC)/%_avx2.o: CXXFLAGS += $(_AVX2)
  MISA77_VOBJS := isa/target_sse2.o isa/target_avx2.o
else ifeq ($(ARCH),aarch64)
  MISA77_VOBJS := isa/target_neon.o
endif

$(BUILD)/$(MISA77_SRC)/%.o: $(MISA77_SRC)/%.cpp
	@mkdir -p $(@D)
	$(CXX) -O3 $(CXXFLAGS) -std=c++20 -Imisa77/include -I$(MISA77_SRC) -c $< -o $@

MISA77_SRCS := $(wildcard $(MISA77_SRC)/*.cpp)
OB += $(call obj,$(MISA77_SRCS) $(MISA77_SRC)/isa/target_portable.o) $(addprefix $(BUILD)/$(MISA77_SRC)/,$(MISA77_VOBJS))
endif

OPENZL_LIB :=
ifneq ($(wildcard openzl/.),)
OPENZL_SRCS := $(shell find openzl -type f -name '*.[ch]' -o -name '*.cpp' -o -name '*.cc' -o -name 'CMakeLists.txt' -o -name 'Makefile')
ifdef CROSS  # NOTWORKING
#OPENZL_LIB = $(BUILD)/openzl/libopenzl.a
#$(OPENZL_LIB): $(OPENZL_SRCS)
#	export CC=$(CROSS)-linux-gnu-gcc
#	export CXX=$(CROSS)-linux-gnu-g++
#	cmake -S openzl -B $(BUILD)/openzl -DCMAKE_C_COMPILER=$(CROSS)-linux-gnu-gcc -DCMAKE_CXX_COMPILER=$(CROSS)-linux-gnu-g++
#	cmake --build $(BUILD)/openzl --config Release
else
CXXFLAGS += -D_OPENZL -Iopenzl/include -Iopenzl/src
ifeq ($(OS), Windows)
OPENZL_LIB = openzl/libopenzl.a
$(OPENZL_LIB): $(OPENZL_SRCS)
	cd openzl && $(MAKE) lib
else
OPENZL_LIB = $(BUILD)/openzl/libopenzl.a
$(OPENZL_LIB): $(OPENZL_SRCS)
	cmake -S openzl -B $(BUILD)/openzl
	cmake --build $(BUILD)/openzl --config Release
endif
endif
LDFLAGS += $(OPENZL_LIB)
endif

# 'oo2core_9_win64.dll', 'liboo2corelinuxarm64.so.9' or 'liboo2corelinux64.so.9' must be in the same directory as turbobench[.exe]
# download corresponding library from https://github.com/WorkingRobot/OodleUE
CXXFLAGS+=-D_OODLE
ifneq ($(wildcard pcodec_/.),)
endif

ifneq ($(wildcard pivco-huffman/.),)
ifneq ($(wildcard OodleUE/.),)
OODLE_DIR := OodleUE/Engine/Source/Runtime/OodleDataCompression/Sdks/2.9.16
CXXFLAGS += -D_OODLE_EC -I$(OODLE_DIR)/src/oodle2/core -I$(OODLE_DIR)/src/oodle2/core/public -DOODLE_IMPORT_LIB
OODLE_SRCS := $(wildcard $(OODLE_DIR)src/oodle2/core/*.cpp) $(wildcard $(OODLE_DIR)src/oodle2/core/public/*.cpp) $(wildcard $(OODLE_DIR)src/oodle2/base/*.cpp) $(wildcard $(OODLE_DIR)include/*.cpp)
ifeq ($(OS), Windows)
OODLE_STATIC_LIB := $(OODLE_DIR)/lib/Win64/oo2core_win64.lib
else ifeq ($(OS), Darwin)
OODLE_STATIC_LIB := $(OODLE_DIR)/lib/Mac/liboo2coremac64.a
else ifeq ($(ARCH), aarch64)
OODLE_STATIC_LIB := $(OODLE_DIR)/lib/LinuxArm64/liboo2corelinuxarm64.a
else ifeq ($(ARCH), x86_64)
OODLE_STATIC_LIB := $(OODLE_DIR)/lib/Linux/liboo2corelinux64.a
else
OODLE_STATIC_LIB := pivco-huffman/ext/oodle/build-out/ar/liboodle-data-static.a
endif 
OB += $(call obj,$(LZHAM_SRCS)) pivco-huffman/extras/bench/bench_oodle_wrapper.o
LDFLAGS+=$(OODLE_STATIC_LIB)
endif
endif

FIRETRAIL_LIB :=
ifneq ($(wildcard firetrail/.),)
CXXFLAGS+=-D_FIRETRAIL
FIRETRAIL_LIB=firetrail/libfiretrail.a
LDFLAGS += $(FIRETRAIL_LIB)
$(FIRETRAIL_LIB): firetrail/src/root.zig 
	cd firetrail && zig build-lib -O ReleaseFast -femit-bin=libfiretrail.a src/root.zig -lc
OB+=$(FIRETRAIL_LIB)	
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

LZ_LIB :=
LZ_DIR=../lz
ifneq ($(wildcard $(LZ_DIR)/.),)
CXXFLAGS+=-D_LZ
LZ_SRCS := $(shell find $(LZ_DIR)/lib -type f -name '*.[c]')
LZ_LIB = $(BUILD)/lz/liblz.a
$(LZ_LIB):  $(LZ_SRCS)
	@mkdir -p $(BUILD)/lz
	$(MAKE) $(LZ_LIB) -C $(LZ_DIR) BUILD=$(BUILD)/lz
LDFLAGS += $(LZ_LIB)
CFLAGS  += -D_NQUANT
endif

IC_LIB :=
IC_DIR=../ic
ifneq ($(wildcard $(IC_DIR)/.),)
CXXFLAGS+=-D_IC
IC_SRCS := $(shell find $(IC_DIR)/lib -type f -name '*.[c]')
IC_LIB = $(BUILD)/ic/libic.a
$(IC_LIB):  $(IC_SRCS)
	@mkdir -p $(BUILD)/ic
	$(MAKE) $(IC_LIB) -C $(IC_DIR) BUILD=$(BUILD)/ic
LDFLAGS += $(IC_LIB)
endif

XZ_LIB :=
ifneq ($(wildcard xz/.),)
CXXFLAGS += -D_XZ
XZ_SRCS := $(shell find xz/src/liblzma -type f -name '*.[c]' -o -name '*.cpp' -o -name '*.cc')
XZ_LIB = $(BUILD)/xz/liblzma.a
ifdef CROSS
$(XZ_LIB): $(XZ_SRCS)
	export CC=$(CROSS)-linux-gnu-gcc && cmake -S xz -B $(BUILD)/xz && $(MAKE) -C $(BUILD)/xz
else
$(XZ_LIB): $(XZ_SRCS)
	cmake -S xz -B $(BUILD)/xz && $(MAKE) -C $(BUILD)/xz
endif
LDFLAGS += $(XZ_LIB)
endif

ifneq ($(wildcard zlib/.),)
CXXFLAGS+=-D_ZLIB
ZD=zlib/
OB+=$(call obj,$(ZD)adler32.o $(ZD)crc32.o $(ZD)compress.o $(ZD)deflate.o $(ZD)infback.o $(ZD)inffast.o $(ZD)inflate.o $(ZD)inftrees.o $(ZD)trees.o $(ZD)uncompr.o $(ZD)zutil.o)
endif

ZLIB_NG_LIB :=
ifneq ($(wildcard zlib-ng/.),)
CXXFLAGS += -D_ZLIB_NG
ZLIB_NG_SRCS := $(shell find zlib-ng -type f -name '*.[c]' -o -name '*.cpp' -o -name '*.cc')
ZLIB_NG_LIB = $(BUILD)/zlib-ng/libz-ng.a
ifdef CROSS
$(ZLIB_NG_LIB): $(ZLIB_NG_SRCS)
	export CC=$(CROSS)-linux-gnu-gcc && cmake -S zlib-ng -B $(BUILD)/zlib-ng -DWITH_NEON=OFF -DBUILD_TESTING=OFF -DWITH_GTEST=OFF -DWITH_GZFILEOP=OFF 
	cmake --build $(BUILD)/zlib-ng --config Release
	cp $(BUILD)/zlib-ng/zconf-ng.h zlib-ng_
else
$(ZLIB_NG_LIB): $(ZLIB_NG_SRCS)
	cmake -S zlib-ng -B $(BUILD)/zlib-ng -DWITH_NEON=OFF -DBUILD_TESTING=OFF -DWITH_GTEST=OFF -DWITH_GZFILEOP=OFF
	cmake --build $(BUILD)/zlib-ng --config Release 
	cp $(BUILD)/zlib-ng/zconf-ng.h zlib-ng_
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
ZSTD_SRCS := $(wildcard zstd/lib/common/*.c) $(wildcard zstd/lib/compress/*.c) $(wildcard zstd/lib/decompress/*.c) $(wildcard zstd/lib/decompress/*.S) $(wildcard zstd/lib/dictBuilder/*.c)
ZSTD_C_SRCS := $(filter %.c,$(ZSTD_SRCS))
ZSTD_S_SRCS := $(filter %.S,$(ZSTD_SRCS))
ZSTD_OBJS := $(call obj,$(ZSTD_C_SRCS)) $(ZSTD_S_SRCS)
OB += $(ZSTD_OBJS)
endif

ifneq ($(wildcard zpaq/.),)
ifneq ($(OS),Darwin)
CXXFLAGS+=-D_ZPAQ -Izpaq
ifeq ($(HAVE_OPENMP),yes)
$(BUILD)/libzpaq_omp.cpp: zpaq/libzpaq.cpp
	(echo '#include <omp.h>'; cat $<) > $@
CXXFLAGS+=-fopenmp
LDFLAGS += -fopenmp
OB+=$(call obj,$(BUILD)/libzpaq_omp.o)
else
OB+=$(call obj,zpaq/libzpaq.o)
endif
ifneq ($(ARCH),x86_64)
  CXXFLAGS+= -DNOJIT
endif
endif
endif

ifneq ($(wildcard zxc/.),)
ifneq (,$(filter $(ARCH),x86_64 aarch64))
CXXFLAGS += -D_ZXC 
CFLAGS+=-DZXC_STATIC_DEFINE
ZXCDIR = zxc/src/lib
#ZXC_BUILD = $(CC) -O3 -I$(ZXCDIR)/vendors -DNDEBUG -DZXC_STATIC_DEFINE $(ZXC_FLAGS) $< -c -o $@
ZXC_BUILD = $(CC) -O3 -DZXC_STATIC_DEFINE -DNDEBUG -I$(ZXCDIR)/vendors $(ZXC_FLAGS) $< -c -o $@

define ZXC_RULE
$$(BUILD)/$$(ZXCDIR)/%$(1).o: ZXC_FLAGS = $(2)
$$(BUILD)/$$(ZXCDIR)/%$(1).o: $$(ZXCDIR)/%.c
	@mkdir -p $$(dir $$@)
	$$(ZXC_BUILD)
endef
$(eval $(call ZXC_RULE,,))
$(eval $(call ZXC_RULE,_default,-DZXC_FUNCTION_SUFFIX=_default))
$(eval $(call ZXC_RULE,_avx2,-mavx2 -mbmi -mbmi2 -mlzcnt -mno-avx512f -DZXC_FUNCTION_SUFFIX=_avx2 -DZXC_USE_AVX2))
$(eval $(call ZXC_RULE,_avx512,-mavx512bw -mbmi2 -DZXC_FUNCTION_SUFFIX=_avx512 -DZXC_USE_AVX512))
$(eval $(call ZXC_RULE,_neon,$(_SSE) -DZXC_FUNCTION_SUFFIX=_neon -DZXC_USE_NEON64))
ZXC_OBJS = common driver dispatch compress_default decompress_default dict_default huffman_default pivco_tables seekable 

ifeq ($(ARCH),x86_64)
  ZXC_OBJS += $(foreach e,avx2 avx512,compress_$(e) decompress_$(e) huffman_$(e)) 
else ifeq ($(ARCH),aarch64)
  ZXC_OBJS += $(foreach e,neon,compress_$(e) decompress_$(e) huffman_$(e))
endif
OB += $(call obj,$(patsubst %,$(ZXCDIR)/zxc_%.o,$(ZXC_OBJS)))
endif
endif

#------------------------------------ Manual Download ---------------------------------------------------------------------------
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

FSE := EC/fse
ifneq ($(wildcard FSE/.),)
CXXFLAGS+=-D_FSE
OB+=$(call obj,$(LB)EC/fse/fse_compress_.o $(LB)EC/fse/fse_decompress_.o)
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

$(BUILD)/EC/freqtab/src/c_mem.o: EC/freqtab/src/c_mem.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(FREQOPT) -O3 -std=c++11 $< -c -o $@

$(BUILD)/EC/freqtab/src/model.o: EC/freqtab/src/model.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(FREQOPT) -O3 -march=skylake -std=c++11 $< -c -o $@
endif

OB+=$(call obj,EC/freqtab/src/c_mem.o EC/freqtab/src/coder/model.o)
endif

ifdef GANS
CXXFLAGS+=-D_GANS
OB+=$(call obj,EC/rans.o EC/head_cbloom.o)
endif

ifneq ($(wildcard pivco-huffman/.),)
ifndef CROSS
PIVCODIR   = pivco-huffman
CXXFLAGS  += -D_PIVCOHUF -I$(PIVCODIR)/include
PIVCO_BDIR = $(BUILD)/$(PIVCODIR)
PIVCO_SRCS := $(shell find $(PIVCODIR)/src -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.cc' \))
PIVCO_CMAKE_FILES := $(shell find $(PIVCODIR) -maxdepth 2 -name 'CMakeLists.txt')
PIVCO_LIB   = $(PIVCO_BDIR)/libpivco_huffman_local.o
$(PIVCO_LIB): $(PIVCO_SRCS) $(PIVCO_CMAKE_FILES)
	mkdir -p $(PIVCO_BDIR)
	cmake -S $(PIVCODIR) -B $(PIVCO_BDIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(PIVCO_BDIR) --target pivco_huffman_local -j
OB += $(PIVCO_LIB)
# PHAZ: PivCo-Huffman entropy transplant onto zstd (full LZ+entropy compressor; level = zstd level). Built from the pivco-huffman submodule's extras/phaz via
# its own build.sh: patches a private zstd copy (TurboBench's pinned zstd/ SHA 5233c58e) and merges it + pivco into phaz_local.o exporting only
# phaz_compress / phaz_decompress. Requires: git submodule update --init --recursive pivco-huffman zstd
ifneq ($(PHAZ), 0)
CXXFLAGS     += -D_PHAZ
PHAZ_DIR      = $(PIVCODIR)/extras/phaz
PHAZ_BDIR     = $(PIVCODIR)/build
PHAZ_LIB      = $(PHAZ_DIR)/build/phaz_local.o
$(PHAZ_LIB): $(PIVCO_SRCS) $(PIVCO_CMAKE_FILES)
	@mkdir -p $(PHAZ_BDIR)
	cmake -S $(PIVCODIR) -B $(PHAZ_BDIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(PHAZ_BDIR) --target pivco_huffman_local -j
	ZSTD_SRC=$(abspath zstd) MARCH="$(MARCH)" CC=$(CC) PH=$(PIVCODIR) bash $(PHAZ_DIR)/tools/build.sh
OB += $(PHAZ_LIB)
endif
LDFLAGS += -lm
endif
endif

ifdef RECIPARITH
CXXFLAGS+=-D_RECIPARITH
OB+=$(call obj,EC/recip_arith_/reciparith.o)
endif

ifneq ($(and $(wildcard EC/sserangecoding/.),$(filter x86_64,$(ARCH))),)
$(BUILD)/EC/sserangecoding/sserangecoder.o: EC/sserangecoding/sserangecoder.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -O3 $(CFLAGS) -march=corei7-avx -mtune=corei7-avx -mno-aes EC/sserangecoding/sserangecoder.cpp -o $@

CXXFLAGS+=-D_SSERC
OB+=$(BUILD)/EC/sserangecoding/sserangecoder.o
endif

ifneq ($(wildcard EC/subotin/.),)
CXXFLAGS+=-D_SUBOTIN
OB+=$(call obj,EC/subotin_/subotin.o)
endif

ifneq ($(wildcard Turbo-Range-Coder/.),)
ifneq ($(ARCH),loongarch64)
CXXFLAGS += -D_TURBORC
RC_DIR  := Turbo-Range-Coder
RC_BDIR := $(BUILD)/$(RC_DIR)
CFLAGS   += -D_ANS -D_BWT -I$(RC_DIR)/libsais/include 
OB+=$(RC_BDIR)/anscdfs.o $(RC_BDIR)/rc_ss.o $(RC_BDIR)/rc_s.o $(RC_BDIR)/rccdf.o $(RC_BDIR)/rcutil.o $(RC_BDIR)/bec_b.o $(RC_BDIR)/rccm_s.o $(RC_BDIR)/rccm_ss.o \
  $(RC_BDIR)/rcqlfc_s.o $(RC_BDIR)/rcqlfc_ss.o $(RC_BDIR)/rcqlfc_sf.o $(RC_BDIR)/rcbwt.o $(RC_BDIR)/libsais/src/libsais16.o
$(RC_BDIR)/anscdfs.o: $(RC_DIR)/anscdf.c $(RC_DIR)/anscdf_.h
	@mkdir -p $(@D)
	$(CC) -O3 $(CFLAGS) $(_SSE) -falign-loops=32 -w -c $< -o $@
ifeq ($(ARCH), x86_64)
$(RC_BDIR)/anscdfx.o: $(RC_DIR)/anscdf.c $(RC_DIR)/anscdf_.h
	@mkdir -p $(@D)
	$(CC) -O3 $(CFLAGS) $(_AVX2) -falign-loops=32 -w -c $< -o $@
OB       += $(RC_BDIR)/anscdfx.o	
endif
ifeq ($(wildcard $(IC_DIR)/.),)
$(RC_BDIR)/tp.o: $(RC_DIR)/tp.c
	@mkdir -p $(@D)
	$(CC) -O3 $(CFLAGS) $(_SSE) -falign-loops=32 -w -c $< -o $@
OB       += $(RC_BDIR)/tp.o $(RC_BDIR)/tp_.o
ifeq ($(ARCH), x86_64)
$(RC_BDIR)/tp256.o: $(RC_DIR)/tp.c
	@mkdir -p $(@D)
	$(CC) -O3 $(CFLAGS) $(_AVX2) -w -c $< -o $@
OB      += $(RC_BDIR)/tp256.o
endif
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

$(BUILD)/$(TRLEDIR)/%.o: TRLE_FLAGS = $(_AVX2) -w -fstrict-aliasing -falign-loops=32 $(DEBUG)
$(BUILD)/$(TRLEDIR)/%.o: $(TRLEDIR)/%.c
	@mkdir -p $(dir $@)
	$(BUILD_TRLE)

OB+=$(BUILD)/$(TRLEDIR)/trlec.o $(BUILD)/$(TRLEDIR)/trled.o

CXXFLAGS+=-D_MRLE
OB+=$(call obj,Turbo-Run-Length-Encoding/ext/mrle.o)
endif
endif

ifneq ($(wildcard hypersonic-rle-kit/.),)
CXXFLAGS+=-D_HRLE
HRLE=hypersonic-rle-kit

$(BUILD)/$(HRLE)/src/simd_platform.o: $(HRLE)/src/simd_platform.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -mxsave $(MARCH) $(CFLAGS) $< -c -o $@

OB+=$(call obj,$(HRLE)/src/rle_sh.o $(HRLE)/src/rle8_extreme_cpu.o $(HRLE)/src/rle8_low_entropy_cpu.o $(HRLE)/src/rle8_low_entropy_short_cpu.o $(HRLE)/src/rle8_mmtf.o \
  $(HRLE)/src/rle24_extreme_cpu.o $(HRLE)/src/rle48_extreme_cpu.o $(HRLE)/src/rle128_extreme_cpu.o \
  $(HRLE)/src/rleX_extreme_cpu.o $(HRLE)/src/rle8_mmtf.o)
OB+=$(BUILD)/$(HRLE)/src/simd_platform.o
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
$(BUILD)/xpack/lib/xpack_common.o: xpack/lib/xpack_common.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@

$(BUILD)/xpack/lib/xpack_compress.o: xpack/lib/xpack_compress.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@

$(BUILD)/xpack/lib/xpack_decompress.o: xpack/lib/xpack_decompress.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@

$(BUILD)/xpack/lib/x86_cpu_features.o: xpack/lib/x86_cpu_features.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -Ixpack/common -Ixpack $(MARCH) $(CFLAGS) $< -c -o $@

OB+=$(BUILD)/xpack/lib/xpack_common.o $(BUILD)/xpack/lib/xpack_compress.o $(BUILD)/xpack/lib/xpack_decompress.o $(BUILD)/xpack/lib/x86_cpu_features.o
endif

ifneq ($(wildcard pithy/.),)
CXXFLAGS+=-D_PITHY
$(BUILD)/pithy/pithy.o: pithy/pithy.c
	@mkdir -p $(dir $@)
	$(CC) -O2 $(MARCH) $(CFLAGS)  $< -c -o $@
OB+=$(BUILD)/pithy/pithy.o
endif

ifneq ($(wildcard shrinker/.),)
CXXFLAGS+=-D_SHRINKER
$(BUILD)/shrinker/shrinker.o: shrinker/shrinker.c
	@mkdir -p $(dir $@)
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@
OB+=$(call obj,shrinker/Shrinker.o)
endif

ifneq ($(wildcard wlfz/.),)
CXXFLAGS+=-D_WFLZ
$(BUILD)/wflz/wfLZ.o: wflz/wfLZ.c
	@mkdir -p $(dir $@)
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@
OB+=$(BUILD)/wflz/wfLZ.o
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
$(BUILD)/lzmat/lzmat_dec.o: lzmat/lzmat_dec.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -D"__int64=long long" $(MARCH) $(CFLAGS) $< -c -o $@
$(BUILD)/lzmat/lzmat_enc.o: lzmat/lzmat_enc.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -D"__int64=long long" $(MARCH) $(CFLAGS) $< -c -o $@

OB+=$(BUILD)/lzmat/lzmat_enc.o $(BUILD)/lzmat/lzmat_dec.o
endif

ifneq ($(wildcard tornado/.),)
CXXFLAGS+=-D_TORNADO
$(BUILD)/tornado_/tormem.o: tornado_/tormem.cpp
	@mkdir -p $(dir $@)
	$(CXX) -O3 $(TORDEF) -D__x86_$(ARCH)__ -DFREEARC_$(ARCH)BIT -pipe -fforce-addr -fno-exceptions -fno-rtti -c tornado_/tormem.cpp -o $@
ifeq ($(OS), Linux)
TORDEF=-DFREEARC_UNIX -DFREEARC_INTEL_BYTE_ORDER
else
TORDEF=-DFREEARC_WIN -DFREEARC_INTEL_BYTE_ORDER -D_UNICODE -DUNICODE
endif
OB+=$(BUILD)/tornado_/tormem.o
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

OB+=$(BUILD)/plugin.o

$(BUILD)/plugin.o: plugin.cc $(C_BLOSC2_LIB) $(ISAL_LIB) $(OPENZL_LIB) $(XZ_LIB) $(ZLIB_NG_LIB) $(ZSTD_LIB) $(LZ_LIB) $(IC_LIB) 
	@mkdir -p $(dir $@)
	$(CXX) -O3 $(MARCH) $(CXXFLAGS)  $< -c -o $@


turbobench: $(OB) $(BUILD)/turbobench.o $(BUILD)/plugin.o $(BUILD)/cpu.o 
	$(CXX) $^ $(LDFLAGS) -o turbobench

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -O3 $(MARCH) $(CFLAGS) $< -c -o $@

$(BUILD)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) -O3 $(MARCH) $(CXXFLAGS)  $< -c -o $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -O3 $(MARCH) $(CXXFLAGS) $< -c -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) -O3 $(MARCH) $(CFLAGS) $< -c -o $@


ifeq ($(OS),Windows)
clean:
	rmdir /S /Q $(BUILD)
	del /S *~
	del /S *.exe
else
clean:
	rm -rf $(BUILD)
	find . -name "turbobench" -type f -delete
	find . -name "*.o" -type f -delete
	find . -name "*~" -type f -delete
	find . -name "core" -type f -delete

cleana:
	rm -rf $(BUILD)
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

