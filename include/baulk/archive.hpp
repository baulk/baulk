#ifndef BAULK_ARCHIVE_HPP
#define BAULK_ARCHIVE_HPP
#include <bela/os.hpp>
#include <bela/base.hpp>
#include <bela/time.hpp>
#include <bela/io.hpp>
#include <functional>
#include "archive/format.hpp"

namespace baulk::archive {
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

inline std::wstring FileDestination(std::wstring_view arfile) {
  if (auto d = PathRemoveExtension(arfile); d.size() != arfile.size()) {
    return std::wstring(d);
  }
  return bela::StringCat(arfile, L".out");
}

std::optional<std::wstring> JoinSanitizePath(std::wstring_view root, std::string_view filename,
                                             bool always_utf8 = true);

bool CheckArchiveFormat(bela::io::FD &fd, file_format_t &afmt, int64_t &offset, bela::error_code &ec);
// OpenArchiveFile open file and detect archive file format and offset
inline std::optional<bela::io::FD> OpenArchiveFile(std::wstring_view file, file_format_t &afmt, int64_t &offset,
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