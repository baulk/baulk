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
#include "extractor.hpp"

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
    if (!baulk::fs::MakeDirectories(file, ec)) {
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
    sim.SetEnv(L"BAULK_APPDATA", baulk::vfs::AppData());
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

bool PackageExpand(const baulk::Package &pkg, std::wstring_view pkgfile) {
  auto fn = baulk::resolve_extract_handle(pkg.extension);
  if (!fn) {
    bela::FPrintF(stderr, L"baulk unsupport package extension: %s\n", pkg.extension);
    return false;
  }
  std::filesystem::path archive_file(pkgfile);
  std::filesystem::path strict_folder;
  auto destination = baulk::make_unqiue_extracted_destination(archive_file, strict_folder);
  if (!destination) {
    bela::FPrintF(stderr, L"destination '%v' already exists\n", strict_folder);
    return false;
  }
  bela::error_code ec;
  if (!fn(archive_file, *destination, ec)) {
    bela::FPrintF(stderr, L"baulk extract: %v error: %v\n", archive_file.filename(), ec);
    return false;
  }
  std::filesystem::path packages(baulk::vfs::AppPackages());
  auto pkgRoot = packages / pkg.name;
  std::error_code e;
  // rename failed
  if (![&]() -> bool {
        std::wstring oldPath;
        if (std::filesystem::exists(pkgRoot, e)) {
          oldPath = bela::StringCat(pkgRoot, L".old");
          if (std::filesystem::rename(pkgRoot, oldPath, e); e) {
            bela::FPrintF(stderr, L"baulk rename %s to %s error: \x1b[31m%s\x1b[0m\n", pkgRoot, oldPath, ec);
            return false;
          }
        }
        if (std::filesystem::rename(*destination, pkgRoot, e); e) {
          bela::FPrintF(stderr, L"baulk rename %s to %s error: \x1b[31m%s\x1b[0m\n", *destination, pkgRoot, ec);
          if (!oldPath.empty()) {
            std::filesystem::rename(oldPath, pkgRoot, e);
          }
          return false;
        }
        if (!oldPath.empty()) {
          std::filesystem::remove_all(oldPath, e);
        }
        return true;
      }()) {
    return false;
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
  if (!baulk::fs::MakeDirectories(downloads, ec)) {
    bela::FPrintF(stderr, L"baulk: unable make %s error: %s\n", downloads, ec);
    return false;
  }
  bela::FPrintF(stderr, L"baulk: download '\x1b[36m%s\x1b[0m' \nurl: \x1b[36m%s\x1b[0m\n", urlFileName, url);
  std::optional<std::wstring> pkgFile;
  for (int i = 0; i < 4; i++) {
    if (i != 0) {
      bela::FPrintF(stderr, L"baulk: download '\x1b[33m%s\x1b[0m' retries: \x1b[33m%d\x1b[0m\n", urlFileName, i);
    }
    //  downloads, pkg.hashValue, true
    if (pkgFile = baulk::net::WinGet(url,
                                     {
                                         .hash_value = pkg.hashValue,
                                         .cwd = downloads,
                                         .force_overwrite = true,
                                     },
                                     ec);
        !pkgFile) {
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
    bela::FPrintF(stderr, L"'%s' suggests installing: '\x1b[32m%s\x1b[0m'\n", pkg.name,
                  bela::StrJoin(pkg.suggest, L"\x1b[0m' or '\x1b[32m"));
  }
  if (!pkg.notes.empty()) {
    bela::FPrintF(stderr, L"'%s' notes\n-----\n%s\n", pkg.name, pkg.notes);
  }
  DisplayDependencies(pkg);
  return true;
}
} // namespace baulk::package
