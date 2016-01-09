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
// Finite State Coder (FSC) encoder implementation
//
//  based on Jarek Duda's paper: http://arxiv.org/pdf/1311.2540v1.pdf
//
// Author: Skal (pascal.massimino@gmail.com)

#include "./fsc.h"

#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "./bits.h"
#include "./alias.h"

#define USE_INV_DIV  // for speeding up encoder

typedef struct FSCEncoder FSCEncoder;

// #define SHOW_SIMULATION

//------------------------------------------------------------------------------
// States and tables

typedef void (*FSCPutBlockFunc)(const FSCEncoder* enc, const uint8_t* in, int size,
                                FSCBitWriter* const bw);
typedef int (*FSCBuildTablesFunc)(FSCEncoder* const enc, const uint32_t counts[]);
typedef int (*FSCWriteParamsFunc)(FSCEncoder* const enc,
                                  const uint32_t counts[MAX_SYMBOLS],
                                  FSCBitWriter* const bw);

typedef struct {   // encoding interface
  FSCWriteParamsFunc      write_params;
  FSCPutBlockFunc         put_block;
  FSCBuildTablesFunc      build_tables;
  FSCBuildSpreadTableFunc spread;
} EncMethods;
static const EncMethods kEncMethods[CODING_METHOD_LAST];

//------------------------------------------------------------------------------

typedef struct {
  int32_t offset_;
  uint16_t wrap_;
  uint8_t  nb_bits_;
} transf_t;

typedef struct {
  uint32_t start_;
  uint32_t freq_;
#if defined(USE_INV_DIV)
  uint64_t mult_;
  uint32_t imult_;
#endif
} Symbol;

struct FSCEncoder {
  int method_;
  EncMethods methods_;
  int max_symbol_;
  int unique_symbol_;
  uint16_t states_[TAB_SIZE];
  transf_t transforms_[MAX_SYMBOLS];
  size_t in_size_;
  int log_tab_size_;

  Symbol symbols_[MAX_SYMBOLS];
  uint16_t alias_map_[MAX_TAB_SIZE];
};


//------------------------------------------------------------------------------

static int Log2Ceil(uint32_t v) {   // not a critical function
  if (v > 1) {
    int s = 31;
    while (v <= (1U << s)) --s;
    return s + 1;
  } else {
    return 0;
  }
}

// -----------------------------------------------------------------------------

static int BuildTables(FSCEncoder* const enc, const uint32_t counts[]) {
  int s, pos;
  const int log_tab_size = enc->log_tab_size_;
  const int tab_size = 1 << log_tab_size;
  uint16_t state[MAX_SYMBOLS];
  uint8_t* symbols;  // symbols, spread on the [0, tab_size) interval
  const int max_symbol = enc->max_symbol_;
  uint16_t* const tab = enc->states_;
  transf_t* const transforms = enc->transforms_;

  if (max_symbol > MAX_SYMBOLS || max_symbol <= 0) return 0;

  for (s = 0, pos = 0; s < max_symbol; ++s) {
    int cnt = counts[s];
    // start of Is segment of symbol 's' in the states_ array
    // Length of the Is segment: cnt
    // Sum of all segments = tab_size
    state[s] = pos;
    // We map the [tab_size, 2*tab_size) segment to Is segments
    // and then remap then to I using symbols[]
    if (cnt > 0) {
      transf_t* const t = &transforms[s];
      t->nb_bits_ = log_tab_size - Log2Ceil(cnt);  // log(1/ps)
      t->wrap_ = cnt << (1 + t->nb_bits_);
      t->offset_ = pos - cnt;
      pos += cnt;
    }
  }
  if (pos != tab_size) return 0;   // input not normalized!

  symbols = (uint8_t*)malloc(tab_size * sizeof(*symbols));
  if (symbols == NULL) return 0;

  // Prepare map from symbol to state
  if (!enc->methods_.spread(max_symbol, counts, log_tab_size, symbols)) {
    free(symbols);
    return 0;
  }
  for (pos = 0; pos < tab_size; ++pos) {
    const uint8_t s = symbols[pos];
    tab[state[s]++] = pos + tab_size;
  }
  free(symbols);
  return max_symbol;
}

#if defined(USE_INV_DIV)

// As mentioned by Ryg:
//   Alverson 1991: "Integer Division Using Reciprocals"
//   http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.33.1710
#define MULT_SHIFT (8 * sizeof(FSCStateW) + FSC_BITS)
#define DIV_BY_MULT(A, B) (((A) * (B)) >> MULT_SHIFT)

void EncodeDividers(Symbol syms[], int max_symbol) {
  int s;
  for (s = 0; s < max_symbol; ++s) {
    Symbol* const sym = &syms[s];
    const uint32_t freq = sym->freq_;
    sym->imult_ = (1u << MAX_LOG_TAB_SIZE) - freq;
    if (freq > 0) {
      sym->mult_ = ((1ull << MULT_SHIFT) + freq - 1) / freq;
    } else {
      sym->mult_ = 0;  // shouldn't be needed
    }
  }
}
#endif

static int BuildTablesW(FSCEncoder* const enc, const uint32_t counts[]) {
  int s;
  uint64_t start = 0;
  const int log_tab_size = enc->log_tab_size_;
  const int tab_size = 1 << log_tab_size;
  const int max_symbol = enc->max_symbol_;

  for (s = 0; s < max_symbol; ++s) {
    enc->symbols_[s].start_ = start;
    enc->symbols_[s].freq_ = counts[s];
    start += counts[s];
  }
  if (start != tab_size) return 0;   // not normalized?

#if defined(USE_INV_DIV)
  EncodeDividers(enc->symbols_, max_symbol);
#endif
  return 1;
}

static int BuildTablesAliasW(FSCEncoder* const enc, const uint32_t counts[]) {
  return BuildTablesW(enc, counts) &&
         AliasBuildEncMap(counts, enc->max_symbol_, enc->alias_map_);
}

static int IsUniqueSymbol(int max_symbol, const uint32_t counts[]) {
  int i;
  int unique = max_symbol;
  for (i = 0; i < max_symbol; ++i) {
    if (counts[i]) {
      if (unique == max_symbol) {
        unique = i;
      } else {
        unique = -1;   // more than one symbol
        break;
      }
    }
  }
  assert(unique < max_symbol);
  return unique;
}

static int EncoderInit(FSCEncoder* const enc, uint32_t counts[],
                       int max_symbol, int log_tab_size,
                       FSCCodingMethod method) {
  int ok = 0;
  memset(enc, 0, sizeof(*enc));
  if (max_symbol == 0) max_symbol = MAX_SYMBOLS;
  if (log_tab_size < 1) return 0;
  if (method >= CODING_METHOD_LAST) return 0;

  if (method >= CODING_METHOD_16B) {
    log_tab_size = MAX_LOG_TAB_SIZE;
  } else if (log_tab_size > LOG_TAB_SIZE) {
    fprintf(stderr, "!! log_tab_size: %d\n", log_tab_size);
    return 0;
  }
  enc->log_tab_size_ = log_tab_size;
  enc->max_symbol_ = FSCNormalizeCounts(counts, max_symbol, log_tab_size);
  if (enc->max_symbol_ < 1) {
    fprintf(stderr, "!! enc->max_symbol_: %d\n", enc->max_symbol_);
    return 0;
  }

  enc->unique_symbol_ = IsUniqueSymbol(max_symbol, counts);
  assert(enc->unique_symbol_ < max_symbol);
  if (enc->unique_symbol_ >= 0) {
    method = CODING_METHOD_UNIQUE;
  }
  if (enc->max_symbol_ > (1 << log_tab_size)) return 0;

  enc->method_ = method;
  enc->methods_ = kEncMethods[method];
  return enc->methods_.build_tables(enc, counts);
}

// -----------------------------------------------------------------------------
// Coding loop

typedef struct {  // for delayed bitstream writing
  uint16_t val_;
  uint8_t  nb_bits_;
} token_t;

static void PutBlock(const FSCEncoder* enc, const uint8_t* in, int size,
                     FSCBitWriter* bw) {
  token_t tokens[BLOCK_SIZE];
  const transf_t* const transforms = enc->transforms_;
  const uint16_t* const states = enc->states_;
  const int log_tab_size = enc->log_tab_size_;
  const int tab_size = 1 << log_tab_size;
  int state = tab_size;
  int k;
  for (k = size - 1; k >= 0; --k) {
    const transf_t* const transf = &transforms[in[k]];
    const int extra_bit = (state >= transf->wrap_);
    const int nb_bits = transf->nb_bits_ + extra_bit;
    tokens[k].nb_bits_ = nb_bits;
    tokens[k].val_ = state & ((1 << nb_bits) - 1);
    state = states[(state >> nb_bits) + transf->offset_];
  }
  // Direction reversal
  FSCWriteBits(bw, state & (tab_size - 1), log_tab_size);
  for (k = 0; k < size - 1; ++k) {   // no need to write the last token
    FSCWriteBits(bw, tokens[k].val_, tokens[k].nb_bits_);
  }
}

// -----------------------------------------------------------------------------

#define FLUSH_STATE(state, limit) do {                                     \
  if ((state) >= (limit)) {                                                \
    output[--pos] = (FSCType)((state) & FSC_BITS_MASK);                    \
    (state) >>= FSC_BITS;                                                  \
  }                                                                        \
} while (0)

#if defined(USE_INV_DIV)
// Alternative version, which is a little slower than below:
//   const FSCStateW R = state - q * freq;    // <- that's 'state % freq'
//   state = (q << MAX_LOG_TAB_SIZE) + R + start;
#define RENORMALIZE_STATE(state, s) do {                                   \
  const uint32_t start = (s)->start_;                                      \
  const uint32_t q = DIV_BY_MULT(state, s->mult_);                         \
  state = q * s->imult_ + start + state;                                   \
} while (0)
#else
// reference calculation
#define RENORMALIZE_STATE(state, s) do {                                   \
  const uint32_t freq = (s)->freq_, start = (s)->start_;                   \
  state = ((state / freq) << MAX_LOG_TAB_SIZE) + (state % freq) + start;   \
} while (0)
// slower version:
//    (state / freq) * ((1 << MAX_LOG_TAB_SIZE) - freq) + state + start;
#endif  // USE_INV_DIV

// with ALIAS:
#if defined(USE_INV_DIV)
#define RENORMALIZE_STATE_ALIAS(state, s) do {                             \
  const uint32_t freq = (s)->freq_, start = (s)->start_;                   \
  const uint32_t q = DIV_BY_MULT(state, s->mult_);                         \
  const uint32_t R = state - q * freq;    /* <- that's 'state % freq' */   \
  state = (q << MAX_LOG_TAB_SIZE) + enc->alias_map_[R + start];            \
} while (0)
#else
#define RENORMALIZE_STATE_ALIAS(state, s) do {                             \
  const uint32_t freq = (s)->freq_, start = (s)->start_;                   \
  state = ((state / freq) << MAX_LOG_TAB_SIZE)                             \
        + enc->alias_map_[(state % freq) + start];                         \
} while (0)
#endif   // USE_INV_DIV

static int DoPutBlockW1(const FSCEncoder* enc, const uint8_t* in, int size,
                        FSCType output[BLOCK_SIZE]) {
  const FSCStateW norm = (FSC_MAX >> MAX_LOG_TAB_SIZE) << FSC_BITS;
  int pos = BLOCK_SIZE;
  FSCStateW state = 1;
  int k = size;
  // We encode the first few bytes into initial state.
  while (state < FSC_MAX && k > 0) {
    state = (state << 8) | in[--k];
  }
  assert(enc->log_tab_size_ == MAX_LOG_TAB_SIZE);
  while (k > 0) {
    const Symbol* const s = &enc->symbols_[in[--k]];
    FLUSH_STATE(state, norm * s->freq_);
    RENORMALIZE_STATE(state, s);
  }
  FLUSH_STATE(state, 0);
  FLUSH_STATE(state, 0);
  return pos;
}

static int DoPutBlockW2(const FSCEncoder* enc, const uint8_t* in, int size,
                        FSCType output[BLOCK_SIZE]) {
  int pos = BLOCK_SIZE;
  FSCStateW state0 = 1, state1 = 1;
  const FSCStateW norm = (FSC_MAX >> MAX_LOG_TAB_SIZE) << FSC_BITS;
  int k = size;
  assert(enc->log_tab_size_ == MAX_LOG_TAB_SIZE);
  // We encode the first few bytes into initial states.
  while (state0 < FSC_MAX && k > 0) {
    state0 = (state0 << 8) | in[--k];
  }
  while (state1 < FSC_MAX && k > 0) {
    state1 = (state1 << 8) | in[--k];
  }
  if (k & 1) {
    const Symbol* const s1 = &enc->symbols_[in[--k]];
    FLUSH_STATE(state1, norm * s1->freq_);
    RENORMALIZE_STATE(state1, s1);
  }
  while (k > 0) {
    const Symbol* const s0 = &enc->symbols_[in[--k]];
    const Symbol* const s1 = &enc->symbols_[in[--k]];
    FLUSH_STATE(state0, norm * s0->freq_);
    FLUSH_STATE(state1, norm * s1->freq_);
    RENORMALIZE_STATE(state0, s0);
    RENORMALIZE_STATE(state1, s1);
  }
  FLUSH_STATE(state0, 0);
  FLUSH_STATE(state1, 0);
  if (size > 1) {
    FLUSH_STATE(state0, 0);
    FLUSH_STATE(state1, 0);
  }
  return pos;
}

static int DoPutBlockW4(const FSCEncoder* enc, const uint8_t* in, int size,
                        FSCType output[BLOCK_SIZE]) {
  int pos = BLOCK_SIZE;
  FSCStateW states[4] = { FSC_MAX, FSC_MAX, FSC_MAX, FSC_MAX };
  const FSCStateW norm = (FSC_MAX >> MAX_LOG_TAB_SIZE) << FSC_BITS;
  int k = size;
  int r = size & 3;
  assert(enc->log_tab_size_ == MAX_LOG_TAB_SIZE);
  while (r-- > 0) {
    const Symbol* const s = &enc->symbols_[in[--k]];
    FLUSH_STATE(states[3 - r], norm * s->freq_);
    RENORMALIZE_STATE(states[3 - r], s);
  }
  while (k > 0) {
    const Symbol* const s0 = &enc->symbols_[in[--k]];
    const Symbol* const s1 = &enc->symbols_[in[--k]];
    const Symbol* const s2 = &enc->symbols_[in[--k]];
    const Symbol* const s3 = &enc->symbols_[in[--k]];
    FLUSH_STATE(states[0], norm * s0->freq_);
    FLUSH_STATE(states[1], norm * s1->freq_);
    FLUSH_STATE(states[2], norm * s2->freq_);
    FLUSH_STATE(states[3], norm * s3->freq_);
    RENORMALIZE_STATE(states[0], s0);
    RENORMALIZE_STATE(states[1], s1);
    RENORMALIZE_STATE(states[2], s2);
    RENORMALIZE_STATE(states[3], s3);
  }
  for (r = 0; r < 4; ++r) {
    FLUSH_STATE(states[r], 0);
  }
  for (r = 0; r < 4; ++r) {
    if (size > 0) FLUSH_STATE(states[r], 0);
  }
  return pos;
}

// Generic N-states interleaving function (slow)
#if 0
#define NB_STATES 8
static int DoPutBlockWN(const FSCEncoder* enc, const uint8_t* in, int size,
                        FSCType output[BLOCK_SIZE]) {
  int pos = BLOCK_SIZE;
  FSCStateW states[NB_STATES];
  const FSCStateW norm = (FSC_MAX >> MAX_LOG_TAB_SIZE) << FSC_BITS;
  int k = size;
  int r;
  assert(enc->log_tab_size_ == MAX_LOG_TAB_SIZE);
  for (r = 0; r < NB_STATES; ++r) {
    states[r] = FSC_MAX;
  }
  r = size & (NB_STATES - 1);
  while (r-- > 0) {
    const Symbol* const s = &enc->symbols_[in[--k]];
    FLUSH_STATE(states[NB_STATES - 1 - r], norm * s->freq_);
    RENORMALIZE_STATE(states[NB_STATES - 1 - r], s);
  }
  while (k > 0) {
    for (r = 0; r < NB_STATES; ++r) {
      const Symbol* const s = &enc->symbols_[in[--k]];
      FLUSH_STATE(states[r], norm * s->freq_);
      RENORMALIZE_STATE(states[r], s);
    }
  }
  for (r = 0; r < 2 * NB_STATES; ++r) {
    FLUSH_STATE(states[r & (NB_STATES - 1)], 0);
  }
  return pos;
}
#endif

// -----------------------------------------------------------------------------

static int DoPutBlockAliasW1(const FSCEncoder* enc, const uint8_t* in, int size,
                             FSCType output[BLOCK_SIZE]) {
  int pos = BLOCK_SIZE;
  FSCStateW state = FSC_MAX;
  const FSCStateW norm = (FSC_MAX >> MAX_LOG_TAB_SIZE) << FSC_BITS;
  int k = size;
  assert(enc->log_tab_size_ == MAX_LOG_TAB_SIZE);
  while (k > 0) {
    const Symbol* const s = &enc->symbols_[in[--k]];
    FLUSH_STATE(state, norm * s->freq_);
    RENORMALIZE_STATE_ALIAS(state, s);
  }
  FLUSH_STATE(state, 0);
  FLUSH_STATE(state, 0);
  return pos;
}

static int DoPutBlockAliasW2(const FSCEncoder* enc, const uint8_t* in, int size,
                             FSCType output[BLOCK_SIZE]) {
  int pos = BLOCK_SIZE;
  FSCStateW state0 = FSC_MAX, state1 = FSC_MAX;
  const FSCStateW norm = (FSC_MAX >> MAX_LOG_TAB_SIZE) << FSC_BITS;
  int k = size;
  assert(enc->log_tab_size_ == MAX_LOG_TAB_SIZE);
  if (k & 1) {
    const Symbol* const s1 = &enc->symbols_[in[--k]];
    FLUSH_STATE(state1, norm * s1->freq_);
    RENORMALIZE_STATE_ALIAS(state1, s1);
  }
  while (k > 0) {
    const Symbol* const s0 = &enc->symbols_[in[--k]];
    const Symbol* const s1 = &enc->symbols_[in[--k]];
    FLUSH_STATE(state0, norm * s0->freq_);
    FLUSH_STATE(state1, norm * s1->freq_);
    RENORMALIZE_STATE_ALIAS(state0, s0);
    RENORMALIZE_STATE_ALIAS(state1, s1);
  }
  FLUSH_STATE(state0, 0);
  FLUSH_STATE(state1, 0);
  FLUSH_STATE(state0, 0);
  FLUSH_STATE(state1, 0);
  return pos;
}

// -----------------------------------------------------------------------------

#define PUT_BLOCK_WRAPPER(FUNC_NAME, CALL)                                  \
static void FUNC_NAME(const FSCEncoder* enc, const uint8_t* in, int size,   \
                      FSCBitWriter* const bw) {                             \
  FSCType output[BLOCK_SIZE];                                               \
  assert(size <= BLOCK_SIZE);                                               \
  const int pos = CALL(enc, in, size, output);                              \
  assert(pos >= 0);                                                         \
  FSCAppend(bw, (const uint8_t*)&output[pos],                               \
            (BLOCK_SIZE - pos) * sizeof(output[0]));                        \
}

PUT_BLOCK_WRAPPER(PutBlockW1, DoPutBlockW1)
PUT_BLOCK_WRAPPER(PutBlockW2, DoPutBlockW2)
PUT_BLOCK_WRAPPER(PutBlockW4, DoPutBlockW4)
PUT_BLOCK_WRAPPER(PutBlockAliasW1, DoPutBlockAliasW1)
PUT_BLOCK_WRAPPER(PutBlockAliasW2, DoPutBlockAliasW2)

// -----------------------------------------------------------------------------
// Coding

static int SparseIsBetter(const uint32_t seq[], int len, int nb_bits) {
  uint32_t total = 1 << nb_bits;
  uint32_t half = total >> 1;
  int i;
  int saved_bits = -(len - 1);
  for (i = 0; i < len - 1; ++i) {
    const uint32_t c = seq[i];
    if (c == 0) saved_bits += nb_bits;
    total -= c;
    if (total < half) {
      --nb_bits;
      half >>= 1;
    }
  }
  return (saved_bits > 0);
}

static int WriteSequence(const uint32_t seq[], int len, int sparse, int nb_bits,
                         FSCBitWriter* const bw) {
  uint32_t total = 1 << nb_bits;
  uint32_t half = total >> 1;
  int i;
  int total_bits = 0;
  if (sparse == 2) {
    sparse = SparseIsBetter(seq, len, nb_bits);
    FSCWriteBits(bw, sparse, 1);
  }
  for (i = 0; i < len - 1; ++i) {
    const uint32_t c = seq[i];
    if (sparse) {
      FSCWriteBits(bw, c > 0, 1);
      total_bits += 1;
      if (c == 0) continue;
    }
    FSCWriteBits(bw, c, nb_bits);
    total_bits += nb_bits;
    total -= c;
    if (total < half) {
      --nb_bits;
      half >>= 1;
    }
  }
  if (total != seq[len - 1]) return -1;  // verify normalization
  return total_bits;
}

// Write the distribution table as header
static int WriteHeader(FSCEncoder* const enc, const uint32_t counts[MAX_SYMBOLS],
                       FSCBitWriter* bw) {
  const int max_symbol = enc->max_symbol_;
  const int log_tab_size = enc->log_tab_size_;
  uint32_t tab_size = 1u << log_tab_size;

  assert(enc->unique_symbol_ < 0);
  assert(max_symbol > 1);
  FSCWriteBits(bw, max_symbol - 1, 8);

  if (max_symbol < HDR_SYMBOL_LIMIT) {  // Method #1 for small alphabet
    if (WriteSequence(counts, max_symbol, 2, log_tab_size, bw) < 0) {
      return 0;
    }
  } else {  // Method #2 for large alphabet
    int ok = 0;
    int i;
    uint8_t bins[MAX_SYMBOLS];
    uint32_t* const bHisto =
        (uint32_t*)calloc(sizeof(*bHisto), log_tab_size + 1);
    uint16_t bits[MAX_SYMBOLS];
    if (bHisto == NULL) return 0;
    // Decompose into prefix and suffix
    {
      uint32_t total = tab_size;
      for (i = 0; i < max_symbol; ++i) {
        const int c = counts[i] + 1;
        int bin, b;
        for (bin = 0, b = c; b != 1; ++bin) { b >>= 1; }
        if (bin > log_tab_size) goto Error;
        bins[i] = bin;             // prefix
        bits[i] = c - (1 << bin);  // suffix
        ++bHisto[bin];             // record prefix distribution
        if (total < counts[i]) goto Error;
        total -= counts[i];
      }
      if (total != 0) goto Error;   // Unnormalized distribution!?
    }
    if (bHisto[0] == max_symbol - 1) {   // only one symbol?
      FSCWriteBits(bw, 32 - 1, 5);   // special marker for sparse case
    } else {  // Compress the prefix sequence using a sub-encoder
      FSCEncoder enc2;
      if (!EncoderInit(&enc2, bHisto, log_tab_size + 1,
                       TAB_HDR_BITS, CODING_METHOD_BUCKET)) {
        fprintf(stderr, "Sub-Encoder initialization failed!\n");
        goto Error;
      }
      const int hlen = enc2.max_symbol_;
      FSCWriteBits(bw, hlen - 1, 5);
      if (WriteSequence(bHisto, hlen, 2, TAB_HDR_BITS, bw) < 0) {
        fprintf(stderr, "Error during WriteSequence()!\n");
        goto Error;
      }
      enc2.methods_.put_block(&enc2, bins, max_symbol - 1, bw);
      // Write the suffix sequence
      for (i = 0; i < max_symbol - 1; ++i) {
        FSCWriteBits(bw, bits[i], bins[i]);
      }
    }
    ok = 1;
 Error:
    free(bHisto);
    if (!ok) return 0;
  }
 End:
  return !bw->error_;
}

static int WriteParams(FSCEncoder* const enc, const uint32_t counts[MAX_SYMBOLS],
                        FSCBitWriter* const bw) {
  FSCWriteBits(bw, LOG_TAB_SIZE - enc->log_tab_size_, 4);
  return WriteHeader(enc, counts, bw);
}

static int WriteParamsW(FSCEncoder* const enc, const uint32_t counts[MAX_SYMBOLS],
                        FSCBitWriter* const bw) {
  return WriteHeader(enc, counts, bw);
}

// -----------------------------------------------------------------------------

static int WriteParamsUnique(FSCEncoder* const enc, const uint32_t counts[MAX_SYMBOLS],
                             FSCBitWriter* const bw) {
  (void)counts;
  assert(enc->unique_symbol_ >= 0 && enc->unique_symbol_ < enc->max_symbol_);
  FSCWriteBits(bw, enc->unique_symbol_, 8);
  return !bw->error_;
}

static int BuildTablesUnique(FSCEncoder* const enc, const uint32_t counts[]) {
  (void)enc;
  (void)counts;
  return 1;
}

static void PutBlockUnique(const FSCEncoder* enc, const uint8_t* in, int size,
                           FSCBitWriter* const bw) {
  (void)enc;
  (void)in;
  (void)size;
  (void)bw;
}

// -----------------------------------------------------------------------------
// Simulation and comparison against ideal case

#ifdef SHOW_SIMULATION
static void SimulateCoding(const FSCEncoder* enc, const uint32_t counts[],
                           const uint8_t* message, size_t size, int tab_size) {
  const int max_symbol = enc->max_symbol_;
  int s, N;
  const transf_t* const transforms = enc->transforms_;
  const uint16_t* const states = enc->states_;
  int state = tab_size;
  double S0 = 0., S1 = 0.;    // theoretical entropy
  {
    const double norm = 1. / tab_size;
    for (s = 0; s < max_symbol; ++s) {
      if (counts[s] > 0) {
        const double p = norm * counts[s];
        S0 += -p * log(p);
      }
    }
    S0 /= 8. * log(2.);
  }

  {
    uint32_t real_counts[MAX_SYMBOLS];
    FSCCountSymbols(message, size, real_counts);
    const double real_norm = 1. / size;
    for (N = 0; N < size; ++N) {
      S1 += -log(real_norm * real_counts[message[N]]);
    }
    S1 /= size * 8. * log(2.);
  }

  size_t bits = 0;   // count overhead too?
  for (N = size - 1; N >= 0; --N) {
    const transf_t* const transf = &transforms[message[N]];
    const int nb_bits = transf->nb_bits_ + (state >= transf->wrap_);
    bits += nb_bits;
    state = states[(state >> nb_bits) + transf->offset_];
  }
  printf("ENTROPY:\n");
  printf("  Simulated: %.2lf%%    (imperfect coder,       real message)\n",
         100. * bits / (size * 8.));
  printf("  Real:      %.2lf%%    (perfect approx. coder, real message)\n",
         100. * S1);
  printf("  Theory:    %.2lf%%    (perfect approx. coder, perfect message)\n",
         100. * S0);
}
#endif

// -----------------------------------------------------------------------------
// Entry point

static const EncMethods kEncMethods[CODING_METHOD_LAST] = {
  { WriteParams, PutBlock, BuildTables, BuildSpreadTableBucket },
  { WriteParams, PutBlock, BuildTables, BuildSpreadTableReverse },
  { WriteParams, PutBlock, BuildTables, BuildSpreadTableModulo },
  { WriteParams, PutBlock, BuildTables, BuildSpreadTablePack },

  { WriteParamsW, PutBlockW1, BuildTablesW, NULL },
  { WriteParamsW, PutBlockW2, BuildTablesW, NULL },
  { WriteParamsW, PutBlockAliasW1, BuildTablesAliasW, NULL },
  { WriteParamsW, PutBlockAliasW2, BuildTablesAliasW, NULL },
  { WriteParamsW, PutBlockW4, BuildTablesW, NULL },

  { WriteParamsUnique, PutBlockUnique, BuildTablesUnique, NULL },
};

static int Encode(const uint8_t* in, size_t size,
                  uint32_t counts[MAX_SYMBOLS],
                  uint8_t** out, size_t* out_size, int log_tab_size,
                  FSCCodingMethod method) {
  int ok = 0;
  FSCEncoder enc;
  FSCBitWriter bw;

  if (!FSCBitWriterInit(&bw, size >> 8)) return 0;

  if (!EncoderInit(&enc, counts, 0, log_tab_size, method)) {
    fprintf(stderr, "Error during EncoderInit() call\n");
    goto end;
  }
  size_t val = size;
  while (val) {
    FSCWriteBits(&bw, 1, 1);
    FSCWriteBits(&bw, val & 0xff, 8);
    val >>= 8;
  }
  FSCWriteBits(&bw, 0, 1);

  FSCWriteBits(&bw, enc.method_, 4);
  if (!enc.methods_.write_params(&enc, counts, &bw)) {
    fprintf(stderr, "Error during WriteParams() call\n");
    goto end;
  }
#ifdef SHOW_SIMULATION
  SimulateCoding(&enc, counts, in, size, 1 << log_tab_size);
#endif

  FSCPutBlockFunc put_block = enc.methods_.put_block;
  while (size > 0) {
    const int next = (size > BLOCK_SIZE) ? BLOCK_SIZE : size;
    put_block(&enc, in, next, &bw);
    in += next;
    size -= next;
  }
  FSCBitWriterFlush(&bw);
  ok = !bw.error_;

 end:
  if (ok) {
    *out = FSCBitWriterFinish(&bw);
    *out_size = FSCBitWriterNumBytes(&bw);
  } else {
    FSCBitWriterDestroy(&bw);
  }
  return ok;
}

int FSCEncode(const uint8_t* in, size_t in_size,
              uint8_t** out, size_t* out_size, int log_tab_size,
              FSCCodingMethod method) {
  uint32_t counts[MAX_SYMBOLS];
  FSCCountSymbols(in, in_size, counts);
  return Encode(in, in_size, counts, out, out_size, log_tab_size, method);
}

// -----------------------------------------------------------------------------
