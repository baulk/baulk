///
#include "zipinternal.hpp"
#include <brotli/decode.h>

namespace baulk::archive::zip {

// https://github.com/google/brotli/blob/master/c/tools/brotli.c#L884
// Brotli
bool Reader::decompressBrotli(const File &file, const Writer &w, bela::error_code &ec) const {
  auto state = BrotliDecoderCreateInstance(baulk::mem::allocate_simple, baulk::mem::deallocate_simple, nullptr);
  if (state == nullptr) {
    ec = bela::make_error_code(L"BrotliDecoderCreateInstance failed");
    return false;
  }
  auto closer = bela::finally([&] { BrotliDecoderDestroyInstance(state); });
  BrotliDecoderSetParameter(state, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1U);
  Buffer out(outsize);
  Buffer in(insize);
  auto csize = file.compressed_size;
  BrotliDecoderResult result{};
  size_t totalout = 0;
  Summator sum(file.crc32_value);
  while (csize != 0) {
    auto minsize = (std::min)(csize, static_cast<uint64_t>(insize));
    if (!fd.ReadFull({in.data(), static_cast<size_t>(minsize)}, ec)) {
      return false;
    }
    auto avail_in = static_cast<size_t>(minsize);
    const unsigned char *inptr = in.data();
    for (;;) {
      auto outptr = out.data();
      auto avail_out = outsize;
      result = BrotliDecoderDecompressStream(state, &avail_in, &inptr, &avail_out, &outptr, &totalout);
      if (outptr != out.data()) {
        auto have = outptr - out.data();
        sum.Update(out.data(), have); // CRC32 update
        if (!w(out.data(), have)) {
          ec = bela::make_error_code(ErrCanceled, L"canceled");
          return false;
        }
      }
      if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
        break;
      }
      if (result == BROTLI_DECODER_RESULT_ERROR) {
        ec = bela::make_error_code(ErrGeneral, L"BrotliDecoderDecompressStream error");
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
  if (!sum.Valid()) {
    ec = bela::make_error_code(ErrGeneral, L"crc32 want ", file.crc32_value, L" got ", sum.Current(), L" not match");
    return false;
  }
  return true;
}
} // namespace baulk::archive::zip