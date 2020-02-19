CC := gcc
CFLAGS := -O3 -D PRINTON -D_FILE_OFFSET_BITS=64
LDFLAGS := -lm -static -pthread

GLZA: GLZA.c GLZAcomp.c GLZAformat.c GLZAcompress.c GLZAencode.c GLZAdecode.c GLZAmodel.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
