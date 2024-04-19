///
#include <version.hpp>
#include <bela/match.hpp>
#include <bela/strip.hpp>
#include <baulk/fs.hpp>
#include <baulk/vfs.hpp>
#include <baulk/fsmutex.hpp>
#include "commands.hpp"
#include "baulk.hpp"
#include "bucket.hpp"
#include "pkg.hpp"

namespace baulk::commands {
void usage_upgrade() {
  bela::FPrintF(stderr, LR"(Usage: baulk upgrade [<args>]
Upgrade all upgradeable packages.

Example:
  baulk upgrade
  baulk upgrade --force

)");
}

void usage_update_and_upgrade() {
  bela::FPrintF(stderr, LR"(Usage: baulk u [<args>]
Update bucket metadata and upgrade all upgradeable packages.

Example:
  baulk u
  baulk u --force

)");
}

int cmd_upgrade(const argv_t &argv) {
  (void)argv;
  bela::error_code ec;
  auto mtx = MakeFsMutex(vfs::AppFsMutexPath(), ec);
  if (!mtx) {
    bela::FPrintF(stderr, L"baulk upgrade: \x1b[31mbaulk %s\x1b[0m\n", ec);
    return 1;
  }
  if (!InitializeExecutor(ec)) {
    baulk::DbgPrint(L"baulk upgrade: unable initialize compiler executor: %s", ec);
  }

  bela::fs::Finder finder;
  if (finder.First(vfs::AppLocks(), L"*.json", ec)) {
    do {
      if (finder.Ignore()) {
        continue;
      }
      auto pkgName = finder.Name();
      if (!bela::EndsWithIgnoreCase(pkgName, L".json")) {
        continue;
      }
      pkgName.remove_suffix(5);
      auto localMeta = baulk::PackageLocalMeta(pkgName, ec);
      if (!localMeta) {
        continue;
      }
      baulk::Package pkg;
      if (baulk::PackageUpdatableMeta(*localMeta, pkg)) {
        baulk::package::Install(pkg);
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