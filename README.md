## TurboBench:Compressor Benchmark
[![Build ubuntu](https://github.com/powturbo/TurboBench/actions/workflows/build.yaml/badge.svg)](https://github.com/powturbo/TurboBench/actions/workflows/build.yaml)

 - High performance precision, zero overhead. One might call it the **compression lie detector**
 - The only benchmark program including **[LzTurbo](https://sites.google.com/site/powturbo/compression-benchmark)**
 - 100% in-memory benchmark, no I/O overhead
 - Include (>70) allmost all popular, latest or fastest compressors in one compiled package
 - Benchmarking **Entropy Coders**, **Lz77**, **Rolz**, **BWT** and **Context mixing** compressors
 - **Multiple** input files with **recursive** directories
 - Concatenate **multiple small files** into one multiblock file
 - Avoid **cache szenario** and **cpu throttling** found in other benchmarks
 - Automatic **sort**, update and merge of result files
 - **Text**, **html**, **csv**, **markdown** output
 - :sparkles: **Transfer speed sheet** for different connections or devices: GPRS,2G,3G,4G,DSL,Network,HDD,SSD,RAM
 - :+1: **Html plot:** **Speedup** + **Speed/Ratio**
 - All in one executable, no hassless installing of additional packages, graphic libraries, python,...
 - :+1: build in peak memory usage reporting for compression and decompression in html output

### Release:
  - Build or Download turbobench from [releases](https://github.com/powturbo/TurboBench/releases)
  + List all plugins: "./turbobench -l2"<br />
  + List all compiled codecs: "./turbobench -l1"<br />
  + type "./turbobench -h" for help

### Benchmark:
- see also: [Compression Benchmark](https://sites.google.com/site/powturbo/compression-benchmark)

CPU: Sandy bridge i7-2600k at 4.4GHz, gcc 6.3, single thread.
- Realistic and practical benchmark with large files
- No PURE cache benchmark
- All compressors with latest versions (2017-05-10)

- File [app3.tar binary Portable Apps Suite](https://)

 (bold = pareto)  MB=1.000.000

|C Size|ratio%|C MB/s|D MB/s|Name|
|--------:|-----:|--------:|--------:|----------------|
|32823983| 32.8|**3.40**|**67.92**|**lzma 9**|
|32872154| 32.8|0.31|**315.27**|**brotli 11d27**|
|32925079| 32.9|1.70|70.67|lzturbo 49|
|33936389| 33.9|2.57|**1701.35**|**lzturbo 39**|
|34105370| 34.1|3.32|952.59|zstd 22|
|36751363| 36.7|**48.30**|**1701.59**|**lzturbo 32**|
|36920708| 36.7|2.98|**2355.32**|**lzturbo 29**|
|46546059| 46.5|**163.77**|1489.57|**lzturbo 31**|
|46805879| 46.8|44.66|940.64|zstd 9|
|48152545| 48.1|52.94|349.62|brotli 4|
|49497505| 49.4| 2.48|2299.20|lizard 49
|49773790| 49.7|38.08|1952.73|lzturbo 22|
|49860700| 49.8|16.94|295.99|zlib 9|
|49962678| 49.9|35.70|294.24|zlib 6|
|50278958| 50.2|**282.43**|1372.91|**lzturbo 30**|
|52509931| 52.5|**290.96**|347.16|brotli 1|
|52549655| 52.5|239.35|2153.41|lzturbo 21|
|52928477| 52.9|69.17|276.75|zlib 1|
|52983490| 52.9|**393.67**|984.00|zstd 1|
|54251482| 54.2|2.60|**4367.15**|**lzturbo 19**|
|54410769| 54.4|46.37|3305.22|lz4 9|
|55923645| 55.9|188.40|4200.23|lzturbo 12|
|57606731| 57.6|386.90|3948.64|lzturbo 11|
|59085723| 59.0|**698.39**|2196.24|**lzturbo 20**|
|61455711| 61.4|**800.71**|4003.54|**lzturbo 10**|
|61938605| 61.9|730.46|3330.40|lz4 1|
|100098564|100.0|**8647.84**|**8408.10**|**memcpy**|

------------------------------------------------------------------------

Hardware: ODROID C2 - ARM 64 bits - 2Ghz CPU, OS: Ubuntu 16.04, gcc 5.3<br>
Compressors with versions 16.08.2016 <br>
[pd3d.tar](http://www.cbloom.com/pd3d.7z) - 3D Test Set (RAD Game Tools)


|C Size|ratio|C MB/s|D MB/s|Name|
|--------:|-----:|--------:|--------:|----------------|
|8052040| 25.2|**0.53**|**23.23**|**lzma 9**|
|9092280| 28.4|0.08|**52.61**|**brotli 11**|
|9159574| 28.7|0.52|**119.76**|**lzturbo 39**|
|9691094| 30.3|**0.68**|94.02|**zstd 22**|
|9826984| 30.7|**3.24**|**136.91**|**lzturbo 32**|
|10264073| 32.1|**26.15**|**142.28**|**lzturbo 30**|
|10427322| 32.6|4.90|108.76|zstd 9|
|10938385| 34.2|9.46|110.38|lzfse|
|10966870| 34.3|8.92|101.96|zstd 5|
|11059511| 34.6|1.74|98.16|zlib 9|
|11121480| 34.8|7.63|97.47|zlib 6|
|12649309| 39.6|0.61|**366.17**|**lzturbo 29**|
|13302907| 41.6|19.07|**435.28**|**lzturbo 21**|
|14237494| 44.5|0.66|**500.67**|**lzturbo 19**|
|14283317| 44.7|10.04|329.14|lz4 9|
|14723054| 46.1|**103.21**|483.81|**lzturbo 20**|
|14814049| 46.4|8.14|484.09|lzturbo 12|
|16069593| 50.3|**121.12**|365.08|**lz4 1**|
|16166867| 50.6|111.43|475.66|lzturbo 10|
|31952896|100.0|**1676.10**|**1704.00**|**memcpy**|

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

##### - Print + Plot

   + Print result file + "transfer+decompression speedup" plot to file.html for browsing

        ./turbobench -p2 -S2 file.tbb

   
### Compile:

  		git clone --depth=1 --recursive https://github.com/powturbo/TurboBench.git
        cd TurboBench
  		make

###### Turbobench mini: compile (only popular codecs)

		make NCODEC2=1

###### Turbobench incl. Entropy Coder codecs

        make EC=1

### Environment:

###### OS/Compiler (32 + 64 bits):
- Windows: [MinGW-w64](https://sourceforge.net/projects/mingw-w64/) makefile 
- Linux amd/intel: GNU GCC (>=4.6)
- Linux amd/intel: Clang (>=3.2)
- Linux arm: aarch64
- MaxOS: XCode (>=9)
- PowerPC ppc64le
- IBM Z series s390x


### Plugins:
#### Compressor Lz77,Rolz,Bwt,zpaq:
 - [LzTurbo](https://sites.google.com/site/powturbo)
 - [Blosc](https://github.com/Blosc/c-blosc2)
 - [BriefLz](https://github.com/jibsen/brieflz) 
 - [Brotli](https://github.com/google/brotli) 
 - [Bzip2](http://www.bzip.org/downloads.html) 
 - [Chameleon](http://cbloomrants.blogspot.de/2015/03/03-25-15-my-chameleon.html) 
 - [CSC](https://github.com/fusiyuan2010/CSC) 
 - [Density](https://github.com/centaurean/density) 
 - [Doboz](https://bitbucket.org/attila_afra) 
 - [FastLz](http://fastlz.org) 
 - [Fast-lzma](https://github.com/conor42/fast-lzma2) 
 - [glza](https://github.com/jrmuizel/GLZA)
 - [heatshrink](https://github.com/atomicobject/heatshrink) 
 - [Intel(R) Intelligent Storage Acceleration Library](https://github.com/01org/isa-l) 
 - [bsc](https://github.com/IlyaGrebnov/libbsc) 
 - [Libdeflate](https://github.com/ebiggers/libdeflate) 
 - [LibLZF](http://oldhome.schmorp.de/marc/liblzf.html) 
 - [LibLz](https://github.com/mbitsnbites/liblzg) 
 - [LibSLZ](http://1wt.eu/projects/libslz/)
 - [Lz4](https://github.com/Cyan4973/lz4) 
 - [Lz4ultra](https://github.com/emmanuel-marty/lz4ultra)
 - [lizard](https://github.com/inikep/lz5) 
 - [Lzfse](https://github.com/lzfse/lzfse)
 - [Lzham v1.1](https://github.com/richgel999/lzham_codec_devel) 
 - [Lzlib](http://www.nongnu.org/lzip) 
 - [Lzmat](https://github.com/nemequ/lzmat) 
 - [Lzma](http://7-zip.org) 
 - [Lzo](http://www.oberhumer.com/opensource/lzo) 
 - [Lzoma](https://github.com/alef78/lzoma) 
 - [LZSSE](https://github.com/ConorStokes/LZSSE)
 - [LZSA](https://github.com/emmanuel-marty/lzsa)
 - [Miniz](https://github.com/richgel999/miniz) 
 - [ms-compress](https://github.com/coderforlife/ms-compress) 
 - [Nakamichi Washigan](http://www.overclock.net/t/1577282/fastest-open-source-decompressors-benchmark#post_24538188)
 - [Oodle](http://www.radgametools.com/oodle.htm) (windows + linux)
 - [Quantile Compression](https://github.com/mwlon/quantile-compression)
 - [Pithy](https://github.com/johnezang/pithy) 
 - [Quicklz](http://www.quicklz.com) 
 - [sap](https://github.com/CoreSecurity/pysap) 
 - [shoco](https://github.com/Ed-von-Schleck/shoco) 
 - [Shrinker](https://code.google.com/p/data-shrinker) 
 - [Smallz4](https://github.com/stbrumme/smallz4) 
 - [Snappy](https://github.com/google/snappy) 
 - [Snappy-c](https://github.com/andikleen/snappy-c) 
 - [Tornado](http://freearc.org) 
 - [Unishox](https://github.com/siara-cc/Unishox) 
 - [wfLZ](https://github.com/ShaneWF/wflz) 
 - [yalz77](https://github.com/ivan-tkatchev/yalz77) 
 - [yappy v2011]() 
 - [zlib](http://zlib.net)
 - [zlib-ng](https://github.com/Dead2/zlib-ng)
 - [zlib cloudflare](https://github.com/cloudflare/zlib)
 - [xpack](https://github.com/ebiggers/xpack) 
 - [zopfli](https://code.google.com/p/zopfli) 
 - [zstd](https://github.com/facebook/zstd)
 - [zpaq](https://github.com/zpaq/zpaq) 

#### Entropy coder:

###### bitwise range coder
 - [TurboRC-Range Coder v1.3](https://sites.google.com/site/powturbo)
 - [Bitwise RC v2010](http://encode.ru/threads/1153-Simple-binary-rangecoder-demo)
 - [Bitwise vector RC v2012](http://encode.ru/threads/1200-Vectorized-rangecoder)
 - [FastAri v15-10](https://github.com/davidcatt/FastARI)
 - [AOMedia AV1 entropy coder](https://aomedia.googlesource.com/aom/+/master/aom_dsp/)
 - [Daala entropy coder](https://github.com/xiph/daala)
 - [Turbo Range Coder](https://github.com/powturbo/Turbo-Range-Coder) 

###### bytewise range coder
 - [TurboAC](https://sites.google.com/site/powturbo)
 - [subotin range coder v2000](http://ezcodesample.com/ralpha/Subbotin.txt)
 - [Fast AC v2006](http://www.cipr.rpi.edu/research/SPIHT/)
 - [Range Coder/J.Bonfield v15-07](https://github.com/jkbonfield/rans_static)
 - [FQZ/PPMD Range Coder v15-03](http://encode.ru/threads/2149-ao0ec-Bytewise-adaptive-order-0-entropy-coder)
 - [PPMD Range Coder v15-03](http://encode.ru/threads/2149-ao0ec-Bytewise-adaptive-order-0-entropy-coder)

###### ABS: Asymmetric binary systems 
 - [Fpaqc:Asymmetric Binary Coder v07-12](http://www.mattmahoney.net/dc/)

###### ANS: Asymmetric Numeral Systems
 - [TurboANX-ANS v1.3](https://sites.google.com/site/powturbo)
 - [Finite State Coder v15-05](https://github.com/skal65535/fsc)
 - [Finite State Entropy v16-08](https://github.com/Cyan4973/FiniteStateEntropy)
 - [rans_static v16-10](https://github.com/jkbonfield/rans_static)

###### Huffman Coding
 - [TurboHF-Huffmann v1.3](https://sites.google.com/site/powturbo)
 - [Tornado Huf v0.6a](http://freearc.org/Research.aspx) 
 - [zlib Huffmann v1.2.8](https://github.com/Cyan4973/FiniteStateEntropy) 
 - [Fast HF v2006](http://www.cipr.rpi.edu/research/SPIHT/) 
 - [FPC](https://github.com/algorithm314/FPC)
 - [FSE Huff](https://github.com/Cyan4973/FiniteStateEntropy)
 - [Polar Codes v10-07](http://www.ezcodesample.com/prefixer/prefixer_article.html)

###### memcpy
 - [inline memcpy](https://github.com/powturbo/TurboBench)
 - [library memcpy](https://github.com/powturbo/TurboBench)

#### Encoding:
 - [TurboRLE](https://github.com/powturbo/TurboRLE) Turbo Run Length Encoding
 - [TurboBase64](https://github.com/powturbo/TurboBase64) Turbo Base64 Encoding/Decoding 
 - [fastbase64](https://github.com/lemire/fastbase64) Base64 Encoding 
 - [base64](https://github.com/aklomp/base64) Fast Base64 stream encoder/decoder 

#### Transform:
 - [bwt:libdivsufsort](https://github.com/y-256/libdivsufsort)
 - [st: bsc schindler transform](https://github.com/IlyaGrebnov/libbsc)

Last update: 26 APR 2023

