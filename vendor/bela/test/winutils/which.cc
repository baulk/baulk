////
#include <bela/str_cat.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/simulator.hpp>

bool onlyAbsPath() {
  auto v = bela::AsciiStrToLower(bela::GetEnv(L"ENABLE_ABS_PATH"));
  return v == L"true" || v == L"1" || v == L"on" || v == L"yes";
}

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s command\n", argv[0]);
    return 1;
  }
  auto oa = onlyAbsPath();
  bela::FPrintF(stderr, L"ENABLE_ABS_PATH %v\n", oa);
  std::wstring exe;
  if (!bela::env::LookPath(argv[1], exe, oa)) {
    bela::FPrintF(stderr, L"command not found: %s\n", argv[1]);
    return 1;
  }
  bela::FPrintF(stdout, L"%s\n", exe);

  std::vector<std::wstring> paths;
  bela::MakePathEnv(paths);
  std::wstring exe2;
  if (!bela::env::LookPath(argv[1], exe2, paths, oa)) {
    bela::FPrintF(stderr, L"command not found: %s\n", argv[1]);
    return 1;
  }
  bela::FPrintF(stdout, L"%s\n", exe2);
  return 0;
}