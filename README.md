## TurboBench:Compressor Benchmark
[![Build ubuntu](https://github.com/powturbo/TurboBench/actions/workflows/build.yml/badge.svg)](https://github.com/powturbo/TurboBench/actions/workflows/build.yml)

<p align="center">
  <img
    alt="TurboBench Logo" src="turbobench_/TurboBench.jpeg"
    width="400px"
  >
</p>

# TurboBench: The Ultimate Data Compression Benchmark

**100+ Codecs. The Powerhouse in a Single Executable** — *The Compression Lie Detector*

Tired of misleading benchmarks with I/O overhead, cache effects, and CPU throttling? **TurboBench** delivers **high-precision, zero-overhead results** you can actually trust.

## Why TurboBench Stands Out

- **The only benchmark** that includes **LzTurbo** alongside **100+** popular, latest, and fastest compressors (LZ77, Rolz, BWT, Context Mixing, Entropy Coders, and more).
- **100% in-memory** testing — no disk I/O to skew your results.
- Supports **multiple files**, recursive directories, and smart concatenation of small files into multiblock tests.
- Built to avoid common pitfalls like cache scenarios and throttling found in other tools.
- Unlock peak performance — TurboBench reduces **throttling** by intelligently controlling the **CPU frequency**
- Automatic sorting, updating, and merging of result files.
- A **nightly github [release](https://github.com/powturbo/TurboBench/releases) and [benchmarks](https://github.com/powturbo/TurboBench/blob/master/benchmark.md)** are created automatically whenever submodules change.
- Creates a separate result file (file@SET.tbb) for each compressor set 
- TurboBench rank aggregation crowns the top compression libraries with 🥇 🏆 👑 — see who's king!

## Broad Platform Support

**Linux** (amd/intel, ARM, RISC-V, PowerPC, s390x, loongson), **macOS** (Apple M1–M5), and **Windows** (MSYS2, MinGW, Visual C++).

## Rich, Ready-to-Use Outputs

- Text, HTML, CSV, Markdown.
- :sparkles: **Transfer speed sheets** for real-world scenarios: GPRS, 2G/3G/4G/5G, DSL, Network, HDD, SSD, RAM.
- :+1: **HTML plots**: Speedup + Speed/Ratio.
- :new:2026.07 :+1: **Automatic SVG Bar & Scatter Charts** for Compression, Decompression, and Ratio.
- Built-in **peak memory usage** reporting (compression + decompression) in HTML output.

**All-in-one executable** — no Python, no extra graphic libraries, zero dependencies, zero installers, zero configuration, no hassle. Just download, run, and benchmark.

**Perfect for developers, researchers, and performance enthusiasts** who demand accurate, apples-to-apples comparisons.

---
**TurboBench** — Precision you can rely on. The gold standard for compression benchmarking. 🚀

**Try it now:**
  - Build or Download turbobench from [releases](https://github.com/powturbo/TurboBench/releases)
  + List all plugins + version: "./turbobench -l2"<br />
  + List all compiled codecs: "./turbobench -l1"<br />
  + type "./turbobench -h" for help

### Benchmark:
- see : [Compression Benchmark](https://sites.google.com/site/powturbo/compression-benchmark)

### TurboBench vs LzBench

| Feature                                              | TurboBench | Lzbench |
|------------------------------------------------------|:----------:|:-------:|
|**Benchmark**|||
| Large number of codecs (isa-l/zlib-ng/blosc,...)| ✅         |  ⚠️  |
| Control throttling via CPU frequency                 | ✅         | ❌      |
| Automatic sorting, updating & merging of result files| ✅         | ❌      |
| In-place progress display (compression/decompression speed) | ✅    | ❌      |
| Separate results for each compressor set             | ✅         | ❌      |
|**Data Handling**|||
| Concatenation of small files into one multiblock file| ✅         | ❌      |
| Independent compression of many small files| ✅         | ❌      |
| Multiple files & recursive directories               | ✅         | ✅      |
| Support for very large files (multi-GB)| ✅         | ⚠️      |
| Dictionary support  | ✅         |  ❌        |
| Codec parameter support  | ✅         |  ❌        |
|**Metrics**|||
| Rank aggregation & score with medals                 | ✅         | ❌      |
| Pareto-front visualization          | ✅         | ❌      |
| Multiple output formats (HTML, Markdown, CSV, encode.su)| ✅         | ⚠️   |
| SVG bar & scatter images                             | ✅         | ❌      |
| Transfer-speed sheets (GPRS → 5G, HDD, SSD, RAM,...) | ✅         | ❌      |
| Peak memory/Stack usage (compress + decompress)      | ✅         | ❌      |
|**Codec & Configuration Coverage**|||
| Cross-platform support (x86, ARM, RISC-V, PowerPC…)   | ✅         | ⚠️      |
| Submodule architecture with automatic codec update         | ✅         | ❌      |
| Use codecs’ built-in tools, matching original builds & optimizations  | ✅         | ❌       |
|**Advanced**|||
| Incremental benchmarking                             | ✅         | ❌      |
| Simple integration of new codecs  | ✅         |  ❌        |
| Nightly GitHub release w. executables                | ✅         | ❌      |

##### - Groups FASTEST,FAST,EFFICIENT,MAX,OPTIMAL,BWT:
  + test all fast compressors in the lz4, lzturbo, zlib class<br />
    (additional groups can be defined in the "turbobench.ini" file)

        ./turbobench -eFAST file

  + Some codecs can accept additional parameters (see option -l1 or file plugins.cc)
    (colon ':' as separator)

        ./turbobench -elzma,9mt2:d29:a1:fb273:mf=bt4:mc999:lc8:lp0:pb2 file

##### - Codecs:
  + individual codec test (output to screen & file.tbb)<br />

        ./turbobench -elzturbo,19,29,39/brotli,6/zlib,6 file

  + retest or test other compressors and merge the results to file.tbb<br />

        ./turbobench -eFAST/bzip2 file

  + creates a separate result file (file@SET.tbb) for each compressor set with the 'a' option<br />

        ./turbobench -eTURBO -aTURBO file   
        ./turbobench -eFAST  -aFAST file   
        ./turbobench -eturborc,1/pivco,0,1  -aec file   

##### - Print + Plot

   + Print result file + "transfer+decompression speedup" plot to (file.html -p2) for browsing, (file.md -p4) Markdown format

        ./turbobench -p2 -S2 file.tbb
        ./turbobench -p4 file.tbb

##### - Bar/Scatter charts
   + SVG files automatically generated after each benchmark
   
### Compile:

  		git clone --depth=1 --recursive https://github.com/powturbo/TurboBench.git
        cd TurboBench
  		make

### Environment:

###### OS/Compiler (32 + 64 bits):
- Windows: [MinGW-w64](https://sourceforge.net/projects/mingw-w64/) makefile 
- Linux amd/intel/risc-v: GNU GCC (>=4.6)
- Linux amd/intel/risc-v: Clang (>=3.2)
- Linux arm: aarch64
- Linux risc-v: riscv64
- MaxOS: XCode (>=9)
- PowerPC ppc64le
- IBM Z series s390x


### Plugins:
#### Compressor Lz77,Rolz,zpaq:
| Compressor | Added | Language | Comment |
|---|---|---|---|
| [LzTurbo](https://sites.google.com/site/powturbo) |  |c  |  |
| [AMD AOCL](https://github.com/amd/aocl-compression) | 2026/09 |c++  |tuned lz4,zstd,bzip2 for amd64  |
| [BriefLz](https://github.com/jibsen/brieflz) |  |c  |  |
| [Brotli](https://github.com/google/brotli) |  |c  |  |
| [Chameleon](http://cbloomrants.blogspot.de/2015/03/03-25-15-my-chameleon.html) |  |c  |  |
| [CSC](https://github.com/fusiyuan2010/CSC) |  |c  |  |
| [Density](https://github.com/g1mv/density) | 2026/09 | rust |  |
| [Doboz](https://bitbucket.org/attila_afra) |  |c  |  |
| [FastLz](http://fastlz.org) |  |c  |  |
| [Fast-lzma](https://github.com/conor42/fast-lzma2) |  |c  |  |
| [glza](https://github.com/kidq330/GLZA) |  |c  |  |
| [Glyd](https://github.com/surya-koritala/Glyd) | 2026/09 | rust |  |
| [heatshrink](https://github.com/atomicobject/heatshrink) |  |c  |short strings|
| [Iguana](https://github.com/ClickHouse/iguana) | 2026/09 |c++  |Not reliable|
| [Intel(R) ISA-L](https://github.com/01org/isa-l) |  |c  |  |
| [kanzi](https://github.com/flanglet/kanzi-cpp) | 2026/07 |c++  |lz,bwt,paq|
| [lib](https://github.com/vurtun/lib) | 2026/09 |c  |  |
| [Libdeflate](https://github.com/ebiggers/libdeflate) |  |c  |  |
| [LibLZF](http://oldhome.schmorp.de/marc/liblzf.html) |  |c  |  |
| [LibLz](https://github.com/mbitsnbites/liblzg) |  |c  |  |
| [LibSLZ](https://github.com/wtarreau/libslz) |  |c  |  |
| [Lz4](https://github.com/Cyan4973/lz4) |  |c  |  |
| [Lz4ultra](https://github.com/emmanuel-marty/lz4ultra) |  |c  |  |
| [lzjody](https://github.com/deep-soft/lzjody) |  |c  |  |
| [lizard](https://github.com/inikep/lz5) |  |c  |  |
| [Lzfse](https://github.com/lzfse/lzfse) |  |c  |  |
| [Lzham v1.1](https://github.com/richgel999/lzham_codec_devel) |  |c++  |  |
| [Lzlib](https://github.com/aonez/lzlib) | 2026/09 |c  |  |
| [Lzmat](https://github.com/nemequ/lzmat) |  |c  |  |
| [Lzma](http://7-zip.org) |  |c  |  |
| [Lzo](http://www.oberhumer.com/opensource/lzo) |  |c  |  |
| [Lzoma](https://github.com/alef78/lzoma) |  |c  |  |
| [lzraven](https://github.com/anat0m1a/liblzraven) | 2026/09 |c  |Reimplementation of Apple's lzraven|
| [LZSSE](https://github.com/ConorStokes/LZSSE) |  |c  |  |
| [LZSA](https://github.com/emmanuel-marty/lzsa) |  |c  |  |
| [mbrotli](https://github.com/Mnwa/mbrotli) | 2026/09 | rust |  |
| [Memlz](https://github.com/rrrlasse/memlz) | 2026/07 |c  |  |
| [Miniz](https://github.com/richgel999/miniz) |  |c  |  |
| [misa77](https://github.com/welcome-to-the-sunny-side/misa77) | 2026/07 |c++ |  |
| [ms-compress](https://github.com/coderforlife/ms-compress) |  |c++  |  |
| [mzip](https://github.com/Cranot/mzip) |2026/09  |c  |too slow|
| [Oodle](http://www.radgametools.com/oodle.htm) |  |c++  |windows/linux/macos x86_64/arm64, dynamic lib|
| [Pithy](https://github.com/johnezang/pithy) |  |c  |  |
| [Quicklz](https://github.com/robottwo/quicklz) |  |c  |  |
| [sap](https://github.com/CoreSecurity/pysap) |  |c++|  |
| [shoco](https://github.com/Ed-von-Schleck/shoco) |  |c  |short strings|
| [Shrinker](https://code.google.com/p/data-shrinker) |  |c  |  |
| [Smallz4](https://github.com/stbrumme/smallz4) |  |c  |  |
| [smaz](https://github.com/antirez/smaz) |  |c  |short strings  |
| [Snappy](https://github.com/google/snappy) |  |c++ |  |
| [Snappy-c](https://github.com/andikleen/snappy-c) |  |c  |  |
| [Tamp](https://github.com/BrianPugh/tamp) | 2026/07 |c |  |
| [Tornado](http://freearc.org) |  |c++  |  |
| [Unishox](https://github.com/siara-cc/Unishox) |  |c  |short strings|
| [wfLZ](https://github.com/ShaneWF/wflz) |  |c  |  |
| [yalz77](https://github.com/ivan-tkatchev/yalz77) |  |c  |  |
| [yappy v2011]() |  |c  |  |
| [xpack](https://github.com/ebiggers/xpack) |  |  |  |
| [xz](https://github.com/tukaani-project/xz) | 2026/05 |c  |  |
| [zlib](http://zlib.net) |  |c  |  |
| [zlib-ng](https://github.com/Dead2/zlib-ng) |  |c  |  |
| [zlib cloudflare](https://github.com/cloudflare/zlib) |  |  |  |
| [zopfli](https://code.google.com/p/zopfli) |  |c  |  |
| [zstd](https://github.com/facebook/zstd) |  |c  |  |
| [zpaq](https://github.com/zpaq/zpaq) |  |c++  |  |
| [zxc](https://github.com/hellobertrand/zxc) | 2026/05 |c  |  |

#### BWT - Burrows–Wheeler transform:
| Compressor | Added | Language | Comment |
|---|---|---|---|
| [libbsc/bsc](https://github.com/IlyaGrebnov/libbsc) |  |c++ |  |
| [BwtSatan](https://github.com/powturbo/Turbo-Range-Coder) |2026/09   |c  |  |
| [bzip2](http://www.bzip.org/downloads.html) |  |c  |  |
| [bzip3](https://github.com/iczelia/bzip3) |  |c  |  |
| [kanzi bwt (level 5,6,7)](https://github.com/flanglet/kanzi-cpp) |2026/07  |c  |  | 
| [Pulsar / rust](https://github.com/ceedot-rock/pulsar-best) |2026/09  |c  |very slow  | 

#### Integer / Floating point:
| Compressor | Added | Language | Comment |
|---|---|---|---|
| [Blosc](https://github.com/Blosc/c-blosc2) |  |c  |  |
| [OpnZL](https://github.com/facebook/openzl) |2026/07  |c  |  |
| [Pcodec](https://github.com/pcodec/pcodec) |2026/07  |c  |  | 

#### Entropy coder:

| Compressor | Added | Language | Comment |
| ---------- | ----- | -------- | ------- |
| bitwise range coder |  |  |  |
| [TurboRC-Range Coder](https://github.com/powturbo/Turbo-Range-Coder) |  |c  |  |
| [Bitwise RC v2010](http://encode.su/threads/1153-Simple-binary-rangecoder-demo) |  |c  |  |
| [Bitwise vector RC v2012](http://encode.su/threads/1200-Vectorized-rangecoder) |  |c  |  |
| [AOMedia AV1 entropy coder](https://aomedia.googlesource.com/aom/+/master/aom_dsp/) |  |c  |  |
| [Daala entropy coder](https://github.com/xiph/daala) |  |c  |  |
| [FastAri v15-10](https://github.com/davidcatt/FastARI) |  |c  |  |
| bytewise range coder |  |  |  |
| [TurboAC](https://sites.google.com/site/powturbo) |  |c  |  |
| [subotin range coder v2000](http://ezcodesample.com/ralpha/Subbotin.txt) |  |c  |  |
| [Fast AC v2006](http://www.cipr.rpi.edu/research/SPIHT/) |  |c  |  |
| [FQZ/PPMD Range Coder v15-03](http://encode.su/threads/2149-ao0ec-Bytewise-adaptive-order-0-entropy-coder) |  |c  |  |
| [PPMD Range Coder v15-03](http://encode.su/threads/2149-ao0ec-Bytewise-adaptive-order-0-entropy-coder) |  |c  |  |
| [sserangecoding](https://github.com/richgel999/sserangecoding) |  |c  |  |
| ABS: Asymmetric binary systems |  |  |  |
| [Fpaqc:Asymmetric Binary Coder v07-12](http://www.mattmahoney.net/dc/) |  |c++  |  |
| ANS: Asymmetric Numeral Systems |  |  |  |
| [TurboANX-ANS](https://sites.google.com/site/powturbo) |  |c  |  |
| [TurborANS](https://sites.google.com/site/powturbo) |  |c  |  |
| [Finite State Coder v15-05](https://github.com/skal65535/fsc) |  |c  |  |
| [Finite State Entropy v16-08](https://github.com/Cyan4973/FiniteStateEntropy) |  |c  |  |
| [Oodle tans](http://www.radgametools.com/oodle.htm)  |  |c++  |windows/linux/macos|
| Huffman Coding |  |  |  |
| [TurboHF-Huffmann v1.3](https://sites.google.com/site/powturbo) |  |c  |  |
| [Tornado Huf v0.6a](http://freearc.org/Research.aspx) |  |c++  |  |
| [zlib Huffmann v1.2.8](https://github.com/Cyan4973/FiniteStateEntropy) |  |c  |  |
| [Fast HF v2006](http://www.cipr.rpi.edu/research/SPIHT/) |  |c  |  |
| [FPC](https://github.com/algorithm314/FPC) |  |c  |  |
| [FSE Huff](https://github.com/Cyan4973/FiniteStateEntropy) |  |c  |  |
| [Oodle huffman](http://www.radgametools.com/oodle.htm) |  |c++  |windows/linux/macos|
| [Polar Codes v10-07](http://www.ezcodesample.com/prefixer/prefixer_article.html) |c  |  |  |
| [Pivco-Huffman](https://github.com/MarcinZukowski/pivco-huffman)  |2026.07  |c++  |  |

###### memcpy
 - [inline memcpy](https://github.com/powturbo/TurboBench)
 - [library memcpy](https://github.com/powturbo/TurboBench)

#### Encoding:
 - [TurboRLE](https://github.com/powturbo/TurboRLE) Turbo Run Length Encoding
 - [tcobs](https://github.com/rokath/tcobs) short messages compression with COBS framing
 - [TurboBase64](https://github.com/powturbo/TurboBase64) Turbo Base64 Encoding/Decoding 
 - [fastbase64](https://github.com/lemire/fastbase64) Base64 Encoding 
 - [base64](https://github.com/aklomp/base64) Fast Base64 stream encoder/decoder 

#### Transform:
 - [bwt:libdivsufsort](https://github.com/y-256/libdivsufsort)
 - [st: bsc schindler transform](https://github.com/IlyaGrebnov/libbsc)

# TurboBench Manual

## Usage
`turbobench [options] [file(s)]`

### General Options
* `-eS`  : `S` = compressors/groups separated by `/`. Levels can be specified after `,`. (e.g., `-ezlib,1/FAST`)
* `-b#s` : `#` = blocksize {`filesize`}. max=1GB.
* `-d#`  : `#` = log2 dictionary size: 15-30 {`blocksize`}. (Only brotli, lzham, lzlib, lzma, zstd)
* `-B#s` : `#` = max benchmark filesize {`1GB`} (e.g., `-B4G`)
* `-s#s` : `#` = min buffer size to duplicate & test small files (e.g., `-s50`)
* `-r`   : process directories recursively
* `-a#`  : Add category # to tbb filename. file.tbb -> file@#.tbb

**Size Modifiers (`s`):**
* `K, M, G` = 1,000, 1,000,000, 1,000,000,000
* `k, m, h` = 1024, 1MB, 1GB (default is `m`, e.g., `64k` or `64K`)

### Benchmark Options
* `-iX,Y` : Decompression/Compression iterations {`3`}. e.g. -i15,15
* `-t#` : `#` = min time in seconds per iteration {`1`}
* `-S#` : Sleep `#` min after 2 min processing (minimizing CPU throttling)
* `-K#t` : Max time limit for all benchmarks {`24h`}. `t` = `M`:millisecond, `s`:second, `m`:minute, `h`:hour. (e.g., `3h`)
* `-D` : No process real-time priority setting

### Check Options
* `-C#` : `#=0` compress only, `#=1` No check, `#=2` ignore errors, `#=3` exit on error, `#=4` abort on error
* `-f#` : check reading/writing outside bounds: `#=1` compress, `#=2` decompress, `#=3` both

### Output & Plotting Options
* `-v#`   `#` = verbosity 0..3 {`1`}
* `-RX,Y` Show/Reveal only when compression/decompression speed > `X/Y` MB/s

* `-Kstr` Rank Aggregation: str = combination of R/C/D = Ratio/Comp./Decomp. {RCD}
* `-kstr` `str` = Remark/Comment string
* `-U     print memory/stack usage
* `-l#`   `#=1` print all groups/plugins, `#=2` print all codecs
* `-S#`   Plot transfer speed: 
  * `#=1` Comp speedup
  * `#=2` Decomp speedup
  * `#=3` Comp 'MB/s'
  * `#=4` Decomp 'MB/s' 
  * `#=4` Comp+Decomp speedup
  * `#=5` Comp+Decomp 'MB/s'
* `-p#` : `#` = print format (`1`=text, `2`=html, `3`=htm, `4`=markdown, `5/6`=vBulletin, `7`=csv, `8`=tsv)
* `-Q#` : `#` = Plot window size (`0`: 1920x1080, `1`: 1600x900, `2`: 1280x720, `3`: 800x600) {`1`}
* `-g` : `-g` = no merge w/ old result 'file.tbb', `-gg` = process w/o output (use for fuzzing)
* `-o` : Print on standard output
* `-G` : Plot memcpy
* `-w` : Plot Speedup linear x-axis {`log`}
* `-z` : Plot Ratio/Speed logarithmic x-axis {`linear`}

### Multiblock/Join
* `-Moutput` : Step 1: Concatenate all input files into a single output file organized into multiple blocks 
* `-m` : Step 2: Process each block independently from the merged file created in Step 1
* `-N` : Text files character delimiter (e.g., `-N9` for newline, 1 block/line)

### Predefined Groups: (see turbobench.ini file)

### Examples
```bash
./turbobench enwik9 -eFAST/bzip2/lzma,5,9
./turbobench enwik9 -eFAST/OPTIMAL/bsc,0:e2 -i0
./turbobench eECODER -k"entropy coder test"
./turbobench enwik9 -elzma,9:fb273:lc2:lp2:t2
./turbobench enwik9 -eFAST -aFAST
```

Last update: 28 SEP 2026

