// A simple program download network resource
#include <bela/parseargv.hpp>
#include <filesystem>
#include <baulk/hash.hpp>
#include <baulk/net/client.hpp>
#include <baulk/debug.hpp>
#include <version.hpp>

namespace baulk {
bool IsDebugMode = false;

void Usage() {
  constexpr std::wstring_view usage = LR"(wind - Streamlined file download tool
Usage: wind [option]... [url]...
  -h|--help        Show usage text and quit
  -v|--version     Show version number and quit
  -V|--verbose     Make the operation more talkative
  -W|--cwd         Save the file to the specified directory
  -R|--replace     Replace the file when it exists
  -K|--insecure    Allow insecure server connections when using SSL
  -O|--output      Write file to the specified path
  -A|--user-agent  Send User-Agent <name> to server
  --https-proxy    Use this proxy. Equivalent to setting the environment variable 'HTTPS_PROXY'
  --no-cache       Download directly without caching

Example:
  wind https://aka.ms/win32-x64-user-stable

)";
  bela::terminal::WriteAuto(stderr, usage);
}

void Version() {
  bela::FPrintF(stdout, L"wind %s\nRelease:    %s\nCommit:     %s\nBuild Time: %s\n", BAULK_VERSION, BAULK_REFNAME,
                BAULK_REVISION, BAULK_BUILD_TIME);
}

class Executor {
public:
  Executor() = default;
  Executor(const Executor &) = delete;
  Executor &operator=(const Executor &) = delete;
  bool ParseArgv(int argc, wchar_t **argv);
  int Execute() {
    if (urls.empty()) {
      bela::FPrintF(stderr, L"wind: \x1b[31mmissing URL\x1b[0m\n");
      Usage();
      return 1;
    }
    if (urls.size() == 1) {
      return single_download();
    }
    return multi_download();
  }

private:
  int single_download();
  int multi_download();
  std::vector<std::wstring> urls;
  std::filesystem::path cwd;
  std::filesystem::path destination;
  bool replace{false};
};

void verify_file(const std::filesystem::path &file) {
  bela::error_code ec;
  auto filename = file.filename();
  auto sums = baulk::hash::HashSums(file.native(), ec);
  if (!sums) {
    bela::FPrintF(stderr, L"unable check %s checksum: %v\n", filename.native(), ec);
    return;
  }
  bela::FPrintF(stderr, L"\x1b[34mSHA256:%s %s\x1b[0m\n", sums->sha256sum, filename.native());
  bela::FPrintF(stderr, L"\x1b[34mBLAKE3:%s %s\x1b[0m\n", sums->blake3sum, filename.native());
}

bool Executor::ParseArgv(int argc, wchar_t **argv) {
  using baulk::net::HttpClient;
  bela::ParseArgv pa(argc, argv);
  pa.Add(L"help", bela::no_argument, L'h')
      .Add(L"version", bela::no_argument, L'v')
      .Add(L"verbose", bela::no_argument, L'V')
      .Add(L"cwd", bela::required_argument, L'W')
      .Add(L"replace", bela::no_argument, L'R')
      .Add(L"insecure", bela::no_argument, L'K')
      .Add(L"output", bela::required_argument, L'O')
      .Add(L"user-agent", bela::required_argument, 'A')
      .Add(L"https-proxy", bela::required_argument, 1001)
      .Add(L"no-cache", bela::no_argument, 1002); // option
  bela::error_code ec;
  auto ret = pa.Execute(
      [&](int val, const wchar_t *oa, const wchar_t *) {
        switch (val) {
        case 'h':
          Usage();
          exit(0);
        case 'v':
          Version();
          exit(0);
        case 'V':
          baulk::IsDebugMode = true;
          HttpClient::DefaultClient().SetDebugMode(true);
          break;
        case 'f':
          replace = true;
          break;
        case 'k':
          HttpClient::DefaultClient().SetInsecureMode(true);
          break;
        case 'd':
          // ww.nocache = true;
          break;
        case 'w':
          cwd = oa;
          break;
        case 'o':
          destination = oa;
          break;
        case 'A':
          HttpClient::DefaultClient().SetUserAgent(oa);
          break;
        case 1001:
          HttpClient::DefaultClient().SetProxyURL(oa);
          break;
        case 1002:
          HttpClient::DefaultClient().SetNoCache(true);
          break;
        default:
          break;
        }
        return true;
      },
      ec);
  if (!ret) {
    bela::FPrintF(stderr, L"ParseArgv: %s\n", ec.message);
    return false;
  }
  for (const auto p : pa.UnresolvedArgs()) {
    urls.emplace_back(p);
  }
  if (cwd.empty()) {
    std::error_code e;
    if (cwd = std::filesystem::current_path(e); e) {
      ec = bela::make_error_code_from_std(e);
      bela::FPrintF(stderr, L"unable resolve current path: %s\n", ec.message);
      return false;
    }
  }
  HttpClient::DefaultClient().InitializeProxyFromEnv();
  return true;
}

int Executor::single_download() {
  auto u = urls[0];
  bela::error_code ec;
  auto file = baulk::net::WinGet(urls[0],
                                 {
                                     .hash_value = L"",
                                     .cwd = cwd,
                                     .destination = destination,
                                     .force_overwrite = replace,
                                 },
                                 ec);
  if (!file) {
    bela::FPrintF(stderr, L"download failed: \x1b[31m%v\x1b[0m\n", ec);
    return 1;
  }
  baulk::verify_file(*file);
  bela::FPrintF(stdout, L"\x1b[32m'%s' saved\x1b[0m\n", file->native());
  return 0;
}
int Executor::multi_download() {
  size_t success = 0;
  bela::error_code ec;
  for (const auto &u : urls) {
    auto file = baulk::net::WinGet(u,
                                   {
                                       .hash_value = L"",
                                       .cwd = cwd,
                                       .force_overwrite = replace,
                                   },
                                   ec);
    if (!file) {
      bela::FPrintF(stderr, L"download failed: \x1b[31m%s\x1b[0m\n", ec);
      continue;
    }
    baulk::verify_file(*file);
    bela::FPrintF(stdout, L"\x1b[32m'%s' saved\x1b[0m\n", *file);
  }
  return success == urls.size() ? 0 : 1;
}

} // namespace baulk

int wmain(int argc, wchar_t **argv) {
  baulk::Executor e;
  if (!e.ParseArgv(argc, argv)) {
    return 1;
  }
  return e.Execute();
}