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
// CsObjectInt.h: interface for the CsObjectInt class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSOBJECTINT_H__2AFFECE8_60FE_11D2_87FA_204C4F4F5020__INCLUDED_)
#define AFX_CSOBJECTINT_H__2AFFECE8_60FE_11D2_87FA_204C4F4F5020__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define BUF_SIZE1 (sizeof(COUNT_INT) * HSIZE > 2*WSIZE ? \
                   sizeof(COUNT_INT) * HSIZE : 2*WSIZE)


/* Not because of Windows 3.1X, but for Windows-Emulation on OS/2 -mt- */
#ifdef SAPonWINDOWS
#define DUMMY_SIZE 32768U
#else

#define DUMMY_SIZE (sizeof(HUFTREE) > 8 ? \
                    sizeof(HUFTREE) * 0x1E00 : 0xF000)
#endif

#define BUF_SIZE2 (sizeof(CODE_ENTRY) * CSIZE > DUMMY_SIZE ? \
                   sizeof(CODE_ENTRY) * CSIZE : DUMMY_SIZE)


/* Constants .........................................................*/

#define DEFAULT_LZH_LEVEL 2

#define MAX_BITS 15   /* All codes must not exceed MAX_BITS bits */

#define MAX_BL_BITS 7 /* Bit length codes must not exceed MAX_BL_BITS */

/* number of length codes, not counting the special END_BLOCK code */
#define LENGTH_CODES 29

#define LITERALS  256               /* number of literal bytes 0..255 */
#define END_BLOCK 256               /* end of block literal code */

/* number of Literal or Length codes, including the END_BLOCK code */
#define L_CODES (LITERALS+1+LENGTH_CODES)

#define D_CODES   30                /* number of distance codes */
#define BL_CODES  19  /* no of codes used to transfer the bit lengths */

#define STATIC_TREES 1
#define DYN_TREES    2

/*
 * The current code is general and allows CS_DIST_BUFSIZE < CS_LIT_BUFSIZE (to save
 * memory at the expense of compression). Some optimizations would be possible
 * if we rely on CS_DIST_BUFSIZE == CS_LIT_BUFSIZE.
 */

/* repeat previous bit length 3-6 times (2 bits of repeat count) */
#define REP_3_6      16

/* repeat a zero length 3-10 times  (3 bits of repeat count) */
#define REPZ_3_10    17

/* repeat a zero length 11-138 times  (7 bits of repeat count) */
#define REPZ_11_138  18

/* Local data ........................................................*/

/* Data structure describing a single value and its code string. */
typedef struct CT_DATA
{
    union
    {
        unsigned short freq;       /* frequency count */
        unsigned short code;       /* bit string */
    } fc;
    union
    {
        unsigned short dad;        /* father node in Huffman tree */
        unsigned short len;        /* length of bit string */
    } dl;
} CT_DATA;

#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len

struct HUFT
{
  unsigned char e;                /* number of extra bits or operation .........*/
  unsigned char b;                /* number of bits in this code or subcode ....*/
  union
  {
    unsigned short n;   /* literal, length base, or distance base ....*/
    struct HUFT *t;     /* pointer to next level of table ............*/
  } v;
};

typedef struct HUFT HUFTREE;
#define DE_STACK_SIZE 0x1000

#define HEAP_SIZE (2*L_CODES+1)            /* maximum heap size ......*/

typedef struct TREE_DESC
{
    CT_DATA *dyn_tree;           /* the dynamic tree */
    CT_DATA *static_tree;        /* corresponding static tree or NULL */
    int     *extra_bits;         /* extra bits for each code or NULL */
    int     extra_base;          /* base index for extra_bits */
    int     elems;               /* max number of elements in the tree */
    int     max_length;          /* max bit length for the codes */
    int     max_code;            /* largest code with non zero freq. */
} TREE_DESC;




typedef struct CSH
{
	int init;
	SAP_UINT sum_size;
	SAP_UINT sum_in;
	SAP_INT in_offset;
	SAP_INT out_offset;
	unsigned int bytes_pending;
	SAP_INT BlockStart;
	unsigned StrStart;
	unsigned MatchStart;
	int MatchAvail;
	unsigned MatchLen;
	unsigned ins_h;
	int PrevLen;

	int eoInput;            /* flag set at end of input ...........*/
	int eoData;             /* flag set at end of data ............*/
	unsigned Lookahead;     /* no of valid bytes ahead in window ..*/

	int pack_level;         /* index into config table ............*/
	CT_DATA dyn_ltree[HEAP_SIZE];   /* literal/len tree */
	CT_DATA dyn_dtree[2*D_CODES+1]; /* distance tree */

/* The static literal tree. Since the bit lengths are imposed, there is no
 * need for the L_CODES extra codes used during heap construction. However
 * The codes 286 and 287 are needed to build a canonical tree
 * (see HufTabInit below). */

	CT_DATA static_ltree[L_CODES+2];

/* The static distance tree.
 * (Actually a trivial tree since all codes use 5 bits.) */

	CT_DATA static_dtree[D_CODES];

/* Huffman tree for the bit lengths */
	CT_DATA bl_tree[2*BL_CODES+1];  /*Huffman tree for the bit lens*/

	TREE_DESC l_desc;

	TREE_DESC d_desc;

	TREE_DESC bl_desc; 

	SAP_BYTE window[(BUF_SIZE1)];
	SAP_BYTE InterBuf[(BUF_SIZE2)];

	unsigned short bl_count[MAX_BITS+1];

/* The lengths of the bit length codes are sent in order of decreasing
 * probability, to avoid transmitting the lengths for unused
 * bit length codes. */


/* The sons of heap[n] are heap[2*n] and heap[2*n+1].
   heap[0] is not used. The same heap array is used to build
   all trees. */

	int heap[2*L_CODES+1]; /* heap used to build the Huffman trees */
	int heap_len;               /* number of elements in the heap */
	int heap_max;               /* element of largest frequency */


/* Depth of each subtree used as tie breaker for trees of equal freq. */
	unsigned short depth[2*L_CODES+1];

/* length code for each normalized match length (0 == MIN_MATCH) */
	unsigned short length_code[MAX_MATCH-MIN_MATCH+1];

/* distance codes. The first 256 values correspond to the distances
 * 3 .. 258, the last 256 values correspond to the top 8 bits of
 * the 15 bit distances. */

	unsigned short dist_code[512];

/* First normalized length for each code (0 = MIN_MATCH) */
	int base_length[LENGTH_CODES];

/* First normalized distance for each code (0 = distance of 1) */
	int base_dist[D_CODES];

/* flag_buf is a bit array distinguishing literals from lengths in
   CsLitLenBuf, and thus indicating the presence or absence
   of a distance. */

	unsigned short flag_buf[(CS_LIT_BUFSIZE/8)];

	unsigned last_lit;       /* running index in CsLitLenBuf */
	unsigned last_dist;      /* running index in CsDistBuf */
	unsigned last_flags;     /* running index in flag_buf */
	SAP_BYTE flags;          /* current flags not yet in flag_buf */
	SAP_BYTE flag_bit;       /* current bit used in flags */

/* bits are filled in flags starting at bit 0 (least significant).
 * Note: these flags are overkill in the current code since we don't
 * take advantage of CS_DIST_BUFSIZE == CS_LIT_BUFSIZE. */

	SAP_UINT opt_len;       /* bit len of block with optimal trees */
	SAP_UINT static_len;    /* bit len of block with static trees */

	unsigned bi_buf;

/* Number of bits used within bi_buf. (bi_buf might be implemented on
 * more than 16 bits on some systems.) */

/* Number of valid bits in bi_buf.  All bits above the last valid bit
 * are always zero. */
	int bi_valid;


/* Current input and output buffers ..................................*/
	SAP_BYTE *in_buf, *out_buf;

/* Size of current input and output buffers ..........................*/
	SAP_INT in_size, out_size;

  /* Link to older string with same hash index.
     To limit the size of this array to 64K, this link is
     maintained only for the last WSIZE strings.
     An index in this array is thus a window index modulo WSIZE. */

  unsigned short CsPrev[WSIZE];

  /* Heads of the hash chains or NIL .................................*/
  unsigned short CsHead[CS_HASH_SIZE];

  SAP_BYTE CsLitLenBuf[CS_LIT_BUFSIZE]; /* buffer for literals/lengths */
  unsigned short CsDistBuf[CS_DIST_BUFSIZE]; /* buffer for distances */

} CSH;



typedef struct CSHU
{
 SAP_BYTE *OutPtr;
 SAP_UINT SumOut;

 SAP_BYTE *MemOutbuffer, *MemInbuffer;
 unsigned MemOutoffset, MemOutsize, MemInoffset, MemInsize;
 unsigned BytesPending, SlideOffset;
 unsigned wp;

 SAP_UINT bb;                         /* bit buffer ............*/
 unsigned bk;                         /* bits in bit buffer ....*/

 SAP_UINT bytebuf;
 unsigned AllocStackSize;

 HUFTREE *htp;     /* pointer to table entry */
 unsigned save_n, save_d, save_e;

 int lastblockflag;                /* last block flag ....................*/
 int staterun;            /* state of last run ..................*/
 int NonSenseflag;
 SAP_INT OrgLen;
 unsigned blocktype;         /* block type */

 HUFTREE *tlitlen;      /* literal/length code table */
 HUFTREE *tdistcode;    /* distance code table */
 int blitlen;           /* lookup bits for tl */
 int bdistlen;          /* lookup bits for td */

 int dd_ii;
 unsigned dd_jj;
 unsigned dd_lastlen;     /* last length */
 unsigned dd_maskbit;     /* mask for bit lengths table */
 unsigned dd_nolen;       /* number of lengths to get */
 HUFTREE *dd_tl;          /* literal/length code table */
 HUFTREE *dd_td;          /* distance code table */
 int      dd_bl;          /* lookup bits for tl */
 int      dd_bd;          /* lookup bits for td */
 unsigned dd_nb;          /* number of bit length codes */
 unsigned dd_nl;          /* number of literal/length codes */
 unsigned dd_nd;          /* number of distance codes */
 unsigned dd_ll[286+30];  /* literal/length and distance lengths */
 SAP_BYTE Slide[(BUF_SIZE1)];
 HUFTREE InterBuf[DE_STACK_SIZE];
} CSHU;


typedef struct CSC
{
 int n_bits;                 /* number of bits/code ............*/
 int maxbits;                /* user settable max # bits/code ..*/
 CODE_INT maxcode;           /* maximum code, given n_bits .....*/

                                   /* storage for GETCODE / PUTCODE ..*/
 BYTE_TYP buf1[MAX_CS_BITS];

 int cs_offset;
 int csc_offset;
 int put_n_bytes;

 BYTE_TYP *outptr;
 BYTE_TYP *end_outbuf;

                                   /* should never be generated ......*/
 CODE_INT maxmaxcode; /* = (CODE_INT)1 << CS_BITS; */

 COUNT_INT LARGE_ARRAY htab [HSIZE];      /* hash table ........*/
 CODE_ENTRY LARGE_ARRAY codetab [CSIZE];  /* code table ........*/
 CODE_ENTRY Prefixtab[(BUF_SIZE1)/4]; // = (CODE_ENTRY *) &CsDeWindowBuf[0];
 BYTE_TYP Suffixtab[(BUF_SIZE2)/4]; // = (BYTE_TYP *) &CsDeInterBuf[0];
 CODE_INT hsize; // = HSIZE;     /* for dynamic table sizing .......*/


 CODE_INT  free_ent;            /* first unused entry ..........*/
 long int  bytes_out;           /* length of compressed output .*/
 long int  rest_len;            /* rest bytes to decompress ....*/
 int       block_compress;      /* block compression ...........*/
 int       clear_flg;           /* clear hash table ............*/
 long int  ratio;               /* compression ratio ...........*/
 COUNT_INT checkpoint;          /* ratio check point for compr. */

/* states for get_code ...............................................*/
 int get_size, get_r_bits;

 BYTE_TYP * in_ptr;             /* global input ptr ............*/
 BYTE_TYP * end_inbuf;          /* end of input buffer .........*/
 BYTE_TYP *stack_end;  

 int hshift;
 CODE_INT ent;
 int sflush;
 long org_len;
 long in_count_sum;


 BYTE_TYP *sstackp; // = (BYTE_TYP *) 0;
 long dorg_len;
 CODE_INT scode, soldcode, sincode, sfinchar;
 int restart;

} CSC;


class CsObjectInt
{
public:
	CsObjectInt ();
	~CsObjectInt ();

	int CsCompr (SAP_INT    sumlen,
             SAP_BYTE * inbuf,
             SAP_INT    inlen,
             SAP_BYTE * outbuf,
             SAP_INT    outlen,
             SAP_INT    option,
             SAP_INT  * bytes_read,
             SAP_INT  * bytes_written);

	int CsDecompr (SAP_BYTE * inbuf,     /* ptr input .......*/
               SAP_INT    inlen,         /* len of input ....*/
               SAP_BYTE * outbuf,        /* ptr output ......*/
               SAP_INT    outlen,        /* len output ......*/
               SAP_INT    option,        /* decompr. option  */
               SAP_INT *  bytes_read,    /* bytes read ......*/
               SAP_INT *  bytes_decompressed); /* bytes decompr.  */

	int CsGetAlgorithm (SAP_BYTE * data);

	SAP_INT CsGetLen (SAP_BYTE * data);
	int CsGetVersion (SAP_BYTE * data);
	int CsSetHead (SAP_BYTE * outbuf,
               SAP_INT    len,
               SAP_BYTE   veralg,
               SAP_BYTE   special);
	int CsInitCompr (SAP_BYTE * outbuf, SAP_INT sumlen, SAP_INT option);
	int CsInitDecompr (SAP_BYTE * inbuf);

private:
	int algorithm;

	int CsDecomprLZH (SAP_BYTE * inp,             /* ptr input .......*/
                  SAP_INT    inlen,               /* len of input ....*/
                  SAP_BYTE * outp,                /* ptr output ......*/
                  SAP_INT    outlen,              /* len output ......*/
                  SAP_INT    option,              /* decompr. option  */
                  SAP_INT *  bytes_read,          /* bytes read ......*/
                  SAP_INT *  bytes_decompressed); /* bytes decompr.   */

	int CsComprLZH (SAP_INT    sumlen,
                SAP_BYTE * inbuf,
                SAP_INT    inlen,
                SAP_BYTE * outbuf,
                SAP_INT    outlen,
                SAP_INT    option,
                SAP_INT  * bytes_read,
                SAP_INT  * bytes_written);

	union
	{
		CSHU cshu;
		CSH  csh;
		CSC  csc;
	};

/* decompress functions in csulzh.cpp */
 int BuildHufTree (unsigned *, unsigned, unsigned, int *, int *,
                         HUFTREE **, int *);
 HUFTREE * AllocHufTree (unsigned size);
 int FlushOut (unsigned);
 int DecompCodes (int * state, HUFTREE *, HUFTREE *, int, int);
 int DecompFixed (int *state);
 int DecompDynamic (int *state);
 void NoBits (void);
 int DecompBlock (int *state, int * x);
 void SendBits (unsigned value, int len);

/* compress functions in cslzh.cpp */

 void HashInit (void);
 void LongestMatchInit (int level, SAP_BYTE *flags);
 int  LongestMatch (unsigned cur_match);
 void FillWindow   (void);
 int  MemRead (SAP_BYTE *buf, unsigned size);
 SAP_INT ComprLZH (unsigned opt);

 int  ct_tally (int dist, int lc);
 void FlushBlock (int eof);
 void HufTabInit (void);

 void InitBlock     (void);
 void pqdownheap     (CT_DATA *tree, int k);
 void GenBitLen     (TREE_DESC *desc);
 void GenCodes      (CT_DATA *tree, int max_code);
 void BuildTree     (TREE_DESC *desc);
 void ScanTree      (CT_DATA *tree, int max_code);
 void SendTree      (CT_DATA *tree, int max_code);
 int  BuildBlTree   (void);
 void SendAllTrees  (int lcodes, int dcodes, int blcodes);
 void CompressBlock (CT_DATA *ltree, CT_DATA *dtree);
 void BitBufInit    (void);
 void FlushIncomp   (void);
 unsigned short ReverseCode (unsigned code, int len);
 void InitDesc (void);

 /* cslzc.cpp */
 int InitComp (BYTE_TYP * outbuf,
                     SAP_INT outlen,
                     SAP_INT sumlen);

 int ClearBlock (SAP_INT in_count, SAP_INT bytes_out);
 CODE_INT GetCode (void);
 int PutCode (CODE_INT c);
  SAP_INT CsGetStorageSize (void);

 int CsComprLZC (SAP_INT    sumlen,
                SAP_BYTE * inbuf,
                SAP_INT    inlen,
                SAP_BYTE * outbuf,
                SAP_INT    outlen,
                SAP_INT    option,
                SAP_INT *  bytes_read,
                SAP_INT *  bytes_written);

 int CsDecomprLZC (SAP_BYTE * inbuf,
                  SAP_INT    inlen,
                  SAP_BYTE * outbuf,
                  SAP_INT    outlen,
                  SAP_INT    option,
                  SAP_INT *  bytes_read,
                  SAP_INT *  bytes_written);

};



#endif // !defined(AFX_CSOBJECTINT_H__2AFFECE8_60FE_11D2_87FA_204C4F4F5020__INCLUDED_)
