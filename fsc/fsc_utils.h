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

#ifndef FSC_UTILS_H_
#define FSC_UTILS_H_

#include "./fsc.h"
#include <sys/time.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

// Random generator
typedef struct {
  unsigned int seed_;
} FSCRandom;

void FSCInitRandom(FSCRandom* const rg);
int FSCRandomBits(FSCRandom* const rg, int num_bits);

// Timing
typedef struct timeval MyClock;
double GetElapsed(MyClock* new_clock, MyClock* old_clock);

// Helper functions
static double xlogx(double p) { return (p == 0.) ? 0 : -p * log2(p); }
double GetEntropy(const uint8_t* in, size_t size);
int DrawSymbol(const uint64_t cumul[256], int max_symbol,
               int total, int nb_bits, FSCRandom* rg);

// Option-parsing utils
int FSCParseCodingMethodOpt(const char opt[], FSCCodingMethod* const method);
void FSCPrintCodingOptions();

#endif  // FSC_UTILS_H_
