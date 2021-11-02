//
#include "vfsinternal.hpp"

namespace baulk::vfs {
namespace vfs_internal {
const FsRedirectionTable &AppPathFsTable() { return baulk::vfs::PathFs::Instance().Table(); }
} // namespace vfs_internal
bool InitializePathFs(bela::error_code &ec) {
  if (!PathFs::Instance().Initialize(ec)) {
    return false;
  }
  return PathFs::Instance().NewFsPaths(ec);
}

bool InitializeFastPathFs(bela::error_code &ec) {
  //
  return PathFs::Instance().Initialize(ec);
}

std::wstring_view AppMode() { return PathFs::Instance().Mode(); }
// AppLocation app location
std::wstring_view AppLocation() { return PathFs::Instance().Table().appLocation; }

std::wstring AppLocationPath(std::wstring_view name) {
  const auto &table = PathFs::Instance().Table();
  if (name.empty()) {
    if (table.appLocationFlat) {
      return table.appLocation;
    }
    return bela::StringCat(table.appLocation, L"\\bin");
  }
  if (table.appLocationFlat) {
    return bela::StringCat(table.appLocation, L"\\", name);
  }
  return bela::StringCat(table.appLocation, L"\\bin\\", name);
}

// AppBasePath basePath
std::wstring_view AppBasePath() { return PathFs::Instance().Table().basePath; }
// AppData
std::wstring_view AppData() { return PathFs::Instance().Table().appData; }
// AppEtc
std::wstring_view AppEtc() { return PathFs::Instance().Table().etc; }
// AppTemp temp dir
std::wstring_view AppTemp() { return PathFs::Instance().Table().temp; }
// AppPackages packages install local
std::wstring_view AppPackages() { return PathFs::Instance().Table().packages; }
// AppLocks ...
std::wstring_view AppLocks() { return PathFs::Instance().Table().locks; }
// AppBuckets
std::wstring_view AppBuckets() { return PathFs::Instance().Table().buckets; }
// AppLinks
std::wstring_view AppLinks() { return PathFs::Instance().Table().appLinks; }
// AppPackageFolder find package full path
std::wstring AppPackageFolder(std::wstring_view packageName) {
  return bela::StringCat(PathFs::Instance().Table().packages, L"\\", packageName);
}

// AppPackageVFS
std::wstring AppPackageVFS(std::wstring_view packageName) { return PathFs::Instance().PackageVFS(packageName); }

// AppFsMutexPath pid file path
std::wstring AppFsMutexPath() { return bela::StringCat(PathFs::Instance().Table().temp, L"\\baulk.pid"); }
// AppDefaultProfile
std::wstring AppDefaultProfile() {
  return bela::StringCat(PathFs::Instance().Table().basePath, L"\\config\\baulk.json");
}

} // namespace baulk::vfs