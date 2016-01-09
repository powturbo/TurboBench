##
## blzpack - BriefLZ example
##
## GCC Makefile
##

.SUFFIXES:

.PHONY: clean all

CFLAGS = -Wall -Wextra -ansi -pedantic -Ofast -flto -DNDEBUG
LDFLAGS =

ifeq ($(OS),Windows_NT)
  LDFLAGS += -static
  ifeq ($(CC),cc)
    CC = gcc
  endif
endif

objs = blzpack.o parg.o brieflz.o depack.o

target = blzpack

all: $(target)

%.o : %.c
	$(CC) $(CFLAGS) -I.. -o $@ -c $<

%.o : ../%.c
	$(CC) $(CFLAGS) -I.. -o $@ -c $<

$(target): $(objs)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) $(objs) $(target)
