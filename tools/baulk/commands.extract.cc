#include "baulk.hpp"
#include <bela/terminal.hpp>
#include <baulk/archive.hpp>
#include "extract.hpp"
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
  auto arfile = argv[0];
  auto dest = argv.size() > 1 ? std::wstring(argv[1]) : baulk::archive::FileDestination(arfile);
  bela::error_code ec;
  return extract_auto_with_mode(arfile, dest, false, ec) ? 0 : 1;
}

} // namespace baulk::commands