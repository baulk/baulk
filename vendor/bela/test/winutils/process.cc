#include <bela/process.hpp>
#include <bela/terminal.hpp>

int stty_size() {
  bela::process::Process p;
  if (p.CaptureWithMode(bela::process::CAPTURE_USEIN | bela::process::CAPTURE_USEERR, L"stty", L"size") != 0) {
    bela::FPrintF(stderr, L"ExitCode: %d Error: %s\nOutput: %s\n", p.ExitCode(), p.ErrorCode().message, p.Out());
    return 1;
  }
  bela::FPrintF(stderr, L"result: %s\n", p.Out());
  return 0;
}

int wmain() {
  stty_size();
  bela::env::Simulator simulator;
  simulator.InitializeEnv();
  simulator.SetEnv(L"BELA_DEBUG_MODE", L"true");
  bela::process::Process p(&simulator);
  // vswhere use stdout output
  if (p.CaptureWithMode(bela::process::CAPTURE_USEIN | bela::process::CAPTURE_USEERR, L"vswhere", L"-format", L"json",
                        L"-utf8") == 0) {
    bela::FPrintF(stderr, L"result: %s\n", p.Out());
  } else {
    bela::FPrintF(stderr, L"Exit %s\n%s\n", p.ErrorCode().message, p.Out());
  }
  bela::FPrintF(stderr, L"\x1b[33mRun cmd\x1b[0m\n");
  auto exitcode = p.Execute(L"cmd");
  bela::FPrintF(stderr, L"\x1b[33mcmd exit %d\x1b[0m\n", exitcode);
  return 0;
}