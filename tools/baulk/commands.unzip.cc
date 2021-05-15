///
#include "baulk.hpp"
#include "commands.hpp"
#include <bela/match.hpp>
#include "decompress.hpp"

namespace baulk::commands {

inline std::wstring resolveZipDestination(const argv_t &argv, std::wstring_view zipfile) {
  if (argv.size() > 1) {
    return std::wstring(argv[1]);
  }
  if (bela::EndsWithIgnoreCase(zipfile, L".zip")) {
    return std::wstring(zipfile.data(), zipfile.size() - 4);
  }
  return bela::StringCat(zipfile, L".out");
}

void usage_unzip() {
  bela::FPrintF(stderr, LR"(Usage: baulk unzip zipfile destination
Extract compressed files in a ZIP archive (experimental).

Example:
  baulk unzip curl-7.76.0.zip
  baulk unzip curl-7.76.0.zip curl-dest

)");
}

int cmd_unzip(const argv_t &argv) {
  if (argv.empty()) {
    usage_unzip();
    return 1;
  }
  auto zipfile = argv[0];
  auto dest = resolveZipDestination(argv, zipfile);
  bela::error_code ec;
  if (!baulk::zip::Decompress(zipfile, dest, ec)) {
    bela::FPrintF(stderr, L"decompress %s error: %s\n", zipfile, ec.message);
    return 1;
  }
  return 0;
}
} // namespace baulk::commands