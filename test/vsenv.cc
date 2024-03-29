#include <baulk/vs.hpp>
#include <baulk/venv.hpp>
#include <bela/terminal.hpp>
#include <baulk/vs/searcher.hpp>

namespace baulk {
bool IsDebugMode = true;
}

class dotcom_global_initializer {
public:
  dotcom_global_initializer() {
    auto hr = CoInitialize(NULL);
    if (FAILED(hr)) {
      auto ec = bela::make_system_error_code();
      MessageBoxW(nullptr, ec.data(), L"CoInitialize", IDOK);
      exit(1);
    }
  }
  ~dotcom_global_initializer() { CoUninitialize(); }
};

int wmain() {
  dotcom_global_initializer di;
  bela::error_code ec;
  baulk::vs::Searcher searcher;
  searcher.Initialize(ec);
  baulk::vs::vs_instances_t vsis;
  searcher.Search(vsis, ec);
  baulk::vfs::InitializeFastPathFs(ec);
  bela::env::Simulator sim;
  sim.InitializeCleanupEnv();
  auto vs = baulk::env::InitializeVisualStudioEnv(sim, L"x64", true, ec);
  if (!vs) {
    bela::FPrintF(stderr, L"error %s\n", ec);
    return 1;
  }
  bela::FPrintF(stderr, L"vs %v\n", *vs);
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