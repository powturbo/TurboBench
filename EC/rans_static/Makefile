# If your compiler doesn't support specific platform, comment out these lines
PROGS_SSE=r8x16_sse
PROGS_AVX2=r8x16_avx2 r16x16_avx2 r32x16_avx2 r8x16b_avx2 r16x16b_avx2 r32x16b_avx2
#PROGS_KNL=r16x16_avx512 r32x16_avx512 r16x16b_avx512 r32x16b_avx512
PROGS_EXPERIMENTAL=r4x16 r8x16 r16x16 r32x16 r4x16b r8x16b r16x16b r32x16b $(PROGS_SSE) $(PROGS_AVX2) $(PROGS_KNL)
CFLAGS=-O3 -g -mtune=native -Wall
OMP=-fopenmp

BLK_SIZE = -DBLK_SIZE=1013*1047
DEFINES += $(BLK_SIZE) -DNTRIALS=1

PROGS_OLD=rANS_static4c rANS_static4j rANS_static64c
PROGS=arith_static rANS_static rANS_static4x8 rANS_static4x16 $(PROGS_OLD) $(PROGS_KNL) $(PROGS_EXPERIMENTAL)
all: $(PROGS)

old: $(OLD_PROGS)

.c.o:
	$(CC) $(CFLAGS) $(DEFINES) -c $<

arith_static: DEFINES+=-DTEST_MAIN
arith_static: arith_static.o
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $< $(LIBS)

rANS_static4c: DEFINES+=-DTEST_MAIN
rANS_static4c: rANS_static4c.o
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $< $(LIBS)

rANS_static4j: DEFINES+=-DTEST_MAIN
rANS_static4j: rANS_static4j.o
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $< $(LIBS)

rANS_static64c: DEFINES+=-DTEST_MAIN
rANS_static64c: rANS_static64c.o
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $< $(LIBS)

rANS_static4x8: rANS_static4x8.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -o $@ $< $(LIBS)

rANS_static4x16: rANS_static4x16.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -o $@ $< $(LIBS)

# Generic N-way state, N-way buffer
r4x16: rNx16.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN $(OMP) -DNX=4 -o $@ $< $(LIBS)
r8x16: rNx16.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN $(OMP) -DNX=8 -o $@ $< $(LIBS)
r16x16: rNx16.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN $(OMP) -DNX=16 -o $@ $< $(LIBS)
r32x16: rNx16.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN $(OMP) -DNX=32 -o $@ $< $(LIBS)

# Custom SIMD versions for specific platforms
r8x16_sse: r8x16_sse.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -msse4 $(OMP) -o $@ $< $(LIBS)
r8x16_avx2: r8x16_avx2.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -march=core-avx2 $(OMP) -o $@ $< $(LIBS)
r16x16_avx2: r16x16_avx2.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -march=core-avx2 $(OMP) -o $@ $< $(LIBS)
r32x16_avx2: r32x16_avx2.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -march=core-avx2 $(OMP) -o $@ $< $(LIBS)
r16x16_avx512: r16x16_avx512.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -march=knl $(OMP) -o $@ $< $(LIBS)
r32x16_avx512: r32x16_avx512.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -march=knl $(OMP) -o $@ $< $(LIBS)

# Generic N-way state, interleaved buffer
r4x16b: rNx16b.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN $(OMP) -DNX=4 -o $@ $< $(LIBS)
r8x16b: rNx16b.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN $(OMP) -DNX=8 -o $@ $< $(LIBS)
r16x16b: rNx16b.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN $(OMP) -DNX=16 -o $@ $< $(LIBS)
r32x16b: rNx16b.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN $(OMP) -DNX=32 -o $@ $< $(LIBS)

# Custom SIMD versions for specific platforms
r8x16b_avx2: r8x16b_avx2.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -march=core-avx2 $(OMP) -o $@ $< $(LIBS)
r16x16b_avx2: r16x16b_avx2.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -march=core-avx2 $(OMP) -o $@ $< $(LIBS)
r32x16b_avx2: r32x16b_avx2.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -march=core-avx2 $(OMP) -o $@ $< $(LIBS)
r16x16b_avx512: r16x16b_avx512.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -march=knl $(OMP) -o $@ $< $(LIBS)
r32x16b_avx512: r32x16b_avx512.c
	$(CC) $(CFLAGS) $(DEFINES) -DTEST_MAIN -march=knl $(OMP) -o $@ $< $(LIBS)


OBJ=rANS_static.o rANS_static4x8.o rANS_static4x16.o rANS_test.o
rANS_static: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

clean:
	-rm *.o
	-rm $(PROGS) $(OLD_PROGS)
