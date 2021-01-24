///
#include "tarinternal.hpp"
#include <bela/endian.hpp>
#include "zstd.hpp"
#include "bzip.hpp"
#include "brotli.hpp"
#include "gzip.hpp"
#include "xz.hpp"

namespace baulk::archive::tar {
// https://github.com/file/file/blob/6fc66d12c0ca172f4681adb63c6f662ac33cbc7c/magic/Magdir/compress
// https://github.com/facebook/zstd/blob/dev/doc/zstd_compression_format.md
//# Zstandard/LZ4 skippable frames
//# https://github.com/facebook/zstd/blob/dev/zstd_compression_format.md
// 0         lelong&0xFFFFFFF0  0x184D2A50
constexpr const uint8_t xzMagic[] = {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00};
constexpr const uint8_t gzMagic[] = {0x1F, 0x8B, 0x8};
constexpr const uint8_t bz2Magic[] = {0x42, 0x5A, 0x68};
constexpr const uint8_t lzMagic[] = {0x4C, 0x5A, 0x49, 0x50};

// MakeReader make reader
std::shared_ptr<ExtractReader> MakeReader(FileReader &fd, bela::error_code &ec) {
  uint8_t magic[512];
  auto n = fd.ReadAt(magic, sizeof(magic), 0, ec);
  if (n < 4) {
    return nullptr;
  }
  if (!fd.PositionAt(0, ec)) {
    return nullptr;
  }
  if (memcmp(xzMagic, magic, sizeof(xzMagic)) == 0) {
    if (auto r = std::make_shared<xz::Reader>(&fd); r->Initialize(ec)) {
      return r;
    }
    return nullptr;
  }
  if (memcmp(gzMagic, magic, sizeof(gzMagic)) == 0) {
    if (auto r = std::make_shared<gzip::Reader>(&fd); r->Initialize(ec)) {
      return r;
    }
    return nullptr;
  }
  if (memcmp(bz2Magic, magic, sizeof(bz2Magic)) == 0) {
    if (auto r = std::make_shared<bzip::Reader>(&fd); r->Initialize(ec)) {
      return r;
    }
    return nullptr;
  }
  // ZSTD
  auto zstdmagic = bela::cast_fromle<uint32_t>(magic);
  if (zstdmagic == 0xFD2FB528U || (zstdmagic & 0xFFFFFFF0) == 0x184D2A50) {
    if (auto r = std::make_shared<zstd::Reader>(&fd); r->Initialize(ec)) {
      return r;
    }
    return nullptr;
  }
  // vaild is good tar file. must >=512 bytes
  if (n == 512) {
    if (auto uh = reinterpret_cast<const ustar_header *>(magic); getFormat(*uh) != FormatUnknown) {
      ec = bela::make_error_code(ErrNoFilter, L"tar no filter");
      return nullptr;
    }
  }
  auto p = bela::RealPathByHandle(fd.FD(), ec);
  if (!p) {
    return nullptr;
  }
  // support brotli decoder
  if (bela::EndsWithIgnoreCase(*p, L".tar.br") || bela::EndsWithIgnoreCase(*p, L".tbr")) {
    if (auto r = std::make_shared<brotli::Reader>(&fd); r->Initialize(ec)) {
      return r;
    }
    return nullptr;
  }
  ec = bela::make_error_code(ErrNotTarFile, L"not a tar file");
  return nullptr;
}
} // namespace baulk::archive::tar
