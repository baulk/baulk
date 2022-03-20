#ifndef BAULK_ARCHIVE_HPP
#define BAULK_ARCHIVE_HPP
#include <bela/os.hpp>
#include <bela/base.hpp>
#include <bela/time.hpp>
#include <bela/io.hpp>
#include <functional>
#include <filesystem>
#include "archive/format.hpp"

namespace baulk::archive {
using bela::ErrCanceled;
using bela::ErrEnded;
using bela::ErrGeneral;
using bela::ErrUnimplemented;
namespace fs = std::filesystem;
class File {
public:
  File(HANDLE fd_) : fd(fd_) {}
  File(File &&o) noexcept;
  File &operator=(File &&o) noexcept;
  File(const File &) = delete;
  File &operator=(const File &) = delete;
  ~File();
  bool WriteFull(const void *data, size_t bytes, bela::error_code &ec);
  bool Discard();
  bool Chtimes(bela::Time t, bela::error_code &ec);
  static std::optional<File> NewFile(const fs::path &path, bela::Time modified, bool overwrite_mode,
                                     bela::error_code &ec);

private:
  File() = default;
  HANDLE fd{INVALID_HANDLE_VALUE};
};

bool Chtimes(const fs::path &file, bela::Time t, bela::error_code &ec);
bool NewSymlink(const fs::path &path, const fs::path &source, bool overwrite_mode, bela::error_code &ec);

std::wstring_view PathRemoveExtension(std::wstring_view p);

inline std::wstring FileDestination(std::wstring_view arfile) {
  auto ends_with_path_separator = [](std::wstring_view p) -> bool {
    if (p.empty()) {
      return false;
    }
    return bela::IsPathSeparator(p.back());
  };
  if (auto d = PathRemoveExtension(arfile); d.size() != arfile.size() && !ends_with_path_separator(d)) {
    return std::wstring(d);
  }
  return bela::StringCat(arfile, L".out");
}

std::optional<std::wstring> JoinSanitizePath(std::wstring_view root, std::string_view child_path,
                                             bool always_utf8 = true);
std::optional<fs::path> JoinSanitizeFsPath(const fs::path &root, std::string_view child_path, bool always_utf8,
                                           std::wstring &encoded_path);
bool CheckArchiveFormat(bela::io::FD &fd, file_format_t &afmt, int64_t &offset, bela::error_code &ec);
// OpenArchiveFile open file and detect archive file format and offset
inline std::optional<bela::io::FD> OpenArchiveFile(std::wstring_view file, int64_t &offset, file_format_t &afmt,
                                                   bela::error_code &ec) {
  if (auto fd = bela::io::NewFile(file, ec); fd) {
    if (CheckArchiveFormat(*fd, afmt, offset, ec)) {
      return std::move(fd);
    }
  }
  return std::nullopt;
}

} // namespace baulk::archive

#endif