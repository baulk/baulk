//
#include "baulk.hpp"
#include "fs.hpp"
#include "bucket.hpp"
#include "commands.hpp"

namespace baulk::commands {
// check upgradable
int cmd_list_all() {
  baulk::fs::Finder finder;
  bela::error_code ec;
  auto locksdir = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks");
  if (!finder.First(locksdir, L"*.json", ec)) {
    return true;
  }
  size_t upgradable = 0;
  do {
    if (finder.Ignore()) {
      continue;
    }
    auto name = finder.Name();
    if (!bela::EndsWithIgnoreCase(name, L".json")) {
      continue;
    }

    name.remove_suffix(5);
    auto opkg = baulk::bucket::PackageLocalMeta(name, ec);
    if (!opkg) {
      continue;
    }
    baulk::Package pkg;
    if (baulk::bucket::PackageUpdatableMeta(*opkg, pkg)) {
      upgradable++;
      bela::FPrintF(
          stderr,
          L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s-->\x1b[32m%s/%s\x1b[0m%s\n",
          opkg->name, opkg->bucket, opkg->version, pkg.bucket, pkg.version,
          baulk::BaulkIsFrozenPkg(name) ? L" \x1b[33mfrozen\x1b[0m" : L"");
      continue;
    }
    bela::FPrintF(stderr, L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s\n",
                  opkg->name, opkg->bucket, opkg->version);
  } while (finder.Next());
  bela::FPrintF(stderr, L"\x1b[33m%d packages can be updated.\x1b[0m\n",
                upgradable);
  return 0;
}

int cmd_list(const argv_t &argv) {
  if (argv.empty()) {
    return cmd_list_all();
  }
  return 0;
}
} // namespace baulk::commands