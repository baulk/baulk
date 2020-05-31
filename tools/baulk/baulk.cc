#include <bela/subsitute.hpp>
#include <bela/path.hpp>
#include <baulkrev.hpp>
#include "baulk.hpp"
#include "baulkargv.hpp"
#include "commands.hpp"
#include "version.h"

namespace baulk {
bool IsDebugMode = false;
bool IsForceMode = false;
wchar_t UserAgent[UerAgentMaximumLength] = L"Wget/5.0 (Baulk)";
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

void Usage() {
  constexpr std::wstring_view usage =
      LR"(baulk - Minimal Package Manager for Windows
Usage: baulk [option] command pkg ...
  -h|--help        Show usage text and quit
  -v|--version     Show version number and quit
  -V|--verbose     Make the operation more talkative
  -F|--force       Turn on force mode. such as force update frozen package
  -P|--profile     Set profile path. default: $0\config\baulk.json
  -A|--user-agent  Send User-Agent <name> to server
  --https-proxy    Use this proxy. Equivalent to setting the environment variable 'HTTPS_PROXY'


Command:
  exec             Execute a command
  list             List all installed packages
  search           Search for available packages, or specific package details
  install          Install specific packages. upgrade if already installed.
  uninstall        Uninstall specific packages
  update           Update ports metadata
  upgrade          Upgrade all upgradeable packages
  freeze           Freeze specific package
  unfreeze         UnFreeze specific package
  b3sum            Calculate the BLAKE3 checksum of a file
  sha256sum        Calculate the SHA256 checksum of a file

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
void Version() {
  // baulk
  bela::FPrintF(stderr,
                L"baulk %s\nRelease:    %s\nCommit:     %s\nBuild Time: %s\n",
                BAULK_VERSION_FULL, BAULK_RELEASE_NAME, BAULK_RELEASE_COMMIT,
                BAULK_BUILD_TIME);
}

bool ParseArgv(int argc, wchar_t **argv, baulkcommand_t &cmd) {
  baulk::cli::BaulkArgv ba(argc, argv);
  std::wstring_view profile;
  ba.Add(L"help", baulk::cli::no_argument, 'h')
      .Add(L"version", baulk::cli::no_argument, 'v')
      .Add(L"verbose", baulk::cli::no_argument, 'V')
      .Add(L"force", baulk::cli::no_argument, L'F')
      .Add(L"profile", baulk::cli::required_argument, 'P')
      .Add(L"user-agent", baulk::cli::required_argument, 'A')
      .Add(L"https-proxy", baulk::cli::required_argument, 1001) // option
      .Add(L"upgrade")
      .Add(L"exec"); // subcommand
  bela::error_code ec;
  auto result = ba.Execute(
      [&](int val, const wchar_t *oa, const wchar_t *) {
        switch (val) {
        case 'h':
          Usage();
          exit(0);
        case 'v':
          Version();
          exit(0);
        case 'c':
          profile = oa;
          break;
        case 'V':
          baulk::IsDebugMode = true;
          break;
        case 'F':
          baulk::IsForceMode = true;
          break;
        case 'A':
          if (auto len = wcslen(oa); len < 256) {
            wmemcmp(baulk::UserAgent, oa, len);
            baulk::UserAgent[len] = 0;
          }
          break;
        case 1001:
          SetEnvironmentVariableW(L"HTTPS_PROXY", oa);
          break;
        default:
          return false;
        }
        return true;
      },
      ec);
  if (!result) {
    bela::FPrintF(stderr, L"baulk ParseArgv error: %s\n", ec.message);
    return false;
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
      {L"exec", baulk::commands::cmd_exec},
      {L"install", baulk::commands::cmd_install},
      {L"list", baulk::commands::cmd_list},
      {L"search", baulk::commands::cmd_search},
      {L"uninstall", baulk::commands::cmd_uninstall},
      {L"update", baulk::commands::cmd_update},
      {L"upgrade", baulk::commands::cmd_upgrade},
      {L"freeze", baulk::commands::cmd_freeze},
      {L"unfreeze", baulk::commands::cmd_unfreeze},
      {L"b3sum", baulk::commands::cmd_b3sum},
      {L"sha256sum", baulk::commands::cmd_sha256sum}};
  for (const auto &c : cmdmaps) {
    if (subcmd == c.name) {
      cmd.cmd = c.cmd;
      return true;
    }
  }
  bela::FPrintF(stderr, L"baulk unsupport command: %s\n", subcmd);
  return false;
}

int wmain(int argc, wchar_t **argv) {
  baulkcommand_t cmd;
  if (!ParseArgv(argc, argv, cmd)) {
    return 1;
  }
  return cmd();
}