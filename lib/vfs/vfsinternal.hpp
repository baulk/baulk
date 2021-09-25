///
#include <atomic>
#include <baulk/vfs.hpp>

namespace baulk::vfs {
namespace vfs_internal {
struct FsRedirectionTable {
  std::wstring root;         // baulk root
  std::wstring etc;          // baulk etc
  std::wstring vfs;          // baulk pakcage vfs
  std::wstring pakcage_root; // baulk package root
  std::wstring temp;         // baulk temp
  std::wstring locks;        // baulk locks
  std::wstring buckets;      // baulk buckets
  // LocalState
  bool InitializeFromDesktopBridge(bela::error_code &ec);
  bool InitializeFromPortable(bela::error_code &ec);
  bool InitializeFromLegacy(bela::error_code &ec);
  bool InitializeFromSystemAppData(bela::error_code &ec);
  bool InitializeFromLocalAppData(bela::error_code &ec);
};
} // namespace vfs_internal

class PathFs {
public:
  PathFs(const PathFs &) = delete;
  PathFs &operator=(const PathFs &) = delete;
  static PathFs &Instance();
  bool Initialize(bela::error_code &ec);
  const auto &Table() { return table; }
  std::wstring Join(std::wstring_view child);

private:
  PathFs() {}
  vfs_internal::FsRedirectionTable table;
  std::atomic_bool initialized{false};
};
} // namespace baulk::vfs