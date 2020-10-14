///
#include <baulkrev.hpp>
#include <bela/match.hpp>
#include <bela/strip.hpp>
#include <jsonex.hpp>
#include "net.hpp"
#include "commands.hpp"
#include "baulk.hpp"
#include "fs.hpp"
#include "bucket.hpp"
#include "pkg.hpp"

namespace baulk::commands {
int cmd_upgrade(const argv_t &argv) {
  bela::error_code ec;
  auto locker = baulk::BaulkCloser::BaulkMakeLocker(ec);
  if (!locker) {
    bela::FPrintF(stderr, L"baulk upgrade: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }
  if (!baulk::BaulkInitializeExecutor(ec)) {
    baulk::DbgPrint(L"unable initialize compiler executor: %s", ec.message);
  }

  bela::fs::Finder finder;
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
int cmd_update_and_upgrade(const argv_t &argv) {
  baulk::DbgPrint(L"run baulk update");
  if (auto i = baulk::commands::cmd_update(argv); i != 0) {
    return i;
  }
  baulk::DbgPrint(L"run baulk upgrade");
  return baulk::commands::cmd_upgrade(argv);
}

} // namespace baulk::commands