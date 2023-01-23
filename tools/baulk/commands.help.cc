#include <bela/subsitute.hpp>
#include <bela/terminal.hpp>
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <baulk/fs.hpp>
#include "commands.hpp"
#include "baulk.hpp"

namespace baulk::commands {
void usage_baulk() {
  constexpr std::wstring_view usage = LR"(baulk - Minimal Package Manager for Windows
Usage: baulk [option] <command> [<args>]
  -h|--help        Show usage text and quit
  -v|--version     Show version number and quit
  -V|--verbose     Make the operation more talkative
  -Q|--quiet       Make the operation more quiet
  -F|--force       Turn on force mode. such as force update frozen package
  -P|--profile     Set profile path. default: $0\config\baulk.json
  -A|--user-agent  Send User-Agent <name> to server
  -k|--insecure    Allow insecure server connections when using SSL
  -T|--trace       Turn on trace mode. track baulk execution details.
  --https-proxy    Use this proxy. Equivalent to setting the environment variable 'HTTPS_PROXY'
  --force-delete   When uninstalling the package, forcefully delete the related directories

Command:
  version          Show version number and quit
  list             List installed packages based on package names
  search           Search in package descriptions
  info             Show package information
  install          Install specific packages. upgrade if already installed. (alias: i)
  uninstall        Uninstall specific packages. (alias: r)
  update           Update bucket metadata
  upgrade          Upgrade all upgradeable packages
  freeze           Freeze specific package
  unfreeze         UnFreeze specific package
  b3sum            Calculate the BLAKE3 checksum of a file
  sha256sum        Calculate the SHA256 checksum of a file
  cleancache       Cleanup download cache
  bucket           Add, delete or list buckets
  untar            Extract files in a tar archive. support: tar.xz tar.bz2 tar.gz tar.zstd
  unzip            Extract compressed files in a ZIP archive
  extract           Extract files according to the detected archive format
  brand            Display os device details

Alias:
  h  help
  i  install
  r  uninstall
  l  list
  s  search
  u  update and upgrade
  e  extract

See 'baulk help <command>' to read usage a specific subcommand.

)";
  bela::error_code ec;
  auto exeparent = bela::ExecutableParent(ec);
  if (!exeparent) {
    auto msg = bela::Substitute(usage, L"$Prefix");
    bela::terminal::WriteAuto(stderr, msg);
    return;
  }
  bela::PathStripName(*exeparent);
  auto msg = bela::Substitute(usage, *exeparent);
  bela::terminal::WriteAuto(stderr, msg);
}

struct command_usage_map_t {
  const std::wstring_view name;
  decltype(baulk::commands::usage_baulk) *usage;
};

int cmd_help(const baulk::commands::argv_t &argv) {
  if (argv.empty()) {
    usage_baulk();
    return 0;
  }
  constexpr command_usage_map_t usages[] = {
      {L"install", baulk::commands::usage_install},       // install
      {L"i", baulk::commands::usage_install},             // install
      {L"list", baulk::commands::usage_list},             // list installed
      {L"search", baulk::commands::usage_search},         // search from bucket
      {L"uninstall", baulk::commands::usage_uninstall},   // uninstall
      {L"r", baulk::commands::usage_uninstall},           // uninstall
      {L"update", baulk::commands::usage_update},         // update bucket
      {L"upgrade", baulk::commands::usage_upgrade},       // upgrade
      {L"u", baulk::commands::usage_update_and_upgrade},  // update and upgrade
      {L"freeze", baulk::commands::usage_freeze},         // freeze
      {L"unfreeze", baulk::commands::usage_unfreeze},     // unfreeze
      {L"b3sum", baulk::commands::usage_b3sum},           // b3sum
      {L"sha256sum", baulk::commands::usage_sha256sum},   // sha256sum
      {L"cleancache", baulk::commands::usage_cleancache}, // cleancache
      {L"bucket", baulk::commands::usage_bucket},         // bucket command
      {L"untar", baulk::commands::usage_untar},           // untar
      {L"unzip", baulk::commands::usage_unzip},           // unzip
      {L"extract", baulk::commands::usage_extract},       // extract
      {L"e", baulk::commands::usage_extract},             // extract
      {L"brand", baulk::commands::usage_brand},           // brand
  };
  auto subcmd = argv[0];
  for (const auto &u : usages) {
    if (subcmd == u.name) {
      u.usage();
      return 0;
    }
  }
  bela::FPrintF(stderr, L"Undocumented usage of subcommand '\x1b[31m%s\x1b[0m'\n", subcmd);
  return 0;
}
} // namespace baulk::commands
