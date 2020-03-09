#include <bela/path.hpp>
#include <bela/stdwriter.hpp>

int wmain() {
  bela::error_code ec;
  auto e = bela::Executable(ec);
  if (!e) {
    bela::FPrintF(stderr, L"FAILED: Executable: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"Executable: '%s'\n", *e);
  auto p = bela::ExecutablePath(ec);
  if (!p) {
    bela::FPrintF(stderr, L"FAILED: ExecutablePath: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"ExecutablePath: '%s'\n", *p);
  return 0;
}