///
#ifndef BAULK_ARCHIVE_FORMAT_HPP
#define BAULK_ARCHIVE_FORMAT_HPP
#include <cstdint>

namespace baulk::archive {
enum class file_format_t : uint32_t {
  none,
  /// archive
  epub,
  zip,
  tar,
  rar,
  gz,
  bz2,
  zstd,
  _7z,
  xz,
  eot,
  crx,
  deb,
  lz,
  rpm,
  cab,
  msi,
  dmg,
  xar,
  wim,
  nsis,
  z,
  brotli,
  exe, // Currently only supports PE self-extracting files (ELF/Mach-O) not currently supported
};
const wchar_t *FormatToMIME(file_format_t t);
} // namespace baulk::archive

#endif