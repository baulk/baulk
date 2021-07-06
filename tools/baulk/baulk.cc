#include <bela/path.hpp>
#include <baulkrev.hpp>
#include "baulk.hpp"
#include "baulkargv.hpp"
#include "commands.hpp"

namespace baulk {
bool IsDebugMode = false;
bool IsForceMode = false;
bool IsForceDelete = false;
bool IsQuietMode = false;
bool IsTraceMode = false;
bool IsInsecureMode = false;
wchar_t UserAgent[UerAgentMaximumLength] = L"Wget/7.0 (Baulk)";
int cmd_uninitialized(const baulk::commands::argv_t &argv) {
  bela::FPrintF(stderr, L"baulk uninitialized command\n");
  return 1;
}
} // namespace baulk

struct baulkcommand_t {
  baulk::commands::argv_t argv;
  decltype(baulk::cmd_uninitialized) *cmd{baulk::cmd_uninitialized};
  int operator()() const { return this->cmd(this->argv); }
};

struct command_map_t {
  const std::wstring_view name;
  decltype(baulk::commands::cmd_install) *cmd;
};

void Version() {
  bela::FPrintF(stdout, L"baulk %s\nRelease:    %s\nCommit:     %s\nBuild Time: %s\n", BAULK_VERSION, BAULK_REFNAME,
                BAULK_REVISION, BAULK_BUILD_TIME);
}

int cmd_version(const baulk::commands::argv_t &) {
  Version();
  return 0;
}

bool ParseArgv(int argc, wchar_t **argv, baulkcommand_t &cmd) {
  baulk::cli::BaulkArgv ba(argc - 1, argv + 1);
  std::wstring_view profile;
  ba.Add(L"help", baulk::cli::no_argument, 'h')
      .Add(L"version", baulk::cli::no_argument, 'v')
      .Add(L"verbose", baulk::cli::no_argument, 'V')
      .Add(L"quiet", baulk::cli::no_argument, 'Q')
      .Add(L"force", baulk::cli::no_argument, L'F')
      .Add(L"profile", baulk::cli::required_argument, 'P')
      .Add(L"user-agent", baulk::cli::required_argument, 'A')
      .Add(L"insecure", baulk::cli::no_argument, 'k')
      .Add(L"https-proxy", baulk::cli::required_argument, 1001) // option
      .Add(L"force-delete", baulk::cli::no_argument, 1002)
      .Add(L"trace", baulk::cli::no_argument, 'T')
      .Add(L"bucket")
      .Add(L"unzip")
      .Add(L"untar");

  bela::error_code ec;
  auto result = ba.Execute(
      [&](int val, const wchar_t *oa, const wchar_t *) {
        switch (val) {
        case 'h':
          baulk::commands::usage_baulk();
          exit(0);
        case 'v':
          Version();
          exit(0);
        case 'P':
          profile = oa;
          break;
        case 'V':
          baulk::IsDebugMode = true;
          break;
        case 'Q':
          baulk::IsQuietMode = true;
          break;
        case 'F':
          baulk::IsForceMode = true;
          break;
        case 'T':
          baulk::IsTraceMode = true;
          break;
        case 'k':
          baulk::IsInsecureMode = true;
          break;
        case 'A':
          if (auto len = wcslen(oa); len < 256) {
            wmemcpy_s(baulk::UserAgent, baulk::UerAgentMaximumLength, oa, len);
            baulk::UserAgent[len] = 0;
          }
          break;
        case 1001:
          SetEnvironmentVariableW(L"HTTPS_PROXY", oa);
          break;
        case 1002:
          baulk::IsForceDelete = true;
          break;
        default:
          return false;
        }
        return true;
      },
      ec);
  if (!result) {
    bela::FPrintF(stderr, L"baulk ParseArgv error: \x1b[31m%s\x1b[0m\n", ec.message);
    return false;
  }
  if (baulk::IsDebugMode && baulk::IsQuietMode) {
    baulk::IsQuietMode = false;
    bela::FPrintF(stderr, L"\x1b[33mbaulk warning: --verbose --quiet is set at the same time, "
                          L"quiet mode is turned off\x1b[0m\n");
  }
  if (ba.Argv().empty()) {
    bela::FPrintF(stderr, L"baulk no command input\n");
    return false;
  }
  // Initialize baulk env
  baulk::InitializeBaulkEnv(argc, argv, profile);
  auto subcmd = ba.Argv().front();
  cmd.argv.assign(ba.Argv().begin() + 1, ba.Argv().end());
  constexpr command_map_t cmdmaps[] = {
      {L"help", baulk::commands::cmd_help},
      {L"h", baulk::commands::cmd_help}, // help alias
      {L"version", cmd_version},
      {L"install", baulk::commands::cmd_install},       // install
      {L"i", baulk::commands::cmd_install},             // install
      {L"list", baulk::commands::cmd_list},             // list installed
      {L"l", baulk::commands::cmd_list},                // list installed alias
      {L"search", baulk::commands::cmd_search},         // search from bucket
      {L"s", baulk::commands::cmd_search},              // search alias
      {L"uninstall", baulk::commands::cmd_uninstall},   // uninstall
      {L"r", baulk::commands::cmd_uninstall},           // uninstall
      {L"update", baulk::commands::cmd_update},         // update bucket
      {L"upgrade", baulk::commands::cmd_upgrade},       // upgrade
      {L"u", baulk::commands::cmd_update_and_upgrade},  // update and upgrade
      {L"freeze", baulk::commands::cmd_freeze},         // freeze
      {L"unfreeze", baulk::commands::cmd_unfreeze},     // unfreeze
      {L"b3sum", baulk::commands::cmd_b3sum},           // b3sum
      {L"sha256sum", baulk::commands::cmd_sha256sum},   // sha256sum
      {L"cleancache", baulk::commands::cmd_cleancache}, // cleancache
      {L"bucket", baulk::commands::cmd_bucket},         // bucket command
      {L"untar", baulk::commands::cmd_untar},           // untar
      {L"unzip", baulk::commands::cmd_unzip},           // unzip
  };
  for (const auto &c : cmdmaps) {
    if (subcmd == c.name) {
      cmd.cmd = c.cmd;
      return true;
    }
  }
  bela::FPrintF(stderr, L"baulk unsupport command: \x1b[31m%s\x1b[0m\n", subcmd);
  return false;
}

int wmain(int argc, wchar_t **argv) {
  baulkcommand_t cmd;
  if (!ParseArgv(argc, argv, cmd)) {
    return 1;
  }
  return cmd();
}