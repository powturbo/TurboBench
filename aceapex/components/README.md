# Components measured in the papers but not part of the default build

**rans1_v2.c** — order-1 rANS over literals, N=4 lines, 12-bit frequency
table, 64 KiB chunks. Measured at 2.1178 bits per byte against zstd-3's
2.861 on chr1 literals, encode 409 MB/s, decode 728 MB/s, round-trip exact.

Not in the default build. Decode falls 11% and region seek 43%, because the
four rANS lines share one byte stream with variable-length renormalisation:
reading a region means expanding the whole chunk, and checkpoints do not
help. Separate substreams per line would fix it, but that changes the
htscodecs schema this implementation deliberately follows. The figures are
correct; the trade is not.

**scan_bench.cu** — composition scan over the repeat-distance chain,
verified command by command against a CPU reference. The paper numbers on
an H100 and an RTX PRO 4000 come from this file.

**rans1_v4.c** — the same coder with the seek problem removed. Two changes:
each of the four rANS lines writes its own substream instead of interleaving
bytes with the others, and every 4096 symbols the encoder records that line's
state, stream offset and context. A decoder can then enter a line at the
nearest checkpoint rather than from its start.

Reading a 16 kB region out of a 64 KiB chunk now expands 27.8% of the symbols
instead of all of them. Density is 2.1331 bits per byte against zstd-3's 2.861,
so the coder stays 25.5% denser than the entropy layer currently in the build,
and the separate substreams and checkpoints together cost 0.72% against
rans1_v2. Verified on 200 random ranges over two corpora — chr1 literals and a
FASTQ — with zero byte differences against a full decode. Checkpoint spacing is
a knob: 1024 brings a 4 kB region down to 6.8% of the work but costs 2.5% of
density; 4096 is the measured optimum.

Build: gcc -O3 -march=native -o rans1_v4 rans1_v4.c -lm
Round-trip: ./rans1_v4 t <file>
Range check: RLEN=16000 ./rans1_v4 r <file> 100

All three are published here so the numbers in the papers have code behind them.
