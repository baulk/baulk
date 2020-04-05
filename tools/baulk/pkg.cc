//
#include <bela/stdwriter.hpp>
#include <bela/path.hpp>
#include <version.hpp>
#include "bucket.hpp"
#include "launcher.hpp"
#include "pkg.hpp"
#include "net.hpp"
#include "hash.hpp"

namespace baulk::package {
// Package cached
std::optional<std::wstring> PackageCached(std::wstring_view filename,
                                          std::wstring_view hash) {
  auto pkgfile = bela::StringCat(baulk::BaulkRoot(), L"\\",
                                 baulk::BaulkPkgTmpDir, L"\\", filename);
  if (!bela::PathExists(pkgfile)) {
    return std::nullopt;
  }
  bela::error_code ec;
  if (!baulk::hash::HashEqual(pkgfile, hash, ec)) {
    bela::FPrintF(stderr, L"package file %s error: %s\n", filename, ec.message);
    return std::nullopt;
  }
  return std::make_optional(std::move(pkgfile));
}

int PackageMakeLinks(const baulk::Package &pkg) {
  // check package is good installed
  // rebuild launcher and links
  bela::error_code ec;
  if (!baulk::BaulkMakePkgLinks(pkg, true, ec)) {
    bela::FPrintF(stderr, L"baulk unable make %s links: %s\n", pkg.name,
                  ec.message);
    return 1;
  }
  return 0;
}

int PackageExpand(const baulk::Package &pkg, std::wstring_view pkgfile) {
  //
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
      return PackageMakeLinks(pkg);
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
  auto url = baulk::net::BestUrl(pkg.urls);
  if (url.empty()) {
    bela::FPrintF(stderr, L"baulk: \x1b[31m%s\x1b[0m no valid url\n", pkg.name);
    return 1;
  }
  baulk::DbgPrint(L"baulk '%s/%s' url: '%s'\n", pkg.name, pkg.version, url);
  if (!pkg.checksum.empty()) {
    auto filename = baulk::net::UrlFileName(url);
    baulk::DbgPrint(L"baulk '%s/%s' filename: '%s'\n", pkg.name, pkg.version,
                    filename);
    if (auto pkgfile = PackageCached(filename, pkg.checksum); pkgfile) {
      return PackageExpand(pkg, *pkgfile);
    }
  }
  auto pkgtmpdir =
      bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BaulkPkgTmpDir);
  auto pkgfile = baulk::net::WinGet(url, pkgtmpdir, true, ec);
  if (!pkgfile) {
    bela::FPrintF(stderr, L"baulk get %s: \x1b[31m%s\x1b[0m\n", url,
                  ec.message);
    return 1;
  }
  if (!pkg.checksum.empty() &&
      !baulk::hash::HashEqual(*pkgfile, pkg.checksum, ec)) {
    bela::FPrintF(stderr, L"baulk get %s: \x1b[31m%s\x1b[0m\n", url,
                  ec.message);
    // retry download
    pkgfile = baulk::net::WinGet(url, pkgtmpdir, true, ec);
    if (!pkgfile) {
      bela::FPrintF(stderr, L"baulk get %s: \x1b[31m%s\x1b[0m\n", url,
                    ec.message);
      return 1;
    }
    if (!baulk::hash::HashEqual(*pkgfile, pkg.checksum, ec)) {
      bela::FPrintF(stderr, L"baulk get %s: \x1b[31m%s\x1b[0m\n", url,
                    ec.message);
      return 1;
    }
  }
  return PackageExpand(pkg, *pkgfile);
}
} // namespace baulk::package