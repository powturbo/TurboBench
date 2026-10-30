"""aceapex - read ACEAPEX (.aet) archives. See README.md. Thin ctypes layer over c/aceapex_decode.c."""
import ctypes, glob, mmap, os

__all__ = ["open", "Archive", "DecodeError", "__version__"]
__version__ = "2.5.0"

class DecodeError(Exception):
    pass

_ERR = {-1: "output buffer too small", -2: "corrupt or unsupported archive", -3: "out of memory"}

class _Range(ctypes.Structure):
    _fields_ = [("offset", ctypes.c_uint64), ("length", ctypes.c_uint64), ("dst", ctypes.c_void_p), ("written", ctypes.c_int64)]

def _load():
    here = os.path.dirname(os.path.abspath(__file__))
    cands = glob.glob(os.path.join(here, "_aceapex_decode*.so")) + glob.glob(os.path.join(here, "..", "..", "libaceapex_decode.so"))
    env = os.environ.get("ACEAPEX_DECODE_SO")
    if env: cands.insert(0, env)
    for c in cands:
        if os.path.exists(c):
            lib = ctypes.CDLL(c); break
    else:
        raise ImportError("aceapex: compiled decoder not found (build the package, or set ACEAPEX_DECODE_SO)")
    lib.aceapex_decoded_size.restype = ctypes.c_int64
    lib.aceapex_decoded_size.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    lib.aceapex_decompress.restype = ctypes.c_int64
    lib.aceapex_decompress.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t]
    lib.aceapex_decompress_region.restype = ctypes.c_int64
    lib.aceapex_decompress_region.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_uint64, ctypes.c_uint64]
    lib.aceapex_decompress_ranges.restype = ctypes.c_int64
    lib.aceapex_decompress_ranges.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(_Range), ctypes.c_size_t, ctypes.c_int]
    lib.aceapex_dec_open.restype = ctypes.c_void_p
    lib.aceapex_dec_open.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    lib.aceapex_dec_size.restype = ctypes.c_int64
    lib.aceapex_dec_size.argtypes = [ctypes.c_void_p]
    lib.aceapex_dec_region.restype = ctypes.c_int64
    lib.aceapex_dec_region.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_uint64, ctypes.c_uint64]
    lib.aceapex_dec_ranges.restype = ctypes.c_int64
    lib.aceapex_dec_ranges.argtypes = [ctypes.c_void_p, ctypes.POINTER(_Range), ctypes.c_size_t]
    lib.aceapex_dec_close.restype = None
    lib.aceapex_dec_close.argtypes = [ctypes.c_void_p]
    return lib

_lib = None
def _l():
    global _lib
    if _lib is None: _lib = _load()
    return _lib

def _check(rc):
    if rc < 0: raise DecodeError(_ERR.get(rc, f"error {rc}"))
    return rc

class Archive:
    """An .aet archive: a path (mapped copy-on-write, never written) or bytes (copied once)."""
    def __init__(self, data):
        if isinstance(data, (bytes, bytearray, memoryview)):
            self._mm = None; self._store = bytearray(data)
        else:
            fd = os.open(data, os.O_RDONLY)
            try: self._mm = mmap.mmap(fd, 0, access=mmap.ACCESS_COPY)
            finally: os.close(fd)
            self._store = self._mm
        self._n = len(self._store)
        self._cbuf = (ctypes.c_char * self._n).from_buffer(self._store) if self._n else (ctypes.c_char * 1)()
        self._addr = ctypes.cast(self._cbuf, ctypes.c_void_p)
        self.size = _check(_l().aceapex_decoded_size(self._addr, self._n))
        # persistent decoder: chunk tables parsed once, last chunks and the zstd context kept between reads
        self._h = _l().aceapex_dec_open(self._addr, self._n)
        if not self._h: raise DecodeError(_ERR[-2])

    def _ptr(self):
        return self._addr

    def read(self, offset, length):
        """Original bytes [offset, offset+length)."""
        if length == 0: return b""
        out = ctypes.create_string_buffer(length)
        n = _check(_l().aceapex_dec_region(self._h, out, length, offset, length))
        return out.raw[:n]

    def ranges(self, spans):
        """List of (offset, length) -> list of bytes; each distinct block is decoded once."""
        k = len(spans); arr = (_Range * k)(); bufs = []
        for i, (o, l) in enumerate(spans):
            b = ctypes.create_string_buffer(max(l, 1)); bufs.append(b)
            arr[i].offset = o; arr[i].length = l; arr[i].dst = ctypes.cast(b, ctypes.c_void_p); arr[i].written = 0
        _check(_l().aceapex_dec_ranges(self._h, arr, k))
        res = []
        for i in range(k):
            w = arr[i].written
            if w < 0: raise DecodeError(f"range {i}: {_ERR.get(w, w)}")
            res.append(bufs[i].raw[:w])
        return res

    def decompress(self):
        out = ctypes.create_string_buffer(max(self.size, 1))
        n = _check(_l().aceapex_decompress(self._ptr(), self._n, out, self.size))
        return out.raw[:n]

    def close(self):
        if getattr(self, "_h", None): _l().aceapex_dec_close(self._h); self._h = None
        self._cbuf = None; self._addr = None
        if self._mm is not None: self._mm.close(); self._mm = None
    def __del__(self):
        try: self.close()
        except Exception: pass
    def __enter__(self): return self
    def __exit__(self, *a): self.close()

def open(path_or_bytes):
    return Archive(path_or_bytes)
