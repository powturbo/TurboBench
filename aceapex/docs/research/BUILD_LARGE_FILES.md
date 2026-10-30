# Encoding and decoding large files with ACEAPEX

Two settings decide whether a multi-gigabyte file goes through or dies with an
out-of-memory kill. Both are ours, not the format's.

## 1. Do not force a small block size on large inputs

ACEAPEX_BS is a benchmark convention for small-corpus comparisons. On a large
file it is harmful: at 16 KB a 53.9 GB input becomes 3.3 million blocks, each
holding four allocations alive until concatenation. Without the override,
compute_block_size() picks 1 MB - 64x fewer allocations.

Measured on this machine (125 GB RAM):

| input | ACEAPEX_BS=16384 | default block size |
|-------|------------------|--------------------|
| 40 GiB | killed (OOM) | completed, RSS 117 GiB |
| 53.9 GB | killed | completed, RSS 118 GiB |

## 2. Build the decoder without the analysis instrumentation

aceapex_depth.cpp doubles as the depth-analysis bench used to measure MaxLevel
for the papers. That instrumentation is fatal at scale:

- g_record = true appends every token to a vector (24 bytes each; ~4.4 billion
  tokens on a 53.9 GB output is over 100 GB)
- the depth block builds tok_of, one uint32 per output byte (215 GB for a
  53.9 GB output)
- streams.bin is dumped unconditionally
- parallel_decode is called with a hardcoded 1 thread

In your build copy:

```
g_record=true      ->  g_record=false
hdr.block_size,1)  ->  hdr.block_size,threads)
```

and delete the streams.bin dump block. Resident memory drops from 118 to
87.9 GiB on the 53.9 GB case, and decode runs on all threads.

## Verified results

Full reference file, ENA ERR174310_1.fastq, complete and untruncated:
53,868,884,409 bytes, gz md5 b6099227bfb6d15c97395975eeeccd28,
207,579,467 records. Completeness confirmed by the last record id equalling
the archive's record count.

```
encode  228.78 MB/s (235.5 s)   .aet 19,136,891,145   ratio 2.8149
decode  useful work 14.3 s -> 3.77 GB/s
        match phase 5.6 s = 9.6 GB/s, entropy 8.7 s = 6.2 GB/s, in parallel
verify  byte-compare identical, 53,868,884,409 bytes
```

Silesia, 12 files, 211,938,580 bytes, tarred to 211,957,760:

```
encode  222.89 MB/s (0.951 s)   .aet 69,042,786   ratio 3.0699
decode  1372.12 MB/s algorithmic / 700.31 MB/s wall
        entropy 0.033 s, match 0.030 s - near-equal here, unlike FASTQ
        where entropy costs twice the match phase
verify  byte-compare identical
```

## Reading the wall-clock numbers honestly

This machine's array reads at 2.2 GB/s and writes at 819 MB/s. For the 53.9 GB
decode that is 8.7 s reading the archive and 65.8 s writing the output, against
14.3 s of actual decoding - storage is 72% of the wall time. Quoted "MB/s wall"
figures therefore measure the disk more than the codec.
