////
#include <bela/strcat.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/simulator.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s command\n", argv[0]);
    return 1;
  }
  std::wstring exe;
  if (!bela::env::ExecutableExistsInPath(argv[1], exe)) {
    bela::FPrintF(stderr, L"command not found: %s\n", argv[1]);
    return 1;
  }
  bela::FPrintF(stdout, L"%s\n", exe);

  std::vector<std::wstring> paths;
  bela::MakePathEnv(paths);
  std::wstring exe2;
  if (!bela::env::ExecutableExistsInPath(argv[1], exe2, paths)) {
    bela::FPrintF(stderr, L"command not found: %s\n", argv[1]);
    return 1;
  }
  bela::FPrintF(stdout, L"%s\n", exe2);
  return 0;
}