///
#include <bela/endian.hpp>
#include "zipinternal.hpp"

namespace baulk::archive::zip {

bool Reader::Decompress(const File &file, const Receiver &receiver, int64_t &decompressed, bela::error_code &ec) const {
  uint8_t buf[fileHeaderLen];
  if (!ReadAt(buf, fileHeaderLen, file.position, ec)) {
    return false;
  }
  bela::endian::LittenEndian b(buf, sizeof(buf));
  if (auto sig = b.Read<uint32_t>(); sig != fileHeaderSignature) {
    ec = bela::make_error_code(L"zip: not a valid zip file");
    return false;
  }
  b.Discard(22);
  auto filenameLen = static_cast<int>(b.Read<uint16_t>());
  auto extraLen = static_cast<int>(b.Read<uint16_t>());
  auto position = file.position + fileHeaderLen + filenameLen + extraLen;
  if (!PositionAt(position, ec)) {
    return false;
  }
  switch (file.method) {
  case ZIP_STORE: {
    uint8_t buffer[4096];
    auto csize = file.compressedSize;
    while (csize != 0) {
      auto minsize = (std::min)(csize, static_cast<uint64_t>(sizeof(buffer)));
      if (!ReadFull(buffer, static_cast<size_t>(minsize), ec)) {
        return false;
      }
      if (!receiver(buffer, static_cast<size_t>(minsize))) {
        return false;
      }
      csize -= minsize;
    }
  } break;
  case ZIP_DEFLATE:
    return decompressDeflate(file, receiver, decompressed, ec);
  case ZIP_DEFLATE64:
    return decompressDeflate64(file, receiver, decompressed, ec);
  case 20:
    [[fallthrough]];
  case ZIP_ZSTD:
    return decompressZstd(file, receiver, decompressed, ec);
  case ZIP_LZMA2:
    return decompressLZMA2(file, receiver, decompressed, ec);
  case ZIP_XZ:
    return decompressXz(file, receiver, decompressed, ec);
  case ZIP_BZIP2:
    return decompressBz2(file, receiver, decompressed, ec);
  case ZIP_PPMD:
    return decompressPpmd(file, receiver, decompressed, ec);
  case ZIP_BROTLI:
    return decompressBrotli(file, receiver, decompressed, ec);
  default:
    ec = bela::make_error_code(1, L"unsupport zip method ", file.method);
    return false;
  }
  return true;
}

} // namespace baulk::archive::zip