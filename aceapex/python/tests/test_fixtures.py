# The same fixtures that judge the C decoder, through the Python layer.
import hashlib, os, random, pytest, aceapex
ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
FX = os.path.join(ROOT, "verify", "fixtures")

def manifest():
    with open(os.path.join(FX, "conf", "manifest.tsv")) as f:
        for line in f:
            name, n, sha, denv = line.rstrip("\n").split("\t")
            yield name, int(n), sha, denv

@pytest.mark.parametrize("name,n,sha,denv", list(manifest()))
def test_conformance_full(name, n, sha, denv, monkeypatch):
    if denv != "-":
        k, v = denv.split("="); monkeypatch.setenv(k, v)
    a = aceapex.open(os.path.join(FX, "conf", name + ".aet"))
    assert a.size == n
    out = a.decompress()
    assert len(out) == n and hashlib.sha256(out).hexdigest() == sha

def test_empty():
    a = aceapex.open(os.path.join(FX, "empty.aet"))
    assert a.size == 0 and a.decompress() == b"" and a.read(0, 0) == b""
    with pytest.raises(aceapex.DecodeError): a.read(0, 1)

def test_regions_and_ranges():
    exp = open(os.path.join(FX, "chr1_4MiB.sha256")).read().strip()
    a = aceapex.open(os.path.join(FX, "chr1_4MiB.zstd-1.4.8.aet"))
    full = a.decompress(); assert hashlib.sha256(full).hexdigest() == exp
    r = random.Random(7); spans = []
    for _ in range(200):
        l = r.choice([1, 17, 4096, 16384, 70000]); spans.append((r.randrange(0, len(full) - l + 1), l))
    got = a.ranges(spans)
    assert all(g == full[o:o + l] for g, (o, l) in zip(got, spans))
    o, l = spans[0]; assert a.read(o, l) == full[o:o + l]
    assert a.read(len(full) - 1, 1) == full[-1:]

def test_bytes_input_and_corruption():
    raw = open(os.path.join(FX, "chr1_4MiB.zstd-1.5.5.aet"), "rb").read()
    a = aceapex.open(raw); assert a.size == 4194304
    full = a.decompress()
    # a flipped byte inside a zstd frame either fails to decode or yields different bytes;
    # it can never come back as the original (a flipped size in the block table can be
    # harmless: the block decoder only reads what the command stream asks for)
    bad = bytearray(raw); bad[len(raw) // 2] ^= 0xFF
    try:
        assert aceapex.open(bytes(bad)).decompress() != full
    except aceapex.DecodeError:
        pass
    with pytest.raises(aceapex.DecodeError): aceapex.open(b"not an archive at all")
