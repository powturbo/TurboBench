import ctypes
import os

_lib = ctypes.CDLL(os.path.join(os.path.dirname(__file__), '../libaceapex.so'))

_lib.aceapex_compress_bound.restype = ctypes.c_size_t
_lib.aceapex_compress_bound.argtypes = [ctypes.c_size_t]

_lib.aceapex_compress.restype = ctypes.c_int64
_lib.aceapex_compress.argtypes = [
    ctypes.c_void_p, ctypes.c_size_t,
    ctypes.c_void_p, ctypes.c_size_t,
    ctypes.c_int, ctypes.c_int]

_lib.aceapex_decompress.restype = ctypes.c_int64
_lib.aceapex_decompress.argtypes = [
    ctypes.c_void_p, ctypes.c_size_t,
    ctypes.c_void_p, ctypes.c_size_t]

def compress(data: bytes, level: int = 2, threads: int = 8) -> bytes:
    src = ctypes.create_string_buffer(data)
    bound = _lib.aceapex_compress_bound(len(data))
    dst = ctypes.create_string_buffer(bound)
    n = _lib.aceapex_compress(src, len(data), dst, bound, level, threads)
    if n < 0: raise RuntimeError(f"compress failed: {n}")
    return bytes(dst[:n])

def decompress(data: bytes, max_size: int) -> bytes:
    src = ctypes.create_string_buffer(data)
    dst = ctypes.create_string_buffer(max_size)
    n = _lib.aceapex_decompress(src, len(data), dst, max_size)
    if n < 0: raise RuntimeError(f"decompress failed: {n}")
    return bytes(dst[:n])
