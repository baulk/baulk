// baulk vfs
#ifndef BAULK_VFS_HPP
#define BAULK_VFS_HPP
#include <bela/base.hpp>
#include <bela/phmap.hpp>

namespace baulk::vfs {
class FsProvider;
class PathMappingTable {
public:
  PathMappingTable() = default;
  std::wstring_view Root() const { return root; }

private:
  friend class FsProvider;
  // LocalState
  bool InitializeFromDesktopBridge(bela::error_code &ec);
  bool InitializeFromPortable(bela::error_code &ec);
  bool InitializeFromLegacy(bela::error_code &ec);
  bool InitializeFromLocalAppData(bela::error_code &ec);
  
  std::wstring root;    // baulk root
  std::wstring etc;     // baulk etc
  std::wstring vfs;     // baulk pakcage vfs
  std::wstring pkgroot; // baulk package root
  std::wstring temp;    // baulk temp
  std::wstring locks;   // baulk locks
  std::wstring buckets; // baulk buckets
};
// InitializeVFS: baulk initialize vfs context
bool InitializeVFS(bela::error_code &ec);

} // namespace baulk::vfs

#endif