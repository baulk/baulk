#include <bela/match.hpp>
#include <bela/terminal.hpp>

struct kv {
  const wchar_t *a;
  const wchar_t *b;
};

int wmain() {
  constexpr const kv kvv[] = {
      {L"jack", L"kj"}, {L"mmmvn", L"mv"},     {L"helloworld", L"he"},
      {L"world", L""},  {L"xxxxwwe3we", L"x"}, {L"xxxmmmv", L"mv"},
  };

  for (const auto &k : kvv) {
    bela::FPrintF(stderr, L"%s startswith %s %v\n", k.a, k.b, bela::StartsWith(k.a, k.b));
    bela::FPrintF(stderr, L"%s endswith %s %v\n", k.a, k.b, bela::EndsWith(k.a, k.b));
  }
  return 0;
}