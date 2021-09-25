// baulk vfs
#ifndef BAULK_VFS_HPP
#define BAULK_VFS_HPP
#include <bela/base.hpp>
#include <bela/phmap.hpp>

namespace baulk::vfs {
// InitializePathFs: baulk initialize vfs PathFs
bool InitializePathFs(bela::error_code &ec);
std::wstring_view Root();
std::wstring_view VFS();
std::wstring_view Etc();
} // namespace baulk::vfs

#endif