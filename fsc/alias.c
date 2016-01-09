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
// Tools for implementing Vose's alias sampling method.
//
// Author: Skal (pascal.massimino@gmail.com)

#include "./alias.h"

#include <stdio.h>
#include <assert.h>

int AliasInit(AliasTable t, const uint32_t counts[], int max_symbol) {
  // partition: small symbols at bottom, larges on top
  uint8_t symbols[ALIAS_MAX_SYMBOLS];
  int l = ALIAS_MAX_SYMBOLS, s = 0;
  int i;
  alias_tab_t proba[ALIAS_MAX_SYMBOLS];
  const uint32_t cut = MAX_TAB_SIZE >> LOG2_MAX_SYMBOLS;   // 1/n
  uint32_t total = 0;
  assert((MAX_TAB_SIZE % ALIAS_MAX_SYMBOLS) == 0);
  if (max_symbol > ALIAS_MAX_SYMBOLS || max_symbol <= 0) return 0;

  for (i = 0; i < ALIAS_MAX_SYMBOLS; ++i) {
    proba[i] = (i < max_symbol) ? counts[i] : 0;
    total += proba[i];
    if (proba[i] >= cut) {
      symbols[--l] = i;
    } else {
      symbols[s++] = i;
    }
    assert(s <= l);
  }
  assert(s == l);
  if (total != MAX_TAB_SIZE) return 0;   // unnormalized

  while (s > 0) {
    const int S = symbols[--s];
    const int L = symbols[l++];
    assert(proba[S] < cut);       // check that S is a small one
    t[S].cut_ = proba[S] + S * cut;
    t[S].other_ = L;
    proba[L] -= cut - proba[S];   // decrease large proba
    if (proba[L] >= cut) {
      --l;                // large symbol stays large. Reuse the slot.
    } else {
      symbols[s++] = L;   // large becomes small
    }
  }
  while (l < ALIAS_MAX_SYMBOLS) {
    const int L = symbols[l++];
    t[L].other_ = L;
    t[L].cut_ = cut + L * cut;  // large symbols with max proba
  }

  // Accumulate counts and compute the start_.
  uint32_t c[MAX_SYMBOLS] = { 0 };
  for (s = 0; s < MAX_SYMBOLS; ++s) {
    const int other = t[s].other_;
    const int count_s = t[s].cut_ - s * cut;
    const int count_other = cut - count_s;    // complement to 'cut'
    t[s].start_       = s * cut - c[s];
    t[s].other_start_ = s * cut + count_s - c[other];
    c[s]     += count_s;
    c[other] += count_other;
  }
  return AliasVerifyTable(t, counts, max_symbol);
}

//------------------------------------------------------------------------------

void AliasGenerateMap(const AliasTable t, alias_t map[MAX_TAB_SIZE]) {
  int r;
  for (r = 0; r < MAX_TAB_SIZE; ++r) {
    uint32_t dummy;
    map[r] = AliasSearchSymbol(t, r, &dummy);
  }
}

int AliasSpreadMap(int max_symbol, const uint32_t counts[],
                   int log_tab_size, uint8_t symbols[]) {
  AliasTable t;
  int i;
  assert(log_tab_size == MAX_LOG_TAB_SIZE);  // TODO(skal): support more sizes!
  if (!AliasInit(t, counts, max_symbol)) return 0;
  for (i = 0; i < (1 << log_tab_size); ++i) {
    uint32_t dummy;
    symbols[i] = AliasSearchSymbol(t, i, &dummy);
  }
  return 1;
}

int AliasBuildEncMap(const uint32_t counts[], int max_symbol,
                     uint16_t map[MAX_TAB_SIZE]) {
  AliasTable t;
  uint32_t r;
  uint32_t starts[MAX_SYMBOLS];
  uint32_t start = 0;
  if (!AliasInit(t, counts, max_symbol)) return 0;
  
  for (r = 0; r < max_symbol; ++r) {
    starts[r] = start;
    start += counts[r];
  }
  if (start != MAX_TAB_SIZE) return 0;

  for (r = 0; r < MAX_TAB_SIZE; ++r) {
    uint32_t rank;
    const uint32_t s = AliasSearchSymbol(t, r, &rank);
    map[rank + starts[s]] = r;
  }
  return 1;
}

//------------------------------------------------------------------------------

int AliasVerifyTable(const AliasTable t,
                     const uint32_t counts[], int max_symbol) {
  int error = 0;
#ifdef DEBUG_ALIAS
  int i, s;
  uint32_t c[MAX_SYMBOLS] = { 0 };
  const float norm = 100.f / MAX_TAB_SIZE;
  {
    uint32_t r;
    alias_t map[MAX_TAB_SIZE];
    AliasGenerateMap(t, map);
    for (r = 0; r < MAX_TAB_SIZE; ++r) ++c[map[r]];
  }
  for (s = 0; s < max_symbol; ++s) {
    error += abs(c[s] - counts[s]);
  }
  for (; s < MAX_SYMBOLS; ++s) {
    error += (c[s] != 0);
  }

  memset(c, 0, sizeof(c));
  for (i = 0; i < MAX_TAB_SIZE; ++i) {
    uint32_t rank;
    const int s = AliasSearchSymbol(t, i, &rank);
    const int count = c[s]++;
    if (rank != count) {
      const int r = i >> (MAX_LOG_TAB_SIZE - LOG2_MAX_SYMBOLS);
      const int use_alias = (i >= t[r].cut_);
      printf("%c s=%d%c %d / %d   r=%d  bucket=%d offset=%d | %d\n",
             " !"[rank != count], s, " *"[use_alias], rank, count, i, r,
             t[r].start_, t[r].other_start_);
      error += (rank != count);
    }
  }
  // printf("Error: %d\n", error);
#else
  (void)t;
  (void)counts;
  (void)max_symbol;
#endif
  return (error == 0);
}

//------------------------------------------------------------------------------
