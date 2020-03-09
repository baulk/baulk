#include "commands.hpp"

namespace baulk::commands {

struct Searcher {
  std::wstring bucketroot;
  bool SearchAll();
  bool SearchOne(std::wstring_view pkg);
};
int cmd_search_all() {
  //
  return 0;
}

int cmd_search(const argv_t &argv) {
  if (argv.empty()) {
    return cmd_search_all();
  }
  for (const auto pkg : argv) {
    //
  }
  return 0;
}
} // namespace baulk::commands