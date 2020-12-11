#include <bela/str_split.hpp>
#include <bela/terminal.hpp>

int wmain() {
  const wchar_t *paths[] = {
      L"/////////////Temp/bela/build",
  };
  for (const auto p : paths) {
    std::vector<std::wstring_view> pvv = bela::StrSplit(p, bela::ByChar('/'), bela::SkipEmpty());
    bela::FPrintF(stderr, L"path: %s\n", p);
    for (const auto e : pvv) {
      bela::FPrintF(stderr, L"      %s\n", e);
    }
  }

  return 0;
}