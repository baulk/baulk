/// migrate legacy mode to new Portable mode
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/fs.hpp>
#include <bela/io.hpp>
#include <baulk/vfs.hpp>
#include <baulk/fs.hpp>

bool InitializePathFs(std::wstring_view basePath, bela::error_code &ec) {
  constexpr std::wstring_view envContent = LR"({
    // Do not modify, baulk relies on this file to select the operating mode  
    "mode": "Portable"
})";
  auto baulkEnv = bela::StringCat(basePath, L"\\baulk.env");
  if (!bela::io::WriteText(envContent, baulkEnv, ec)) {
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
    return baulk::fs::MakeDir(target, ec);
  }
  if (!baulk::fs::MakeParentDir(target, ec)) {
    return false;
  }
  std::filesystem::rename(source, target, e);
  if (e) {
    ec = bela::from_std_error_code(e);
    return false;
  }
  return true;
}

int wmain(int argc, wchar_t **argv) {
  bela::error_code ec;
  if (!baulk::vfs::InitializeFastPathFs(ec)) {
    bela::FPrintF(stderr, L"baulk-mirage InitializeFastPathFs error %s\n", ec.message);
    return false;
  }
  if (baulk::vfs::AppMode() != baulk::vfs::LegacyMode) {
    bela::FPrintF(stderr, L"baulk-migrate: current mode is %s not need migrate\n", baulk::vfs::AppMode());
    return 0;
  }
  const auto &legacyTable = baulk::vfs::vfs_internal::AppPathFsTable();
  baulk::vfs::vfs_internal::FsRedirectionTable newTable(legacyTable.appLocation);
  if (!newTable.InitializeFromPortable(ec)) {
    bela::FPrintF(stderr, L"baulk-mirage InitializeFromPortable error %s\n", ec.message);
    return 1;
  }
  if (!moveFolder(legacyTable.appData, newTable.appData, ec)) {
    bela::FPrintF(stderr, L"baulk-mirage move folder 'appdata' error %s\n", ec.message);
    return 1;
  }
  if (!moveFolder(legacyTable.etc, newTable.etc, ec)) {
    bela::FPrintF(stderr, L"baulk-mirage move folder 'etc' error %s\n", ec.message);
    return 1;
  }
  if (!moveFolder(legacyTable.vfs, newTable.vfs, ec)) {
    bela::FPrintF(stderr, L"baulk-mirage move folder 'vfs' error %s\n", ec.message);
    return 1;
  }
  if (!moveFolder(legacyTable.temp, newTable.temp, ec)) {
    bela::FPrintF(stderr, L"baulk-mirage move folder 'temp' error %s\n", ec.message);
    return 1;
  }
  if (!moveFolder(legacyTable.packages, newTable.packages, ec)) {
    bela::FPrintF(stderr, L"baulk-mirage move folder 'packages' error %s\n", ec.message);
    return 1;
  }

  if (!moveFolder(legacyTable.locks, newTable.locks, ec)) {
    bela::FPrintF(stderr, L"baulk-mirage move folder 'locks' error %s\n", ec.message);
    return 1;
  }
  if (!moveFolder(legacyTable.buckets, newTable.buckets, ec)) {
    bela::FPrintF(stderr, L"baulk-mirage move folder 'buckets' error %s\n", ec.message);
    return 1;
  }
  if (!moveFolder(legacyTable.appLinks, newTable.appLinks, ec)) {
    bela::FPrintF(stderr, L"baulk-mirage move folder 'appLinks' error %s\n", ec.message);
    return 1;
  }
  if (!InitializePathFs(newTable.basePath, ec)) {
    bela::FPrintF(stderr, L"baulk-mirage initialize PathFs error %s\n", ec.message);
    return 1;
  }
  bela::FPrintF(stderr, L"baulk-migrate: migrate success\n");
  return 0;
}