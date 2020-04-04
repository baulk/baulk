//
#include <bela/stdwriter.hpp>
#include <version.hpp>
#include "commands.hpp"
#include "baulk.hpp"
#include "bucket.hpp"

namespace baulk::commands {

int Reconstruct(const baulk::Package &pkg) {
  // check package is good installed
  // rebuild launcher and links
  return 0;
}

int install_pkg(std::wstring_view name) {
  bela::error_code ec;
  auto pkg = baulk::bucket::PackageMetaEx(name, ec);
  if (!pkg) {
    bela::FPrintF(stderr, L"\x1b[31mbaulk: %s\x1b[0m\n", ec.message);
    return 1;
  }
  if (pkg->urls.empty()) {
    bela::FPrintF(stderr, L"baulk: '%s' empty urls\n", name);
    return 1;
  }
  baulk::DbgPrint(L"baulk '%s' version '%s' url: '%s'\n", pkg->name,
                  pkg->version, pkg->urls.front());
  auto lpkg = baulk::bucket::PackageLocalMeta(name, ec);
  if (lpkg) {
    baulk::version::version pkgversion(pkg->version);
    baulk::version::version oldversion(lpkg->version);
    // new version less installed version or weights < weigths
    if (pkgversion < oldversion ||
        (pkgversion == oldversion && pkg->weights <= lpkg->weights)) {
      return Reconstruct(*pkg);
    }
    if (baulk::BaulkIsFrozenPkg(name) && !baulk::IsForceMode) {
      bela::FPrintF(stderr,
                    L"baulk \x1b[31mcannot\x1b[0m upgrade \x1b[35m%s\x1b[0m "
                    L"from \x1b[33m%s\x1b[0m@\x1b[34m%s\x1b[0m to "
                    L"\x1b[32m%s\x1b[0m@\x1b[34m%s\x1b[0m. it has been "
                    L"\x1b[31mfrozen\x1b[0m\n",
                    name, lpkg->version, lpkg->bucket, pkg->version,
                    pkg->bucket);
      return 0;
    }
    bela::FPrintF(stderr,
                  L"baulk will upgrade \x1b[35m%s\x1b[0m from "
                  L"\x1b[33m%s\x1b[0m@\x1b[34m%s\x1b[0m to "
                  L"\x1b[32m%s\x1b[0m@\x1b[34m%s\x1b[0m\n",
                  name, lpkg->version, lpkg->bucket, pkg->version, pkg->bucket);
  }
  // Install package
  return 0;
}

int cmd_install(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr,
                  L"usage: baulk install package\nInstall specific packages. "
                  L"upgrade or repair installation if already installed\n");
    return 1;
  }
  bela::error_code ec;
  if (!baulk::BaulkInitializeExecutor(ec)) {
    baulk::DbgPrint(L"unable initialize compiler executor: %s", ec.message);
  }
  for (auto pkg : argv) {
    install_pkg(pkg);
  }
  return 0;
}
} // namespace baulk::commands