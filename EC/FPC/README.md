# FPC - Fast Prefix Coder

FPC is a FAST entropy coder that uses prefix codes (huffman) and has higher compression ratio than many AC or ANS implementations for non-skewed probability distributions.

## Features
 * Advanced adaptive block subdivision
 * Optimal length limited prefix codes 
 using fast implementation of package - merge algorithm
 * Optimised for both in order and out of order cpus
 * Support for both big and little endian cpus
 * License ISC

## Configuration options
 * Number of streams (default 3)
 * Max bit length (default 11)
 * Adaptive step (default 2048).Lower increases compression ration but it is slower.

## Benchmark
Tested using TurboBench

### silesia.tar
#### Core i5-4460  CPU @ 3.20GHz
```
      C Size  ratio%     C MB/s     D MB/s   Name            File
   124509536    58.7      25.32      32.61   fpaq0p_sh                        silesia.tar
   128317932    60.5     162.82     813.92   fpc 0                            silesia.tar
   129091908    60.9      35.89      22.03   subotin                          silesia.tar
   129654526    61.2     476.96     821.30   fpc 16                           silesia.tar
   129979639    61.3     345.70     483.55   fse                              silesia.tar
   129989013    61.3     248.55     243.08   zlibh                            silesia.tar
   130059551    61.4     295.89     591.30   rans_static16                    silesia.tar
   130134503    61.4     574.79     715.36   fsehuf                           silesia.tar
   130170979    61.4     548.16     897.28   fpc 32                           silesia.tar
   130373605    61.5     234.33     229.23   fsc                              silesia.tar
   130731327    61.7     143.04      44.31   FastAC                           silesia.tar
   131948580    62.3     104.39      75.04   FastHF                           silesia.tar
   211957977   100.0    6030.96    6103.20   memcpy                           silesia.tar
```

### enwik8

#### orange pi pc plus allwinner h3
```
      C Size  ratio%     C MB/s     D MB/s   Name            File
    61251140    61.3       3.62       5.41   fpaq0p_sh                        enwik8
    62677385    62.7       5.14       3.19   subotin                          enwik8
    62782152    62.8      13.30      85.07   fpc 0                            enwik8
    63155812    63.2      61.85      86.17   fpc 16                           enwik8
    63188022    63.2      19.96       7.13   FastAC                           enwik8
    63193639    63.2      40.10      62.90   zlibh                            enwik8
    63202025    63.2      40.61      62.91   fse                              enwik8
    63287917    63.3      29.37      62.32   rans_static16                    enwik8
    63327425    63.3      20.31      29.96   fsc                              enwik8
    63415358    63.4      66.47      90.70   fpc 32                           enwik8
    63420890    63.4      64.20      61.11   fsehuf                           enwik8
    63648861    63.6      21.80      15.27   FastHF                           enwik8
   100000004   100.0     635.93    1099.78   memcpy                           enwik8
```

### compile
```
make
```
