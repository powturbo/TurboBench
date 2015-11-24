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

#include "integral_types.h"

#ifndef GIPFELI_INTERNAL_ENUM_H_
#define GIPFELI_INTERNAL_ENUM_H_

namespace gipfeli {

static const uint32 LITERAL = 0;
static const uint32 COPY = 0x800000;

inline bool CommandIsCopy(uint32 val) {
  return val & COPY;
}

inline int CommandCopyLength(uint32 val) {
  return val >> 24;
}

inline int CommandCopyOffset(uint32 val) {
  return val & 0x1ffff;
}

inline int CommandLiteralLength(uint32 val) {
  return val;
}

inline uint32 CommandCopy(uint32 length, uint32 offset) {
  return COPY | (length << 24) | offset;
}

}  // namespace gipfeli

#endif  // GIPFELI_INTERNAL_ENUM_H_
