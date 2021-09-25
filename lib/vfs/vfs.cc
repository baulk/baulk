//
#include "vfsinternal.hpp"
#include <bela/path.hpp>
#include <bela/env.hpp>
#include <appmodel.h>

namespace baulk::vfs {


PathFs &PathFs::Instance() {
  static PathFs inst;
  return inst;
}
bool PathFs::Initialize(bela::error_code &ec) {
  if (initialized) {
    return true;
  }
  // TODO init
  initialized = true;
  return true;
}


} // namespace baulk::vfs