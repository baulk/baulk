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
std::wstring_view AppData();
std::wstring_view AppEtc();
std::wstring_view AppVFS();
std::wstring_view AppTemp();
std::wstring_view AppLocks();
std::wstring_view AppBuckets();
std::wstring_view AppBinLocation();
// cat ..
std::wstring AppPackagePath(std::wstring_view packageName);
std::wstring AppPIDFile();
std::wstring AppDefaultProfile();

} // namespace baulk::vfs

#endif