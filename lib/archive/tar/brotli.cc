//
//
#include "brotli.hpp"

namespace baulk::archive::tar::brotli {
// tar support brotli
Reader::~Reader() {
  if (state != nullptr) {
    BrotliDecoderDestroyInstance(state);
  }
}

bool Reader::Initialize(bela::error_code &ec) {
  state = BrotliDecoderCreateInstance(0, 0, 0);
  if (state == nullptr) {
    ec = bela::make_error_code(L"BrotliDecoderCreateInstance failed");
    return false;
  }
  BrotliDecoderSetParameter(state, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1u);
  out.grow(outsize);
  in.grow(insize);
  return true;
}

bool Reader::decompress(bela::error_code &ec) {
  if (result == BROTLI_DECODER_RESULT_ERROR) {
    ec = bela::make_error_code(ErrGeneral, L"BrotliDecoderDecompressStream error");
    return false;
  }
  for (;;) {
    if (result == BROTLI_DECODER_NEEDS_MORE_INPUT || pickBytes == 0) {
      auto n = r->Read(in.data(), in.capacity(), ec);
      if (n <= 0) {
        return false;
      }
      pickBytes += static_cast<int64_t>(n);
      avail_in = static_cast<size_t>(n);
      inptr = in.data();
    }
    auto outptr = out.data();
    auto avail_out = outsize;
    result = BrotliDecoderDecompressStream(state, &avail_in, &inptr, &avail_out, &outptr, &totalout);
    if (outptr != out.data()) {
      auto have = outptr - out.data();
      out.pos() = 0;
      out.size() = have;
      return true;
    }
  }
  return false;
}

ssize_t Reader::Read(void *buffer, size_t len, bela::error_code &ec) {
  if (out.pos() == out.size()) {
    if (!decompress(ec)) {
      return -1;
    }
  }
  auto minsize = (std::min)(len, out.size() - out.pos());
  memcpy(buffer, out.data() + out.pos(), minsize);
  out.pos() += minsize;
  return minsize;
}

bool Reader::Discard(int64_t len, bela::error_code &ec) {
  while (len > 0) {
    if (out.pos() == out.size()) {
      if (!decompress(ec)) {
        return false;
      }
    }
    // seek position
    auto minsize = (std::min)(static_cast<size_t>(len), out.size() - out.pos());
    out.pos() += minsize;
    len -= minsize;
  }
  return true;
}

// Avoid multiple memory copies
bool Reader::WriteTo(const Writer &w, int64_t filesize, int64_t &extracted, bela::error_code &ec) {
  while (filesize > 0) {
    if (out.pos() == out.size()) {
      if (!decompress(ec)) {
        return false;
      }
    }
    auto minsize = (std::min)(static_cast<size_t>(filesize), out.size() - out.pos());
    auto p = out.data() + out.pos();
    out.pos() += minsize;
    extracted += minsize;
    if (!w(p, minsize, ec)) {
      return false;
    }
  }
  return true;
}

} // namespace baulk::archive::tar::brotli