# ACEAPEX @ID@ — static command-line tool for Linux x86-64

One file, no dependencies (zstd @ZSTD@ is linked in). Runs on any x86-64 CPU with SSE4.2 (2009 and later) and uses
AVX2 where the CPU has it. Archive format ACEPX2: archives written by ACEAPEX 2.1 or later decode with this binary,
and its archives decode with any ACEAPEX 2.1 or later.

    ./aceapex --version
    sha256sum -c SHA256SUMS

## Compress

    ./aceapex c --in genome.fa --out genome.fa.aet --threads 16

Two profiles, chosen when compressing (decompression needs no settings):

    # default: best ratio for decompressing whole files (1 MiB blocks)
    ./aceapex c --in genome.fa --out genome.fa.aet

    # random access by coordinate (16 KiB blocks); also the zstd-free profile a GPU decodes without nvCOMP
    ACEAPEX_BS=16384 LIT_CHUNK=65536 AX_PROFILE=open ./aceapex c --in genome.fa --out genome.open.aet

## Decompress

    ./aceapex d --in genome.fa.aet --out genome.fa --threads 16
    ./aceapex d --in genome.fa.aet -c | md5sum        # to stdout, streamed: ~35 MB of memory at any size

The archive carries an XXH3 hash of the original; decompression checks it and prints "hash OK".

## Regions

    ./aceapex faidx genome.open.aet                         # builds genome.open.aet.fai once (samtools .fai format)
    ./aceapex faidx genome.open.aet chr1:1000000-1005000    # prints what `samtools faidx` prints (60 bases a line)
    ./aceapex faidx genome.open.aet -r regions.txt          # one region per line
    ./aceapex r --in genome.fa.aet --out part.bin --region OFFSET LENGTH    # raw bytes of the original file

## Notes

- The bytes of default-profile archives depend on the zstd version the tool was built with (here @ZSTD@); every
  ACEAPEX build decodes them. The open profile does not use zstd.
- `--threads N` sets the threads; decompression defaults to the physical cores.
