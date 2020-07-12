//
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <version.hpp>
#include <jsonex.hpp>
#include <time.hpp>
#include "bucket.hpp"
#include "launcher.hpp"
#include "pkg.hpp"
#include "net.hpp"
#include "hash.hpp"
#include "fs.hpp"
#include "decompress.hpp"

namespace baulk::package {

inline void AddArray(nlohmann::json &root, const char *name, const std::vector<std::wstring> &av) {
  if (!av.empty()) {
    nlohmann::json jea;
    for (const auto &a : av) {
      jea.emplace_back(bela::ToNarrow(a));
    }
    root[name] = std::move(jea);
  }
}

bool PackageLocalMetaWrite(const baulk::Package &pkg, bela::error_code &ec) {
  try {
    nlohmann::json j;
    j["version"] = bela::ToNarrow(pkg.version);
    j["bucket"] = bela::ToNarrow(pkg.bucket);
    j["date"] = baulk::time::TimeNow();
    if (!pkg.venv.empty()) {
      nlohmann::json venv;
      AddArray(venv, "path", pkg.venv.paths);
      AddArray(venv, "include", pkg.venv.includes);
      AddArray(venv, "lib", pkg.venv.libs);
      AddArray(venv, "env", pkg.venv.envs);
      j["venv"] = std::move(venv);
    }
    auto file = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks");
    if (!baulk::fs::MakeDir(file, ec)) {
      return false;
    }
    bela::StrAppend(&file, L"\\", pkg.name, L".json");
    return bela::io::WriteTextAtomic(j.dump(4), file, ec);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, bela::ToWide(e.what()));
  }
  return false;
}

// Package cached
std::optional<std::wstring> PackageCached(std::wstring_view filename, std::wstring_view hash) {
  auto pkgfile = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BaulkPkgTmpDir, L"\\", filename);
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
  bela::error_code ec;
  baulk::BaulkRemovePkgLinks(pkg.name, ec);
  // check package is good installed
  // rebuild launcher and links
  if (!baulk::BaulkMakePkgLinks(pkg, true, ec)) {
    bela::FPrintF(stderr, L"baulk unable make %s links: %s\n", pkg.name, ec.message);
    return 1;
  }
  bela::FPrintF(stderr,
                L"baulk install \x1b[35m%s\x1b[0m/\x1b[34m%s\x1b[0m version "
                L"\x1b[32m%s\x1b[0m success.\n",
                pkg.name, pkg.bucket, pkg.version);
  return 0;
}

inline bool BaulkRename(std::wstring_view source, std::wstring_view target, bela::error_code &ec) {
  if (bela::PathExists(target)) {
    baulk::fs::PathRemove(target, ec);
  }
  if (MoveFileExW(source.data(), target.data(),
                  MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  return true;
}

int PackageExpand(const baulk::Package &pkg, std::wstring_view pkgfile) {
  auto h = baulk::LookupHandler(pkg.extension);
  if (!h) {
    bela::FPrintF(stderr, L"baulk unsupport package extension: %s\n", pkg.extension);
    return 1;
  }
  std::wstring outdir(pkgfile);
  if (auto pos = outdir.rfind('.'); pos != std::wstring::npos) {
    outdir.resize(pos);
  } else {
    outdir.append(L".out");
  }
  baulk::DbgPrint(L"Decompress %s to %s\n", pkg.name, outdir);
  bela::error_code ec;
  if (bela::PathExists(outdir)) {
    baulk::fs::PathRemove(outdir, ec);
  }
  if (!h->decompress(pkgfile, outdir, ec)) {
    bela::FPrintF(stderr, L"baulk decompress %s error: %s\n", pkgfile, ec.message);
    return 1;
  }
  h->regularize(outdir);
  auto pkgdir = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\pkgs\\", pkg.name);
  std::wstring pkgold;
  std::error_code e;
  if (bela::PathExists(pkgdir)) {
    pkgold = bela::StringCat(pkgdir, L".old");
    if (!BaulkRename(pkgdir, pkgold, ec)) {
      bela::FPrintF(stderr, L"baulk rename %s error: \x1b[31m%s\x1b[0m\n", pkgdir, ec.message);
      return 1;
    }
  }
  if (!BaulkRename(outdir, pkgdir, ec)) {
    bela::FPrintF(stderr, L"baulk rename %s error: \x1b[31m%s\x1b[0m\n", pkgdir, ec.message);
    if (!pkgold.empty()) {
      BaulkRename(pkgdir, pkgold, ec);
    }
    return 1;
  }
  if (bela::EqualsIgnoreCase(pkg.extension, L"exe")) {
    auto fn = baulk::fs::FileName(pkgfile);
    auto rfn = bela::StringCat(pkg.name, std::filesystem::path(pkgfile).extension().wstring());
    if (!bela::EqualsIgnoreCase(fn, rfn)) {
      auto expnadfile = bela::StringCat(pkgdir, L"/", fn);
      auto target = bela::StringCat(pkgdir, L"/", rfn);
      if (MoveFileEx(expnadfile.data(), target.data(), MOVEFILE_REPLACE_EXISTING) != TRUE) {
        ec = bela::make_system_error_code();
        bela::FPrintF(stderr, L"baulk rename package file %s to %s error: \x1b[31m%s\x1b[0m\n",
                      expnadfile, target, ec.message);
      }
    }
  }
  if (!pkgold.empty()) {
    baulk::fs::PathRemove(pkgold, ec);
  }
  // create a links
  if (!PackageLocalMetaWrite(pkg, ec)) {
    bela::FPrintF(stderr, L"baulk unable write local meta: %s\n", ec.message);
    return 1;
  }
  return PackageMakeLinks(pkg);
}

int BaulkInstall(const baulk::Package &pkg) {
  bela::error_code ec;
  auto pkglocal = baulk::bucket::PackageLocalMeta(pkg.name, ec);
  if (pkglocal) {
    baulk::version::version pkgversion(pkg.version);
    baulk::version::version oldversion(pkglocal->version);
    // new version less installed version or weights < weigths
    if (pkgversion < oldversion || (pkgversion == oldversion && pkg.weights <= pkglocal->weights)) {
      return PackageMakeLinks(pkg);
    }
    if (baulk::BaulkIsFrozenPkg(pkg.name) && !baulk::IsForceMode) {
      // Since the metadata has been updated, we cannot rebuild the frozen
      // package launcher
      bela::FPrintF(stderr,
                    L"baulk \x1b[31mskip upgrade\x1b[0m "
                    L"\x1b[35m%s\x1b[0m(\x1b[31mfrozen\x1b[0m) from "
                    L"\x1b[33m%s\x1b[0m@\x1b[34m%s\x1b[0m to "
                    L"\x1b[32m%s\x1b[0m@\x1b[34m%s\x1b[0m.\n",
                    pkg.name, pkglocal->version, pkglocal->bucket, pkg.version, pkg.bucket);
      return 0;
    }
    bela::FPrintF(stderr,
                  L"baulk will upgrade \x1b[35m%s\x1b[0m from "
                  L"\x1b[33m%s\x1b[0m@\x1b[34m%s\x1b[0m to "
                  L"\x1b[32m%s\x1b[0m@\x1b[34m%s\x1b[0m\n",
                  pkg.name, pkglocal->version, pkglocal->bucket, pkg.version, pkg.bucket);
  }
  auto url = baulk::net::BestUrl(pkg.urls);
  if (url.empty()) {
    bela::FPrintF(stderr, L"baulk: \x1b[31m%s\x1b[0m no valid url\n", pkg.name);
    return 1;
  }
  baulk::DbgPrint(L"baulk '%s/%s' url: '%s'\n", pkg.name, pkg.version, url);
  if (!pkg.checksum.empty()) {
    auto filename = baulk::net::UrlFileName(url);
    baulk::DbgPrint(L"baulk '%s/%s' filename: '%s'\n", pkg.name, pkg.version, filename);
    if (auto pkgfile = PackageCached(filename, pkg.checksum); pkgfile) {
      return PackageExpand(pkg, *pkgfile);
    }
  }
  auto pkgtmpdir = bela::StringCat(baulk::BaulkRoot(), L"\\", baulk::BaulkPkgTmpDir);
  if (!baulk::fs::MakeDir(pkgtmpdir, ec)) {
    bela::FPrintF(stderr, L"baulk unable make %s error: %s\n", pkgtmpdir, ec.message);
    return 1;
  }
  auto pkgfile = baulk::net::WinGet(url, pkgtmpdir, true, ec);
  if (!pkgfile) {
    bela::FPrintF(stderr, L"baulk get %s: \x1b[31m%s\x1b[0m\n", url, ec.message);
    return 1;
  }
  if (!pkg.checksum.empty() && !baulk::hash::HashEqual(*pkgfile, pkg.checksum, ec)) {
    bela::FPrintF(stderr, L"baulk get %s: \x1b[31m%s\x1b[0m\n", url, ec.message);
    // retry download
    pkgfile = baulk::net::WinGet(url, pkgtmpdir, true, ec);
    if (!pkgfile) {
      bela::FPrintF(stderr, L"baulk get %s: \x1b[31m%s\x1b[0m\n", url, ec.message);
      return 1;
    }
    if (!baulk::hash::HashEqual(*pkgfile, pkg.checksum, ec)) {
      bela::FPrintF(stderr, L"baulk get %s: \x1b[31m%s\x1b[0m\n", url, ec.message);
      return 1;
    }
  }
  return PackageExpand(pkg, *pkgfile);
}
} // namespace baulk::package