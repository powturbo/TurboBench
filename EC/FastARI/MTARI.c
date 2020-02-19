/*  MTARI 0.2 - A Fast Multithreaded Bitwise Arithmetic Compressor
Copyright (C) 2013  David Catt */

#define _CRT_SECURE_NO_WARNINGS
#define BUFSIZE 0x400000
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "FastAri.h"

/* ============BLOCK FUNCTIONS============ */
int enc_blk(char* bin, char* bout, size_t ilen, size_t* olen, int par, void* workmem) {
	return !fa_compress(bin, bout, ilen, olen, workmem);
}
int dec_blk(char* bin, char* bout, size_t ilen, size_t* olen, void* workmem) {
	return !fa_decompress(bin, bout, ilen, olen, workmem);
}
/* ======================================= */

/* ==============DRIVER CORE============== */
void fwritesize(size_t v, FILE* f) {
	int s = 8;
	while(s) {
		fputc(v & 0xff, f);
		v >>= 8;
		--s;
	}
}
int freadsize(size_t* v, FILE* f) {
	int s = 0;
	int c;
	*v = 0;
	while(s < 64) {
		if((c = fgetc(f)) == EOF) { *v = 0;  return 0; }
		*v |= c << s;
		s += 8;
	}
	return 1;
}
int encode(FILE* fin, FILE* fout, int tcnt, size_t bsz, int par) {
	/* Declare variables */
	char** ibufs;
	char** obufs;
	size_t* ilens;
	size_t* olens;
	int mp, pi, rc;
	void** wbufs;
	/* Validate parameters */
	if(!fin) return EXIT_FAILURE;
	if(!fout) return EXIT_FAILURE;
	if(bsz < 1) return EXIT_FAILURE;
	/* Setup thread count */
	if(tcnt < 1) {
#ifdef _OPENMP
		tcnt = omp_get_num_procs();
#else
		tcnt = 1;
#endif
	}
#ifdef _OPENMP
	if(tcnt > omp_get_num_procs()) tcnt = omp_get_num_procs();
	omp_set_num_threads(tcnt);
#else
	tcnt = 1;
#endif
	/* Allocate arrays */
	if(!(ibufs = calloc(tcnt, sizeof(char*)))) return EXIT_FAILURE;
	if(!(obufs = calloc(tcnt, sizeof(char*)))) { free(ibufs); return EXIT_FAILURE; }
	if(!(ilens = calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); return EXIT_FAILURE; }
	if(!(olens = calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); free(ilens); return EXIT_FAILURE; }
	if(!(wbufs = calloc(tcnt, sizeof(void*)))) { free(ibufs); free(obufs); free(ilens); free(olens); return EXIT_FAILURE; }
	/* Allocate buffers */
	for(pi = 0; pi < tcnt; ++pi) {
		if(!(ibufs[pi] = malloc(bsz * sizeof(char))) || !(obufs[pi] = malloc(((bsz + (bsz / 10)) * sizeof(char)) + 1024)) || !(wbufs[pi] = malloc(FA_WORKMEM))) {
			if (ibufs[pi]) free(ibufs[pi]);
			if (obufs[pi]) free(obufs[pi]);
			for (mp = 0; mp < pi; ++mp) { free(ibufs[mp]); free(obufs[mp]); free(wbufs[mp]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			free(wbufs);
			return EXIT_FAILURE;
		}
	}
	/* Process input */
	while(1) {
		/* Read input */
		for(mp = 0; mp < tcnt; ++mp) {
			if((ilens[mp] = fread(ibufs[mp], sizeof(char), bsz, fin)) < 1) break;
		}
		if(!mp) break;
		/* Process input */
		rc = 0;
#pragma omp parallel for
		for(pi = 0; pi < mp; ++pi) {
			if(ilens[pi] > 0) {
				rc += !enc_blk(ibufs[pi], obufs[pi], ilens[pi], &(olens[pi]), par, wbufs[pi]);
			}
		}
		/* Check for processing errors */
		if(rc) {
			for (pi = 0; pi < tcnt; ++pi) { free(ibufs[pi]); free(obufs[pi]); free(wbufs[pi]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			free(wbufs);
			return EXIT_FAILURE;
		}
		/* Write output */
		for(pi = 0; pi < mp; ++pi) {
			fwritesize(ilens[pi], fout);
			fwritesize(olens[pi], fout);
			fwrite(obufs[pi], sizeof(char), olens[pi], fout);
		}
	}
	/* Flush output */
	fwritesize(0, fout);
	fwritesize(0, fout);
	fflush(fout);
	/* Free memory */
	for (pi = 0; pi < tcnt; ++pi) { free(ibufs[pi]); free(obufs[pi]); free(wbufs[pi]); }
	free(ibufs);
	free(obufs);
	free(ilens);
	free(olens);
	free(wbufs);
	/* Return success */
	return EXIT_SUCCESS;
}
int decode(FILE* fin, FILE* fout, int tcnt) {
	/* Declare variables */
	char** ibufs;
	char** obufs;
	size_t* ilens;
	size_t* olens;
	void** wbufs;
	int mp, pi, rc, es = 0;
	/* Validate parameters */
	if(!fin) return EXIT_FAILURE;
	if(!fout) return EXIT_FAILURE;
	/* Setup thread count */
	if(tcnt < 1) {
#ifdef _OPENMP
		tcnt = omp_get_num_procs();
#else
		tcnt = 1;
#endif
	}
#ifdef _OPENMP
	if(tcnt > omp_get_num_procs()) tcnt = omp_get_num_procs();
	omp_set_num_threads(tcnt);
#else
	tcnt = 1;
#endif
	/* Allocate arrays */
	if(!(ibufs = calloc(tcnt, sizeof(char*)))) return EXIT_FAILURE;
	if(!(obufs = calloc(tcnt, sizeof(char*)))) { free(ibufs); return EXIT_FAILURE; }
	if(!(ilens = calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); return EXIT_FAILURE; }
	if(!(olens = calloc(tcnt, sizeof(size_t)))) { free(ibufs); free(obufs); free(ilens); return EXIT_FAILURE; }
	if(!(wbufs = calloc(tcnt, sizeof(void*)))) { free(ibufs); free(obufs); free(ilens); free(olens); return EXIT_FAILURE; }
	/* Allocate work memory */
	for(pi = 0; pi < tcnt; ++pi) {
		if(!(wbufs[pi] = malloc(FA_WORKMEM))) {
			for (mp = 0; mp < pi; ++mp) { free(wbufs[mp]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			free(wbufs);
			return EXIT_FAILURE;
		}
	}
	/* Process input */
	while(1) {
		/* Read input */
		rc = 0;
		for(mp = 0; mp < tcnt; ++mp) {
			if(!freadsize(&(olens[mp]), fin)) { es = 1; break; }
			if(!freadsize(&(ilens[mp]), fin)) { free(ibufs[mp]); rc = 1; break; }
			if(!ilens[mp] || !olens[mp]) { es = 1; break; }
			if(!(ibufs[mp] = malloc(ilens[mp]))) { rc = 1; break; }
			if(!(obufs[mp] = malloc(olens[mp]))) { free(ibufs[mp]); rc = 1; break; }
			if(fread(ibufs[mp], sizeof(char), ilens[mp], fin) < ilens[mp]) { free(ibufs[mp]); free(obufs[mp]); rc = 1; break; }
		}
		if(rc) {
			for(pi = 0; pi < mp; ++pi) { free(ibufs[pi]); free(obufs[pi]); free(wbufs[pi]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			free(wbufs);
			return EXIT_FAILURE;
		}
		/* Process input */
#pragma omp parallel for
		for(pi = 0; pi < mp; ++pi) {
			if(ilens[pi] > 0) {
				rc += !dec_blk(ibufs[pi], obufs[pi], ilens[pi], &(olens[pi]), wbufs[pi]);
			}
		}
		/* Check for processing errors */
		if(rc) {
			for(pi = 0; pi < mp; ++pi) { free(ibufs[pi]); free(obufs[pi]); free(wbufs[pi]); }
			free(ibufs);
			free(obufs);
			free(ilens);
			free(olens);
			free(wbufs);
			return EXIT_FAILURE;
		}
		/* Write output */
		rc = 0;
		for(pi = 0; pi < mp; ++pi) {
			free(ibufs[pi]);
			fwrite(obufs[pi], sizeof(char), olens[pi], fout);
			free(obufs[pi]);
		}
		if (es) break;
	}
	/* Flush output */
	fflush(fout);
	/* Free memory */
	free(ibufs);
	free(obufs);
	free(ilens);
	free(olens);
	for(pi = 0; pi < tcnt; ++pi) { free(wbufs[pi]); }
	free(wbufs);
	/* Return success */
	return EXIT_SUCCESS;
}
/* ======================================= */

int main(int argc, char** argv) {
	FILE *fin, *fout;
	int tcnt = 0, rc;
	if(((argc == 4) || (argc == 5)) && ((argv[1][0] == 'c') || (argv[1][0] == 'd'))) {
		if(argv[1][0] == 'c') {
			if(argc == 5) tcnt = atoi(argv[4]);
			if(strcmp(argv[2], ":")) {
				if(!(fin = fopen(argv[2], "rb"))) { fprintf(stderr, "ERROR: Could not open input file.\n"); return EXIT_FAILURE; }
			} else {
#ifdef _WIN32
				_setmode(_fileno(stdin), O_BINARY);
				fin = stdin;
#else
				fin = freopen(NULL, "rb", stdin);
#endif
			}
			if(strcmp(argv[3], ":")) {
				if(!(fout = fopen(argv[3], "wb"))) { fclose(fin); fprintf(stderr, "ERROR: Could not open output file.\n"); return EXIT_FAILURE; }
			} else {
#ifdef _WIN32
				_setmode(_fileno(stdout), O_BINARY);
				fout = stdout;
#else
				fout = freopen(NULL, "wb", stdout);
#endif
			}
			rc = encode(fin, fout, tcnt, BUFSIZE, 0);
			fflush(fout);
			fclose(fin);
			fclose(fout);
			if(rc != EXIT_SUCCESS) fprintf(stderr, "ERROR: Compress failed.\n");
			return rc;
		} else if(argv[1][0] == 'd') {
			if(argc == 5) tcnt = atoi(argv[4]);
			if(strcmp(argv[2], ":")) {
				if(!(fin = fopen(argv[2], "rb"))) { fprintf(stderr, "ERROR: Could not open input file.\n"); return EXIT_FAILURE; }
			} else {
#ifdef _WIN32
				_setmode(_fileno(stdin), O_BINARY);
				fin = stdin;
#else
				fin = freopen(NULL, "rb", stdin);
#endif
			}
			if(strcmp(argv[3], ":")) {
				if(!(fout = fopen(argv[3], "wb"))) { fclose(fin); fprintf(stderr, "ERROR: Could not open output file.\n"); return EXIT_FAILURE; }
			} else {
#ifdef _WIN32
				_setmode(_fileno(stdout), O_BINARY);
				fout = stdout;
#else
				fout = freopen(NULL, "wb", stdout);
#endif
			}
			rc = decode(fin, fout, tcnt);
			fflush(fout);
			fclose(fin);
			fclose(fout);
			if(rc != EXIT_SUCCESS) fprintf(stderr, "ERROR: Decompress failed.\n");
			return rc;
		}
	} else {
		printf("MTARI  v0.2                 (c) 2013 by David Catt\n\n");
		printf("usage: MTARI [c|d] [infile] [outfile] <threads>\n\n");
		printf("modes:\n");
		printf("  c  - Compress infile to outfile\n");
		printf("  d  - Decompress infile to outfile\n\n");
		printf("note: If the number of threads is not specified, uses all cores\n");
		return EXIT_FAILURE;
	}
}
