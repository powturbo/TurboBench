# ACEPX4 Interleaved Stream Design

## Core Constraint
stream(p) = p mod S
Match allowed: (dist % S == 0) OR (src < interleave_start)

## Key Properties
- Decoder independence: stream i never reads bytes from stream j after interleave_start
- No synchronization barriers needed
- Encoder: adaptive fallback to best allowed candidate from shortlist

## Status
HYPOTHESIS — not validated
Critical unknown: fraction of matches with dist%4==0 on target data

## Related Work
- Rapidgzip 2023: parallel gzip block-level, intra-block deps remain
- Cross-stream coding Liu 2009: synchronization, NOT prohibition
- GPU stream LZ77: accelerates serial deps, does not change match policy

## Experimental Plan
1. Measure % matches with dist%S==0 on nci/silesia → ratio estimate
2. If >40% viable: prototype encoder constraint
3. Measure branch miss reduction in dec_worker
4. Measure FSE as potential new bottleneck
5. Validate on 4344P and 9575F
