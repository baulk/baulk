//
#include "vfsinternal.hpp"

namespace baulk::vfs {
bool InitializePathFs(bela::error_code &ec) {
  //
  return PathFs::Instance().Initialize(ec);
}

} // namespace baulk::vfs