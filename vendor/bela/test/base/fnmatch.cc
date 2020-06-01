#include <bela/terminal.hpp>
#include <bela/fnmatch.hpp>

void round0() {
  constexpr const std::wstring_view rules[] = {L"dev", L"dev*", L"dev?", L"dev+X", L"dev!",
                                               L"car", L"*car", L"?car", L"爱*了"};
  constexpr const std::wstring_view strs[] = {L"dev",     L"devXX",    L"dev/jack", L"car",
                                              L"jackcar", L"jack/car", L"jack.car", L"爱不了"};
  bela::FPrintF(stderr, L"|Text\\Rules|");
  for (auto r : rules) {
    bela::FPrintF(stderr, L"`%s`|", r);
  }
  bela::FPrintF(stderr, L"\n|---");
  for (auto i = 0; i < std::size(rules); i++) {
    bela::FPrintF(stderr, L"|---");
  }
  bela::FPrintF(stderr, L"|\n");
  for (auto s : strs) {
    bela::FPrintF(stderr, L"|`%s`|", s);
    for (auto r : rules) {
      bela::FPrintF(stderr, L"`%b`|", bela::FnMatch(r, s, 0));
    }
    bela::FPrintF(stderr, L"\n");
  }
}

int wmain() {
  round0();
  return 0;
}