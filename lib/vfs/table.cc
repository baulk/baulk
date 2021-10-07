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
  // TODO: create_directories others ?
  return true;
}

// baulk >=4.0
bool FsRedirectionTable::InitializeFromNewest(bela::error_code &ec) {
  etc = bela::StringCat(root, L"\\etc");
  vfs = bela::StringCat(root, L"\\vfs");
  pakcage_root = bela::StringCat(root, L"\\packages");
  temp = bela::StringCat(root, L"\\tmp");
  locks = bela::StringCat(root, L"\\locks");
  buckets = bela::StringCat(root, L"\\buckets");
  binlocation = bela::StringCat(root, L"\\local\\bin");
  return pathFsNewBinLocation(*this, ec);
}

// Protable (baulk >=4.0)
bool FsRedirectionTable::InitializeFromPortable(std::wstring_view portableRoot, bela::error_code &ec) {
  root = portableRoot;
  return InitializeFromNewest(ec);
}

// Legacy Install (baulk <=3.0)
bool FsRedirectionTable::InitializeFromLegacy(std::wstring_view portableRoot, bela::error_code &ec) {
  root = portableRoot;
  etc = bela::StringCat(portableRoot, L"\\bin\\etc");
  vfs = bela::StringCat(portableRoot, L"\\bin\\vfs");
  pakcage_root = bela::StringCat(portableRoot, L"\\bin\\pkgs");
  temp = bela::StringCat(portableRoot, L"\\bin\\pkgs\\.pkgtmp");
  locks = bela::StringCat(portableRoot, L"\\bin\\locks");
  buckets = bela::StringCat(portableRoot, L"\\buckets");
  binlocation = bela::StringCat(portableRoot, L"\\bin\\links");
  return true;
}

// Windows Store (baulk >=4.0)
bool FsRedirectionTable::InitializeFromDesktopBridge(bela::error_code &ec) {
  //
  return true;
}

// User Install (baulk >=4.0)
bool FsRedirectionTable::InitializeFromLocalAppData(bela::error_code &ec) {
  auto root = bela::WindowsExpandEnv(L"%LOCALAPPDATA%\\baulk");
  if (!bela::PathExists(root, bela::FileAttribute::Dir)) {
  }
  return InitializeFromNewest(ec);
}

// System Install (baulk >=4.0)
bool FsRedirectionTable::InitializeFromSystemAppData(bela::error_code &ec) {
  auto root = bela::WindowsExpandEnv(L"%SystemDrive%\\baulk");
  return InitializeFromNewest(ec);
}
} // namespace baulk::vfs::vfs_internal