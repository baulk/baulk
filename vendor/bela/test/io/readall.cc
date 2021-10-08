#include <bela/io.hpp>
#include <bela/terminal.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s file\n", argv[0]);
    return 1;
  }
  std::wstring buf;
  bela::error_code ec;
  if (!bela::io::ReadFile(argv[1], buf, ec)) {
    bela::FPrintF(stderr, L"open file %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"File read utf16 chars %d\n", buf.size());
  std::string u8buf;
  if (!bela::io::ReadFile(argv[1], u8buf, ec)) {
    bela::FPrintF(stderr, L"u8 open file %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"U8 File read utf8 chars %d\n%s\n", u8buf.size(), u8buf);
  return 0;
}