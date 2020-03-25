#include <bela/stdwriter.hpp>
#include <bela/match.hpp>
#include <bela/strip.hpp>
#include <version.hpp>

inline std::wstring_view cleanup_version(std::wstring_view ver) {
  return bela::StripPrefix(ver, L"v");
}

int wmain() {
  const std::wstring_view vs[] = {L"19.00",   L"2020-02-25/2.2.0-24-gb93c271",
                                  L"3.2",     L"1.35.0",
                                  L"v0.13.0", L"2.091.0"};
  for (auto v : vs) {
    baulk::version::version sv(cleanup_version(v));
    bela::FPrintF(stderr, L"resolve version: %s to %s\n", v, sv.to_wstring());
  }
  return 0;
}