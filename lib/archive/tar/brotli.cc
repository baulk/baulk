//
//
#include "brotli.hpp"

namespace baulk::archive::tar::brotli {
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

ssize_t Reader::CopyBuffer(void *buffer, size_t len, bela::error_code &ec) {
  auto minsize = (std::min)(len, out.size() - out.pos());
  memcpy(buffer, out.data() + out.pos(), minsize);
  out.pos() += minsize;
  return minsize;
}

ssize_t Reader::Read(void *buffer, size_t len, bela::error_code &ec) {
  if (out.pos() != out.size()) {
    return CopyBuffer(buffer, len, ec);
  }
  if (result == BROTLI_DECODER_RESULT_ERROR) {
    ec = bela::make_error_code(ErrGeneral, L"BrotliDecoderDecompressStream error");
    return -1;
  }
  for (;;) {
    if (result == BROTLI_DECODER_NEEDS_MORE_INPUT || pickBytes == 0) {
      auto n = r->Read(in.data(), in.capacity(), ec);
      if (n <= 0) {
        return n;
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
       return CopyBuffer(buffer, len, ec);
    }
  }
  return -1;
}
} // namespace baulk::archive::tar::brotli