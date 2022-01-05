//
#include <baulk/fs.hpp>
#include <baulk/vfs.hpp>
#include <baulk/fsmutex.hpp>
#include "baulk.hpp"
#include "bucket.hpp"
#include "commands.hpp"

namespace baulk::commands {

// check upgradable
int cmd_list_all() {
  bela::fs::Finder finder;
  bela::error_code ec;
  size_t upgradable = 0;
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
        upgradable++;
        bela::FPrintF(stderr,
                      L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s --> "
                      L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m%s%s\n",
                      localMeta->name, localMeta->bucket, localMeta->version, pkg.version, pkg.bucket,
                      baulk::IsFrozenedPackage(pkgName) ? L" \x1b[33m(frozen)\x1b[0m" : L"",
                      StringCategory(*localMeta));
        continue;
      }
      bela::FPrintF(stderr, L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s%s\n", localMeta->name, localMeta->bucket,
                    localMeta->version, StringCategory(*localMeta));
    } while (finder.Next());
  }
  bela::FPrintF(stderr, L"\x1b[32m%d packages can be updated.\x1b[0m\n", upgradable);
  return 0;
}

void usage_list() {
  bela::FPrintF(stderr, LR"(Usage: baulk list [package]...
List installed packages based on package names.

Example:
  baulk list
  baulk list curl

)");
}

int cmd_list(const argv_t &argv) {
  if (argv.empty()) {
    return cmd_list_all();
  }
  bela::error_code ec;
  for (const auto a : argv) {
    auto localMeta = baulk::PackageLocalMeta(a, ec);
    if (!localMeta) {
      baulk::DbgPrint(L"list package '%s' error: %s", a, ec);
      continue;
    }
    baulk::Package pkg;
    if (baulk::PackageUpdatableMeta(*localMeta, pkg)) {
      bela::FPrintF(stderr,
                    L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s --> "
                    L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m%s%s\n",
                    localMeta->name, localMeta->bucket, localMeta->version, pkg.version, pkg.bucket,
                    baulk::IsFrozenedPackage(a) ? L" \x1b[33m(frozen)\x1b[0m" : L"", StringCategory(*localMeta));
      continue;
    }
    bela::FPrintF(stderr, L"\x1b[32m%s\x1b[0m/\x1b[34m%s\x1b[0m %s%s\n", localMeta->name, localMeta->bucket,
                  localMeta->version, StringCategory(*localMeta));
  }
  return 0;
}
} // namespace baulk::commands
