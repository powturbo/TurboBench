/*@(#)cslzh.h		20.7	SAP	97/11/11 


    ========== licence begin  GPL
    Copyright (c) 1994-2005 SAP AG

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
/*      Definitions for LZH Algorithm                                 */
/*      Lempel-Ziv-Huffman                                            */
/*--------------------------------------------------------------------*/

#ifndef CSLZH_INCL       /* cannot be included twice .................*/
#define CSLZH_INCL

#define REGISTER register

/* The minimum and maximum match lengths .............................*/
#define MIN_MATCH  3
#define MAX_MATCH  258

/* Minimum amount of lookahead, except at the end of the input file ..*/
#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)

/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE. ...............*/
#define MAX_DIST  (WSIZE-MIN_LOOKAHEAD)

#define NONSENSE_LENBITS 2

#define EODATA (-1)

/* Maximum window size = 32K (must be a power of 2) ..................*/
#define WSIZE  ((unsigned) 0x4000)             /* 16K */

#define CS_HASH_BITS  14

#define CS_HASH_SIZE (unsigned)(1 << CS_HASH_BITS)

#define CS_LIT_BUFSIZE  (unsigned) 0x4000      /* 16K */
#define CS_DIST_BUFSIZE  CS_LIT_BUFSIZE

#define HASH_MASK (CS_HASH_SIZE-1)
#define WMASK     (WSIZE-1)

/* Configuration parameters ..........................................*/
/* speed options for the general purpose bit flag */
#define FAST_PA107 4
#define SLOW_PA107 0

/* Matches of length 3 are discarded if their distance exceeds TOO_FAR*/
#ifndef TOO_FAR
#define TOO_FAR 4096
#endif

/* Types centralized here for easy modification ......................*/
typedef SAP_BYTE   uch;      /* unsigned 8-bit value  ................*/
typedef SAP_USHORT ush;      /* unsigned 16-bit value ................*/
typedef SAP_UINT   ulg;      /* unsigned 32-bit value ................*/


#endif   /* CSLZH_INCL */

