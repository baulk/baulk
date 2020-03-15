#include <bela/stdwriter.hpp>
#include "commands.hpp"
#include "fs.hpp"

namespace baulk::commands {
int cmd_freeze(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk freeze package ...\n");
    return 1;
  }
  return 0;
}
int cmd_unfreeze(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk unfreeze package ...\n");
    return 1;
  }
  return 0;
}

} // namespace baulk::commands