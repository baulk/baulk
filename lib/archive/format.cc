///
#include <bela/io.hpp>
#include <bela/pe.hpp>
#include <baulk/archive.hpp>
#include <utility>
#include "tar/tarinternal.hpp"

namespace baulk::archive {

struct oleheader_t {
  uint32_t id[2]; // D0CF11E0 A1B11AE1
  uint32_t clid[4];
  uint16_t verminor; // 0x3e
  uint16_t verdll;   // 0x03
  uint16_t byteorder;
  uint16_t lsectorB;
  uint16_t lssectorB;

  uint16_t reserved1;
  uint32_t reserved2;
  uint32_t reserved3;

  uint32_t cfat; // count full sectors
  uint32_t dirstart;

  uint32_t reserved4;

  uint32_t sectorcutoff; // min size of a standard stream ; if less than this
                         // then it uses short-streams
  uint32_t sfatstart;    // first short-sector or EOC
  uint32_t csfat;        // count short sectors
  uint32_t difstart;     // first sector master sector table or EOC
  uint32_t cdif;         // total count
  uint32_t MSAT[109];    // First 109 MSAT
};

const wchar_t *FormatToMIME(file_format_t t) {
  struct name_table {
    file_format_t t;
    const wchar_t *name;
  };
  constexpr name_table tables[] = {
      /// archive
      {.t = file_format_t::epub, .name = L"application/epub"}, //
      {.t = file_format_t::zip, .name = L"application/zip"},
      {.t = file_format_t::tar, .name = L"application/x-tar"},
      {.t = file_format_t::rar, .name = L"application/vnd.rar"},
      {.t = file_format_t::gz, .name = L"application/gzip"},
      {.t = file_format_t::bz2, .name = L"application/x-bzip2"},
      {.t = file_format_t::zstd, .name = L"application/x-zstd"},
      {.t = file_format_t::_7z, .name = L"application/x-7z-compressed"},
      {.t = file_format_t::xz, .name = L"application/x-xz"},
      {.t = file_format_t::eot, .name = L"application/octet-stream"},
      {.t = file_format_t::crx, .name = L"application/x-google-chrome-extension"},
      {.t = file_format_t::deb, .name = L"application/vnd.debian.binary-package"},
      {.t = file_format_t::lz, .name = L"application/x-lzip"},
      {.t = file_format_t::rpm, .name = L"application/x-rpm"},
      {.t = file_format_t::cab, .name = L"application/vnd.ms-cab-compressed"},
      {.t = file_format_t::msi, .name = L"application/x-msi"},
      {.t = file_format_t::dmg, .name = L"application/x-apple-diskimage"},
      {.t = file_format_t::xar, .name = L"application/x-xar"},
      {.t = file_format_t::wim, .name = L"application/x-ms-wim"},
      {.t = file_format_t::z, .name = L"application/x-compress"}, //
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
// # Zstandard/LZ4 skippable frames
// # https://github.com/facebook/zstd/blob/dev/zstd_compression_format.md
// 0         lelong&0xFFFFFFF0  0x184D2A50
constexpr const uint8_t xzMagic[] = {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00};
constexpr const uint8_t gzMagic[] = {0x1F, 0x8B, 0x8};
constexpr const uint8_t bz2Magic[] = {0x42, 0x5A, 0x68};
constexpr const uint8_t lzMagic[] = {0x4C, 0x5A, 0x49, 0x50};
constexpr const uint8_t nsisSignature[] = {0xEF, 0xBE, 0xAD, 0xDE, 'N', 'u', 'l', 'l',
                                           's',  'o',  'f',  't',  'I', 'n', 's', 't'};

constexpr bool is_zip_magic(const uint8_t *buf, size_t size) {
  return (size > 3 && buf[0] == 0x50 && buf[1] == 0x4B && (buf[2] == 0x3 || buf[2] == 0x5 || buf[2] == 0x7) &&
          (buf[3] == 0x4 || buf[3] == 0x6 || buf[3] == 0x8));
}

bool is_msi_archive(const bela::bytes_view &bv) {
  constexpr const uint8_t msoleMagic[] = {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};
  constexpr const auto olesize = sizeof(oleheader_t);
  if (!bv.starts_bytes_with(msoleMagic) || bv.size() < 520) {
    return false;
  }
  if (bv[512] == 0xEC && bv[513] == 0xA5) {
    // L"Microsoft Word 97-2003"
    return false;
  }
  if (bv[512] == 0x09 && bv[513] == 0x08) {
    // L"Microsoft Excel 97-2003"
    return false;
  }
  if (bv[512] == 0xA0 && bv[513] == 0x46) {
    // L"Microsoft PowerPoint 97-2003"
    return false;
  }
  return true;
}

file_format_t analyze_format_internal(const bela::bytes_view &bv) {
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
  if (bv.match_with(4, nsisSignature, std::size(nsisSignature))) {
    return file_format_t::nsis;
  }
  if (bv.size() >= 512) {
    if (auto uh = bv.unchecked_cast<tar::ustar_header>(); getFormat(*uh) != tar::FormatUnknown) {
      return file_format_t::tar;
    }
  }
  if (is_msi_archive(bv)) {
    return file_format_t::msi;
  }
  return file_format_t::none;
}

constexpr size_t magic_size = 1024;

bool CheckFormat(bela::io::FD &fd, file_format_t &afmt, int64_t &offset, bela::error_code &ec) {
  uint8_t magicBytes[magic_size] = {0};
  int64_t outlen = 0;
  if (!fd.ReadAt(magicBytes, 0, outlen, ec)) {
    return false;
  }
  if (afmt = analyze_format_internal(bela::bytes_view(magicBytes, static_cast<size_t>(outlen)));
      afmt != file_format_t::exe) {
    return true;
  }
  bela::pe::File pefile;
  if (!pefile.NewFile(fd.NativeFD(), bela::SizeUnInitialized, ec)) {
    return false;
  }
  if (std::cmp_less(pefile.OverlayLength(), magic_size)) {
    // EXE
    return true;
  }
  offset = pefile.OverlayOffset();
  if (!fd.ReadAt(magicBytes, offset, outlen, ec)) {
    return false;
  }
  if (auto nfmt = analyze_format_internal(bela::bytes_view(magicBytes, static_cast<size_t>(outlen)));
      nfmt != file_format_t::none) {
    afmt = nfmt;
  }
  return true;
}

} // namespace baulk::archive