#include <bela/process.hpp>
#include <bela/terminal.hpp>

int stty_size() {
  bela::process::Process p;
  if (p.CaptureWithMode(bela::process::CAPTURE_USEIN | bela::process::CAPTURE_USEERR, L"stty",
                        L"size") != 0) {
    bela::FPrintF(stderr, L"%s\n", p.ErrorCode().message);
    return 1;
  }
  bela::FPrintF(stderr, L"result: %s\n", p.Out());
  return 0;
}

int wmain(int argc, wchar_t **argv) {
  stty_size();
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