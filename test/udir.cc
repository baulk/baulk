///
#include <bela/path.hpp>
#include <bela/match.hpp>
#include <bela/terminal.hpp>
#include <filesystem>

inline bool DirSkipFaster(const wchar_t *dir) {
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
  std::wstring_view Name() const { return std::wstring_view(wfd.cFileName); }
  bool Next() { return FindNextFileW(hFind, &wfd) == TRUE; }
  bool First(std::wstring_view dir, std::wstring_view suffix, bela::error_code &ec) {
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
    bela::FPrintF(stderr, L"bad %s\n", ec.message);
    return std::nullopt;
  }
  int count = 0;
  std::wstring subdir;
  do {
    if (finder.Ignore()) {
      continue;
    }
    bela::FPrintF(stderr, L"%s\n", finder.Name());
    if (!finder.IsDir()) {
      bela::FPrintF(stderr, L"not dir %s\n", ec.message);
      return std::nullopt;
    }
    count++;
    if (subdir.empty()) {
      subdir = bela::StringCat(dir, L"\\", finder.Name());
    }
  } while (finder.Next());

  if (count != 1) {
    return std::nullopt;
  }
  bela::FPrintF(stderr, L"subdir: %s\n", subdir);
  return std::make_optional(std::move(subdir));
}

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s dir\n", argv[0]);
    return 1;
  }
  auto dir = std::filesystem::absolute(argv[1]).wstring();
  //   if (!dir.empty() && bela::IsPathSeparator(dir.back())) {
  //     dir.pop_back();
  //   }
  auto ud = UniqueSubdirectory(dir);
  if (ud) {
    bela::FPrintF(stderr, L"Unique subdir: %s\n", *ud);
    return 0;
  }
  bela::FPrintF(stderr, L"Dir no subdir: %s\n", dir);
  return 0;
}