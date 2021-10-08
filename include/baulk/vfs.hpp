// baulk vfs
#ifndef BAULK_VFS_HPP
#define BAULK_VFS_HPP
#include <bela/base.hpp>
#include <bela/str_cat.hpp>

namespace baulk::vfs {
// InitializePathFs: baulk initialize vfs PathFs
bool InitializePathFs(bela::error_code &ec);
bool IsPackaged();
std::wstring_view PathFsModel();
std::wstring_view AppBasePath(); // root
std::wstring AppPackagePath(std::wstring_view packageName);
std::wstring_view AppData();
std::wstring_view AppVFS();

inline std::wstring AppProfile() {
  //
  return bela::StringCat(AppBasePath(), L"\\config\\baulk.json");
}

} // namespace baulk::vfs

#endif