///
#include "zipinternal.hpp"
#include <bela/endian.hpp>

namespace hazel::zip {

bool Reader::Decompress(const File &file, const Receiver &receiver, bela::error_code &ec) const {
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
    auto compressedSize = file.compressedSize;
    while (compressedSize != 0) {
      auto minsize = (std::min)(compressedSize, static_cast<uint64_t>(sizeof(buffer)));
      if (!ReadFull(buffer, static_cast<size_t>(minsize), ec)) {
        return false;
      }
      if (!receiver(buffer, static_cast<size_t>(minsize))) {
        return false;
      }
      compressedSize -= minsize;
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
  case ZIP_XZ:
    break;
  case ZIP_BZIP2:
    break;
  default:
    ec = bela::make_error_code(1, L"support zip method ", file.method);
    return false;
  }
  return true;
}

} // namespace hazel::zip