///
#include <version.hpp>
#include <bela/match.hpp>
#include <bela/strip.hpp>
#include <bela/io.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/process.hpp>
#include <bela/parseargv.hpp>
#include <bela/str_split_narrow.hpp>
#include <baulk/net.hpp>
#include <baulk/json_utils.hpp>
#include <baulk/fs.hpp>
#include <baulk/vfs.hpp>
#include <baulk/archive/extract.hpp>
#include "executor.hpp"

// https://www.catch22.net/tuts/win32/self-deleting-executables
// https://stackoverflow.com/questions/10319526/understanding-a-self-deleting-program-in-c
namespace baulk {
bool IsDebugMode = false;
void Usage() {
  constexpr std::wstring_view usage = LR"(baulk-update - Baulk self update utility
Usage: baulk-update [option]
  -h|--help        Show usage text and quit
  -v|--version     Show version number and quit
  -V|--verbose     Make the operation more talkative
  -F|--force       Turn on force mode. such as force update frozen package
  -A|--user-agent  Send User-Agent <name> to server
  -k|--insecure    Allow insecure server connections when using SSL
  -S|--specified   Download and install the specified version of baulk
  --https-proxy    Use this proxy. Equivalent to setting the environment variable 'HTTPS_PROXY'

Example:
  baulk-update                  # Update to the latest version released
  baulk-update -F               # Force update to latest version released 
  baulk-update "-S4.0.0-beta.2" # Install the specified version

)";
  bela::terminal::WriteAuto(stderr, usage);
}

void Version() {
  bela::FPrintF(stdout, L"baulk-update %s\nRelease:    %s\nCommit:     %s\nBuild Time: %s\n", BAULK_VERSION,
                BAULK_REFNAME, BAULK_REVISION, BAULK_BUILD_TIME);
}

bool Executor::ParseArgv(int argc, wchar_t **argv) {
  bela::ParseArgv pa(argc, argv);
  pa.Add(L"help", bela::no_argument, 'h')
      .Add(L"version", bela::no_argument, 'v')
      .Add(L"verbose", bela::no_argument, 'V')
      .Add(L"force", bela::no_argument, L'F')
      .Add(L"insecure", bela::no_argument, L'k')
      .Add(L"profile", bela::required_argument, 'P')
      .Add(L"user-agent", bela::required_argument, 'A')
      .Add(L"specified", bela::required_argument, 'S')
      .Add(L"https-proxy", bela::required_argument, 1002);

  bela::error_code ec;
  auto result = pa.Execute(
      [&](int val, const wchar_t *oa, const wchar_t *) {
        using baulk::net::HttpClient;
        switch (val) {
        case 'h':
          Usage();
          exit(0);
        case 'v':
          Version();
          exit(0);
        case 'V':
          IsDebugMode = true;
          HttpClient::DefaultClient().SetDebugMode(true);
          break;
        case 'F':
          forceMode = true;
          break;
        case 'k':
          HttpClient::DefaultClient().SetInsecureMode(true);
          break;
        case 'A':
          if (wcslen(oa) != 0) {
            HttpClient::DefaultClient().SetUserAgent(oa);
          }
          break;
        case 'S':
          specified = oa;
          if (!specified.empty()) {
            forceMode = true;
          }
          break;
        case 1002:
          HttpClient::DefaultClient().SetProxyURL(oa);
          break;
        default:
          return false;
        }
        return true;
      },
      ec);
  if (!result) {
    bela::FPrintF(stderr, L"baulk-update ParseArgv error: %s\n", ec);
    return false;
  }
  return true;
}
} // namespace baulk

int wmain(int argc, wchar_t **argv) {
  bela::error_code ec;
  if (!baulk::vfs::InitializeFastPathFs(ec)) {
    bela::FPrintF(stderr, L"baulk-exec InitializeFastPathFs error %s\n", ec);
    return 1;
  }
  baulk::Executor executor;

  if (!executor.ParseArgv(argc, argv)) {
    return 1;
  }
  return executor.Execute() ? 0 : 1;
}
