//
#include "vfsinternal.hpp"

namespace baulk::vfs {
bool InitializePathFs(bela::error_code &ec) {
  if(!PathFs::Instance().Initialize(ec)){
    return false;
  }
  return PathFs::Instance().NewFsPaths(ec);
}

bool InitializeFastPathFs(bela::error_code &ec) {
  //
  return PathFs::Instance().Initialize(ec);
}

std::wstring_view PathFsModel() { return PathFs::Instance().Model(); }
// AppExecutableRoot execuatble root
std::wstring_view AppExecutableRoot() { return PathFs::Instance().Table().executableRoot; }
// AppBasePath basePath
std::wstring_view AppBasePath() { return PathFs::Instance().Table().basePath; }
// AppData
std::wstring_view AppData() { return PathFs::Instance().Table().appData; }
// AppEtc
std::wstring_view AppEtc() { return PathFs::Instance().Table().etc; }
// AppUserVFS
std::wstring_view AppUserVFS() { return PathFs::Instance().Table().userVFS; }
// AppTemp temp dir
std::wstring_view AppTemp() { return PathFs::Instance().Table().temp; }
// AppLocks ...
std::wstring_view AppLocks() { return PathFs::Instance().Table().locks; }
// AppBuckets
std::wstring_view AppBuckets() { return PathFs::Instance().Table().buckets; }
// AppLinks
std::wstring_view AppLinks() { return PathFs::Instance().Table().appLinks; }
// AppPackageRoot find package full path
std::wstring AppPackageRoot(std::wstring_view packageName) {
  return bela::StringCat(PathFs::Instance().Table().packageRoot, L"\\", packageName);
}
// AppFsMutexPath pid file path
std::wstring AppFsMutexPath() { return bela::StringCat(PathFs::Instance().Table().temp, L"\\baulk.pid"); }
// AppDefaultProfile
std::wstring AppDefaultProfile() {
  return bela::StringCat(PathFs::Instance().Table().basePath, L"\\config\\baulk.json");
}

} // namespace baulk::vfs