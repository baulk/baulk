///
#include "baulk.hpp"
#include "commands.hpp"
#include <bela/match.hpp>
#include <baulk/archive.hpp>
#include "extractor.hpp"

namespace baulk::commands {
void usage_unzip() {
  bela::FPrintF(stderr, LR"(Usage: baulk unzip [zipfile] [destination]
Extract compressed files in a ZIP archive.

Example:
  baulk unzip curl-7.80.0.zip
  baulk unzip curl-7.80.0.zip curl-dest

)");
}

int cmd_unzip(const argv_t &argv) {
  if (argv.empty()) {
    usage_unzip();
    return 1;
  }
  return baulk::extract_command_unchecked(argv, baulk::extract_zip);
}
} // namespace baulk::commands
