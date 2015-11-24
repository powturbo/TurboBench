TurboBench: Compressor Benchmark
================================
+ **TurboBench**
 - The only benchmark program including **LzTurbo**
 - 100% in-memory benchmark, no I/O overhead
 - Include (>50) allmost all popular, latest or fastest compressors in one compiled package 
 - **Minimum** plugins call **overhead**
 - Set one, a group or several compressors to benchmark at the command line
 - **Multiple** input files with **recursive** directories
 - Concatenate **multiple small files** into one multiblock file
 - Benchmark **multiblock** file as one large block, but each block processed separatelly
 - Avoid **cache szenario** found in other benchmarks with small files
 - Set block size, file size limit,...
 - Set number of **iterations**, number of **runs**, **benchamrks**, set max. **time** per run
   and for all benchmarks. 
 - Automatic **sort** by compressed length
 - 64 bits **Linux** and **Windows** versions (gcc 5.2/mingw64 gcc5.2)
 - Benchmarking **Entropy Coders**, **Lz77** and **BWT** compressors
 - :new: **Text**, **html**, **csv**, **markdown** and other output formats without restart
 - 100% C/C++, w/o inline assembly
 - Enable disable groups or individual codecs
 - No other compressor benchmark includes more codecs or offer more precision and features
 - :new: html output with sortable tables
 - :new: **Transfer speed sheet** for different connections or devices: GPRS,2G,3G,4G,DSL,Network,HDD,SSD,RAM
 - :+1: **Speedup html Plot**
 - :new: automatic benchmark file update
 - **Without** installing any external package

### Download benchmark program incl. LzTurbo:
 - [TurboBench Linux](https://sites.google.com/site/powturbo/downloads)
 - [TurboBench Windows](https://sites.google.com/site/powturbo/downloads)

### Benchmark:
CPU: Sandy bridge i7-2600k at 4.2GHz, gcc 5.2, ubuntu 15.10, single thread.
- Realistic and practical benchmark with large files
- No PURE cache benchmark

##### - Data files:
#### TurboBench compressor benchmark: 
- File [app3.tar binary Portable Apps Suite](http://compressionratings.com/download.html)

- [html output+Speedup](http://htmlpreview.github.io/?https://cdn.rawgit.com/powturbo/TurboBench/master/app3.tar.html)

(bold = pareto)  MB=1.000.0000

|C Size|ratio%|C MB/s|D MB/s|Name|File|
|--------:|-----:|--------:|--------:|---------|---------|
|32798929| 32.8|**2.79**|**65.49**|**lzma 9**|app3.tar|
|32922377| 32.9|1.61|**69.65**|**lzturbo 49**|app3.tar|
|33761620| 33.7|2.55|**272.86**|**lzham 4**|app3.tar|
|34109576| 34.1|2.17|**1318.56**|**lzturbo 39**|app3.tar|
|35638896| 35.6|1.19|950.41|zstd 20|app3.tar|
|37025201| 37.0|**66.88**|**1389.48**|**lzturbo 32**|app3.tar|
|37313258| 37.3|2.40|**2149.57**|**lzturbo 29**|app3.tar|
|41668560| 41.6|0.19|246.43|brotli 11|app3.tar|
|45799999| 45.8|27.03|354.38|brotli 6|app3.tar|
|46304388| 46.3|35.75|347.99|brotli 5|app3.tar|
|46480016| 46.4|**184.29**|1192.20|**lzturbo 31**|app3.tar|
|46875269| 46.8|48.58|695.99|zstd 9|app3.tar|
|48836109| 48.8|113.95|676.74|zstd 5|app3.tar|
|49045431| 49.0|4.52|32.50|lzlib 9|app3.tar|
|49324183| 49.3|141.06|337.46|brotli 1|app3.tar|
|49915412| 49.9|**288.52**|1082.09|**lzturbo 30a**|app3.tar|
|49962678| 49.9|35.70|294.24|zlib 6|app3.tar|
|50027825| 50.0|52.85|1899.72|lzturbo 22|app3.tar|
|50311200| 50.3|**312.35**|1090.91|**lzturbo 30**|app3.tar|
|50337788| 50.3|6.64|1148.53|lz5 9|app3.tar|
|52597358| 52.5|262.30|2068.57|lzturbo 21|app3.tar|
|52928477| 52.9|69.17|276.75|zlib 1|app3.tar|
|53112430| 53.1|298.70|442.42|zstd 1|app3.tar|
|54265487| 54.2|2.00|**3766.50**|**lzturbo 19**|app3.tar|
|54423519| 54.4|46.33|1918.40|lz4 9|app3.tar|
|55400947| 55.3|**465.29**|1900.38|**lzturbo 20a**|app3.tar|
|55764172| 55.7|291.57|992.19|lz5 1|app3.tar|
|55923645| 55.9|141.96|3713.00|lzturbo 12|app3.tar|
|57606731| 57.6|263.89|3500.06|lzturbo 11|app3.tar|
|59090242| 59.0|**631.43**|2099.72|**lzturbo 20**|app3.tar|
|60016380| 60.0|596.92|3361.22|lzturbo 10a|app3.tar|
|61460109| 61.4|**705.32**|3500.41|**lzturbo 10**|app3.tar|
|61938605| 61.9|671.21|2069.81|lz4 1|app3.tar|
|100098564|100.0|**8632.55**|**8502.41**|**memcpy**|app3.tar|

### Plugins:
#### Compressor Lz77,Rolz,Bwt:
 - [LzTurbo v1.3](https://sites.google.com/site/powturbo)
 - [balz v1.20](http://sourceforge.net/projects/balz) 
 - [bcm v1.0](http://sourceforge.net/projects/bcm) 
 - [Blosc v1.7.1](https://github.com/Blosc/c-blosc2) 
 - [BriefLz v1.1.0](https://github.com/jibsen/brieflz) 
 - [Brotli v15-11](https://github.com/google/brotli) 
 - [Bzip2 v1.06](http://www.bzip.org/downloads.html) 
 - [Chameleon v15-03](http://cbloomrants.blogspot.de/2015/03/03-25-15-my-chameleon.html) 
 - [Crush v1.0.0](http://sourceforge.net/projects/crush) 
 - [CSC v15-06](https://github.com/fusiyuan2010/CSC) 
 - [Density v0.13.0](https://github.com/centaurean/density) 
 - [Doboz v14-01-14](https://bitbucket.org/attila_afra) 
 - [FastLz v0.1.0](http://fastlz.org) 
 - [Gipfeli v15.10](https://github.com/google/gipfeli) 
 - [heatshrink v0.4.1](https://github.com/atomicobject/heatshrink) 
 - [bsc v3.1.0](https://github.com/IlyaGrebnov/libbsc) 
 - [LibLZF v1.06](http://oldhome.schmorp.de/marc/liblzf.html) 
 - [LibLzg v1.0.8](https://github.com/mbitsnbites/liblzg) 
 - [Lz4 v1.7.1](https://github.com/Cyan4973/lz4) 
 - [Lz5 v1.7.2](https://github.com/inikep/lz5) 
 - [Lzfse](https://developer.apple.com/library/prerelease/mac/documentation/Performance/Reference/Compression/) *
 - [Lzham v1.1](https://github.com/richgel999/lzham_codec_devel) 
 - [Lzlib v1.7](http://www.nongnu.org/lzip) 
 - [Lzmat v1.0](https://github.com/nemequ/lzmat) 
 - [Lzma v9.35](http://7-zip.org) 
 - [Lzo v2.09](http://www.oberhumer.com/opensource/lzo) 
 - [ms-compress v15.09](https://github.com/coderforlife/ms-compress) 
 - [Pithy v2011](https://github.com/johnezang/pithy) 
 - [Quicklz v1.5.1](http://www.quicklz.com) 
 - [sap v15-11](https://github.com/CoreSecurity/pysap) 
 - [Shrinker v0.1/r9](https://code.google.com/p/data-shrinker) 
 - [Snappy v1.1.3](https://github.com/google/snappy) 
 - [Snappy-c v1.1.2/14.04](https://github.com/andikleen/snappy-c) 
 - [Tornado v0.6a](http://freearc.org) 
 - [wfLZ v15-04](https://github.com/ShaneWF/wflz) 
 - [Yalz77 v15-09](https://github.com/ivan-tkatchev/yalz77) 
 - [Yappy v.]() 
 - [zlib v1.2.8](http://zlib.net)
 - [Libzling v15-09](https://github.com/richox/libzling) 
 - [Zopfli v15-05](https://code.google.com/p/zopfli) 
 - [ZSTD v0.3.6](https://github.com/Cyan4973/zstd) 

* LzFSE: included but not tested

#### Entropy coder:
 - [TurboANX-ANS v1.3](https://sites.google.com/site/powturbo)
 - [TurboHF-Huffmann v1.3](https://sites.google.com/site/powturbo)
 - [TurboRC-Range Coder v1.3](https://sites.google.com/site/powturbo)
 - [bcm range coder v1.0](http://sourceforge.net/projects/bcm) 
 - [Finite State Coder v15-05](https://github.com/skal65535/fsc) 
 - [Finite State Entropy v15-11](https://github.com/Cyan4973/FiniteStateEntropy) 
 - [Fast HF v2006](http://www.cipr.rpi.edu/research/SPIHT/) 
 - [FastAri v15-10](https://github.com/davidcatt/FastARI) 
 - [Fast AC v2006](http://www.cipr.rpi.edu/research/SPIHT/) 
 - [Range Coder/J.Bonfield v15-07](ftp://ftp.sanger.ac.uk/pub/users/jkb) 
 - [ANS/J.Bonfield v15-08](ftp://ftp.sanger.ac.uk/pub/users/jkb) 
 - [Polar Codes v10-07](http://www.ezcodesample.com/prefixer/prefixer_article.html) 
 - [subotin v2000](http://ezcodesample.com/ralpha/Subbotin.txt) 
 - [Tornado Huf v0.6a](http://freearc.org/Research.aspx) 
 - [zlib Huffmann v1.2.8](https://github.com/Cyan4973/FiniteStateEntropy) 
 - [inline memcpy](https://github.com/powturbo/TurboBench)
 - [library memcpy](https://github.com/powturbo/TurboBench)

### Compile:

  		git clone --recursive git://github.com/powturbo/TurboBench.git 
        copy "snappy_/snappy-stubs-public.h" to "snappy"

  		make

### Testing:
  + List all compiled codecs: "./turbobench -l1"<br />
  + List all plugins: "./turbobench -l2"<br />
  + type "./turbobench -h" for help

##### - Groups FASTEST,FAST,EFFICIENT,OPTIMAL,BWT:
  + test all fast compressors in the lz4, lzturbo, zlib class<br />


        ./turbobench -eFAST inputfile

##### - Codecs:

  + individual codec test<br />


        ./turbobench -elzturbo,30,31,32,39/zlib,1,5,9 inputfile

        ./turbobench -eFAST/bzip2 inputfile

### Environment:
###### OS/Compiler (64 bits):
- Linux: GNU GCC (>=4.6)
- clang (>=3.2) 
- Windows: MinGW-w64

### References:

Last update: 24 NOV 2015

