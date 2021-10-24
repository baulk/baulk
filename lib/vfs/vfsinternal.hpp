///
#include <atomic>
#include <thread>
#include <baulk/vfs.hpp>

namespace baulk::vfs {

class PathFs {
public:
  PathFs(const PathFs &) = delete;
  PathFs &operator=(const PathFs &) = delete;
  static PathFs &Instance();
  bool Initialize(bela::error_code &ec);
  bool NewFsPaths(bela::error_code &ec);
  const auto &Table() { return table; }
  std::wstring_view Mode() const { return mode; }
  std::wstring Join(std::wstring_view child);
  std::wstring PackageVFS(std::wstring_view pkgName) const {
    if (mode == LegacyMode) {
      return table.vfs;
    }
    return bela::StringCat(table.vfs, L"\\", pkgName);
  }

private:
  PathFs() {}
  bool InitializeInternal(bela::error_code &ec);
  bool InitializeBaulkEnv(std::wstring_view envfile, bela::error_code &ec);
  std::once_flag initialized;
  std::wstring mode;
  vfs_internal::FsRedirectionTable table;
};

std::wstring_view GetAppBasePath();

} // namespace baulk::vfs