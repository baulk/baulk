#include <bela/process.hpp>
#include <bela/stdwriter.hpp>

int wmain(int argc, wchar_t **argv) {
  bela::process::Process p;
  if (p.Capture(L"vswhere", L"-format", L"json", L"-utf8") == 0) {
    bela::FPrintF(stderr, L"result: %s\n", p.Out());
  }
  bela::FPrintF(stderr, L"\x1b[33mRun cmd\x1b[0m\n");
  p.SetEnv(L"BELA_DEBUG", L"1");
  auto exitcode = p.Execute(L"cmd");
  bela::FPrintF(stderr, L"\x1b[33mcmd exit %d\x1b[0m\n", exitcode);
  return 0;
}