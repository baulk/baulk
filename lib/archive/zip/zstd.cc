///
#include "zipinternal.hpp"
#include <zstd.h>

namespace baulk::archive::zip {
// zstd
// https://github.com/facebook/zstd/blob/dev/examples/streaming_decompression.c
bool Reader::decompressZstd(const File &file, const Receiver &receiver, int64_t &decompressed,
                            bela::error_code &ec) const {
  const auto boutsize = ZSTD_DStreamOutSize();
  const auto binsize = ZSTD_DStreamInSize();
  Buffer outbuf(boutsize);
  Buffer inbuf(binsize);
  auto zds = ZSTD_createDCtx();
  if (zds == nullptr) {
    ec = bela::make_error_code(L"ZSTD_createDStream() out of memory");
    return false;
  }
  auto closer = bela::finally([&] { ZSTD_freeDCtx(zds); });
  auto csize = file.compressedSize;
  uint32_t crc32val = 0;
  while (csize != 0) {
    auto minsize = (std::min)(csize, static_cast<uint64_t>(binsize));
    if (!ReadFull(inbuf.data(), static_cast<size_t>(minsize), ec)) {
      return false;
    }
    ZSTD_inBuffer in{inbuf.data(), minsize, 0};
    while (in.pos < in.size) {
      ZSTD_outBuffer out{outbuf.data(), boutsize, 0};
      auto result = ZSTD_decompressStream(zds, &out, &in);
      if (ZSTD_isError(result) != 0) {
        ec = bela::make_error_code(1, L"ZSTD_decompressStream: ", bela::ToWide(ZSTD_getErrorName(result)));
        return false;
      }
      crc32val = crc32_fast(out.dst, out.pos, crc32val);
      if (!receiver(out.dst, out.pos)) {
        ec = bela::make_error_code(ErrCanceled, L"canceled");
        return false;
      }
      decompressed += out.pos;
    }
    csize -= minsize;
  }
  if (crc32val != file.crc32) {
    ec = bela::make_error_code(1, L"crc32 want ", file.crc32, L" got ", crc32val, L" not match");
    return false;
  }
  return true;
}
} // namespace baulk::archive::zip