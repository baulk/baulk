#include "baulk.hpp"
#include "commands.hpp"
#include <version.hpp>

namespace baulk::commands {

std::wstring VersionAlignment(std::wstring_view ver, size_t maxlen = 10) {
  if (ver.size() > maxlen) {
    baulk::version::version v(ver);
    return v.to_wstring();
  }
  return std::wstring(ver);
}

int cmd_search_all() {
  //
  return 0;
}

int cmd_search(const argv_t &argv) {
  if (!baulk::InitializeBaulkBuckets()) {
    return 1;
  }
  //
  if (argv.empty()) {
    return cmd_search_all();
  }
  for (const auto pkg : argv) {
    //
  }
  return 0;
}
} // namespace baulk::commands