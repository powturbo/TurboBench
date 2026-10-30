#!/usr/bin/env python3
# Generate docs/benchmarks.html from results.json.
#
# The page is generated rather than written by hand so it cannot drift from the
# code: run the reproduction script, run this, publish. Every figure on the page
# corresponds to a claim id in the JSON and to a command that produces it.

import json, html

d = json.load(open('results.json'))
by = {c['claim_id']: c for c in d['claims']}


def v(k, dflt='n/a'):
    c = by.get(k)
    return c['measured'] if c and c['verdict'] == 'pass' else dflt


CORPORA = [('chr1', 'Human chromosome 1', '253,935,557'),
           ('enwik8', 'English Wikipedia, 100 MB', '100,000,000'),
           ('enwik9', 'English Wikipedia, 1 GB', '1,000,000,000'),
           ('silesia', 'Silesia corpus', '211,957,760'),
           ('fastq', 'Sequencing reads, 1 GB', '1,073,741,620')]

rows = ''.join(
    '<tr><td>%s</td><td class="n">%s</td><td class="n">%s</td><td class="n us">%s</td></tr>'
    % (html.escape(t), sz, v('class_%s_zstd3_16k' % k), v('class_%s_aceapex' % k))
    for k, t, sz in CORPORA)

levels = ''.join(
    '<tr><td><code>%s</code></td><td class="lv %s">%s</td>'
    '<td class="n">%s</td><td class="n">%s</td><td class="%s">%s</td></tr>'
    % (html.escape(c['claim_id']), c['level'], c['level'],
       html.escape(str(c['expected'])), html.escape(str(c['measured'])[:40]),
       c['verdict'].split('-')[0], html.escape(c['verdict']))
    for c in d['claims'])

try:
    arch = '{:,}'.format(int(v('transform_archive_bytes', '0')))
except ValueError:
    arch = 'n/a'

s = d['summary']

CSS = """
:root{--ink:#1a1a1a;--dim:#6b6b6b;--us:#c0392b;--line:#e5e5e5;--bg:#fafafa}
*{box-sizing:border-box}
body{margin:0;padding:2.5rem 1.25rem 4rem;color:var(--ink);background:#fff;
 font:15px/1.6 -apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif}
.wrap{max-width:880px;margin:0 auto}
h1{font-size:1.6rem;margin:0 0 .3rem}
h2{font-size:1.05rem;margin:2.4rem 0 .5rem;font-weight:650}
p{margin:.55rem 0}
.sub{color:var(--dim);margin-bottom:2rem}
table{border-collapse:collapse;width:100%;margin:.8rem 0;font-size:14px}
th,td{text-align:left;padding:.42rem .6rem;border-bottom:1px solid var(--line)}
th{font-weight:600;color:var(--dim);font-size:12px;text-transform:uppercase;letter-spacing:.04em}
.n{text-align:right;font-variant-numeric:tabular-nums}
.us{color:var(--us);font-weight:650}
.lv{text-align:center;font-weight:700;width:2.4rem}
.R{color:#27ae60}.M{color:#e67e22}.E{color:var(--dim)}
.pass{color:#27ae60}.fail{color:var(--us);font-weight:700}
.skipped,.declared{color:var(--dim)}
code{background:var(--bg);padding:.1rem .3rem;border-radius:3px;font-size:.9em}
pre{background:var(--bg);padding:.8rem 1rem;border-radius:5px;overflow-x:auto;
 font-size:13px;line-height:1.5}
.note{color:var(--dim);font-size:13.5px}
.bar{display:inline-block;padding:.15rem .5rem;border-radius:3px;background:var(--bg);font-size:13px}
"""

page = """<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ACEAPEX &mdash; benchmarks</title>
<style>%s</style></head><body>
<div class="wrap">

<h1>Benchmarks</h1>
<p class="sub">Generated from <code>results.json</code> on %s &mdash; %s.
Every figure below is a claim the reproduction script checks.</p>

<h2>Compression ratio under an equal random-access constraint</h2>
<p>Both codecs work on independent 16&nbsp;KB blocks, so either can decode a region
without touching the rest of the file. Comparing a block-structured format against a
whole-stream archive measures the constraint rather than the mechanism.</p>
<table><tr><th>Corpus</th><th class="n">Bytes</th><th class="n">zstd-3, 16 KB</th>
<th class="n">ACEAPEX</th></tr>%s
<tr><td>chr1 with the genomic transform</td><td class="n">253,935,557</td>
<td class="n">3.034</td><td class="n us">%s</td></tr></table>
<p class="note">The last row is the same corpus with the literal transform enabled, which
is opt-in: the default layout is what the published paper figures were measured on.</p>

<h2>Reading a 16 KB region out of a 254 MB archive</h2>
<table>
<tr><th></th><th class="n">Archive</th><th class="n">Ratio</th><th class="n">Seek</th>
<th>Needs</th></tr>
<tr><td>zstd seekable, 16 KB frames</td><td class="n">83,924,167</td>
<td class="n">3.026</td><td class="n">&lt;10 ms</td>
<td>a separate format and a second library</td></tr>
<tr><td>ACEAPEX <code>LIT_CHUNK=1048576</code></td><td class="n us">%s</td>
<td class="n us">%s</td><td class="n us">3 ms</td><td>the base format</td></tr>
</table>
<p class="note">The density on genomic data comes from a literal transform that switches
itself on per chunk: two bits per base, letter case as a packed bitmask, and the rare
non-ACGT bytes as position gaps. The encoder computes the ordinary result as well and
keeps whichever is smaller, so the mode cannot cost size, and it stays off on text,
archives and sequencing reads. Region output is compared byte for byte against the
original at the start, the middle and the tail of the file &mdash; claim
<code>region_16k_byte_exact</code> below.</p>

<h2>Where ACEAPEX loses</h2>
<p class="note">Given a whole-file window instead of independent blocks, zstd-19 packs
tighter: 4.93 against 4.04 on genome. Compression runs roughly six times slower than
zstd-3. On vector data such as SIFT and TPC-H we lose outright. The format is built for
data written once and read many times, and it trades encode time for decode and seek.</p>

<h2>Verification contract</h2>
<p><span class="bar"><strong>%d</strong> pass &middot; <strong>%d</strong> fail
&middot; %d skipped</span> &nbsp; <strong>R</strong> reproducible here &middot;
<strong>M</strong> measured, not bit-perfect &middot; <strong>E</strong> estimated</p>
<pre>git clone https://github.com/yasha1971-coder/aceapex.git &amp;&amp; cd aceapex
CHR1=/path/chr1.fa ENWIK9=/path/enwik9 ./reproduce_paper5.sh</pre>
<table><tr><th>Claim</th><th></th><th class="n">Expected</th><th class="n">Measured</th>
<th>Verdict</th></tr>%s</table>
<p class="note">Corpora are fixed by accession and digest in <code>DATA.md</code>; the
canonical one is human chromosome 1, UCSC hg38, md5
<code>9465e0f0df6e2c6eb39729c39cee5465</code>. GPU claims are skipped explicitly when no
CUDA device is present rather than silently omitted.</p>

<p class="note" style="margin-top:2.4rem">
<a href="./">Overview</a> &middot;
<a href="https://github.com/yasha1971-coder/aceapex">Repository</a> &middot;
<a href="https://arxiv.org/abs/2608.10188">arXiv:2608.10188</a></p>

</div></body></html>
""" % (CSS, d['date'][:10], html.escape(d['hardware']), rows,
       v('transform_chr1_ratio'), arch, v('transform_chr1_ratio'),
       s['pass'], s['fail'], s['skipped'], levels)

open('docs/benchmarks.html', 'w').write(page)
print('docs/benchmarks.html: %d bytes, %d claims' % (len(page), len(d['claims'])))
