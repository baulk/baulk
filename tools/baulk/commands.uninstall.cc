//
#include "baulk.hpp"
#include "commands.hpp"

namespace baulk::commands {
int cmd_uninstall(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk uninstall package\n");
    return 1;
  }
  bela::error_code ec;
  auto locker = baulk::BaulkCloser::BaulkMakeLocker(ec);
  if (!locker) {
    bela::FPrintF(stderr, L"baulk upgrade: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }
  return false;
}
} // namespace baulk::commands