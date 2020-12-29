///
#include "zipinternal.hpp"
#include <zlib.h>

namespace baulk::archive::zip {
// DEFLATE
// https://github.com/madler/zlib/blob/master/examples/zpipe.c#L92
bool Reader::decompressDeflate(const File &file, const Receiver &receiver, int64_t &decompressed,
                               bela::error_code &ec) const {
  z_stream zs;
  memset(&zs, 0, sizeof(zs));
  if (auto zerr = inflateInit2(&zs, -MAX_WBITS); zerr != Z_OK) {
    ec = bela::make_error_code(1, bela::ToWide(zError(zerr)));
    return false;
  }
  auto closer = bela::finally([&] { inflateEnd(&zs); });
  Buffer out(outsize);
  Buffer in(insize);
  int64_t uncsize = 0;
  auto csize = file.compressedSize;
  int ret = Z_OK;
  while (csize != 0) {
    auto minsize = (std::min)(csize, static_cast<uint64_t>(in.capacity()));
    if (!ReadFull(in.data(), static_cast<size_t>(minsize), ec)) {
      return false;
    }
    zs.avail_in = static_cast<int>(minsize);
    zs.next_in = in.data();
    do {
      zs.avail_out = static_cast<int>(out.capacity());
      zs.next_out = out.data();
      ret = inflate(&zs, Z_NO_FLUSH);
      switch (ret) {
      case Z_NEED_DICT:
        ret = Z_DATA_ERROR;
        [[fallthrough]];
      case Z_DATA_ERROR:
        [[fallthrough]];
      case Z_MEM_ERROR:
        ec = bela::make_error_code(ret, bela::ToWide(zError(ret)));
        return false;
      default:
        break;
      }
      auto have = minsize - zs.avail_out;
      if (!receiver(out.data(), have)) {
        ec = bela::make_error_code(ErrCanceled, L"canceled");
        return false;
      }
      decompressed += have;
    } while (zs.avail_out == 0);
    csize -= minsize;
    if (ret == Z_STREAM_END) {
      break;
    }
  }
  return true;
}
// DEFLATE64
bool Reader::decompressDeflate64(const File &file, const Receiver &receiver, int64_t &decompressed,
                                 bela::error_code &ec) const {
  //
  return false;
}
} // namespace baulk::archive::zip