//
#ifndef BAULK_COMMANDS_HPP
#define BAULK_COMMANDS_HPP
#include <string_view>
#include <vector>

namespace baulk::commands {
using argv_t = std::vector<std::wstring_view>;
int cmd_help(const argv_t &argv);
//
int cmd_info(const argv_t &argv);
int cmd_install(const argv_t &argv);
int cmd_list(const argv_t &argv);
int cmd_search(const argv_t &argv);
int cmd_uninstall(const argv_t &argv);
int cmd_update(const argv_t &argv);
int cmd_upgrade(const argv_t &argv);
int cmd_update_and_upgrade(const argv_t &argv);
int cmd_freeze(const argv_t &argv);
int cmd_unfreeze(const argv_t &argv);
//
int cmd_b3sum(const argv_t &argv);
int cmd_sha256sum(const argv_t &argv);
//
int cmd_cleancache(const argv_t &argv);
//
int cmd_bucket(const argv_t &argv);
// archive
int cmd_unzip(const argv_t &argv);
int cmd_untar(const argv_t &argv);
int cmd_version(const argv_t &argv);
//
void Version();

// USAGE:
void usage_baulk();
void usage_info();
void usage_install();
void usage_list();
void usage_search();
void usage_uninstall();
void usage_update();
void usage_upgrade();
void usage_update_and_upgrade();
void usage_freeze();
void usage_unfreeze();
void usage_sha256sum();
void usage_b3sum();
void usage_cleancache();
void usage_bucket();
void usage_untar();
void usage_unzip();
} // namespace baulk::commands

#endif