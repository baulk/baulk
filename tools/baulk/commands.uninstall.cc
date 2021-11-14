//
#include <bela/path.hpp>
#include <baulk/vfs.hpp>
#include <baulk/fs.hpp>
#include <baulk/fsmutex.hpp>
#include "baulk.hpp"
#include "pkg.hpp"
#include "launcher.hpp"
#include "commands.hpp"

namespace baulk::commands {

int uninstall_package(std::wstring_view pkgName) {
  auto lockfile = bela::StringCat(vfs::AppLocks(), pkgName, L".json");
  bela::error_code ec;
  if (bela::PathExists(lockfile)) {
    if (baulk::IsForceDelete) {
      if (!baulk::package::PackageForceDelete(pkgName, ec)) {
        bela::FPrintF(stderr, L"baulk uninstall '%s' force-delete: \x1b[31m%s\x1b[0m\n", pkgName, ec);
      }
    }
  } else if (!baulk::IsForceMode) {
    bela::FPrintF(stderr, L"No local metadata found, \x1b[34m%s\x1b[0m may not be installed.\n", pkgName);
    return 1;
  }

  if (!baulk::RemovePackageLinks(pkgName, ec)) {
    bela::FPrintF(stderr, L"baulk uninstall '%s' links: \x1b[31m%s\x1b[0m\n", pkgName, ec);
  }
  bela::fs::ForceDeleteFolders(lockfile, ec);
  auto packageRoot = vfs::AppPackageFolder(pkgName);
  if (!bela::fs::ForceDeleteFolders(packageRoot, ec)) {
    bela::FPrintF(stderr, L"baulk uninstall '%s' error: \x1b[31m%s\x1b[0m\n", pkgName, ec);
    return 1;
  }
  bela::FPrintF(stderr, L"baulk uninstall \x1b[34m%s\x1b[0m done.\n", pkgName);
  return 0;
}

void usage_uninstall() {
  bela::FPrintF(stderr, LR"(Usage: baulk uninstall [package]...
Uninstall specific packages. (alias: r)

Example:
  baulk uninstall wget
  baulk r wget

)");
}

int cmd_uninstall(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk uninstall package\n");
    return 1;
  }
  bela::error_code ec;
  auto mtx = MakeFsMutex(vfs::AppFsMutexPath(), ec);
  if (!mtx) {
    bela::FPrintF(stderr, L"baulk uninstall: \x1b[31mbaulk %s\x1b[0m\n", ec);
    return 1;
  }
  for (auto a : argv) {
    uninstall_package(a);
  }
  return 0;
}
} // namespace baulk::commands