///
#include "zipinternal.hpp"
#define ZSTD_STATIC_LINKING_ONLY 1
#include <zstd.h>

namespace baulk::archive::zip {
// zstd
// https://github.com/facebook/zstd/blob/dev/examples/streaming_decompression.c
bool Reader::decompressZstd(const File &file, const Writer &w, bela::error_code &ec) const {
  const auto boutsize = ZSTD_DStreamOutSize();
  const auto binsize = ZSTD_DStreamInSize();
  Buffer outbuf(boutsize);
  Buffer inbuf(binsize);
  auto zds = ZSTD_createDCtx_advanced(ZSTD_customMem{
      .customAlloc = baulk::mem::allocate_simple, .customFree = baulk::mem::deallocate_simple, .opaque = nullptr});
  if (zds == nullptr) {
    ec = bela::make_error_code(L"ZSTD_createDStream() out of memory");
    return false;
  }
  auto closer = bela::finally([&] { ZSTD_freeDCtx(zds); });
  auto csize = file.compressed_size;
  Summator sum(file.crc32_value);
  while (csize != 0) {
    auto minsize = (std::min)(csize, static_cast<uint64_t>(binsize));
    if (!fd.ReadFull({inbuf.data(), static_cast<size_t>(minsize)}, ec)) {
      return false;
    }
    ZSTD_inBuffer in{inbuf.data(), minsize, 0};
    while (in.pos < in.size) {
      ZSTD_outBuffer out{outbuf.data(), boutsize, 0};
      auto result = ZSTD_decompressStream(zds, &out, &in);
      if (ZSTD_isError(result) != 0) {
        ec = bela::make_error_code(ErrGeneral, L"ZSTD_decompressStream: ",
                                   bela::encode_into<char, wchar_t>(ZSTD_getErrorName(result)));
        return false;
      }
      sum.Update(out.dst, out.pos);
      if (!w(out.dst, out.pos)) {
        ec = bela::make_error_code(ErrCanceled, L"canceled");
        return false;
      }
    }
    csize -= minsize;
  }
  if (!sum.Valid()) {
    ec = bela::make_error_code(ErrGeneral, L"crc32 want ", file.crc32_value, L" got ", sum.Current(), L" not match");
    return false;
  }
  return true;
}
} // namespace baulk::archive::zip