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
// Finite State Coder compress / decompress utility
//
// fsc [options] < in_file > out_file
//
// Example:
//    fsc < foo > foo.compressed
//    fsc -d < foo.compressed > bar
//    diff foo bar
//
// Basically it's just a wrapper around calls to FSCEncode() / FSCDecode()
//
// Author: Skal (pascal.massimino@gmail.com)

#include "./fsc_utils.h"

//------------------------------------------------------------------------------

static int ExperimentalSpread(int max_symbol, const uint32_t counts[],
                              int log_tab_size, uint8_t symbols[]) {
  return BuildSpreadTableBucket(max_symbol, counts, log_tab_size, symbols);
}

static void Help() {
  printf("usage: ./fsc [options] < in_file > out_file\n");
  printf("options:\n");
  printf("-c           : compression mode (default)\n");
  printf("-d           : decompression mode\n");
  printf("-s           : don't emit output, just print stats\n");
  printf("-l           : change log-table-size (in [2..14], default 12)\n");
  FSCPrintCodingOptions();
  printf("-h           : this help\n");
  exit(0);
}

int main(int argc, const char* argv[]) {
  int log_tab_size = 12;
  int compress = 1;
  FSCCodingMethod method = CODING_METHOD_DEFAULT;
  int stats_only = 0;
  int ok = 0;
  int c;

  for (c = 1; c < argc; ++c) {
    if (!strcmp(argv[c], "-l") && c + 1 < argc) {
      log_tab_size = atoi(argv[++c]);
      if (log_tab_size > LOG_TAB_SIZE) log_tab_size = LOG_TAB_SIZE;
      else if (log_tab_size < 2) log_tab_size = 2;
    } else if (FSCParseCodingMethodOpt(argv[c], &method)) {
      continue;
    } else if (!strcmp(argv[c], "-m") && c + 1 < argc) {
      method = (FSCCodingMethod)atoi(argv[++c]);
    } else if (!strcmp(argv[c], "-s")) {
      stats_only = 1;
    } else if (!strcmp(argv[c], "-c")) {
      compress = 1;
    } else if (!strcmp(argv[c], "-d")) {
      compress = 0;
    } else if (!strcmp(argv[c], "-h")) {
      Help();
    }
  }

  uint8_t* out = NULL;
  size_t out_size = 0;
  uint8_t* in = NULL;
  size_t in_size = 0;

  // Read input
  fseek(stdin, 0L, SEEK_END);
  in_size = ftell(stdin);
  fseek(stdin, 0L, SEEK_SET);
  if (in_size == (size_t)-1) {
    fprintf(stderr, "Missing/erroneous input!\n");
    goto End;
  }
  in = (uint8_t*)malloc(in_size * sizeof(*in));
  if (in == NULL) {
    fprintf(stderr, "Malloc(%lu) failed!\n", in_size);
    exit(-1);
  }
  ok = (fread(in, in_size, 1, stdin) == 1);
  if (!ok) {
    fprintf(stderr, "Error reading from stdin!\n");
    goto End;
  }

  // Compress or decompress.
  MyClock start, tmp;
  if (compress) {   // encoding
    GetElapsed(&start, NULL);
    ok = FSCEncode(in, in_size, &out, &out_size, log_tab_size, method);
    if (!ok) {
      fprintf(stderr, "ERROR while encoding!\n");
      goto End;
    }

    if (stats_only) {
      const double elapsed = GetElapsed(&tmp, &start);
      const double entropy = GetEntropy(in, in_size);
      const double MS = 1.e-6 * in_size;
      const double reduction = 1. * out_size / in_size;
      printf("Enc time: %.3f sec [%.2lf MS/s] (%ld bytes out, %ld in).\n",
             elapsed, MS / elapsed, out_size, in_size);
      printf("Entropy: %.4lf vs expected %.4lf "
             "(off by %.5lf bit/symbol [%.3lf%%])\n",
             reduction, entropy, reduction - entropy,
             100. * (reduction - entropy) / entropy);
    }
  } else {         // decoding
    GetElapsed(&start, NULL);
    ok = FSCDecode(in, in_size, &out, &out_size);
    if (!ok) {
      fprintf(stderr, "ERROR while decoding!\n");
      goto End;
    }
    if (stats_only) {
      const double elapsed = GetElapsed(&tmp, &start);
      const double MS = 1.e-6 * out_size;
      printf("Dec time: %.3f sec [%.2lf MS/s].\n", elapsed, MS / elapsed);
    }
  }

  if (!stats_only) {
    ok = (fwrite(out, out_size, 1, stdout) == 1);
    if (!ok) {
      fprintf(stderr, "Error writing to stdout!\n");
      goto End;
    }
  }

 End:
  free(in);
  free(out);
  return !ok;
}
