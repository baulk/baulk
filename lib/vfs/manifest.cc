//
#include <bela/io.hpp>
#include <baulk/json_utils.hpp>
#include "vfsinternal.hpp"

namespace baulk::vfs {
bool PathFs::InitializeBaulkEnv(std::wstring_view envfile, bela::error_code &ec) {
  using namespace std::string_view_literals;
  auto j = baulk::parse_json_file(envfile, ec);
  if (!j) {
    return false;
  }
  auto jv = j->view();
  mode = jv.get("mode"sv, L"Legacy"sv);
  return true;
}
} // namespace baulk::vfs
