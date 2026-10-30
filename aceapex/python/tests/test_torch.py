# aceapex.torch: windows by coordinate. numpy path always; tensor path when torch is installed.
import hashlib, os, pytest
import aceapex, aceapex.torch as at
FX = os.path.join(os.path.dirname(__file__), "..", "..", "verify", "fixtures")
A = os.path.join(FX, "chr1_4MiB.zstd-1.4.8.aet")

def test_random_windows_numpy():
    full = aceapex.open(A).decompress()
    ds = at.RandomWindows(A, length=4096, count=64, seed=3)
    assert len(ds) == 64
    x = ds.numpy_batch(range(64))
    assert x.shape == (64, 4096) and x.dtype.name == "uint8"
    for i, (o, l) in enumerate(ds.spans): assert bytes(x[i]) == full[o:o + l]

def test_tiled_covers_everything():
    full = aceapex.open(A).decompress()
    ds = at.TiledWindows(A, length=65536)
    assert sum(l for _, l in ds.spans) == len(full)
    x = ds.numpy_batch(range(len(ds)))
    assert b"".join(bytes(r) for r in x) == full

def test_pickle_reopens_handle():
    import pickle
    ds = at.RandomWindows(A, length=1000, count=3, seed=1); ds.numpy_batch([0])
    ds2 = pickle.loads(pickle.dumps(ds))
    assert ds2._a is None and ds2.numpy_batch([1]).shape == (1, 1000)

def test_torch_tensors():
    torch = pytest.importorskip("torch")
    full = aceapex.open(A).decompress()
    ds = at.RandomWindows(A, length=8192, count=16, seed=7)
    t = ds[0]; o, l = ds.spans[0]
    assert t.dtype == torch.uint8 and t.shape == (8192,) and bytes(t.numpy()) == full[o:o + l]
    b = ds.batch(range(16)); assert b.shape == (16, 8192)
    dl = torch.utils.data.DataLoader(ds, batch_size=4, num_workers=2)
    n = sum(x.shape[0] for x in dl); assert n == 16
