#
# Simple makefile for gcc compiler
# 

EXES = fsc test bit_test div_test bit_cmp
all: libfsc.a $(EXES)

CC = gcc
CFLAGS = -O3 -DNDEBUG
AR = ar
ARFLAGS = r
LDFLAGS = -lm

%.o: %.c fsc.h divide.h
	$(CC) $(CFLAGS) -c $< -o $@

%.a:
	$(AR) $(ARFLAGS) $@ $^

libfscutils.a: fsc_utils.o fsc_utils.h divide.h

libfsc.a: fsc_enc.o fsc_dec.o fsc.h bits.o bits.h alias.o alias.h histo.o divide.h

test: test.o libfsc.a libfscutils.a
	gcc -o test test.o ./libfsc.a ./libfscutils.a $(LDFLAGS) $(CFLAGS)

fsc: fsc.o libfsc.a libfscutils.a
	gcc -o fsc fsc.o ./libfsc.a ./libfscutils.a $(LDFLAGS) $(CFLAGS)

bit_test: bit_test.o libfsc.a libfscutils.a
	gcc -o bit_test bit_test.o ./libfscutils.a ./libfsc.a $(LDFLAGS) $(CFLAGS)

div_test: div_test.o libfsc.a libfscutils.a divide.h
	gcc -o div_test div_test.o ./libfscutils.a ./libfsc.a $(LDFLAGS) $(CFLAGS)

bit_cmp: bit_cmp.o libfsc.a libfscutils.a
	gcc -o bit_cmp bit_cmp.o ./libfscutils.a ./libfsc.a $(LDFLAGS) $(CFLAGS)

pak: clean
	tar czf fsc_oss.tgz *.c *.h Makefile AUTHORS CONTRIBUTORS LICENSE README

clean:
	rm -f *~ *.o *.a $(EXES)

bug: test
	./test -s 2 40
	./test -s 13 10000
	./test -s 3 -p

bench: $(EXES)
	./bit_test
	./quick_check.sh
