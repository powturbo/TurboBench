/*
 * BriefLZ - small fast Lempel-Ziv
 *
 * C depacker
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

#include "brieflz.h"

/* Internal data structure */
struct blz_state {
	const unsigned char *src;
	unsigned char *dst;
	unsigned int tag;
	unsigned int bits_left;
};

static unsigned int
blz_getbit(struct blz_state *bs)
{
	unsigned int bit;

	/* Check if tag is empty */
	if (!bs->bits_left--) {
		/* Load next tag */
		bs->tag = (unsigned int) bs->src[0]
		       | ((unsigned int) bs->src[1] << 8);
		bs->src += 2;
		bs->bits_left = 15;
	}

	/* Shift bit out of tag */
	bit = (bs->tag & 0x8000) ? 1 : 0;
	bs->tag <<= 1;

	return bit;
}

static unsigned long
blz_getgamma(struct blz_state *bs)
{
	unsigned long result = 1;

	/* Input gamma2-encoded bits */
	do {
		result = (result << 1) + blz_getbit(bs);
	} while (blz_getbit(bs));

	return result;
}

unsigned long
blz_depack(const void *src, void *dst, unsigned long depacked_size)
{
	struct blz_state bs;
	unsigned long dst_size = 1;

	/* Check for empty input */
	if (depacked_size == 0) {
		return 0;
	}

	bs.src = (const unsigned char *) src;
	bs.dst = (unsigned char *) dst;
	bs.bits_left = 0;

	/* First byte verbatim */
	*bs.dst++ = *bs.src++;

	/* Main decompression loop */
	while (dst_size < depacked_size) {
		if (blz_getbit(&bs)) {
			/* Input match length and offset */
			unsigned long len = blz_getgamma(&bs) + 2;
			unsigned long off = blz_getgamma(&bs) - 2;

			off = (off << 8) + (unsigned long) *bs.src++ + 1;

			/* Copy match */
			{
				const unsigned char *p = bs.dst - off;
				unsigned long i;

				for (i = len; i > 0; --i) {
					*bs.dst++ = *p++;
				}
			}

			dst_size += len;
		}
		else {
			/* Copy literal */
			*bs.dst++ = *bs.src++;

			dst_size++;
		}
	}

	/* Return decompressed size */
	return dst_size;
}
