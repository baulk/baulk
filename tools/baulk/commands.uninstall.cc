//
#include <bela/path.hpp>
#include "baulk.hpp"
#include "fs.hpp"
#include "pkg.hpp"
#include "launcher.hpp"
#include "commands.hpp"

namespace baulk::commands {

int uninstallone(std::wstring_view pkgname) {
  auto lockfile = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks\\", pkgname, L".json");
  bela::error_code ec;
  if (bela::PathExists(lockfile)) {
    if (baulk::IsForceDelete) {
      if (!baulk::package::PackageForceDelete(pkgname, ec)) {
        bela::FPrintF(stderr, L"baulk uninstall '%s' force-delete: \x1b[31m%s\x1b[0m\n", pkgname, ec.message);
      }
    }
  } else if (!baulk::IsForceMode) {
    bela::FPrintF(stderr, L"No local metadata found, \x1b[34m%s\x1b[0m may not be installed.\n", pkgname);
    return 1;
  }

  if (!baulk::BaulkRemovePkgLinks(pkgname, ec)) {
    bela::FPrintF(stderr, L"baulk uninstall '%s' links: \x1b[31m%s\x1b[0m\n", pkgname, ec.message);
  }
  baulk::fs::PathRemove(lockfile, ec);
  auto pkgdir = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\pkgs\\", pkgname);
  // Use PathRemoveEx. remove the read-only attribute first
  if (!baulk::fs::PathRemoveEx(pkgdir, ec)) {
    bela::FPrintF(stderr, L"baulk uninstall '%s' error: \x1b[31m%s\x1b[0m\n", pkgname, ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"baulk uninstall \x1b[34m%s\x1b[0m done.\n", pkgname);
  return 0;
}

int cmd_uninstall(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk uninstall package\n");
    return 1;
  }
  bela::error_code ec;
  auto locker = baulk::BaulkCloser::BaulkMakeLocker(ec);
  if (!locker) {
    bela::FPrintF(stderr, L"baulk upgrade: \x1b[31m%s\x1b[0m\n", ec.message);
    return 1;
  }
  for (auto a : argv) {
    uninstallone(a);
  }
  return 0;
}
} // namespace baulk::commands