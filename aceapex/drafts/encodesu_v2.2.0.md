<!-- DRAFT for encode.su thread 4487. NOT POSTED. Posting is the user's decision. -->

ACEAPEX 2.2.0 is tagged. Two things changed.

1. The encoder for DNA is new (l1): a small head table, no chain, only matches >= 32 bytes, short
repeats left to the literal coder. chr1: 68.1 MB -> 59.4 MB, encode 65 -> 444 MB/s per thread.
The old matcher was losing to zstd-3 on its own literal stream.

2. A genome archive can be written without zstd (rANS tokens + an open DNA pack). The GPU harness
decodes it with no nvCOMP call.

RTX PRO 6000 Blackwell, 16 KiB blocks, bit-perfect, median of 3:

| | archive | on-device | with H2D |
|---|---|---|---|
| chr1 zstd | 60 442 704 | 3.50 ms | 4.55 ms |
| chr1 open | 63 083 287 | 2.83 ms (89.9 GB/s) | 3.92 ms |
| T2T zstd | 822 393 156 | 35.5 ms | 49.8 ms |
| T2T open | 853 264 869 | 27.1 ms | 31.1 ms pipelined (101.6 GB/s) |

zstd rows decode literals with nvCOMP; open rows use no nvCOMP.

The open profile costs +3.8-4.4 % bytes against zstd. Tried and dropped, with numbers in the
CHANGELOG: two-pass CPU decode, match prefetch, a fused GPU kernel, order-1 literals as a format mode.

Check it yourself:
```
git clone https://github.com/yasha1971-coder/aceapex && cd aceapex && git checkout v2.2.0 && make && make test
```
