////
#include <bela/escaping.hpp>
#include <bela/stdwriter.hpp>
#include <bela/base.hpp>
// .\test\escape\title_test.exe "vcpkg \U0001F496 Environment"
int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s escaped-title\n", argv[0]);
    return 1;
  }
  std::wstring title;
  if (!bela::CUnescape(argv[1], &title)) {
    bela::FPrintF(stderr, L"Unable unescape title: %s\n", argv[1]);
  }
  if (SetConsoleTitleW(title.data()) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"SetConsoleTitleW [%s] %s\n", title, ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"New console title is [%s]\n", title);
  (void)getc(stdin); // wait
  return 0;
}