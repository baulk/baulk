//
#include "baulk.hpp"
#include "commands.hpp"
#include "extract.hpp"
#include <baulk/archive.hpp>

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
  auto tarfile = bela::PathAbsolute(argv[0]);
  auto dest = argv.size() > 1 ? std::wstring(argv[1]) : baulk::archive::FileDestination(tarfile);
  bela::error_code ec;
  if (!baulk::extract_tar(tarfile, dest, ec)) {
    bela::FPrintF(stderr, L"baulk untar %s error: %s\n", tarfile, ec);
    return 1;
  }
  return 0;
}
} // namespace baulk::commands
