#include <bela/semver.hpp>
#include <bela/terminal.hpp>

int wmain() {
  bela::semver::version v1(L"7.8.1-rc.1");
  bela::semver::version v2("7.8.1.0-rc.1");
  bela::semver::version v3(u8"7.8.1-rc.1");
  bela::FPrintF(stderr, L"v1(%s)%s=v2(%s)\n", v1.make_string_version(), (v1 == v2 ? L"=" : L"!"),
                v2.make_string_version<char>());
  bela::FPrintF(stderr, L"version: %s\n", v3.make_string_version<char8_t>());
  return 0;
}