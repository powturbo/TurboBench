# powturbo  (c) Copyright 2015-2017
CC ?= gcc
CXX ?= g++
#CC=clang
#CXX=clang++

ifeq ($(OS),Windows_NT)
  UNAME := Windows
CC=gcc
CXX=g++
else
  UNAME := $(shell uname -s)
ifeq ($(UNAME),$(filter $(UNAME),Linux Darwin FreeBSD GNU/kFreeBSD))
LDFLAGS+= -lrt
endif
endif

CFLAGS=-march=native -minline-all-stringops

all: trle

trle.o: $(TP)trle.c
	$(CC) -O2 $(CFLAGS) -c $(TP)trle.c -o trle.o

trle: trlec.o trled.o trle.o
	$(CC) trle.o trlec.o trled.o $(LDFLAGS) -o trle
 
.c.o:
	$(CC) -O3 $(CFLAGS) $< -c -o $@

clean:
	rm  *.o

cleanw:
	del .\*.o

