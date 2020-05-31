///
#include "commands.hpp"
#include "baulk.hpp"
#include "fs.hpp"
#include "bucket.hpp"
#include "pkg.hpp"

namespace baulk::commands {

int upgrade_self() {
  baulk::DbgPrint(L"baulk upgrade --self");
  return 0;
}

int cmd_upgrade(const argv_t &argv) {
  if (argv.size() > 0 && argv[0] == L"--self") {
    return upgrade_self();
  }
  bela::error_code ec;
  auto locker = baulk::BaulkCloser::BaulkMakeLocker(ec);
  if (!locker) {
    bela::FPrintF(stderr, L"baulk upgrade: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }
  if (!baulk::BaulkInitializeExecutor(ec)) {
    baulk::DbgPrint(L"unable initialize compiler executor: %s", ec.message);
  }

  baulk::fs::Finder finder;
  auto locksdir = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks");
  if (finder.First(locksdir, L"*.json", ec)) {
    do {
      if (finder.Ignore()) {
        continue;
      }
      auto pkgname = finder.Name();
      if (!bela::EndsWithIgnoreCase(pkgname, L".json")) {
        continue;
      }
      pkgname.remove_suffix(5);
      auto opkg = baulk::bucket::PackageLocalMeta(pkgname, ec);
      if (!opkg) {
        continue;
      }
      baulk::Package pkg;
      if (baulk::bucket::PackageUpdatableMeta(*opkg, pkg)) {
        baulk::package::BaulkInstall(pkg);
        continue;
      }
    } while (finder.Next());
  }
  return 0;
}
} // namespace baulk::commands