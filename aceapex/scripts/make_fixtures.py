#!/usr/bin/env python3
"""Conformance fixtures for the ACEPX2 format (docs/FORMAT_ACEPX2.md §7).

Inputs are generated from fixed seeds, so only the archive and the SHA-256 of the input
are stored; the input itself is reproducible with --regen. Each case pins the encoder
environment so the archive exercises one layout: tiny inputs, block boundaries, the
DNA pack with lowercase and N runs, plain (mode 0) tagged chunks, the original 4-part
literal layout, small and large blocks, and the pre-chunk-field LEGACY layout written
by aceapex_depth (decoders need FSE_CHUNK for that one; the manifest says so).

Usage: scripts/make_fixtures.py [--bin ./aceapex] [--depth /path/aceapex_depth] [--regen]
Writes verify/fixtures/conf/<name>.aet and verify/fixtures/conf/manifest.tsv:
    name <TAB> orig_size <TAB> sha256(input) <TAB> decode_env ('-' or VAR=value)
"""
import argparse, hashlib, os, random, subprocess, sys, tempfile

def gen(name, n, seed):
    r = random.Random(seed)
    if name.startswith('rand'):
        return bytes(r.getrandbits(8) for _ in range(n))
    if name.startswith('dna'):
        out = bytearray()
        while len(out) < n:
            k = r.choice([1, 1, 1, 50, 300, 2000])
            kind = r.random()
            if kind < 0.80: out += bytes(r.choice(b'ACGT') for _ in range(k))
            elif kind < 0.93: out += bytes(r.choice(b'acgt') for _ in range(k))
            elif kind < 0.99: out += b'N' * k
            else: out += bytes(r.choice(b'RYKMSWn') for _ in range(k))
        return bytes(out[:n])
    if name.startswith('alln'):
        return b'N' * n
    if name.startswith('text'):
        words = [''.join(r.choice('abcdefghijklmnopqrstuvwxyz') for _ in range(r.randint(2, 9))) for _ in range(400)]
        out = []
        while sum(len(w) + 1 for w in out) < n:
            out.append(r.choice(words))
        return (' '.join(out)).encode()[:n]
    if name.startswith('zeros'):
        return b'\0' * n
    raise ValueError(name)

# name, size, seed, encoder env (profile), decode env
CASES = [
    ('rand_1B',        1,       1, 'ACEAPEX_BS=16384 LIT_CHUNK=65536 FSE_CHUNK=4096', '-'),
    ('rand_7B',        7,       2, 'ACEAPEX_BS=16384 LIT_CHUNK=65536 FSE_CHUNK=4096', '-'),
    ('text_767B',      767,     3, 'ACEAPEX_BS=16384 LIT_CHUNK=65536 FSE_CHUNK=4096', '-'),
    ('dna_4095B',      4095,    4, 'ACEAPEX_BS=4096 LIT_CHUNK=65536 FSE_CHUNK=4096',  '-'),
    ('dna_4096B',      4096,    5, 'ACEAPEX_BS=4096 LIT_CHUNK=65536 FSE_CHUNK=4096',  '-'),
    ('dna_4097B',      4097,    6, 'ACEAPEX_BS=4096 LIT_CHUNK=65536 FSE_CHUNK=4096',  '-'),
    ('alln_1MiB',      1 << 20, 7, 'ACEAPEX_BS=16384 LIT_CHUNK=65536 FSE_CHUNK=4096', '-'),
    ('dna_mixed_300K', 300000,  8, 'ACEAPEX_BS=16384 LIT_CHUNK=65536 FSE_CHUNK=4096', '-'),
    ('text_200K',      200000,  9, 'ACEAPEX_BS=16384 LIT_CHUNK=65536 FSE_CHUNK=4096', '-'),
    ('text_legacy4',   300000, 10, 'LIT_CHUNK=0',                                     '-'),
    ('rand_bigfse',    400000, 11, 'ACEAPEX_BS=65536 LIT_CHUNK=1048576 FSE_CHUNK=1048576', '-'),
    ('dna_default',    2 << 20, 12, '',                                              '-'),
    ('zeros_1MiB',     1 << 20, 13, 'ACEAPEX_BS=16384 LIT_CHUNK=65536 FSE_CHUNK=4096', '-'),
    ('dna_depth_legacy', 300000, 14, 'DEPTH FSE_CHUNK=4096',                          'FSE_CHUNK=4096'),
    # rANS token profile (ADR-018): chunk entry bit 62, 64 KiB token chunks by default
    ('dna_rans_2MiB',  2 << 20, 15, 'AX_TOK=rans',                                   '-'),
    ('dna_rans_4k',    300000, 16, 'ACEAPEX_BS=16384 LIT_CHUNK=65536 FSE_CHUNK=4096 AX_TOK=rans', '-'),
    ('text_rans_200K', 200000, 17, 'AX_TOK=rans',                                    '-'),
    # open profile (ADR-019): literal chunks mode 2 (open DNA pack) / mode 3 (open plain), no zstd
    ('dna_open_2MiB',  2 << 20, 18, 'AX_PROFILE=open',                               '-'),
    ('dna_open_mixed_300K', 300000, 19, 'ACEAPEX_BS=16384 FSE_CHUNK=4096 AX_PROFILE=open', '-'),
    ('dna_open_4097B', 4097,   20, 'ACEAPEX_BS=4096 AX_PROFILE=open',                '-'),
    ('dna_openlit_300K', 300000, 21, 'ACEAPEX_BS=16384 AX_LIT=open',                 '-'),
    ('text_open_200K', 200000, 22, 'AX_PROFILE=open',                                '-'),
    # FAST profile (2.4.0, header version 3, docs/FORMAT_FAST.md): tokens without an entropy stage. A token in the
    # encoder column (no '=') is a CLI argument. Cases: the headline level, a long literal run (random: ll = 63 + extension),
    # long matches (zeros: mc = 63 + extension, one token per 1 MiB block), the optimal parse across a block boundary, tiny.
    ('text_fast_200K',  200000,  23, '--level 14',                                      '-'),
    ('rand_fast_70K',   70000,   24, '--level 11',                                      '-'),
    ('zeros_fast_1500K', 1500000, 25, '--level 11',                                     '-'),
    ('dna_fast_opt_2MiB', 2097152 + 1, 26, '--level 15',                                '-'),
    ('rand_fast_7B',    7,       27, '--level 18',                                      '-'),
]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--bin', default='./aceapex')
    ap.add_argument('--depth', default='')
    ap.add_argument('--out', default='verify/fixtures/conf')
    ap.add_argument('--regen', action='store_true', help='only regenerate inputs into /tmp/conf_inputs')
    ap.add_argument('--only', default='', help='comma list: build these cases and APPEND them to the manifest')
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    rows = []
    only = set(x for x in a.only.split(',') if x)
    for name, n, seed, env, denv in CASES:
        if only and name not in only: continue
        data = gen(name, n, seed); sha = hashlib.sha256(data).hexdigest()
        if a.regen:
            os.makedirs('/tmp/conf_inputs', exist_ok=True); open(f'/tmp/conf_inputs/{name}', 'wb').write(data); continue
        with tempfile.TemporaryDirectory() as t:
            src = os.path.join(t, 'in'); open(src, 'wb').write(data)
            dst = os.path.join(a.out, name + '.aet')
            if env.startswith('DEPTH'):
                if not a.depth: print(f'skip {name}: --depth not given'); continue
                b = a.depth; env = env[len('DEPTH '):]
            else:
                b = a.bin
            toks = env.split(); envs = [t for t in toks if '=' in t]; args = [t for t in toks if '=' not in t]
            cmd = ['env', '-i', 'PATH=' + os.environ['PATH']] + envs + [b, 'c', '--in', src, '--out', dst, '--threads', '2'] + args
            r = subprocess.run(cmd, capture_output=True, text=True)
            if r.returncode != 0 or not os.path.exists(dst):
                print(f'FAIL {name}: {r.stderr[-300:]}'); sys.exit(1)
            rows.append((name, len(data), sha, denv)); print(f'{name}: {len(data)} B -> {os.path.getsize(dst)} B')   # the text generator can stop one byte short of n
    if a.regen: print('inputs in /tmp/conf_inputs'); return
    mf = os.path.join(a.out, 'manifest.tsv')
    keep = []
    if only and os.path.exists(mf):              # --only: replace those rows, keep the rest (idempotent)
        keep = [l for l in open(mf) if l.split('\t')[0] not in only]
    with open(mf, 'w') as f:
        f.writelines(keep)
        for name, n, sha, denv in rows: f.write(f'{name}\t{n}\t{sha}\t{denv}\n')
    print(f'{len(rows)} fixtures, manifest written')

if __name__ == '__main__':
    main()
