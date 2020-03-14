//

//
#include <bela/stdwriter.hpp>
#include "commands.hpp"
#include "fs.hpp"

namespace baulk::commands {
int cmd_exec(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk exec [command [arguments ...]]\n");
    return 1;
  }
  for (auto a : argv) {
    bela::FPrintF(stderr, L"exec arg: %s\n", a);
  }
  return 0;
}
} // namespace baulk::commands