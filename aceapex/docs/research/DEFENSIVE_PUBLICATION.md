# Defensive Publication: Independent Block Parallel LZ77 Decoding

**Date:** 2026-05-27  
**Author:** Yakiv Shavidze (GitHub: yasha1971-coder)  
**Repository:** https://github.com/yasha1971-coder/aceapex

## Purpose
This document establishes prior art for the techniques described below,
preventing future patent claims on this method.

## Core Method
Each compressed block stores absolute offsets from block_start.
Blocks are fully independent — zero cross-block dependencies.
Decode is parallelizable across arbitrary thread counts.

match = (abs_src, length)
src = block_start + abs_src
D1_safe = (src >= block_start) && (src + length <= block_end)

## Citation
@misc{aceapex2026defensive,
  title = {Independent Block Parallel LZ77 Decoding with Absolute Offsets},
  author = {Yakiv Shavidze},
  year = {2026},
  month = {05},
  howpublished = {GitHub repository, yasha1971-coder/aceapex},
  url = {https://github.com/yasha1971-coder/aceapex}
}

## DOI (Zenodo)https://doi.org/10.5281/zenodo.20440965

Published: May 29, 2026
Archived: Software Heritage swh:1:dir:33b1a9c7c5f86b07a57c7d546ca78175c9c234bf
