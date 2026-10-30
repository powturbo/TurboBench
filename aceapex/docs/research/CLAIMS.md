# Claims

> ACEAPEX is an ongoing engineering investigation. Earlier papers and results document specific stages of the project and may be superseded. This file and results.json carry the current claims and their reproduction status.

Each answer below is a measurement with a fixed scope and a way to check it.
Levels: **R** reproducible here with a command, an expected value and a tolerance;
**M** measured but not bit-perfect, with the reason stated; **E** estimated from
measured quantities. Machine-readable form: [`results.json`](results.json).

---

### Is there a lossless format that supports random access into GPU-resident compressed data?

Yes. Match search is confined to the current block, so a block never
references outside itself — that is what makes it independently decodable.
Within a block, an explicit reference is additionally redirected to its earlier
originating position when the substitution validates byte for byte, which
shortens dependency chains; repeat-coded references are left as they are. Each block is
therefore self-contained and any region decodes without touching the rest of
the file. (On the wire the offset is encoded as a distance from the resolved
origin; the resolution, not the encoding, is what removes the dependency.) On a 50,000,000,000-byte archive of
3,051,758 blocks, decoding one 16 KB tile takes **292–387 µs regardless of
position** — start, 16 GB in, 33 GB in, and at the end — with four distinct FNV
values and no trend with position. Widening the request scales far better than
linearly: one block in 0.339 ms, one thousand blocks (16 MB) in 0.423 ms.

*Level M — the 50 GB original is not on disk, so the FNV values have nothing to
compare against. Position-invariance is measured; bit-perfect correctness is
shown on smaller corpora. Paper 2, Paper 3, Paper 5 §8.*

---

### How does a region read compare to zstd's seekable format?

Both solve the same problem on CPU: pull a slice out of a compressed file without
touching the rest. Same file, same region, same host.

| | archive | ratio | 16 KB seek | needs |
|---|---|---|---|---|
| plain FASTA + fai | 253,935,580 | 1.000 | 0.007 ms | nothing |
| bgzip + fai + gzi | 76,578,367 | 3.316 | 0.140 ms | two index files |
| **ACEAPEX + fai** | **69,309,759** | **3.708** | **0.081 ms** | one index file |

Against bgzip with a fai index, which is the standard for compressed reference
genomes and answers exactly this question: **11.8% denser, 1.73x faster on median
seek, 2.03x faster at the 99th percentile** (0.142 ms against 0.288), and thirteen
times faster to compress (1.2 s against 15.3 s). Output is verified byte for byte
against htslib.

Region latency is linear in decode amplification — the bytes unpacked divided by the
block size — at about 12 µs per unit, measured across nine configurations spanning
4.75x to 40x. The entropy chunk size barely affects ratio, 0.17% between 8 KB and
131 KB, so it can be made small almost for free; the real trade lives in the literal
chunk size, where 64 KB gives 3.708 and 256 KB gives 3.769. The density comes from a literal transform that
switches itself on per chunk when the data is genomic: two bits per base, the letter
case as a packed bitmask, and the rare non-ACGT bytes as position gaps. The encoder
computes the ordinary result as well and keeps whichever is smaller, so the mode can
never cost size, and it stays off on text, archives and sequencing reads where the
non-ACGT share runs 74 to 92 percent. Widening the request costs almost nothing: one
block 3 ms, ten blocks 3 ms, a hundred blocks — 1.6 MB — 5 ms. The phase breakdown
for a single block is read 0.000 s, entropy 0.001–0.005 s, match layer 0.000 s: the
archive is mapped rather than read, so the kernel faults in only the pages the range
touches.

Verified byte for byte against the original at the start, the middle and the tail of
the file.

```bash
aceapex c --in chr1.fa --out chr1.aet          # LIT_CHUNK=1048576 for region reads
aceapex r --in chr1.aet --out slice.bin --region 126959616 16384
```

*Level R. The chunked literal scheme is opt-in: without `LIT_CHUNK` the encoder uses
the previous layout, which is what the published figures were measured on. Chunk size
is data-dependent — genome improves at 1 MB, text prefers 16 MB.*

---

### What does random access cost in compression ratio?

The cost is blockwise encoding, not the match layer. Whole-stream zstd-3 against
zstd-3 over independent 16 KB chunks differs by **6.68% on genome and 24.19% on
text**. The match layer returns about three quarters of that: on chr1, 3.034
(blockwise zstd-3) against 3.181 (ACEAPEX).

*Level R. Paper 5 §9.*

---

### How does ACEAPEX compare to zstd, lz4 and brotli?

Only once the constraint is equalised. Under independent 16 KB blocks, where
every codec is equally seekable:

| corpus | lz4 | zstd-3 | brotli-9 | zstd-19 | ACEAPEX |
|---|---|---|---|---|---|
| chr1 | 1.786 | 3.034 | 3.287 | **3.475** | 3.181 |
| enwik8 | 1.595 | 2.334 | 2.564 | 2.488 | **2.638** |
| enwik9 | 1.750 | 2.567 | 2.824 | 2.736 | **2.981** |
| silesia | 1.907 | 2.675 | 2.882 | 2.936 | **3.005** |
| FASTQ 1 GB | 2.324 | 3.629 | 3.910 | **4.029** | 3.965 |

First on three of five against zstd-19, ahead of the speed-comparable zstd-3
everywhere by 4.9% to 16.1%. Given a whole-file window instead, zstd-19 packs
tighter — ratio is not our axis.

*Level R. Paper 5 §9, Table 6.*

---

### What limits GPU LZ77 decompression throughput?

Two things, and neither is the one usually assumed.

**Parse, not copy.** Across four corpora, parse holds **63.7–71.5%** of
device-resident decode time. Full-pipe decode of human chromosome 1 runs at
71.0 GB/s, bit-perfect.

**Write granularity.** Copying 61,089,878 logical bytes moves 1,392,400,000
bytes of cache-line traffic at 128 B granularity — **4.4% bus efficiency**. A
coalesced write of the same 61 MB takes 0.027 ms against our 1.056 ms, a gap of
39×. The cause is that the median match is 7 bytes and 99.4% of tokens are
shorter than a warp.

*Level R. Paper 5 §3 and §7; the granularity mechanism is Paper 4.*

---

### Does bounding back-reference chain depth reduce decode latency?

Almost not at all, and we measured it three ways rather than assuming.

A two-pass encoder holds every block's chain depth at or below a chosen L,
bit-perfect on a full corpus, at a compression cost within ±0.006%. On the
wavefront decoder, cutting 51 waves to 17 gives **−2.63%**. On per-block seek
with equal token mass, **−2.8%** for the configuration that actually reduces
depth, while forcing an equal number of leaf tokens or an equal number outside
the cluster changes nothing. On the dense full pipe the effect is below the 6%
bench noise.

For the file's own latency spike the answer is provably zero: comparing all four
streams of every one of 15,499 blocks, baseline against L=32, exactly 16 blocks
differ, and the 181-block spike cluster contains none of them. A cap that alters
no byte in a region cannot alter that region's decode time.

*Level R. Paper 5 §4.*

---

### Can self-overlapping matches be decoded in parallel?

Yes. A self-overlapping match, where the source range intersects the
destination, is treated by every implementation we know as inherently serial. It
is not: writing the fill as `out[dst+k] = out[src + k mod dist]` places the
source entirely outside the written range, so the threads are independent. The
period is known to the encoder from the start — it is in the format.

Match-layer speedup, all bit-perfect: **proteins 8.42×, chr1 4.53×, silesia
3.00×, enwik9 2.75×**.

*Level R — match layer only, entropy outside the timer. Paper 5 §5.*

---

### How much does removing the last sequential element of the parse cost?

**0.540% of compression ratio.** Decomposing all 10,566,105 commands of chr1
shows that a four-entry distance history accounts for 5.45% of commands, varint
lengths for 0.61%, and the remaining 93.94% is parallelisable by prefix sum.
Suppressing the repeat codes — the distance goes out as a varint, the match
itself is preserved, the format is unchanged and the decoder is the same — drops
the chained share from 5.46% to 0.02% and grows the dependency-free parse run
from 4 commands to 706 at the median.

For comparison, Gompresso pays up to 19% in ratio to eliminate a match-layer
dependency; the element removed here is a parse-layer one, and the two are not
directly comparable.

*Level R. Paper 5 §6.*

---

### What did you try that did not work?

Ten hypotheses these measurements refuted are listed in Paper 5 §10 with the
number that killed each one, including one methodological error of our own:
comparing a blockwise format against whole-stream zstd without equalising the
constraint first.

---

## Checking any of this

```bash
git clone https://github.com/yasha1971-coder/aceapex.git && cd aceapex
CHR1=/path/chr1.fa ENWIK9=/path/enwik9 ./reproduce_paper5.sh
```

A fresh clone of tag `paper5-v1` on a CPU-only host gives 17 pass, 0 fail,
6 skipped — three GPU claims need a CUDA device, three are the declared M and E
entries. Corpora are fixed by accession and digest in [`DATA.md`](DATA.md);
the canonical one is human chromosome 1, UCSC hg38, md5
`9465e0f0df6e2c6eb39729c39cee5465`, 253,935,557 bytes.
