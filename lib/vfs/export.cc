//
#include "vfsinternal.hpp"

namespace baulk::vfs {
bool InitializePathFs(bela::error_code &ec) {
  //
  return PathFs::Instance().Initialize(ec);
}

std::wstring_view PathFsModel() { return PathFs::Instance().Model(); }

// AppBasePath basePath
std::wstring_view AppBasePath() { return PathFs::Instance().Table().basePath; }
// AppData
std::wstring_view AppData() { return PathFs::Instance().Table().appdata; }
// AppVFS
std::wstring_view AppEtc() { return PathFs::Instance().Table().etc; }
// AppVFS
std::wstring_view AppVFS() { return PathFs::Instance().Table().vfs; }
// AppTemp temp dir
std::wstring_view AppTemp() { return PathFs::Instance().Table().temp; }
// AppLocks ...
std::wstring_view AppLocks() { return PathFs::Instance().Table().locks; }
// AppBuckets
std::wstring_view AppBuckets() { return PathFs::Instance().Table().buckets; }
// AppBinLocation
std::wstring_view AppBinLocation() { return PathFs::Instance().Table().binlocation; }
// AppPackagePath find package full path
std::wstring AppPackagePath(std::wstring_view packageName) {
  return bela::StringCat(PathFs::Instance().Table().pakcage_root, L"\\", packageName);
}
// AppPIDFile pid file
std::wstring AppPIDFile() { return bela::StringCat(PathFs::Instance().Table().temp, L"\\baulk.pid"); }
// AppDefaultProfile
std::wstring AppDefaultProfile() {
  return bela::StringCat(PathFs::Instance().Table().basePath, L"\\config\\baulk.json");
}

} // namespace baulk::vfs