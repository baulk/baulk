///
#include "baulk.hpp"
#include "commands.hpp"
#include <bela/match.hpp>
#include <baulk/archive.hpp>
#include "extract.hpp"

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
  auto zipfile = argv[0];
  auto dest = argv.size() > 1 ? std::wstring(argv[1]) : baulk::archive::FileDestination(zipfile);
  bela::error_code ec;
  if (!baulk::extract_zip(zipfile, dest, ec)) {
    bela::FPrintF(stderr, L"baulk unzip %s error: %s\n", zipfile, ec);
    return 1;
  }
  return 0;
}
} // namespace baulk::commands
