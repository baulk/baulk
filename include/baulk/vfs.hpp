// baulk vfs
#ifndef BAULK_VFS_HPP
#define BAULK_VFS_HPP
#include <bela/base.hpp>
#include <bela/str_cat.hpp>

namespace baulk::vfs {
// InitializePathFs: baulk initialize vfs PathFs
bool InitializePathFs(bela::error_code &ec);
// Executable is packaged
bool IsPackaged();
// Baulk PathFsModel
// - Portable
// - User
// - System
// - Packaged
// - Legacy
std::wstring_view PathFsModel();
// Baulk install path: Only used to find the command path of baulk
// eg:
//    auto baulkLnkExe=bela::StringCat(AppExecutableRoot(),L"\\bin\\baulk-lnk.exe");
std::wstring_view AppExecutableRoot();
// Baulk vfs root directory
std::wstring_view AppBasePath(); // root
std::wstring_view AppData();
std::wstring_view AppEtc();
// Baulk package vfs root
std::wstring_view AppUserVFS();
// Baulk vfs temp dir
std::wstring_view AppTemp();
std::wstring_view AppLocks();
std::wstring_view AppBuckets();
// Baulk app link or launcher location
std::wstring_view AppLinks();
// AppPackagePath: concat package full path
std::wstring AppPackagePath(std::wstring_view packageName);
// AppFsMutexPath: return FsMutex file path
std::wstring AppFsMutexPath();
std::wstring AppDefaultProfile();

} // namespace baulk::vfs

#endif