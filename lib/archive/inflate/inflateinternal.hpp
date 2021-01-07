///
#ifndef BAULK_ARCHIVE_INFLATE_INTERNAL_HPP
#define BAULK_ARCHIVE_INFLATE_INTERNAL_HPP
#include <cstdint>
#include <cassert>
#include <cstring>
#include <archive.hpp>

namespace baulk::archive::inflate::inflate_internal {
using uint = unsigned int;
// https://github.com/dotnet/runtime/blob/master/src/libraries/System.IO.Compression/src/System/IO/Compression/DeflateManaged/InputBuffer.cs
class InputBuffer final {
private:
  uint8_t *_buffer{nullptr}; // byte array to store input
  int _start{0};             // start poisition of the buffer
  int _end{0};               // end position of the buffer
  uint _bitBuffer{0};        // store the bits here, we can quickly shift in this buffer
  int _bitsInBuffer{0};      // number of bits available in bitBuffer
  uint GetBitMask(int count) { return (1U << count) - 1; }

public:
  /// <summary>Total bits available in the input buffer.</summary>
  int AvailableBits() const { return _bitsInBuffer; };

  /// <summary>Total bytes available in the input buffer.</summary>
  int AvailableBytes() const { return (_end - _start) + (_bitsInBuffer / 8); }

  /// <summary>Ensure that count bits are in the bit buffer.</summary>
  /// <param name="count">Can be up to 16.</param>
  /// <returns>Returns false if input is not sufficient to make this true.</returns>
  bool EnsureBitsAvailable(int count) {
    assert(0 < count && count <= 16);

    // manual inlining to improve perf
    if (_bitsInBuffer < count) {
      if (NeedsInput()) {
        return false;
      }
      assert(_buffer != nullptr);
      // insert a byte to bitbuffer
      _bitBuffer |= static_cast<uint>(_buffer[_start++]) << _bitsInBuffer;
      _bitsInBuffer += 8;

      if (_bitsInBuffer < count) {
        if (NeedsInput()) {
          return false;
        }
        // insert a byte to bitbuffer
        _bitBuffer |= static_cast<uint>(_buffer[_start++]) << _bitsInBuffer;
        _bitsInBuffer += 8;
      }
    }
    return true;
  }

  /// <summary>
  /// This function will try to load 16 or more bits into bitBuffer.
  /// It returns whatever is contained in bitBuffer after loading.
  /// The main difference between this and GetBits is that this will
  /// never return -1. So the caller needs to check AvailableBits to
  /// see how many bits are available.
  /// </summary>

  uint TryLoad16Bits() {
    assert(_buffer != nullptr);
    if (_bitsInBuffer < 8) {
      if (_start < _end) {
        _bitBuffer |= static_cast<uint>(_buffer[_start++]) << _bitsInBuffer;
        _bitsInBuffer += 8;
      }

      if (_start < _end) {
        _bitBuffer |= static_cast<uint>(_buffer[_start++]) << _bitsInBuffer;
        _bitsInBuffer += 8;
      }
    } else if (_bitsInBuffer < 16) {
      if (_start < _end) {
        _bitBuffer |= static_cast<uint>(_buffer[_start++]) << _bitsInBuffer;
        _bitsInBuffer += 8;
      }
    }

    return _bitBuffer;
  }

  /// <summary>Gets count bits from the input buffer. Returns -1 if not enough bits available.</summary>
  int GetBits(int count) {
    assert(0 < count && count <= 16);

    if (!EnsureBitsAvailable(count)) {
      return -1;
    }

    int result = static_cast<int>(_bitBuffer & GetBitMask(count));
    _bitBuffer >>= count;
    _bitsInBuffer -= count;
    return result;
  }

  /// <summary>
  /// Copies length bytes from input buffer to output buffer starting at output[offset].
  /// You have to make sure, that the buffer is byte aligned. If not enough bytes are
  /// available, copies fewer bytes.
  /// </summary>
  /// <returns>Returns the number of bytes copied, 0 if no byte is available.</returns>
  int CopyTo(uint8_t *output, int outlen, int offset, int length) {
    assert(output != nullptr);
    assert(offset >= 0);
    assert(length >= 0);
    assert(offset <= outlen - length);
    assert((_bitsInBuffer % 8) == 0);

    // Copy the bytes in bitBuffer first.
    int bytesFromBitBuffer = 0;
    while (_bitsInBuffer > 0 && length > 0) {
      output[offset++] = static_cast<uint8_t>(_bitBuffer);
      _bitBuffer >>= 8;
      _bitsInBuffer -= 8;
      length--;
      bytesFromBitBuffer++;
    }

    if (length == 0) {
      return bytesFromBitBuffer;
    }

    int avail = _end - _start;
    if (length > avail) {
      length = avail;
    }

    assert(_buffer != nullptr);
    memcpy(_buffer + _start, output + offset, length);
    _start += length;
    return bytesFromBitBuffer + length;
  }

  /// <summary>
  /// Return true is all input bytes are used.
  /// This means the caller can call SetInput to add more input.
  /// </summary>

  bool NeedsInput() const { return _start == _end; }

  /// <summary>
  /// Set the byte array to be processed.
  /// All the bits remained in bitBuffer will be processed before the new bytes.
  /// We don't clone the byte array here since it is expensive.
  /// The caller should make sure after a buffer is passed in.
  /// It will not be changed before calling this function again.
  /// </summary>

  void SetInput(uint8_t *buffer, int buflen, int offset, int length) {
    assert(buffer != nullptr);
    assert(offset >= 0);
    assert(length >= 0);
    assert(offset <= buflen - length);

    if (_start == _end) {
      _buffer = buffer;
      _start = offset;
      _end = offset + length;
    }
  }

  /// <summary>Skip n bits in the buffer.</summary>

  void SkipBits(int n) {
    assert(_bitsInBuffer >= n);
    _bitBuffer >>= n;
    _bitsInBuffer -= n;
  }

  /// <summary>Skips to the next byte boundary.</summary>
  void SkipToByteBoundary() {
    _bitBuffer >>= (_bitsInBuffer % 8);
    _bitsInBuffer = _bitsInBuffer - (_bitsInBuffer % 8);
  }
};
// https://github.com/dotnet/runtime/blob/master/src/libraries/System.IO.Compression/src/System/IO/Compression/DeflateManaged/OutputWindow.cs
class OutputWindow final {
private:
  constexpr static int WindowSize = 262144;
  constexpr static int WindowMask = 262143;
  uint8_t *_window{nullptr};
  int _end{0};
  int _bytesUsed{0};
  void ClearBytesUsed() { _bytesUsed = 0; }

public:
  OutputWindow() {
    _window = baulk::archive::archive_internal::Allocate<uint8_t>(WindowSize); // allocate
  }
  OutputWindow(const OutputWindow &) = delete;
  OutputWindow &operator=(const OutputWindow &) = delete;
  ~OutputWindow() { baulk::archive::archive_internal::Deallocate(_window, WindowSize); }
  void Write(uint8_t b) {
    assert(_bytesUsed < WindowSize);
    _window[_end++] = b;
    _end &= WindowMask;
    ++_bytesUsed;
  }
  void WriteLengthDistance(int length, int distance) {
    assert(_bytesUsed + length <= WindowSize);
    // move backwards distance bytes in the output stream,
    // and copy length bytes from this position to the output stream.
    _bytesUsed += length;
    int copyStart = (_end - distance) & WindowMask; // start position for coping.

    int border = WindowSize - length;
    if (copyStart <= border && _end < border) {
      if (length <= distance) {
        memcpy(_window + copyStart, _window + _end, length);
        _end += length;
      } else {
        // The referenced string may overlap the current
        // position; for example, if the last 2 bytes decoded have values
        // X and Y, a string reference with <length = 5, distance = 2>
        // adds X,Y,X,Y,X to the output stream.
        while (length-- > 0) {
          _window[_end++] = _window[copyStart++];
        }
      }
    } else {
      // copy byte by byte
      while (length-- > 0) {
        _window[_end++] = _window[copyStart++];
        _end &= WindowMask;
        copyStart &= WindowMask;
      }
    }
  }
  /// <summary>
  /// Copy up to length of bytes from input directly.
  /// This is used for uncompressed block.
  /// </summary>
  int CopyFrom(InputBuffer &input, int length) {
    length = (std::min)((std::min)(length, WindowSize - _bytesUsed), input.AvailableBytes());
    int copied;

    // We might need wrap around to copy all bytes.
    int tailLen = WindowSize - _end;
    if (length > tailLen) {
      // copy the first part
      copied = input.CopyTo(_window, WindowSize, _end, tailLen);
      if (copied == tailLen) {
        // only try to copy the second part if we have enough bytes in input
        copied += input.CopyTo(_window, WindowSize, 0, length - tailLen);
      }
    } else {
      // only one copy is needed if there is no wrap around.
      copied = input.CopyTo(_window, WindowSize, _end, length);
    }

    _end = (_end + copied) & WindowMask;
    _bytesUsed += copied;
    return copied;
  }
  /// <summary>Free space in output window.</summary>
  int FreeBytes() const { return WindowSize - _bytesUsed; }

  /// <summary>Bytes not consumed in output window.</summary>
  int AvailableBytes() const { return _bytesUsed; }

  /// <summary>Copy the decompressed bytes to output array.</summary>
  int CopyTo(uint8_t *output, int outlen, int offset, int length) {
    int copy_end;
    if (length > _bytesUsed) {
      // we can copy all the decompressed bytes out
      copy_end = _end;
      length = _bytesUsed;
    } else {
      copy_end = (_end - _bytesUsed + length) & WindowMask; // copy length of bytes
    }

    int copied = length;

    int tailLen = length - copy_end;
    if (tailLen > 0) {
      // this means we need to copy two parts separately
      // copy tailLen bytes from the end of output window
      memcpy(_window + (WindowSize - tailLen), output + offset, tailLen);
      offset += tailLen;
      length = copy_end;
    }
    memcpy(_window + (copy_end - length), output + offset, length);
    _bytesUsed -= copied;
    assert(_bytesUsed >= 0);
    return copied;
  }

private:
};
} // namespace baulk::archive::inflate::inflate_internal

#endif