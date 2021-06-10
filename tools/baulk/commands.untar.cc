//
#include "baulk.hpp"
#include "commands.hpp"
#include "decompress.hpp"
#include <tar.hpp>

namespace baulk::commands {
inline std::wstring resolveTarDestination(const argv_t &argv, std::wstring_view tarfile) {
  if (argv.size() > 1) {
    return bela::PathAbsolute(argv[1]);
  }
  if (auto d = baulk::archive::tar::PathRemoveExtension(tarfile); d.size() != tarfile.size()) {
    return std::wstring(d);
  }
  return bela::StringCat(tarfile, L".out");
}

void usage_untar() {
  bela::FPrintF(stderr, LR"(Usage: baulk untar tarfile destination
Extract files in a tar archive. support: tar.xz tar.bz2 tar.gz tar.zstd.

Example:
  baulk untar curl-7.76.0.tar.gz
  baulk untar curl-7.76.0.tar.gz curl-dest

)");
}

int cmd_untar(const argv_t &argv) {
  if (argv.empty()) {
    usage_untar();
    return 1;
  }
  auto tarfile = bela::PathAbsolute(argv[0]);
  auto root = resolveTarDestination(argv, tarfile);
  bela::error_code ec;
  if (!baulk::tar::Decompress(tarfile, root, ec)) {
    bela::FPrintF(stderr, L"decompress %s error: %s\n", tarfile, ec.message);
    return 1;
  }
  return 0;
}
} // namespace baulk::commands