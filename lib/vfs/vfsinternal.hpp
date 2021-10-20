///
#include <atomic>
#include <thread>
#include <baulk/vfs.hpp>

namespace baulk::vfs {
namespace vfs_internal {
struct FsRedirectionTable {
public:
  std::wstring executableRoot; // baulk.exe root
  std::wstring basePath;       // baulk root
  std::wstring appData;        // baulk appdata
  std::wstring etc;            // baulk etc
  std::wstring userVFS;        // baulk pakcage vfs
  std::wstring packageRoot;    // baulk package root
  std::wstring temp;           // baulk temp
  std::wstring locks;          // baulk locks
  std::wstring buckets;        // baulk buckets
  std::wstring appLinks;       // baulk app links location
  // LocalState
  bool InitializeFromPackaged(bela::error_code &ec);
  bool InitializeFromPortable(bela::error_code &ec);
  bool InitializeFromLegacy(bela::error_code &ec);
  bool InitializeFromSystemAppData(bela::error_code &ec);
  bool InitializeFromLocalAppData(bela::error_code &ec);

private:
  bool InitializeFromNewest(bela::error_code &ec);
};
} // namespace vfs_internal

class PathFs {
public:
  PathFs(const PathFs &) = delete;
  PathFs &operator=(const PathFs &) = delete;
  static PathFs &Instance();
  bool Initialize(bela::error_code &ec);
  bool NewFsPaths(bela::error_code &ec);
  const auto &Table() { return table; }
  std::wstring_view Model() const { return fsmodel; }
  std::wstring Join(std::wstring_view child);

private:
  PathFs() {}
  bool InitializeInternal(bela::error_code &ec);
  bool InitializeBaulkEnv(std::wstring_view envfile, bela::error_code &ec);
  std::once_flag initialized;
  std::wstring fsmodel;
  vfs_internal::FsRedirectionTable table;
};

std::wstring_view GetAppBasePath();

} // namespace baulk::vfs