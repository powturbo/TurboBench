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

#ifndef GIPFELI_INTERNAL_ENTROPY_CODE_BUILDER_H_
#define GIPFELI_INTERNAL_ENTROPY_CODE_BUILDER_H_

#include "integral_types.h"

namespace gipfeli {

// Orders symbols according to sampled data.
// It assigns the most frequent 32 values 6-bit code (with prefix 0),
// then next 64 values 8-bit code (with prefix 10) and all other values have
// 10-bit code (with prefix 11). Returns for each symbol i, its new value
// in assign_value and its bit-length in assign_length.
class EntropyCodeBuilder {
 public:
  void FindLimits(uint8* symbols);

  void ProduceSymbolOrder(uint8* symbols,
                          int* assign_value,
                          int* assign_length);
  int limit_32_;
  int limit_96_;
  int choose_at_limit_32_;
  int choose_at_limit_96_;
};

}  // namespace gipfeli

#endif  // GIPFELI_INTERNAL_ENTROPY_CODE_BUILDER_H_
