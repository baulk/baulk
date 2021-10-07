//
#include "vfsinternal.hpp"

namespace baulk::vfs {
bool InitializePathFs(bela::error_code &ec) {
  //
  return PathFs::Instance().Initialize(ec);
}

std::wstring_view PathFsModel() { return PathFs::Instance().Model(); }

} // namespace baulk::vfs