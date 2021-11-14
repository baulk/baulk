///
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <filesystem>

constexpr bool is_dot_or_separator(wchar_t ch) { return bela::IsPathSeparator(ch) || ch == L'.'; }

std::wstring_view PathRemoveExtension(std::wstring_view p) {
  if (p.empty()) {
    return L".";
  }
  auto i = p.size() - 1;
  for (; i != 0 && is_dot_or_separator(p[i]); i--) {
    p.remove_suffix(1);
  }
  constexpr std::wstring_view extensions[] = {L".tar.gz",   L".tar.bz2", L".tar.xz", L".tar.zst",
                                              L".tar.zstd", L".tar.br",  L".tar.lz4"};
  for (const auto e : extensions) {
    if (bela::EndsWithIgnoreCase(p, e)) {
      return p.substr(0, p.size() - e.size());
    }
  }
  if (auto pos = p.find_last_of(L"\\/."); pos != std::wstring_view::npos && p[pos] == L'.') {
    // if (pos >= 1 && bela::IsPathSeparator(p[pos - 1])) {
    //   return p;
    // }
    return p.substr(0, pos);
  }
  return p;
}

int wmain() {
  constexpr std::wstring_view paths[] = {L"",
                                         L"//////////////////",
                                         L"/home/path/some.tgz",
                                         L"/home/path/folder",
                                         L"/home/path/some.tgz//////////////",
                                         L"/home/path/some.tgz.tar.gz",
                                         L"C:/jackson.zip",
                                         L"/home/path/some.tgz....",
                                         L"/home/path/.zip"};
  for (const auto sv : paths) {
    bela::FPrintF(stderr, L"[%s]--> %s\n", sv, PathRemoveExtension(sv));
  }
  return 0;
}