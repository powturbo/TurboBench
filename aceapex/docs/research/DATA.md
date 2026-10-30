# DATA — Provenance for ACEAPEX Reproduction (accession + md5)

All benchmark numbers in ACEAPEX papers use these exact inputs. A reviewer can obtain
them independently by accession and verify by md5. **Measure only from these** (honest,
non-degenerate). Defective/degenerate inputs produce misleading ratios (see note at bottom).

## Primary genomics input (Papers 2-5 throughput, seek)
```
File:      NA12878 / ERR194147 (first 1,073,741,620 bytes = 1 GiB prefix)
Accession: ENA ERR194147  (https://www.ebi.ac.uk/ena/browser/view/ERR194147)
md5:       9af9ffaa0e15dba938408a711740e101
Quality:   Phred qual38 (real quality distribution, NOT degenerate)
Use:       D1-dense throughput, seek O(1), 50GB scale, min_match_len lever; flat-latency control
```
Obtain the exact 1 GiB prefix:
```bash
head -c 1073741620 ERR194147.fastq > NA12878_REAL.fastq
md5sum NA12878_REAL.fastq   # must be 9af9ffaa0e15dba938408a711740e101
```

## Genome assembly input (Paper 5 bounded-latency: latency spikes)
```
File:      chr1.fa (human chromosome 1, UCSC hg38 assembly)
Source:    https://hgdownload.soe.ucsc.edu/goldenPath/hg38/chromosomes/chr1.fa.gz
           (gunzip after download)
Size:      ~254 MB (253,935,557 bytes uncompressed)
Use:       Paper 5 bounded-latency — per-block seek-latency spikes on k-mer-dense
           soft-masked repeat regions (cluster ~blocks 7590-7770); depth-cap equalization.
Note:      UCSC hg38 uses soft-masking (lowercase = RepeatMasker/TRF regions). Latency
           spikes correlate with LOW k-mer uniqueness in these regions, NOT with soft-mask
           fraction per se (verified: 81% soft-mask block = normal latency, 98% = spike;
           driver is k-mer duplication -> long LZ-chains -> high local MaxLevel -> parse-wall).
```
Verify:
```bash
md5sum chr1.fa   # pin to hg38 goldenPath chromosomes/chr1.fa.gz, compute after gunzip
```

## Text input (Paper 5 min_match data-type curve)
```
File:      enwik9 (first 1e9 bytes of English Wikipedia dump)
Source:    http://mattmahoney.net/dc/enwik9.zip
md5:       e206c3450ac99950df65bf70ef61a12d
Use:       parse-bound data point (min_match_len lever +2.7%)
```

## Mixed-data input (Paper 5 parse-bound extreme)
```
File:      silesia.tar (Silesia corpus, 12 files, tarred)
Source:    http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia
Use:       strong parse-bound data point (min_match_len lever +20.2%)
```

## Repetitive / protein inputs (seek universality + Paper 5 spikes)
```
pizzachili dna.200MB / english.200MB / proteins.200MB
Source:    http://pizzachili.dcc.uchile.cl/texts.html
Use:       seek O(1) across data types (bit-perfect); proteins.200MB carries latency
           spikes (~1%, block ~450 uniq-8mers 27% -> 854us) = second file confirming
           k-mer-duplication mechanism (N=2 with chr1); dna200 is flat (0 spikes, control).
```

## Reproduction recipes
- `BUILD_D1_DENSE.md` — exact build of D1-dense throughput pipeline.
- `reproduce_paper5.sh` — min_match_len data-dependent throughput lever sweep.
- `reproduce_bounded_latency.sh` — Paper 5 bounded-latency: latency-spike scan (chr1 +
  proteins spike, fastq + dna200 flat controls), k-mer-uniqueness cause, depth-cap
  equalization (spike 462->185us, median preserved, +1.28% size). Self-contained
  (installs glog/gflags, builds v7ra from e2e_seek.cu).
- `storage_kpi.sh` — O(1) seek + parallel-decode KPIs.

Judge of correctness everywhere = bit-perfect round-trip (FNV hash printed by the GPU
binaries MATCHES original). Per REPRODUCIBILITY_LAW: a stranger reproduces from this
public repo + the accessions above alone.

## Note on excluded (degenerate) data
Earlier ACEAPEX numbers that appeared anomalously high (e.g. FASTQ ratio 11-14) came from
a DEFECTIVE NA12878 file with quality collapsed to ~2 symbols, which is NOT representative.
All published numbers use the honest ERR194147 (md5 9af9ffaa) above.
