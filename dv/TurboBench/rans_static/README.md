Introduction
------------

These are my derivations of Fabien 'Ryg's rANS coder plus a
static arithmetic coder derived from Eugene Shelwein's work.
Therefore none of the really complex stuff is my own and the real
credit belongs elsewhere.

The originals can be found at:
-    https://github.com/rygorous/ryg_rans
-    http://ctxmodel.net (defunct?)

I thought about forking this, but I didn't do that way back when I
first created my variants and so it'd be a fork in name only, with no
actual history.

The arithmetic coder was designed to be a fast unrolled variant that
coded using multiple arithmetic coders simultaneously. I also
implemented a static order-1 model in addition to the usual order-0.

About the same time I'd finished that code the ANS implementations
started arriving so I took Ryg's original rANS (two way interleaved
variant) and produced a 4-way interleaved version.  Successive
optimisations lead version "4c".  Most recently I explored some newer
tweaks and ideas.

- arith_static.c
	The unrolled order-0/1 arithmetic coder.

- rANS_static4x8.c
	A 4-way unrolled order-0/1 rANS coder with 8-bit renormalisation.
	This is compatible with the rANS_static4c (and 4j) variants
	used in CRAM, but more heavily optimised with additional
	assembly.

- rANS_static4x16.c
	A variant of the rANS_static4 code that uses 16-bit
	renormalisation.  It is incompatible with the 4x8 output.
	This is the same idea used in Ryg's SIMD variant; by keeping
	the rANS state as 15+16 instead of 23+8 we only ever have one
	normalisation step instead of two.  This also uses assembly.

	Note that to further test the maximum performance of order-1
	encoding, the table sizes were reduced to 9 bits for 4x16
	instead of 12 bits used in order-0 (and 12 in both order-0 and
	order-1 in 4x8).  Thus the two are not directly comparable
	with the order-1 method.

- rANS_static64c.c, rans64.h
	A 4 way unrolled version of Ryg's rans64.h.
	(This needs a mulhi cpu instruction to do 64-bit by 64-bit
	multiplication and take the top 64-bits of the result.
	Available on most(all?) x86_64 architectures, but I needed to
	consider 32-bit hosts too.)

There are also some more experimental codecs for investigating use of
SIMD (in a rather poor state code-wise at present with lots of
warnings and commented out bits of code).  On some of these the
order-1 coder is non functioning or not yet updated.

- rNx16.c
	A generic N-way interleaving with N states and N decode/encode
	buffers.  This is an attempt to permit automatic vectorisation
	within compilers by removing dependency between the loop over
	N states, but it suffers from a large amount of memory
	gathers.  Compile with eg -DNX=32 to change N.
	NB: at present only icc seems able to vectorise the decoder.

- rNx16b.c
	A generic N-way interleaving with N states and one shared
	decode/encode buffer.  This has better memory utilisation than
	rNx16.c, but the shared buffer makes automatic vectorisation
	challenging.
	With -DNX=4 this is binary compatible with rANS_static4x16.c
	output. but around 10-20% slower to due no manual reordering
	of the statements.

- r8x16_sse.c
- r8x16_avx2.c
- r16x16_avx2.c
- r16x16_avx512.c
- r32x16_avx2.c
- r32x16_avx512.c
	Various custom versions of rNx16.c using SSE, AVX2 or AVX512
	instructions for N 8, 16 and 32.

- r8x16b_avx2.c
- r16x16b_avx2.c
- r16x16b_avx512.c
- r32x16b_avx2.c
- r32x16b_avx512.c
	Various custom versions of rNx16b.c using SSE, AVX2 or AVX512
	instructions for N 8, 16 and 32.
	These are typically faster than the rNx16.c variants.


PS.
See http://encode.ru/threads/1867-Unrolling-arithmetic-coding
for the thread that started this particular ball rolling.


Usage
-----

The rANS_test.c file builds the rANS_static test application.  This
has -8 and -16 parameters to control 8 and 16 bit renormalisation
values, using the code above.  It is also possible to build
rANS_static4c, rANS_static4j and rANS_static64c test executables with
"make old", but these are included only for testing older code
variants.

Order 0 encoding and decoding

    rANS_static -o0 in in.rans0
    rANS_static -d in.rans0 in.decoded

Order 1 encoding and decoding

    rANS_static -o0 in in.rans1
    rANS_static -d in.rans1 in.decoded

Testing/benchmarking order 1

    rANS_static -o1 -t in


Benchmarks
----------

Q40 and Q8 are DNA sequencing quality values with approx 40 and 8
distinct values each, representing high and low complexity data.

    $ head -5 /tmp/Q40
    AAA7DDFFFHHHGFHIGHGIGGIJIGHCIGGHBGFCGGGEGGCGEG@F;FC;8@@F>@E@6)=?B66?@A;?(555;=((,55(((((+2(39(288<
    :?A=DA;?BDH?F<BGG>F>?;EF>8:8?E>FHGIIIEIHICH9@F(?BFEEGHEIIC<@DEEEEEC2?=A;;?@?;((5,5?9(9<0()9388(>AC
    AAA@ABFFFHHHGHGJIGEEIGHIGIIIIIJIHEGIGGIGGGIDGIIIJH>B<@DD;;B?E===;@FDGH3=@EAHA;?))7;6((.5(,;(,((5(,
    ?BABFABDFHHHHHHHIJJJJJIIJJJGHGIJDIE@DDGDF@GHGGGAEGGCEE>CDFDB;>>A@=;AC??BBDC(<A<?C<AB?<??AB8<(28?09
    2((2+&)&)&(+24++(((+22(:))&&0-))&&&)&3,,,((,,'',''-(/)).))))(((((0(()))3.--2))).))2)50-(((((((((((
    
 $ head -5 /tmp/Q8
    --A--JJ7-----<---7-
    7----<--<<-<7-7--FF
    --FAAA7-JFFFF<FFJ<7<FFFJFAJFFFJJAAF<A<JJ<FJJ7FJJFFJJ7F7FAAJJFFFFJJJJFJFJJFJFJFJJJJFJFJJFFJF<JJFJJJJJ7FJJJJF<7<FJJJAJJJFAFJFFFJF<JFJJJJJJFAJJFJJJJJFFFFF
    JJJ<JJJJ<JJJJJ7FFJFJJFJJF<AFAAJJJFJFJJ7FJFJJ7FFJJFJFFFFF<<AAFJJAFF
    FFFFFJFJJFJJJFJJJJJAJJJJAJJJFJJJJFJJJJFJJJJJJJJJJJJJJJFJFJFAJJJJJJJFJAJJJJ<AJJJJJJFJJJJFFJFJFJJFFJFJJFFJFJJAJJJJFJJ7JJJJAJJJ7J-FAJAJ7JJJAFFAFFFAFF-JFFA


Tested on an "Intel(R) Core(TM) i5-4570 CPU @ 3.20GHz" (from
/proc/cpuinfo on Ubuntu Trusty) using Gcc, Clang and Intel compilers.

Also if cache memory is tight on your CPU, consider switching to the
rans_uncompress_O1c_4x16() function instead of rans_uncompress_O1_sfb_4x16()
or changing ssym[] array to be uint8_t instead of uint16_t in sfb.  (This
will work just fine as 8-bit instead, but seems to be slower on my
system.)

-----------------------------------------------------------------------------

gcc 6.2.0, q8 test file, order 0
    
    arith_static        240.2 MB/s enc, 146.5 MB/s dec	 73124567 bytes -> 16852961 bytes
    rANS_static4c       294.2 MB/s enc, 335.2 MB/s dec	 73124567 bytes -> 16850374 bytes
    rANS_static4j       294.3 MB/s enc, 328.1 MB/s dec	 73124567 bytes -> 16847058 bytes
    rANS_static4x16     369.3 MB/s enc, 664.5 MB/s dec	 73124567 bytes -> 16847245 bytes
    rANS_static4x8      300.0 MB/s enc, 681.2 MB/s dec	 73124567 bytes -> 16847000 bytes
    rANS_static64c      385.5 MB/s enc, 538.4 MB/s dec	 73124567 bytes -> 16851067 bytes
    r4x16               354.2 MB/s enc, 389.3 MB/s dec	 73124567 bytes -> 16848349 bytes
    r4x16b              386.5 MB/s enc, 588.5 MB/s dec	 73124567 bytes -> 16847245 bytes
    r8x16               351.7 MB/s enc, 393.3 MB/s dec	 73124567 bytes -> 16850295 bytes
    r8x16_avx2          351.6 MB/s enc, 393.1 MB/s dec	 73124567 bytes -> 16850295 bytes
    r8x16_sse           352.1 MB/s enc, 576.8 MB/s dec	 73124567 bytes -> 16850295 bytes
    r8x16b              383.9 MB/s enc, 583.2 MB/s dec	 73124567 bytes -> 16848087 bytes
    r8x16b_avx2         360.0 MB/s enc, 404.1 MB/s dec	 73124567 bytes -> 16848087 bytes
    r16x16              283.2 MB/s enc, 394.3 MB/s dec	 73124567 bytes -> 16854189 bytes
    r16x16_avx2         283.2 MB/s enc, 895.9 MB/s dec	 73124567 bytes -> 16854189 bytes
    r16x16b             369.3 MB/s enc, 527.1 MB/s dec	 73124567 bytes -> 16849773 bytes
    r16x16b_avx2        289.0 MB/s enc, 982.3 MB/s dec	 73124567 bytes -> 16849773 bytes
    r32x16              291.2 MB/s enc, 380.3 MB/s dec	 73124567 bytes -> 16861939 bytes
    r32x16_avx2         291.2 MB/s enc, 904.1 MB/s dec	 73124567 bytes -> 16861939 bytes
    r32x16b             377.1 MB/s enc, 537.1 MB/s dec	 73124567 bytes -> 16853107 bytes
    r32x16b_avx2        534.7 MB/s enc, 1430.6 MB/s dec	 73124567 bytes -> 16853107 bytes
    
gcc 6.2.0, q40 test file, order 0

    arith_static        246.0 MB/s enc, 144.0 MB/s dec	 94602182 bytes -> 53713263 bytes
    rANS_static4c       305.0 MB/s enc, 360.4 MB/s dec	 94602182 bytes -> 53694731 bytes
    rANS_static4j       305.0 MB/s enc, 333.7 MB/s dec	 94602182 bytes -> 53690573 bytes
    rANS_static4x16     302.4 MB/s enc, 659.2 MB/s dec	 94602182 bytes -> 53689007 bytes
    rANS_static4x8      326.9 MB/s enc, 673.2 MB/s dec	 94602182 bytes -> 53688173 bytes
    rANS_static64c      299.7 MB/s enc, 423.3 MB/s dec	 94602182 bytes -> 53695650 bytes
    r4x16               286.8 MB/s enc, 388.3 MB/s dec	 94602182 bytes -> 53690447 bytes
    r4x16b              382.9 MB/s enc, 582.6 MB/s dec	 94602182 bytes -> 53689007 bytes
    r8x16               238.4 MB/s enc, 392.8 MB/s dec	 94602182 bytes -> 53692915 bytes
    r8x16_avx2          238.1 MB/s enc, 393.0 MB/s dec	 94602182 bytes -> 53692915 bytes
    r8x16_sse           238.3 MB/s enc, 571.0 MB/s dec	 94602182 bytes -> 53692915 bytes
    r8x16b              379.8 MB/s enc, 581.7 MB/s dec	 94602182 bytes -> 53690035 bytes
    r8x16b_avx2         241.6 MB/s enc, 403.2 MB/s dec	 94602182 bytes -> 53690035 bytes
    r16x16              198.3 MB/s enc, 391.4 MB/s dec	 94602182 bytes -> 53697949 bytes
    r16x16_avx2         198.2 MB/s enc, 879.0 MB/s dec	 94602182 bytes -> 53697949 bytes
    r16x16b             365.0 MB/s enc, 527.3 MB/s dec	 94602182 bytes -> 53692189 bytes
    r16x16b_avx2        204.6 MB/s enc, 970.6 MB/s dec	 94602182 bytes -> 53692189 bytes
    r32x16              202.5 MB/s enc, 329.2 MB/s dec	 94602182 bytes -> 53708047 bytes
    r32x16_avx2         202.4 MB/s enc, 705.4 MB/s dec	 94602182 bytes -> 53708047 bytes
    r32x16b             372.4 MB/s enc, 533.3 MB/s dec	 94602182 bytes -> 53696527 bytes
    r32x16b_avx2        525.1 MB/s enc, 1415.0 MB/s dec	 94602182 bytes -> 53696527 bytes
    
gcc 6.2.0, q8 test file, order 1
    
    arith_static        175.9 MB/s enc, 131.3 MB/s dec	 73124567 bytes -> 15858338 bytes
    rANS_static4c       190.9 MB/s enc, 280.7 MB/s dec	 73124567 bytes -> 15849499 bytes
    rANS_static4j       191.6 MB/s enc, 281.3 MB/s dec	 73124567 bytes -> 15849499 bytes
    rANS_static4x16     231.8 MB/s enc, 478.1 MB/s dec	 73124567 bytes -> 15866708 bytes
    rANS_static4x8      197.5 MB/s enc, 430.0 MB/s dec	 73124567 bytes -> 15849499 bytes
    rANS_static64c      241.7 MB/s enc, 391.5 MB/s dec	 73124567 bytes -> 15850232 bytes
    r4x16               231.7 MB/s enc, 467.8 MB/s dec	 73124567 bytes -> 15866708 bytes
    r4x16b              251.0 MB/s enc, 461.0 MB/s dec	 73124567 bytes -> 15866708 bytes
    r8x16               231.2 MB/s enc, 468.1 MB/s dec	 73124567 bytes -> 15866708 bytes
    r8x16_avx2          231.2 MB/s enc, 469.1 MB/s dec	 73124567 bytes -> 15866708 bytes
    r8x16_sse           231.7 MB/s enc, 468.9 MB/s dec	 73124567 bytes -> 15866708 bytes
    r8x16b              234.1 MB/s enc, 433.0 MB/s dec	 73124567 bytes -> 15867650 bytes
    r8x16b_avx2         231.8 MB/s enc, 469.1 MB/s dec	 73124567 bytes -> 15866708 bytes
    r16x16              231.2 MB/s enc, 468.7 MB/s dec	 73124567 bytes -> 15866708 bytes
    r16x16_avx2         231.6 MB/s enc, 468.7 MB/s dec	 73124567 bytes -> 15866708 bytes
    r16x16b             238.3 MB/s enc, 390.2 MB/s dec	 73124567 bytes -> 15869416 bytes
    r16x16b_avx2        230.7 MB/s enc, 469.0 MB/s dec	 73124567 bytes -> 15866708 bytes
    r32x16              230.9 MB/s enc, 468.8 MB/s dec	 73124567 bytes -> 15866708 bytes
    r32x16_avx2         ERR
    r32x16b             236.0 MB/s enc, 392.1 MB/s dec	 73124567 bytes -> 15872840 bytes
    r32x16b_avx2        237.1 MB/s enc, 1039.4 MB/s dec	 73124567 bytes -> 15872840 bytes
    
gcc 6.2.0, q40 test file, order 1

    arith_static        127.5 MB/s enc,  97.7 MB/s dec	 94602182 bytes -> 43399150 bytes
    rANS_static4c       153.2 MB/s enc, 204.5 MB/s dec	 94602182 bytes -> 43165948 bytes
    rANS_static4j       153.4 MB/s enc, 224.6 MB/s dec	 94602182 bytes -> 43165948 bytes
    rANS_static4x16     180.5 MB/s enc, 425.6 MB/s dec	 94602182 bytes -> 43381697 bytes
    rANS_static4x8      159.6 MB/s enc, 366.3 MB/s dec	 94602182 bytes -> 43165948 bytes
    rANS_static64c      204.0 MB/s enc, 279.9 MB/s dec	 94602182 bytes -> 43166873 bytes
    r4x16               181.4 MB/s enc, 421.2 MB/s dec	 94602182 bytes -> 43381697 bytes
    r4x16b              243.5 MB/s enc, 436.7 MB/s dec	 94602182 bytes -> 43381697 bytes
    r8x16               181.2 MB/s enc, 421.6 MB/s dec	 94602182 bytes -> 43381697 bytes
    r8x16_avx2          181.6 MB/s enc, 324.2 MB/s dec	 94602182 bytes -> 43381697 bytes
    r8x16_sse           181.1 MB/s enc, 324.9 MB/s dec	 94602182 bytes -> 43381697 bytes
    r8x16b              224.7 MB/s enc, 407.4 MB/s dec	 94602182 bytes -> 43383127 bytes
    r8x16b_avx2         177.9 MB/s enc, 315.8 MB/s dec	 94602182 bytes -> 43381697 bytes
    r16x16              181.4 MB/s enc, 416.7 MB/s dec	 94602182 bytes -> 43381697 bytes
    r16x16_avx2         181.5 MB/s enc, 325.2 MB/s dec	 94602182 bytes -> 43381697 bytes
    r16x16b             227.9 MB/s enc, 379.6 MB/s dec	 94602182 bytes -> 43385998 bytes
    r16x16b_avx2        181.2 MB/s enc, 323.9 MB/s dec	 94602182 bytes -> 43381697 bytes
    r32x16              181.5 MB/s enc, 422.6 MB/s dec	 94602182 bytes -> 43381697 bytes
    r32x16_avx2         ERR
    r32x16b             223.5 MB/s enc, 381.4 MB/s dec	 94602182 bytes -> 43391024 bytes
    r32x16b_avx2        234.5 MB/s enc, 985.9 MB/s dec	 94602182 bytes -> 43391024 bytes



-----------------------------------------------------------------------------

clang 3.7.0, q8 test file, order 0

    arith_static        220.6 MB/s enc, 123.3 MB/s dec	 73124567 bytes -> 16852961 bytes
    rANS_static4c       276.1 MB/s enc, 330.8 MB/s dec	 73124567 bytes -> 16850374 bytes
    rANS_static4j       275.6 MB/s enc, 325.8 MB/s dec	 73124567 bytes -> 16847058 bytes
    rANS_static4x16     336.2 MB/s enc, 612.8 MB/s dec	 73124567 bytes -> 16847245 bytes
    rANS_static4x8      280.3 MB/s enc, 636.7 MB/s dec	 73124567 bytes -> 16847000 bytes
    rANS_static64c      374.0 MB/s enc, 545.4 MB/s dec	 73124567 bytes -> 16851067 bytes
    r4x16               319.3 MB/s enc, 411.9 MB/s dec	 73124567 bytes -> 16848349 bytes
    r4x16b              336.7 MB/s enc, 598.1 MB/s dec	 73124567 bytes -> 16847245 bytes
    r8x16               282.6 MB/s enc, 338.2 MB/s dec	 73124567 bytes -> 16850295 bytes
    r8x16_avx2          282.5 MB/s enc, 322.3 MB/s dec	 73124567 bytes -> 16850295 bytes
    r8x16_sse           277.9 MB/s enc, 713.4 MB/s dec	 73124567 bytes -> 16850295 bytes
    r8x16b              319.7 MB/s enc, 579.2 MB/s dec	 73124567 bytes -> 16848087 bytes
    r8x16b_avx2         297.2 MB/s enc, 328.5 MB/s dec	 73124567 bytes -> 16848087 bytes
    r16x16              272.7 MB/s enc, 325.1 MB/s dec	 73124567 bytes -> 16854189 bytes
    r16x16_avx2         272.9 MB/s enc, 850.8 MB/s dec	 73124567 bytes -> 16854189 bytes
    r16x16b             341.7 MB/s enc, 560.5 MB/s dec	 73124567 bytes -> 16849773 bytes
    r16x16b_avx2        297.7 MB/s enc, 961.3 MB/s dec	 73124567 bytes -> 16849773 bytes
    r32x16              277.7 MB/s enc, 332.5 MB/s dec	 73124567 bytes -> 16861939 bytes
    r32x16_avx2         285.0 MB/s enc, 840.2 MB/s dec	 73124567 bytes -> 16861939 bytes
    r32x16b             338.4 MB/s enc, 554.5 MB/s dec	 73124567 bytes -> 16853107 bytes
    r32x16b_avx2        566.4 MB/s enc, 1220.7 MB/s dec	 73124567 bytes -> 16853107 bytes
    
clang 3.7.0, q40 test file, order 0

    arith_static        226.6 MB/s enc, 125.8 MB/s dec	 94602182 bytes -> 53713263 bytes
    rANS_static4c       287.8 MB/s enc, 349.1 MB/s dec	 94602182 bytes -> 53694731 bytes
    rANS_static4j       286.2 MB/s enc, 346.6 MB/s dec	 94602182 bytes -> 53690573 bytes
    rANS_static4x16     276.6 MB/s enc, 610.0 MB/s dec	 94602182 bytes -> 53689007 bytes
    rANS_static4x8      307.4 MB/s enc, 650.2 MB/s dec	 94602182 bytes -> 53688173 bytes
    rANS_static64c      291.3 MB/s enc, 444.4 MB/s dec	 94602182 bytes -> 53695650 bytes
    r4x16               263.6 MB/s enc, 328.9 MB/s dec	 94602182 bytes -> 53690447 bytes
    r4x16b              335.4 MB/s enc, 594.0 MB/s dec	 94602182 bytes -> 53689007 bytes
    r8x16               187.2 MB/s enc, 208.8 MB/s dec	 94602182 bytes -> 53692915 bytes
    r8x16_avx2          187.6 MB/s enc, 209.1 MB/s dec	 94602182 bytes -> 53692915 bytes
    r8x16_sse           187.6 MB/s enc, 727.3 MB/s dec	 94602182 bytes -> 53692915 bytes
    r8x16b              316.9 MB/s enc, 590.6 MB/s dec	 94602182 bytes -> 53690035 bytes
    r8x16b_avx2         200.4 MB/s enc, 205.5 MB/s dec	 94602182 bytes -> 53690035 bytes
    r16x16              193.3 MB/s enc, 218.0 MB/s dec	 94602182 bytes -> 53697949 bytes
    r16x16_avx2         190.9 MB/s enc, 800.4 MB/s dec	 94602182 bytes -> 53697949 bytes
    r16x16b             337.8 MB/s enc, 544.0 MB/s dec	 94602182 bytes -> 53692189 bytes
    r16x16b_avx2        207.0 MB/s enc, 955.0 MB/s dec	 94602182 bytes -> 53692189 bytes
    r32x16              199.7 MB/s enc, 218.7 MB/s dec	 94602182 bytes -> 53708047 bytes
    r32x16_avx2         199.8 MB/s enc, 700.0 MB/s dec	 94602182 bytes -> 53708047 bytes
    r32x16b             341.5 MB/s enc, 556.8 MB/s dec	 94602182 bytes -> 53696527 bytes
    r32x16b_avx2        553.4 MB/s enc, 1216.0 MB/s dec	 94602182 bytes -> 53696527 bytes
    
   
clang 3.7.0, q8 test file, order 1

    
    arith_static        171.1 MB/s enc, 107.5 MB/s dec	 73124567 bytes -> 15858338 bytes
    rANS_static4c       186.7 MB/s enc, 279.4 MB/s dec	 73124567 bytes -> 15849499 bytes
    rANS_static4j       185.8 MB/s enc, 264.0 MB/s dec	 73124567 bytes -> 15849499 bytes
    rANS_static4x16     218.9 MB/s enc, 434.7 MB/s dec	 73124567 bytes -> 15866708 bytes
    rANS_static4x8      186.7 MB/s enc, 405.2 MB/s dec	 73124567 bytes -> 15849499 bytes
    rANS_static64c      242.7 MB/s enc, 430.9 MB/s dec	 73124567 bytes -> 15850232 bytes
    r4x16               212.5 MB/s enc, 422.7 MB/s dec	 73124567 bytes -> 15866708 bytes
    r4x16b              219.9 MB/s enc, 288.1 MB/s dec	 73124567 bytes -> 15866708 bytes
    r8x16               218.3 MB/s enc, 436.9 MB/s dec	 73124567 bytes -> 15866708 bytes
    r8x16_avx2          218.7 MB/s enc, 454.0 MB/s dec	 73124567 bytes -> 15866708 bytes
    r8x16_sse           218.1 MB/s enc, 461.4 MB/s dec	 73124567 bytes -> 15866708 bytes
    r8x16b              222.7 MB/s enc, 329.5 MB/s dec	 73124567 bytes -> 15867650 bytes
    r8x16b_avx2         219.1 MB/s enc, 467.5 MB/s dec	 73124567 bytes -> 15866708 bytes
    r16x16              213.2 MB/s enc, 425.3 MB/s dec	 73124567 bytes -> 15866708 bytes
    r16x16_avx2         213.8 MB/s enc, 457.3 MB/s dec	 73124567 bytes -> 15866708 bytes
    r16x16b             212.7 MB/s enc, 343.5 MB/s dec	 73124567 bytes -> 15869416 bytes
    r16x16b_avx2        219.0 MB/s enc, 467.3 MB/s dec	 73124567 bytes -> 15866708 bytes
    r32x16              218.3 MB/s enc, 433.4 MB/s dec	 73124567 bytes -> 15866708 bytes
    r32x16_avx2         ERR
    r32x16b             202.2 MB/s enc, 341.1 MB/s dec	 73124567 bytes -> 15872840 bytes
    r32x16b_avx2        269.8 MB/s enc, 842.3 MB/s dec	 73124567 bytes -> 15872840 bytes

clang 3.7.0, q40 test file, order 1

    arith_static        121.6 MB/s enc,  79.5 MB/s dec	 94602182 bytes -> 43399150 bytes
    rANS_static4c       153.3 MB/s enc, 215.7 MB/s dec	 94602182 bytes -> 43165948 bytes
    rANS_static4j       154.7 MB/s enc, 215.8 MB/s dec	 94602182 bytes -> 43165948 bytes
    rANS_static4x16     175.2 MB/s enc, 392.4 MB/s dec	 94602182 bytes -> 43381697 bytes
    rANS_static4x8      155.3 MB/s enc, 356.7 MB/s dec	 94602182 bytes -> 43165948 bytes
    rANS_static64c      203.9 MB/s enc, 309.2 MB/s dec	 94602182 bytes -> 43166873 bytes
    r4x16               175.4 MB/s enc, 399.3 MB/s dec	 94602182 bytes -> 43381697 bytes
    r4x16b              212.5 MB/s enc, 284.7 MB/s dec	 94602182 bytes -> 43381697 bytes
    r8x16               173.3 MB/s enc, 399.8 MB/s dec	 94602182 bytes -> 43381697 bytes
    r8x16_avx2          175.5 MB/s enc, 314.3 MB/s dec	 94602182 bytes -> 43381697 bytes
    r8x16_sse           173.9 MB/s enc, 311.9 MB/s dec	 94602182 bytes -> 43381697 bytes
    r8x16b              216.4 MB/s enc, 312.3 MB/s dec	 94602182 bytes -> 43383127 bytes
    r8x16b_avx2         174.9 MB/s enc, 314.6 MB/s dec	 94602182 bytes -> 43381697 bytes
    r16x16              175.2 MB/s enc, 399.2 MB/s dec	 94602182 bytes -> 43381697 bytes
    r16x16_avx2         172.3 MB/s enc, 311.8 MB/s dec	 94602182 bytes -> 43381697 bytes
    r16x16b             203.1 MB/s enc, 311.4 MB/s dec	 94602182 bytes -> 43385998 bytes
    r16x16b_avx2        174.9 MB/s enc, 313.3 MB/s dec	 94602182 bytes -> 43381697 bytes
    r32x16              172.1 MB/s enc, 393.7 MB/s dec	 94602182 bytes -> 43381697 bytes
    r32x16_avx2         ERR
    r32x16b             195.3 MB/s enc, 307.9 MB/s dec	 94602182 bytes -> 43391024 bytes
    r32x16b_avx2        260.4 MB/s enc, 821.9 MB/s dec	 94602182 bytes -> 43391024 bytes

-----------------------------------------------------------------------------

icc 15.0.0, q8 test file, order 0

    arith_static        237.0 MB/s enc, 158.8 MB/s dec	 73124567 bytes -> 16852961 bytes
    rANS_static4c       306.2 MB/s enc, 316.1 MB/s dec	 73124567 bytes -> 16850374 bytes
    rANS_static4j       308.4 MB/s enc, 348.7 MB/s dec	 73124567 bytes -> 16847058 bytes
    rANS_static4x16     380.5 MB/s enc, 637.8 MB/s dec	 73124567 bytes -> 16847245 bytes
    rANS_static4x8      306.2 MB/s enc, 654.5 MB/s dec	 73124567 bytes -> 16847000 bytes
    rANS_static64c      448.2 MB/s enc, 516.0 MB/s dec	 73124567 bytes -> 16851067 bytes
    r4x16               336.5 MB/s enc, 302.2 MB/s dec	 73124567 bytes -> 16848349 bytes
    r4x16b              380.6 MB/s enc, 241.9 MB/s dec	 73124567 bytes -> 16847245 bytes
    r8x16               340.9 MB/s enc, 582.0 MB/s dec	 73124567 bytes -> 16850295 bytes
    r8x16_avx2          340.6 MB/s enc, 579.5 MB/s dec	 73124567 bytes -> 16850295 bytes
    r8x16_sse           339.4 MB/s enc, 565.9 MB/s dec	 73124567 bytes -> 16850295 bytes
    r8x16b              374.3 MB/s enc, 520.2 MB/s dec	 73124567 bytes -> 16848087 bytes
    r8x16b_avx2         348.7 MB/s enc, 580.5 MB/s dec	 73124567 bytes -> 16848087 bytes
    r16x16              339.4 MB/s enc, 880.1 MB/s dec	 73124567 bytes -> 16854189 bytes
    r16x16_avx2         339.8 MB/s enc, 962.8 MB/s dec	 73124567 bytes -> 16854189 bytes
    r16x16b             355.8 MB/s enc, 514.3 MB/s dec	 73124567 bytes -> 16849773 bytes
    r16x16b_avx2        348.0 MB/s enc, 973.0 MB/s dec	 73124567 bytes -> 16849773 bytes
    r32x16              293.9 MB/s enc, 856.1 MB/s dec	 73124567 bytes -> 16861939 bytes
    r32x16_avx2         293.8 MB/s enc, 917.8 MB/s dec	 73124567 bytes -> 16861939 bytes
    r32x16b             356.4 MB/s enc, 516.2 MB/s dec	 73124567 bytes -> 16853107 bytes
    r32x16b_avx2        432.8 MB/s enc, 1359.0 MB/s dec	 73124567 bytes -> 16853107 bytes
    
icc 15.0.0, q40 test file, order 0

    arith_static        262.8 MB/s enc, 158.4 MB/s dec	 94602182 bytes -> 53713263 bytes
    rANS_static4c       305.2 MB/s enc, 332.5 MB/s dec	 94602182 bytes -> 53694731 bytes
    rANS_static4j       312.5 MB/s enc, 358.4 MB/s dec	 94602182 bytes -> 53690573 bytes
    rANS_static4x16     309.6 MB/s enc, 627.5 MB/s dec	 94602182 bytes -> 53689007 bytes
    rANS_static4x8      321.7 MB/s enc, 650.9 MB/s dec	 94602182 bytes -> 53688173 bytes
    rANS_static64c      341.1 MB/s enc, 409.2 MB/s dec	 94602182 bytes -> 53695650 bytes
    r4x16               275.6 MB/s enc, 300.5 MB/s dec	 94602182 bytes -> 53690447 bytes
    r4x16b              375.5 MB/s enc, 239.0 MB/s dec	 94602182 bytes -> 53689007 bytes
    r8x16               229.9 MB/s enc, 578.5 MB/s dec	 94602182 bytes -> 53692915 bytes
    r8x16_avx2          230.3 MB/s enc, 579.5 MB/s dec	 94602182 bytes -> 53692915 bytes
    r8x16_sse           233.1 MB/s enc, 564.7 MB/s dec	 94602182 bytes -> 53692915 bytes
    r8x16b              370.0 MB/s enc, 509.3 MB/s dec	 94602182 bytes -> 53690035 bytes
    r8x16b_avx2         237.1 MB/s enc, 581.2 MB/s dec	 94602182 bytes -> 53690035 bytes
    r16x16              224.2 MB/s enc, 862.3 MB/s dec	 94602182 bytes -> 53697949 bytes
    r16x16_avx2         224.3 MB/s enc, 963.5 MB/s dec	 94602182 bytes -> 53697949 bytes
    r16x16b             351.4 MB/s enc, 512.3 MB/s dec	 94602182 bytes -> 53692189 bytes
    r16x16b_avx2        229.4 MB/s enc, 947.5 MB/s dec	 94602182 bytes -> 53692189 bytes
    r32x16              204.9 MB/s enc, 671.2 MB/s dec	 94602182 bytes -> 53708047 bytes
    r32x16_avx2         205.5 MB/s enc, 733.1 MB/s dec	 94602182 bytes -> 53708047 bytes
    r32x16b             352.6 MB/s enc, 514.7 MB/s dec	 94602182 bytes -> 53696527 bytes
    r32x16b_avx2        432.8 MB/s enc, 1347.4 MB/s dec	 94602182 bytes -> 53696527 bytes


icc 15.0.0, q8 test file, order 0

    arith_static        180.8 MB/s enc, 133.5 MB/s dec	 73124567 bytes -> 15858338 bytes
    rANS_static4c       185.9 MB/s enc, 266.3 MB/s dec	 73124567 bytes -> 15849499 bytes
    rANS_static4j       194.9 MB/s enc, 297.8 MB/s dec	 73124567 bytes -> 15849499 bytes
    rANS_static4x16     174.6 MB/s enc, 414.7 MB/s dec	 73124567 bytes -> 15866708 bytes
    rANS_static4x8      197.2 MB/s enc, 379.1 MB/s dec	 73124567 bytes -> 15849499 bytes
    rANS_static64c      172.8 MB/s enc, 368.9 MB/s dec	 73124567 bytes -> 15850232 bytes
    r4x16               175.3 MB/s enc, 394.5 MB/s dec	 73124567 bytes -> 15866708 bytes
    r4x16b              243.2 MB/s enc, 215.9 MB/s dec	 73124567 bytes -> 15866708 bytes
    r8x16               176.5 MB/s enc, 394.1 MB/s dec	 73124567 bytes -> 15866708 bytes
    r8x16_avx2          176.2 MB/s enc, 410.4 MB/s dec	 73124567 bytes -> 15866708 bytes
    r8x16_sse           229.0 MB/s enc, 415.6 MB/s dec	 73124567 bytes -> 15866708 bytes
    r8x16b              238.3 MB/s enc, 284.1 MB/s dec	 73124567 bytes -> 15867650 bytes
    r8x16b_avx2         175.0 MB/s enc, 400.8 MB/s dec	 73124567 bytes -> 15866708 bytes
    r16x16              175.5 MB/s enc, 393.6 MB/s dec	 73124567 bytes -> 15866708 bytes
    r16x16_avx2         176.4 MB/s enc, 408.4 MB/s dec	 73124567 bytes -> 15866708 bytes
    r16x16b             233.2 MB/s enc, 311.6 MB/s dec	 73124567 bytes -> 15869416 bytes
    r16x16b_avx2        176.9 MB/s enc, 412.5 MB/s dec	 73124567 bytes -> 15866708 bytes
    r32x16              174.7 MB/s enc, 393.8 MB/s dec	 73124567 bytes -> 15866708 bytes
    r32x16_avx2         ERR
    r32x16b             231.4 MB/s enc, 308.3 MB/s dec	 73124567 bytes -> 15872840 bytes
    r32x16b_avx2        258.6 MB/s enc, 1060.8 MB/s dec	 73124567 bytes -> 15872840 bytes
    
icc 15.0.0, q40 test file, order 1

    arith_static        124.3 MB/s enc, 101.6 MB/s dec	 94602182 bytes -> 43399150 bytes
    rANS_static4c       145.5 MB/s enc, 213.5 MB/s dec	 94602182 bytes -> 43165948 bytes
    rANS_static4j       155.2 MB/s enc, 238.5 MB/s dec	 94602182 bytes -> 43165948 bytes
    rANS_static4x16     149.0 MB/s enc, 383.3 MB/s dec	 94602182 bytes -> 43381697 bytes
    rANS_static4x8      156.7 MB/s enc, 328.9 MB/s dec	 94602182 bytes -> 43165948 bytes
    rANS_static64c      154.6 MB/s enc, 290.0 MB/s dec	 94602182 bytes -> 43166873 bytes
    r4x16               148.7 MB/s enc, 383.5 MB/s dec	 94602182 bytes -> 43381697 bytes
    r4x16b              236.2 MB/s enc, 185.3 MB/s dec	 94602182 bytes -> 43381697 bytes
    r8x16               149.7 MB/s enc, 381.2 MB/s dec	 94602182 bytes -> 43381697 bytes
    r8x16_avx2          149.5 MB/s enc, 309.0 MB/s dec	 94602182 bytes -> 43381697 bytes
    r8x16_sse           178.0 MB/s enc, 311.8 MB/s dec	 94602182 bytes -> 43381697 bytes
    r8x16b              232.8 MB/s enc, 263.0 MB/s dec	 94602182 bytes -> 43383127 bytes
    r8x16b_avx2         148.4 MB/s enc, 302.2 MB/s dec	 94602182 bytes -> 43381697 bytes
    r16x16              150.3 MB/s enc, 383.7 MB/s dec	 94602182 bytes -> 43381697 bytes
    r16x16_avx2         150.2 MB/s enc, 308.5 MB/s dec	 94602182 bytes -> 43381697 bytes
    r16x16b             223.5 MB/s enc, 294.4 MB/s dec	 94602182 bytes -> 43385998 bytes
    r16x16b_avx2        149.8 MB/s enc, 307.9 MB/s dec	 94602182 bytes -> 43381697 bytes
    r32x16              147.7 MB/s enc, 382.8 MB/s dec	 94602182 bytes -> 43381697 bytes
    r32x16_avx2         ERR
    r32x16b             221.2 MB/s enc, 292.0 MB/s dec	 94602182 bytes -> 43391024 bytes
    r32x16b_avx2        250.6 MB/s enc, 986.6 MB/s dec	 94602182 bytes -> 43391024 bytes

-----------------------------------------------------------------------------


To do
-----

More work can be done as this is largely just experimental at present.

- Tidy up the API. A lot of functions have inappropriate names, eg
  r16x16_avx2 has a function rans_compress_O1_4x16(), due to starting
  from the 4x16 interface.

- Work out which TF_SHIFT_O1 is appropriate.  On low entropy data we
  can get away with 9 or 10 as it's much faster, but on high entropy
  data or with a large alphabet this is very suboptimal and we'd want
  11 or so in order to compress well.

- Move the frequency table code (counting, encoding, decoding) into a
  file shared by all codecs.

- Compress the frequency table. O1 table can be large, but is
  trivially encoded itself using order-0 rans.  We should also permit
  this table to be an argument to the functions so we can build a
  table from a large data set and apply the same table to many smaller
  data sets (permitting higher degree of random access without
  incurring high costs of storing complex tables).

- Order-1 table is also just an array of order-0 tables, but there is
  often strong correlation between each of these tables which we
  haven't exploited.  Eg.  maybe store the order-0 frequencies and
  then a set of deltas against it?  Or rotate our table so we have
  the same symbol with all order-1 frequencies against it (encode
  with delta and order-1)?  Lots to explore.

