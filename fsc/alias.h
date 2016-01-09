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
// http://web.eecs.utk.edu/~vose/Publications/random.pdf
// http://en.wikipedia.org/wiki/Alias_method
//
// Author: Skal (pascal.massimino@gmail.com)

#ifndef ALIAS_H_
#define ALIAS_H_

#include "./fsc.h"

#define LOG2_MAX_SYMBOLS 8   // such that (1 << LOG2_MAX_SYMBOLS) >= MAX_SYMBOLS
#define ALIAS_MAX_SYMBOLS (1 << LOG2_MAX_SYMBOLS)
// #define DEBUG_ALIAS

typedef uint8_t alias_t;        // enough to encode MAX_SYMBOLS
typedef uint32_t alias_tab_t;   // enough to store MAX_TAB_SIZE index

typedef struct {
  alias_tab_t cut_;
  alias_t other_;    // other symbol if residual >= cut_
  int32_t start_;
  int32_t other_start_;
} AliasPair;

typedef AliasPair AliasTable[ALIAS_MAX_SYMBOLS];

static inline alias_t AliasSearchSymbol(const AliasTable t, uint32_t r,
                                        uint32_t* const rank) {
  const int s = r >> (MAX_LOG_TAB_SIZE - LOG2_MAX_SYMBOLS);
  const int use_alias = (r >= t[s].cut_);
  *rank = r - (use_alias ? t[s].other_start_ : t[s].start_);
  return use_alias ? t[s].other_ : (alias_t)s;
}

int AliasInit(AliasTable t, const uint32_t counts[], int max_symbol);
void AliasGenerateMap(const AliasTable t, alias_t map[MAX_TAB_SIZE]);

int AliasVerifyTable(const AliasTable t,
                     const uint32_t counts[], int max_symbol);   // debug

// encoding:
int AliasBuildEncMap(const uint32_t counts[], int max_symbol,
                     uint16_t map[MAX_TAB_SIZE]);

// Spread function for alias look-up.
int AliasSpreadMap(int max_symbol, const uint32_t counts[],
                   int log_tab_size, uint8_t symbols[]);

#endif   // ALIAS_H_
