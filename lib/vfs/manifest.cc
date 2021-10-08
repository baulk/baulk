//
#include <bela/io.hpp>
#include <toml.hpp>
#include "vfsinternal.hpp"

namespace baulk::vfs {
bool PathFs::DetectPathFsModel(std::wstring_view envfile, bela::error_code &ec) {
  std::wstring text;
  constexpr int64_t envFileSize = 1024 * 1024 * 4;
  if (!bela::io::ReadFile(envfile, text, ec, envFileSize)) {
    //
    return false;
  }

  return true;
}
} // namespace baulk::vfs
