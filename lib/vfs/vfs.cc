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
  bool result = true;
  std::call_once(initialized, [&, this] {
    //
    result = InitializeInternal(ec);
  });
  return result;
}

bool PathFs::InitializeInternal(bela::error_code &ec) {
  //
  return false;
}

} // namespace baulk::vfs