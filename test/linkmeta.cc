#include <bela/path.hpp>
#include <bela/stdwriter.hpp>

struct LinkMeta {
  LinkMeta() = default;
  LinkMeta(std::wstring_view path_, std::wstring_view alias_)
      : path(path_), alias(alias_) {}
  LinkMeta(std::wstring_view sv) {
    if (auto pos = sv.find('@'); pos != std::wstring_view::npos) {
      path.assign(sv.data(), pos);
      alias.assign(sv.substr(pos + 1));
      return;
    }
    path.assign(sv);
    alias.assign(bela::BaseName(path));
  }
  std::wstring path;
  std::wstring alias;
};

int wmain() {
  std::vector<LinkMeta> lm;
  constexpr std::wstring_view launchers[] = {L"wget.exe",
                                             L"bin\\python.exe@python3.exe",
                                             L"cmd\\git.exe",
                                             L"cmdxx",
                                             L".",
                                             L"cmdxx\\@zzz.exe"};

  for (const auto l : launchers) {
    lm.emplace_back(l);
  }
  for (const auto &l : lm) {
    bela::FPrintF(stderr, L"Path: %s Alias: %s\n", l.path, l.alias);
  }
  return 0;
}