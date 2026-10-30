# Bounded Critical-Path Parsing (BCP)

**A compression principle, not a codec.** ACEAPEX is a reference implementation of BCP;
they are not synonyms. The principle is stated so it benefits any format (nvCOMP-class,
OpenZL, future designs), because a principle that helps only its author's code is not
adopted.

Russian: «парсинг с ограниченным критическим путём». Short form: depth-capped LZ.

---

## The principle (one sentence)

> If an encoder admits a match only when `level(src) + 1 ≤ L`, then the dependency DAG
> of the decompressed stream has `MaxLevel ≤ L`, the parallel critical path of decoding
> is bounded by `O(L)`, and compressed size pays a measurable price `C_L(S) − C_∞(S)`.

Where `level` is the dependency depth of an output position: a literal has level 0; a
match referencing sources of maximum level *m* has level *m+1*. Capping the admissible
`level(src)+1` at `L` bounds the whole DAG by construction.

## Status of each claim (honest vocabulary)

| Claim | Statement | Status |
|-------|-----------|--------|
| **T1: M≤L** | `level(src)+1 ≤ L ⇒ MaxLevel ≤ L` (induction; literal=0, match=src+1) | **PROVEN** (theorem; prototype confirmed exactly L) |
| **T2: Span** | parallel decode critical path = `O(L)` (wavefront, work–span model) | TO FORMALIZE |
| **T3: Access** | `AccessCost(i) = O(L + B_i)`, independent of position *i* | TO FORMALIZE |
| **T4: min C_L** | optimal L-bounded parse: poly-time, or NP-hard, or bounded approximation | OPEN PROBLEM |
| **Price** | `C_L(S) − C_∞(S)` measured | MEASURED partially (chr1 −1.3%, nci −48% @L=64) |

T1 is deliberately **not sold as a breakthrough** — a reviewer will correctly call it a
consequence of the definition. The scientific weight is in T4 (the optimization) and in
the measure below.

## The exported object: z_L(S)

The vector into existing theory is a **measure**, not a codec number:

- **z_L(S)** — the number of phrases of an optimal L-bounded LZ77 parse of string S.
- **C_L(S)** — its bit cost. The family interpolates: `z_∞(S) = z(S)` (ordinary LZ77).

This joins the existing **zoo of repetitiveness measures** (δ, γ, b, r, g, z, z_end, z_no)
as a new member parameterized by dependency depth. Conjectured relations (all UNPROVEN):
monotonicity `z_{L+1} ≤ z_L` and `z ≤ z_L`; separation `∃S: z_L = ω(z)` at fixed L
(the nci −48% price is an empirical precursor); a "mirror" of Ganardi–Jeż–Lohrey
(balancing SLPs to depth `O(log n)` is nearly free) asking whether depth `O(log n)` is
nearly free for LZ too.

The lesson generalized from 7-zip (whoever defines the *tool* defines the field) lifted
to mathematics: **whoever defines the *measure* defines the axis.** z_L is designed as a
member of a living community's zoo, not as one codec's private quantity.

## Intellectual-property declaration

- The **principle and its theorems are public domain / commons.** No one should patent
  depth-capped parsing, including the author.
- Prior art (defensive publication): arXiv **2606.04268 / 2606.18900 / 2606.24531 /
  2607.18541**. Publication dates are the shield.
- **Code is MIT** (github.com/yasha1971-coder/aceapex). **Principle is nobody's.**
- History's two lessons: arithmetic coding was delayed ~two decades by patents; ANS is
  free because Duda actively opposed patenting it — and freedom is the direct cause of
  its ubiquity. BCP follows ANS.

## What must never be claimed

- **Not** "ACEAPEX Law" in papers. Self-named laws are not adopted; the field names them
  (Amdahl did not call it Amdahl's law). In papers: "theorem / measure / constrained
  parsing."
- **Not** "the theorem guarantees P99." The theorem gives *depth*; P99 is a measurement
  on specific hardware. Provable bound on dependency depth + empirically characterized
  physical latency profile — the two are separate.
- **Not** a principle that serves only ACEAPEX. BCP must be useful to competitors or it
  is not a principle.
- The nci −48% price on repetitive data is **not hidden** — it is a future separation
  theorem and an honest domain boundary, not an embarrassment.

## Readiness for "named principle" (not yet — vision only)

Three conditions, none yet met:
- **U1:** theorem is general and non-trivial (not a corollary of one encoder's construction).
- **U2:** the effect reproduces outside this encoder and this GPU.
- **U3:** others use the principle itself without using ACEAPEX code.

Target sentence of the epoch (forbidden in papers until a non-author writes it):
> "Shannon — the limit; LZ77 — the dictionary; ANS — fast entropy; BCP — compressed data
> as addressable parallel memory with a proven dependency-depth bound."

Until then this line lives in vision documents only. Export of the principle is gated on
G1 (replication of the mechanism: full-corpus regression + interventional A/B), because a
principle without a replicated effect is marketing.
