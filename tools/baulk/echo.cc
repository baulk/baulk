///
#include "echo.hpp"

namespace baulk::echo {
constexpr std::string_view commands[] = {"version", "info",   "install", "uninstall", "list",
                                         "search",  "update", "upgrade", "clean"};
int Processor::Execute() {
  //   for (;;) {
  //   }
  return 0;
}
} // namespace baulk::echo