//
///
#include <bela/pe.hpp>
#include <bela/terminal.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s pefile\n", argv[0]);
    return 1;
  }
  bela::error_code ec;
  auto version = bela::pe::Lookup(argv[1], ec);
  if (!version) {
    bela::FPrintF(stderr, L"unable lookup version: %v", ec);
    return 1;
  }
  bela::FPrintF(stderr, L"FileDescription: %s\nProductName: %s\nLegalCopyright: %s\n", version->FileDescription,
                version->ProductName, version->LegalCopyright);
  return 0;
}