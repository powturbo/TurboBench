/*@(#)cslzc.h  	20.8	SAP	97/11/11 


    ========== licence begin  GPL
    Copyright (c) 1992-2005 SAP AG

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
    ========== licence end




*/

/*
 *     SAP AG Walldorf
 *     Systeme, Anwendungen und Produkte in der Datenverarbeitung
 *
 */

/*--------------------------------------------------------------------*/
/* Internal Header File                                               */
/*                                                                    */
/*      Definitions for LZC Algorithm                                 */
/*                                                                    */
/*--------------------------------------------------------------------*/

#ifndef CSLZC_INCL       /* cannot be included twice .................*/
#define CSLZC_INCL

/*--------------------------------------------------------------------*/
/* Macros (internal)                                                  */
/*--------------------------------------------------------------------*/
                                     /* for 16 Bit OS ................*/
#if defined ( OS_16 )
#define LARGE_ARRAY far
#else
#define LARGE_ARRAY
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef CS_BITS
#define CS_BITS        13      /* max. number of bits (default) ......*/
#endif

/* If a new version is added, change CS_VERSION
 */

#define CS_VERSION     (BYTE_TYP) 1

#define CS_ALGORITHM   (BYTE_TYP) 1    /* never change this ..........*/


#define FIRST          257     /* first free entry ...................*/
#define CLEAR          256     /* table clear output code ............*/
                               /* ratio check interval ...............*/
#define CHECK_GAP      4096  /* 10000 */

/* Defines for 8. byte of header .....................................*/
#define BIT_MASK       0x1f
#define BLOCK_MASK     0x80

/* Masks 0x40 and 0x20 are free. */

#define INIT_CS_BITS 9         /* initial number of bits/code ........*/
#define MAX_CS_BITS 16         /* max. number of bits/code ...........*/

#if CS_BITS <= INIT_CS_BITS    /* CS_BITS at least INIT_CS_BITS + 1 ..*/
#undef CS_BITS
#define CS_BITS (INIT_CS_BITS + 1)
#endif

#if CS_BITS >  MAX_CS_BITS     /* CS_BITS at most MAX_CS_BITS ........*/
#undef CS_BITS
#define CS_BITS MAX_CS_BITS
#endif

/*--------------------------------------------------------------------*/
/* TYPES  (depending on CS_BITS)                                      */
/*--------------------------------------------------------------------*/

typedef SAP_BYTE        BYTE_TYP;

#if CS_BITS > 15
typedef long int        CODE_INT;
#else
typedef int             CODE_INT;
#endif

typedef long int        COUNT_INT;
typedef unsigned short  CODE_ENTRY;

/*--------------------------------------------------------------------*/
/* Size of the hash table depending on CS_BITS .......................*/
/*--------------------------------------------------------------------*/

#if CS_BITS == 16
#define HSIZE  69001U          /* 95% occupancy  65536/69001 .........*/
#define CSIZE  69001U
#endif
#if CS_BITS == 15
#define HSIZE  35023U          /* 94% occupancy  32768/35023 .........*/
#define CSIZE  69001U
#endif
#if CS_BITS == 14
#define HSIZE  18013U          /* 91% occupancy  16384/18013 .........*/
#define CSIZE  35023U
#endif
#if CS_BITS == 13
#define HSIZE  9001U           /* 91% occupancy   8192/9001  .........*/
#define CSIZE  18013U
#endif
#if CS_BITS <= 12
#define HSIZE  5003U           /* 80% occupancy   4096/5003  .........*/
#define CSIZE  9001U
#endif

/*--------------------------------------------------------------------*/
/* Access Macros for code and hash tables ............................*/
/*--------------------------------------------------------------------*/

#define MAXCODE(n_bits)  (((CODE_INT) 1 << (n_bits)) - 1)
#define HTABOF(i)        csc.htab[i]
#define CODETABOF(i)     csc.codetab[i]

/*
 * To save much memory, we overlay the table used by CsCompr () with
 * those used by CsDecompr ().  The TAB_PREFIX table is the same size
 * and type as the codetab. The TAB_SUFFIX table needs 2^BITS
 * characters.  We get this from the beginning of htab.
 * The output stack uses the rest of htab, and contains characters.
 * There is plenty of room for any possible stack (stack used to
 * be 8000 characters).
 */

#define TAB_PREFIXOF(i) csc.Prefixtab[i]
#define TAB_SUFFIXOF(i) csc.Suffixtab[i]

/* following definition gives a compiler warning on HP 64 bit (2001-05-15)
   Maybe this will work again some time later.

#define DE_STACK  &TAB_SUFFIXOF((CODE_INT)1<<(CS_BITS+1))

   Definition beneath works on HP, too, but requires an extra global var.
*/

extern CODE_INT DE_STACK_OFFSET;
#define DE_STACK  &TAB_SUFFIXOF(DE_STACK_OFFSET)

/* Clear Hash Table ..................................................*/
#define CL_HASH(size) \
  memset (csc.htab, 0xff, (size) * sizeof (COUNT_INT));


#ifdef USE_MEMCPY           /* use memcpy to transfer bytes ..........*/

#define BYTES_OUT(dst,src,len)              \
    memcpy (dst,src,len); dst += len;

#define BYTES_IN(to,from,len)              \
    memcpy (to,from,len); from += len;

#else                       /* native copy ...........................*/

#define BYTES_OUT(dst,src,len)              \
    {                                       \
      register int i_i = len;               \
      register BYTE_TYP *bufp = src;        \
      while (i_i-- > 0) *dst++ = *bufp++;   \
    }

#define BYTES_IN(to,from,len)               \
    {                                       \
      register int i_i = len;               \
      register BYTE_TYP *bufp = to;         \
      while (i_i-- > 0) *bufp++ = *from++;  \
    }

#endif    /* USE_MEMCPY   */

#endif    /*  CSLZC_INCL  */
