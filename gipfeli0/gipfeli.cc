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

#include "gipfeli.h"

#include <algorithm>
#include <stdlib.h>
#include <string>

#include "entropy.h"
#include "lz77.h"

namespace gipfeli {

const char* HumanReadableStatus(Status err) {
  switch (err) {
    case kOk:
      return "No error occurred.";
    case kInputFail:
      return "Error: Problem with reading the input.";
    case kOutputFail:
      return "Error: Problem with writing the output.";
    case kCorrupted:
      return "Error: Input format is corrupted.";
    case kNotInitialised:
      return "Error: Initialization missing.";
    default:
      return "Error: Internal error occurred.";
  }
}

class Compressor::Impl {
 public:
  Impl() : lz77_(NULL), lz77_input_size_(0) {}

  ~Impl() {
    delete lz77_;
  }

  LZ77* ReallocateLZ77(int size) {
    if (lz77_ == NULL || lz77_input_size_ < size) {
      lz77_input_size_ = size;
      delete lz77_;
      lz77_ = new LZ77(size);
    } else {
      lz77_->ResetHashTable(size);
    }
    return lz77_;
  }

 private:
  LZ77* lz77_;
  size_t lz77_input_size_;
};

Status Compressor::Init() {
  delete impl_;
  impl_ = new Impl();
  return kOk;
}

Compressor::~Compressor() {
  delete impl_;
}

// Compresses data in "input".
// It first runs lz77 on input, passes it to entropy encoding
// and compressed result saves to "output". Returns an error code.
/*Status*/ int Compressor::Compress(const char* input,
                            size_t input_length,
                            char *output /*std::string* output*/) {
  if (impl_ == NULL) return kNotInitialised;
  uint32 content_size = 0;
  uint32 commands_size = 0;
  char* content = NULL;
  uint32* commands = NULL;
  //output->resize(MaxCompressedSize(input_length));
  //char* output_position = output->empty() ? NULL : &(*output)[0];
  char* output_position=output;
  // Store length of the input. First number of bytes of the length and then
  // the length.
  int bytes_used = 0;
  size_t len = input_length;
  output_position++;
  while (len > 0) {
    *output_position++ = len & 0xff;
    len >>= 8;
    bytes_used++;
  }
  // To store bytes_used, 3 bits are sufficient. We could
  // use the next 5 bits to store additional information.
  output[0] = bytes_used;

  size_t lz77_init_size = std::min<size_t>(kBlockSize, input_length);
  LZ77* lz77 = impl_->ReallocateLZ77(lz77_init_size);
  Entropy e;
  // split input to kBlock sized blocks
  for (size_t position = 0; position < input_length;) {
    const size_t length = std::min<size_t>(kBlockSize, input_length - position);
    lz77->CompressFragment(input + position, length, &content,
                           &content_size, &commands, &commands_size);
    output_position = e.Compress(reinterpret_cast<uint8*>(content),
                                 content_size, commands, commands_size,
                                 output_position);
    position += length;
  }
  /*if (!output->empty()) {
    output->resize(output_position - &(*output)[0]);
  }
  return kOk;*/
  return output_position - output;
}

// This method gives the upper bound for the compressed size of the input.
// The result is based on the following estimate.
// Maximum size for each unit of size kBlockSize:
// 10: Header Information
// 48: Conversion Mask
//  3: First Emit Literal Command.
// Emit Copies of length >= 6 (including necessary Emit Literal afterwards)
//   make always savings in the length.
// Emit Copies of length 4 or 5 followed by Emit Literals of length < 37
//   make always savings. If length >= 37 then the worst ratio is for
//   length 37 and it is overhead of 2 bits for every (4 * 8) + (37 * 8) bits.
// Every emitted literal can bring 2 bits overhead for each 8 bits.
size_t Compressor::MaxCompressedSize(size_t nbytes) {
  size_t times = nbytes / kBlockSize;
  size_t res = times * (10 + 48 + 3 + kBlockSize + (kBlockSize + 163) / 164 +
                        (kBlockSize / 4));
  nbytes -= times * kBlockSize;
  return res + 10 + 48 + 3 + nbytes + (nbytes + 163) / 164 + (nbytes / 4);
}

}  // namespace gipfeli
