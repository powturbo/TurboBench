TurboRLE: Turbo Run Length Encoding [![Build Status](https://travis-ci.org/powturbo/TurboRLE.svg?branch=master)](https://travis-ci.org/powturbo/TurboRLE)
===================================

##### Efficient and fastest **Run Length Encoding** library
  - 100% C (C++ compatible headers), without inline assembly
  - Most efficient compression 
  - No other RLE compress or decompress faster with better compression
  - :sparkles: faster compression and 2x faster decompression with :+1: SIMD
  - :+1: **Java** Critical Natives Interface. Access TurboRLE **incl. SIMD!** from Java
  - Compress better and up to 8 times faster and decompress up to 4 times faster than other fast RLEs
  - Can be faster than memcpy
  - :+1: **ZERO!** byte overhead

  - No modification of the raw data, preserving compressibility
  - Order preserving 

##### TurboRLE
  - **TRLE**: TurboRLE - Efficient and fast Run Length Encoding
  - **SRLE**: TurboRLE Escape - Fast Run Length Encoding with automatic escape determination 

## Benchmark:
CPU: Sandy bridge i7-2600k at 4.2GHz, gcc 6.2, ubuntu 16.10
- with [TurboBench](https://github.com/powturbo/TurboBench)
- Single thread
- Realistic and practical benchmark with large files

###### Functions benchmarked
  - **MRLE**: Mespotine RLE [MRLE](http://encode.ru/threads/2121-No-more-encoding-overhead-in-Run-Length-Encoding-Read-about-Mespotine-RLE-here-)
  - **RLE64**: Run Length Encoding - [RLE64](http://sourceforge.net/projects/nikkhokkho/files/RLE64/)
<p>

------------------------------------------------------------------------
- BMP File: [girl.bmp in RLE64Samples](http://sourceforge.net/projects/nikkhokkho/files/RLE64/3.00/)

|C Size|ratio%|C MB/s|D MB/s|Name|
|--------:|-----:|--------:|--------:|----------------|
|3289669|  0.8|**2122**|**10499**|**trle**|
|4482388|  1.1|346|3467|mrle|
|4732081|  1.2|**7971**|10156|**srle 8**|
|4732082|  1.2|2110|10494|srle 0 (auto escape)|
|8431853|  2.1|4848|10272|srle 16|
|8832647|  2.2|1274|2921|rle64 8|
|9265516|  2.3|2241|5722|rle64 16|
|13727062|  3.4|**8515**|10421|**srle 32**|
|15175482|  3.8|4609|9360|rle64 32|
|19844801|  4.9|**14114**|**10611**|**srle 64**|
|21910714|  5.4|8301|10232|rle64 64|
|403920058|100.0|9391|9161|memcpy|

(**bold** = pareto)  MB=1.000.000.

- Checkers program "End Game Table Base": [1034.db](http://encode.ru/threads/2077-EGTB-compression?p=41392&viewfull=1#post41392)

|C Size|ratio%|C MB/s|D MB/s|Name|
|--------:|-----:|--------:|--------:|----------------|
|82421332| 19.7|**801**|**4145**|**trle**|
|88055364| 21.0|273|1255|mrle|
|92422320| 22.0|**814**|**5936**|**srle 0**|
|92423009| 22.0|**1178**|**6697**|**srle 8**|
|93905327| 22.4|780|1660|rle64 8|
|113620895| 27.1|**1906**|5550|**srle 16**|
|117590491| 28.0|1341|2826|rle64 16|
|136948765| 32.7|**3581**|**8360**|**srle 32**|
|143953177| 34.3|2971|5506|rle64 32|
|165561604| 39.5|**5924**|**8953**|**srle 64**|
|176442237| 42.1|5090|7872|rle64 64|
|419225629|100.0|**9323**|**9179**|**memcpy**|

- Text File: [enwik9bwt](http://mattmahoney.net/dc/textdata.html) enwik9 bwt generated w.<br>
[libdivsufsort](https://code.google.com/p/libdivsufsort/)

|C Size|ratio%|C MB/s|D MB/s|Name|
|--------:|-----:|--------:|--------:|----------------|
|378377069| 37.8|**500**|**2090**|**trle**|
|419339698| 41.9|**506**|**5937**|**srle 0**|
|419340318| 41.9|**626**|4408|**srle 8**|
|422296235| 42.2|558|1364|rle64 8|
|487461871| 48.7|**1396**|4373|**srle 16**|
|498420792| 49.8|1113|2511|rle64 16|
|549214826| 54.9|**2778**|**7540**|**srle 32**|
|563503744| 56.4|2730|5074|rle64 32|
|576619945| 57.7|217|793|mrle|
|605764304| 60.6|**4998**|**7784**|**srle 64**|
|620676412| 62.1|**5247**|7376|**rle64 64**|
|1000000012|100.0|**9364**|**9184**|**memcpy**|

## Compile:

        make

## Usage:

        ./trle file
        ./trle -e# file

		# = function id (see file trle.c)

### Environment:

###### OS/Compiler (32 + 64 bits):
- Linux: GNU GCC (>=4.6)
- clang (>=3.2) 
- Windows: MinGW
- Windows: Visual Studio 2015

#### References
  - [Real-Time Compression of IEC 61869-9 Sampled Value Data](https://pure.strath.ac.uk/portal/files/55444712/Blair_etal_AMPS2016_Real_time_compression_of_IEC_61869_9_sampled_value_data.pdf)
  - [Understanding Compression: Data Compression for Modern Developers](https://books.google.de/books?id=2C2rDAAAQBAJ&pg=PA216&lpg=PA216&dq=%22turborle%22&source=bl&ots=TiLU4Qf47s&sig=tkk0Dnk9NnU0JMR3Z6iW4TRquxg&hl=de&sa=X&ved=0ahUKEwjZq-Li5uXSAhXFCJoKHe77B6cQ6AEIyAEwHQ#v=onepage&q=%22turborle%22&f=false)
  - [Understanding Compression](http://file.allitebooks.com/20160805/Understanding%20Compression.pdf)
  
Last update: 27 MAR 2017


