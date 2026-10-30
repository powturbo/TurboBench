# Data provenance for ACEAPEX GPU benchmarks

All numbers reproduce from PUBLIC data identified by accession + md5.

## Primary honest dataset
- **NA12878 / ERR194147** (1000 Genomes, Illumina Platinum)
  - file used: first 1 GB slice, FASTQ
  - md5 (1GB slice): `9af9ffaa...` (full: verify against ENA ERR194147)
  - get: https://www.ebi.ac.uk/ena/browser/view/ERR194147
- **silesia.tar** (Silesia compression corpus)
  - md5: `00e6a383ea2f18f1`
  - get: http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia
- **enwik9** (first 1e9 bytes of English Wikipedia dump)
  - get: http://mattmahoney.net/dc/enwik9.zip
- **Pizza&Chili corpus** (dna/english/proteins, 200MB each)
  - get: http://pizzachili.dcc.uchile.cl/texts.html

## WITHDRAWN (do NOT use — degenerate quality strings)
- NA12878 samples giving ratio 11-14: quality strings collapsed to ~2 symbols.
  Real ERR194147 gives ratio ~3.97. Any ratio 11+ on FASTQ = defective source.
