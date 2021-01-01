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
  uint32_t crc32val = 0;
  while (csize != 0) {
    auto minsize = (std::min)(csize, static_cast<uint64_t>(insize));
    if (!ReadFull(in.data(), static_cast<size_t>(minsize), ec)) {
      return false;
    }
    size_t avail_in = static_cast<size_t>(minsize);
    const unsigned char *inptr = in.data();
    for (;;) {
      auto outptr = out.data();
      auto avail_out = outsize;
      result = BrotliDecoderDecompressStream(state, &avail_in, &inptr, &avail_out, &outptr, &totalout);
      if (outptr != out.data()) {
        auto have = outptr - out.data();
        crc32val = crc32_fast(out.data(), have, crc32val);
        if (!receiver(out.data(), have)) {
          ec = bela::make_error_code(ErrCanceled, L"canceled");
          return false;
        }
        decompressed += have;
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
    csize -= minsize;
    if (result == BROTLI_DECODER_RESULT_SUCCESS) {
      break;
    }
  }
  if (crc32val != file.crc32) {
    ec = bela::make_error_code(1, L"crc32 want ", file.crc32, L" got ", crc32val, L" not match");
    return false;
  }
  return true;
}
} // namespace baulk::archive::zip