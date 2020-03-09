//
#include <bela/stdwriter.hpp>
#include "commands.hpp"

namespace baulk::commands {
int cmd_install(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk install package\n");
    return 1;
  }
  return 0;
}
} // namespace baulk::commands