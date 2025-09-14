#include <bela/path.hpp>
#include <baulk/argv.hpp>
#include <baulk/net.hpp>
#include <objbase.h>
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
      .Add(L"github-proxy", cli::required_argument, 1003)
      .Add(L"trace", cli::no_argument, 'T')
      .Add(L"bucket");

  bela::error_code ec;
  auto result = pa.Execute(
      [&](int val, const wchar_t *oa, const wchar_t *) {
        using baulk::net::HttpClient;
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
          net::HttpClient::DefaultClient().SetDebugMode(true);
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
          HttpClient::DefaultClient().SetInsecureMode(true);
          break;
        case 'A':
          HttpClient::DefaultClient().SetUserAgent(oa);
          break;
        case 1001:
          HttpClient::DefaultClient().SetProxyURL(oa);
          break;
        case 1002:
          IsForceDelete = true;
        case 1003:
          HttpClient::DefaultClient().SetGhProxy(oa);
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
      {.name = L"help", .cmd_entry = baulk::commands::cmd_help, .require_context = false},
      {.name = L"h", .cmd_entry = baulk::commands::cmd_help, .require_context = false}, // help alias
      {.name = L"version", .cmd_entry = baulk::commands::cmd_version, .require_context = true},
      {.name = L"info", .cmd_entry = baulk::commands::cmd_info, .require_context = true},       // info
      {.name = L"install", .cmd_entry = baulk::commands::cmd_install, .require_context = true}, // install
      {.name = L"i", .cmd_entry = baulk::commands::cmd_install, .require_context = true},       // install
      {.name = L"list", .cmd_entry = baulk::commands::cmd_list, .require_context = true},       // list installed
      {.name = L"l", .cmd_entry = baulk::commands::cmd_list, .require_context = true},          // list installed alias
      {.name = L"search", .cmd_entry = baulk::commands::cmd_search, .require_context = true},   // search from bucket
      {.name = L"s", .cmd_entry = baulk::commands::cmd_search, .require_context = true},        // search alias
      {.name = L"uninstall", .cmd_entry = baulk::commands::cmd_uninstall, .require_context = true}, // uninstall
      {.name = L"r", .cmd_entry = baulk::commands::cmd_uninstall, .require_context = true},         // uninstall
      {.name = L"update", .cmd_entry = baulk::commands::cmd_update, .require_context = true},       // update bucket
      {.name = L"upgrade", .cmd_entry = baulk::commands::cmd_upgrade, .require_context = true},     // upgrade
      {.name = L"u",
       .cmd_entry = baulk::commands::cmd_update_and_upgrade,
       .require_context = true},                                                                  // update and upgrade
      {.name = L"freeze", .cmd_entry = baulk::commands::cmd_freeze, .require_context = true},     // freeze
      {.name = L"unfreeze", .cmd_entry = baulk::commands::cmd_unfreeze, .require_context = true}, // unfreeze
      {.name = L"cleancache", .cmd_entry = baulk::commands::cmd_cleancache, .require_context = true}, // cleancache
      {.name = L"bucket", .cmd_entry = baulk::commands::cmd_bucket, .require_context = true},         // bucket command
      {.name = L"b3sum", .cmd_entry = baulk::commands::cmd_b3sum, .require_context = false},          // b3sum
      {.name = L"sha256sum", .cmd_entry = baulk::commands::cmd_sha256sum, .require_context = false},  // sha256sum
      {.name = L"untar", .cmd_entry = baulk::commands::cmd_untar, .require_context = false},          // untar
      {.name = L"unzip", .cmd_entry = baulk::commands::cmd_unzip, .require_context = false},          // unzip
      {.name = L"extract", .cmd_entry = baulk::commands::cmd_extract, .require_context = false},      // extract
      {.name = L"e", .cmd_entry = baulk::commands::cmd_extract, .require_context = false},            // extract command
      {.name = L"brand", .cmd_entry = baulk::commands::cmd_brand, .require_context = false},          // brand
  };
  auto subcmd = pa.Argv().front();
  for (const auto &c : cmdmaps) {
    if (subcmd == c.name) {
      if (c.require_context) {
        if (!baulk::InitializeContext(profile, ec)) {
          bela::FPrintF(stderr, L"baulk initialize context error: \x1b[31m%s\x1b[0m\n", ec);
          return std::nullopt;
        }
        net::HttpClient::DefaultClient().InitializeProxyFromEnv();
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

class dotcom_global_initializer {
public:
  dotcom_global_initializer() {
    auto hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
      auto ec = bela::make_system_error_code();
      MessageBoxW(nullptr, ec.data(), L"CoInitialize", IDOK);
      exit(1);
    }
  }
  ~dotcom_global_initializer() { CoUninitialize(); }
};

int wmain(int argc, wchar_t **argv) {
  dotcom_global_initializer di;
  if (auto cmd = baulk::ParseArgv(argc, argv); cmd) {
    return (*cmd)();
  }
  return 1;
}