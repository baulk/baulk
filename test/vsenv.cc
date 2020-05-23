#include "../tools/baulk/compiler.hpp"
#include <bela/terminal.hpp>

namespace baulk {
bool IsDebugMode = true;
}

int wmain() {
  baulk::compiler::Executor executor;
  bela::error_code ec;
  if (!executor.Initialize(ec)) {
    bela::FPrintF(stderr, L"compiler env initialize error: %s\n", ec.message);
    return 1;
  }
  auto exitcode = executor.Execute(L"", L"pwsh");
  bela::FPrintF(stderr, L"Exitcode: %d\n", exitcode);
  return exitcode;
}