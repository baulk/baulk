#include "vfsinternal.hpp"
#include <bela/path.hpp>
#include <bela/env.hpp>
#include <filesystem>

namespace baulk::vfs::vfs_internal {

// baulk >=4.0
bool FsRedirectionTable::InitializeFromNewest(bela::error_code &ec) {
  etc = bela::StringCat(basePath, L"\\etc");
  appData = bela::StringCat(basePath, L"\\appdata");
  userVFS = bela::StringCat(basePath, L"\\uservfs");
  packageRoot = bela::StringCat(basePath, L"\\packages");
  temp = bela::StringCat(basePath, L"\\temp");
  locks = bela::StringCat(basePath, L"\\locks");
  buckets = bela::StringCat(basePath, L"\\buckets");
  appLinks = bela::StringCat(basePath, L"\\local\\bin");
  return true;
}

// Protable (baulk >=4.0)
bool FsRedirectionTable::InitializeFromPortable(bela::error_code &ec) {
  basePath = executableRoot;
  return InitializeFromNewest(ec);
}

// Legacy Install (baulk <=3.0)
bool FsRedirectionTable::InitializeFromLegacy(bela::error_code &ec) {
  basePath = executableRoot;
  etc = bela::StringCat(basePath, L"\\bin\\etc");
  appData = bela::StringCat(basePath, L"\\bin\\appdata");
  userVFS = bela::StringCat(basePath, L"\\bin\\vfs");
  packageRoot = bela::StringCat(basePath, L"\\bin\\pkgs");
  temp = bela::StringCat(basePath, L"\\bin\\pkgs\\.pkgtmp");
  locks = bela::StringCat(basePath, L"\\bin\\locks");
  buckets = bela::StringCat(basePath, L"\\buckets");
  appLinks = bela::StringCat(basePath, L"\\bin\\links");
  return true;
}

// Windows Store (baulk >=4.0)
bool FsRedirectionTable::InitializeFromPackaged(bela::error_code &ec) {
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