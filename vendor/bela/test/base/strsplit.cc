#include <bela/str_split.hpp>
#include <bela/str_split_narrow.hpp>
#include <bela/str_join.hpp>
#include <bela/str_join_narrow.hpp>
#include <bela/str_replace.hpp>
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
    bela::FPrintF(stderr, L"Join: %s\n", bela::StrJoin(pvv, L"/"));
  }
  const char *Apaths[] = {
      "/////////////Temp/bela/build",
  };
  for (const auto p : Apaths) {
    std::vector<std::string_view> pvv = bela::narrow::StrSplit(p, bela::narrow::ByChar('/'), bela::narrow::SkipEmpty());
    bela::FPrintF(stderr, L"path: %s\n", p);
    for (const auto e : pvv) {
      bela::FPrintF(stderr, L"      %s\n", e);
    }
    bela::FPrintF(stderr, L"Join: %s\n", bela::narrow::StrJoin(pvv, "/"));
  }
  bela::FPrintF(stderr, L"%s\n", bela::StrReplaceAll("++++++++++++++++++++dexxxABCdefg", {{"de", "xx"}}));
  bela::FPrintF(stderr, L"%s\n", bela::StrReplaceAll(L"wwwwwwwwwwwwwwwwde~~~~~~~~~~~~ABCdefg", {{L"de", L"xx"}}));
  return 0;
}