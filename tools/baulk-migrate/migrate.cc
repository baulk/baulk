//
#include "migrate.hpp"
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/fs.hpp>
#include <bela/io.hpp>
#include <bela/strip.hpp>
#include <baulk/vfs.hpp>
#include <baulk/fs.hpp>
#include <baulk/fsmutex.hpp>
#include <bela/process.hpp>

namespace baulk {
bool IsDebugMode = true;
bool InitializePathFs(std::wstring_view basePath, bela::error_code &ec) {
  constexpr std::wstring_view envContent = LR"({
    // Do not modify, baulk relies on this file to select the operating mode  
    "mode": "Portable"
})";
  auto baulkEnv = bela::StringCat(basePath, L"\\baulk.env");
  if (!bela::io::WriteText(baulkEnv, envContent, ec)) {
    return false;
  }
  return true;
}
bool moveFolder(std::wstring_view source, std::wstring_view target, bela::error_code &ec) {
  std::error_code e;
  if (std::filesystem::equivalent(source, target, e)) {
    return true;
  }
  if (!bela::PathExists(source, bela::FileAttribute::Dir)) {
    return baulk::fs::MakeDirectories(target, ec);
  }
  if (!baulk::fs::MakeParentDirectories(target, ec)) {
    return false;
  }
  std::filesystem::rename(source, target, e);
  if (e) {
    ec = bela::make_error_code_from_std(e);
    return false;
  }
  DbgPrint(L"move from %s to %s", source, target);
  return true;
}

bool IsLocked() {
  bela::error_code ec;
  auto mtx = MakeFsMutex(vfs::AppFsMutexPath(), ec);
  if (!mtx) {
    bela::FPrintF(stderr, L"baulk-migrate: \x1b[31mMakeFsMutex %s %s\x1b[0m\n", vfs::AppFsMutexPath(), ec);
    return true;
  }
  return false;
}

int Migrator::Execute() {
  bela::error_code ec;
  if (!baulk::vfs::InitializeFastPathFs(ec)) {
    bela::FPrintF(stderr, L"baulk-migrate InitializeFastPathFs error %s\n", ec);
    return 1;
  }
  if (baulk::vfs::AppMode() != baulk::vfs::LegacyMode) {
    bela::FPrintF(stderr, L"baulk-migrate: current mode is %s not need migrate\n", baulk::vfs::AppMode());
    return 0;
  }
  if (IsLocked()) {
    return 1;
  }
  const auto &legacyTable = baulk::vfs::vfs_internal::AppPathFsTable();
  constexpr std::wstring_view fsExt{L".json"};
  std::error_code e;
  for (const auto &pkg : std::filesystem::directory_iterator{legacyTable.locks, e}) {
    auto filename = pkg.path().filename().wstring();
    if (bela::EndsWithIgnoreCase(filename, fsExt)) {
      auto pkgName = bela::StripSuffix(filename, L".json");
      DbgPrint(L"found %v", pkgName);
      pkgs.emplace_back(pkgName);
    }
  }
  baulk::vfs::vfs_internal::FsRedirectionTable newTable(legacyTable.appLocation);
  if (!newTable.InitializeFromPortable(ec)) {
    bela::FPrintF(stderr, L"baulk-migrate InitializeFromPortable error %s\n", ec);
    return 1;
  }
  if (!moveFolder(legacyTable.appData, newTable.appData, ec)) {
    bela::FPrintF(stderr, L"baulk-migrate move folder 'appdata' error %s\n", ec);
    return 1;
  }
  if (!moveFolder(legacyTable.etc, newTable.etc, ec)) {
    bela::FPrintF(stderr, L"baulk-migrate move folder 'etc' error %s\n", ec);
    return 1;
  }
  if (!moveFolder(legacyTable.vfs, newTable.vfs, ec)) {
    bela::FPrintF(stderr, L"baulk-migrate move folder 'vfs' error %s\n", ec);
    return 1;
  }
  if (!moveFolder(legacyTable.temp, newTable.temp, ec)) {
    bela::FPrintF(stderr, L"baulk-migrate move folder 'temp' error %s\n", ec);
    return 1;
  }
  if (!moveFolder(legacyTable.packages, newTable.packages, ec)) {
    bela::FPrintF(stderr, L"baulk-migrate move folder 'packages' error %s\n", ec);
    return 1;
  }

  if (!moveFolder(legacyTable.locks, newTable.locks, ec)) {
    bela::FPrintF(stderr, L"baulk-migrate move folder 'locks' error %s\n", ec);
    return 1;
  }
  if (!moveFolder(legacyTable.buckets, newTable.buckets, ec)) {
    bela::FPrintF(stderr, L"baulk-migrate move folder 'buckets' error %s\n", ec);
    return 1;
  }
  if (!moveFolder(legacyTable.appLinks, newTable.appLinks, ec)) {
    bela::FPrintF(stderr, L"baulk-migrate move folder 'appLinks' error %s\n", ec);
    return 1;
  }
  if (!InitializePathFs(newTable.basePath, ec)) {
    bela::FPrintF(stderr, L"baulk-migrate initialize PathFs error %s\n", ec);
    return 1;
  }
  return 0;
}

int Migrator::PostMigrate() {
  for (const auto &pkg : pkgs) {
    bela::FPrintF(stderr, L"reinstall: %v\n", pkg);
    make_link_once(pkg);
  }
  return 0;
}

bool Migrator::make_link_once(std::wstring_view pkgName) {
  bela::process::Process ps;
  if (ps.Execute(L"baulk-exec", L"--cleanup", L"--vs", L"baulk.exe", L"i", pkgName) != 0) {
    bela::FPrintF(stderr, L"unable reinstall %s error: %v\n", pkgName, ps.ErrorCode());
    return false;
  }
  return true;
}

} // namespace baulk