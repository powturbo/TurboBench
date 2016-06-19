TurboBench: Compressor Benchmark[![Build Status](https://travis-ci.org/powturbo/TurboBench.svg?branch=master)](https://travis-ci.org/powturbo/TurboBench)
================================
+ **TurboBench**
 - The only benchmark program including **[LzTurbo](https://sites.google.com/site/powturbo/)**
 - 100% in-memory benchmark, no I/O overhead
 - Include (>60) allmost all popular, latest or fastest compressors in one compiled package
 - No other compressor benchmark includes more codecs or offer more precision and features
 - Benchmarking **Entropy Coders**, **Lz77**, **Rolz**, **BWT** and **Context mixing** compressors
 - **Minimum** plugins call **overhead**
 - Set one, a group or several compressors to benchmark at the command line
 - **Multiple** input files with **recursive** directories
 - Concatenate **multiple small files** into one multiblock file
 - Benchmark **multiblock** file as one large block, but each block processed separatelly
 - Avoid **cache szenario** found in other benchmarks with small files
 - Set block size, file size limit,...
 - Set number of **iterations**, number of **runs**, **benchmarks**, set max. **time** per run
   and for all benchmarks.
 - Automatic **sort**
 - :new: automatic update & merge of result files
 - :new: **Text**, **html**, **csv**, **markdown** and other output formats without retesting
 - :new: html output with sortable tables
 - :new: **Transfer speed sheet** for different connections or devices: GPRS,2G,3G,4G,DSL,Network,HDD,SSD,RAM
 - :+1: **Html plot:** **Speedup** + **Speed/Ratio**
 - **Linux** and **Windows** binaries
 - 100% C/C++, w/o inline assembly 
 - Enable/disable groups or individual codecs at compile time
 - All in one executable, no hassless installing of additional packages, graphic libraries, python,...
 - :+1: build in peak memory usage reporting for compression and decompression in html output
 - :new: **Encoding** and **Transform** plugins 

### Download benchmark executable incl. LzTurbo:
 - [TurboBench Linux](https://sites.google.com/site/powturbo/downloads)
 - [TurboBench Windows](https://sites.google.com/site/powturbo/downloads)

### Benchmark:
CPU: Sandy bridge i7-2600k at 4.2GHz, gcc 5.2, ubuntu 15.10, single thread.
- Realistic and practical benchmark with large files
- No PURE cache benchmark

##### - Data files:
#### TurboBench compressor benchmark:
- File [app3.tar binary Portable Apps Suite](http://compressionratings.com/download.html)

- [html output+Speedup](http://htmlpreview.github.io/?https://github.com/powturbo/TurboBench/master/turbobench_/app3.tar.html)
  (download app3.tar.html to local file for better browsing)

------------------------------------------------------------------------
![Seedup](turbobench_/app3.tar.png "Speedup: Transfer+decompression")
------------------------------------------------------------------------
![Speed/Ratio](turbobench_/app3r.tar.png "Speed/Ratio: Decompression")
------------------------------------------------------------------------

 (bold = pareto)  MB=1.000.000

|C Size|ratio%|C MB/s|D MB/s|Name|
|--------:|-----:|--------:|--------:|----------------|
|32798929| 32.8|**2.79**|**65.49**|**lzma 9**|
|32922377| 32.9|1.61|**69.65**|**lzturbo 49**|
|33761620| 33.7|2.64|**277.04**|**lzham 4**|
|34109576| 34.1|2.17|**1318.56**|**lzturbo 39**|
|35638896| 35.6|1.19|950.41|zstd 20|
|36944243| 36.9|**69.99**|**1411.77**|**lzturbo 32**|
|37313258| 37.3|2.40|**2149.57**|**lzturbo 29**|
|41668560| 41.6|0.19|246.43|brotli 11|
|45799999| 45.8|27.03|354.38|brotli 6|
|46304388| 46.3|35.75|347.99|brotli 5|
|46492542| 46.4|**191.69**|1186.68|**lzturbo 31**|
|46875269| 46.8|48.58|695.99|zstd 9|
|48836109| 48.8|113.95|676.74|zstd 5|
|49324183| 49.3|141.06|337.46|brotli 1|
|49860700| 49.8|16.94|295.99|zlib 9|
|49918739| 49.9|**288.52**|1082.09|**lzturbo 30a**|
|49962678| 49.9|35.70|294.24|zlib 6|
|50027825| 50.0|52.85|1899.72|lzturbo 22|
|50311200| 50.3|**312.35**|1090.91|**lzturbo 30**|
|50337788| 50.3|6.73|1428.58|lz5 9|
|52597358| 52.5|262.30|2068.57|lzturbo 21|
|52928477| 52.9|69.17|276.75|zlib 1|
|53112430| 53.1|298.70|442.42|zstd 1|
|54265487| 54.2|2.01|**3883.96**|**lzturbo 19**|
|54423519| 54.4|46.33|1918.40|lz4 9|
|55400947| 55.3|**465.29**|1900.38|**lzturbo 20a**|
|55764172| 55.7|405.79|1517.45|lz5 1|
|55923645| 55.9|141.96|3713.00|lzturbo 12|
|57606731| 57.6|263.89|3500.06|lzturbo 11|
|59090242| 59.0|**631.43**|2099.72|**lzturbo 20**|
|60016380| 60.0|596.92|3361.22|lzturbo 10a|
|61460109| 61.4|**705.32**|3500.41|**lzturbo 10**|
|61938605| 61.9|671.21|2069.81|lz4 1|
|100098564|100.0|**8647.84**|**8408.10**|**memcpy**|

### Plugins:
#### Compressor Lz77,Rolz,Bwt,zpaq:
 - [LzTurbo v1.3](https://sites.google.com/site/powturbo)
 - [balz v1.20](http://sourceforge.net/projects/balz) 
 - [bcm v1.1b](https://github.com/encode84/bcm) 
 - [Blosc v2.0/16-04](https://github.com/Blosc/c-blosc2)
 - [BriefLz v1.1.0](https://github.com/jibsen/brieflz) 
 - [Brotli v16-06](https://github.com/google/brotli) 
 - [Bzip2 v1.06](http://www.bzip.org/downloads.html) 
 - [Chameleon v15-03](http://cbloomrants.blogspot.de/2015/03/03-25-15-my-chameleon.html) 
 - [Crush v1.0.0](http://sourceforge.net/projects/crush) 
 - [CSC v16-01](https://github.com/fusiyuan2010/CSC) 
 - [Density v0.13.0](https://github.com/centaurean/density) 
 - [Doboz v14-01-14](https://bitbucket.org/attila_afra) 
 - [FastLz v0.1.0](http://fastlz.org) 
 - [Gipfeli v15.12](https://github.com/google/gipfeli) 
 - [heatshrink v0.4.1](https://github.com/atomicobject/heatshrink) 
 - [bsc v3.1.0](https://github.com/IlyaGrebnov/libbsc) 
 - [Libdeflate v16-06](https://github.com/ebiggers/libdeflate) 
 - [LibLZF v1.06](http://oldhome.schmorp.de/marc/liblzf.html) 
 - [LibLzg v1.0.8](https://github.com/mbitsnbites/liblzg) 
 - [Lz4 v1.7.1](https://github.com/Cyan4973/lz4) 
 - [Lz5 v1.4.0](https://github.com/inikep/lz5) 
 - [Lzfse v16-06](https://github.com/lzfse/lzfse)
 - [Lzham v1.1/15-12](https://github.com/richgel999/lzham_codec_devel) 
 - [Lzlib v1.8](http://www.nongnu.org/lzip) 
 - [Lzmat v1.0](https://github.com/nemequ/lzmat) 
 - [Lzma v9.35](http://7-zip.org) 
 - [Lzo v2.09](http://www.oberhumer.com/opensource/lzo) 
 - [Lzoma v16-01](https://github.com/alef78/lzoma) 
 - [LZSSE v16-03-28](https://github.com/ConorStokes/LZSSE)
 - [Miniz v13-10](https://github.com/richgel999/miniz) 
 - [ms-compress v16.06](https://github.com/coderforlife/ms-compress) 
 - [Nakamichi v15-10](http://www.overclock.net/t/1577282/fastest-open-source-decompressors-benchmark#post_24538188) 
 - [Pithy v2011](https://github.com/johnezang/pithy) 
 - [Quicklz v1.5.1](http://www.quicklz.com) 
 - [sap v16-04](https://github.com/CoreSecurity/pysap) 
 - [shoco v2015](https://github.com/Ed-von-Schleck/shoco) 
 - [Shrinker v0.1/r9](https://code.google.com/p/data-shrinker) 
 - [Snappy v1.1.3/16-05](https://github.com/google/snappy) 
 - [Snappy-c v1.1.2/14.04](https://github.com/andikleen/snappy-c) 
 - [Tornado v0.6a](http://freearc.org) 
 - [wfLZ v15-04](https://github.com/ShaneWF/wflz) 
 - [Yalz77 v15-09](https://github.com/ivan-tkatchev/yalz77) 
 - [Yappy v2011]() 
 - [zlib v1.2.8](http://zlib.net)
 - [Libzling v16-01](https://github.com/richox/libzling) 
 - [xpack v16-06](https://github.com/ebiggers/xpack) 
 - [Zopfli v16-05](https://code.google.com/p/zopfli) 
 - [ZSTD v0.7.0](https://github.com/Cyan4973/zstd) 
 - [zpaq v7.13](https://github.com/zpaq/zpaq) 

#### Entropy coder:
###### bitwise range coder
 - [TurboRC-Range Coder v1.3](https://sites.google.com/site/powturbo)
 - [Bitwise RC v2010](http://encode.ru/threads/1153-Simple-binary-rangecoder-demo)
 - [Bitwise vector RC v2012](http://encode.ru/threads/1200-Vectorized-rangecoder)
 - [bcm range coder v1.0](http://sourceforge.net/projects/bcm)
 - [FastAri v15-10](https://github.com/davidcatt/FastARI)

###### bytewise range coder
 - [TurboAC v1.3](https://sites.google.com/site/powturbo)
 - [subotin range coder v2000](http://ezcodesample.com/ralpha/Subbotin.txt)
 - [Fast AC v2006](http://www.cipr.rpi.edu/research/SPIHT/)
 - [Range Coder/J.Bonfield v15-07](ftp://ftp.sanger.ac.uk/pub/users/jkb)
 - [FQZ/PPMD Range Coder v15-03](http://encode.ru/threads/2149-ao0ec-Bytewise-adaptive-order-0-entropy-coder)
 - [PPMD Range Coder v15-03](http://encode.ru/threads/2149-ao0ec-Bytewise-adaptive-order-0-entropy-coder)

###### ABS: Asymmetric binary systems 
 - [Fpaqc:Asymmetric Binary Coder v07-12](http://www.mattmahoney.net/dc/)

###### ANS: Asymmetric Numeral Systems
 - [TurboANX-ANS v1.3](https://sites.google.com/site/powturbo)
 - [Finite State Coder v15-05](https://github.com/skal65535/fsc)
 - [Finite State Entropy v16-06](https://github.com/Cyan4973/FiniteStateEntropy)
 - [ANS/J.Bonfield v16-06](ftp://ftp.sanger.ac.uk/pub/users/jkb)
 - [Nania Adaptive rANS v2015](http://encode.ru/threads/2079-nARANS-(Nania-Adaptive-Range-Variant-of-ANS))

###### Huffman Coding
 - [TurboHF-Huffmann v1.3](https://sites.google.com/site/powturbo)
 - [Tornado Huf v0.6a](http://freearc.org/Research.aspx) 
 - [zlib Huffmann v1.2.8](https://github.com/Cyan4973/FiniteStateEntropy) 
 - [Fast HF v2006](http://www.cipr.rpi.edu/research/SPIHT/) 
 - [FSE Huff v16-06](https://github.com/Cyan4973/FiniteStateEntropy)
 - [Polar Codes v10-07](http://www.ezcodesample.com/prefixer/prefixer_article.html)
 - [inline memcpy](https://github.com/powturbo/TurboBench)
 - [library memcpy](https://github.com/powturbo/TurboBench)

#### Encoding:
 - [TurboRLE](https://github.com/powturbo/TurboRLE)

#### Transform:
 - [bwt:libdivsufsort](https://github.com/y-256/libdivsufsort)

### Testing:
  + List all plugins: "./turbobench -l2"<br />
  + List all compiled codecs: "./turbobench -l1"<br />
  + type "./turbobench -h" for help

##### - Groups FASTEST,FAST,EFFICIENT,MAX,OPTIMAL,BWT:
  + test all fast compressors in the lz4, lzturbo, zlib class<br />


        ./turbobench -eFAST file

##### - Codecs:

  + individual codec test (output to screen & file.tbb)<br />


        ./turbobench -elzturbo,19,29,39/brotli,6/zlib,6 file

  + retest or test other compressors and merge the results to file.tbb<br />


        ./turbobench -eFAST/bzip2 file

##### - Print + Plot

   + Print result file + "transfer+decompression speedup" plot to file.html for browsing


        ./turbobench -p2 -S2 file.tbb

   
### Compile:

  		git clone --recursive git://github.com/powturbo/TurboBench.git
        cd TurboBench
  		make

### Environment:
###### OS/Compiler (32 + 64 bits):
- Linux: GNU GCC (>=4.6)
- clang (>=3.2) 
- Windows: MinGW
- Windows: Visual Studio 2015

### References:
- [CompFuzz Results](https://github.com/nemequ/compfuzz/wiki/Results) - list of vulnerable codecs

Last update: 19 JUN 2016

