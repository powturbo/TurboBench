##mkdir -p /tmp/ram && sudo mount -t tmpfs -o size=8G tmpfs /tmp/ram/
CC ?= gcc
CXX ?= g++
#CC=clang
#CXX=clang++
#----- Compile for 'American Fuzzy Lop (http://lcamtuf.coredump.cx/afl/) 
# add '-static' to LDFLAG
# Run turbobench w/o output : afl-fuzz -i testcase_dir -o finding_dir -- ./turbobench -elzturbo,12 -v0 -gg -k0
#CC=~/b/afl/afl-gcc 
#CXX=~/b/afl/afl-g++
#------------------------------------------------
ifeq ($(OS),Windows_NT)
  UNAME := Windows
CC=gcc
CXX=g++
else
  UNAME := $(shell uname -s)
ifeq ($(UNAME),$(filter $(UNAME),Linux FreeBSD GNU/kFreeBSD))
LDFLAGS+=-lpthread -lrt -lm
endif
endif

ifeq ($(STATIC),1)
LDFLAGS+=-static
endif

#DDEBUG=-g -DUSE_WRAP
#DDEBUG=-DDEBUG -DDMALLOC -g
#DDEBUG=-g
#DDEBUG=-DNDEBUG -g
DDEBUG=-DNDEBUG -s 


ifeq ($(UNAME), Windows)
LDFLAGS+=-Wl,--stack,8194304 -lpthread
endif
#-static-libgcc
#LIBJPG=packJPG.dll

MARCH=-march=native
#MARCH=-msse3
#MARCH=-march=core2
#MARCH=-march=corei7-avx -mtune=corei7-avx -ffast-math
#MARCH=-march=broadwell
#MARCH=-march=armv8-a
#CFLAGS+=-ffast-math -fstrict-aliasing
#CFLAGS+=-minline-all-stringops
# -fomit-frame-pointer 
#CXXFLAGS+=-std=c++11
# -I/..wimlib/include 
L=
#../../dev/lz/
U=../util/
IC=../ic/
#DEV=../dev/

CFLAGS+=$(DDEBUG) -w -std=gnu99 -fpermissive -Wall 

all: lzturbo
	
$(L)lzcd.o: $(L)lzcd.c
	$(CC) -O3 $(DDEBUG) $(MARCH) -falign-loops=32 -fstrict-aliasing -w -fpermissive -Wall -c $(L)lzcd.c -o $(L)lzcd.o
#-minline-all-stringops 

$(L)lz8d.o: $(L)lz8d.c $(L)lz8_.h 
	$(CC) -O2 $(DDEBUG) $(MARCH) -falign-loops=32 -fomit-frame-pointer  -w -fpermissive -Wall -c $(L)lz8d.c -o $(L)lz8d.o

$(L)lzbd.o: $(L)lzbd.c
	$(CC) -O2 $(DDEBUG) $(MARCH) -falign-loops=32 -fno-strict-aliasing -w -fpermissive -Wall -c $(L)lzbd.c -o $(L)lzbd.o
#-minline-all-stringops 

$(L)lzbc0.o: $(L)lzbc0.c
	$(CC) -O2 $(DDEBUG) $(MARCH) -falign-loops=32 -fstrict-aliasing -w -fpermissive -Wall -c $(L)lzbc0.c -o $(L)lzbc0.o
#-minline-all-stringops 

$(L)mhd.o: $(L)mhd.c 
	$(CC) -O3 $(DDEBUG) $(MARCH) -falign-loops=32 -w -fpermissive -Wall -c $(L)mhd.c -o $(L)mhd.o

$(L)mhc.o: $(L)mhc.c 
	$(CC) -O3 $(DDEBUG) $(MARCH) -falign-loops=32 -w -fpermissive -Wall -c $(L)mhc.c -o $(L)mhc.o

$(L)lzhd.o: $(L)lzhd.c
	$(CC) -O3 $(DDEBUG) $(MARCH) -falign-loops=32 -fstrict-aliasing -w -fpermissive -Wall -c $(L)lzhd.c -o $(L)lzhd.o

$(L)lzrd.o: $(L)lzrd.c
	$(CC) -O3 $(DDEBUG) $(MARCH) -falign-loops=32 -w -fpermissive -Wall -c $(L)lzrd.c -o $(L)lzrd.o

$(L)lzbwt.o: $(L)lzbwt.c
	$(CC) -O3 $(DDEBUG) $(MARCH) -falign-loops=32 -w -fpermissive -Wall -c $(L)lzbwt.c -o $(L)lzbwt.o

$(L)lzhc0.o: $(L)lzhc0.c
	$(CC) -O3 $(DDEBUG) $(MARCH) -falign-loops=32 -w -fpermissive -Wall -c $(L)lzhc0.c -o $(L)lzhc0.o

SSE41=-march=core2 -msse4.1 -mtune=core2
AVX=-march=corei7-avx
AAVX2=-march=core-avx2

$(L)ansc_simd.o: $(L)ansc_simd.c $(L)ans_.h 
	$(CC) -c -O3 $(SSE41) $(CFLAGS) -falign-loops=32 $(L)ansc_simd.c -o $(L)ansc_simd.o 

$(L)ansd_simd.o: $(L)ansd_simd.c $(L)ans_.h 
	$(CC) -c -O3 $(SSE41) $(CFLAGS) -falign-loops=32 $(L)ansd_simd.c -o $(L)ansd_simd.o  

$(L)ansc_simda.o: $(L)ansc_simd.c $(L)ans_.h 
	$(CC) -c -O3 $(AVX) $(CFLAGS)  -falign-loops=32 $(L)ansc_simd.c -o $(L)ansc_simda.o 

$(L)ansd_simda.o: $(L)ansd_simd.c $(L)ans_.h 
	$(CC) -c -O2 $(AVX) $(CFLAGS) -falign-loops=32 $(L)ansd_simd.c -o $(L)ansd_simda.o 

$(L)ansc_avx2.o: $(L)ansc_avx2.c $(L)ans_.h 
	$(CC) -c -O3 $(AAVX2) $(CFLAGS) -falign-loops=32 -fstrict-aliasing $(L)ansc_avx2.c -o $(L)ansc_avx2.o  

$(L)ansd_avx2.o: $(L)ansd_avx2.c $(L)ans_.h 
	$(CC) -c -O3 $(AAVX2) $(CFLAGS) -falign-loops=32 $(L)ansd_avx2.c -o $(L)ansd_avx2.o 

$(IC)vp4d.o: $(IC)vp4d.c
	$(CC) -O3 $(AAVX2) $(MARCH) -falign-loops=32  $< -c -o $@ 

$(IC)vp4c.o: $(IC)vp4c.c
	$(CC) -O3 $(AAVX2) $(MARCH) -falign-loops=32  $< -c -o $@ 

$(IC)bitpackc.o: $(IC)bitpackc.c
	$(CC) -O3 $(AAVX2) $(MARCH) -falign-loops=32  $< -c -o $@ 

$(IC)bitunpackc.o: $(IC)bitunpackc.c
	$(CC) -O3 $(AAVX2) $(MARCH) -falign-loops=32  $< -c -o $@ 

$(IC)bitutil.o: $(IC)bitutil.c
	$(CC) -O3 $(AAVX2) $(MARCH) -falign-loops=32  $< -c -o $@ 

$(IC)transpose.o: $(IC)transpose.c
	$(CC) -O3 -c $(IC)transpose.c -o $(IC)transpose.o

$(IC)transpose_sse.o: $(IC)transpose.c
	$(CC) -O3 $(CFLAGS) -DSSE2_ON -mssse3 -c $(IC)transpose.c -o $(IC)transpose_sse.o

$(IC)transpose_avx2.o: $(IC)transpose.c
	$(CC) -O3 $(CFLAGS) -DAVX2_ON -march=haswell -mavx2 -c $(IC)transpose.c -o $(IC)transpose_avx2.o

#HUF=$(L)mhc.o $(L)mhd.o 
OB+=$(HUF) $(L)../util/mem.o $(U)hostinfo.o $(U)fs.o $(L)../ic/bitutil.o $(U)eno.o $(L)ftype.o $(L)lzbc0.o $(L)lzbc.o $(L)lz8c0.o $(L)lz8c1.o $(L)lz8c2.o $(L)lz8c.o $(L)lzhc.o $(L)lzhc0.o $(L)lz8d.o $(L)lzbd.o $(L)lzhd.o $(L)lzrc.o $(L)lzrd.o $(L)trielz.o
OB+=$(L)ansc.o $(L)ansd.o $(L)ans.o $(L)anst.o  
OB+=$(L)ansd_simd.o $(L)ansc_simd.o $(L)ansd_simda.o $(L)ansc_simda.o $(L)ansd_avx2.o $(L)ansc_avx2.o 
OB+=../bwt/bwtiv.o ../bwt/divsufsort.o ../bwt/sssort.o ../bwt/trsort.o 
OB+=$(L)lzcd.o $(L)lzcc0.o $(L)lzcc.o $(L)lzbwt.o
OB+=$(L)../util/thread.o $(U)dbgmem.o   
OB+=$(IC)bitunpack.o $(IC)bitpack.o $(IC)vp4c.o $(IC)vp4d.o $(IC)vsimple.o $(IC)vint.o
#OB+=$(L)ansc.o $(L)ansd.o $(L)ans.o $(L)anst.o ../../dev/sa/sbr.o  
OB+=$(L)ansd_simd.o $(L)ansc_simd.o $(L)ansd_simda.o $(L)ansc_simda.o $(L)ansd_avx2.o $(L)ansc_avx2.o 

SA=../../dev/sa/

divsufsort.o: $(SA)divsufsort.c
	$(CC) $(OMP) -DINLINE=inline -DPROJECT_VERSION_FULL=20 -c $(SA)divsufsort.c

OB+=$(L)ansn.o

OBJT=$(OB) $(IC)vint.o $(L)mhc.o $(L)mhd.o $(U)util.o $(U)hostinfo.o $(U)fs.o $(U)eno.o $(L)lz8c0.o $(L)lz8c1.o $(L)lz8c2.o $(L)lz8c.o $(L)lz8d.o $(L)lzbc0.o $(L)lzbc.o $(L)lzbd.o $(L)lzhc0.o $(L)lzhc.o $(L)lzhd.o $(L)lzrc.o $(L)lzrd.o $(L)trielz.o $(U)mem.o
 
lzturbo: $(OBJT) $(L)lzturbo.o $(U)thread.o
	$(CC) $^ $(LDFLAGS) -o lzturbo
#/usr/local/lib/libpackjpg.so 

#-----------
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

cleant:
	find ~/dev/ -name "*.o" -type f -delete
	find ~/dev/ -name "*~" -type f -delete
	find ~/dev/ -name "core" -type f -delete
	find . -name "*.o" -type f -delete

LW="..\..\dev\"

cleantw:
	del /S $(LW)*.o 
	del /S $(LW)*~
	del /S $(LW)*.exe
	del /S ..\*.o
	del /S ..\*~

