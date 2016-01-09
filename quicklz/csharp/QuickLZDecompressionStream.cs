
// QuickLZDecompressionStream
// Public domain by Pascal Craponne (picrap@gmail.com). No copyright pending

// Released 5-May-2011: Fixed a missing Dispose call on _sourceStream

namespace QuickLZ
{
    using System;
    using System.IO;
    using QuickLZSharp;

    /// <summary>
    /// Stream to decompress a given input stream
    /// </summary>
    public class QuickLZDecompressionStream : Stream
    {
        #region overrides without any interested

        /// <summary>
        /// Clears all buffers for this stream and causes any buffered data to be written to the underlying device.
        /// </summary>
        /// <exception cref="T:System.IO.IOException">
        /// An I/O error occurs.
        /// </exception>
        public override void Flush()
        {
        }

        /// <summary>
        /// Sets the position within the current stream.
        /// </summary>
        /// <param name="offset">A byte offset relative to the <paramref name="origin"/> parameter.</param>
        /// <param name="origin">A value of type <see cref="T:System.IO.SeekOrigin"/> indicating the reference point used to obtain the new position.</param>
        /// <returns>
        /// The new position within the current stream.
        /// </returns>
        /// <exception cref="T:System.IO.IOException">
        /// An I/O error occurs.
        /// </exception>
        /// <exception cref="T:System.NotSupportedException">
        /// The stream does not support seeking, such as if the stream is constructed from a pipe or console output.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// Methods were called after the stream was closed.
        /// </exception>
        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new NotSupportedException();
        }

        /// <summary>
        /// Sets the length of the current stream.
        /// </summary>
        /// <param name="value">The desired length of the current stream in bytes.</param>
        /// <exception cref="T:System.IO.IOException">
        /// An I/O error occurs.
        /// </exception>
        /// <exception cref="T:System.NotSupportedException">
        /// The stream does not support both writing and seeking, such as if the stream is constructed from a pipe or console output.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// Methods were called after the stream was closed.
        /// </exception>
        public override void SetLength(long value)
        {
            throw new NotSupportedException();
        }

        /// <summary>
        /// Writes a sequence of bytes to the current stream and advances the current position within this stream by the number of bytes written.
        /// </summary>
        /// <param name="buffer">An array of bytes. This method copies <paramref name="count"/> bytes from <paramref name="buffer"/> to the current stream.</param>
        /// <param name="offset">The zero-based byte offset in <paramref name="buffer"/> at which to begin copying bytes to the current stream.</param>
        /// <param name="count">The number of bytes to be written to the current stream.</param>
        /// <exception cref="T:System.ArgumentException">
        /// The sum of <paramref name="offset"/> and <paramref name="count"/> is greater than the buffer length.
        /// </exception>
        /// <exception cref="T:System.ArgumentNullException">
        /// 	<paramref name="buffer"/> is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentOutOfRangeException">
        /// 	<paramref name="offset"/> or <paramref name="count"/> is negative.
        /// </exception>
        /// <exception cref="T:System.IO.IOException">
        /// An I/O error occurs.
        /// </exception>
        /// <exception cref="T:System.NotSupportedException">
        /// The stream does not support writing.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// Methods were called after the stream was closed.
        /// </exception>
        public override void Write(byte[] buffer, int offset, int count)
        {
            throw new NotSupportedException();
        }

        /// <summary>
        /// Gets a value indicating whether the current stream supports reading.
        /// </summary>
        /// <value></value>
        /// <returns>true if the stream supports reading; otherwise, false.
        /// </returns>
        public override bool CanRead
        {
            get { return true; }
        }

        /// <summary>
        /// Gets a value indicating whether the current stream supports seeking.
        /// </summary>
        /// <value></value>
        /// <returns>true if the stream supports seeking; otherwise, false.
        /// </returns>
        public override bool CanSeek
        {
            get { return false; }
        }

        /// <summary>
        /// Gets a value indicating whether the current stream supports writing.
        /// </summary>
        /// <value></value>
        /// <returns>true if the stream supports writing; otherwise, false.
        /// </returns>
        public override bool CanWrite
        {
            get { return false; }
        }

        /// <summary>
        /// Gets the length in bytes of the stream.
        /// </summary>
        /// <value></value>
        /// <returns>
        /// A long value representing the length of the stream in bytes.
        /// </returns>
        /// <exception cref="T:System.NotSupportedException">
        /// A class derived from Stream does not support seeking.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// Methods were called after the stream was closed.
        /// </exception>
        public override long Length
        {
            get { throw new NotSupportedException(); }
        }

        /// <summary>
        /// Gets or sets the position within the current stream.
        /// </summary>
        /// <value></value>
        /// <returns>
        /// The current position within the stream.
        /// </returns>
        /// <exception cref="T:System.IO.IOException">
        /// An I/O error occurs.
        /// </exception>
        /// <exception cref="T:System.NotSupportedException">
        /// The stream does not support seeking.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// Methods were called after the stream was closed.
        /// </exception>
        public override long Position
        {
            get { throw new NotSupportedException(); }
            set { throw new NotSupportedException(); }
        }

        #endregion

        /// <summary>
        /// QuickLZ engine
        /// </summary>
        private readonly QuickLZ _quickLZ;

        /// <summary>
        /// Stream we're reading from
        /// </summary>
        private readonly Stream _sourceStream;

        /// <summary>
        /// Temporary buffer where raw data is stored.
        /// Kept to be reused from one buffer fill to another
        /// </summary>
        private byte[] _readBuffer;

        /// <summary>
        /// Unpacked buffer
        /// </summary>
        private byte[] _unpackedBuffer;
        /// <summary>
        /// Read position un unpacked buffer
        /// </summary>
        private int _unpackedOffset;

        /// <summary>
        /// Length for unpacked data
        /// </summary>
        private int _unpackedLength;

        /// <summary>
        /// Room for header
        /// </summary>
        private readonly byte[] _header = new byte[9];

        /// <summary>
        /// Allocates the specified length.
        /// </summary>
        /// <param name="length">The length.</param>
        /// <returns></returns>
        protected virtual byte[] Alloc(int length)
        {
            return new byte[length];
        }

        /// <summary>
        /// Frees the specified data.
        /// </summary>
        /// <param name="data">The data.</param>
        protected virtual void Free(byte[] data)
        {
        }

        /// <summary>
        /// Ensures the unpacked buffer has enough space.
        /// </summary>
        /// <param name="packedBuffer">The packed buffer.</param>
        private void EnsureUnpackedBuffer(byte[] packedBuffer)
        {
            int unpackedLength = _quickLZ.sizeDecompressed(packedBuffer);
            if (_unpackedBuffer == null || _unpackedBuffer.Length < unpackedLength)
            {
                if (_unpackedBuffer != null)
                    Free(_unpackedBuffer);
                _unpackedBuffer = Alloc(unpackedLength);
            }
        }

        /// <summary>
        /// Fills or refills the read buffer.
        /// </summary>
        private void Fill()
        {
            int headerLength = _sourceStream.Read(_header, 0, _header.Length);
            // the normal end is here
            if (headerLength == 0)
            {
                _unpackedBuffer = null;
                return;
            }
            if (headerLength != _header.Length)
                throw new InvalidDataException("QuickLZ input buffer corrupted (header)");
            int sizeCompressed = _quickLZ.sizeCompressed(_header);
            if (_readBuffer == null || _readBuffer.Length < sizeCompressed)
            {
                if (_readBuffer != null)
                    Free(_readBuffer);
                _readBuffer = Alloc(sizeCompressed);
            }
            Buffer.BlockCopy(_header, 0, _readBuffer, 0, _header.Length);
            int bodyLength = _sourceStream.Read(_readBuffer, _header.Length, sizeCompressed - _header.Length);
            if (bodyLength != sizeCompressed - _header.Length)
                throw new InvalidDataException("QuickLZ input buffer corrupted (body)");
            EnsureUnpackedBuffer(_readBuffer);
            _unpackedLength = _quickLZ.decompress(_readBuffer, _unpackedBuffer);
            _unpackedOffset = 0;
        }

        /// <summary>
        /// Reads a sequence of bytes from the current stream and advances the position within the stream by the number of bytes read.
        /// </summary>
        /// <param name="buffer">An array of bytes. When this method returns, the buffer contains the specified byte array with the values between <paramref name="offset"/> and (<paramref name="offset"/> + <paramref name="count"/> - 1) replaced by the bytes read from the current source.</param>
        /// <param name="offset">The zero-based byte offset in <paramref name="buffer"/> at which to begin storing the data read from the current stream.</param>
        /// <param name="count">The maximum number of bytes to be read from the current stream.</param>
        /// <returns>
        /// The total number of bytes read into the buffer. This can be less than the number of bytes requested if that many bytes are not currently available, or zero (0) if the end of the stream has been reached.
        /// </returns>
        /// <exception cref="T:System.ArgumentException">
        /// The sum of <paramref name="offset"/> and <paramref name="count"/> is larger than the buffer length.
        /// </exception>
        /// <exception cref="T:System.ArgumentNullException">
        /// 	<paramref name="buffer"/> is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentOutOfRangeException">
        /// 	<paramref name="offset"/> or <paramref name="count"/> is negative.
        /// </exception>
        /// <exception cref="T:System.IO.IOException">
        /// An I/O error occurs.
        /// </exception>
        /// <exception cref="T:System.NotSupportedException">
        /// The stream does not support reading.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// Methods were called after the stream was closed.
        /// </exception>
        public override int Read(byte[] buffer, int offset, int count)
        {
            if (_unpackedBuffer == null || _unpackedOffset == _unpackedLength)
                Fill();

            // to do: something smarter than the double test
            if (_unpackedBuffer == null)
                return 0;

            // 1. If we don't have enough data available, then split
            if (_unpackedOffset + count > _unpackedLength)
            {
                int available = _unpackedLength - _unpackedOffset;
                // this is the part we're sure to get
                int r1 = Read(buffer, offset, available);
                // this is the part we're not
                int r2 = Read(buffer, offset + available, count - available);
                return r1 + r2;
            }
            // 2. we have enough buffer, use it
            Buffer.BlockCopy(_unpackedBuffer, _unpackedOffset, buffer, offset, count);
            _unpackedOffset += count;
            return count;
        }

        /// <summary>
        /// Releases the unmanaged resources used by the <see cref="T:System.IO.Stream"/> and optionally releases the managed resources.
        /// </summary>
        /// <param name="disposing">true to release both managed and unmanaged resources; false to release only unmanaged resources.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
                Pool.Default.Free(_quickLZ);
            base.Dispose(disposing);
            _sourceStream.Dispose();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="QuickLZDecompressionStream"/> class.
        /// </summary>
        /// <param name="sourceStream">The source stream.</param>
        public QuickLZDecompressionStream(Stream sourceStream)
        {
            _quickLZ = Pool.Default.Alloc();
            _sourceStream = sourceStream;
            Fill();
        }
    }
}
