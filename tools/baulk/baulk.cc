#include <bela/path.hpp>
#include <baulk/argv.hpp>
#include <baulk/net.hpp>
#include "baulk.hpp"
#include "commands.hpp"

namespace baulk {
bool IsDebugMode = false;
bool IsForceMode = false;
bool IsForceDelete = false;
bool IsQuietMode = false;
bool IsTraceMode = false;

int cmd_uninitialized(const baulk::commands::argv_t & /*unused*/) {
  bela::FPrintF(stderr, L"baulk uninitialized command\n");
  return 1;
}
struct command_t {
  commands::argv_t argv;
  decltype(cmd_uninitialized) *cmd_entry{baulk::cmd_uninitialized};
  int operator()() const { return this->cmd_entry(this->argv); }
};

struct command_map_t {
  const std::wstring_view name;
  decltype(baulk::cmd_uninitialized) *cmd_entry;
  bool require_context{false};
};

std::optional<command_t> ParseArgv(int argc, wchar_t **argv) {
  cli::ParseArgv pa(argc - 1, argv + 1);
  std::wstring_view profile;
  pa.Add(L"help", cli::no_argument, 'h')
      .Add(L"version", cli::no_argument, 'v')
      .Add(L"verbose", cli::no_argument, 'V')
      .Add(L"quiet", cli::no_argument, 'Q')
      .Add(L"force", cli::no_argument, L'F')
      .Add(L"profile", cli::required_argument, 'P')
      .Add(L"user-agent", cli::required_argument, 'A')
      .Add(L"insecure", cli::no_argument, 'k')
      .Add(L"https-proxy", cli::required_argument, 1001) // option
      .Add(L"force-delete", cli::no_argument, 1002)
      .Add(L"trace", cli::no_argument, 'T')
      .Add(L"bucket")
      .Add(L"unzip")
      .Add(L"untar");

  bela::error_code ec;
  auto result = pa.Execute(
      [&](int val, const wchar_t *oa, const wchar_t *) {
        switch (val) {
        case 'h':
          baulk::InitializeContext(profile, ec);
          commands::usage_baulk();
          exit(0);
        case 'v':
          baulk::InitializeContext(profile, ec);
          commands::Version();
          exit(0);
        case 'P':
          profile = oa;
          break;
        case 'V':
          IsDebugMode = true;
          break;
        case 'Q':
          IsQuietMode = true;
          break;
        case 'F':
          IsForceMode = true;
          break;
        case 'T':
          IsTraceMode = true;
          break;
        case 'k':
          net::HttpClient::DefaultClient().InsecureMode() = true;
          break;
        case 'A':
          net::HttpClient::DefaultClient().UserAgent() = oa;
          break;
        case 1001:
          net::HttpClient::DefaultClient().ProxyURL() = oa;
          break;
        case 1002:
          IsForceDelete = true;
          break;
        default:
          return false;
        }
        return true;
      },
      ec);
  if (!result) {
    bela::FPrintF(stderr, L"baulk parse argv error: \x1b[31m%s\x1b[0m\n", ec);
    return std::nullopt;
  }
  if (IsDebugMode && IsQuietMode) {
    IsQuietMode = false;
    bela::FPrintF(stderr, L"\x1b[33mbaulk warning: --verbose --quiet is set at the same time, "
                          L"quiet mode is turned off\x1b[0m\n");
  }
  if (pa.Argv().empty()) {
    bela::FPrintF(stderr, L"baulk no command input\n");
    return std::nullopt;
  }

  constexpr command_map_t cmdmaps[] = {
      {L"help", baulk::commands::cmd_help, false},
      {L"h", baulk::commands::cmd_help, false}, // help alias
      {L"version", baulk::commands::cmd_version, true},
      {L"info", baulk::commands::cmd_info, true},             // info
      {L"install", baulk::commands::cmd_install, true},       // install
      {L"i", baulk::commands::cmd_install, true},             // install
      {L"list", baulk::commands::cmd_list, true},             // list installed
      {L"l", baulk::commands::cmd_list, true},                // list installed alias
      {L"search", baulk::commands::cmd_search, true},         // search from bucket
      {L"s", baulk::commands::cmd_search, true},              // search alias
      {L"uninstall", baulk::commands::cmd_uninstall, true},   // uninstall
      {L"r", baulk::commands::cmd_uninstall, true},           // uninstall
      {L"update", baulk::commands::cmd_update, true},         // update bucket
      {L"upgrade", baulk::commands::cmd_upgrade, true},       // upgrade
      {L"u", baulk::commands::cmd_update_and_upgrade, true},  // update and upgrade
      {L"freeze", baulk::commands::cmd_freeze, true},         // freeze
      {L"unfreeze", baulk::commands::cmd_unfreeze, true},     // unfreeze
      {L"cleancache", baulk::commands::cmd_cleancache, true}, // cleancache
      {L"bucket", baulk::commands::cmd_bucket, true},         // bucket command
      {L"b3sum", baulk::commands::cmd_b3sum, false},          // b3sum
      {L"sha256sum", baulk::commands::cmd_sha256sum, false},  // sha256sum
      {L"untar", baulk::commands::cmd_untar, false},          // untar
      {L"unzip", baulk::commands::cmd_unzip, false},          // unzip
      {L"extract", baulk::commands::cmd_extract, false},      // extract
      {L"e", baulk::commands::cmd_extract, false},            // extract command
      {L"brand", baulk::commands::cmd_brand, false},          // brand
  };
  auto subcmd = pa.Argv().front();
  for (const auto &c : cmdmaps) {
    if (subcmd == c.name) {
      if (c.require_context) {
        if (!baulk::InitializeContext(profile, ec)) {
          bela::FPrintF(stderr, L"baulk initialize context error: \x1b[31m%s\x1b[0m\n", ec);
          return std::nullopt;
        }
      }
      return std::make_optional<command_t>(command_t{
          .argv = commands::argv_t(pa.Argv().begin() + 1, pa.Argv().end()),
          .cmd_entry = c.cmd_entry,
      });
    }
  }
  bela::FPrintF(stderr, L"baulk unsupport command: \x1b[31m%s\x1b[0m\n", subcmd);
  return std::nullopt;
}

} // namespace baulk

int wmain(int argc, wchar_t **argv) {
  if (auto cmd = baulk::ParseArgv(argc, argv); cmd) {
    return (*cmd)();
  }
  return 1;
}