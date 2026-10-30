<!-- DRAFT: comment for PR #336 proposing aceapex in INT_MT. NOT POSTED. -->

# Measurements behind the comment

lzbench branch aceapex-2.2.0 at bd98777 (aceapex 2.2.1, lzbench 2.4.1), silesia.tar, AMD EPYC 4344P
(8 cores / 16 threads), gcc 11.4, bundled zstd 1.5.7, default iterations. As in lzbench's README: `-I#`
pinned to cores 0..#-1 (`taskset -c 0-$((#-1)) ./lzbench -eaceapex,1,2,3/zstd,1,3 -I# silesia.tar`); -I16
uses the SMT siblings. Two runs (2.2.0 and 2.2.1, the decoder is the same code); where they differ the range
is given. aceapex archives are the same bytes at every -I.

| codec | ratio % | -I1 comp / decomp | -I8 comp / decomp | -I16 comp / decomp |
|---|---|---|---|---|
| aceapex 2.2.1 -1 | 32.56 | 87-89 / 1107-1117 | 386-401 / 2926-2967 | 370-378 / 3387-3540 |
| aceapex 2.2.1 -2 | 32.38 | 65-66 / 1121-1132 | 313-321 / 2954-3013 | 311-312 / 2749-3609 |
| aceapex 2.2.1 -3 | 31.44 | 195-200 / 1030-1036 | 623 / 2271-2284 | 620-640 / 2097-2718 |
| zstd 1.5.7 -1 | 34.53-34.58 | 690 / 2153 | 4277 / 2127 | 5365 / 2095 |
| zstd 1.5.7 -3 | 31.20-31.24 | 411 / 1899 | 2250 / 1892 | 2465 / 1854 |

(-I1 compression of aceapex from the 2.2.1 run: 2.2.0 started helper threads in its entropy stage at one
thread; pinned, both give the same single-core figures. -I16 decompression varies between runs by up to
25 % for levels 2-3 on this machine - SMT siblings; -I8 is stable.)

---

## Comment for PR #336 (draft)

Numbers for internal threads (`-I#`, pinned to cores 0..#-1 with taskset as in the README), silesia.tar,
EPYC 4344P 8C/16T, this branch (aceapex 2.2.1):

| | -I1 comp / decomp | -I8 comp / decomp | -I16 comp / decomp |
|---|---|---|---|
| aceapex 2.2.1 -1 | 89 / 1114 | 401 / 2967 | 378 / 3387 |
| aceapex 2.2.1 -2 | 66 / 1122 | 321 / 3013 | 311 / 2749 |
| aceapex 2.2.1 -3 | 199 / 1030 | 623 / 2284 | 620 / 2097 |
| zstd 1.5.7 -1 | 690 / 2153 | 4277 / 2127 | 5365 / 2095 |
| zstd 1.5.7 -3 | 411 / 1899 | 2250 / 1892 | 2465 / 1854 |

aceapex uses -I in both directions: its blocks are independent, so decompression scales as well as
compression (2.7x at -I8 for levels 1-2), and the output is identical at every thread count. With 2.2.1 -I1
is really one thread (2.2.0 still used helper threads in its entropy stage; fixed). Would you consider
adding `aceapex,1,2,3` to INT_MT? Not done in this PR - the aliases are yours.
