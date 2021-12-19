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
std::optional<std::wstring> JoinSanitizePath(std::wstring_view root, std::string_view filename,
                                             bool always_utf8 = true);

// OpenCompressedFile open file and detect archive file format and offset
std::optional<bela::io::FD> OpenCompressedFile(std::wstring_view file, file_format_t &afmt, int64_t &offset,
                                               bela::error_code &ec);

} // namespace baulk::archive

#endif