#!env python
"""
This is a test script to demonstrate how to use the quicklz python module.
Run this script from the same directory as quicklz.so.
"""

from quicklz import QLZStateCompress, QLZStateDecompress, qlz_compress, qlz_decompress

def compress_data(raw_data):
    """
    This function accepts uncompressed data as an argument,
    and returns the compressed form of that data.
    """
    state = QLZStateCompress()
    return qlz_compress(raw_data, state)

def decompress_data(compressed_chunk):
    """
    This function accepts a self-contained (not streamed) chunk of data
    and returns the decompressed contents.
    """
    state = QLZStateDecompress()
    return qlz_decompress(compressed_chunk, state)

if __name__ == '__main__':
    data = "This is an uncompressed string."
    print "uncompressed data:", data
    print "compressed data:", compress_data(data)
    print "restored data:", decompress_data(compress_data(data))
