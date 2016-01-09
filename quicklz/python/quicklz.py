"""
This is a I{fast} compression library which uses quicklz to compress and
decompress data. Some ideas and code comes from the Python 2.7 gzip.py module.

The user of this class doesn't have to worry about compression,
but random access is not allowed.

QuickLZ is hosted at U{http://www.quicklz.com}
"""

from __future__ import absolute_import

import io

class QuickLZFile(io.BufferedIOBase):
    """
    The QuickLZFile class simulates most of the methods of a file object
    with the exception of the readinto() and truncate() methods.
    """
    myfileobj = None
    max_read_chunk = 10 * 1024 * 1024 # 10Mb

    def __init__(self, filename=None, mode=None,
        compresslevel=2, fileobj=None, mtime=None):
        """
        QuickLZFile Constructor.
        """
        # guarantee the file is opened in binary mode on platforms
        # that care about that sort of thing
        if mode and 'b' not in mode:
            mode += 'b'
        if fileobj is None:
            fileobj = self.myfileobj = __builtin__.open(filename, mode or 'rb')
        if filename is None:
            if hasattr(fileobj, 'name'): filename = fileobj.name
            else: filename = ''
        if mode is None:
            if hasattr(fileobj, 'mode'): mode = fileobj.mode
            else: mode = 'rb'

        self.name = filename

        if mode[0:1] == 'r':
            self.mode = READ
            
            # Buffer data read from quicklz file. extrastart is offset in
            # stream where buffer starts. extrasize is number of
            # bytes remaining in buffer from current stream position.
            self.readbuf = ""
            self.readbufsize = 0

            # Starts small, scales exponentially
            self.min_readsize = 100

        elif mode[0:1] == 'w' or mode[0:1] == 'a':
            self.mode = WRITE
            self._init_write(filename)
        else:
            raise IOError, "Mode " + mode + " not supported"

        self.fileobj = fileobj
        self.offset = 0
        self.mtime = mtime

        if self.mode == WRITE:
            self._write_qzip_header()

    def _init_write(self, filename):
        self.size = 0
        self.writebuf = []
        self.bufsize = 0

    def _write_qzip_header(self):
        # no-op
        pass

    def _init_read():
        self.size = 0

    def write(self, data):
        if self.mode != WRITE:
            import errno
            raise IOError(errno.EBADF, "write() on read-only QuickLZFile object")

        if self.fileobj is None:
            raise ValueError, "write() on closed QuickLZFile object"

        if hasattr(data, "tobytes"):
            assert isinstance(data, memoryview)
            data = data.tobytes()

        if len(data) > 0:
            self.size = self.size + len(data)
            self.fileobj.write( self.compress.compress(data) )
            self.offset += len(data)

        return len(data)

    def read(self, size=-1):
        if self.mode != READ:
            import errno
            raise IOError(errno.EBADF, "read() on write-only QuickLZFile object")

        if self.readbufsize <= 0 and self.fileobj is None:
            raise EOFError, "Reached EOF"

        readsize = 1024
        if size < 0:        # get the whole thing
            try:
                while True:
                    state = qlz_state_decompress()
                    header_buffer = self.fileobj.read(9)
                    size = qlz_size_compressed(header_buffer)
                    readbuffer = self.fileobj.read(size - 9)
                    self.readbuffer += qlz_decompress(readbuffer, state)
            except EOFError:
                size = self.extrasize
        else:               # just get some more of it
            try:
                while size > self.extrasize: #XXX: TODO: edit here
                    self._read(readsize)
                    readsize = min(self.max_read_chunk, readsize * 2)
            except EOFError:
                if size > self.extrasize:
                    size = self.extrasize

        offset = self.offset - self.extrastart
        chunk = self.extrabuf[offset: offset + size]
        self.extrasize = self.extrasize - size

        self.offset += size
        return chunk


