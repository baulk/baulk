#include <baulk/vs.hpp>
#include <baulk/venv.hpp>
#include <bela/terminal.hpp>

namespace baulk {
bool IsDebugMode = true;
}

int wmain() {
  bela::error_code ec;
  baulk::vfs::InitializeFastPathFs(ec);
  bela::env::Simulator sim;
  sim.InitializeCleanupEnv();
  if (!baulk::env::InitializeVisualStudioEnv(sim, L"x64", true, ec)) {
    bela::FPrintF(stderr, L"error %s\n", ec);
    return 1;
  }
  baulk::env::Constructor cs(true);
  std::vector<std::wstring> envs = {L"rust"};
  if (!cs.InitializeEnvs(envs, sim, ec)) {
    bela::FPrintF(stderr, L"unable initialize venvs: %s\n", ec);
  } else {
    bela::FPrintF(stderr, L"rust initialize success\n");
  }
  bela::process::Process p(&sim);
  auto exitcode = p.Execute(L"pwsh");
  bela::FPrintF(stderr, L"Exitcode: %d\n", exitcode);
  return exitcode;
}