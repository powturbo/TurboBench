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
// Compare various encoding stategy (bit-by-bit vs byte-by-byte)

#include "./fsc_utils.h"

static void Generate(uint8_t* in, size_t size, int p, uint8_t* in8, int N8,
                     FSCRandom* rg) {
  uint8_t syms[255];
  int i, k;
  for (i = 0; i < 255; ++i) {
    syms[i] = (i < p);
  }
  for (i = 0; i < size; ++i) {
    int k;
    do {
      k = FSCRandomBits(rg, 8);
    } while (k >= 255);
    in[i] = syms[k];
  }
  // Pack 8 bits together
  memset(in8, 0, N8 * sizeof(*in8));
  for (k = 0; k < 8; ++k) {
    for (i = k; i < size; i += 8) {
      const int bit = in[i];
      if (k == 0) in8[i >> 3] = bit;
      else in8[i >> 3] |= bit << k;
    }
  }
}

static int CheckErrors(size_t N, const uint8_t* out, const uint8_t* base,
                       const char* name) {
  int nb_errors = 0;
  int i;
  for (i = 0; i < N; ++i) {
    nb_errors += (out[i] != base[i]);
  }
  if (nb_errors) {
    printf("%s Decoding errors! (%d)\n", name, nb_errors);
    for (i = 0; i < (N > 40 ? 40 : N); ++i) {
      printf("[%d/%d]%c", out[i], base[i], " *"[out[i] != base[i]]);
    }
    printf("\n");
    for (i = 0; i < N; ++i) {
      printf("%c", ".*"[out[i] != base[i]]);
      if ((i & 31) == 31) printf("\n");
    }
    printf("\n");
  }
  return nb_errors;
}

//------------------------------------------------------------------------------

void Help() {
  printf("usage: ./bit_test [options] [size]\n");
  printf("-l <int>           : max table size bits for bit-by-bit\n");
  printf("-l8 <int>          : max table size bits for byte-by-byte\n");
  printf("-p <int>           : try only one proba value\n");
  printf("-fsc               : skip FSC\n");
  printf("-fsc8              : skip FSC8\n");
  FSCPrintCodingOptions();
  printf("-h                 : this help\n");
  exit(0);
}

int main(int argc, const char* argv[]) {
  int N = 10000;
  int log_tab_size = 7;
  int log_tab_size_8 = LOG_TAB_SIZE - 1;
  int nb_errors = 0;
  int pmin = 0, pmax = 255;
  FSCCodingMethod method = CODING_METHOD_DEFAULT;
  int skip_FSC = 0;
  int skip_FSC8 = 0;
  int c;

  for (c = 1; c < argc; ++c) {
    if (!strcmp(argv[c], "-h")) {
      Help();
    } else if (FSCParseCodingMethodOpt(argv[c], &method)) {
      continue;
    } else if (!strcmp(argv[c], "-m") && c + 1 < argc) {
      method = (FSCCodingMethod)atoi(argv[++c]);
    } else if (!strcmp(argv[c], "-fsc")) {
      skip_FSC = 1;
    } else if (!strcmp(argv[c], "-fsc8")) {
      skip_FSC8 = 1;
    } else if (!strcmp(argv[c], "-l") && c + 1 < argc) {
      log_tab_size = atoi(argv[++c]);
    } else if (!strcmp(argv[c], "-p") && c + 1 < argc) {
      pmin = pmax = atoi(argv[++c]);
    } else if (!strcmp(argv[c], "-l8") && c + 1 < argc) {
      log_tab_size_8 = atoi(argv[++c]);
    } else {
      N = atoi(argv[c]);
      if (N <= 2) N = 2;
    }
  }
  const int N8 = (N + 7) >> 3;
  const double MS = 1.e-6 * N / 8.;

  uint8_t* const base = (uint8_t*)malloc((N8 + N) * sizeof(*base));
  if (base == NULL) return 0;
  uint8_t* const base8 = base + N;
  uint8_t* out = NULL;
  size_t out_size = 0;
  uint8_t* out8 = NULL;
  size_t out8_size = 0;

  FSCRandom r;
  FSCInitRandom(&r);

  int p;
  for (p = pmin; p <= pmax && nb_errors == 0; ++p) {
    const double P = p / 255.;
    MyClock start, tmp;
    double S_FSC = 0., S_FSC8 = 0.;
    double t_FSC_enc = 0., t_FSC_dec = 0.;
    double t_FSC8_enc = 0., t_FSC8_dec = 0.;
    int i;

    Generate(base, N, p, base8, N8, &r);
    const double S0 = xlogx(P) + xlogx(1. - P);
    const double S1 = GetEntropy(base, N);
    uint8_t* bits = NULL;
    size_t bits_size = 0;

    if (!skip_FSC) {
      GetElapsed(&start, NULL);
      nb_errors = !FSCEncode(base, N, &bits, &bits_size, log_tab_size, method);
      if (nb_errors) {
        printf("Encoding error!\n");
        goto end;
      }
      S_FSC = 8.0 * bits_size / N;
      t_FSC_enc = MS / GetElapsed(&tmp, &start);

      GetElapsed(&start, NULL);

      nb_errors = !FSCDecode(bits, bits_size, &out, &out_size);
      t_FSC_dec = MS / GetElapsed(&tmp, &start);
      nb_errors += (out_size != N);
      nb_errors += CheckErrors(out_size, out, base, "FSC");
      free(bits);
    }

    if (!skip_FSC8) {
      GetElapsed(&start, NULL);
      nb_errors =
          !FSCEncode(base8, N8, &bits, &bits_size, log_tab_size_8, method);
      if (nb_errors) {
        printf("FSC8 Encoding error!\n");
        goto end;
      }
      S_FSC8 = 8.0 * bits_size / N;
      t_FSC8_enc = MS / GetElapsed(&tmp, &start);

      GetElapsed(&start, NULL);
      nb_errors = !FSCDecode(bits, bits_size, &out8, &out8_size);
      t_FSC8_dec = MS / GetElapsed(&tmp, &start);
      nb_errors += (out8_size != N8);
      nb_errors += CheckErrors(N8, out8, base8, "FSC8");
      free(bits);
    }

    printf("%.7lf %.7lf %.7lf %.7lf %.7lf "
           "  %3.1lf     %3.1lf  "
           "  %3.1lf     %3.1lf\n",
           P, S_FSC, S_FSC8, S0, S1,
           t_FSC_enc,  t_FSC_dec,
           t_FSC8_enc, t_FSC8_dec);
  }
  printf("# 1 Proba|2  S_FSC |3 S_FSC8 |4 measure|5 theory "
         "|6 FSC enc|7 FSC dec"
         "|8 FSC8 c|9 FSC8 d\n");

 end:
  free(base);
  free(out);
  free(out8);
  return (nb_errors != 0);
}
