///
#include "commands.hpp"
#include "baulk.hpp"

namespace baulk::commands {
int cmd_upgrade(const argv_t &argv) {
  //
  bela::error_code ec;
  if (!baulk::BaulkInitializeExecutor(ec)) {
    baulk::DbgPrint(L"unable initialize compiler executor: %s\n", ec.message);
  }
  return true;
}
} // namespace baulk::commands