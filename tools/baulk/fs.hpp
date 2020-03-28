#ifndef BAULK_FS_HPP
#define BAULK_FS_HPP
#include <bela/base.hpp>
#include <optional>
#include <string_view>
#include <filesystem>

namespace baulk::fs {
inline constexpr bool DirSkipFaster(const wchar_t *dir) {
  return (dir[0] == L'.' &&
          (dir[1] == L'\0' || (dir[1] == L'.' && dir[2] == L'\0')));
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
  bool IsDir() const {
    return (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  }
  std::wstring_view Name() const { return std::wstring_view(wfd.cFileName); }
  bool Next() { return FindNextFileW(hFind, &wfd) == TRUE; }
  bool First(std::wstring_view dir, std::wstring_view suffix,
             bela::error_code &ec) {
    auto d = bela::StringCat(dir, L"\\", suffix);
    hFind = FindFirstFileW(d.data(), &wfd);
    if (hFind == INVALID_HANDLE_VALUE) {
      ec = bela::make_system_error_code();
      return false;
    }
    return true;
  }

private:
  HANDLE hFind{INVALID_HANDLE_VALUE};
  WIN32_FIND_DATAW wfd;
};

bool IsExecutablePath(std::wstring_view p);
std::optional<std::wstring> FindExecutablePath(std::wstring_view p);

inline std::wstring BaseName(std::wstring_view p) {
  return std::filesystem::path(p).parent_path().wstring();
}

inline std::wstring FileName(std::wstring_view p) {
  return std::filesystem::path(p).filename().wstring();
}

inline bool PathRemove(std::wstring_view path, bela::error_code &ec) {
  std::error_code e;
  if (!std::filesystem::remove_all(path, e)) {
    ec = bela::from_std_error_code(e);
    return false;
  }
  return true;
}

inline bool MakeDir(std::wstring_view path, bela::error_code &ec) {
  std::error_code e;
  if (!std::filesystem::create_directories(path, e)) {
    ec = bela::from_std_error_code(e);
    return false;
  }
  return true;
}

inline bool SymLink(std::wstring_view _To, std::wstring_view NewLink,
                    bela::error_code &ec) {
  std::error_code e;
  std::filesystem::create_symlink(_To, NewLink, e);
  if (e) {
    ec = bela::from_std_error_code(e);
    return false;
  }
  return true;
}
std::optional<std::wstring> UniqueSubdirectory(std::wstring_view dir);
bool FlatPackageInitialize(std::wstring_view dir, std::wstring_view dest,
                           bela::error_code &ec);
std::optional<std::wstring> BaulkMakeTempDir(bela::error_code &ec);
} // namespace baulk::fs

#endif