// baulk vfs
#ifndef BAULK_VFS_HPP
#define BAULK_VFS_HPP
#include <bela/base.hpp>
#include <bela/phmap.hpp>

namespace baulk::vfs {
// InitializePathFs: baulk initialize vfs PathFs
bool InitializePathFs(bela::error_code &ec);
std::wstring_view AppRoot();
std::wstring_view AppData();
std::wstring_view AppVFS();
} // namespace baulk::vfs

#endif