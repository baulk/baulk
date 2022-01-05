//
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/io.hpp>
#include <bela/simulator.hpp>
#include <bela/datetime.hpp>
#include <bela/semver.hpp>
#include <baulk/fs.hpp>
#include <baulk/vfs.hpp>
#include <baulk/json_utils.hpp>
#include <baulk/net.hpp>
#include <baulk/hash.hpp>
#include "bucket.hpp"
#include "launcher.hpp"
#include "pkg.hpp"

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
    std::wstring file(vfs::AppLocks());
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

bool PackageForceDelete(std::wstring_view pkgName, bela::error_code &ec) {
  auto pkglocal = baulk::PackageLocalMeta(pkgName, ec);
  if (!pkglocal) {
    return false;
  }
  bela::env::Simulator sim;
  sim.InitializeEnv();
  auto pkgFolder = baulk::vfs::AppPackageFolder(pkgName);
  auto pkgVFS = baulk::vfs::AppPackageVFS(pkgName);
  sim.SetEnv(L"BAULK_ROOT", baulk::vfs::AppBasePath());
  sim.SetEnv(L"BAULK_VFS", pkgVFS);
  sim.SetEnv(L"BAULK_PACKAGE_VFS", pkgVFS);
  sim.SetEnv(L"BAULK_PKGROOT", pkgFolder);
  sim.SetEnv(L"BAULK_PACKAGE_FOLDER", pkgFolder);
  for (const auto &p : pkglocal->forceDeletes) {
    auto realdir = sim.PathExpand(p);
    baulk::DbgPrint(L"force delete: %s@%s", pkgName, realdir);
    bela::error_code ec2;
    if (bela::fs::ForceDeleteFolders(realdir, ec2)) {
      bela::FPrintF(stderr, L"force delete %s \x1b[31m%s\x1b[0m\n", realdir, ec);
    }
  }
  return true;
}

// Package cached
std::optional<std::wstring> PackageCached(std::wstring_view filename, std::wstring_view hash) {
  auto pkgfile = bela::StringCat(vfs::AppTemp(), L"\\", filename);
  if (!bela::PathExists(pkgfile)) {
    return std::nullopt;
  }
  bela::error_code ec;
  if (!baulk::hash::HashEqual(pkgfile, hash, ec)) {
    bela::FPrintF(stderr, L"package file %s error: %s\n", filename, ec);
    return std::nullopt;
  }
  return std::make_optional(std::move(pkgfile));
}

bool PackageMakeLinks(const baulk::Package &pkg) {
  if (!pkg.venv.mkdirs.empty()) {
    bela::env::Simulator sim;
    sim.InitializeEnv();
    auto pkgFolder = baulk::vfs::AppPackageFolder(pkg.name);
    auto pkgVFS = baulk::vfs::AppPackageVFS(pkg.name);
    sim.SetEnv(L"BAULK_ROOT", baulk::vfs::AppBasePath());
    sim.SetEnv(L"BAULK_VFS", pkgVFS);
    sim.SetEnv(L"BAULK_PACKAGE_VFS", pkgVFS);
    sim.SetEnv(L"BAULK_PKGROOT", pkgFolder);
    sim.SetEnv(L"BAULK_PACKAGE_FOLDER", pkgFolder);
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
  baulk::RemovePackageLinks(pkg.name, ec);
  // check package is good installed
  // rebuild launcher and links
  if (!baulk::MakePackageLinks(pkg, true, ec)) {
    bela::FPrintF(stderr, L"baulk unable make %s links: %s\n", pkg.name, ec);
    return false;
  }
  bela::FPrintF(stderr,
                L"baulk install \x1b[35m%s\x1b[0m/\x1b[34m%s\x1b[0m version "
                L"\x1b[32m%s\x1b[0m success.\n",
                pkg.name, pkg.bucket, pkg.version);
  return true;
}

inline bool RenameForce(std::wstring_view source, std::wstring_view target, bela::error_code &ec) {
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

bool PackageExpand(const baulk::Package &pkg, std::wstring_view pkgfile) {
  auto h = baulk::LookupHandler(pkg.extension);
  if (!h) {
    bela::FPrintF(stderr, L"baulk unsupport package extension: %s\n", pkg.extension);
    return false;
  }
  auto outdir = UnarchivePath(pkgfile);
  DbgPrint(L"baulk decompress %s to %s\n", pkg.name, outdir);
  bela::error_code ec;
  if (bela::PathExists(outdir)) {
    bela::fs::ForceDeleteFolders(outdir, ec);
  }
  if (!h->decompress(pkgfile, outdir, ec)) {
    bela::FPrintF(stderr, L"baulk decompress %s error: %s\n", pkgfile, ec);
    return false;
  }
  h->regularize(outdir);
  auto pkgRoot = baulk::vfs::AppPackageFolder(pkg.name);
  std::wstring pkgold;
  std::error_code e;
  if (bela::PathExists(pkgRoot)) {
    pkgold = bela::StringCat(pkgRoot, L".old");
    if (!RenameForce(pkgRoot, pkgold, ec)) {
      bela::FPrintF(stderr, L"baulk rename %s to old error: \x1b[31m%s\x1b[0m\n", pkgRoot, ec);
      return false;
    }
  }
  if (!RenameForce(outdir, pkgRoot, ec)) {
    bela::FPrintF(stderr, L"baulk rename %s to %s error: \x1b[31m%s\x1b[0m\n", outdir, pkgRoot, ec);
    if (!pkgold.empty()) {
      RenameForce(pkgRoot, pkgold, ec);
    }
    return false;
  }
  // rename exe name
  if (bela::EqualsIgnoreCase(pkg.extension, L"exe")) {
    auto fn = baulk::fs::FileName(pkgfile);
    std::wstring rfn;
    if (!pkg.rename.empty()) {
      rfn = pkg.rename;
    } else {
      rfn = bela::StringCat(pkg.name, std::filesystem::path(pkgfile).extension().native());
    }
    if (!bela::EqualsIgnoreCase(fn, rfn)) {
      auto expnadfile = bela::StringCat(pkgRoot, L"/", fn);
      auto target = bela::StringCat(pkgRoot, L"/", rfn);
      if (MoveFileExW(expnadfile.data(), target.data(), MOVEFILE_REPLACE_EXISTING) != TRUE) {
        ec = bela::make_system_error_code();
        bela::FPrintF(stderr, L"baulk rename package file %s to %s error: \x1b[31m%s\x1b[0m\n", expnadfile, target, ec);
      }
    }
  }
  if (!pkgold.empty()) {
    bela::fs::ForceDeleteFolders(pkgold, ec);
  }
  // create a links
  if (!PackageLocalMetaWrite(pkg, ec)) {
    bela::FPrintF(stderr, L"baulk unable write local meta: %s\n", ec);
    return false;
  }
  return PackageMakeLinks(pkg);
}

bool DependenciesExists(const std::vector<std::wstring_view> &dv) {
  for (const auto d : dv) {
    auto pkglock = bela::StringCat(vfs::AppLocks(), L"\\", d, L".json");
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

bool PackageInstall(const baulk::Package &pkg) {
  bela::error_code ec;
  auto pkgLocal = baulk::PackageLocalMeta(pkg.name, ec);
  if (pkgLocal) {
    bela::version pkgVersion(pkg.version);
    bela::version localVersion(pkgLocal->version);
    // new version less installed version or weights < weigths
    if (pkgVersion < localVersion || (pkgVersion == localVersion && pkg.weights <= pkgLocal->weights)) {
      return PackageMakeLinks(pkg);
    }
    if (baulk::IsFrozenedPackage(pkg.name) && !baulk::IsForceMode) {
      // Since the metadata has been updated, we cannot rebuild the frozen
      // package launcher
      bela::FPrintF(stderr,
                    L"baulk \x1b[31mskip upgrade\x1b[0m "
                    L"\x1b[35m%s\x1b[0m(\x1b[31mfrozen\x1b[0m) from "
                    L"\x1b[33m%s\x1b[0m@\x1b[34m%s\x1b[0m to "
                    L"\x1b[32m%s\x1b[0m@\x1b[34m%s\x1b[0m.\n",
                    pkg.name, pkgLocal->version, pkgLocal->bucket, pkg.version, pkg.bucket);
      return true;
    }
    bela::FPrintF(stderr,
                  L"baulk will upgrade \x1b[35m%s\x1b[0m from "
                  L"\x1b[33m%s\x1b[0m@\x1b[34m%s\x1b[0m to "
                  L"\x1b[32m%s\x1b[0m@\x1b[34m%s\x1b[0m\n",
                  pkg.name, pkgLocal->version, pkgLocal->bucket, pkg.version, pkg.bucket);
  }
  auto url = baulk::net::BestUrl(pkg.urls, LocaleName());
  if (url.empty()) {
    bela::FPrintF(stderr, L"baulk: \x1b[31m%s\x1b[0m no valid url\n", pkg.name);
    return false;
  }
  DbgPrint(L"baulk '%s/%s' url: '%s'\n", pkg.name, pkg.version, url);
  auto urlFileName = net::url_path_name(url);
  if (!pkg.hashValue.empty()) {
    DbgPrint(L"baulk '%s/%s' filename: '%s'\n", pkg.name, pkg.version, urlFileName);
    if (auto pkgFile = PackageCached(urlFileName, pkg.hashValue); pkgFile) {
      return PackageExpand(pkg, *pkgFile);
    }
  }
  auto downloads = vfs::AppTemp();
  if (!baulk::fs::MakeDir(downloads, ec)) {
    bela::FPrintF(stderr, L"baulk: unable make %s error: %s\n", downloads, ec);
    return false;
  }
  bela::FPrintF(stderr, L"baulk: download '\x1b[36m%s\x1b[0m' \nurl: \x1b[36m%s\x1b[0m\n", urlFileName, url);
  std::optional<std::wstring> pkgFile;
  for (int i = 0; i < 4; i++) {
    if (i != 0) {
      bela::FPrintF(stderr, L"baulk: download '\x1b[33m%s\x1b[0m' retries: \x1b[33m%d\x1b[0m\n", urlFileName, i);
    }
    if (pkgFile = baulk::net::WinGet(url, downloads, pkg.hashValue, true, ec); !pkgFile) {
      bela::FPrintF(stderr, L"baulk: download '%s' error: \x1b[31m%s\x1b[0m\n", urlFileName, ec);
      continue;
    }
    // hash not check
    if (pkg.hashValue.empty()) {
      break;
    }
    if (hash::HashEqual(*pkgFile, pkg.hashValue, ec)) {
      break;
    }
    bela::FPrintF(stderr, L"baulk download '%s' error: \x1b[31m%s\x1b[0m\n", bela::BaseName(*pkgFile), ec);
  }
  if (!pkgFile) {
    return false;
  }
  if (!PackageExpand(pkg, *pkgFile)) {
    return false;
  }
  if (!pkg.suggest.empty()) {
    bela::FPrintF(stderr, L"Suggest installing: \x1b[32m%s\x1b[0m\n\n",
                  bela::StrJoin(pkg.suggest, L"\x1b[0m\n  \x1b[32m"));
  }
  if (!pkg.notes.empty()) {
    bela::FPrintF(stderr, L"Notes: %s\n", pkg.notes);
  }
  DisplayDependencies(pkg);
  return true;
}
} // namespace baulk::package
