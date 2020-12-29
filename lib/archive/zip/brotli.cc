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
  while (csize != 0) {
    auto minsize = (std::min)(csize, static_cast<uint64_t>(in.size()));
    if (!ReadFull(in.data(), static_cast<size_t>(minsize), ec)) {
      return false;
    }
    // BrotliDecoderDecompressStream
    csize -= minsize;
  }
  return true;
}
} // namespace baulk::archive::zip