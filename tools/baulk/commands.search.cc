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