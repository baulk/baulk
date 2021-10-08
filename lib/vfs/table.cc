#include "vfsinternal.hpp"
#include <bela/path.hpp>
#include <bela/env.hpp>
#include <filesystem>

namespace baulk::vfs::vfs_internal {

bool create_directories(std::wstring_view d, bela::error_code &ec) {
  if (bela::PathExists(d, bela::FileAttribute::Dir)) {
    return true;
  }
  std::error_code e;
  if (!std::filesystem::create_directories(d, e)) {
    ec = bela::from_std_error_code(e, L"create_directories: ");
    return false;
  }
  return true;
}

bool pathFsNewBinLocation(const FsRedirectionTable &table, bela::error_code &ec) {
  if (!create_directories(table.binlocation, ec)) {
    return false;
  }
  create_directories(table.temp, ec);
  // TODO: create_directories others ?
  return true;
}

// baulk >=4.0
bool FsRedirectionTable::InitializeFromNewest(bela::error_code &ec) {
  etc = bela::StringCat(basePath, L"\\etc");
  vfs = bela::StringCat(basePath, L"\\vfs");
  pakcage_root = bela::StringCat(basePath, L"\\packages");
  temp = bela::StringCat(basePath, L"\\tmp");
  locks = bela::StringCat(basePath, L"\\locks");
  buckets = bela::StringCat(basePath, L"\\buckets");
  binlocation = bela::StringCat(basePath, L"\\local\\bin");
  return pathFsNewBinLocation(*this, ec);
}

// Protable (baulk >=4.0)
bool FsRedirectionTable::InitializeFromPortable(std::wstring_view portableRoot, bela::error_code &ec) {
  basePath = portableRoot;
  return InitializeFromNewest(ec);
}

// Legacy Install (baulk <=3.0)
bool FsRedirectionTable::InitializeFromLegacy(std::wstring_view portableRoot, bela::error_code &ec) {
  basePath = portableRoot;
  etc = bela::StringCat(basePath, L"\\bin\\etc");
  vfs = bela::StringCat(basePath, L"\\bin\\vfs");
  pakcage_root = bela::StringCat(basePath, L"\\bin\\pkgs");
  temp = bela::StringCat(basePath, L"\\bin\\pkgs\\.pkgtmp");
  locks = bela::StringCat(basePath, L"\\bin\\locks");
  buckets = bela::StringCat(basePath, L"\\buckets");
  binlocation = bela::StringCat(basePath, L"\\bin\\links");
  return true;
}

// Windows Store (baulk >=4.0)
bool FsRedirectionTable::InitializeFromDesktopBridge(bela::error_code &ec) {
  basePath = GetAppBasePath();
  return InitializeFromNewest(ec);
}

// User Install (baulk >=4.0)
bool FsRedirectionTable::InitializeFromLocalAppData(bela::error_code &ec) {
  basePath = bela::WindowsExpandEnv(L"%LOCALAPPDATA%\\baulk");
  return InitializeFromNewest(ec);
}

// System Install (baulk >=4.0)
bool FsRedirectionTable::InitializeFromSystemAppData(bela::error_code &ec) {
  basePath = bela::WindowsExpandEnv(L"%SystemDrive%\\baulk");
  return InitializeFromNewest(ec);
}
} // namespace baulk::vfs::vfs_internal