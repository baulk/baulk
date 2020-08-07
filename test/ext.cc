#include <bela/terminal.hpp>
#include <bela/match.hpp>
#include <bela/strcat.hpp>
#include <bela/path.hpp>

inline std::wstring UnarchivePath(std::wstring_view path) {
  auto dir = bela::DirName(path);
  auto filename = bela::BaseName(path);
  auto extName = bela::ExtensionEx(filename);
  if (filename.size() <= extName.size()) {
    return bela::StringCat(dir, L"\\out");
  }
  if (extName.empty()) {
    return bela::StringCat(dir, L"\\", filename, L".out");
  }
  return bela::StringCat(dir, L"\\", filename.substr(0, filename.size() - extName.size()));
}

int wmain() {
  constexpr std::wstring_view files[] = {
      L"out", //
      L"out.exe", L"C:\\Windows\\Notepad.exe", L"some.tar.gz",
      L".tgz",    L"C:\\Jacksome\\note\\",     L"C:\\Jacksome\\note\\zzzz.tar.gz"};
  for (auto f : files) {
    bela::FPrintF(stderr, L"[%s] --> [%s]\n", f, UnarchivePath(f));
  }
  return 0;
}