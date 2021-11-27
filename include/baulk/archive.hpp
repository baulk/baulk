#ifndef BAULK_ARCHIVE_HPP
#define BAULK_ARCHIVE_HPP
#include <bela/os.hpp>
#include <bela/base.hpp>
#include <bela/time.hpp>
#include <bela/io.hpp>
#include <functional>

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
  z,
  exe, // Currently only supports PE self-extracting files (ELF/Mach-O) not currently supported
};
const wchar_t *ArchiveFormatName(file_format_t t);
using bela::ErrCanceled;
using bela::ErrEnded;
using bela::ErrGeneral;
using bela::ErrUnimplemented;

class File {
public:
  File(File &&o) noexcept;
  File &operator=(File &&o) noexcept;
  File(const File &) = delete;
  File &operator=(const File &) = delete;
  ~File();
  bool WriteFull(const void *data, size_t bytes, bela::error_code &ec);
  bool Discard();
  bool Chtimes(bela::Time t, bela::error_code &ec);
  static std::optional<File> NewFile(std::wstring_view path, bool overwrite, bela::error_code &ec);

private:
  File() = default;
  HANDLE fd{INVALID_HANDLE_VALUE};
};

bool Chtimes(std::wstring_view file, bela::Time t, bela::error_code &ec);
bool NewSymlink(std::wstring_view path, std::wstring_view linkname, bela::error_code &ec, bool overwrite = false);
std::wstring_view PathRemoveExtension(std::wstring_view p);
std::optional<std::wstring> JoinSanitizePath(std::wstring_view root, std::string_view filename,
                                             bool always_utf8 = true);

std::optional<bela::io::FD> NewArchiveFile(std::wstring_view file, file_format_t &afmt, int64_t &offset,
                                           bela::error_code &ec);

class Extractor {
public:
  virtual bool Extract(bela::error_code &ec) = 0;
};

struct ExtractorOptions {
  std::function<bool(const std::wstring_view filename)> new_file;
  std::function<bool(int64_t total, int64_t decompressed)> show_progress;
};

std::shared_ptr<Extractor> NewExtractor(std::wstring_view filename, std::wstring_view chroot,
                                        const ExtractorOptions &opts, bela::error_code &ec);

} // namespace baulk::archive

#endif