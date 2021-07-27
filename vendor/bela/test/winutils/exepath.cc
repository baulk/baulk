#include <bela/path.hpp>
#include <bela/terminal.hpp>

int wmain() {
  bela::error_code ec;
  auto e = bela::Executable(ec);
  if (!e) {
    bela::FPrintF(stderr, L"FAILED: Executable: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"Executable: '%s'\n", *e);
  auto p = bela::ExecutableParent(ec);
  if (!p) {
    bela::FPrintF(stderr, L"FAILED: ExecutablePath: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"ExecutableParent: '%s'\n", *p);
  auto fp = bela::ExecutableFinalPath(ec);
  if (!fp) {
    bela::FPrintF(stderr, L"FAILED: ExecutableFinalPath: %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"ExecutableFinalPath: '%s'\n", *fp);
  return 0;
}