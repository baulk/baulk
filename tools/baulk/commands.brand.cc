//
#include <bela/terminal.hpp>
#include "commands.hpp"

namespace baulk::commands {
void usage_brand() {
  bela::FPrintF(stderr, L"Usage: baulk brand\n"); //
}

int cmd_brand(const argv_t &) { return 0; }
} // namespace baulk::commands