#include "baulk.hpp"
#include <bela/terminal.hpp>
#include <baulk/archive.hpp>
#include "extractor.hpp"
#include "commands.hpp"

namespace baulk::commands {

void usage_extract() {
  bela::FPrintF(stderr, LR"(Usage: baulk extract [archive] [destination]
Extract files in a tar archive. support: tar.xz tar.bz2 tar.gz tar.zstd

Example:
  baulk extract curl-7.80.0.tar.gz
  baulk extract curl-7.80.0.tar.gz curl-dest
  baulk e curl-7.80.0.tar.gz
  baulk e curl-7.80.0.tar.gz curl-dest

)");
}

int cmd_extract(const argv_t &argv) {
  if (argv.empty()) {
    usage_extract();
    return 1;
  }
  return baulk::extract_command_unchecked(argv, baulk::extract_command_auto);
}

} // namespace baulk::commands