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
// Histograms / cumulative frequencies / spread functions
//
// Author: Skal (pascal.massimino@gmail.com)

#include "./fsc.h"

#include <stdio.h>
#include <math.h>
#include <assert.h>

//------------------------------------------------------------------------------

void FSCCountSymbols(const uint8_t* in, size_t in_size,
                     uint32_t counts[MAX_SYMBOLS]) {
  size_t n;
  memset(counts, 0, MAX_SYMBOLS * sizeof(counts[0]));
  for (n = 0; n < in_size; ++n) ++counts[in[n]];
}

//------------------------------------------------------------------------------
// Selection helper function

static void SwapU32(uint32_t* const A, uint32_t* const B) {
  const uint32_t tmp = *A;
  *A = *B;
  *B = tmp;
}
static void CheckSwapU32(uint32_t* const A, uint32_t* const B) {
  assert(A <= B);
  if (A != B && *A < *B) SwapU32(A, B);
}

// select the Mth largest keys amongst N
void Select(uint32_t* const keys, int M, int N) {
  if (M == N || N <= 1) return;  // done
  int low = 0, hi = N - 1;
  while (1) {
    if (low + 1 >= hi) {   // only 1 or 2 left
      if (low + 1 == hi) CheckSwapU32(keys + low, keys + low + 1);
      return;  // done!
    }
    const int mid = (low + hi) >> 1;
    // sort low | mid | hi triplet of entries
    CheckSwapU32(keys + low, keys + hi);
    CheckSwapU32(keys + mid, keys + hi);
    CheckSwapU32(keys + low, keys + mid);
    // move mid in position low + 1 (will serve as pivot)
    SwapU32(keys + low + 1, keys + mid);
    const uint32_t pivot = keys[low + 1];
    // and start loop over [low + 2, hi - 1] sub-range
    int i = low + 2;
    int j = hi - 1;
    while (1) {
      while (keys[i] > pivot) ++i;
      while (keys[j] < pivot) --j;
      if (j < i) break;  // they crossed the streams!
      SwapU32(keys + i, keys + j);
    }
    keys[low + 1] = keys[j];    // move pivot back to position
    keys[j] = pivot;
    // recurse down (only one branch)
    if (j >= M) {
      hi = j - 1;
    } else {
      low = j + 1;
    }
  }
}

//------------------------------------------------------------------------------
// Analyze counts[] and renormalize with Squeaky Wheel fix, so that
// the total is rescaled to be equal to tab_size exactly.
int FSCNormalizeCounts(uint32_t counts[MAX_SYMBOLS], int max_symbol,
                       int log_tab_size) {
  const int tab_size = 1 << log_tab_size;
  uint64_t total = 0;
  int nb_symbols = 0;
  int n;
  int last_nz = 0;

  for (n = 0; n < max_symbol; ++n) {
    total += counts[n];
    if (counts[n] > 0) {
      ++nb_symbols;
      last_nz = n + 1;
    }
  }
  if (nb_symbols < 1) return 0;   // won't work
  if (log_tab_size < 1) return 0;
  if (nb_symbols > tab_size) return 0;
  max_symbol = last_nz;

  uint32_t keys[MAX_SYMBOLS];
  int miss = tab_size;
  const float norm = 1.f * tab_size / total;
  int non_zero = 0;
  const float key_norm = (float)((1u << 24) / MAX_SYMBOLS);
  for (n = 0; n < max_symbol; ++n) {
    if (counts[n] > 0) {
      const float target = norm * counts[n];
      counts[n] = (uint32_t)(target + .5);  // round
      if (counts[n] == 0) counts[n] = 1;
      miss -= counts[n];
      const uint32_t error = (uint32_t)(key_norm * (target - counts[n]));
      keys[non_zero++] = (error * MAX_SYMBOLS) + n;
    }
  }
  if (miss == 0) return max_symbol;

  if (miss > 0) {
    Select(keys, miss, non_zero);
    for (n = 0; n < miss; ++n) {
      ++counts[keys[n] % MAX_SYMBOLS];
    }
  } else {
    // Overflow case. We need to decrease some counts, but need extra care
    // to not make any counts[] go to zero. So we just loop and shave off
    // the largest elements greater than 2 until we're good. It's garanteed
    // to terminate.
    non_zero = 0;
    const uint32_t cap_count = (1u << 23) - 1;  // to avoid overflow
    for (n = 0; n < max_symbol; ++n) {
      if (counts[n] > 1) {
        const uint32_t c = (counts[n] > cap_count) ? cap_count : counts[n];
        keys[non_zero++] = (c * MAX_SYMBOLS) + n;
      }
    }
    assert(non_zero > 0);
    miss = -miss;
    Select(keys, miss, non_zero);
    int to_fix = miss;
    while (to_fix > 0) {
      for (n = 0; n < miss && to_fix > 0; ++n) {
        const uint32_t idx = keys[n] % MAX_SYMBOLS;
        if (counts[idx] > 1) {
          --counts[idx];
          --to_fix;
        }
      }
    }
  }
  return max_symbol;
}

//------------------------------------------------------------------------------
// Spread functions

#define MAX_INSERT_ITERATION 0   // limit bucket-sort complexity (0=off)

// insert with limited bucket sort
#define INSERT(s, key) do {                      \
  const double k = (key);                        \
  const int b = (int)(k);                        \
  if (b < tab_size) {                            \
    const int S = (s);                           \
    int16_t* p = &buckets[b];                    \
    int M = MAX_INSERT_ITERATION;                \
    while (M-- && *p != -1 && keys[*p] < k) {    \
      p = &next[*p];                             \
    }                                            \
    next[S] = *p;                                \
    *p = S;                                      \
    keys[S] = k;                                 \
  }                                              \
} while (0)

int BuildSpreadTableBucket(int max_symbol, const uint32_t counts[],
                           int log_tab_size, uint8_t symbols[]) {
  const int tab_size = 1 << log_tab_size;
  int s, n, pos;
  int16_t* buckets = NULL;        // entry to linked list of bucket's symbol
  int16_t next[MAX_SYMBOLS];        // linked list of symbols in the same bucket
  double keys[MAX_SYMBOLS];           // key associated to each symbol
  buckets = (int16_t*)malloc(tab_size * sizeof(*buckets));
  if (buckets == NULL) return 0;

  for (n = 0; n < tab_size; ++n) {
    buckets[n] = -1;  // NIL
  }
  for (s = 0; s < max_symbol; ++s) {
    if (counts[s] > 0) {
      INSERT(s, 0.5 * tab_size / counts[s]);
    }
  }
  for (n = 0, pos = 0; n < tab_size && pos < tab_size; ++pos) {
    while (1) {
      const int s = buckets[pos];
      if (s < 0) break;
      symbols[n++] = s;
      buckets[pos] = next[s];   // POP s
      INSERT(s, keys[s] + 1. * tab_size / counts[s]);
    }
  }
  // n < tab_size can happen due to rounding errors
  for (; n != tab_size; ++n) symbols[n] = symbols[n - 1];
  free(buckets);
  return 1;
}

//------------------------------------------------------------------------------

static inline int ReverseBits(int i, int max_bits) {
  const int tab_size = 1 << max_bits;
  int v = 0, n = max_bits;
  while (n-- > 0) {
    v |= (i & 1) << n;
    i >>= 1;
  }
  return v;
}

int BuildSpreadTableReverse(int max_symbol, const uint32_t counts[],
                            int log_tab_size, uint8_t symbols[]) {
  const int tab_size = 1 << log_tab_size;
  int s, n, pos;
  for (s = 0, pos = 0; s < max_symbol; ++s) {
    for (n = 0; n < counts[s]; ++n, ++pos) {
      symbols[ReverseBits(pos, log_tab_size)] = s;
    }
  }
  return 1;
}

//------------------------------------------------------------------------------

int BuildSpreadTableModulo(int max_symbol, const uint32_t counts[],
                           int log_tab_size, uint8_t symbols[]) {
  const int tab_size = 1 << log_tab_size;
  const int kStep = ((tab_size >> 1) + (tab_size >> 3) + 1);
  int s, n, pos;
  for (s = 0, pos = 0; s < max_symbol; ++s) {
    for (n = 0; n < counts[s]; ++n, ++pos) {
      const int v = pos * kStep;
      const int slot = (v ^ CRYPTO_KEY) & (tab_size - 1);
      symbols[slot] = s;
    }
  }
  return 1;
}

//------------------------------------------------------------------------------

int BuildSpreadTablePack(int max_symbol, const uint32_t counts[],
                         int log_tab_size, uint8_t symbols[]) {
  const int tab_size = 1 << log_tab_size;
  const int kStep = ((tab_size >> 1) + (tab_size >> 3) + 1);
  int s, n, pos;
  for (s = 0, pos = 0; s < max_symbol; ++s) {
    for (n = 0; n < counts[s]; ++n, ++pos) {
      symbols[pos] = s;
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
