// Test reparse pointer

#include <hazel/fs.hpp>
#include <bela/terminal.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s path\n", argv[0]);
    return 1;
  }
  hazel::fs::FileReparsePoint frp;
  bela::error_code ec;
  if (!hazel::fs::LookupReparsePoint(argv[1], frp, ec)) {
    bela::FPrintF(stderr, L"unable lookup reparse point %s\n", ec.message);
    return 1;
  }
  for (auto &kv : frp.attributes) {
    bela::FPrintF(stderr, L"%s: %s\n", kv.first, kv.second);
  }
  return 0;
}