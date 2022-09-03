#include <bela/io.hpp>
#include <bela/terminal.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage %s text\n", argv[0]);
    return 1;
  }
  bela::error_code ec;
  if (!bela::io::WriteText(L"u8.txt", std::wstring_view{argv[1]}, ec)) {
    bela::FPrintF(stderr, L"write u8: %s\n", ec);
  }
  if (!bela::io::WriteTextU16LE(L"u16.txt", std::wstring_view{argv[1]}, ec)) {
    bela::FPrintF(stderr, L"write u16: %s\n", ec);
  }
  auto u8 = bela::io::ReadLine(L"u8.txt", ec);
  if (u8) {
    bela::FPrintF(stderr, L"read u8 success: %s\n", *u8);
  } else {
    bela::FPrintF(stderr, L"read u8: %s\n", ec);
  }
  auto u16 = bela::io::ReadLine(L"u16.txt", ec);
  if (u16) {
    bela::FPrintF(stderr, L"read u16 success: %s\n", *u16);
  } else {
    bela::FPrintF(stderr, L"read u16: %s\n", ec);
  }
  if (!bela::io::AtomicWriteText(L"atomic.txt", bela::io::as_bytes<char>("789456133"), ec)) {
    bela::FPrintF(stderr, L"atomic update file error: %s\n", ec);
  }
  if (!bela::io::AtomicWriteText(
          L"atomic.txt",
          bela::io::as_bytes<char>("😊✔️💜🤣👌😁🎉💖😒😂😍🤣🎉🎉🎉🎉🎉"), ec)) {
    bela::FPrintF(stderr, L"atomic update file error: %v\n", ec);
  }
  return true;
}