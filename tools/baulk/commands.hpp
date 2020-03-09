//
#ifndef BAULK_COMMANDS_HPP
#define BAULK_COMMANDS_HPP
#include <string_view>
#include <vector>

namespace baulk::commands {
using argv_t = std::vector<std::wstring_view>;
int cmd_install(const argv_t &argv);
int cmd_list(const argv_t &argv);
int cmd_search(const argv_t &argv);
int cmd_uninstall(const argv_t &argv);
int cmd_update(const argv_t &argv);
int cmd_upgrade(const argv_t &argv);
} // namespace baulk::commands

#endif