///
#include "zipinternal.hpp"
#include <brotli/decode.h>

namespace baulk::archive::zip {

// https://github.com/google/brotli/blob/master/c/tools/brotli.c#L884
// Brotli
bool Reader::decompressBrotli(const File &file, const Receiver &receiver, int64_t &decompressed,
                              bela::error_code &ec) const {
  auto state = BrotliDecoderCreateInstance(0, 0, 0);
  if (state == nullptr) {
    ec = bela::make_error_code(L"BrotliDecoderCreateInstance failed");
    return false;
  }
  auto closer = bela::finally([&] { BrotliDecoderDestroyInstance(state); });
  BrotliDecoderSetParameter(state, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1u);
  Buffer out(outsize);
  Buffer in(insize);
  auto csize = file.compressedSize;
  BrotliDecoderResult result{};
  size_t totalout = 0;
  while (csize != 0) {
    auto minsize = (std::min)(csize, static_cast<uint64_t>(in.size()));
    if (!ReadFull(in.data(), static_cast<size_t>(minsize), ec)) {
      return false;
    }
    auto avail_in = minsize;
    const unsigned char *inptr = in.data();
    for (;;) {
      auto outptr = out.data();
      auto avail_out = out.capacity();
      result = BrotliDecoderDecompressStream(state, &avail_in, &inptr, &avail_out, &outptr, &totalout);
      if (outptr != out.data()) {
        auto hava = outptr - out.data();
        if (!receiver(out.data(), hava)) {
          ec = bela::make_error_code(ErrCanceled, L"canceled");
          return false;
        }
        decompressed += hava;
      }
      if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
        break;
      }
      if (result == BROTLI_DECODER_RESULT_ERROR) {
        ec = bela::make_error_code(1, L"BrotliDecoderDecompressStream error");
        return false;
      }
      if (result == BROTLI_DECODER_RESULT_SUCCESS) {
        break;
      }
    }
    // BrotliDecoderDecompressStream
    csize -= minsize;
    if (result == BROTLI_DECODER_RESULT_SUCCESS) {
      break;
    }
  }
  return true;
}
} // namespace baulk::archive::zip