// Test reparse pointer

#include <bela/repasepoint.hpp>
#include <bela/terminal.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s path\n", argv[0]);
    return 1;
  }
  bela::ReparsePoint rp;
  bela::error_code ec;
  if (!rp.Analyze(argv[1], ec)) {
    if (ec) {
      bela::FPrintF(stderr, L"Analyze %s error: %s\n", argv[0], ec.message);
      return 1;
    }
    bela::FPrintF(stderr, L"Analyze %s bad callback return\n", argv[0]);
    return 1;
  }
  for (const auto &e : rp.Attributes()) {
    bela::FPrintF(stderr, L"%s: %s\n", e.name, e.value);
  }
  return 0;
}