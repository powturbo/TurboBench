# aceapex (Python)

Read ACEAPEX `.aet` archives from Python: the whole file, one region by original byte
offset, or a batch of ranges decoded block-by-block once. Wraps `c/aceapex_decode.c`
(C99, libzstd only) through ctypes; no C++ and no threads. Encoding is not included -
use the `aceapex` CLI.

    pip install .            # needs a C compiler and libzstd-dev
    python -c "import aceapex; a = aceapex.open('chr1.aet'); print(a.size, a.read(1_000_000, 16384)[:60])"

    a.ranges([(0, 16384), (2_000_000, 4096)])   # -> list of bytes, each block decoded once
    a.decompress()                              # -> whole input as bytes

Every call fails closed: a corrupted archive raises `aceapex.DecodeError`, never returns
partial bytes. The same fixtures that judge the C decoder (`verify/fixtures/`) run under
`pytest python/tests`.

## Windows for sequence models (PyTorch optional)

    import aceapex.torch as at, torch
    ds = at.RandomWindows("t2t.aet", length=8192, count=100_000, seed=1)   # or TiledWindows
    dl = torch.utils.data.DataLoader(ds, batch_size=64, num_workers=4)
    for x in dl: ...                       # uint8 [64, 8192], ASCII bases, decoded per batch from the archive

`ds.batch(idx)` decodes a batch through one `ranges` call (each block once); `ds.numpy_batch(idx)`
does the same without torch. Workers re-open their own decoder handle after pickling.
