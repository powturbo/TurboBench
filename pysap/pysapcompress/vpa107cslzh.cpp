/*



    ========== licence begin  GPL
    Copyright (c) 2000-2005 SAP AG

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
#include <stdlib.h>
#include <string.h>

#include "hpa101saptype.h"        /* Common SAP Header Files ...............*/
#include "hpa106cslzc.h"          /* Internal Definitions for LZC algorithm */
#include "hpa107cslzh.h"

#include "hpa104CsObject.h"
#include "hpa105CsObjInt.h"


#define H_SHIFT  ((CS_HASH_BITS+MIN_MATCH-1)/MIN_MATCH)

typedef struct CONFIG
{
   int good_length;
   int max_lazy;
   unsigned max_chain;
   unsigned char flag;
} CONFIG;

static CONFIG ConfigTab[10] =
{
  /*      good lazy chain flag */
  /* 0 */ {0,    0,    0,  0},     /* store only */
  /* 1 */ {4,    4,   16,  FAST_PA107},  /* maximum speed  */
  /* 2 */ {6,    8,   16,  0},
  /* 3 */ {8,   16,   32,  0},
  /* 4 */ {8,   32,   64,  0},
  /* 5 */ {8,   64,  128,  0},
  /* 6 */ {8,  128,  256,  0},
  /* 7 */ {8,  128,  512,  0},
  /* 8 */ {32, 258, 1024,  0},
  /* 9 */ {32, 258, 4096,  SLOW_PA107}   /* maximum compression */
};

/* Note: the current code requires max_lazy >= MIN_MATCH and
         max_chain >= 4 but these restrictions can easily be
         removed at a small cost.
 */

#define BIT_BUF_SIZE (8 * 2 * sizeof(char))

int NEAR CsExtraLenBits[LENGTH_CODES+2] /* extra bits for each length code */
   = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,99,99};

int NEAR CsExtraDistBits[D_CODES] /* extra bits for each distance code */
= {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int extra_blbits[BL_CODES]/* extra bits for each bit length code */
   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,3,7};


static unsigned char bl_order[BL_CODES]
   = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};

/* Update a hash value with the given input byte
 * IN  assertion: all calls to to UPDATE_HASH are made with consecutive
 *    input characters, so that a running hash key can be computed from the
 *    previous key instead of complete recalculation each time.
 */
#define UPDATE_HASH(h,c) (h = (((h)<<H_SHIFT) ^ (c)) & HASH_MASK)

/* =================================================================
 Insert string s in the dictionary and set match_head to the previous head
 of the hash chain (the most recent string with same hash key). Return
 the previous length of the hash chain.
 IN: all calls to to INSERT_STRING are made with consecutive
  input characters and the first MIN_MATCH bytes of s are valid
  (except for the last MIN_MATCH-1 bytes of the input file). */

#define INSERT_STRING(s, match_head)                  \
   (UPDATE_HASH(csh.ins_h, csh.window[(s) + MIN_MATCH-1]),    \
    csh.CsPrev[(s) & WMASK] = match_head = csh.CsHead[csh.ins_h], \
    csh.CsHead[csh.ins_h] = (unsigned short) (s))

  /* !!! Korrektur: ab 21.10.96 */

void CsObjectInt::HashInit (void)
{
  int j;

  /* If Lookahead < MIN_MATCH, ins_h is garbage, but this is .........*/
  /* not important since only literal bytes will be emitted. .........*/

  csh.ins_h = 0;
  for (j = 0; j < MIN_MATCH-1; j++) UPDATE_HASH (csh.ins_h, csh.window[j]);
}


void CsObjectInt::LongestMatchInit (int level, unsigned char *flags)
/*--------------------------------------------------------------------*/
/*  Initialize the "longest match" routines for a new file            */
/*  0: store, 1: best speed, 9: best compression                      */
/*  general purpose bit flag                                          */
/*--------------------------------------------------------------------*/
{
  register unsigned j;

  /* Initialize the hash table .......................................*/
  for (j = 0;  j < CS_HASH_SIZE; j++) csh.CsHead[j] = 0;
  /* CsPrev will be initialized on the fly ...........................*/

  /* Set the default configuration parameters: .......................*/
  csh.pack_level  = level;
  *flags          |= ConfigTab[level].flag;

  csh.StrStart   = 0;
  csh.BlockStart = 0;
  csh.Lookahead  = 0;

  csh.MatchAvail = 0;
  csh.MatchLen   = MIN_MATCH - 1;
  csh.PrevLen    = 0;
  csh.MatchStart = 0;
  csh.eoData     = 0;
}


#ifndef UNALIGNED_OK
 int CsObjectInt::LongestMatch (unsigned cur_match)         /* current match */
{
  register unsigned char *scan = csh.window + csh.StrStart;           /* current string */
  register unsigned char *match = scan;                       /* matched string */
  register int len;                        /* length of current match */
  int best_len = csh.PrevLen;                 /* best match length so far */
  unsigned limit = csh.StrStart > (unsigned)MAX_DIST ?
                          csh.StrStart - (unsigned)MAX_DIST : 0;

  int good_match        = ConfigTab[csh.pack_level].good_length;
                                             /* max hash chain length */
  unsigned chain_length = ConfigTab[csh.pack_level].max_chain;

  /* Stop when cur_match becomes <= limit. To simplify the code,
   * we prevent matches with the string of window index 0. */

  register unsigned char scan_start = *scan;
  register unsigned char scan_end1  = scan[best_len-1];
  register unsigned char scan_end   = scan[best_len];

  /* Do not waste too much time if we already have a good match: */
  if (csh.PrevLen >= good_match)
  {
    chain_length >>= 2;
  }

  do
  {
    match = csh.window + cur_match;

    /* Skip to next match if the match length cannot increase
     * or if the match length is less than 2: */

    if (match[best_len] != scan_end ||
        match[best_len-1] != scan_end1 || *match != scan_start)
       continue;

  /* It is not necessary to compare scan[1] and match[1] since they
   * are always equal when the other bytes match, given that
   * the hash keys are equal and that h_shift+8 <= CS_HASH_BITS,
   * that is, when the last byte is entirely included in the hash key.
   * The condition is equivalent to
   *       (CS_HASH_BITS+2)/3 + 8 <= CS_HASH_BITS
   * or: CS_HASH_BITS >= 13
   * Also, we check for a match at best_len-1 to get rid quickly of
   * the match with the suffix of the match made at the previous step,
   * which is known to fail. */

#if CS_HASH_BITS >= 13
    len = 1;
#else
    len = 0;
#endif
    do {} while (++len < MAX_MATCH && scan[len] == match[len]);

    if (len > best_len)
    {
      csh.MatchStart = cur_match;
      best_len = len;

      if (len == MAX_MATCH) break;
      scan_end1  = scan[best_len-1];
      scan_end   = scan[best_len];
    }
  } while (--chain_length != 0 &&
           (cur_match = csh.CsPrev[cur_match & WMASK]) > limit);

  return best_len;
}


#else    /* UNALIGNED_OK */

int CsObjectInt::LongestMatch (unsigned cur_match) /* current match */
{
  REGISTER uch *scan = csh.window + csh.StrStart; /* current string */
  REGISTER uch *match = scan;             /* matched string */
  REGISTER int len;                           /* length of current match */
  int best_len = csh.PrevLen;                 /* best match length so far */
  unsigned limit = csh.StrStart > (unsigned)MAX_DIST ? csh.StrStart - (unsigned)MAX_DIST : 0;
  int good_match        = ConfigTab[csh.pack_level].good_length;
                                             /* max hash chain length */
  unsigned chain_length = ConfigTab[csh.pack_level].max_chain;

  /* Stop when cur_match becomes <= limit. To simplify the code,
   * we prevent matches with the string of window index 0. */

  REGISTER ush scan_start = *(ush*)scan;
  REGISTER ush scan_end   = *(ush*)(scan+best_len-1);

  /* Do not waste too much time if we already have a good match: */
  if (csh.PrevLen >= good_match)
  {
    chain_length >>= 2;
  }

  do
  {
    match = csh.window + cur_match;

    /* Skip to next match if the match length cannot increase
     * or if the match length is less than 2: */

    /* This code assumes sizeof(unsigned short) == 2 and
     * sizeof(unsigned long) == 4. Do not use UNALIGNED_OK if your
     * compiler uses different sizes. */

    if (*(ush*)(match+best_len-1) != scan_end ||
        *(ush*)match != scan_start) continue;

    len = MIN_MATCH - 4;
    /* It is not necessary to compare scan[2] and match[2] since they are
     * always equal when the other bytes match, given that the hash keys
     * are equal and that CS_HASH_BITS >= 8.
     */
    do {} while ((len+=4) < MAX_MATCH-3 &&
                 *(SAP_UINT*)(scan+len) == *(SAP_UINT*)(match+len));
    /* The funny do {} generates better code for most compilers */

    if (*(ush*)(scan+len) == *(ush*)(match+len)) len += 2;
    if (scan[len] == match[len]) len++;


    if (len > best_len)
    {
      csh.MatchStart = cur_match;
      best_len = len;

      if (len == MAX_MATCH) break;
      scan_end = *(ush*)(scan+best_len-1);
    }
  } while (--chain_length != 0 &&
           (cur_match = csh.CsPrev[cur_match & WMASK]) > limit);

  return best_len;
}
#endif


void CsObjectInt::FillWindow (void)
/*--------------------------------------------------------------------*/
/* Fill the window when the Lookahead becomes insufficient.           */
/* Updates StrStart and Lookahead, and sets eoInput if end of         */
/* input file.                                                        */
/* IN assertion: Lookahead < MIN_LOOKAHEAD && StrStart + Lookahead>0  */
/* OUT assertion: at least one byte has been read, or eoInput is set. */
/*--------------------------------------------------------------------*/
{
  register unsigned n, m;
  unsigned more = (unsigned)((SAP_UINT)2*WSIZE
                            - (SAP_UINT)csh.Lookahead - (SAP_UINT) csh.StrStart);
  int rc;

  csh.eoInput = 0;

  /* If the window is full, move the upper half to the lower one to make
   * room in the upper half. */
  if (more == 0)
  {
    /* By the IN assertion, the window is not empty so we can't confuse
     * more == 0 with more == 64K on a 16 bit machine. */

    memcpy (csh.window, csh.window + WSIZE, (unsigned)WSIZE);
    csh.MatchStart -= WSIZE;
    csh.StrStart   -= WSIZE;

    /* StrStart - WSIZE = WSIZE - Lookahead > WSIZE - MIN_LOOKAHEAD
     * so we now have StrStart > MAX_DIST: */

    csh.BlockStart -= (SAP_INT) WSIZE;

    for (n = 0; n < CS_HASH_SIZE; n++)
    {
      m = csh.CsHead[n];
      csh.CsHead[n] = (unsigned short)(m >= WSIZE ? m-WSIZE : 0);
    }

    for (n = 0; n < WSIZE; n++)
    {
      m = csh.CsPrev[n];
      csh.CsPrev[n] = (unsigned short)(m >= WSIZE ? m-WSIZE : 0);

      /* If n is not on any hash chain, CsPrev[n] is garbage but
       * its value will never be used. */
    }
    more = WSIZE;
  }
  else
  if (more == (unsigned)EODATA)
  {
    /* Very unlikely, but possible on 16 bit machine if StrStart == 0
     * and Lookahead == 1 (input done one byte at time) */
    more--;
  }

  rc = MemRead (csh.window + csh.StrStart + csh.Lookahead, more);

  if (rc == -1) csh.eoData = 1;
    else
    if (rc == 0) csh.eoInput = 1;
      else
      csh.Lookahead += (unsigned) rc;
}


/*
 * Processes a new input file and return its compressed length.
 * We use a lazy evaluation for matches:
 * a match is finally adopted only if there is
 * no better match at the next window position.
 */

SAP_INT CsObjectInt::ComprLZH (unsigned opt)
{
  unsigned short HashHead;    /* head of hash chain ..................*/
  unsigned PrevMatch;         /* previous match ......................*/
  int flush;                  /* set if current block must be flushed */
  unsigned eof;
  unsigned max_lazy_match = ConfigTab[csh.pack_level].max_lazy;
  unsigned init = opt;


  if (!csh.eoData)                /* fill lookahead buffer ...............*/
  {
    while (csh.Lookahead < MIN_LOOKAHEAD && !csh.eoData)
    {
      FillWindow ();
	  if (init)
	  {
		  init = 0;
		  HashInit ();
	  }
      if (csh.eoInput) return (csh.out_offset);
    }
  }

  while (csh.Lookahead != 0)              /* Process the input block. */
  {
    /* Insert the string window[StrStart .. StrStart+2] in the
     * dictionary, and set HashHead to the head of the hash chain: */
    INSERT_STRING (csh.StrStart, HashHead);

    /* Find the longest match, discarding those <= PrevLen. */
    csh.PrevLen = (int) csh.MatchLen, PrevMatch = csh.MatchStart;
    csh.MatchLen = MIN_MATCH-1;

    if (HashHead != 0 && (unsigned) csh.PrevLen < max_lazy_match &&
        csh.StrStart - HashHead <= MAX_DIST)
    {
      /* To simplify the code, we prevent matches with the string
       * of window index 0 (in particular we have to avoid a match
       * of the string with itself at the start of the input file). */

      csh.MatchLen = LongestMatch ((unsigned)HashHead);
      /* LongestMatch() sets MatchStart */

      if (csh.MatchLen > csh.Lookahead) csh.MatchLen = csh.Lookahead;
      /* Ignore a length 3 match if it is too distant: */

      if (csh.MatchLen == MIN_MATCH && csh.StrStart - csh.MatchStart > TOO_FAR)
      {
        /* If PrevMatch is also MIN_MATCH, MatchStart is garbage
         * but we will ignore the current match anyway. */
        csh.MatchLen--;
      }
    }

    /* If there was a match at the previous step and the current .....*/
    /* match is not better, output the previous match: ...............*/

    if (csh.PrevLen >= MIN_MATCH && csh.MatchLen <= (unsigned) csh.PrevLen)
    {
      flush = ct_tally (csh.StrStart - 1 - PrevMatch, csh.PrevLen - MIN_MATCH);

      /* Insert in hash table all strings up to the end of the match..*/
      /* StrStart-1 and StrStart are already inserted. ...............*/

      csh.Lookahead -= csh.PrevLen-1;
      csh.PrevLen -= 2;
      do
      {
        csh.StrStart++;
        INSERT_STRING(csh.StrStart, HashHead);

        /* StrStart never exceeds WSIZE-MAX_MATCH, so there are
         * always MIN_MATCH bytes ahead. If Lookahead < MIN_MATCH
         * these bytes are garbage, but it does not matter since the
         * next Lookahead bytes will always be emitted as literals. */

      } while (--csh.PrevLen != 0);

      csh.MatchAvail = 0;
      csh.MatchLen = MIN_MATCH-1;
    }
    else
    if (csh.MatchAvail)
    {
      /* If there was no match at the previous position, output a
       * single literal. If there was a match but the current match
       * is longer, truncate the previous match to a single literal. */

      flush = ct_tally (0, csh.window[csh.StrStart-1]);
      (csh.Lookahead)--;
    }
    else
    {
      /* There is no previous match to compare with, wait for
       * the next step to decide. */

      csh.MatchAvail = 1;
      flush = 0;
      (csh.Lookahead)--;
    }

    if (flush)
    {
      FlushBlock (0);
      csh.BlockStart = csh.StrStart;
      (csh.StrStart)++;

      if (csh.bytes_pending) return csh.out_offset;
    }
    else
      (csh.StrStart)++;

    /* Make sure that we always have enough Lookahead, except
     * at the end of the input file. We need MAX_MATCH bytes
     * for the next match, plus MIN_MATCH bytes to insert the
     * string following the next match. */

    if (!csh.eoData)
    {
      while (csh.Lookahead < MIN_LOOKAHEAD && !(csh.eoInput))
      {
        FillWindow ();
        if (csh.eoData) break;
        if (csh.eoInput) return (csh.out_offset);
      }
    }
  }

  if (csh.MatchAvail) ct_tally (0, csh.window[csh.StrStart-1]);

  eof = (csh.eoData && (csh.bytes_pending == 0)) ? 1 : 0;
  FlushBlock (eof);

  return (csh.out_offset);
}

#define PUTSHORT(w)                                                \
{ if (csh.out_offset < csh.out_size-1)                                     \
  {                                                                \
    csh.out_buf[csh.out_offset++] = (unsigned char) ((w) & 0xff);               \
    csh.out_buf[csh.out_offset++] = (unsigned char) ((unsigned short)(w) >> 8);            \
  }                                                                \
  else                                                             \
  if (csh.out_offset < csh.out_size)                                       \
  {                                                                \
    csh.out_buf[csh.out_offset++] = (unsigned char) ((w) & 0xff);               \
    csh.InterBuf[csh.bytes_pending++] = (unsigned char) ((unsigned short)(w) >> 8);        \
  }                                                                \
  else                                                             \
  {                                                                \
    csh.InterBuf[csh.bytes_pending++] = (unsigned char) ((w) & 0xff);           \
    csh.InterBuf[csh.bytes_pending++] = (unsigned char) ((unsigned short)(w) >> 8);        \
  }                                                                \
}

#define PUTBYTE(b)                                         \
         (csh.out_offset < csh.out_size) ?                         \
         (csh.out_buf[csh.out_offset++] = (unsigned char) (b)) :        \
         (csh.InterBuf[csh.bytes_pending++] = (unsigned char) (b))


/* ====================================================================
 * Send a value on a given number of bits.
 * IN assertion: length <= 16 and value fits in length bits.
 */

#define SEND_BITS(value,length)                         \
{                                                       \
  if (csh.bi_valid > (int)BIT_BUF_SIZE - (length))          \
  {                                                     \
    csh.bi_buf |= ((value) << csh.bi_valid);                    \
    PUTSHORT(csh.bi_buf);                                   \
    csh.bi_buf = (unsigned short)(value) >> (BIT_BUF_SIZE - csh.bi_valid); \
    csh.bi_valid += (length) - BIT_BUF_SIZE;                \
  }                                                     \
  else                                                  \
  {                                                     \
    csh.bi_buf |= (value) << csh.bi_valid;                      \
    csh.bi_valid += (length);                               \
  }                                                     \
}


/* Send a code of the given tree. c and tree must not have side effects */
#define SEND_CODE(c,tree) SendBits(tree[c].Code, tree[c].Len)

#define D_CODE(dist) \
   ((dist) < 256 ? csh.dist_code[dist] : csh.dist_code[256+((dist)>>7)])



void CsObjectInt::SendBits (unsigned value, int len)
/*--------------------------------------------------------------------*/
/* Send a value with len bits (len <=16)                              */
/*--------------------------------------------------------------------*/
{
  SEND_BITS (value,len);
}

void CsObjectInt::HufTabInit (void)
/*--------------------------------------------------------------------*/
/* Allocate the match buffer, initialize the various tables.          */
/*--------------------------------------------------------------------*/
{
  int n;        /* iterates over tree elements */
  int nend;
  int bits;     /* bit counter */
  int length;   /* length value */
  int code;     /* code value */
  int dist;     /* distance index */

  /* Initialize the first block of the first file ....................*/
  InitBlock ();
#if 0
  if (csh.static_dtree[0].Len != 0) return; /* HufTabInit already called */
#endif
  /* Initialize the mapping length (0..255) -> length code (0..28) ...*/
  length = 0;
  for (code = 0; code < LENGTH_CODES-1; code++)
  {
    csh.base_length[code] = length;
    nend = (1 << CsExtraLenBits[code]);
    for (n = 0; n < nend; n++)
    {
      csh.length_code[length++] = (unsigned char)code;
    }
  }

  csh.length_code[length-1] = (unsigned char)code;

  /* Initialize the mapping dist (0..32K) -> dist code (0..29) .......*/
  dist = 0;
  for (code = 0; code < 16; code++)
  {
    csh.base_dist[code] = dist;
    nend = (1 << CsExtraDistBits[code]);
    for (n = 0; n < nend; n++)
    {
      csh.dist_code[dist++] = (unsigned char)code;
    }
  }

  dist >>= 7; /* from now on, all distances are divided by 128 .......*/
  for ( ; code < D_CODES; code++)
  {
    csh.base_dist[code] = dist << 7;
    nend = 1 << (CsExtraDistBits[code]-7);
    for (n = 0; n < nend; n++)
    {
      csh.dist_code[256 + dist++] = (unsigned char)code;
    }
  }

  /* Construct the codes of the static literal tree ..................*/
  for (bits = 0; bits <= MAX_BITS; bits++) csh.bl_count[bits] = 0;
  n = 0;
  while (n <= 143) csh.static_ltree[n++].Len = 8, csh.bl_count[8]++;
  while (n <= 255) csh.static_ltree[n++].Len = 9, csh.bl_count[9]++;
  while (n <= 279) csh.static_ltree[n++].Len = 7, csh.bl_count[7]++;
  while (n <= 287) csh.static_ltree[n++].Len = 8, csh.bl_count[8]++;

  /* Codes 286 and 287 do not exist, but we must include them in the
   * tree construction to get a canonical Huffman tree (longest code
   * all ones) */

  GenCodes (csh.static_ltree, L_CODES+1);

  /* The static distance tree is trivial .............................*/
  for (n = 0; n < D_CODES; n++)
  {
    csh.static_dtree[n].Len = 5;
    csh.static_dtree[n].Code = ReverseCode (n, 5);
  }
}


void CsObjectInt::InitBlock (void)
/*--------------------------------------------------------------------*/
/* Initialize a new block.                                                                   */
/*--------------------------------------------------------------------*/
{
  int n; /* iterates over tree elements */

  /* Initialize the trees. ...........................................*/
  for (n = 0; n < L_CODES;  n++) csh.dyn_ltree[n].Freq = 0;
  for (n = 0; n < D_CODES;  n++) csh.dyn_dtree[n].Freq = 0;
  for (n = 0; n < BL_CODES; n++) csh.bl_tree[n].Freq = 0;

  csh.dyn_ltree[END_BLOCK].Freq = 1;
  csh.opt_len = csh.static_len = 0L;
  csh.last_lit = csh.last_dist = csh.last_flags = 0;
  csh.flags = 0;
  csh.flag_bit = 1;
}


#define SMALLEST 1
/* Index within the heap array of least frequent node in the Huffman tree */


/* ===============================================================
 * Remove the smallest element from the heap and recreate the heap with
 * one less element. Updates heap and heap_len. */

#define PQREMOVE(tree, top)            \
{                                      \
    top = csh.heap[SMALLEST];              \
    csh.heap[SMALLEST] = csh.heap[csh.heap_len--]; \
    pqdownheap(tree, SMALLEST);        \
}

/* ===============================================================
 * Compares to subtrees, using the tree depth as tie breaker when
 * the subtrees have equal frequency.
 * This minimizes the worst case length. */

#define SMALLER(tree, n, m)                           \
   (tree[n].Freq < tree[m].Freq ||                    \
   (tree[n].Freq == tree[m].Freq && csh.depth[n] <= csh.depth[m]))

/* ================================================================
 * Restore the heap property by moving down the tree starting at node k,
 * exchanging a node with the smallest of its two sons if necessary, stopping
 * when the heap property is re-established (each father smaller than its
 * two sons).
 */

void CsObjectInt::pqdownheap (
						CT_DATA *tree,    /* the tree to restore .....*/
                        int k)            /* node to move down .......*/
{
  int v = csh.heap[k];
  int j = k << 1;  /* left son of k ..................................*/

  while (j <= csh.heap_len)
  {
    /* Set j to the smallest of the two sons: ........................*/
    if (j < csh.heap_len && SMALLER(tree, csh.heap[j+1], csh.heap[j])) j++;

    /* Exit if v is smaller than both sons ...........................*/
    if (SMALLER(tree, v, csh.heap[j])) break;

    /* Exchange v with the smallest son ..............................*/
    csh.heap[k] = csh.heap[j],  k = j;

    /* And continue down the tree, setting j to the left son of k ....*/
    j <<= 1;
  }
  csh.heap[k] = v;
}

/* =================================================================
 * Compute the optimal bit lengths for a tree and update the total bit length
 * for the current block.
 * IN assertion: the fields freq and dad are set, heap[heap_max] and
 *    above are the tree nodes sorted by increasing frequency.
 * OUT assertions: the field len is set to the optimal bit length, the
 *     array bl_count contains the frequencies for each bit length.
 *     The length opt_len is updated;
 *     static_len is also updated if stree is not null.
 */

void CsObjectInt::GenBitLen (TREE_DESC *desc) /* the tree descriptor .......*/
{
  CT_DATA *tree       = desc->dyn_tree;
  int *extra          = desc->extra_bits;
  int base            = desc->extra_base;
  int max_code        = desc->max_code;
  int max_length      = desc->max_length;
  CT_DATA *stree = desc->static_tree;
  int h;              /* heap index */
  int n, m;           /* iterate over the tree elements */
  int bits;           /* bit length */
  int xbits;          /* extra bits */
  unsigned short f;              /* frequency */
  int overflow = 0;   /* # of elements with bit length too large */

  for (bits = 0; bits <= MAX_BITS; bits++) csh.bl_count[bits] = 0;

  /* In a first pass, compute the optimal bit lengths (which may
   * overflow in the case */

  tree[csh.heap[csh.heap_max]].Len = 0; /* root of the heap */

  for (h = csh.heap_max+1; h < HEAP_SIZE; h++)
  {
    n = csh.heap[h];
    bits = tree[tree[n].Dad].Len + 1;
    if (bits > max_length) bits = max_length, overflow++;

    /* We overwrite tree[n].Dad which is no longer needed */
    tree[n].Len = (unsigned short) bits;

    if (n > max_code) continue; /* not a leaf node */

    csh.bl_count[bits]++;
    xbits = 0;
    if (n >= base) xbits = extra[n-base];
    f = tree[n].Freq;
    csh.opt_len += (SAP_UINT)f * (bits + xbits);
    if (stree) csh.static_len += (SAP_UINT)f * (stree[n].Len + xbits);
  }
  if (overflow == 0) return;

  /* Find the first bit length which could increase: */
  do
  {
    bits = max_length-1;
    while (csh.bl_count[bits] == 0) bits--;
    csh.bl_count[bits]--;      /* move one leaf down the tree */
    csh.bl_count[bits+1] += 2; /* move one overflow item as its brother */
    csh.bl_count[max_length]--;
    /* The brother of the overflow item also moves one step up,
     * but this does not affect bl_count[max_length] */

    overflow -= 2;
  } while (overflow > 0);

  /* Now recompute all bit lengths, scanning in increasing frequency.
   * h is still equal to HEAP_SIZE. It is simpler to reconstruct all
   * lengths instead of fixing only the wrong ones. */

  for (bits = max_length; bits != 0; bits--)
  {
    n = csh.bl_count[bits];
    while (n != 0)
    {
      m = csh.heap[--h];
      if (m > max_code) continue;
      if (tree[m].Len != (unsigned) bits)
      {
        csh.opt_len +=
           ((SAP_INT)bits - (SAP_INT)tree[m].Len)*(SAP_INT)tree[m].Freq;
        tree[m].Len = (unsigned short) bits;
      }
      n--;
    }
  }
}

/* ================================================================
 * Generate the codes for a given tree and bit counts (which need not be
 * optimal).
 * IN assertion:
 *    the array bl_count contains the bit length statistics for
 * the given tree and the field len is set for all tree elements.
 * OUT assertion: the field code is set for all tree elements of non
 *     zero code length.
 */
void CsObjectInt::GenCodes (
					  CT_DATA *tree,   /* the tree to decorate */
                      int  max_code) /* largest code with non zero frequency */
{
  unsigned short next_code[MAX_BITS+1]; /* next code value for each bit length */
  unsigned short code = 0;              /* running code value */
  int bits;                  /* bit index */
  int n;                     /* code index */

  /* The distribution counts are first used to generate the code values
   * without bit reversal. ...........................................*/

  for (bits = 1; bits <= MAX_BITS; bits++)
  {
    next_code[bits] = code = (unsigned short)((code + csh.bl_count[bits-1]) << 1);
  }

  for (n = 0;  n <= max_code; n++)
  {
    int len = tree[n].Len;
    if (len == 0) continue;

    /* Now reverse the bits ..........................................*/
    tree[n].Code = ReverseCode (next_code[len]++, len);
  }
}

/* ================================================================
 * Construct one Huffman tree and assigns the code bit strings and
   lengths. Update the total bit length for the current block.
   The field freq is set for all tree elements.
 * OUT assertions: the fields len and code are set to the optimal bit length
 *     and corresponding code. The length opt_len is updated; static_len is
 *     also updated if stree is not null. The field max_code is set.
 */

void CsObjectInt::BuildTree (TREE_DESC *desc)        /* the tree descriptor */
{
  CT_DATA *tree   = desc->dyn_tree;
  CT_DATA *stree  = desc->static_tree;
  int elems       = desc->elems;
  int n, m;          /* iterate over heap elements */
  int max_code = -1; /* largest code with non zero frequency */
  int node = elems;  /* next internal node of the tree */

  /* Construct the initial heap, with least frequent element in
   * heap[SMALLEST]. The sons of heap[n] are heap[2*n] and heap[2*n+1].
   * heap[0] is not used. ............................................*/

  csh.heap_len = 0, csh.heap_max = HEAP_SIZE;

  for (n = 0; n < elems; n++)
  {
    if (tree[n].Freq != 0)
    {
      csh.heap[++(csh.heap_len)] = max_code = n;
      csh.depth[n] = 0;
    }
    else
    {
      tree[n].Len = 0;
    }
  }

  /* lzh format requires that at least one distance code exists,
   * and that at least one bit should be sent even if there is only one
   * possible code. So to avoid special checks later on we force
   * at least two codes of non zero frequency. .......................*/

  while (csh.heap_len < 2)
  {
    /* new is 0 or 1 so it does not have extra bits ..................*/
    int new1 = csh.heap[++(csh.heap_len)] = (max_code < 2 ? ++max_code : 0);
    tree[new1].Freq = 1;
    csh.depth[new1] = 0;
    csh.opt_len--;
    if (stree) csh.static_len -= stree[new1].Len;
  }

  desc->max_code = max_code;

  /* The elements heap[heap_len/2+1 .. heap_len] are leaves of the tree,
   * establish sub-heaps of increasing lengths: ......................*/

  for (n = csh.heap_len/2; n >= 1; n--) pqdownheap(tree, n);

  /* Construct the Huffman tree by repeatedly combining the least two
   * frequent nodes. .................................................*/
  do
  {
    PQREMOVE(tree, n);    /* n = node of least frequency .............*/
    m = csh.heap[SMALLEST];   /* m = node of next least frequency ........*/

    csh.heap[--(csh.heap_max)] = n; /* keep the nodes sorted by frequency ......*/
    csh.heap[--(csh.heap_max)] = m;

    /* Create a new node father of n and m */
    tree[node].Freq = tree[n].Freq + tree[m].Freq;
    csh.depth[node] = (unsigned char) (MAX(csh.depth[n], csh.depth[m]) + 1);
    tree[n].Dad = tree[m].Dad = (unsigned short) node;

    /* and insert the new node in the heap ...........................*/
    csh.heap[SMALLEST] = node++;
    pqdownheap(tree, SMALLEST);

  } while (csh.heap_len >= 2);

  csh.heap[--(csh.heap_max)] = csh.heap[SMALLEST];

  /* At this point, the fields freq and dad are set. We can now
   * generate the bit lengths. .......................................*/

  GenBitLen (desc);

  /* The field len is now set, we can generate the bit codes .........*/
  GenCodes (tree, max_code);
}


void CsObjectInt::ScanTree (
					  CT_DATA *tree, /* the tree to be scanned .......*/
                      int max_code)  /* largest code of non zero freq */
/*--------------------------------------------------------------------*/
/* Scan a literal or distance tree to determine the frequencies of    */
/* the codes in the bit length tree. Updates opt_len to take into     */
/* account the repeat counts. (The contribution of the bit length     */
/* codes will be added later during the construction of bl_tree.)     */
/*--------------------------------------------------------------------*/
{
  int n;                     /* iterates over all tree elements */
  int prevlen = -1;          /* last emitted length */
  int curlen;                /* length of current code */
  int nextlen = tree[0].Len; /* length of next code */
  int count = 0;             /* repeat count of the current code */
  int max_count = 7;         /* max repeat count */
  int min_count = 4;         /* min repeat count */

  if (nextlen == 0) max_count = 138, min_count = 3;
  tree[max_code+1].Len = (unsigned short)-1; /* guard */

  for (n = 0; n <= max_code; n++)
  {
    curlen = nextlen;
    nextlen = tree[n+1].Len;
    if (++count < max_count && curlen == nextlen)
    {
      continue;
    }
    else if (count < min_count)
    {
      csh.bl_tree[curlen].Freq += (unsigned short) count;
    }
    else if (curlen != 0)
    {
      if (curlen != prevlen) (csh.bl_tree[curlen].Freq)++;
      (csh.bl_tree[REP_3_6].Freq)++;
    }
    else if (count <= 10)
    {
      (csh.bl_tree[REPZ_3_10].Freq)++;
    }
    else
    {
      (csh.bl_tree[REPZ_11_138].Freq)++;
    }

    count = 0;
    prevlen = curlen;
    if (nextlen == 0)
    {
      max_count = 138, min_count = 3;
    }
    else if (curlen == nextlen)
    {
      max_count = 6, min_count = 3;
    }
    else
    {
      max_count = 7, min_count = 4;
    }
  }
}


void CsObjectInt::SendTree (
					  CT_DATA *tree,  /* the tree to be scanned       */
                       int max_code)  /* largest code of freq != 0    */
/*--------------------------------------------------------------------*/
/* Send a literal or distance tree in compressed form, using the      */
/* codes in bl_tree.                                                  */
/*--------------------------------------------------------------------*/
{
  int n;                     /* iterates over all tree elements */
  int prevlen = -1;          /* last emitted length */
  int curlen;                /* length of current code */
  int nextlen = tree[0].Len; /* length of next code */
  int count = 0;             /* repeat count of the current code */
  int max_count = 7;         /* max repeat count */
  int min_count = 4;         /* min repeat count */

  if (nextlen == 0) max_count = 138, min_count = 3;

  for (n = 0; n <= max_code; n++)
  {
    curlen = nextlen; nextlen = tree[n+1].Len;
    if (++count < max_count && curlen == nextlen)
    {
      continue;
    }
    else if (count < min_count)
    {
      do {
           SEND_CODE (curlen, csh.bl_tree);
         } while (--count != 0);
    }
    else if (curlen != 0)
    {
      if (curlen != prevlen)
      {
        SEND_CODE(curlen, csh.bl_tree); count--;
      }
      SEND_CODE (REP_3_6, csh.bl_tree);
      SendBits (count-3, 2);
    }
    else if (count <= 10)
    {
      SEND_CODE (REPZ_3_10, csh.bl_tree);
      SendBits (count-3, 3);
    }
    else
    {
      SEND_CODE (REPZ_11_138, csh.bl_tree);
      SendBits (count-11, 7);
    }
    count = 0; prevlen = curlen;
    if (nextlen == 0)
    {
      max_count = 138, min_count = 3;
    }
    else if (curlen == nextlen)
    {
      max_count = 6, min_count = 3;
    }
    else
    {
      max_count = 7, min_count = 4;
    }
  }
}


int CsObjectInt::BuildBlTree (void)
/*--------------------------------------------------------------------*/
/* Construct the Huffman tree for the bit lengths and return the      */
/* index in bl_order of the last bit length code to send.             */
/*--------------------------------------------------------------------*/
{
  int max_blindex;  /* index of last bit length code of non zero freq */

  /* Determine the bit length frequencies for literal, distance trees */
  ScanTree (csh.dyn_ltree, csh.l_desc.max_code);
  ScanTree (csh.dyn_dtree, csh.d_desc.max_code);

  /* Build the bit length tree: ......................................*/
  BuildTree (&(csh.bl_desc));

  /* opt_len now includes the length of the tree representations, except
   * the lengths of the bit lengths codes and the 5+5+4 bits for
   * the counts. */

  /* Determine the number of bit length codes to send. The lzh format
   * requires that at least 4 bit length codes be sent. ..............*/

  for (max_blindex = BL_CODES-1; max_blindex >= 3; max_blindex--)
  {
    if (csh.bl_tree[bl_order[max_blindex]].Len != 0) break;
  }

  /* Update opt_len to include the bit length tree and counts ........*/
  csh.opt_len += 3 * (max_blindex + 1) + 5 + 5 + 4;

  return max_blindex;
}


void CsObjectInt::SendAllTrees (int lcodes, int dcodes, int blcodes)
                           /* number of codes for each tree ..........*/
/*--------------------------------------------------------------------*/
/* Send the header for a block using dynamic Huffman trees:           */
/* the counts, the lengths of the bit length codes, the literal       */
/* tree and the distance tree.                                        */
/*   lcodes >= 257, dcodes >= 1, blcodes >= 4.                        */
/*--------------------------------------------------------------------*/
{
  int rank;                                      /* index in bl_order */

  SendBits (lcodes-257, 5);
  SendBits (dcodes-1,   5);
  SendBits (blcodes-4,  4);

  for (rank = 0; rank < blcodes; rank++)
  {
    SendBits (csh.bl_tree[bl_order[rank]].Len, 3);
  }

  SendTree (csh.dyn_ltree, lcodes-1);           /* send the literal tree  */
  SendTree (csh.dyn_dtree, dcodes-1);           /* send the distance tree */
}


void CsObjectInt::FlushBlock (int eof)          /* true for last block .....*/
/*--------------------------------------------------------------------*/
/* Determine the best encoding for the current block:                 */
/* dynamic trees or static trees and output the encoded block to      */
/* the zip file.                                                      */
/*--------------------------------------------------------------------*/
{
  SAP_UINT opt_lenb, static_lenb;  /* opt_len and static_len in bytes */
  int max_blindex;  /* index of last bit length code of non zero freq */


  csh.flag_buf[csh.last_flags] = csh.flags; /* Save the flags for the last 8 items*/

  /* Construct the literal and distance trees ........................*/
  BuildTree (&(csh.l_desc));
  BuildTree (&(csh.d_desc));

  /* Build the bit length tree for the above two trees, and get the
     index in bl_order of the last bit length code to send. ..........*/

  max_blindex = BuildBlTree ();

  /* Determine the best encoding. Compute first length in bytes ......*/
  opt_lenb = (csh.opt_len + 3 + 7) >> 3;
  static_lenb = (csh.static_len + 3 + 7) >> 3;

  if (static_lenb <= opt_lenb)
  {
    SendBits ((STATIC_TREES<<1)+eof, 3);
    CompressBlock (csh.static_ltree, csh.static_dtree);
  }
  else
  {
    SendBits ((DYN_TREES<<1)+eof, 3);
    SendAllTrees (csh.l_desc.max_code+1, csh.d_desc.max_code+1, max_blindex+1);
    CompressBlock (csh.dyn_ltree, csh.dyn_dtree);
  }

  InitBlock ();

  if (eof) FlushIncomp ();
}


int CsObjectInt::ct_tally (
					 int dist,  /* distance of matched string */
                     int lc)   /* match length-MIN_MATCH or unmatched char (if dist==0) */
/*--------------------------------------------------------------------*/
/* Save the match info and tally the frequency counts. Return true if */
/* the current block must be flushed.                                 */
/*--------------------------------------------------------------------*/
{
  csh.CsLitLenBuf[(csh.last_lit)++] = (unsigned char)lc;

  if (dist == 0)
  {
    (csh.dyn_ltree[lc].Freq)++;                /* lc is the unmatched char .*/
  }
  else
  {
    /* Here, lc is the match length - MIN_MATCH ......................*/
    dist--;                              /* dist = match distance - 1 */

    (csh.dyn_ltree[csh.length_code[lc]+LITERALS+1].Freq)++;
    (csh.dyn_dtree[D_CODE(dist)].Freq)++;

    csh.CsDistBuf[(csh.last_dist)++] = (unsigned short) dist;
    csh.flags |= csh.flag_bit;
  }

  csh.flag_bit <<= 1;

  if ((csh.last_lit & 7) == 0)   /* Output the flags if they fill a byte: */
  {
    csh.flag_buf[csh.last_flags++] = csh.flags;
    csh.flags = 0, csh.flag_bit = 1;
  }

  /* Try to guess if it is profitable to stop the current block here .*/
  if (csh.pack_level > 2 && (csh.last_lit & 0xfff) == 0)
  {
    /* Compute an upper bound for the compressed length ..............*/
    SAP_UINT out_length = (SAP_UINT)csh.last_lit * 8L;
    SAP_UINT in_length = (SAP_UINT)csh.StrStart - csh.BlockStart;
    int dcode;
    for (dcode = 0; dcode < D_CODES; dcode++)
    {
      out_length += (SAP_UINT)csh.dyn_dtree[dcode].Freq *
                          (5L + CsExtraDistBits[dcode]);
    }
    out_length >>= 3;

    if (csh.last_dist < csh.last_lit/2 && out_length < in_length/2) return 1;
  }

  /* We avoid equality with CS_LIT_BUFSIZE because of wraparound at 64K
   * on 16 bit machines and because stored blocks are restricted to
   * 64K-1 bytes. */
  return (csh.last_lit == CS_LIT_BUFSIZE-1 || csh.last_dist == CS_DIST_BUFSIZE);
}


void CsObjectInt::CompressBlock (
						   CT_DATA *ltree,      /* literal tree ......*/
                           CT_DATA *dtree )     /* distance tree .....*/
/*--------------------------------------------------------------------*/
/* Send the block data compressed using the given Huffman trees       */
/*--------------------------------------------------------------------*/
{
  unsigned dist;      /* distance of matched string */
  int lc;             /* match length or unmatched char (if dist==0) */
  unsigned lx = 0;    /* running index in CsLitLenBuf */
  unsigned dx = 0;    /* running index in CsDistBuf */
  unsigned fx = 0;    /* running index in flag_buf */
  unsigned short flag = 0;       /* current flags */
  unsigned code;      /* the code to send */
  int extra;          /* number of extra bits to send */

  if (csh.last_lit != 0)
  do
  {
    if ((lx & 7) == 0) flag = csh.flag_buf[fx++];
    lc = csh.CsLitLenBuf[lx++];
    if ((flag & 1) == 0)
    {
      SEND_CODE (lc, ltree);             /* send a literal byte ......*/
    }
    else
    {
      /* Here, lc is the match length - MIN_MATCH ....................*/
      code = csh.length_code[lc];
      SEND_CODE (code+LITERALS+1, ltree); /* send the length code ....*/
      extra = CsExtraLenBits[code];
      if (extra != 0)
      {
        lc -= csh.base_length[code];
        SendBits (lc, extra);       /* send the extra length bits ....*/
      }
      dist = csh.CsDistBuf[dx++];

      code = D_CODE (dist);         /* dist is the match distance - 1 */

      SEND_CODE (code, dtree);      /* send the distance code ........*/
      extra = CsExtraDistBits[code];

      if (extra != 0)
      {
        dist -= csh.base_dist[code];
        SendBits (dist, extra);     /* send the extra distance bits ..*/
      }
    } /* literal or match pair ? */

    flag >>= 1;
  } while (lx < csh.last_lit);

  SEND_CODE (END_BLOCK, ltree);     /* end of block ..................*/
}


void CsObjectInt::BitBufInit (void)
/*--------------------------------------------------------------------*/
/* Initialize the bit string routines.                                */
/*--------------------------------------------------------------------*/
{
  unsigned x, y;

  csh.bi_buf   = 0;
  csh.bi_valid = 0;

  x = rand () & ((1 << NONSENSE_LENBITS) - 1);
  SendBits (x, NONSENSE_LENBITS);

  if (x)
  {
    y = rand () & ((1 << x) - 1);
    SendBits (y, (int) x);
  }
}


unsigned short CsObjectInt::ReverseCode (unsigned code,    /* the value to invert */
                                   int      len)     /* its bit length .....*/
/*--------------------------------------------------------------------*/
/*  Reverse the first len bits of a code, using straightforward code  */
/*  (a faster method would use a table) 1 <= len <= 15                */
/*--------------------------------------------------------------------*/
{
  register unsigned res = 0;
  do
  {
    res |= code & 1;
    code >>= 1, res <<= 1;
  } while (--len > 0);
  return (unsigned short) (res >> 1);
}


void CsObjectInt::FlushIncomp (void)
/*--------------------------------------------------------------------*/
/* Write out any remaining bits in an incomplete byte.                */
/*--------------------------------------------------------------------*/
{
  if (csh.bi_valid > 8)
  {
    PUTSHORT(csh.bi_buf);
  }
  else if (csh.bi_valid > 0)
  {
    PUTBYTE(csh.bi_buf);
  }

  csh.bi_buf = 0;
  csh.bi_valid = 0;

  PUTBYTE(csh.bi_buf);   /* add a 0x0 to make it easier for decompress ...*/
}


int CsObjectInt::MemRead (unsigned char *buf, unsigned size)
/*--------------------------------------------------------------------*/
/* Read a memory chunk into buf                                       */
/*--------------------------------------------------------------------*/
{
  if (csh.sum_in + csh.in_offset >= csh.sum_size)
    return -1; /* end of data ........................................*/

  if (csh.in_offset < csh.in_size)
  {
    SAP_UINT block_size = csh.in_size - csh.in_offset;
    if (block_size > (SAP_UINT) size)
      block_size = (SAP_UINT) size;

    memcpy (buf, csh.in_buf + csh.in_offset, (unsigned) block_size);
    csh.in_offset += block_size;
    return (int) block_size;
  }
  else
  {
    return 0;  /* end of input .......................................*/
  }
}

void CsObjectInt::InitDesc (void)
{
	csh.l_desc.dyn_tree = csh.dyn_ltree;
	csh.l_desc.static_tree = csh.static_ltree;
	csh.l_desc.extra_bits = CsExtraLenBits;
	csh.l_desc.extra_base = LITERALS+1;
	csh.l_desc.elems = L_CODES;
	csh.l_desc.max_length = MAX_BITS;
	csh.l_desc.max_code = 0;

	csh.d_desc.dyn_tree = csh.dyn_dtree;
	csh.d_desc.static_tree = csh.static_dtree;
	csh.d_desc.extra_bits = CsExtraLenBits;
	csh.d_desc.extra_base = 0;
	csh.d_desc.elems = D_CODES;
	csh.d_desc.max_length = MAX_BITS;
	csh.d_desc.max_code = 0;

	csh.bl_desc.dyn_tree = csh.bl_tree;
	csh.bl_desc.static_tree = NULL;
	csh.bl_desc.extra_bits = extra_blbits;
	csh.bl_desc.extra_base = 0;
	csh.bl_desc.elems = BL_CODES;
	csh.bl_desc.max_length = MAX_BL_BITS;
	csh.bl_desc.max_code = 0;
}




int CsObjectInt::CsComprLZH (SAP_INT    sumlen,
                SAP_BYTE * inbuf,
                SAP_INT    inlen,
                SAP_BYTE * outbuf,
                SAP_INT    outlen,
                SAP_INT    option,
                SAP_INT  * bytes_read,
                SAP_INT  * bytes_written)
{
  unsigned char Algo;
  SAP_INT rc;
  int level;

  if (option & CS_INIT_COMPRESS)
  {
     InitDesc ();
  }

  csh.in_buf     = inbuf;
  csh.in_size    = inlen;
  csh.in_offset  = 0;

  csh.out_buf    = outbuf;
  csh.out_size   = outlen;

  if (option & CS_INIT_COMPRESS)
  {
	  if (outlen < CS_HEAD_SIZE) 
	  {
	     return CS_E_OUT_BUFFER_LEN;
	  }

    csh.sum_size   = sumlen;
    csh.sum_in     = 0;
    csh.out_offset = CS_HEAD_SIZE;

    csh.bytes_pending = 0;

    level = (int) (option >> 4);
    if (level < 1 || level > 9) level = DEFAULT_LZH_LEVEL;
    csh.pack_level = level;

    Algo = (unsigned char)((CS_LZH_VERSION << 4) | CS_ALGORITHM_LZH);

    rc = CsSetHead (outbuf, sumlen, Algo, (unsigned char) level);
    if (rc < 0)
	{ 
		return (int) rc;
	}

    BitBufInit ();
    HufTabInit ();
    LongestMatchInit (level, &csh.flags);
	csh.init = 1;

    if (csh.out_offset == outlen) 
	{ 
		return CS_END_OUTBUFFER;
	}
  }
  else
  {
    csh.out_offset = 0;
    if (csh.bytes_pending)
    {
      if (csh.bytes_pending < (unsigned) csh.out_size)
      {
        memcpy (csh.out_buf, csh.InterBuf, (unsigned) csh.bytes_pending);
        csh.out_offset    = (unsigned) csh.bytes_pending;
        csh.bytes_pending = 0;
      }
      else
      {
        memcpy (csh.out_buf, csh.InterBuf, (unsigned) csh.out_size);
        if (csh.bytes_pending > (unsigned) csh.out_size)
          memmove (csh.InterBuf, csh.InterBuf + csh.out_size,
                                 (unsigned)(csh.bytes_pending - csh.out_size));
        csh.bytes_pending -= (unsigned) csh.out_size;
        *bytes_read    = 0;
        *bytes_written = csh.out_size;

        return CS_END_OUTBUFFER;
      }
    }
  }

  rc = ComprLZH (csh.init); /* !!! Korrektur: ab 21.10.96 */
  csh.init = 0;
  if (rc < 0) 
  { 
	  return (int) rc;
  }

  *bytes_read    = csh.in_offset;
  *bytes_written = rc;
  csh.sum_in   += csh.in_offset;

  if (csh.bytes_pending) return CS_END_OUTBUFFER;
  else
	  if (csh.sum_in >= csh.sum_size) 
	  {
		 return CS_END_OF_STREAM;
	  }
      else
         return CS_END_INBUFFER;
}


