# ACEAPEX Encode — measured results

All numbers on NVIDIA H100 80GB HBM3, CUDA 12.4. Every row bit-perfect (FNV +
byte-compare of reconstruction vs original). Throughput = stage1-2 device-resident
(index build + match find); PCIe and host greedy parse excluded.

## Throughput across formats (32MB each, k=4, chain D=8)

| format         | throughput | coverage | bit-perfect | type             |
|----------------|------------|----------|-------------|------------------|
| dna (P&C)      | 3533 MB/s  | 100.0%   | YES         | pure DNA         |
| enwik9         | 3168 MB/s  | 99.3%    | YES         | text             |
| x-ray (Sil.)   | 2968 MB/s  | 93.3%    | YES         | scientific       |
| nci (Sil.)     | 1970 MB/s  | 99.9%    | YES         | chemistry        |
| mozilla (Sil.) | 1328 MB/s  | 85.6%    | YES         | binary           |
| dickens (Sil.) | 1324 MB/s  | 99.7%    | YES         | literature       |
| xml (Sil.)     | 853 MB/s   | 98.8%    | YES         | markup           |
| chr1 (hg38)    | 750 MB/s   | 100.0%   | YES         | genome           |
| english (P&C)  | 362 MB/s   | 99.7%    | YES         | natural English  |
| proteins (P&C) | 307 MB/s   | 99.7%    | YES         | proteins         |

Spread is 11.5x (307-3533 MB/s). "3 GB/s" would be wrong — that is the upper bound
(dna/text/x-ray). On english/proteins it drops to ~0.3 GB/s.

## Why the spread (profiled)

Bottleneck = find stage. dna find = 5.67 ms; english find = 84.1 ms (15x). makekeys and
radix are the same (~0.7 ms, ~2.7 ms). dna/text have long exact repeats (sorted neighbor
gives a long match, jump forward, few iterations). english/proteins have diverse short
matches (scan D neighbors, extend short LCPs, more work per byte). k=6 on proteins did
not help (284 MB/s): bottleneck is match structure, not key width.

## Scale (enwik9, k=4, D=8)

| size | throughput | coverage |
|------|------------|----------|
| 2MB  | 504 MB/s   | 97.2%    |
| 8MB  | 2487 MB/s  | 98.5%    |
| 32MB | 3157 MB/s  | 99.3%    |

Throughput grows with size (fixed makekeys overhead amortizes). Coverage grows 97->99.3%
(global offsets catch distant repeats). radix-sort alone ~4.5 GB/s.

## Distance vs absolute coding (order-0 entropy estimate, 2MB)

| data   | ABS ratio | DIST ratio | gain |
|--------|-----------|------------|------|
| enwik9 | 3.005     | 3.218      | +7%  |
| fastq  | 13.833    | 15.689     | +13% |

## Cost of globality (enwik9 2MB)

Global hash finder: 278421 factors. Shun/Zhao local-window LPF (max offset 731): 240423
factors (denser on 2MB). Global offsets cost ~+50% offset-bytes but catch distant repeats;
coverage gain at scale (97->99.3%) offsets this.

## Honest negatives (things tried that did NOT work)

- word-wise LCP extend (8-byte xor+ctz) + early exit: english +26% but dna DROPPED
  3533->2985, enwik9 3168->2351. Reverted; byte-wise is better on average.
- delta-coding consecutive absolute offsets: only 3.5% (offsets scattered). The working
  transform is i-src (distance from position), not delta between sources.
- k=12 on real fastq: coverage dropped 97->92%. k=8 optimal for real fastq (k=12 held
  only for synthetic pure-ACGT DNA).
