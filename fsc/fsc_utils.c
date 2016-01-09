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
// Testing utilities for FSC
//
// Author: Skal (pascal.massimino@gmail.com)

#include "./fsc_utils.h"

//------------------------------------------------------------------------------
// Random

void FSCInitRandom(FSCRandom* const rg) {
  rg->seed_ = 0x81231f3u;
}

int FSCRandomBits(FSCRandom* const rg, int num_bits) {
  const int val = rand_r(&rg->seed_) >> 6;
  return val & ((1 << num_bits) - 1);
}

//------------------------------------------------------------------------------
// Timing

double GetElapsed(MyClock* new_clock, MyClock* old_clock) {
  gettimeofday(new_clock, NULL);
  if (old_clock != NULL) {
    const double elapsed =
        new_clock->tv_sec - old_clock->tv_sec
      + (new_clock->tv_usec - old_clock->tv_usec) / 1000000.0;
    *old_clock = *new_clock;
    return elapsed;
  } else {
    return 0.;
  }
}

//------------------------------------------------------------------------------
// Misc

double GetEntropy(const uint8_t* in, size_t size) {
  double S = 0.;
  uint32_t counts[MAX_SYMBOLS];
  FSCCountSymbols(in, size, counts);
  uint32_t total = 0;
  int i;
  for (i = 0; i < MAX_SYMBOLS; ++i) {
    total += counts[i];
  }
  for (i = 0; i < MAX_SYMBOLS; ++i) {
    if (counts[i]) {
      const double p = 1.f * counts[i] / total;
      S += -p * log(p);
    }
  }
  S /= 8. * log(2.);
  return S;
}

int DrawSymbol(const uint64_t cumul[256], int max_symbol,
               int total, int nb_bits, FSCRandom* rg) {
  int p;
  do {
    p = FSCRandomBits(rg, nb_bits);
  } while (p > total);

  int symbol_l = 0, symbol_u = max_symbol;
  while (symbol_l + 1 < symbol_u) {
    const int mid = (symbol_u + symbol_l) >> 1;
    const int c = cumul[mid];
    if (p == c) return mid;
    else if (p < c) symbol_u = mid;
    else symbol_l = mid;
  }
  return symbol_l;
}

//------------------------------------------------------------------------------

int FSCParseCodingMethodOpt(const char opt[], FSCCodingMethod* const method) {
  if (!strcmp(opt, "-buck")) {
    *method = CODING_METHOD_BUCKET;
  } else if (!strcmp(opt, "-rev")) {
    *method = CODING_METHOD_REVERSE;
  } else if (!strcmp(opt, "-mod")) {
    *method = CODING_METHOD_MODULO;
  } else if (!strcmp(opt, "-pack")) {
    *method = CODING_METHOD_PACK;
  } else if (!strcmp(opt, "-w")) {
    *method = CODING_METHOD_16B;
  } else if (!strcmp(opt, "-w2")) {
    *method = CODING_METHOD_16B_2X;
  } else if (!strcmp(opt, "-w4")) {
    *method = CODING_METHOD_16B_4X;
  } else if (!strcmp(opt, "-a")) {
    *method = CODING_METHOD_16B_ALIAS;
  } else if (!strcmp(opt, "-a2")) {
    *method = CODING_METHOD_16B_ALIAS_2X;
  } else {
    return 0;
  }
  return 1;
}

void FSCPrintCodingOptions() {
  printf("-w                 : use word-based coding.\n");
  printf("-w2                : use word-based coding 2x interleave.\n");
  printf("-w4                : use word-based coding 4x interleave.\n");
  printf("-a                 : use word-based coding + alias.\n");
  printf("-a2                : use word-based coding + alias + interleave.\n");
  printf("-mod               : use modulo spread function\n");
  printf("-rev               : use reverse spread function\n");
  printf("-pack              : use pack spread function\n");
  printf("-buck              : use bucket spread function\n");
}
