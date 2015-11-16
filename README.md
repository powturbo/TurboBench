TurboBench: Compressor Benchmark
================================
+ **TurboBench**
 - 100% in memory benchmark, no I/O overhead
 - Include (>50) allmost all popular, latest or fastest compressors in one compiled package 
 - **Minimum** plugins call **overhead**
 - Set one, a group or several compressors to benchmark at the command line
 - **Multiple** input files with **recursive** directories
 - Concatenate **multiple small files** into one multiblock file
 - Benchmark **multiblock** file as one large block, but each block processed separatelly
 - Avoid **cache szenario** found in other benchmarks with small files
 - Set block size, file size limit,...
 - Set number of **iterations**, number of **runs**, **benchamrks**, set max. **time** per run
 - Automatic **sort** by compressed length
 - 64 bits **Linux** and **Windows** versions (gcc 5.2/mingw64 gcc5.2)
 - The only benchmark program including **LzTurbo**
 - Benchmarking **Entropy Coders**, **Lz77** and **BWT** compressors
 - **Text**, **html**, **csv**, **markdown** and other output formats
 - 100% C/C++, w/o inline assembly
 - Enable disable groups or individual codecs
 - No other compressor benchmark includes more codecs or offer more precision and features
 - Transfer speed sheet for different connections: GPRS,3G,4G,DSL,Network,HDD,SSD,RAM

### Download benchmark program:
 - [TurboBench Linux incl LzTurbo](https://sites.google.com/site/powturbo/downloads)
 - [TurboBench Windows incl. LzTurbo](https://sites.google.com/site/powturbo/downloads)

### Benchmark:
CPU: Sandy bridge i7-2600k at 4.2GHz, gcc 5.2, ubuntu 15.10, single thread.
- Realistic and practical benchmark with large files
- No PURE cache benchmark

##### - Data files:
 - Comming soon...

### Compile:
  *make*

### Testing:
##### - Groups:
  + test all fast compressor in the lz4, zlib class<br />


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

Last update: 16 NOV 2015

