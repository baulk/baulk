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

// AppPackagePath find package full path
std::wstring AppPackagePath(std::wstring_view packageName) {
  //
  return bela::StringCat(PathFs::Instance().Table().pakcage_root, L"\\", packageName);
}

} // namespace baulk::vfs