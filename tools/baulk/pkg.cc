//
#include <bela/stdwriter.hpp>
#include <version.hpp>
#include "bucket.hpp"
#include "pkg.hpp"
#include "net.hpp"

namespace baulk::package {

int Reconstruct(const baulk::Package &pkg) {
  // check package is good installed
  // rebuild launcher and links
  return 0;
}

int BaulkInstall(const baulk::Package &pkg) {
  bela::error_code ec;
  auto pkglocal = baulk::bucket::PackageLocalMeta(pkg.name, ec);
  if (pkglocal) {
    baulk::version::version pkgversion(pkg.version);
    baulk::version::version oldversion(pkglocal->version);
    // new version less installed version or weights < weigths
    if (pkgversion < oldversion ||
        (pkgversion == oldversion && pkg.weights <= pkglocal->weights)) {
      return Reconstruct(pkg);
    }
    if (baulk::BaulkIsFrozenPkg(pkg.name) && !baulk::IsForceMode) {
      // Since the metadata has been updated, we cannot rebuild the frozen
      // package launcher
      bela::FPrintF(stderr,
                    L"baulk \x1b[31mcannot\x1b[0m upgrade \x1b[35m%s\x1b[0m "
                    L"from \x1b[33m%s\x1b[0m@\x1b[34m%s\x1b[0m to "
                    L"\x1b[32m%s\x1b[0m@\x1b[34m%s\x1b[0m. it has been "
                    L"\x1b[31mfrozen\x1b[0m\n",
                    pkg.name, pkglocal->version, pkglocal->bucket, pkg.version,
                    pkg.bucket);
      return 0;
    }
    bela::FPrintF(stderr,
                  L"baulk will upgrade \x1b[35m%s\x1b[0m from "
                  L"\x1b[33m%s\x1b[0m@\x1b[34m%s\x1b[0m to "
                  L"\x1b[32m%s\x1b[0m@\x1b[34m%s\x1b[0m\n",
                  pkg.name, pkglocal->version, pkglocal->bucket, pkg.version,
                  pkg.bucket);
  }
  auto url = baulk::net::BestURL(pkg.urls);
  if (url.empty()) {
    bela::FPrintF(stderr, L"baulk: \x1b[31m%s\x1b[0m no valid url\n", pkg.name);
    return 1;
  }
  if (!pkg.checksum.empty()) {
  }

  return 0;
}
} // namespace baulk::package