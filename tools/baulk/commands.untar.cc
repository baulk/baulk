//
#include "baulk.hpp"
#include "commands.hpp"
#include "extractor.hpp"

namespace baulk::commands {
void usage_untar() {
  bela::FPrintF(stderr, LR"(Usage: baulk untar [tarfile] [destination]
Extract files in a tar archive. support: tar.xz tar.bz2 tar.gz tar.zstd

Example:
  baulk untar curl-7.80.0.tar.gz
  baulk untar curl-7.80.0.tar.gz curl-dest

)");
}

int cmd_untar(const argv_t &argv) {
  if (argv.empty()) {
    usage_untar();
    return 1;
  }
  return baulk::extract_command_unchecked(argv, baulk::extract_tar);
}
} // namespace baulk::commands
