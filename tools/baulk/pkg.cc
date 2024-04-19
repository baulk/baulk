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
    j["mask"] = bela::integral_cast(pkg.mask);
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
    DbgPrint(L"write %s lock: %s", pkg.name, file);
    return bela::io::AtomicWriteText(file, bela::io::as_bytes<char>(j.dump(4)), ec);
  } catch (const std::exception &e) {
    ec = bela::make_error_code(bela::ErrGeneral, bela::encode_into<char, wchar_t>(e.what()));
  }
  return false;
}

bool Drop(std::wstring_view pkgName, bela::error_code &ec) {
  auto pkgLocal = baulk::PackageLocalMeta(pkgName, ec);
  if (!pkgLocal) {
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
  for (const auto &p : pkgLocal->forceDeletes) {
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
std::optional<std::filesystem::path> PackageCached(const std::filesystem::path &downloads, std::wstring_view filename,
                                                   std::wstring_view hash) {
  std::filesystem::path archive_file = downloads / filename;
  std::error_code e;
  if (!std::filesystem::exists(archive_file, e)) {
    return std::nullopt;
  }
  bela::error_code ec;
  if (!baulk::hash::HashEqual(archive_file, hash, ec)) {
    bela::FPrintF(stderr, L"package file %s error: %s\n", filename, ec);
    return std::nullopt;
  }
  return std::make_optional(std::move(archive_file));
}

bool NewLinks(const baulk::Package &pkg) {
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
  baulk::DropLinks(pkg.name, ec);
  // check package is good installed
  // rebuild launcher and links
  if (!baulk::MakeLinks(pkg, true, ec)) {
    bela::FPrintF(stderr, L"baulk unable make %s links: %s\n", pkg.name, ec);
    return false;
  }
  bela::FPrintF(stderr,
                L"baulk install \x1b[35m%s\x1b[0m/\x1b[34m%s\x1b[0m version "
                L"\x1b[32m%s\x1b[0m success.\n",
                pkg.name, pkg.bucket, pkg.version);
  return true;
}

// single exe package
bool expand_fallback_exe(const baulk::Package &pkg, const std::filesystem::path &archive_file) {
  std::filesystem::path packages(baulk::vfs::AppPackages());
  auto pkgRoot = packages / pkg.name;
  auto oldPath = packages / bela::StringCat(pkg.name, L".out");
  auto exefile = bela::StringCat(pkg.name, L".exe");
  std::error_code e;
  bela::error_code ec;
  if (std::filesystem::exists(pkgRoot, e)) {
    if (std::filesystem::rename(pkgRoot, oldPath, e); e) {
      ec = e;
      bela::FPrintF(stderr, L"baulk rename %s to %s error: \x1b[31m%s\x1b[0m\n", pkgRoot, oldPath, ec);
      return false;
    }
  }
  if (!std::filesystem::create_directories(pkgRoot, e)) {
    ec = e;
    bela::FPrintF(stderr, L"baulk rename %s to %s error: \x1b[31m%s\x1b[0m\n", pkgRoot, oldPath, ec);
    std::filesystem::rename(oldPath, pkgRoot, e);
    return false;
  }
  auto exePath = pkgRoot / exefile;
  if (!std::filesystem::copy_file(archive_file, exePath, std::filesystem::copy_options::overwrite_existing, e)) {
    ec = e;
    bela::FPrintF(stderr, L"baulk rename %s to %s error: \x1b[31m%s\x1b[0m\n", pkgRoot, oldPath, ec);
    std::filesystem::rename(oldPath, pkgRoot, e);
    return false;
  }
  auto pkgCopy = pkg;
  pkgCopy.links.emplace_back(exefile, exefile);
  pkgCopy.mask |= MaskCompatibilityMode; // keep launcher
  if (!PackageLocalMetaWrite(pkgCopy, ec)) {
    bela::FPrintF(stderr, L"baulk write local meta error: %s\n", ec);
    return false;
  }
  std::filesystem::remove_all(oldPath, e);
  return NewLinks(pkgCopy);
}

bool Expand(const baulk::Package &pkg, const std::filesystem::path &archive_file) {
  auto fn = baulk::resolve_extract_handle(pkg.extension);
  if (!fn) {
    bela::FPrintF(stderr, L"baulk unsupport package extension: %s\n", pkg.extension);
    return false;
  }
  std::filesystem::path strict_folder;
  auto destination = baulk::make_unqiue_extracted_destination(archive_file, strict_folder);
  if (!destination) {
    bela::FPrintF(stderr, L"destination '%v' already exists\n", strict_folder);
    return false;
  }
  bela::error_code ec;
  if (!fn(archive_file, *destination, ec)) {
    if (ec == baulk::archive::ErrNoOverlayArchive) {
      return expand_fallback_exe(pkg, archive_file);
    }
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
    bela::FPrintF(stderr, L"baulk write local meta error: %s\n", ec);
    return false;
  }
  return NewLinks(pkg);
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

bool Install(const baulk::Package &pkg) {
  bela::error_code ec;
  auto pkgLocal = baulk::PackageLocalMeta(pkg.name, ec);
  if (pkgLocal) {
    bela::version pkgVersion(pkg.version);
    bela::version localVersion(pkgLocal->version);
    // new version less installed version or weights < weigths
    if (pkgVersion < localVersion || (pkgVersion == localVersion && pkg.weights <= pkgLocal->weights)) {
      if ((pkgLocal->mask & MaskCompatibilityMode) != 0) {
        bela::FPrintF(stderr,
                      L"baulk already installed \x1b[35m%s\x1b[0m/\x1b[34m%s\x1b[0m version \x1b[32m%s\x1b[0m "
                      L"[\x1b[36mCompatibility Mode\x1b[0m]\n",
                      pkg.name, pkg.bucket, pkgLocal->version);
        return true;
      }
      return NewLinks(pkg);
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
  std::filesystem::path downloads(vfs::AppTemp());
  auto filename = net::url_path_name(url);
  if (!pkg.hash.empty()) {
    DbgPrint(L"baulk '%s/%s' filename: '%s'\n", pkg.name, pkg.version, filename);
    if (auto archive_file = PackageCached(downloads, filename, pkg.hash); archive_file) {
      return Expand(pkg, *archive_file);
    }
  }
  if (!baulk::fs::MakeDirectories(downloads, ec)) {
    bela::FPrintF(stderr, L"baulk: unable make %s error: %s\n", downloads, ec);
    return false;
  }
  bela::FPrintF(stderr, L"baulk: download '\x1b[36m%s\x1b[0m' \nurl: \x1b[36m%s\x1b[0m\n", filename, url);
  std::optional<std::filesystem::path> archive_file;
  for (int i = 0; i < 4; i++) {
    if (i != 0) {
      bela::FPrintF(stderr, L"baulk: download '\x1b[33m%s\x1b[0m' retries: \x1b[33m%d\x1b[0m\n", filename, i);
    }
    //  downloads, pkg.hash, true
    if (archive_file = baulk::net::WinGet(url,
                                          {
                                              .hash_value = pkg.hash,
                                              .cwd = downloads,
                                              .force_overwrite = true,
                                          },
                                          ec);
        !archive_file) {
      bela::FPrintF(stderr, L"baulk: download '%s' error: \x1b[31m%s\x1b[0m\n", filename, ec);
      continue;
    }
    // hash not check
    if (pkg.hash.empty()) {
      break;
    }
    if (hash::HashEqual(*archive_file, pkg.hash, ec)) {
      break;
    }
    bela::FPrintF(stderr, L"baulk download '%s' error: \x1b[31m%s\x1b[0m\n", archive_file->filename(), ec);
  }
  if (!archive_file) {
    return false;
  }
  if (!Expand(pkg, *archive_file)) {
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
