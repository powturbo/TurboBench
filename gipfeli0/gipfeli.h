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
//
// util/compression/gipfeli is a fast compression library for the
// wide gipfeli compression format, designed by Jyrki Alakuijala,
// and implemented by Rastislav Lenhardt as an intern project.
// The goal of this project is to eventually replace zippy in its all
// current uses: most importantly, RPCs, temporary files in mapreduce,
// but also as bmdiff-gipfeli storage for bigtable and mapreduce output use.
//
// Here is a simple example for compressing data:
//
// u_c_g::Compressor compressor;
// u_c_g::Status status = compressor.Init();
// CHECK(status == u_c_g::kOk) << u_c_g::HumanReadableStatus(status);
// string compressed_abc;
// status = compressor.Compress("abc", 3, &compressed_abc);
// CHECK(status == u_c_g::kOk) << u_c_g::HumanReadableStatus(status);

#ifndef GIPFELI_PUBLIC_GIPFELI_H_
#define GIPFELI_PUBLIC_GIPFELI_H_

#include <stdlib.h>  // for size_t
#include <string>

namespace gipfeli {

// Error codes from Gipfeli functions.
enum Status {
  // Non-error returns >= 0
  kOk         = 0,

  // Error returns < 0
  kInputFail  = -1,
  kOutputFail = -2,
  kCorrupted  = -3,
  kInternal   = -4,
  kNotInitialised = -5,
};

const char *HumanReadableStatus(Status err);

// A compressor for the gipfeli format.
// All functions return an int status code.
class Compressor {
 public:
  Compressor() : impl_(NULL) {}
  ~Compressor();

  // Initialize a new compressor.
  Status Init();

  // Compress data until no more can be read. Returns an error code.
  int /*Status*/ Compress(const char* input, size_t input_length, char *output /* std::string* output*/);

  // Compress may expand the data in some situtions,
  // for example when the input is uncompressible or very small.
  // This helper returns the maximum size output for a single call to
  // Compress with nbytes worth of input. The returned value is a
  // reasonable upper bound, roughly equal to the nbytes + (nbytes >> 8) + 256.
  static size_t MaxCompressedSize(size_t nbytes);

 private:
  class Impl;
  Impl* impl_;
};

class Uncompressor {
 public:
  Uncompressor() : impl_(NULL) {}
  ~Uncompressor();

  // Initialize a new uncompressor.
  Status Init();

  // Uncompress the data, returns a status.
  Status Uncompress(const char* compressed, size_t compressed_length,
                    /*std::string*/ char *uncompressed, int uncompressed_size);

  // Stores the size of uncompressed string into "uncompressed_length".
  // Returns an error code.
  static Status GetUncompressedLength(const char* compressed,
                                      size_t compressed_length,
                                      size_t* uncompressed_length);
 private:
  class Impl;
  Impl* impl_;
};

}  // namespace gipfeli

#endif  // GIPFELI_PUBLIC_GIPFELI_H_
