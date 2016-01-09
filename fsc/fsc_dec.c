//Copyright 2014 The FSC Authors. All Rights Reserved.
//
//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//------------------------------------------------------------------------------
//
// Finite State Coder (FSC) decoder implementation
//
//  based on Jarek Duda's paper: http://arxiv.org/pdf/1311.2540v1.pdf
//
// Author: Skal (pascal.massimino@gmail.com)

#include "./fsc.h"
#include <stdio.h>
#include <assert.h>

#include "./bits.h"
#include "./alias.h"

//------------------------------------------------------------------------------
// Decoding

typedef enum {
  FSC_OK = 0,
  FSC_ERROR = 1,
  FSC_EOF = 2
} FSC_STATUS;

//------------------------------------------------------------------------------
// Generic methods for decoding process

typedef int (*FSCReadParamsFunc)(FSCDecoder* dec, FSCBitReader* br,
                                 uint32_t counts[MAX_SYMBOLS]);
typedef int (*FSCBuildTables)(FSCDecoder* dec, const uint32_t counts[]);
typedef int (*FSCGetBlockFunc)(FSCDecoder* dec, uint8_t* out, int size,
                               FSCBitReader* br);

typedef struct {
  FSCReadParamsFunc       read_params;
  FSCGetBlockFunc         get_block;
  FSCBuildTables          build_tables;
  FSCBuildSpreadTableFunc spread;
} DecMethods;
static const DecMethods kDecMethods[CODING_METHOD_LAST];

//------------------------------------------------------------------------------

typedef struct {
  int16_t next_;      // relative delta jump from this state to the next
  uint8_t symbol_;    // symbol associated to the state
  int8_t len_;        // number of bits to read for transitioning this state
} FSCState;

typedef struct {
  uint32_t start_;
  uint32_t freq_;
} Symbol;

struct FSCDecoder {
  FSCCodingMethod method_;
  DecMethods methods_;

  FSCBitReader br_;
  FSC_STATUS status_;
  int log_tab_size_;
  int max_symbol_;
  int unique_symbol_;
  uint32_t out_size_;

  FSCState tab_[TAB_SIZE];   // ~16k for LOG_TAB_SIZE=12

  Symbol symbols_[MAX_SYMBOLS];
  uint8_t map_[MAX_TAB_SIZE];
  AliasTable alias_;
};

//------------------------------------------------------------------------------
// State table building

static int SymbolsInit(FSCDecoder* dec,
                       const uint32_t counts[], int max_symbol) {
  uint32_t start = 0;
  int s;
  if (max_symbol > MAX_SYMBOLS || max_symbol <= 0) return 0;
  for (s = 0; s < max_symbol; ++s) {
    const uint32_t freq = counts[s];
    dec->symbols_[s].start_ = start & 0xffff;
    dec->symbols_[s].freq_ = freq;
    start += freq;
  }
  return (start == (1 << dec->log_tab_size_));
}

//------------------------------------------------------------------------------

static int BuildSymbolMap(FSCDecoder* dec,
                          const uint32_t counts[], int max_symbol) {
  if (!SymbolsInit(dec, counts, max_symbol)) return 0;
  uint32_t start = 0;
  int s;
  for (s = 0; s < max_symbol; ++s) {
    const uint32_t freq = counts[s];
    int i;
    for (i = 0; i < freq; ++i) dec->map_[start++] = s;
  }
  return 1;
}

static int BuildStateTableW(FSCDecoder* dec, const uint32_t counts[]) {
  return BuildSymbolMap(dec, counts, dec->max_symbol_);
}

static uint8_t NextSymbol(const FSCDecoder* const dec, FSCStateW* const state) {
  uint32_t rank;
  const uint32_t r = (*state) & (MAX_TAB_SIZE - 1);
  const uint8_t s = dec->map_[r];
  rank = r - dec->symbols_[s].start_;
  const int freq = dec->symbols_[s].freq_;
  *state = freq * ((*state) >> MAX_LOG_TAB_SIZE) + rank;
  return s;
}

//------------------------------------------------------------------------------

static int BuildStateTableAliasW(FSCDecoder* dec, const uint32_t counts[]) {
  return SymbolsInit(dec, counts, dec->max_symbol_) &&
         AliasInit(dec->alias_, counts, dec->max_symbol_);
}

static uint8_t NextSymbolAlias(const FSCDecoder* const dec, FSCStateW* const state) {
  uint32_t rank;
  const uint32_t r = (*state) & (MAX_TAB_SIZE - 1);
  const uint8_t s = AliasSearchSymbol(dec->alias_, r, &rank);
  const int freq = dec->symbols_[s].freq_;
  *state = freq * ((*state) >> MAX_LOG_TAB_SIZE) + rank;
  return s;
}

//------------------------------------------------------------------------------

static int Log2(uint32_t v) {
  int s = 31;
  while (v < (1 << s)) --s;
  return s;
}

static int BuildStateTable(FSCDecoder* dec, const uint32_t counts[]) {
  int s, i, pos;
  uint16_t state[MAX_SYMBOLS];   // next state of symbol 's'
  FSCState* const tab = dec->tab_;
  const int log_tab_size = dec->log_tab_size_;
  const int tab_size = 1 << log_tab_size;
  const int max_symbol = dec->max_symbol_;

  assert(max_symbol <= MAX_SYMBOLS && max_symbol > 0);
  uint8_t* const symbols = (uint8_t*)malloc(tab_size * sizeof(*symbols));
  if (symbols == NULL) return 0;
  if (!dec->methods_.spread(max_symbol, counts, log_tab_size, symbols)) {
    free(symbols);
    return 0;
  }

  uint8_t nb_bits[MAX_SYMBOLS];
  uint16_t wrap[MAX_SYMBOLS];
  for (s = 0; s < max_symbol; ++s) {
    state[s] = counts[s];
    const int len = (state[s] > 0) ? Log2(state[s]) : 0;
    nb_bits[s] = log_tab_size - len;
    wrap[s] = (2 << len);
  }

  for (pos = 0; pos < tab_size; ++pos) {
    s = symbols[pos];
    tab[pos].symbol_ = s;
    const int next_state = state[s]++;
    const int len = nb_bits[s] - (next_state >= wrap[s]);
    const int new_pos = (next_state << len) - tab_size;
    tab[pos].next_ = new_pos - pos;   // how to jump from Is to I
    tab[pos].len_  = len;
  }
  free(symbols);
  if (pos != tab_size) return 0;   // input not normalized!

  return 1;
}

//------------------------------------------------------------------------------
// Decoding loop

static int GetBlock(FSCDecoder* dec, uint8_t* out, int size, FSCBitReader* br) {
  const FSCState* state = dec->tab_;   // state_idx=0 at start
  int next_nb_bits = dec->log_tab_size_;
  int n;
  for (n = 0; n < size; ++n) {
    FSCFillBitWindow(br);
    state += FSCSeeBits(br) & ((1 << next_nb_bits) - 1);
    FSCDiscardBits(br, next_nb_bits);
    *out++ = state->symbol_;
    next_nb_bits = state->len_;
    state += state->next_;
  }
  return !br->eof_;
}

//------------------------------------------------------------------------------

#define RENORMALIZE_STATE(state) do {                         \
  if ((state) < FSC_MAX) {                                    \
    if (buf < buf_end) {                                      \
      (state) = ((state) << FSC_BITS) | (*buf++);             \
    } else {                                                  \
      lbr.eof_ |= 1;                                          \
    }                                                         \
  }                                                           \
} while (0)

static int GetBlockW1(FSCDecoder* dec, uint8_t* out, int size,
                      FSCBitReader* br) {
  FSCBitReader lbr = *br;  // it's faster to make a local copy
  const FSCType* buf = (const FSCType*)FSCBitAlign(&lbr);
  const FSCType* const buf_end = (const FSCType*)FSCGetByteEnd(&lbr);
  const Symbol* const syms = dec->symbols_;

  lbr.eof_ = (buf == buf_end);
  if (lbr.eof_) goto End;
  const FSCType* buf0 = buf;
  FSCStateW state = *buf++;

  int n;
  for (n = 0; n < size - FSC_BITS / 8; ++n) {
    RENORMALIZE_STATE(state);
    if (lbr.eof_) break;
    out[n] = NextSymbol(dec, &state);
  }
  RENORMALIZE_STATE(state);
  FSCSetReadBufferPos(&lbr, (const uint8_t*)buf);
  // The trailing bytes are encoded in the final state's lower bytes.
  while (state != 1 && n < size) {
    out[n++] = state & 0xff;
    state >>= 8;
  }
 End:
  *br = lbr;
  return !br->eof_;
}

static int GetBlockW2(FSCDecoder* dec, uint8_t* out, int size,
                      FSCBitReader* br) {
  FSCBitReader lbr = *br;  // it's faster to make a local copy
  const FSCType* buf = (const FSCType*)FSCBitAlign(&lbr);
  const FSCType* const buf_end = (const FSCType*)FSCGetByteEnd(&lbr);
  const Symbol* const syms = dec->symbols_;
  lbr.eof_ = (buf == buf_end);
  if (lbr.eof_) goto End;
  FSCStateW state1 = *buf++;
  FSCStateW state0 = (size > 1) ? (*buf++) : 0;

  int n;
  const int size_limit = (size - 2 * (FSC_BITS / 8)) & ~1;
  for (n = 0; n < size_limit; n += 2) {
    RENORMALIZE_STATE(state1);
    RENORMALIZE_STATE(state0);
    if (lbr.eof_) break;
    out[n + 0] = NextSymbol(dec, &state1);
    out[n + 1] = NextSymbol(dec, &state0);
  }
  RENORMALIZE_STATE(state1);
  RENORMALIZE_STATE(state0);
  if (size & 1) {
    RENORMALIZE_STATE(state1);
    if (!lbr.eof_) out[n++] = NextSymbol(dec, &state1);
  }

  FSCSetReadBufferPos(&lbr, (const uint8_t*)buf);
  // The trailing bytes are encoded in the final state's lower bytes.
  while (state1 != 1 && n < size) {
    out[n++] = state1 & 0xff;
    state1 >>= 8;
  }
  while (state0 != 1 && n < size) {
    out[n++] = state0 & 0xff;
    state0 >>= 8;
  }

 End:
  *br = lbr;
  return !br->eof_;
}

static int GetBlockW4(FSCDecoder* dec, uint8_t* out, int size,
                      FSCBitReader* br) {
  FSCBitReader lbr = *br;  // it's faster to make a local copy
  const FSCType* buf = (const FSCType*)FSCBitAlign(&lbr);
  const FSCType* const buf_end = (const FSCType*)FSCGetByteEnd(&lbr);
  const Symbol* const syms = dec->symbols_;
  FSCStateW states[4];
  lbr.eof_ = (buf == buf_end);
  if (lbr.eof_) goto End;
  int r;
  for (r = 0; r < 4; ++r) {
    states[r] = (size > 0) ? *buf++ : 0;
  }

  int n;
  for (n = 0; n < (size & ~3); n += 4) {
    RENORMALIZE_STATE(states[0]);
    RENORMALIZE_STATE(states[1]);
    RENORMALIZE_STATE(states[2]);
    RENORMALIZE_STATE(states[3]);
    if (lbr.eof_) break;
    out[n + 0] = NextSymbol(dec, &states[0]);
    out[n + 1] = NextSymbol(dec, &states[1]);
    out[n + 2] = NextSymbol(dec, &states[2]);
    out[n + 3] = NextSymbol(dec, &states[3]);
  }
  RENORMALIZE_STATE(states[0]);
  RENORMALIZE_STATE(states[1]);
  RENORMALIZE_STATE(states[2]);
  RENORMALIZE_STATE(states[3]);
  for (; n < size; ++n) {
    RENORMALIZE_STATE(states[n & 3]);
    if (!lbr.eof_) out[n] = NextSymbol(dec, &states[n & 3]);
    RENORMALIZE_STATE(states[n & 3]);
  }
  FSCSetReadBufferPos(&lbr, (const uint8_t*)buf);
 End:
  *br = lbr;
  return !br->eof_;
}

static int GetBlockAliasW1(FSCDecoder* dec, uint8_t* out, int size,
                           FSCBitReader* br) {
  FSCBitReader lbr = *br;  // it's faster to make a local copy
  const FSCType* buf = (const FSCType*)FSCBitAlign(&lbr);
  const FSCType* const buf_end = (const FSCType*)FSCGetByteEnd(&lbr);
  const Symbol* const syms = dec->symbols_;
  lbr.eof_ = (buf == buf_end);
  if (lbr.eof_) goto End;
  FSCStateW state = *buf++;

  int n;
  for (n = 0; n < size; ++n) {
    RENORMALIZE_STATE(state);
    if (lbr.eof_) break;
    out[n] = NextSymbolAlias(dec, &state);
  }
  RENORMALIZE_STATE(state);
  FSCSetReadBufferPos(&lbr, (const uint8_t*)buf);
 End:
  *br = lbr;
  return !br->eof_;
}

static int GetBlockAliasW2(FSCDecoder* dec, uint8_t* out, int size,
                           FSCBitReader* br) {
  FSCBitReader lbr = *br;  // it's faster to make a local copy
  const FSCType* buf = (const FSCType*)FSCBitAlign(&lbr);
  const FSCType* const buf_end = (const FSCType*)FSCGetByteEnd(&lbr);
  const Symbol* const syms = dec->symbols_;
  lbr.eof_ = (buf == buf_end);
  if (lbr.eof_) goto End;
  FSCStateW state1 = (*buf++);
  FSCStateW state0 = (size > 1) ? (*buf++) : 0;

  int n;
  for (n = 0; n + 1 < size; n += 2) {
    RENORMALIZE_STATE(state1);
    RENORMALIZE_STATE(state0);
    if (lbr.eof_) break;
    out[n + 0] = NextSymbolAlias(dec, &state1);
    out[n + 1] = NextSymbolAlias(dec, &state0);
  }
  RENORMALIZE_STATE(state0);
  if (size & 1) {
    RENORMALIZE_STATE(state1);
    if (!lbr.eof_) out[n++] = NextSymbolAlias(dec, &state1);
    RENORMALIZE_STATE(state0);
  }
  FSCSetReadBufferPos(&lbr, (const uint8_t*)buf);
 End:
  *br = lbr;
  return !br->eof_;
}

//------------------------------------------------------------------------------
// Header

static int ReadSequence(uint32_t seq[], int len, int sparse, int nb_bits,
                        FSCBitReader* br) {
  uint32_t total = 1 << nb_bits;
  uint32_t half = total >> 1;
  int i;
  if (sparse == 2) sparse = FSCReadBits(br, 1);
  for (i = 0; i < len - 1; ++i ) {
    uint16_t c;
    if (sparse && !FSCReadBits(br, 1)) {
      seq[i] = 0;
      continue;
    }
    c = FSCReadBits(br, nb_bits);
    seq[i] = c;
    if (total < c) return 0;   // normalization problem
    total -= c;
    if (total < half) {
      --nb_bits;
      half >>= 1;
    }
  }
  seq[len - 1] = total;   // remaining part
  return 1;
}

static int ReadHeader(FSCDecoder* dec, FSCBitReader* br, uint32_t counts[TAB_SIZE]) {
  const int log_tab_size = dec->log_tab_size_;
  const uint32_t tab_size = 1 << log_tab_size;
  const int max_symbol = 1 + FSCReadBits(br, 8);
  dec->max_symbol_ = max_symbol;
  dec->unique_symbol_ = -1;

  if (max_symbol < HDR_SYMBOL_LIMIT) {  // Use method #1 for small alphabet
    if (!ReadSequence(counts, max_symbol, 2, log_tab_size, br)) {
      return 0;
    }
  } else {  // Use more complex method #2 for large alphabet
    const int hlen = 1 + FSCReadBits(br, 5);
    uint32_t bHisto[LOG_TAB_SIZE + 1];
    uint8_t bins[MAX_SYMBOLS] = { 0 };
    if (hlen == 32) {   // sparse case
      int i;
      for (i = 0; i < max_symbol - 1; ++i) counts[i] = 0;
      counts[max_symbol - 1] = tab_size;
    } else {
      if (!ReadSequence(bHisto, hlen, 2, TAB_HDR_BITS, br)) {
        return 0;
      }
      {
        FSCDecoder dec2;
        memset(&dec2, 0, sizeof(dec2));
        dec2.log_tab_size_ = TAB_HDR_BITS;
        dec2.max_symbol_ = hlen;
        dec2.method_ = CODING_METHOD_BUCKET;
        dec2.methods_ = kDecMethods[dec2.method_];
        if (hlen > log_tab_size) return 0;
        if (!dec2.methods_.build_tables(&dec2, bHisto)) {
          fprintf(stderr, "Sub-Decoder initialization failed!\n");
          return 0;
        }
        dec2.methods_.get_block(&dec2, bins, max_symbol - 1, br);
      }
      {
        int i;
        uint32_t total = tab_size;
        for (i = 0; i < max_symbol - 1; ++i) {
          const int b = bins[i];
          const int residue = (b > 0) ? FSCReadBits(br, b) : 0;
          const int c = (1 << b) | residue;
          counts[i] = c - 1;
          if (total < counts[i]) return 0;   // normalization error
          total -= counts[i];
        }
        counts[max_symbol - 1] = total;   // remaining part
      }
    }
  }
  return !br->eof_;
}

static int ReadParams(FSCDecoder* dec, FSCBitReader* br,
                      uint32_t counts[MAX_SYMBOLS]) {
  dec->log_tab_size_ = LOG_TAB_SIZE - FSCReadBits(br, 4);
  return ReadHeader(dec, br, counts);
}

static int ReadParamsW(FSCDecoder* dec, FSCBitReader* br,
                       uint32_t counts[MAX_SYMBOLS]) {
  dec->log_tab_size_ = MAX_LOG_TAB_SIZE;
  return ReadHeader(dec, br, counts);
}

//------------------------------------------------------------------------------
// corner case of only-one-symbol

static int GetBlockUnique(FSCDecoder* dec, uint8_t* out, int size,
                          FSCBitReader* br) {
  memset(out, dec->unique_symbol_, size);
  return 1;
}

static int ReadParamsUnique(FSCDecoder* dec, FSCBitReader* br,
                            uint32_t counts[MAX_SYMBOLS]) {
  memset(counts, 0, MAX_SYMBOLS * sizeof(counts[0]));
  dec->unique_symbol_ = FSCReadBits(br, 8);
  dec->max_symbol_ = dec->unique_symbol_ + 1;
  dec->log_tab_size_ = MAX_LOG_TAB_SIZE;
  return !br->eof_;
}

static int BuildTableUnique(FSCDecoder* dec, const uint32_t counts[]) {
  (void)dec;
  (void)counts;
  return 1;
}

//------------------------------------------------------------------------------

static const DecMethods kDecMethods[CODING_METHOD_LAST] = {
  { ReadParams, GetBlock, BuildStateTable, BuildSpreadTableBucket },
  { ReadParams, GetBlock, BuildStateTable, BuildSpreadTableReverse },
  { ReadParams, GetBlock, BuildStateTable, BuildSpreadTableModulo },
  { ReadParams, GetBlock, BuildStateTable, BuildSpreadTablePack },

  { ReadParamsW, GetBlockW1, BuildStateTableW, NULL },
  { ReadParamsW, GetBlockW2, BuildStateTableW, NULL },
  { ReadParamsW, GetBlockAliasW1, BuildStateTableAliasW, NULL },
  { ReadParamsW, GetBlockAliasW2, BuildStateTableAliasW, NULL },

  { ReadParamsW, GetBlockW4, BuildStateTableW, NULL },

  { ReadParamsUnique, GetBlockUnique, BuildTableUnique, NULL },
};

//------------------------------------------------------------------------------

FSCDecoder* FSCInit(const uint8_t* input, size_t len) {
  FSCDecoder* dec = (FSCDecoder*)calloc(1, sizeof(*dec));
  if (dec == NULL) return NULL;

  FSCInitBitReader(&dec->br_, input, len);
  dec->unique_symbol_ = -1;
  dec->out_size_ = 0;
  int i;
  for (i = 0; i < 8 && FSCReadBits(&dec->br_, 1); ++i) {
    dec->out_size_ |= FSCReadBits(&dec->br_, 8) << (8 * i);
  }

  dec->method_ = (FSCCodingMethod)FSCReadBits(&dec->br_, 4);
  if (dec->method_ >= CODING_METHOD_LAST) goto Error;
  dec->methods_ = kDecMethods[dec->method_];

  uint32_t counts[MAX_SYMBOLS];
  if (!dec->methods_.read_params(dec, &dec->br_, counts) ||
      !dec->methods_.build_tables(dec, counts)) {
 Error:
    dec->status_ = FSC_ERROR;
  } else {
    dec->status_ = FSC_OK;
  }
  return dec;
}

int FSCIsOk(FSCDecoder* dec) {
  return (dec != NULL) && (dec->status_ != FSC_ERROR);
}

void FSCDelete(FSCDecoder* dec) {
  free(dec);
}

int FSCDecompress(FSCDecoder* dec, uint8_t** out, size_t* out_size) {
  if (dec == NULL || out == NULL || out_size == NULL) return 0;
  size_t size = dec->out_size_;
  int need_allocate = (*out == NULL);
  if (need_allocate) {
    *out = (uint8_t*)malloc(size * sizeof(*out));
    if (*out == NULL) return 0;
    *out_size = size;
  } else {
    if (*out_size < size) return 0;  // not enough room
  }

  uint8_t* ptr = *out;
  FSCGetBlockFunc get_block = dec->methods_.get_block;
  while (size > 0 && dec->status_ == FSC_OK) {
    const int next_size = (size > BLOCK_SIZE) ? BLOCK_SIZE : (int)size;
    if (!get_block(dec, ptr, next_size, &dec->br_)) {
      dec->status_ = FSC_EOF;
      break;
    }
    ptr += next_size;
    size -= next_size;
  }
  if (dec->status_ == FSC_ERROR) {
    if (need_allocate) {
      free(*out);
      *out = 0;
      *out_size = 0;
    }
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------

int FSCDecode(const uint8_t* in, size_t in_size, uint8_t** out, size_t* size) {
  FSCDecoder* const dec = FSCInit(in, in_size);
  if (dec == NULL || out == NULL || size == NULL) return 0;
  const int ok = FSCDecompress(dec, out, size) && FSCIsOk(dec);
  FSCDelete(dec);
  return ok;
}

//------------------------------------------------------------------------------
