// baulk vfs
#ifndef BAULK_VFS_HPP
#define BAULK_VFS_HPP
#include <bela/base.hpp>
#include <bela/str_cat.hpp>

namespace baulk::vfs {
namespace vfs_internal {
class FsRedirectionTable {
public:
  FsRedirectionTable() = default;
  FsRedirectionTable(std::wstring_view exeRoot) : appLocation(exeRoot) {}
  FsRedirectionTable(const FsRedirectionTable &) = delete;
  FsRedirectionTable &operator=(const FsRedirectionTable &) = delete;
  std::wstring appLocation; // baulk.exe root
  std::wstring basePath;    // baulk root
  std::wstring appData;     // baulk appdata
  std::wstring etc;         // baulk etc
  std::wstring vfs;         // baulk pakcage vfs
  std::wstring packages;    // baulk package root
  std::wstring temp;        // baulk temp
  std::wstring locks;       // baulk locks
  std::wstring buckets;     // baulk buckets
  std::wstring appLinks;    // baulk app links location
  bool appLocationFlat{false};
  // LocalState
  bool InitializeFromPackaged(bela::error_code &ec);
  bool InitializeFromPortable(bela::error_code &ec);
  bool InitializeFromLegacy(bela::error_code &ec);
  bool InitializeFromSystemAppData(bela::error_code &ec);
  bool InitializeFromLocalAppData(bela::error_code &ec);

private:
  bool InitializeFromNewest(bela::error_code &ec);
};
const FsRedirectionTable &AppPathFsTable();
} // namespace vfs_internal

// InitializePathFs: baulk initialize vfs PathFs
bool InitializePathFs(bela::error_code &ec);
// InitializeFastPathFs: call by baulk-exec and other tools
bool InitializeFastPathFs(bela::error_code &ec);
// Executable is packaged
bool IsPackaged();
// Baulk PathFsModel
// - Portable
// - User
// - System
// - Packaged
// - Legacy
std::wstring_view AppMode();
constexpr std::wstring_view PackagedMode = L"Packaged";
constexpr std::wstring_view LegacyMode = L"Legacy";
constexpr std::wstring_view UserMode = L"User";
constexpr std::wstring_view SystemMode = L"System";
constexpr std::wstring_view PortableMode = L"Portable";
// Baulk install path: Only used to find the command path of baulk
// eg:

std::wstring_view AppLocation();
// auto baulkLnkExe=AppLocationPath(L"\\baulk-lnk.exe");
using namespace std::string_view_literals;
std::wstring AppLocationPath(std::wstring_view name = L""sv);
//
// Baulk vfs root directory
std::wstring_view AppBasePath(); // root
std::wstring_view AppData();
std::wstring_view AppEtc();
// Baulk vfs temp dir
std::wstring_view AppTemp();
std::wstring_view AppPackages();
std::wstring_view AppLocks();
std::wstring_view AppBuckets();
// Baulk app link or launcher location
std::wstring_view AppLinks();
// AppPackageFolder: concat package full path
std::wstring AppPackageFolder(std::wstring_view packageName);
// AppPackageVFS
std::wstring AppPackageVFS(std::wstring_view packageName);
// AppFsMutexPath: return FsMutex file path
std::wstring AppFsMutexPath();
std::wstring AppDefaultProfile();

} // namespace baulk::vfs

#endif