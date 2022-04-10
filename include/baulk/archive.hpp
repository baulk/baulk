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
constexpr long ErrExtractGeneral = 800000;
constexpr long ErrAnotherWay = 800001;
constexpr long ErrNoOverlayArchive = 800002;
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
inline bool MakeDirectories(const fs::path &path, bela::Time modified, bela::error_code &ec) {
  std::error_code e;
  if (fs::create_directories(path, e); e) {
    ec = bela::make_error_code_from_std(e, L"fs::create_directories() ");
    return false;
  }
  return baulk::archive::Chtimes(path, modified, ec);
}

bool NewSymlink(const fs::path &path, const fs::path &source, bool overwrite_mode, bela::error_code &ec);

std::wstring_view PathStripExtension(std::wstring_view p);

inline std::wstring FileDestination(std::wstring_view arfile) {
  auto ends_with_path_separator = [](std::wstring_view p) -> bool {
    if (p.empty()) {
      return false;
    }
    return bela::IsPathSeparator(p.back());
  };
  if (auto d = PathStripExtension(arfile); d.size() != arfile.size() && !ends_with_path_separator(d)) {
    return std::wstring(d);
  }
  return bela::StringCat(arfile, L".out");
}

std::optional<std::wstring> JoinSanitizePath(std::wstring_view root, std::string_view child_path,
                                             bool always_utf8 = true);
//
std::wstring EncodeToNativePath(std::string_view filename, bool always_utf8);
bool IsHarmfulPath(std::string_view child_path);
std::optional<fs::path> JoinSanitizeFsPath(const fs::path &root, std::string_view child_path, bool always_utf8,
                                           std::wstring &encoded_path);

//
bool CheckFormat(bela::io::FD &fd, file_format_t &afmt, int64_t &offset, bela::error_code &ec);
// OpenFile open file and detect archive file format and offset
inline std::optional<bela::io::FD> OpenFile(std::wstring_view file, int64_t &offset, file_format_t &afmt,
                                            bela::error_code &ec) {
  if (auto fd = bela::io::NewFile(file, ec); fd) {
    if (CheckFormat(*fd, afmt, offset, ec)) {
      return std::move(fd);
    }
  }
  return std::nullopt;
}

} // namespace baulk::archive

#endif