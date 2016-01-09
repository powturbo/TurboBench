#!/bin/sh

OPT=-O4
#OPT=-Os
#DEBUG=-g -DDEBUG #-fsanitize=undefined #-D_HAVE_PROFILER_

CFLAGS=-Wall -Wno-format -fPIC $(OPT) $(DEBUG) $(TEST) -D_7Z_TYPES_

CC=g++
INCLUDE_PATH = -I ./libcsc/ -I ./archiver/

LIBCSCOBJS=$(addprefix libcsc/, csc_analyzer.o csc_encoder_main.o csc_coder.o csc_filters.o csc_lz.o csc_memio.o csc_enc.o csc_dec.o csc_model.o csc_profiler.o csc_mf.o csc_default_alloc.o)

LIBCSCDECOBJS=$(addprefix libcsc/, csc_dec.o csc_memio.o csc_filters.o csc_default_alloc.o)

CSARCOBJS=$(addprefix archiver/, csa_indexpack.o csa_worker.o csa_file.o csa_adler32.o csa_common.o csa_progress.o)

LIBOUTPUT=libcsc.a

all: $(LIBOUTPUT) csc decomp csarc

csc: ./libcsc/csc.cpp $(LIBOUTPUT)
	$(CC) $(INCLUDE_PATH) $(CFLAGS) -o $@ $^

decomp: ./libcsc/decomp.cpp $(LIBCSCDECOBJS)
	$(CC) $(INCLUDE_PATH) $(CFLAGS) -o $@ $^

csarc: ./archiver/csarc.cpp $(CSARCOBJS) $(LIBOUTPUT)
	$(CC) $(INCLUDE_PATH) $(CFLAGS) -o $@ $^ -pthread

$(LIBOUTPUT): $(LIBCSCOBJS)
	ar rvs $@ $^

%.o: %.cpp
	$(CC) $(INCLUDE_PATH) -c $(CFLAGS) -o $@ $^

clean:
	rm -f $(CSARCOBJS) $(LIBCSCDECOBJS) $(LIBOUTPUT) $(LIBCSCOBJS) csc decomp csarc

rebuild: clean all
