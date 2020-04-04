//
#include <bela/stdwriter.hpp>
#include "commands.hpp"
#include "baulk.hpp"

namespace baulk::commands {
// install subcommand
void install_usage() {
  bela::FPrintF(stderr, LR"(usage: baulk install package
Install specific packages. upgrade or repair installation if already installed
)");
}

int cmd_install(const argv_t &argv) {
  if (argv.empty()) {
    install_usage();
    return 1;
  }
  bela::error_code ec;
  if (!baulk::BaulkInitializeExecutor(ec)) {
    baulk::DbgPrint(L"unable initialize compiler executor: %s", ec.message);
  }
  return 0;
}
} // namespace baulk::commands