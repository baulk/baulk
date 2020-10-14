#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/fs.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: delfile %s\n", argv[0]);
    return 1;
  }
  bela::error_code ec;
  if (!bela::fs::RemoveAll(argv[1], ec)) {
    bela::FPrintF(stderr, L"remove all: %s %d %s\n", argv[1], ec.code, ec.message);
  }
  return 0;
}