///
#include "zipinternal.hpp"
#include <bela/endian.hpp>
#include <zlib.h>
#include <zstd.h>
#include <bzlib.h>

namespace baulk::archive::zip {
// DEFLATE
bool Reader::decompressDeflate(const File &file, const Receiver &receiver, bela::error_code &ec) const {
  //
  return false;
}
// DEFLATE64
bool Reader::decompressDeflate64(const File &file, const Receiver &receiver, bela::error_code &ec) const {
  //
  return false;
}
// zstd
bool Reader::decompressZstd(const File &file, const Receiver &receiver, bela::error_code &ec) const {
  auto zds = ZSTD_createDStream();
  if (zds == nullptr) {
    ec = bela::make_error_code(L"ZSTD_createDStream() out of memory");
    return false;
  }
  auto closer = bela::finally([&] { ZSTD_freeDStream(zds); });
  //ZSTD_decompressStream(zds);
  return true;
}
// bzip2
bool Reader::decompressBz2(const File &file, const Receiver &receiver, bela::error_code &ec) const {
  //
  return false;
}
// XZ
bool Reader::decompressXz(const File &file, const Receiver &receiver, bela::error_code &ec) const {
  //
  return false;
}
// LZMA2
bool Reader::decompressLZMA2(const File &file, const Receiver &receiver, bela::error_code &ec) const {
  //
  return false;
}

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
    return decompressDeflate(file, receiver, ec);
  case ZIP_DEFLATE64:
    return decompressDeflate64(file, receiver, ec);
  case ZIP_ZSTD:
    return decompressZstd(file, receiver, ec);
  case ZIP_LZMA2:
    return decompressLZMA2(file, receiver, ec);
  case ZIP_XZ:
    return decompressXz(file, receiver, ec);
  case ZIP_BZIP2:
    return decompressBz2(file, receiver, ec);
  default:
    ec = bela::make_error_code(1, L"unsupport zip method ", file.method);
    return false;
  }
  return true;
}

} // namespace baulk::archive::zip