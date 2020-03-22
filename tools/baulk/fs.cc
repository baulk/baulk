///
#include <bela/path.hpp>
#include <bela/match.hpp>
#include "fs.hpp"

namespace baulk::fs {
inline bool DirSkipFaster(const wchar_t *dir) {
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

std::optional<std::wstring> UniqueSubdirectory(std::wstring_view dir) {
  Finder finder;
  bela::error_code ec;
  if (!finder.First(dir, L"*", ec)) {
    return std::nullopt;
  }
  int count = 0;
  std::wstring subdir;
  do {
    if (finder.Ignore()) {
      continue;
    }
    if (!finder.IsDir()) {
      return std::nullopt;
    }
    if (subdir.empty()) {
      subdir = bela::StringCat(dir, L"\\", finder.Name());
    }
    count++;
  } while (finder.Next());

  if (count != 1) {
    return std::nullopt;
  }
  return std::make_optional(std::move(subdir));
}

bool IsExecutableSuffix(std::wstring_view name) {
  constexpr std::wstring_view suffixs[] = {L".exe", L".com", L".bat", L".cmd"};
  for (const auto s : suffixs) {
    if (bela::EndsWithIgnoreCase(name, s)) {
      return true;
    }
  }
  return false;
}

bool IsExecutablePath(std::wstring_view p) {
  bela::error_code ec;
  Finder finder;
  if (!finder.First(p, L"*", ec)) {
    return false;
  }
  do {
    if (finder.Ignore()) {
      continue;
    }
    if (finder.IsDir()) {
      continue;
    }
    if (IsExecutableSuffix(finder.Name())) {
      return true;
    }
  } while (finder.Next());
  return false;
}

std::optional<std::wstring> FindExecutablePath(std::wstring_view p) {
  if (!bela::PathExists(p, bela::FileAttribute::Dir)) {
    return std::nullopt;
  }
  if (IsExecutablePath(p)) {
    return std::make_optional<std::wstring>(p);
  }
  auto p2 = bela::StringCat(p, L"\\bin");
  if (IsExecutablePath(p2)) {
    return std::make_optional(std::move(p2));
  }
  p2 = bela::StringCat(p, L"\\cmd");
  if (IsExecutablePath(p2)) {
    return std::make_optional(std::move(p2));
  }
  return std::nullopt;
}

bool FlatPackageInitialize(std::wstring_view dir, std::wstring_view dest,
                           bela::error_code &ec) {
  auto subfirst = UniqueSubdirectory(dir);
  if (!subfirst) {
    return true;
  }
  std::wstring currentdir = *subfirst;
  for (int i = 0; i < 10; i++) {
    auto subdir_ = UniqueSubdirectory(currentdir);
    if (!subdir_) {
      break;
    }
    currentdir.assign(std::move(*subdir_));
  }
  std::error_code e;
  std::filesystem::path destpath(dest);
  for (const auto &p : std::filesystem::directory_iterator(currentdir)) {
    auto newpath = destpath / p.path().filename();
    std::filesystem::rename(p.path(), newpath, e);
  }
  if (!std::filesystem::remove_all(*subfirst, e)) {
    ec = bela::from_std_error_code(e);
    return false;
  }
  return true;
}

std::optional<std::wstring> BaulkMakeTempDir(bela::error_code &ec) {
  std::error_code e;
  auto tmppath = std::filesystem::temp_directory_path(e);
  if (e) {
    ec = bela::from_std_error_code(e);
    return std::nullopt;
  }
  auto tmpdir = tmppath.wstring();
  auto len = tmpdir.size();
  wchar_t X = 'A';
  bela::AlphaNum an(GetCurrentThreadId());
  for (wchar_t X = 'A'; X < 'Z'; X++) {
    bela::StrAppend(&tmpdir, L"\\BaulkTemp", X, an);
    if (!bela::PathExists(tmpdir, bela::FileAttribute::Dir) &&
        baulk::fs::MakeDir(tmpdir, ec)) {
      return std::make_optional(std::move(tmpdir));
    }
    tmpdir.resize(len);
  }
  ec = bela::make_error_code(1, L"cannot create tempdir");
  return std::nullopt;
}

} // namespace baulk::fs