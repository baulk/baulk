///
#include "zipinternal.hpp"
#include <bzlib.h>

namespace baulk::archive::zip {
// bzip2
bool Reader::decompressBz2(const File &file, const Writer &w, bela::error_code &ec) const {
  bz_stream bzs{nullptr};
  bzs.bzalloc = baulk::mem::allocate_bz;
  bzs.bzfree = baulk::mem::deallocate_simple;
  if (auto ret = BZ2_bzDecompressInit(&bzs, 0, 0); ret != BZ_OK) {
    ec = bela::make_error_code(ret, L"BZ2_bzDecompressInit error");
    return false;
  }
  auto closer = bela::finally([&] { BZ2_bzDecompressEnd(&bzs); });
  Buffer out(outsize);
  Buffer in(insize);
  int64_t uncsize = 0;
  auto csize = file.compressed_size;
  int ret = BZ_OK;
  Summator sum(file.crc32_value);
  while (csize != 0) {
    auto minsize = (std::min)(csize, static_cast<uint64_t>(insize));
    if (!fd.ReadFull({in.data(), static_cast<size_t>(minsize)}, ec)) {
      return false;
    }
    bzs.avail_in = static_cast<unsigned int>(minsize);
    bzs.next_in = reinterpret_cast<char *>(in.data());
    do {
      bzs.avail_out = static_cast<int>(outsize);
      bzs.next_out = reinterpret_cast<char *>(out.data());
      ret = BZ2_bzDecompress(&bzs);
      switch (ret) {
      case BZ_DATA_ERROR:
        [[fallthrough]];
      case BZ_MEM_ERROR:
        ec = bela::make_error_code(ret, L"bzlib error ", ret);
        return false;
      default:
        break;
      }
      auto have = outsize - bzs.avail_out;
      sum.Update(out.data(), have); // CRC32 update
      if (!w(out.data(), have)) {
        ec = bela::make_error_code(ErrCanceled, L"canceled");
        return false;
      }
    } while (bzs.avail_out == 0);
    csize -= minsize;
    if (ret == BZ_STREAM_END) {
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