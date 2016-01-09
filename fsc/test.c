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
// Test proggy for FSC compression
//
// Generate various PDF and compress / decompress them
//
// Author: Skal (pascal.massimino@gmail.com)

#include "./fsc_utils.h"

//------------------------------------------------------------------------------

static int GeneratePdf(int type, int param, uint32_t pdf[256],
                       FSCRandom* rg, int max_symbol) {
  int i;
  uint32_t total = 0;
  const double fparam = param / 16.0;
  for (i = 0; i < max_symbol; ++i) {
    const double x = (1. / 256.) * i;
    double v = 0;
    if (FSCRandomBits(rg, 8) < 254) {
      if (type == 0) v = 65536. / (1. + 512. * fparam * x);            // ~1/x
      else if (type == 1) v = 65536. / (1. + fparam * 1024. * x * x);  // ~1/x/x
      else if (type == 2) v = FSCRandomBits(rg, 8);                    // random
      else if (type == 3) {                                            // exp
        v = 1000 * exp(-x * 64. * fparam);
      } else if (type == 4) {
        v = 1000 * exp(-log(1. + 512. * x) * fparam);                  // power
      } else {
        v = 256;                                                       // uniform
      }
      uint32_t V = (uint32_t)v;
      V += FSCRandomBits(rg, 4);  // a little extra noise
      pdf[i] = V;
      total += V;
    } else {  // force few 0's from time to time
      pdf[i] = 0;
    }
  }
  return total;
}

static void PrintPdf(const uint8_t* base, int size) {
  uint32_t counts[MAX_SYMBOLS];
  FSCCountSymbols(base, size, counts);
  uint32_t max = 0;
  int max_symbol = 0;
  int i;
  for (i = 0; i < MAX_SYMBOLS; ++i) {
    if (counts[i] > max) max = counts[i];
    if (counts[i]) max_symbol = i + 1;
  }

  for (i = 0; i < max_symbol; ++i) {
    int len = 80 * counts[i] / max;
    printf("#%d ", i);
    while (len-- > 0) printf("-");
    printf("|\n");
  }
  for (i = max_symbol; i < 256; ++i) {
    if (counts[i]) {
      fprintf(stderr, "Error for symbol #%d (%d)!!\n", i, counts[i]);
      exit(-1);
    }
  }
}

static void SavePdfToFile(const uint8_t* base, int size, const char* pdf_file) {
  FILE* const file = fopen(pdf_file, "wb");
  if (file == NULL) {
    fprintf(stderr, "Error opening pdf output file [%s] !\n", pdf_file);
  } else {
    if (fwrite(base, size * sizeof(base[0]), 1, file) != 1) {
      fprintf(stderr, "Error while writing pdf output file\n");
    }
    fclose(file);
    fprintf(stderr, "Saved pdf into file [%s].\n", pdf_file);
  }
}

//------------------------------------------------------------------------------

static void Help() {
  printf("usage: ./test [options] [size]\n");
  printf("options:\n");
  printf("-t <int>           : distribution type (in [0..5])\n");
  printf("-p <int>           : distribution param (>=0)\n");
  printf("-s <int>           : number of symbols (in [2..256]))\n");
  printf("-l <int>           : max table size bits (<= LOG_TAB_SIZE)\n");
  printf("-save <string>     : save input message to file\n");
  printf("-d                 : print distribution\n");
  printf("-f <string>        : message file name\n");
  FSCPrintCodingOptions();
  printf("-h                 : this help\n");
  exit(0);
}

int main(int argc, const char* argv[]) {
  int N = 100000000;
  int pdf_type = 2;
  int pdf_param = 5;
  int max_symbol = MAX_SYMBOLS;
  int print_pdf = 0;
  int log_tab_size = LOG_TAB_SIZE;
  FSCCodingMethod method = CODING_METHOD_DEFAULT;
  const char* in_file = NULL;
  const char* pdf_file = NULL;
  int c;

  for (c = 1; c < argc; ++c) {
    if (!strcmp(argv[c], "-t") && c + 1 < argc) {
      pdf_type = atoi(argv[++c]);
      if (pdf_type < 0) pdf_type = 0;
      else if (pdf_type > 5) pdf_type = 5;
    } else if (!strcmp(argv[c], "-p") && c + 1 < argc) {
      pdf_param = atoi(argv[++c]);
      if (pdf_type < 0) pdf_param = 0;
    } else if (!strcmp(argv[c], "-s") && c + 1 < argc) {
      max_symbol = atoi(argv[++c]);
      if (max_symbol < 2) max_symbol = 2;
      else if (max_symbol > 256) max_symbol = 256;
    } else if (!strcmp(argv[c], "-l") && c + 1 < argc) {
      log_tab_size = atoi(argv[++c]);
      if (log_tab_size > LOG_TAB_SIZE) log_tab_size = LOG_TAB_SIZE;
    } else if (!strcmp(argv[c], "-f") && c + 1 < argc) {
      in_file = argv[++c];
    } else if (FSCParseCodingMethodOpt(argv[c], &method)) {
      continue;
    } else if (!strcmp(argv[c], "-m") && c + 1 < argc) {
      method = (FSCCodingMethod)atoi(argv[++c]);
    } else if (!strcmp(argv[c], "-save") && c + 1 < argc) {
      pdf_file = argv[++c];
    } else if (!strcmp(argv[c], "-d")) {
      print_pdf = 1;
    } else if (!strcmp(argv[c], "-h")) {
      Help();
    } else {
      N = atoi(argv[c]);
      if (N <= 2) N = 2;
    }
  }
  uint8_t* base;
  FILE* file = NULL;
  if (in_file != NULL) {
    file = fopen(in_file, "rb");
    if (file == NULL) {
      fprintf(stderr, "Error opening file %s!\n", in_file);
      exit(-1);
    }
    fseek(file, 0L, SEEK_END);
    N = ftell(file);
    fseek(file, 0L, SEEK_SET);
    printf("Read File [%s] (%d bytes)\n", in_file, N);
  }

  base = (uint8_t*)malloc(N * sizeof(*base));
  if (base == NULL) {
    fprintf(stderr, "Malloc(%d) failed!\n", N);
    exit(-1);
  }

  if (file != NULL) {
    N = fread(base, 1, N, file);
    fclose(file);
  }  else {
    uint32_t pdf[256] = { 0 };
    int i;
    FSCRandom r;
    FSCInitRandom(&r);
    const int total = GeneratePdf(pdf_type, pdf_param, pdf, &r, max_symbol);
    const int nb_bits = 1 + log2(total - 1);
    uint64_t cumul[256 + 1];
    cumul[0] = 0;
    for (i = 1; i <= max_symbol; ++i) {
      cumul[i] = cumul[i - 1] + pdf[i - 1];
    }
    for (i = 0; i < N; ++i) {
      base[i] = DrawSymbol(cumul, max_symbol, total, nb_bits, &r);
    }
  }
  printf("PDF generated OK (max symbol:%d).\n", max_symbol);
  if (print_pdf) {
    PrintPdf(base, N);
    printf("[Params: type=%d param=%d max_symbol=%d size=%d]\n",
           pdf_type, pdf_param, max_symbol, N);
  }
  if (pdf_file != NULL) {
    SavePdfToFile(base, N, pdf_file);
  }
  const double entropy = GetEntropy(base, N);

  int nb_errors = 0;
  uint8_t* out = NULL;
  size_t out_size = 0;
  // Encode
  uint8_t* bits = NULL;
  size_t bits_size = 0;
  MyClock start, tmp;
  GetElapsed(&start, NULL);
  int ok = FSCEncode(base, N, &bits, &bits_size, log_tab_size, method);
  double elapsed = GetElapsed(&tmp, &start);
  const double MS = 1.e-6 * N; // 8.e-6 * bits_size;
  const double reduction = 1. * bits_size / N;

  printf("Enc time: %.3f sec [%.2lf MS/s] (%ld bytes out, %d in).\n",
         elapsed, MS / elapsed, bits_size, N);
  printf("Entropy: %.4lf vs expected %.4lf "
         "(off by %.5lf bit/symbol [%.3lf%%])\n",
         reduction, entropy, reduction - entropy,
         100. * (reduction - entropy) / entropy);

  if (!ok) {
    fprintf(stderr, "ERROR while encoding!\n");
    nb_errors = 1;
  } else {   // Decode
    GetElapsed(&start, NULL);
    ok = FSCDecode(bits, bits_size, &out, &out_size);
    elapsed = GetElapsed(&tmp, &start);
    printf("Dec time: %.3f sec [%.2lf MS/s].\n", elapsed, MS / elapsed);
    ok &= (out_size == N);

    if (!ok) {
      fprintf(stderr, "Decoding error!\n");
      nb_errors = 1;
    } else {
      int i;
      for (i = 0; i < N; ++i) {
        nb_errors += (out[i] != base[i]);
      }
      printf("#%d errors\n", nb_errors);
      if (nb_errors) fprintf(stderr, "*** PROBLEM!! ***\n");
    }
  }

 End:
  free(base);
  free(out);
  free(bits);
  return (nb_errors != 0);
}
