////
#include <bela/strcat.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s command\n", argv[0]);
    return 1;
  }
  std::wstring exe;
  if (!bela::ExecutableExistsInPath(argv[1], exe)) {
    bela::FPrintF(stderr, L"command not found: %s\n", argv[1]);
    return 1;
  }
  bela::FPrintF(stdout, L"%s\n", exe);
  return 0;
}