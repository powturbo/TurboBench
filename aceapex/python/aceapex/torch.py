"""aceapex.torch - windows of an .aet archive as PyTorch tensors, read by original byte offset.

Sequence models train on random windows of a genome. Keep the archive (0.9 GB for T2T instead
of 3.2 GB) and let the Dataset decode only the windows it is asked for; each distinct 16 KiB
block is decoded once per batch through aceapex.Archive.ranges.

    import aceapex.torch as at
    ds = at.RandomWindows("t2t.aet", length=8192, count=100_000, seed=1)
    dl = torch.utils.data.DataLoader(ds, batch_size=64, num_workers=4)
    for x in dl:            # x: uint8 tensor [64, 8192], ASCII bases; map to tokens as you like
        ...

Workers: the Archive is opened lazily in each worker process (the handle is not shareable).
torch is optional: without it the classes still work and return bytes-backed numpy arrays via .numpy_batch().
"""
import random as _random
import aceapex as _ax
try:
    from torch.utils.data import Dataset as _Base     # a real Dataset when torch is installed
except Exception:                                      # otherwise the same class, usable with numpy
    _Base = object

def _torch():
    import torch
    return torch

class Windows(_Base):
    """Fixed list of (offset, length) windows. Indexable; batches decode through ranges()."""
    def __init__(self, path, spans, dtype=None):
        self.path, self.spans, self.dtype = path, list(spans), dtype
        self._a = None
    def _arc(self):
        if self._a is None: self._a = _ax.open(self.path)
        return self._a
    def __len__(self): return len(self.spans)
    def __getitem__(self, i):
        o, l = self.spans[i]
        return self._tensor(self._arc().read(o, l))
    def batch(self, idx):
        """Decode several windows at once (each distinct block once); returns a stacked tensor
        when all lengths are equal, else a list."""
        spans = [self.spans[i] for i in idx]
        bufs = self._arc().ranges(spans)
        ts = [self._tensor(b) for b in bufs]
        t = _torch()
        return t.stack(ts) if len({len(b) for b in bufs}) == 1 else ts
    def numpy_batch(self, idx):
        """Same as batch() but as a numpy uint8 array [n, length]; no torch needed."""
        import numpy as np
        spans = [self.spans[i] for i in idx]
        return np.stack([np.frombuffer(b, dtype=np.uint8) for b in self._arc().ranges(spans)])
    def _tensor(self, b):
        t = _torch()
        x = t.frombuffer(bytearray(b), dtype=t.uint8)
        return x if self.dtype is None else x.to(self.dtype)
    def __getstate__(self):
        d = dict(self.__dict__); d["_a"] = None; return d   # a worker re-opens its own handle

class RandomWindows(Windows):
    """count windows of one length at random offsets, reproducible from seed."""
    def __init__(self, path, length, count, seed=0, dtype=None):
        size = _ax.open(path).size
        if size < length: raise ValueError("archive smaller than the window")
        r = _random.Random(seed)
        super().__init__(path, [(r.randrange(0, size - length + 1), length) for _ in range(count)], dtype)

class TiledWindows(Windows):
    """Every window of one length on a stride, in order (the whole genome, tiled)."""
    def __init__(self, path, length, stride=None, dtype=None):
        size = _ax.open(path).size; stride = stride or length
        super().__init__(path, [(o, min(length, size - o)) for o in range(0, size, stride)], dtype)
