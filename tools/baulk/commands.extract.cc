#include <bela/terminal.hpp>
#include "commands.hpp"

namespace baulk::commands {

void usage_extract() {
  bela::FPrintF(stderr, LR"(Usage: baulk extract [file] ...)"); //
}

int cmd_extract(const argv_t &argv) {
  if (argv.empty()) {
    usage_extract();
    return 1;
  }
  auto arfile = argv[0];
  return 0;
}

} // namespace baulk::commands