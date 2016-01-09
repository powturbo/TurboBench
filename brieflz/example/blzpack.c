/*
 * blzpack - BriefLZ example
 *
 * Copyright (c) 2002-2015 Joergen Ibsen
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must
 *      not claim that you wrote the original software. If you use this
 *      software in a product, an acknowledgment in the product
 *      documentation would be appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must
 *      not be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source
 *      distribution.
 */

/*
 * This is a simple example packer, which can compress and decompress a
 * single file using BriefLZ.
 *
 * It processes the data in blocks of 1024k. Adjust BLOCK_SIZE to 56k or less
 * to compile for 16-bit.
 *
 * Each compressed block starts with a 24 byte header with the following
 * format:
 *
 *   - 32-bit signature (string "blz",0x1A)
 *   - 32-bit format version (1 in current version)
 *   - 32-bit size of compressed data following header
 *   - 32-bit CRC32 value of compressed data, or 0
 *   - 32-bit size of original uncompressed data
 *   - 32-bit CRC32 value of original uncompressed data, or 0
 *
 * All values in the header are stored in network order (big endian, most
 * significant byte first), and are read and written using the read_be32()
 * and write_be32() functions.
 */

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "brieflz.h"
#include "parg.h"

/*
 * The block-size used to process data.
 */
#define BLOCK_SIZE (1024 * 1024UL)

/*
 * The size of the block header.
 */
#define HEADER_SIZE (6 * 4)

/*
 * Unsigned char type.
 */
typedef unsigned char byte;

/*
 * Get the low-order 8 bits of a value.
 */
#if CHAR_BIT == 8
#  define octet(v) ((byte) (v))
#else
#  define octet(v) ((v) & 0x00FF)
#endif

/*
 * CRC32 algorithm taken from the zlib source, which is
 * Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler
 */

static const unsigned long blz_crctab_n[16] = {
	0x00000000UL, 0x1DB71064UL, 0x3B6E20C8UL, 0x26D930ACUL, 0x76DC4190UL,
	0x6B6B51F4UL, 0x4DB26158UL, 0x5005713CUL, 0xEDB88320UL, 0xF00F9344UL,
	0xD6D6A3E8UL, 0xCB61B38CUL, 0x9B64C2B0UL, 0x86D3D2D4UL, 0xA00AE278UL,
	0xBDBDF21CUL
};

static unsigned long
blz_crc32(const void *src, size_t src_size, unsigned long crc)
{
	const unsigned char *buf = (const unsigned char *) src;
	size_t i;

	if (src_size == 0) {
		return 0;
	}

	crc ^= 0xFFFFFFFFUL;

	for (i = 0; i < src_size; ++i) {
		crc ^= buf[i];
		crc = blz_crctab_n[crc & 0x0F] ^ (crc >> 4);
		crc = blz_crctab_n[crc & 0x0F] ^ (crc >> 4);
	}

	return crc ^ 0xFFFFFFFFUL;
}

/*
 * Store a 32-bit unsigned value in network order.
 */
static void
write_be32(byte *p, unsigned long val)
{
	p[0] = octet(val >> 24);
	p[1] = octet(val >> 16);
	p[2] = octet(val >> 8);
	p[3] = octet(val);
}

/*
 * Read a 32-bit unsigned value in network order.
 */
static unsigned long
read_be32(const byte *p)
{
	return ((unsigned long) octet(p[0]) << 24)
	     | ((unsigned long) octet(p[1]) << 16)
	     | ((unsigned long) octet(p[2]) << 8)
	     | ((unsigned long) octet(p[3]));
}

static unsigned int
ratio(unsigned long x, unsigned long y)
{
	if (x <= ULONG_MAX / 100) {
		x *= 100;
	}
	else {
		y /= 100;
	}

	if (y == 0) {
		y = 1;
	}

	return (unsigned int) (x / y);
}

static int
compress_file(const char *oldname, const char *packedname, int use_checksum)
{
	byte header[HEADER_SIZE] = { 0x62, 0x6C, 0x7A, 0x1A, 0, 0, 0, 1 };
	FILE *oldfile = NULL;
	FILE *packedfile = NULL;
	byte *data = NULL;
	byte *packed = NULL;
	byte *workmem = NULL;
	unsigned long insize = 0, outsize = 0;
	static const char rotator[] = "-\\|/";
	unsigned int counter = 0;
	size_t n_read;
	clock_t clocks;
	int res = 0;

	/* Allocate memory */
	if ((data = (byte *) malloc(BLOCK_SIZE)) == NULL
	 || (packed = (byte *) malloc(blz_max_packed_size(BLOCK_SIZE))) == NULL
	 || (workmem = (byte *) malloc(blz_workmem_size(BLOCK_SIZE))) == NULL) {
		printf("ERR: not enough memory\n");
		res = 1;
		goto out;
	}

	/* Open input file */
	if ((oldfile = fopen(oldname, "rb")) == NULL) {
		printf("ERR: unable to open input file\n");
		res = 1;
		goto out;
	}

	/* Create output file */
	if ((packedfile = fopen(packedname, "wb")) == NULL) {
		printf("ERR: unable to open output file\n");
		res = 1;
		goto out;
	}

	clocks = clock();

	/* While we are able to read data from input file .. */
	while ((n_read = fread(data, 1, BLOCK_SIZE, oldfile)) > 0) {
		size_t packedsize;

		/* Show a little progress indicator */
		printf("%c\r", rotator[counter]);
		counter = (counter + 1) & 0x03;

		/* Compress data block */
		packedsize = blz_pack(data, packed, (unsigned long) n_read, workmem);

		/* Check for compression error */
		if (packedsize == 0) {
			printf("ERR: an error occured while compressing\n");
			res = 1;
			goto out;
		}

		/* Put block-specific values into header */
		write_be32(header + 2 * 4, (unsigned long) packedsize);
		if (use_checksum) {
			write_be32(header + 3 * 4, blz_crc32(packed, packedsize, 0));
			write_be32(header + 4 * 4, (unsigned long) n_read);
			write_be32(header + 5 * 4, blz_crc32(data, n_read, 0));
		}
		else {
			write_be32(header + 3 * 4, 0);
			write_be32(header + 4 * 4, (unsigned long) n_read);
			write_be32(header + 5 * 4, 0);
		}

		/* Write header and compressed data */
		fwrite(header, 1, sizeof(header), packedfile);
		fwrite(packed, 1, packedsize, packedfile);

		/* Sum input and output size */
		insize += (unsigned long) n_read;
		outsize += (unsigned long) (packedsize + sizeof(header));
	}

	clocks = clock() - clocks;

	/* Show result */
	printf("compressed %lu -> %lu bytes (%u%%) in %.2f seconds\n",
	       insize, outsize, ratio(outsize, insize),
	       (double) clocks / (double) CLOCKS_PER_SEC);

out:
	/* Close files */
	if (packedfile != NULL) {
		fclose(packedfile);
	}
	if (oldfile != NULL) {
		fclose(oldfile);
	}

	/* Free memory */
	if (workmem != NULL) {
		free(workmem);
	}
	if (packed != NULL) {
		free(packed);
	}
	if (data != NULL) {
		free(data);
	}

	return res;
}

static int
decompress_file(const char *packedname, const char *newname, int use_checksum)
{
	byte header[HEADER_SIZE];
	FILE *newfile = NULL;
	FILE *packedfile = NULL;
	byte *data = NULL;
	byte *packed = NULL;
	unsigned long insize = 0, outsize = 0;
	static const char rotator[] = "-\\|/";
	unsigned int counter = 0;
	clock_t clocks;
	size_t max_packed_size;
	int res = 0;

	max_packed_size = blz_max_packed_size(BLOCK_SIZE);

	/* Allocate memory */
	if ((data = (byte *) malloc(BLOCK_SIZE)) == NULL
	 || (packed = (byte *) malloc(max_packed_size)) == NULL) {
		printf("ERR: not enough memory\n");
		res = 1;
		goto out;
	}

	/* Open input file */
	if ((packedfile = fopen(packedname, "rb")) == NULL) {
		printf("ERR: unable to open input file\n");
		res = 1;
		goto out;
	}

	/* Create output file */
	if ((newfile = fopen(newname, "wb")) == NULL) {
		printf("ERR: unable to open output file\n");
		res = 1;
		goto out;
	}

	clocks = clock();

	/* While we are able to read a header from input file .. */
	while (fread(header, 1, sizeof(header), packedfile) == sizeof(header)) {
		size_t hdr_packedsize, hdr_depackedsize, depackedsize;
		unsigned long crc;

		/* Show a little progress indicator */
		printf("%c\r", rotator[counter]);
		counter = (counter + 1) & 0x03;

		/* Get compressed and original size from header */
		hdr_packedsize = (size_t) read_be32(header + 2 * 4);
		hdr_depackedsize = (size_t) read_be32(header + 4 * 4);

		/* Verify values in header */
		if (read_be32(header + 0 * 4) != 0x626C7A1AUL /* "blz\x1A" */
		 || read_be32(header + 1 * 4) != 1
		 || hdr_packedsize > max_packed_size
		 || hdr_depackedsize > BLOCK_SIZE) {
			printf("ERR: invalid header in compressed file\n");
			res = 1;
			goto out;
		}

		/* Read compressed data */
		if (fread(packed, 1, hdr_packedsize, packedfile) != hdr_packedsize) {
			printf("ERR: error reading block from compressed file\n");
			res = 1;
			goto out;
		}

		/* Check CRC32 of compressed data */
		crc = read_be32(header + 3 * 4);
		if (use_checksum
		 && crc != 0
		 && crc != blz_crc32(packed, hdr_packedsize, 0)) {
			printf("ERR: compressed data crc error\n");
			res = 1;
			goto out;
		}

		/* Decompress data */
		depackedsize = blz_depack(packed, data, (unsigned long) hdr_depackedsize);

		/* Check for decompression error */
		if (depackedsize != hdr_depackedsize) {
			printf("ERR: an error occured while decompressing\n");
			res = 1;
			goto out;
		}

		/* Check CRC32 of decompressed data */
		crc = read_be32(header + 5 * 4);
		if (use_checksum
		 && crc != 0
		 && crc != blz_crc32(data, depackedsize, 0)) {
			printf("ERR: decompressed file crc error\n");
			res = 1;
			goto out;
		}

		/* Write decompressed data */
		fwrite(data, 1, depackedsize, newfile);

		/* sum input and output size */
		insize += (unsigned long) (hdr_packedsize + sizeof(header));
		outsize += (unsigned long) depackedsize;
	}

	clocks = clock() - clocks;

	/* Show result */
	printf("decompressed %lu -> %lu bytes in %.2f seconds\n",
	       insize, outsize,
	       (double) clocks / (double) CLOCKS_PER_SEC);

out:
	/* Close files */
	if (packedfile != NULL) {
		fclose(packedfile);
	}
	if (newfile != NULL) {
		fclose(newfile);
	}

	/* Free memory */
	if (packed != NULL) {
		free(packed);
	}
	if (data != NULL) {
		free(data);
	}

	return res;
}

static void
print_syntax(void)
{
	printf("Usage: blzpack [options] <infile> <outfile>\n"
	       "\n"
	       "Options:\n"
	       "  -d, --decompress  Decompress\n"
	       "  -c, --checksum    Use checksums if present\n"
	       "  -h, --help        Print this help and exit\n"
	       "  -v, --version     Print version and exit\n"
	       "\n");
}

static void
print_version(void)
{
	printf("blzpack " BLZ_VER_STRING "\n"
	       "\n"
	       "Copyright (c) 2002-2015 Joergen Ibsen\n"
	       "\n"
	       "Licensed under the zlib license.\n"
	       "There is NO WARRANTY, to the extent permitted by law.\n"
	       "\n");
}

int
main(int argc, char *argv[])
{
	struct parg_state ps;
	int flag_decompress = 0;
	int flag_checksum = 0;
	const char *infile = NULL;
	const char *outfile = NULL;
	int c;

	const struct parg_option long_options[] = {
		{ "checksum", PARG_NOARG, NULL, 'c' },
		{ "decompress", PARG_NOARG, NULL, 'd' },
		{ "help", PARG_NOARG, NULL, 'h' },
		{ "version", PARG_NOARG, NULL, 'v' },
		{ 0, 0, 0, 0 }
	};

	parg_init(&ps);

	while ((c = parg_getopt_long(&ps, argc, argv, "cdhv", long_options, NULL)) != -1) {
		switch (c) {
		case 1:
			if (infile == NULL) {
				infile = ps.optarg;
			}
			else if (outfile == NULL) {
				outfile = ps.optarg;
			}
			else {
				printf("Too many arguments.\n\n");
				print_syntax();
				return EXIT_FAILURE;
			}
			break;
		case 'c':
			flag_checksum = 1;
			break;
		case 'd':
			flag_decompress = 1;
			break;
		case 'h':
			print_syntax();
			return EXIT_SUCCESS;
			break;
		case 'v':
			print_version();
			return EXIT_SUCCESS;
			break;
		default:
			printf("Option error at '%s'\n\n", argv[ps.optind - 1]);
			print_syntax();
			return EXIT_FAILURE;
			break;
		}
	}

	if (outfile == NULL) {
		printf("Too few arguments.\n\n");
		print_syntax();
		return EXIT_FAILURE;
	}

#ifdef __WATCOMC__
	/* Unlike BC, which unbuffers stdout if it is a device, OpenWatcom 1.2
	   line buffers stdout; this prevents "rotator" trick based on output
	   of "\r" and writing new line over previous. To make rotator work
	   we unbuffer stdout manually:
	 */
	setbuf(stdout, NULL);
#endif

	if (flag_decompress) {
		return decompress_file(infile, outfile, flag_checksum);
	}
	else {
		return compress_file(infile, outfile, flag_checksum);
	}

	return EXIT_SUCCESS;
}
