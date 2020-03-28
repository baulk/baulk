#include <bela/fnmatch.hpp>
#include "baulk.hpp"
#include "commands.hpp"
#include <version.hpp>

namespace baulk::commands {
// lldb/kali-rolling 1:9.0-49.1 amd64
//   Next generation, high-performance debugger

// package

int cmd_search_all() {
  //
  return 0;
}

int cmd_search(const argv_t &argv) {
  if (argv.empty()) {
    return cmd_search_all();
  }
  // Package is matched
  auto PkgMatch = [&](std::wstring_view pkgname) {
    for (const auto a : argv) {
      if (bela::FnMatch(a, pkgname)) {
        return true;
      }
    }
    return false;
  };
  return 0;
}
} // namespace baulk::commands