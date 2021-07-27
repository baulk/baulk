////
#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <cstdio>

namespace bela {
ssize_t BelaWriteAnsi(HANDLE hDev, std::wstring_view msg);
}

bool IsVTEnable(HANDLE hFile) {
  DWORD dwMode = 0;
  if (!GetConsoleMode(hFile, &dwMode)) {
    return false;
  }
  return ((dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0);
}

int wmain() {
  auto hDev = GetStdHandle(STD_OUTPUT_HANDLE);
  fprintf(stdout, "\x1b[01;34mBEGIN\x1b[0m\n");
  if (hDev == INVALID_HANDLE_VALUE) {
    return 1;
  }
  auto vtenabled = IsVTEnable(hDev);
  bela::BelaWriteAnsi(hDev, vtenabled ? L"\x1b[01;32mOK VT Enabled\x1b[0m\n" : L"\x1b[01;31mBAD VT Disabled\x1b[0m\n");
  bela::FPrintF(stderr, L"\x1b[33m----------------- OK-----\x1b[0m\n");
  vtenabled = IsVTEnable(hDev);
  bela::BelaWriteAnsi(hDev, vtenabled ? L"\x1b[01;32mOK VT Enabled\x1b[0m\n" : L"\x1b[01;31mBAD VT Disabled\x1b[0m\n");
  return 0;
}