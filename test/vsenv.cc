#include <baulk/vs.hpp>
#include <bela/terminal.hpp>

namespace baulk {
bool IsDebugMode = true;
}

int wmain() {
  bela::env::Simulator sim;
  sim.InitializeCleanupEnv();
  bela::error_code ec;
  if (!baulk::env::InitializeVisualStudioEnv(sim, L"arm64", true, ec)) {
    bela::FPrintF(stderr, L"error %s\n", ec.message);
    return 1;
  }
  bela::process::Process p(&sim);
  auto exitcode = p.Execute(L"pwsh");
  bela::FPrintF(stderr, L"Exitcode: %d\n", exitcode);
  return exitcode;
}