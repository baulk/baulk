//
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <bela/simulator.hpp>
#include <bela/datetime.hpp>
#include <bela/semver.hpp>
#include <jsonex.hpp>
#include "bucket.hpp"
#include "launcher.hpp"
#include "pkg.hpp"
#include "net.hpp"
#include <baulk/fs.hpp>
#include <baulk/hash.hpp>
#include "decompress.hpp"

namespace baulk::package {

inline void AddArray(nlohmann::json &root, const char *name, const std::vector<std::wstring> &av) {
  if (!av.empty()) {
    nlohmann::json jea;
    for (const auto &a : av) {
      jea.emplace_back(bela::encode_into<wchar_t, char>(a));
    }
    root[name] = std::move(jea);
  }
}

bool PackageLocalMetaWrite(const baulk::Package &pkg, bela::error_code &ec) {
  try {
    nlohmann::json j;
    j["version"] = bela::encode_into<wchar_t, char>(pkg.version);
    j["bucket"] = bela::encode_into<wchar_t, char>(pkg.bucket);
    j["date"] = bela::FormatTime<char>(bela::Now());
    AddArray(j, "force_delete", pkg.forceDeletes);
    if (!pkg.venv.empty()) {
      nlohmann::json venv;
      if (!pkg.venv.category.empty()) {
        venv["category"] = bela::encode_into<wchar_t, char>(pkg.venv.category);
      }
      AddArray(venv, "path", pkg.venv.paths);
      AddArray(venv, "include", pkg.venv.includes);
      AddArray(venv, "lib", pkg.venv.libs);
      AddArray(venv, "env", pkg.venv.envs);
      AddArray(venv, "dependencies", pkg.venv.dependencies); // venv dependencies
      j["venv"] = std::move(venv);
    }
    auto file = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks");
    if (!baulk::fs::MakeDir(file, ec)) {
      return false;
    }
    bela::StrAppend(&file, L"\\", pkg.name, L".json");
    return bela::io::WriteTextAtomic(j.dump(4), file, ec);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, bela::encode_into<char, wchar_t>(e.what()));
  }
  return false;
}

bool PackageForceDelete(std::wstring_view pkgname, bela::error_code &ec) {
  auto pkglocal = baulk::bucket::PackageLocalMeta(pkgname, ec);
  if (!pkglocal) {
    return false;
  }
  bela::env::Simulator sim;
  sim.InitializeEnv();
  sim.SetEnv(L"BAULK_ROOT", baulk::BaulkRoot());
  sim.SetEnv(L"BAULK_VFS", bela::StringCat(baulk::BaulkRoot(), L"\\bin\\vfs"));
  sim.SetEnv(L"BAULK_PKGROOT", bela::StringCat(baulk::BaulkRoot(), L"\\bin\\pkg\\", pkgname));
  for (const auto &p : pkglocal->forceDeletes) {
    auto realdir = sim.PathExpand(p);
    baulk::DbgPrint(L"force delete: %s@%s", pkgname, realdir);
    bela::error_code ec2;
    if (bela::fs::ForceDeleteFolders(realdir, ec2)) {
      bela::FPrintF(stderr, L"force delete %s \x1b[31m%s\x1b[0m\n", realdir, ec.message);
    }
  }
  return true;
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
  if (!pkg.venv.mkdirs.empty()) {
    bela::env::Simulator sim;
    sim.InitializeEnv();
    sim.SetEnv(L"BAULK_ROOT", baulk::BaulkRoot());
    sim.SetEnv(L"BAULK_VFS", bela::StringCat(baulk::BaulkRoot(), L"\\bin\\vfs"));
    sim.SetEnv(L"BAULK_PKGROOT", bela::StringCat(baulk::BaulkRoot(), L"\\bin\\pkg\\", pkg.name));
    for (const auto &p : pkg.venv.mkdirs) {
      auto realdir = sim.PathExpand(p);
      baulk::DbgPrint(L"mkdir: %s@%s", pkg.name, realdir);
      std::error_code e;
      if (std::filesystem::create_directories(realdir, e); e) {
        bela::FPrintF(stderr, L"mkdir %s \x1b[31m%s\x1b[0m\n", realdir, bela::fromascii(e.message()));
      }
    }
  }
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
    bela::fs::ForceDeleteFolders(target, ec);
  }
  DbgPrint(L"lpExistingFileName %s lpNewFileName %s", source, target);
  if (MoveFileExW(source.data(), target.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) != TRUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  return true;
}

inline std::wstring UnarchivePath(std::wstring_view path) {
  auto dir = bela::DirName(path);
  auto filename = bela::BaseName(path);
  auto extName = bela::ExtensionEx(filename);
  if (filename.size() <= extName.size()) {
    return bela::StringCat(dir, L"\\out");
  }
  if (extName.empty()) {
    return bela::StringCat(dir, L"\\", filename, L".out");
  }
  return bela::StringCat(dir, L"\\", filename.substr(0, filename.size() - extName.size()));
}

int PackageExpand(const baulk::Package &pkg, std::wstring_view pkgfile) {
  auto h = baulk::LookupHandler(pkg.extension);
  if (!h) {
    bela::FPrintF(stderr, L"baulk unsupport package extension: %s\n", pkg.extension);
    return 1;
  }
  auto outdir = UnarchivePath(pkgfile);
  baulk::DbgPrint(L"Decompress %s to %s\n", pkg.name, outdir);
  bela::error_code ec;
  if (bela::PathExists(outdir)) {
    bela::fs::ForceDeleteFolders(outdir, ec);
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
      bela::FPrintF(stderr, L"baulk rename %s to old error: \x1b[31m%s\x1b[0m\n", pkgdir, ec.message);
      return 1;
    }
  }
  if (!BaulkRename(outdir, pkgdir, ec)) {
    bela::FPrintF(stderr, L"baulk rename %s to %s error: \x1b[31m%s\x1b[0m\n", outdir, pkgdir, ec.message);
    if (!pkgold.empty()) {
      BaulkRename(pkgdir, pkgold, ec);
    }
    return 1;
  }
  // rename exe name
  if (bela::EqualsIgnoreCase(pkg.extension, L"exe")) {
    auto fn = baulk::fs::FileName(pkgfile);
    std::wstring rfn;
    if (!pkg.rename.empty()) {
      rfn = pkg.rename;
    } else {
      rfn = bela::StringCat(pkg.name, std::filesystem::path(pkgfile).extension().wstring());
    }
    if (!bela::EqualsIgnoreCase(fn, rfn)) {
      auto expnadfile = bela::StringCat(pkgdir, L"/", fn);
      auto target = bela::StringCat(pkgdir, L"/", rfn);
      if (MoveFileExW(expnadfile.data(), target.data(), MOVEFILE_REPLACE_EXISTING) != TRUE) {
        ec = bela::make_system_error_code();
        bela::FPrintF(stderr, L"baulk rename package file %s to %s error: \x1b[31m%s\x1b[0m\n", expnadfile, target,
                      ec.message);
      }
    }
  }
  if (!pkgold.empty()) {
    bela::fs::ForceDeleteFolders(pkgold, ec);
  }
  // create a links
  if (!PackageLocalMetaWrite(pkg, ec)) {
    bela::FPrintF(stderr, L"baulk unable write local meta: %s\n", ec.message);
    return 1;
  }
  return PackageMakeLinks(pkg);
}

bool DependenciesExists(const std::vector<std::wstring_view> &dv) {
  for (const auto d : dv) {
    auto pkglock = bela::StringCat(baulk::BaulkRoot(), L"\\bin\\locks\\", d, L".json");
    if (bela::PathFileIsExists(pkglock)) {
      return true;
    }
  }
  return false;
}

void DisplayDependencies(const baulk::Package &pkg) {
  if (pkg.venv.dependencies.empty()) {
    return;
  }
  bela::FPrintF(stderr, L"\x1b[33mPackage '%s' depends on: \x1b[34m%s\x1b[0m", pkg.name,
                bela::StrJoin(pkg.venv.dependencies, L"\n    "));
}

int BaulkInstall(const baulk::Package &pkg) {
  bela::error_code ec;
  auto pkglocal = baulk::bucket::PackageLocalMeta(pkg.name, ec);
  if (pkglocal) {
    bela::version pkgversion(pkg.version);
    bela::version oldversion(pkglocal->version);
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
  if (auto ret = PackageExpand(pkg, *pkgfile); ret != 0) {
    return ret;
  }
  if (!pkg.suggest.empty()) {
    bela::FPrintF(stderr, L"Suggest installing:");
    for (auto it : pkg.suggest) {
      bela::FPrintF(stderr, L"  \x1b[32m%s\x1b[0m", it);
    }
    bela::FPrintF(stderr, L"\n");
  }
  if (!pkg.notes.empty()) {
    bela::FPrintF(stderr, L"Notes: %s\n", pkg.notes);
  }
  DisplayDependencies(pkg);
  return 0;
}
} // namespace baulk::package
