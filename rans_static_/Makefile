PROGS=arith_static rANS_static4c rANS_static4j rANS_static4k rANS_static64c rANS_static4_16i 
all: $(PROGS)

CFLAGS=-O3 -g -mtune=native -Wall

.c.o:
	$(CC) $(CFLAGS) $(DEFINES) -c $<

arith_static: arith_static.o
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

rANS_static4c: rANS_static4c.o
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

rANS_static4j: rANS_static4j.o
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

rANS_static4k: rANS_static4k.o
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

rANS_static64c: rANS_static64c.o
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

rANS_static4_16i: rANS_static4_16i.o
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	-rm *.o
	-rm $(PROGS)
