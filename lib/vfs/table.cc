#include "vfsinternal.hpp"
#include <bela/path.hpp>
#include <bela/env.hpp>
#include <filesystem>

namespace baulk::vfs::vfs_internal {
using namespace std::string_view_literals;
// baulk >=4.0
bool FsRedirectionTable::InitializeFromNewest(bela::error_code &ec) {
  if (basePath.empty()) {
    ec = bela::make_error_code(L"BUG: basePath is empty");
    return false;
  }
  etc = bela::StringCat(basePath, L"\\etc"sv);
  appData = bela::StringCat(basePath, L"\\appdata"sv);
  vfs = bela::StringCat(basePath, L"\\vfs"sv);
  packages = bela::StringCat(basePath, L"\\packages"sv);
  temp = bela::StringCat(basePath, L"\\temp"sv);
  locks = bela::StringCat(basePath, L"\\locks"sv);
  buckets = bela::StringCat(basePath, L"\\buckets"sv);
  appLinks = bela::StringCat(basePath, L"\\local\\bin"sv);
  return true;
}

// Protable (baulk >=4.0)
bool FsRedirectionTable::InitializeFromPortable(bela::error_code &ec) {
  basePath = appLocation;
  return InitializeFromNewest(ec);
}

// Legacy Install (baulk <=3.0)
bool FsRedirectionTable::InitializeFromLegacy(bela::error_code &ec) {
  basePath = appLocation;
  etc = bela::StringCat(basePath, L"\\bin\\etc"sv);
  appData = bela::StringCat(basePath, L"\\bin\\appdata"sv);
  vfs = bela::StringCat(basePath, L"\\bin\\vfs"sv);
  packages = bela::StringCat(basePath, L"\\bin\\pkgs"sv);
  temp = bela::StringCat(basePath, L"\\bin\\pkgs\\.pkgtmp"sv);
  locks = bela::StringCat(basePath, L"\\bin\\locks"sv);
  buckets = bela::StringCat(basePath, L"\\buckets"sv);
  appLinks = bela::StringCat(basePath, L"\\bin\\links"sv);
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