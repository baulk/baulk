#include <bela/stdwriter.hpp>
#include <bela/match.hpp>
#include <bela/strip.hpp>
#include <version.hpp>

inline std::wstring_view cleanup_version(std::wstring_view ver) {
  return bela::StripPrefix(ver, L"v");
}

int wmain() {
  const std::wstring_view vs[] = {L"19.00",
                                  L"2020-02-25/2.2.0-24-gb93c271",
                                  L"3.2",
                                  L"1.35.0",
                                  L"v0.13.0",
                                  L"2.091.0",
                                  L"v8.1.0.0p1-Beta",
                                  L"0.49",
                                  L"v7.5.0",
                                  L"10.0.19041.153",
                                  L"7.69.1",
                                  L"1.0-rc.1",
                                  L"1.2.3-rc.4"};
  for (auto v : vs) {
    baulk::version::version sv(cleanup_version(v));
    bela::FPrintF(stderr, L"resolve version: %s to %s\n", v, sv.to_wstring());
  }
  return 0;
}