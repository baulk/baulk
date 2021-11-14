///
#include <hazel/macho.hpp>
#include <bela/terminal.hpp>
#include <bela/und.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s macho-o-file\n", argv[0]);
    return 1;
  }
  hazel::macho::File file;
  bela::error_code ec;
  if (!file.NewFile(argv[1], ec)) {
    bela::FPrintF(stderr, L"parse macho file %s error: %s\n", argv[1], ec);
    return 1;
  }
  std::vector<std::string> libs;
  file.Depends(libs, ec);
  for (const auto &d : libs) {
    bela::FPrintF(stdout, L"need: %s\n", d);
  }
  std::vector<std::string> symbols;
  if (!file.ImportedSymbols(symbols, ec)) {
    bela::FPrintF(stderr, L"parse macho file %s error: %s\n", argv[1], ec);
    return 1;
  }
  for (const auto &s : symbols) {
    bela::FPrintF(stdout, L"S: %s\n", bela::demangle(s));
  }
  return 0;
}