//
#include <bela/stdwriter.hpp>
#include "commands.hpp"
#include "baulk.hpp"

namespace baulk::commands {
int cmd_install(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk install package\n");
    return 1;
  }
  bela::error_code ec;
  if (!baulk::BaulkInitializeExecutor(ec)) {
    baulk::DbgPrint(L"unable initialize compiler executor: %s", ec.message);
  }
  return 0;
}
} // namespace baulk::commands