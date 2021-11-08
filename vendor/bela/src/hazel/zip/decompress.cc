///
#include "zipinternal.hpp"
#include <bela/endian.hpp>

namespace hazel::zip {
bool Reader::Decompress(const File &file, const Writer &w, bela::error_code &ec) const {
  auto realPosition = file.position + baseOffset;
  uint8_t buf[fileHeaderLen];
  if (!fd.ReadAt(buf, realPosition, ec)) {
    return false;
  }
  bela::endian::LittenEndian b(buf);
  if (auto sig = b.Read<uint32_t>(); sig != fileHeaderSignature) {
    ec = bela::make_error_code(L"zip: not a valid zip file");
    return false;
  }
  b.Discard(22);
  auto filenameLen = static_cast<int>(b.Read<uint16_t>());
  auto extraLen = static_cast<int>(b.Read<uint16_t>());
  auto position = realPosition + fileHeaderLen + filenameLen + extraLen;
  if (!fd.Seek(position, ec)) {
    return false;
  }
  switch (file.method) {
  case ZIP_STORE: {
    uint8_t buffer[4096];
    auto cSize = file.compressedSize;
    while (cSize != 0) {
      auto minsize = (std::min)(cSize, static_cast<uint64_t>(sizeof(buffer)));
      if (!fd.ReadFull({buffer, static_cast<size_t>(minsize)}, ec)) {
        return false;
      }
      if (!w(buffer, static_cast<size_t>(minsize))) {
        return false;
      }
      cSize -= minsize;
    }

  } break;
  case ZIP_DEFLATE:
    break;
  case ZIP_DEFLATE64:
    break;
  case ZIP_ZSTD:
    break;
  case ZIP_LZMA2:
    break;
  case ZIP_PPMD:
    break;
  case ZIP_XZ:
    break;
  case ZIP_BZIP2:
    break;
  default:
    ec = bela::make_error_code(ErrGeneral, L"support zip method ", file.method);
    return false;
  }
  return true;
}

} // namespace hazel::zip