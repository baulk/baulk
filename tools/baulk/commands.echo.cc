///
// pkgclean command cleanup pkg cache
#include <bela/terminal.hpp>
#include <baulk/fs.hpp>
#include <baulk/vfs.hpp>
#include "baulk.hpp"
#include "commands.hpp"

namespace baulk::commands {
void usage_echo() {
  bela::FPrintF(stderr, LR"(Usage: baulk echo [<args>]
Baulk interaction mode command

Example:
  baulk echo

)");
}

int cmd_echo(const argv_t &argv) {
  //
  return 0;
}

} // namespace baulk::commands