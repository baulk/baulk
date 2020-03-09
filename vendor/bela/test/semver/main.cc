#include <bela/semver.hpp>
#include <bela/stdwriter.hpp>

int wmain() {
  bela::semver::version v1(L"7.8.1-rc.1");
  bela::semver::version v2("7.8.1-rc.1");
  if (v1 == v2) {
    bela::FPrintF(stderr, L"v1(%s)==v2(%s)\n", v1.to_wstring(), v2.to_string());
    return 0;
  }
  bela::FPrintF(stderr, L"v1(%s)!=v2(%s)\n", v1.to_wstring(), v2.to_string());
  return 0;
}