///
#ifndef BELA_FS_HPP
#define BELA_FS_HPP
#include "base.hpp"
#include <stdio.h>

#ifdef BELA_FS_DISABLE_DEPRECATED_WARNING
#define BELA_FS_DEPRECATE_REMOVE
#define BELA_FS_DEPRECATE_REMOVE_ALL
#else
#define BELA_FS_DEPRECATE_REMOVE [[deprecated("bela::fs::Remove is deprecated. Please use bela::fs::ForceDeleteFile")]]
#define BELA_FS_DEPRECATE_REMOVE_ALL                                                                                   \
  [[deprecated("bela::fs::RemoveAll is deprecated. Please use bela::fs::ForceDeleteFolders")]]
#endif

namespace bela::fs {
constexpr bool DirSkipFaster(const wchar_t *dir) {
  return (dir[0] == L'.' && (dir[1] == L'\0' || (dir[1] == L'.' && dir[2] == L'\0')));
}

class Finder {
public:
  Finder() noexcept = default;
  Finder(const Finder &) = delete;
  Finder &operator=(const Finder &) = delete;
  ~Finder() noexcept {
    if (hFind != INVALID_HANDLE_VALUE) {
      FindClose(hFind);
    }
  }
  const WIN32_FIND_DATAW &FD() const { return wfd; }
  bool Ignore() const { return DirSkipFaster(wfd.cFileName); }
  bool IsDir() const { return (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }
  bool IsReparsePoint() const { return (wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0; }
  int64_t Size() const {
    LARGE_INTEGER li{{
        .LowPart = wfd.nFileSizeLow,
        .HighPart = static_cast<LONG>(wfd.nFileSizeHigh),
    }};
    return li.QuadPart;
  }
  std::wstring_view Name() const { return std::wstring_view(wfd.cFileName); }
  bool Next() { return FindNextFileW(hFind, &wfd) == TRUE; }
  bool First(std::wstring_view file, bela::error_code &ec) {
    if (hFind != INVALID_HANDLE_VALUE) {
      ec = bela::make_error_code(L"Find handle not invalid");
      return false;
    }
    hFind = FindFirstFileW(file.data(), &wfd);
    if (hFind == INVALID_HANDLE_VALUE) {
      ec = bela::make_system_error_code();
      return false;
    }
    return true;
  }
  bool First(std::wstring_view dir, std::wstring_view suffix, bela::error_code &ec) {
    auto d = bela::StringCat(dir, L"\\", suffix);
    return First(d, ec);
  }

private:
  HANDLE hFind{INVALID_HANDLE_VALUE};
  WIN32_FIND_DATAW wfd;
};
// Remove remove file force
bool ForceDeleteFile(HANDLE FileHandle, bela::error_code &ec);
bool ForceDeleteFile(std::wstring_view path, bela::error_code &ec);
inline BELA_FS_DEPRECATE_REMOVE bool Remove(std::wstring_view path, bela::error_code &ec) {
  return ForceDeleteFile(path, ec);
}
bool ForceDeleteFolders(std::wstring_view path, bela::error_code &ec);
inline BELA_FS_DEPRECATE_REMOVE_ALL bool RemoveAll(std::wstring_view path, bela::error_code &ec) {
  return ForceDeleteFolders(path, ec);
}
} // namespace bela::fs

#endif