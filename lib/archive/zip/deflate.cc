///
#include "zipinternal.hpp"
#include <zlib-ng.h>

namespace baulk::archive::zip {
// DEFLATE
// https://github.com/madler/zlib/blob/master/examples/zpipe.c#L92
bool Reader::decompressDeflate(const File &file, const Writer &w, bela::error_code &ec) const {
  zng_stream zs;
  zs.zalloc = baulk::mem::allocate_zlib;
  zs.zfree = baulk::mem::deallocate_simple;
  memset(&zs, 0, sizeof(zs));
  if (auto zerr = zng_inflateInit2(&zs, -MAX_WBITS); zerr != Z_OK) {
    ec = bela::make_error_code(ErrGeneral, bela::encode_into<char, wchar_t>(zng_zError(zerr)));
    return false;
  }
  auto closer = bela::finally([&] { zng_inflateEnd(&zs); });
  Buffer out(outsize);
  Buffer in(insize);
  int64_t uncsize = 0;
  auto csize = file.compressed_size;
  int ret = Z_OK;
  Summator sum(file.crc32_value);
  while (csize != 0) {
    auto minsize = (std::min)(csize, static_cast<uint64_t>(insize));
    if (!fd.ReadFull({in.data(), static_cast<size_t>(minsize)}, ec)) {
      return false;
    }
    zs.avail_in = static_cast<int>(minsize);
    if (zs.avail_in == 0) {
      break;
    }
    zs.next_in = in.data();
    do {
      zs.avail_out = static_cast<int>(outsize);
      zs.next_out = out.data();
      ret = ::zng_inflate(&zs, Z_NO_FLUSH);
      switch (ret) {
      case Z_NEED_DICT:
        ret = Z_DATA_ERROR;
        [[fallthrough]];
      case Z_DATA_ERROR:
        [[fallthrough]];
      case Z_MEM_ERROR:
        ec = bela::make_error_code(ret, bela::encode_into<char, wchar_t>(zng_zError(ret)));
        return false;
      default:
        break;
      }
      auto have = outsize - zs.avail_out;
      sum.Update(out.data(), have); // CRC32 update
      if (!w(out.data(), have)) {
        ec = bela::make_error_code(ErrCanceled, L"canceled");
        return false;
      }
    } while (zs.avail_out == 0);
    csize -= minsize;
    if (ret == Z_STREAM_END) {
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