#include <bela/io.hpp>
#include <bela/terminal.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage %s text\n", argv[0]);
    return 1;
  }
  bela::error_code ec;
  if (!bela::io::WriteText(argv[1], L"u8.txt", ec)) {
    bela::FPrintF(stderr, L"write u8: %s\n", ec.message);
  }
  if (!bela::io::WriteTextU16LE(argv[1], L"u16.txt", ec)) {
    bela::FPrintF(stderr, L"write u16: %s\n", ec.message);
  }
  auto u8 = bela::io::ReadLine(L"u8.txt", ec);
  if (u8) {
    bela::FPrintF(stderr, L"read u8 success: %s\n", *u8);
  } else {
    bela::FPrintF(stderr, L"read u8: %s\n", ec.message);
  }
  auto u16 = bela::io::ReadLine(L"u16.txt", ec);
  if (u16) {
    bela::FPrintF(stderr, L"read u16 success: %s\n", *u16);
  } else {
    bela::FPrintF(stderr, L"read u16: %s\n", ec.message);
  }
  return true;
}