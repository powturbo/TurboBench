
// QuickLZCompressionStream
// Public domain by Pascal Craponne (picrap@gmail.com). No copyright pending

namespace QuickLZ
{
    using System;
    using System.IO;
    using QuickLZSharp;

    /// <summary>
    /// Stream to compress and write back data
    /// </summary>
    public class QuickLZCompressionStream : Stream
    {
        #region overrides without any interested

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
            throw new NotSupportedException();
        }

        /// <summary>
        /// When overridden in a derived class, gets a value indicating whether the current stream supports reading.
        /// </summary>
        /// <value></value>
        /// <returns>true if the stream supports reading; otherwise, false.
        /// </returns>
        public override bool CanRead
        {
            get { return false; }
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
            get { return true; }
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
        /// QuickLZ engine instance
        /// </summary>
        private readonly QuickLZ _quickLZ;

        /// <summary>
        /// The Stream we're writing to
        /// </summary>
        private readonly Stream _targetStream;

        /// <summary>
        /// Write buffer
        /// </summary>
        private readonly byte[] _writeBuffer;

        /// <summary>
        /// Current position in write buffer
        /// </summary>
        private int _writeBufferOffset;

        /// <summary>
        /// Buffer where compressed data is stored
        /// </summary>
        private readonly byte[] _compressedBuffer;

        /// <summary>
        /// Clears all buffers for this stream and causes any buffered data to be written to the underlying device.
        /// </summary>
        /// <exception cref="T:System.IO.IOException">
        /// An I/O error occurs.
        /// </exception>
        public override void Flush()
        {
            if (_writeBufferOffset > 0)
            {
                int compressedLength = _quickLZ.compress(_writeBuffer, _compressedBuffer, _writeBufferOffset);
                _targetStream.Write(_compressedBuffer, 0, compressedLength);
                _writeBufferOffset = 0;
            }
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
            // we have 3 options here:
            // buffer can still be filled --> we fill
            // buffer is full --> we flush
            // buffer is overflood --> we flush and refill

            // 1. there is enough room, the buffer is not full
            int writeLength = _writeBufferOffset + count;
            if (writeLength <= _writeBuffer.Length)
            {
                Buffer.BlockCopy(buffer, offset, _writeBuffer, _writeBufferOffset, count);
                _writeBufferOffset += count;
                // 2. same size: write
                if (_writeBufferOffset == _writeBuffer.Length)
                    Flush();
            }
            // 3. buffer overflow: we split
            else
            {
                int lengthToCauseFlush = _writeBuffer.Length - _writeBufferOffset;
                // this first Write will cause a flush
                Write(buffer, offset, lengthToCauseFlush);
                // this one will refill
                Write(buffer, offset + lengthToCauseFlush, count - lengthToCauseFlush);
            }
        }

        /// <summary>
        /// Releases the unmanaged resources used by the <see cref="T:System.IO.Stream"/> and optionally releases the managed resources.
        /// </summary>
        /// <param name="disposing">true to release both managed and unmanaged resources; false to release only unmanaged resources.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                Flush();
                Pool.Default.Free(_quickLZ);
            }
            base.Dispose(disposing);
            _targetStream.Dispose();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="QuickLZCompressionStream"/> class.
        /// </summary>
        /// <param name="targetStream">The target stream.</param>
        /// <param name="writeBuffer">The write buffer.</param>
        /// <param name="compressionBuffer">The compression buffer.</param>
        public QuickLZCompressionStream(Stream targetStream, byte[] writeBuffer, byte[] compressionBuffer)
        {
            _quickLZ = Pool.Default.Alloc();
            _targetStream = targetStream;
            _writeBuffer = writeBuffer;
            _compressedBuffer = compressionBuffer;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="QuickLZCompressionStream"/> class.
        /// </summary>
        /// <param name="targetStream">The target.</param>
        /// <param name="bufferSize">Size of the buffer.</param>
        public QuickLZCompressionStream(Stream targetStream, int bufferSize)
            : this(targetStream, new byte[bufferSize], new byte[bufferSize + 400])
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="QuickLZCompressionStream"/> class.
        /// </summary>
        /// <param name="targetStream">The target.</param>
        public QuickLZCompressionStream(Stream targetStream)
            : this(targetStream, 1 << 20) // Yes, this is 1 MB
        {
        }
    }
}
