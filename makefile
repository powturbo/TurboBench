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

MAKE    ?= make
CMAKE   ?= cmake
NM      ?= nm
OBJCOPY ?= objcopy

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
FOPENMP:=
ifeq ($(HAVE_OPENMP),no)
  $(warning OpenMP not available)
endif

all: turbobench 
 
# ***************************************************************** codecs *****************************************************************************
#--- A -------------------------
AOCL_LIB:=
ifneq ($(and $(wildcard aocl-compression/.),$(filter x86_64,$(ARCH))),)
PLG_FLAGS += -D_AOCL
AOCL_SRCS := $(shell find aocl-compression -type f \( -name '*.[ch]' -o -name 'CMakeLists.txt' \))
AOCL_BDIR = $(BUILD)/aocl-compression
AOCL_LIB  = $(AOCL_BDIR)/libaocl.a
ifeq ($(OS), Windows)
AOCL_ALIB = $(AOCL_BDIR)/lib/aocl_compression.a
$(AOCL_ALIB): $(AOCL_SRCS)
	mkdir -p $(dir $@)
	$(MAKE) -C aocl-compression BUILD_STATIC_LIBS=1 BUILD_DIR=$(abspath $(AOCL_BDIR)) LIB_DIR=$(abspath $(AOCL_BDIR))/lib
else
ifeq ($(HAVE_OPENMP),yes)
  FOPENMP = -fopenmp
  AOCL_OMP = -DAOCL_ENABLE_THREADS=1 -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
#-DOpenMP_CXX_FLAGS="-fopenmp" -DOpenMP_CXX_LIB_NAMES="omp" -DOpenMP_omp_LIBRARY=/usr/lib/llvm-*/lib/libomp.so 
endif
AOCL_ALIB = $(AOCL_BDIR)/lib/libaocl_compression.a
$(AOCL_ALIB): $(AOCL_SRCS)
	$(CMAKE) -S aocl-compression -B $(AOCL_BDIR) -DCMAKE_INSTALL_PREFIX=$(AOCL_BDIR) -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=1 $(AOCL_OMP)
#		 -DCMAKE_C_FLAGS="-Wno-error=attributes -Wno-error=format -Wno-implicit-function-declaration"                 
	$(CMAKE) --build $(AOCL_BDIR) --target install -j
	@test -f $@ || (echo "ERROR: $@ was not produced by the install step"; exit 1)
endif
$(AOCL_LIB): $(AOCL_ALIB)
	mkdir -p $(dir $@)
	$(NM) -g --defined-only $< | awk '{print $$NF}' | grep -v ':$$' | grep -v '^aocl_llc_' | grep -v '^$$' | LC_ALL=C sort -u | awk 'NF{print $$1" AOCLLZB_"$$1}' > $@.redef
	$(OBJCOPY) --remove-section=".gnu.lto_*" --remove-section=".llvmcmd" --remove-section=".llvmbc" --redefine-syms=$@.redef $< $@
	rm -f $@.redef
	@test -f $@ || (echo "ERROR: failed to create $@"; exit 1)
LIBS += $(AOCL_LIB)
endif

#--- B -------------------------
ifneq ($(wildcard brotli/.),)
PLG_FLAGS+=-D_BROTLI
CXXFLAGS+=-Ibrotli/c/include 
CFLAGS+=-Ibrotli/c/include 
BROTLI_SRCS := $(wildcard brotli/c/common/*.c) $(wildcard brotli/c/dec/*.c) $(wildcard brotli/*.c) $(wildcard brotli/c/enc/*.c)
OB += $(call obj,$(BROTLI_SRCS))
endif

ifneq ($(wildcard bzip2/.),)
PLG_FLAGS+=-D_BZIP2
OB += $(call obj,bzip2/blocksort.o bzip2/huffman.o bzip2/crctable.o bzip2/randtable.o bzip2/compress.o bzip2/decompress.o bzip2/bzlib.o)
endif

ifneq ($(wildcard bzip3/.),)
PLG_FLAGS+=-D_BZIP3
CFLAGS+=-DVERSION=1 -Ibzip3/include -Wno-int-conversion
OB+=$(call obj,bzip3/src/libbz3.o)
endif

#--- C -------------------------
C_BLOSC2_LIB :=
ifneq ($(wildcard c-blosc2/.),)
ifneq ($(OS), Windows)  # not compiling for windows in CI. ar.exe ERROR
C_BLOSC2_SRCS := $(shell find c-blosc2 -type f -name '*.[c]' -o -name '*.cpp' -o -name '*.cc')
ifdef CROSS #ERROR IN C_BLOSC BUILD  
#PLG_FLAGS+=-D_C_BLOSC2
#C_BLOSC2_LIB = $(BUILD)/c-blosc2/blosc/libblosc2.a
#$(C_BLOSC2_LIB): $(C_BLOSC2_SRCS)
#	export CC=$(CROSS)-linux-gnu-gcc
#	export CXX=$(CROSS)-linux-gnu-g++
#	cmake -S c-blosc2 -B $(BUILD)/c-blosc2 -DBLOSC_ZSTD_SOURCE_DIR=zstd -DBUILD_TESTS=OFF -DBUILD_BENCHMARKS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_FUZZERS=OFF \
#	          -DPREFER_EXTERNAL_LZ4=ON -DPREFER_EXTERNAL_ZLIB=ON -DPREFER_EXTERNAL_ZSTD=ON  -DBUILD_SHARED=OFF -DBUILD_SHARED_LIBS=OFF
#	cmake --build $(BUILD)/c-blosc2
else
PLG_FLAGS+=-D_C_BLOSC2
C_BLOSC2_LIB = $(BUILD)/c-blosc2/blosc/libblosc2.a
$(C_BLOSC2_LIB): $(C_BLOSC2_SRCS)
	cmake -S c-blosc2 -B $(BUILD)/c-blosc2 -DBLOSC_ZSTD_SOURCE_DIR=zstd -DBUILD_TESTS=OFF -DBUILD_BENCHMARKS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_FUZZERS=OFF \
	          -DPREFER_EXTERNAL_LZ4=ON -DPREFER_EXTERNAL_ZLIB=ON -DPREFER_EXTERNAL_ZSTD=ON  -DBUILD_SHARED=OFF -DBUILD_SHARED_LIBS=OFF 
	cmake --build $(BUILD)/c-blosc2 --parallel 4
endif
LIBS += $(C_BLOSC2_LIB)
endif
endif

ifneq ($(wildcard ClickhouseXXX/.),)
PLG_FLAGS+=-D_CLICKHOUSE -IClickhouse/src -IClickhouse	#-IClickhouse/base/pcg_random -IContrib/abseil-cpp
OB+=$(call obj,Clickhouse/src/Compression/LZ4_decompress_faster.o)
endif

#--- F -------------------------
FIRETRAIL_LIB :=
ifneq ($(wildcard firetrail/.),)
PLG_FLAGS+=-D_FIRETRAIL
FIRETRAIL_LIB=firetrail/libfiretrail.a
LIBS += $(FIRETRAIL_LIB)
$(FIRETRAIL_LIB): firetrail/src/root.zig 
	cd firetrail && zig build-lib -O ReleaseFast -femit-bin=libfiretrail.a src/root.zig -lc
OB+=$(FIRETRAIL_LIB)	
endif
#--- G -------------------------
ifneq ($(wildcard GLZA/.),)
PLG_FLAGS+=-D_GLZA
GLZA_OBJS := $(call obj,GLZA/GLZAmodel.o GLZA/GLZAcomp.o GLZA/GLZAencode.o GLZA/GLZAcompress.o GLZA/GLZAformat.o GLZA/GLZAdecode.o)
GLZA_BUILD = $(CC) -O2 $(CFLAGS) $< -c -o $@
$(GLZA_DIR)/%.o: GLZA/%.c
	$(GLZA_BUILD)
OB += $(GLZA_OBJS) 
endif

#--- I -------------------------
IC_LIB :=
IC_DIR=../ic
ifneq ($(wildcard $(IC_DIR)/.),)
PLG_FLAGS+=-D_IC
IC_SRCS := $(shell find $(IC_DIR)/lib -type f -name '*.[c]' -name '*.[a]')
IC_LIB = $(BUILD)/ic/libic.a
$(IC_LIB):  $(IC_SRCS)
	@mkdir -p $(BUILD)/ic
	$(MAKE) $(IC_LIB) -C $(IC_DIR) BUILD=$(BUILD)/ic
LIBS += $(IC_LIB)
endif

IGUANA_LIB :=
ifneq ($(wildcard iguana/.),)
ifneq ($(filter $(ARCH),aarch64 x86_64),)
#ifeq ($(filter $(OS),Darwin Windows),)
#ifneq ($(OS),Darwin)
PLG_FLAGS += -D_IGUANA
IGUANA_DIR := iguana/iguana
IGUANA_BD := $(BUILD)/iguana
IGUANA_CC := $(CX)
ifeq ($(OS),Windows)
IGUANA_CC := clang
endif
IGUANA_FLAGS := -std=c++20 -DIGUANA_STATIC -fno-math-errno -DIGUANA_COMPILER_GNU=$(IGUANA_CC)
#-DIGUANA_EXPORTS=1 
ifeq ($(IGUANA_CC),clang)
IGUANA_FLAGS += -DIGUANA_COMPILER_CLANG
endif

OBJS_CX := $(IGUANA_BD)/ans1.o $(IGUANA_BD)/ans32.o $(IGUANA_BD)/ans_bitstream.o $(IGUANA_BD)/ans_byte_statistics.o $(IGUANA_BD)/ans_nibble.o $(IGUANA_BD)/ans_nibble_statistics.o\
           $(IGUANA_BD)/common.o $(IGUANA_BD)/decoder.o $(IGUANA_BD)/encoder.o $(IGUANA_BD)/entropy.o $(IGUANA_BD)/error.o $(IGUANA_BD)/output_stream.o
ifeq ($(ARCH),aarch64)
OBJS_CX += $(IGUANA_BD)/ans32_neon.o $(IGUANA_BD)/decoder_neon.o
endif

$(IGUANA_BD)/ans1.o:                 $(IGUANA_DIR)/ans1.cpp
$(IGUANA_BD)/ans32.o:                $(IGUANA_DIR)/ans32.cpp
$(IGUANA_BD)/ans_bitstream.o:        $(IGUANA_DIR)/ans_bitstream.cpp
$(IGUANA_BD)/ans_byte_statistics.o:  $(IGUANA_DIR)/ans_byte_statistics.cpp
$(IGUANA_BD)/ans_nibble.o:           $(IGUANA_DIR)/ans_nibble.cpp
$(IGUANA_BD)/ans_nibble_statistics.o:$(IGUANA_DIR)/ans_nibble_statistics.cpp
$(IGUANA_BD)/common.o:               $(IGUANA_DIR)/common.cpp
$(IGUANA_BD)/decoder.o:              $(IGUANA_DIR)/decoder.cpp
$(IGUANA_BD)/encoder.o:              $(IGUANA_DIR)/encoder.cpp
$(IGUANA_BD)/entropy.o:              $(IGUANA_DIR)/entropy.cpp
$(IGUANA_BD)/error.o:                $(IGUANA_DIR)/error.cpp
$(IGUANA_BD)/output_stream.o:        $(IGUANA_DIR)/output_stream.cpp
$(IGUANA_BD)/ans32_neon.o:           $(IGUANA_DIR)/ans32_neon.cpp
$(IGUANA_BD)/decoder_neon.o:         $(IGUANA_DIR)/decoder_neon.cpp
$(OBJS_CX): | $(IGUANA_BD)/iguana
	$(IGUANA_CC) -O3 $(CFLAGS) $(IGUANA_FLAGS) $(_SSE) -c $< -o $@

ifeq ($(ARCH),x86_64)
OBJS_CX512 := $(IGUANA_BD)/ans32_avx512.o
$(IGUANA_BD)/ans32_avx512.o: $(IGUANA_DIR)/ans32_avx512.cpp
$(OBJS_CX512): | $(IGUANA_BD)/iguana
	$(IGUANA_CC) -O3 $(CFLAGS) $(IGUANA_FLAGS) -mavx512vl -mavx512bw -c $< -o $@
endif

$(IGUANA_BD)/iguana:
	mkdir -p $@
#IGUANA_OBJS := $(OBJS_CX) $(OBJS_CX512)
IGUANA_LIB  := $(IGUANA_BD)/libiguana.a
$(IGUANA_LIB): $(OBJS_CX) $(OBJS_CX512) | $(IGUANA_BD)/iguana
	$(AR) rcs $@ $^
LIBS += $(IGUANA_LIB)
endif 
#endif 
endif

ISAL_LIB :=
ifneq ($(wildcard isa-l/.),)
ifndef CROSS
NASM ?= $(shell command -v nasm)
ifeq ($(NASM),)
  ifneq ($(wildcard isa-l_/$(OS)-$(ARCH)/isa-l.a),)
    PLG_FLAGS += -D_ISA_L
    ISAL_LIB := isa-l_/$(OS)-$(ARCH)/isa-l.a    
  endif     
else
PLG_FLAGS += -D_ISA_L
ISAL_SRCS := $(shell find isa-l -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.asm' \))
ISAL_LIB := $(BUILD)/bin/isa-l.a 
$(ISAL_LIB): $(ISAL_SRCS)
	@mkdir -p $(BUILD)/isa-l
	@mkdir -p $(BUILD)/bin
	@mkdir -p isa-l/bin
	$(MAKE) -C isa-l -f Makefile.unx O=$(abspath $(BUILD)/isa-l)
	@mv isa-l/bin/isa-l.a $@
endif
LIBS += $(ISAL_LIB)
endif
endif
#--- K ---------------------------
ifneq ($(wildcard kanzi-cpp/.),)
PLG_FLAGS+=-D_KANZI
KANZI_DIR = kanzi-cpp/src
KANZI_SRCS := $(wildcard $(KANZI_DIR)/io/*.cpp) $(wildcard $(KANZI_DIR)/entropy/*.cpp) $(wildcard $(KANZI_DIR)/bitstream/*.cpp) $(wildcard $(KANZI_DIR)/*.cpp) $(wildcard $(KANZI_DIR)/transform/*.cpp)
OB += $(call obj,$(KANZI_SRCS))
endif
#---- L -----------------------
ifneq ($(wildcard lib/.),)
PLG_FLAGS+=-D_LIB
endif
ifneq ($(wildcard libbsc/.),)
PLG_FLAGS+=-D_LIBBSC
LIBBSC_CFLAGS = -O3 -D_LIBBSC -DLIBBSC_SORT_TRANSFORM_SUPPORT -ICSC/src/libcsc 
LIBBSC_LDFLAGS :=
ifeq ($(HAVE_OPENMP),yes)
  LIBBSC_CFLAGS  += -fopenmp -DLIBBSC_OPENMP_SUPPORT -DLIBSAIS_OPENMP
  FOPENMP = -fopenmp
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
PLG_FLAGS+=-D_LIBDEFLATE
CFLAGS+=-Ilibdeflate -Ilibdeflate/common
LIBDEFLATE_SRCS := $(wildcard libdeflate/lib/*.c) libdeflate/lib/arm/cpu_features.c libdeflate/lib/x86/cpu_features.c 
OB += $(call obj,$(LIBDEFLATE_SRCS))
endif

ifneq ($(wildcard libslz/.),)
PLG_FLAGS+=-D_LIBSLZ
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
PLG_FLAGS+=-D_LZ4
CFLAGS+=-Ilz4/lib
OB+=$(call obj,lz4/lib/lz4hc.o lz4/lib/lz4.o lz4/lib/lz4frame.o lz4/lib/xxhash.o)
endif

ifneq ($(wildcard lzav/.),)
PLG_FLAGS+=-D_LZAV
endif

ifneq ($(wildcard lzfse/.),)
PLG_FLAGS+=-D_LZFSE
OB+=$(call obj,lzfse/src/lzfse_decode_base.o lzfse/src/lzfse_decode.o lzfse/src/lzfse_encode_base.o lzfse/src/lzfse_encode.o lzfse/src/lzfse_fse.o lzfse/src/lzvn_decode_base.o lzfse/src/lzvn_encode_base.o)
endif

ifneq ($(wildcard lzham_codec_devel/.),)
ifneq ($(OS),$(filter $(OS),Darwin))
PLG_FLAGS+=-D_LZHAM 
CXXFLAGS+=-D"UINT64_MAX=-1ull" -Ilzham_codec_devel/include -Ilzham_codec_devel/lzhamcomp -Ilzham_codec_devel/lzhamdecomp
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

LZ_LIB :=
LZ_DIR=../lz
ifneq ($(wildcard $(LZ_DIR)/.),)
PLG_FLAGS+=-D_LZ
LZ_SRCS := $(shell find $(LZ_DIR)/lib -type f -name '*.[c]')
LZ_LIB = $(BUILD)/lz/liblz.a
$(LZ_LIB):  $(LZ_SRCS)
	@mkdir -p $(BUILD)/lz
	$(MAKE) $(LZ_LIB) -C $(LZ_DIR) BUILD=$(BUILD)/lz
LIBS += $(LZ_LIB)
CFLAGS  += -D_NQUANT
endif

ifneq ($(wildcard lzlib-1.16/.),)
PLG_FLAGS+=-D_LZLIB
OB+=$(call obj,lzlib-1.16/lzlib.o lzlib_/bbexample.o)
endif

ifneq ($(wildcard lzma/.),)
PLG_FLAGS+=-D_LZMA
CFLAGS+=-D_7ZIP_ST
CXXFLAGS+=-D_7Z_TYPES_
LZMA_SRCS := lzma/C/Alloc.c lzma/C/CpuArch.c lzma/C/LzFind.c lzma/C/LzmaDec.c lzma/C/LzmaEnc.c lzma/C/LzmaLib.c lzma/C/Threads.c lzma/C/LzFindMt.c lzma/C/LzFindOpt.c
OB += $(call obj,$(LZMA_SRCS))
endif

ifneq ($(wildcard lzo/.),)
PLG_FLAGS+=-D_LZO
CXXFLAGS+=-Ilzo/include
CFLAGS+=-Ilzo/include
LZO_SRCS := $(wildcard lzo/src/*.c)
OB += $(call obj,$(LZO_SRCS))
endif

ifneq ($(and $(wildcard LZSSE/.),$(filter x86_64,$(ARCH))),)
PLG_FLAGS += -D_LZSSE
OB += $(addprefix $(BUILD)/LZSSE/, lzsse2/lzsse2.o lzsse4/lzsse4.o lzsse8/lzsse8.o)
$(BUILD)/LZSSE/%.o: LZSSE/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -O2 -msse4.1 -std=c++11 $< -c -o $@
endif

#---- M -----------------------
ifneq ($(wildcard memlz/.),)
PLG_FLAGS+=-D_MEMLZ
endif

MINIZ_LIB:=
ifneq ($(wildcard miniz/.),)
PLG_FLAGS+=-D_MINIZ
MINIZ_SRCS := $(shell find miniz -type f -name '*.[ch]' -o -name 'CMakeLists.txt')
MINIZ_LIB = $(BUILD)/miniz/libminiz.a
$(MINIZ_LIB): $(MINIZ_SRCS)
	$(CMAKE) -S miniz -B $(BUILD)/miniz -DCMAKE_INSTALL_PREFIX=$(BUILD) && make -C $(BUILD)/miniz
LIBS += $(MINIZ_LIB)
endif

ifneq ($(wildcard misa77/.),)
PLG_FLAGS += -D_MISA77
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
PLG_FLAGS += -D_MISA77
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

#---- O -----------------------
OPENZL_LIB :=
ifneq ($(wildcard openzl/.),)
ifneq ($(OPENZL), 0)
OPENZL_SRCS := $(shell find openzl -type f -name '*.[ch]' -o -name '*.cpp' -o -name '*.cc' -o -name 'CMakeLists.txt' -o -name 'Makefile')
ifdef CROSS  # NOTWORKING
#OPENZL_LIB = $(BUILD)/openzl/libopenzl.a
#$(OPENZL_LIB): $(OPENZL_SRCS)
#	export CC=$(CROSS)-linux-gnu-gcc
#	export CXX=$(CROSS)-linux-gnu-g++
#	cmake -S openzl -B $(BUILD)/openzl -DCMAKE_C_COMPILER=$(CROSS)-linux-gnu-gcc -DCMAKE_CXX_COMPILER=$(CROSS)-linux-gnu-g++
#	cmake --build $(BUILD)/openzl --config Release
else
PLG_FLAGS += -D_OPENZL
CXXFLAGS += -Iopenzl/include -Iopenzl/src
ifeq ($(OS), Windows)
OPENZL_LIB = openzl/libopenzl.a
$(OPENZL_LIB): $(OPENZL_SRCS)
	cd openzl && $(MAKE) lib
else
OPENZL_LIB = $(BUILD)/openzl/libopenzl.a
$(OPENZL_LIB): $(OPENZL_SRCS)
	cmake -S openzl -B $(BUILD)/openzl -DOPENZL_ALLOW_INTROSPECTION=OFF -DOPENZL_INSTALL=OFF -DOPENZL_BUILD_CPP=OFF -DOPENZL_BUILD_CUSTOM_PARSERS=OFF -DOPENZL_BUILD_TOOLS=OFF -DOPENZL_BUILD_CLI=OFF -DOPENZL_BUILD_EXAMPLES=OFF
	cmake --build $(BUILD)/openzl --config Release
endif
endif
LIBS += $(OPENZL_LIB)
endif
endif

# 'oo2core_9_win64.dll', 'liboo2corelinuxarm64.so.9' or 'liboo2corelinux64.so.9' must be available the current directory
# ONLY FOR BENCHMARKING: download corresponding library from https://github.com/WorkingRobot/OodleUE
PLG_FLAGS+=-D_OODLE
ifneq ($(and $(wildcard pivco-huffman/.),$(wildcard OodleUE/.)),)
OODLE_DIR := OodleUE/Engine/Source/Runtime/OodleDataCompression/Sdks/2.9.16
CXXFLAGS += -I$(OODLE_DIR)/src/oodle2/core -I$(OODLE_DIR)/src/oodle2/core/public -DOODLE_IMPORT_LIB
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
LIBS+=$(OODLE_STATIC_LIB)
endif

#--- P -------------------------
#ifneq ($(wildcard pcodec_/.),)
#endif

#--- S -------------------------
SNAPPY_LIB := 
ifneq ($(wildcard snappy/.),)
PLG_FLAGS+=-D_SNAPPY
CXXFLAGS+=-I$(BUILD)/snappy
ifneq ($(ARCH),x86_64)
SNAPPY_CMAKEFLAGS = -DSNAPPY_REQUIRE_AVX=ON -DSNAPPY_REQUIRE_AVX2=ON 
endif
SNAPPY_SRCS := $(shell find snappy -type f -name '*.[ch]' -o -name '*.[cc]' )
SNAPPY_LIB := $(BUILD)/snappy/libsnappy.a
$(SNAPPY_LIB): $(SNAPPY_SRCS)
	$(CMAKE) -S snappy -B $(BUILD)/snappy -DSNAPPY_BUILD_TESTS=OFF -DSNAPPY_BUILD_BENCHMARKS=OFF -DSNAPPY_INSTALL=OFF && $(MAKE) -C $(BUILD)/snappy 
LIBS += $(SNAPPY_LIB)
endif

#--- T -------------------------
ifneq ($(wildcard tamp/.),)
PLG_FLAGS+=-D_TAMP
TAMP_DIR = tamp/tamp/_c_src/tamp
OB += $(call obj,$(TAMP_DIR)/common.o $(TAMP_DIR)/compressor.o $(TAMP_DIR)/decompressor.o)
endif

ifneq ($(wildcard Turbo-Range-Coder/.),)
ifneq ($(ARCH),loongarch64)
RC_DIR  := Turbo-Range-Coder
BUILD_DATE := $(shell date +%Y%m%d)
PLG_FLAGS += -D_TURBORC
CFLAGS   += -D_ANS -D_BWT -I$(RC_DIR)/libsais/include -DBUILD_VERSION="\"v$(BUILD_DATE)\"" 
RC_BDIR := $(BUILD)/$(RC_DIR)
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

#--- X -------------------------
XZ_LIB :=
ifneq ($(wildcard xz/.),)
PLG_FLAGS += -D_XZ
XZ_SRCS := $(shell find xz/src/liblzma -type f -name '*.[c]' -o -name '*.cpp' -o -name '*.cc')
XZ_LIB = $(BUILD)/xz/liblzma.a
ifdef CROSS
$(XZ_LIB): $(XZ_SRCS)
	export CC=$(CROSS)-linux-gnu-gcc && cmake -S xz -B $(BUILD)/xz && $(MAKE) -C $(BUILD)/xz
else
$(XZ_LIB): $(XZ_SRCS)
	cmake -S xz -B $(BUILD)/xz && $(MAKE) -C $(BUILD)/xz
endif
LIBS += $(XZ_LIB)
endif

#--- Z -------------------------
ifneq ($(wildcard zlib/.),)
PLG_FLAGS+=-D_ZLIB
ZD=zlib/
OB+=$(call obj,$(ZD)adler32.o $(ZD)crc32.o $(ZD)compress.o $(ZD)deflate.o $(ZD)infback.o $(ZD)inffast.o $(ZD)inflate.o $(ZD)inftrees.o $(ZD)trees.o $(ZD)uncompr.o $(ZD)zutil.o)
endif

ZLIB_NG_LIB :=
ifneq ($(wildcard zlib-ng/.),)
PLG_FLAGS += -D_ZLIB_NG
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
LIBS += $(ZLIB_NG_LIB)
endif

ifneq ($(wildcard zopfli/.),)
PLG_FLAGS+=-D_ZOPFLI
ZOPFLI_SRCS := $(wildcard zopfli/src/zopfli/*.c) 
ZOPFLI_SRCS := $(filter-out %/zopfli_bin.c, $(ZOPFLI_SRCS))
OB += $(call obj,$(ZOPFLI_SRCS))
endif

ifneq ($(wildcard zstd/.),)
PLG_FLAGS+=-D_ZSTD
CXXFLAGS+=-Izstd/lib -Izstd/lib/common
CFLAGS+=-Izstd/lib -Izstd/lib/common
ZSTD_SRCS := $(wildcard zstd/lib/common/*.c) $(wildcard zstd/lib/compress/*.c) $(wildcard zstd/lib/decompress/*.c) $(wildcard zstd/lib/decompress/*.S) $(wildcard zstd/lib/dictBuilder/*.c)
ZSTD_C_SRCS := $(filter %.c,$(ZSTD_SRCS))
ZSTD_S_SRCS := $(filter %.S,$(ZSTD_SRCS))
ZSTD_OBJS := $(call obj,$(ZSTD_C_SRCS)) $(ZSTD_S_SRCS)
OB += $(ZSTD_OBJS)
endif

ifneq ($(wildcard zpaq/.),)
ifneq ($(OS),Darwin)
PLG_FLAGS+=-D_ZPAQ
CXXFLAGS+=-Izpaq
ifeq ($(HAVE_OPENMP),yes)
$(BUILD)/libzpaq_omp.cpp: zpaq/libzpaq.cpp
	(echo '#include <omp.h>'; cat $<) > $@
CXXFLAGS+=-fopenmp
FOPENMP = -fopenmp
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
PLG_FLAGS += -D_ZXC 
#CFLAGS+=-DZXC_STATIC_DEFINE
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
PLG_FLAGS+=-D_BRIEFLZ
CFLAGS+=-Ibrieflz/include
OB+=$(call obj,brieflz/src/brieflz.o brieflz/src/depack.o)
endif

ifneq ($(wildcard fast-lzma2/.),)
PLG_FLAGS+=-D_FLZMA2
FLZMA2_SRCS := $(wildcard fast-lzma2/*.c) 
FLZMA2_SRCS := $(filter-out %/xxhash.c, $(FLZMA2_SRCS))
FLZMA2_OBJS := $(call obj,$(FLZMA2_SRCS))
OB += $(FLZMA2_OBJS)
endif

ifneq ($(wildcard lz4ultra/.),)
PLG_FLAGS+=-D_LZ4ULTRA
CXXFLAGS+=-Ilz4ultra/src -Ilz4ultra/src/libdivsufsort/include
OB+=$(call obj,lz4ultra/src/shrink_inmem.o lz4ultra/src/expand_inmem.o lz4ultra/src/shrink_block.o lz4ultra/src/expand_block.o lz4ultra/src/shrink_context.o lz4ultra/src/matchfinder.o lz4ultra/src/frame.o)
ifeq ($(DIVSORT), 1)
else
OB+=$(call obj,lz4ultra/src/libdivsufsort/lib/divsufsort.o lz4ultra/src/libdivsufsort/lib/sssort.o lz4ultra/src/libdivsufsort/lib/trsort.o)
DIVSORT=1
endif
endif

ifneq ($(wildcard lzsa/.),)
PLG_FLAGS+=-D_LZSA
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
PLG_FLAGS+=-D_SNAPPY_C
OB+=$(call obj,snappy-c/snappy.o snappy-c/util.o)
endif

ifneq ($(wildcard gipfeli/.),)
PLG_FLAGS+=-D_GIPFELI
OB+=$(call obj,gipfeli/lz77.o gipfeli/entropy.o gipfeli/entropy_code_builder.o gipfeli/decompress.o gipfeli/gipfeli-internal.o)
endif

ifneq ($(wildcard tcobs/.),)
PLG_FLAGS+=-D_TCOBS
CXXFLAGS+=-Drestrict=__restrict
OB+=$(call obj,tcobs/v2/tcobsEncode.o tcobs/v2/tcobsDecode.o)
endif

ifdef SMALLZ4
PLG_FLAGS+=-DSMALLZ4
endif

ifneq ($(wildcard Unishox2/.),)
PLG_FLAGS+=-D_UNISHOX2
OB+=$(call obj,Unishox2/unishox2.o turbobench_/unishox.o)
CXXFLAGS+=-Imarisa-trie/include
OB+=$(call obj,Unishox2/Unishox3_Alpha/unishox3.o)
endif

#------------------------- Entropy coder -----------------------------------------
# First download or clone aomedia (git clone https://aomedia.googlesource.com/aom) into TurboBench directory
# after cmake, put the generated "aom_config.h" into the aom directory
# or copy aom_/aom_config.h to aom
ifneq ($(wildcard EC/aom/.),)
PLG_FLAGS+=-D_AOM
OB+=$(call obj,EC/aom_/aom.o EC/aom/aom_dsp/entenc.o EC/aom/aom_dsp/entdec.o EC/aom/aom_dsp/entcode.o)
#OB+=daala_/daala.o
endif

# First download or clone daala (https://github.com/xiph/daala) into TurboBench directory
ifneq ($(wildcard EC/daala/.),)
PLG_FLAGS+=-D_DAALA
OB+=$(call obj,EC/daala_/daala.o)
endif

ifneq ($(wildcard EC/fastac/.),)
PLG_FLAGS+=-D_FASTAC
OB+=$(call obj,EC/fastac/arithmetic_codec.o)
endif

ifneq ($(wildcard EC/fasthf/.),)
PLG_FLAGS+=-D_FASTHF
OB+=$(call obj,EC/fasthf/binary_codec.o)
endif

FSE := EC/fse
ifneq ($(wildcard FSE/.),)
PLG_FLAGS+=-D_FSE
OB+=$(call obj,$(LB)EC/fse/fse_compress_.o $(LB)EC/fse/fse_decompress_.o)
endif

ifneq ($(wildcard EC/fpaq0p/.),)
PLG_FLAGS+=-D_FPAQ0P
OB+=$(call obj,EC/fpaq0p/fpaq0p_sh.o)
endif

ifneq ($(wildcard EC/FPC/.),)
PLG_FLAGS+=-D_FPC
OB+=$(call obj,EC/FPC/fpc.o)
endif

ifdef FREQTAB
PLG_FLAGS+=-D_FREQTAB

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
PLG_FLAGS+=-D_GANS
OB+=$(call obj,EC/rans.o EC/head_cbloom.o)
endif

ifneq ($(wildcard pivco-huffman/.),)
ifndef CROSS
PIVCODIR   = pivco-huffman
CXXFLAGS  += -D_PIVCOHUF
CXXFLAGS  += -I$(PIVCODIR)/include
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
PLG_FLAGS     += -D_PHAZ
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
PLG_FLAGS+=-D_RECIPARITH
OB+=$(call obj,EC/recip_arith_/reciparith.o)
endif

ifneq ($(and $(wildcard EC/sserangecoding/.),$(filter x86_64,$(ARCH))),)
$(BUILD)/EC/sserangecoding/sserangecoder.o: EC/sserangecoding/sserangecoder.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -O3 $(CFLAGS) -march=corei7-avx -mtune=corei7-avx -mno-aes EC/sserangecoding/sserangecoder.cpp -o $@

PLG_FLAGS+=-D_SSERC
OB+=$(BUILD)/EC/sserangecoding/sserangecoder.o
endif

ifneq ($(wildcard EC/subotin/.),)
PLG_FLAGS+=-D_SUBOTIN
OB+=$(call obj,EC/subotin_/subotin.o)
endif

ifneq ($(wildcard EC/vecrc/.),)
PLG_FLAGS+=-D_VECRC
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
PLG_FLAGS+=-D_TURBORLE

TRLEDIR = Turbo-Run-Length-Encoding
BUILD_TRLE = $(CC) -O3 $(TRLE_FLAGS) $< -c -o $@

$(BUILD)/$(TRLEDIR)/%.o: TRLE_FLAGS = $(_AVX2) -w -fstrict-aliasing -falign-loops=32 $(DEBUG)
$(BUILD)/$(TRLEDIR)/%.o: $(TRLEDIR)/%.c
	@mkdir -p $(dir $@)
	$(BUILD_TRLE)

OB+=$(BUILD)/$(TRLEDIR)/trlec.o $(BUILD)/$(TRLEDIR)/trled.o

PLG_FLAGS+=-D_MRLE
OB+=$(call obj,Turbo-Run-Length-Encoding/ext/mrle.o)
endif
endif

ifneq ($(wildcard hypersonic-rle-kit/.),)
PLG_FLAGS+=-D_HRLE
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
PLG_FLAGS+=-D_CHAMELEON
OB+=$(call obj,chameleon/chameleon.o)
endif

ifneq ($(wildcard density/.),)
PLG_FLAGS+=-D_DENSITY
OB+=$(call obj,density/src/buffers/buffer.o density/src/algorithms/algorithms.o density/src/algorithms/dictionaries.o density/src/structure/header.o density/src/globals.o density/src/buffers/buffer.o \
	density/src/algorithms/chameleon/core/chameleon_decode.o density/src/algorithms/chameleon/core/chameleon_encode.o \
	density/src/algorithms/lion/core/lion_decode.o density/src/algorithms/lion/core/lion_encode.o density/src/algorithms/lion/forms/lion_form_model.o \
	density/src/algorithms/cheetah/core/cheetah_decode.o density/src/algorithms/cheetah/core/cheetah_encode.o)
endif

ifneq ($(wildcard lzjody/.),)
PLG_FLAGS+=-D_LZJODY
OB+=$(call obj,lzjody/lzjody.o lzjody/byteplane_xfrm.o)
endif

ifneq ($(wildcard lzoma_/.),)
PLG_FLAGS+=-D_LZOMA
OB+=$(call obj,lzoma_/pack.o lzoma_/unpack.o lzoma_/divsufsort.o)
endif

ifneq ($(wildcard xpack/.),)
PLG_FLAGS+=-D_XPACK
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
PLG_FLAGS+=-D_PITHY
$(BUILD)/pithy/pithy.o: pithy/pithy.c
	@mkdir -p $(dir $@)
	$(CC) -O2 $(MARCH) $(CFLAGS)  $< -c -o $@
OB+=$(BUILD)/pithy/pithy.o
endif

ifneq ($(wildcard shrinker/.),)
PLG_FLAGS+=-D_SHRINKER
$(BUILD)/shrinker/shrinker.o: shrinker/shrinker.c
	@mkdir -p $(dir $@)
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@
OB+=$(call obj,shrinker/Shrinker.o)
endif

ifneq ($(wildcard wlfz/.),)
PLG_FLAGS+=-D_WFLZ
$(BUILD)/wflz/wfLZ.o: wflz/wfLZ.c
	@mkdir -p $(dir $@)
	$(CC) -O2 $(MARCH) $(CFLAGS) $< -c -o $@
OB+=$(BUILD)/wflz/wfLZ.o
endif

ifneq ($(wildcard FastLZ/.),)
PLG_FLAGS+=-D_FASTLZ
OB+=$(call obj,FastLZ/fastlz.o)
endif

ifneq ($(wildcard heatshrink_/.),)
PLG_FLAGS+=-D_HEATSHRINK
OB+=$(call obj,heatshrink_/heatshrink.o heatshrink/heatshrink_encoder.o heatshrink/heatshrink_decoder.o)
endif

ifneq ($(wildcard liblzf/.),)
PLG_FLAGS+=-D_LIBLZF
OB+=$(call obj,liblzf/lzf_c.o liblzf/lzf_c_best.o liblzf/lzf_d.o)
endif

ifneq ($(wildcard liblzg/.),)
PLG_FLAGS+=-D_LIBLZG
OB+=$(call obj,liblzg/src/lib/encode.o liblzg/src/lib/decode.o liblzg/src/lib/checksum.o)
endif

ifneq ($(wildcard shoco/.),)
PLG_FLAGS+=-D_SHOCO
OB+=$(call obj,shoco/shoco.o)
endif

ifneq ($(wildcard smaz/.),)
PLG_FLAGS+=-D_SMAZ
OB+=$(call obj,smaz/smaz.o)
endif

ifneq ($(wildcard yappy/.),)
PLG_FLAGS+=-D_YAPPY
OB+=$(call obj,yappy/yappy.o)
endif

ifneq ($(wildcard CSC/.),)
PLG_FLAGS+=-D_CSC
OB+=$(call obj,CSC/src/libcsc/csc_analyzer.o CSC/src/libcsc/csc_coder.o CSC/src/libcsc/csc_dec.o CSC/src/libcsc/csc_default_alloc.o CSC/src/libcsc/csc_enc.o CSC/src/libcsc/csc_encoder_main.o CSC/src/libcsc/csc_filters.o CSC/src/libcsc/csc_lz.o CSC/src/libcsc/csc_memio.o \
	CSC/src/libcsc/csc_mf.o CSC/src/libcsc/csc_model.o CSC/src/libcsc/csc_profiler.o)
endif

ifneq ($(wildcard doboz/.),)
PLG_FLAGS+=-D_DOBOZ
OB+=$(call obj,doboz/Source/Doboz/Compressor.o doboz/Source/Doboz/Decompressor.o doboz/Source/Doboz/Dictionary.o)
endif

ifneq ($(wildcard libzling/.),)
PLG_FLAGS+=-D_LIBZLING
# Disabled : compile error in gcc 7.2
#OB+=libzling/src/libzling.o libzling/src/libzling_huffman.o libzling/src/libzling_utils.o libzling/src/libzling_lz.o libzling_/libzling_utils_mem.o
endif

ifneq ($(wildcard Behemoth-Rank-Coding/.),)
PLG_FLAGS+=-D_BRC
OB+=$(call obj,Behemoth-Rank-Coding/brc.o)
endif
#----------------------- GPL -------------------------
ifneq ($(wildcard lzmat/.),)
PLG_FLAGS+=-DLZMAT
$(BUILD)/lzmat/lzmat_dec.o: lzmat/lzmat_dec.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -D"__int64=long long" $(MARCH) $(CFLAGS) $< -c -o $@
$(BUILD)/lzmat/lzmat_enc.o: lzmat/lzmat_enc.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -D"__int64=long long" $(MARCH) $(CFLAGS) $< -c -o $@

OB+=$(BUILD)/lzmat/lzmat_enc.o $(BUILD)/lzmat/lzmat_dec.o
endif

ifneq ($(wildcard tornado/.),)
PLG_FLAGS+=-D_TORNADO
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
PLG_FLAGS+=-D_MSCOMPRESS
OB+=$(call obj,ms-compress/src/mscomp.o ms-compress/src/lznt1_compress.o ms-compress/src/lznt1_decompress.o ms-compress/src/xpress_compress.o ms-compress/src/xpress_decompress.o ms-compress/src/xpress_huff_compress.o ms-compress/src/xpress_huff_decompress.o)
endif

ifneq ($(wildcard quicklz_/.),)
PLG_FLAGS+=-D_QUICKLZ
OB+=$(call obj,quicklz_/quicklz1.o quicklz_/quicklz2.o quicklz_/quicklz3.o)
endif

ifneq ($(wildcard pysap/.),)
PLG_FLAGS+=-D_PYSAP
OB+=$(call obj,pysap/pysapcompress/vpa105CsObjInt.o pysap/pysapcompress/vpa106cslzc.o pysap/pysapcompress/vpa107cslzh.o pysap/pysapcompress/vpa108csulzh.o)
endif
#--------------------------------------------------------------------
OB+=$(BUILD)/plugin.o

$(BUILD)/plugin.o: plugin.cc $(LIBS) 
	@mkdir -p $(dir $@)
	$(CXX) -O3 $(MARCH) $(PLG_FLAGS) $(CXXFLAGS) -std=c++20  $< -c -o $@

turbobench: $(OB) $(BUILD)/turbobench.o $(BUILD)/plugin.o $(BUILD)/cpu.o $(LIBS)
	$(CXX) $^ $(LDFLAGS) $(LIBS) $(FOPENMP) -o turbobench

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

#------ Archive ---------------------
ifneq ($(wildcard iguanacmake/.),)
ifeq ($(ARCH),x86_64)
#ifneq ($(filter $(ARCH),aarch64 x86_64),)
CXXFLAGS+=-D_IGUANA
IGUANA_SRCS := $(shell find miniz -type f -name '*.[ch]' -o -name 'CMakeLists.txt')
IGUANA_LIB = $(BUILD)/iguana/libiguana.a
ifeq ($(ARCH),x86_64)
IGUANA_FLAGS=-mavx512vl -mavx512bw
else
IGUANA_FLAGS=$(_SSE)
endif
$(IGUANA_LIB): $(IGUANA_SRCS)
	cp turbobench_/iguana/CMakeLists.txt iguana
	$(CMAKE) -S iguana -B $(BUILD)/iguana -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DIGUANA_STATIC=1 -DCMAKE_CXX_FLAGS='-DIGUANA_COMPILER_GNU="g++" $(IGUANA_FLAGS) -std=c++20' -DCMAKE_INSTALL_PREFIX=$(BUILD) && make -C $(BUILD)/iguana
LIBS += $(IGUANA_LIB)
endif
$(RC_BDIR)/anscdfs.o:               $(RC_DIR)/anscdfs.c
$(RC_BDIR)/rc_ss.o:                 $(RC_DIR)/rc_ss.c
$(RC_BDIR)/rc_s.o :                 $(RC_DIR)/rc_s.c
$(RC_BDIR)/rccdf.o:                 $(RC_DIR)/rccdf.c
$(RC_BDIR)/rcutil.o:                $(RC_DIR)/rcutil.c
$(RC_BDIR)/bec_b.o:                 $(RC_DIR)/bec_b.c
$(RC_BDIR)/rccm_s.o:                $(RC_DIR)/rccm_s.c 
$(RC_BDIR)/rccm_ss.o :              $(RC_DIR)/rccm_ss.c
$(RC_BDIR)/rcqlfc_s.o:              $(RC_DIR)/rcqlfc_s.c
$(RC_BDIR)/rcqlfc_ss.o:             $(RC_DIR)/rcqlfc_ss.c
$(RC_BDIR)/rcqlfc_sf.o:             $(RC_DIR)/rcqlfc_sf.c  
$(RC_BDIR)/rcbwt.o:                 $(RC_DIR)/rcbwt.c
$(RC_BDIR)/libsais/src/libsais16.o: $(RC_DIR)/libsais/src/libsais16.c
RC_FLAGS := -falign-loops=32 -w
$(RC_OBJ): | $(RC_BDIR)
	$(CC) -O3 $(CFLAGS) $(RC_FLAGS) $(_SSE)  -c $< -o $@

endif

