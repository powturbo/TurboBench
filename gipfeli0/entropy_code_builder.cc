// Copyright 2011 Google Inc. All Rights Reserved.
// Authors: Rasto Lenhardt and Jyrki Alakuijala
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "entropy_code_builder.h"

#include <string.h>

#include "integral_types.h"

namespace gipfeli {

// Builds histogram and finds positions (limits), where to split symbols
// to 32 most often used, 64 less used and other even less used.
void EntropyCodeBuilder::FindLimits(uint8* symbols) {
  uint16 histogram[32];
  memset(&histogram[0], 0, sizeof(histogram));
  for (int i = 0; i < 256; i++) {
    histogram[symbols[i]]++;
  }
  limit_32_ = -1;
  limit_96_ = -1;
  choose_at_limit_32_ = 0;
  choose_at_limit_96_ = 0;
  int count = 0;
  for (int i = 31; i >= 0; i--) {
    count += histogram[i];
    if (count >= 32 && limit_32_ == -1) {
      limit_32_ = i;
      choose_at_limit_32_ = histogram[i] - (count - 32);
    }
    if (count >=96 && limit_96_ == -1) {
      limit_96_ = i;
      if (limit_32_ != limit_96_) {
        choose_at_limit_96_ = histogram[i] - (count - 96);
      } else {
        choose_at_limit_96_ = 64;
      }
    }
  }
}

// Builds symbol order and the entropy code using the limits computed
// by FindLimits method.
void EntropyCodeBuilder::ProduceSymbolOrder(uint8* symbols,
                                            int* assign_value,
                                            int* assign_length) {
  int best_index = 0;
  int next_best_index = 0;
  for (int i = 0; i < 256; i++) {
    if (symbols[i] >= limit_32_) {
      if (symbols[i] == limit_32_) {
        if (--choose_at_limit_32_ == 0) {
          limit_32_++;
        }
      }
      assign_value[i] = best_index;
      assign_length[i] = 6;
      ++best_index;
      continue;
    }
    if (symbols[i] >= limit_96_) {
      if (symbols[i] == limit_96_) {
        if (--choose_at_limit_96_ == 0) {
          limit_96_++;
        }
      }
      assign_value[i] = 0x80 | next_best_index;
      assign_length[i] = 8;
      ++next_best_index;
      continue;
    }
    assign_value[i] = 0x300 | i;
    assign_length[i] = 10;
  }
}

}  // namespace gipfeli
