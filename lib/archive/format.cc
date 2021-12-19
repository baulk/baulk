///
#include <bela/io.hpp>
#include <bela/pe.hpp>
#include <baulk/archive.hpp>
#include "tar/tarinternal.hpp"

namespace baulk::archive {

const wchar_t *FormatToMIME(file_format_t t) {
  struct name_table {
    file_format_t t;
    const wchar_t *name;
  };
  constexpr name_table tables[] = {
      /// archive
      {file_format_t::epub, L"application/epub"}, //
      {file_format_t::zip, L"application/zip"},
      {file_format_t::tar, L"application/x-tar"},
      {file_format_t::rar, L"application/vnd.rar"},
      {file_format_t::gz, L"application/gzip"},
      {file_format_t::bz2, L"application/x-bzip2"},
      {file_format_t::zstd, L"application/x-zstd"},
      {file_format_t::_7z, L"application/x-7z-compressed"},
      {file_format_t::xz, L"application/x-xz"},
      {file_format_t::eot, L"application/octet-stream"},
      {file_format_t::crx, L"application/x-google-chrome-extension"},
      {file_format_t::deb, L"application/vnd.debian.binary-package"},
      {file_format_t::lz, L"application/x-lzip"},
      {file_format_t::rpm, L"application/x-rpm"},
      {file_format_t::cab, L"application/vnd.ms-cab-compressed"},
      {file_format_t::msi, L"application/x-msi"},
      {file_format_t::dmg, L"application/x-apple-diskimage"},
      {file_format_t::xar, L"application/x-xar"},
      {file_format_t::wim, L"application/x-ms-wim"},
      {file_format_t::z, L"application/x-compress"}, //
  };

  for (const auto &m : tables) {
    if (m.t == t) {
      return m.name;
    }
  }
  return L"application/octet-stream";
}

constexpr const unsigned k7zSignatureSize = 6;
constexpr const uint8_t k7zSignature[k7zSignatureSize] = {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C};
constexpr const uint8_t PEMagic[] = {'P', 'E', '\0', '\0'};
constexpr const uint8_t rarSignature[] = {0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x01, 0x00};
constexpr const uint8_t rar4Signature[] = {0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00};
constexpr const uint8_t xarSignature[] = {'x', 'a', 'r', '!'};
constexpr const uint8_t dmgSignature[] = {'k', 'o', 'l', 'y'};
constexpr const uint8_t wimMagic[] = {'M', 'S', 'W', 'I', 'M', 0x00, 0x00, 0x00};
constexpr const uint8_t cabMagic[] = {'M', 'S', 'C', 'F', 0, 0, 0, 0};
constexpr const uint8_t ustarMagic[] = {'u', 's', 't', 'a', 'r', 0};
constexpr const uint8_t gnutarMagic[] = {'u', 's', 't', 'a', 'r', ' ', ' ', 0};
constexpr const uint8_t debMagic[] = {0x21, 0x3C, 0x61, 0x72, 0x63, 0x68, 0x3E, 0x0A, 0x64, 0x65, 0x62,
                                      0x69, 0x61, 0x6E, 0x2D, 0x62, 0x69, 0x6E, 0x61, 0x72, 0x79};
// https://github.com/file/file/blob/6fc66d12c0ca172f4681adb63c6f662ac33cbc7c/magic/Magdir/compress
// https://github.com/facebook/zstd/blob/dev/doc/zstd_compression_format.md
//# Zstandard/LZ4 skippable frames
//# https://github.com/facebook/zstd/blob/dev/zstd_compression_format.md
// 0         lelong&0xFFFFFFF0  0x184D2A50
constexpr const uint8_t xzMagic[] = {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00};
constexpr const uint8_t gzMagic[] = {0x1F, 0x8B, 0x8};
constexpr const uint8_t bz2Magic[] = {0x42, 0x5A, 0x68};
constexpr const uint8_t lzMagic[] = {0x4C, 0x5A, 0x49, 0x50};

constexpr bool is_zip_magic(const uint8_t *buf, size_t size) {
  return (size > 3 && buf[0] == 0x50 && buf[1] == 0x4B && (buf[2] == 0x3 || buf[2] == 0x5 || buf[2] == 0x7) &&
          (buf[3] == 0x4 || buf[3] == 0x6 || buf[3] == 0x8));
}

file_format_t analyze_format_internal(bela::bytes_view bv) {
  if (is_zip_magic(bv.data(), bv.size())) {
    return file_format_t::zip;
  }
  if (bv.starts_bytes_with(xzMagic)) {
    return file_format_t::xz;
  }
  if (bv.starts_bytes_with(gzMagic)) {
    return file_format_t::gz;
  }
  if (bv.starts_bytes_with(bz2Magic)) {
    return file_format_t::bz2;
  }
  if (bv.starts_bytes_with(lzMagic)) {
    return file_format_t::lz;
  }
  auto zstdmagic = bv.cast_fromle<uint32_t>(0);
  if (zstdmagic == 0xFD2FB528U || (zstdmagic & 0xFFFFFFF0) == 0x184D2A50) {
    return file_format_t::zstd;
  }
  if (bv.starts_with("MZ") && bv.size() >= 0x3c + 4) {
    if (auto off = bela::cast_fromle<uint32_t>(bv.data() + 0x3c); bv.subview(off).starts_bytes_with(PEMagic)) {
      return file_format_t::exe;
    }
  }
  if (bv.starts_bytes_with(k7zSignature)) {
    return file_format_t::_7z;
  }
  if (bv.starts_bytes_with(rarSignature) || bv.starts_bytes_with(rar4Signature)) {
    return file_format_t::rar;
  }
  if (bv.starts_bytes_with(wimMagic)) {
    return file_format_t::wim;
  }
  if (bv.starts_bytes_with(cabMagic)) {
    return file_format_t::cab;
  }
  if (bv.starts_bytes_with(dmgSignature)) {
    return file_format_t::dmg;
  }
  if (bv.starts_bytes_with(debMagic)) {
    return file_format_t::deb;
  }
  if (bv.size() >= 512) {
    if (auto uh = bv.unchecked_cast<tar::ustar_header>(); getFormat(*uh) != tar::FormatUnknown) {
      return file_format_t::tar;
    }
  }
  return file_format_t::none;
}

constexpr size_t magic_size = 512;

std::optional<bela::io::FD> OpenCompressedFile(std::wstring_view file, file_format_t &afmt, int64_t &offset,
                                               bela::error_code &ec) {
  auto fd = bela::io::NewFile(file, ec);
  if (!fd) {
    return std::nullopt;
  }
  offset = 0;
  DWORD drSize = {0};
  uint8_t buffer[magic_size] = {0};
  if (::ReadFile(fd->NativeFD(), buffer, static_cast<DWORD>(magic_size), &drSize, nullptr) != TRUE) {
    ec = bela::make_system_error_code(L"ReadFile: ");
    return std::nullopt;
  }
  if (afmt = analyze_format_internal(bela::bytes_view(buffer, static_cast<size_t>(drSize)));
      afmt != file_format_t::exe) {
    return std::move(fd);
  }
  bela::pe::File pefile;
  if (!pefile.NewFile(fd->NativeFD(), bela::SizeUnInitialized, ec)) {
    return std::nullopt;
  }
  if (pefile.OverlayLength() < magic_size) {
    return std::nullopt;
  }
  offset = pefile.OverlayOffset();
  if (!fd->Seek(offset, ec)) {
    return std::nullopt;
  }
  if (::ReadFile(fd->NativeFD(), buffer, static_cast<DWORD>(magic_size), &drSize, nullptr) != TRUE) {
    ec = bela::make_system_error_code(L"ReadFile: ");
    return std::nullopt;
  }
  afmt = analyze_format_internal(bela::bytes_view(buffer, static_cast<size_t>(drSize)));
  return std::move(fd);
}

} // namespace baulk::archive