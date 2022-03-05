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
#include "baulk-update.hpp"

// https://www.catch22.net/tuts/win32/self-deleting-executables
// https://stackoverflow.com/questions/10319526/understanding-a-self-deleting-program-in-c

namespace baulk::update {
bool IsDebugMode = false;
bool IsForceMode = false;
bool IsPreview = false;

constexpr std::wstring_view archfilesuffix() {
#if defined(_M_X64)
  return L"win-x64.zip";
#elif defined(_M_X86)
  return L"win-x86.zip";
#elif defined(_M_ARM64)
  return L"win-arm64.zip";
#elif defined(_M_ARM)
  return L"win-arm.zip";
#else
  return L"unknown cpu architecture";
#endif
}

bool detect_baulk_revision(std::wstring &releasename) {
  bela::process::Process ps;
  auto baulkexe = vfs::AppLocationPath(L"baulk-exec.exe");
  if (ps.Capture(baulkexe, L"--version") != 0) {
    if (ps.ErrorCode()) {
      bela::FPrintF(stderr, L"run %s error %s\n", baulkexe, ps.ErrorCode());
    }
    releasename.assign(BAULK_REFNAME);
    return false;
  }

  constexpr std::string_view releaseprefix = "Release:";
  std::vector<std::string_view> lines =
      bela::narrow::StrSplit(ps.Out(), bela::narrow::ByChar('\n'), bela::narrow::SkipEmpty());
  for (auto line : lines) {
    if (line = bela::StripAsciiWhitespace(line); !bela::StartsWith(line, releaseprefix)) {
      continue;
    }
    releasename = bela::encode_into<char, wchar_t>(bela::StripAsciiWhitespace(line.substr(releaseprefix.size())));
    return true;
  }
  releasename.assign(BAULK_REFNAME);
  return false;
}

bool ReleaseIsUpgradableFallback(std::wstring &url, std::wstring &version, std::wstring_view oldver) {
  // https://github.com/baulk/baulk/releases/latest
  constexpr std::wstring_view latest = L"https://github.com/baulk/baulk/releases/latest";
  constexpr std::string_view urlprefix = "/baulk/baulk/releases/download/";
  bela::error_code ec;
  auto resp = baulk::net::RestGet(latest, ec);
  if (!resp) {
    bela::FPrintF(stderr, L"baulk upgrade get latest: \x1b[31m%s\x1b[0m\n", ec);
    return false;
  }
  size_t index = 0;
  auto content = resp->Content();
  for (;;) {
    auto pos = content.find(urlprefix, index);
    if (pos == std::wstring::npos) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s is up to date\x1b[0m\n", oldver);
      return false;
    }
    auto pos2 = content.find('"', pos);
    if (pos2 == std::wstring::npos) {
      bela::FPrintF(stderr, L"baulk upgrade get %s: \x1b[31minvalid html url\x1b[0m\n", oldver);
      return false;
    }
    auto filename = bela::encode_into<char, wchar_t>({content.data() + pos, pos2 - pos});

    index = pos2 + 1;
    std::vector<std::wstring_view> svv = bela::StrSplit(filename, bela::ByChar('/'), bela::SkipEmpty());
    if (svv.size() <= 2) {
      continue;
    }
    auto newrev = svv[svv.size() - 2];
    if (newrev == oldver) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s is up to date\x1b[0m\n", oldver);
      return false;
    }
    version = newrev;
    if (svv.back().find(archfilesuffix()) != std::wstring_view::npos) {
      url = bela::StringCat(L"https://github.com", filename);
      DbgPrint(L"Found new release %s", filename);
      return true;
    }
  }
  bela::FPrintF(stderr, L"\x1b[33mbaulk/%s is up to date\x1b[0m\n", oldver);
  return false;
}

// https://api.github.com/repos/baulk/baulk/releases/latest todo
bool ReleaseIsUpgradable(std::wstring &url, std::wstring &version) {
  std::wstring releasename;
  if (!detect_baulk_revision(releasename)) {
    bela::FPrintF(stderr, L"unable detect baulk meta, use baulk-update release name '%s'\n", releasename);
  }
  constexpr std::wstring_view releaseprefix = L"refs/heads/";
  if (bela::StartsWith(releasename, releaseprefix)) {
    DbgPrint(L"baulk refname '%s' not public release", releasename);
    if (!IsForceMode) {
      return false;
    }
  }
  auto oldver = bela::StripPrefix(releasename, releaseprefix);
  DbgPrint(L"detect current version %s", oldver);
  bela::error_code ec;
  auto resp = baulk::net::RestGet(L"https://api.github.com/repos/baulk/baulk/releases/latest", ec);
  if (!resp) {
    bela::FPrintF(stderr, L"baulk upgrade self get metadata: \x1b[31m%s\x1b[0m\n", ec);
    return false;
  }
  try {
    /* code */
    auto obj = nlohmann::json::parse(resp->Content(), nullptr, true, true);
    auto tagname = bela::encode_into<char, wchar_t>(obj["tag_name"].get<std::string_view>());
    if (oldver == tagname) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s is up to date\x1b[0m", oldver);
      return false;
    }
    version = tagname;
    auto it = obj.find("assets");
    if (it == obj.end()) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s build is not yet complete\x1b[0m", tagname);
      return false;
    }
    for (const auto &p : it.value()) {
      auto name = bela::encode_into<char, wchar_t>(p["name"].get<std::string_view>());
      if (!bela::EndsWithIgnoreCase(name, archfilesuffix())) {
        continue;
      }
      url = bela::encode_into<char, wchar_t>(p["browser_download_url"].get<std::string_view>());
      return true;
    }
    //
  } catch (const std::exception &) {
    return ReleaseIsUpgradableFallback(url, version, oldver);
  }
  return false;
}

inline bool move_file(std::wstring_view src, std::wstring_view target) {
  return MoveFileExW(src.data(), target.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) == TRUE;
}

bool UpdateFile(std::wstring_view src, std::wstring_view target) {
  if (!bela::PathExists(src)) {
    // ignore
    DbgPrint(L"%s not exists", src);
    return true;
  }
  bela::error_code ec;
  if (!baulk::fs::MakeParentDir(target, ec)) {
    DbgPrint(L"failed MakeParentDir %s", ec);
    return false;
  }
  if (!move_file(src, target)) {
    ec = bela::make_system_error_code();
    return false;
  }
  if (!IsDebugMode) {
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mupdate %s done\x1b[0m", target);
    return true;
  }
  DbgPrint(L"update %s done", target);
  return true;
}

int ExecuteInternal(wchar_t *cmdline) {
  // run a new process and wait
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (CreateProcessW(nullptr, cmdline, nullptr, nullptr, FALSE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, nullptr,
                     vfs::AppLocationPath().data(), &si, &pi) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"run post script failed %s\n", ec);
    return -1;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return 0;
}

int PostUpdate() {
  constexpr std::wstring_view pwshcommand =
      L"Sleep 1; Remove-Item -Force -ErrorAction SilentlyContinue baulk-update.del";
  bela::EscapeArgv ea(L"powershell", L"-Command", pwshcommand);
  return ExecuteInternal(ea.data());
}

inline std::wstring UnarchivePath(std::wstring_view path) {
  auto dir = bela::DirName(path);
  auto filename = bela::BaseName(path);
  auto extName = bela::ExtensionEx(filename);
  if (filename.size() <= extName.size()) {
    return bela::StringCat(dir, L"\\out");
  }
  if (extName.empty()) {
    return bela::StringCat(dir, L"\\", filename, L".out");
  }
  return bela::StringCat(dir, L"\\", filename.substr(0, filename.size() - extName.size()));
}

bool UpdateBaulkExec(std::wstring_view newBaulkExec) {
  bela::error_code ec;
  auto appLocation = vfs::AppLocation();
  auto baulkExec = bela::StringCat(appLocation, L"\\bin\\baulk-exec.exe");
  auto baulkExecDel = bela::StringCat(appLocation, L"\\bin\\baulk-exec.del");
  auto baulkExecNew = bela::StringCat(appLocation, L"\\bin\\baulk-exec.new");
  if (bela::PathFileIsExists(baulkExec)) {
    if (MoveFileExW(baulkExec.data(), baulkExecDel.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) == TRUE) {
      if (MoveFileExW(newBaulkExec.data(), baulkExec.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) !=
          TRUE) {
        ec = bela::make_system_error_code();
        bela::FPrintF(stderr, L"baulk-update: update apply new baulk-exec: %s\n", ec);
        return false;
      }
      bela::fs::ForceDeleteFolders(baulkExecNew, ec);
      return true;
    }
  }
  if (MoveFileExW(newBaulkExec.data(), baulkExec.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) == TRUE) {
    return true;
  }
  if (MoveFileExW(newBaulkExec.data(), baulkExecNew.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) !=
      TRUE) {
    ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"baulk-update: update apply baulk-exec.new: %s\n", ec);
    return true;
  }
  return true;
}

int UpdateBaulk() {
  std::wstring url;
  std::wstring version;
  if (!ReleaseIsUpgradable(url, version)) {
    return 0;
  }
  auto filename = baulk::net::url_path_name(url);
  bela::FPrintF(stderr, L"\x1b[32mNew release url %s\x1b[0m\n", url);

  auto appTempDir = vfs::AppTemp();
  bela::error_code ec;
  if (!baulk::fs::MakeDir(appTempDir, ec)) {
    bela::FPrintF(stderr, L"baulk unable make %s error: %s\n", appTempDir, ec);
    return 1;
  }
  auto baulkfile = baulk::net::WinGet(url, appTempDir, L"", true, ec);
  if (!baulkfile) {
    bela::FPrintF(stderr, L"baulk get %s: \x1b[31m%s\x1b[0m\n", url, ec);
    return 1;
  }
  auto outdir = UnarchivePath(*baulkfile);
  if (bela::PathExists(outdir)) {
    bela::fs::ForceDeleteFolders(outdir, ec);
  }
  baulk::archive::ZipExtractor zr;
  if (!zr.OpenReader(*baulkfile, outdir, ec)) {
    bela::FPrintF(stderr, L"baulk extract %s error: %s\n", *baulkfile, ec);
    return 1;
  }
  if (!zr.Extract(ec)) {
    bela::FPrintF(stderr, L"baulk extract %s error: %s\n", *baulkfile, ec);
    return 1;
  }
  baulk::fs::MakeFlattened(outdir, outdir, ec);

  std::filesystem::path opath(outdir);
  auto appLocation = vfs::AppLocation();
  for (const auto &p : std::filesystem::recursive_directory_iterator(outdir)) {
    if (p.is_directory()) {
      continue;
    }
    std::error_code e;
    auto relative = std::filesystem::relative(p.path(), opath, e);
    if (e) {
      continue;
    }
    DbgPrint(L"found %s", relative.native());
    auto target = bela::StringCat(appLocation, L"\\", relative.native());
    if (bela::EqualsIgnoreCase(relative.native(), L"config\\baulk.json") && bela::PathExists(target)) {
      continue;
    }
    UpdateFile(p.path().native(), target);
  }
  if (!IsDebugMode) {
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[32mbaulk has been upgraded to %s\x1b[0m\n", version);
  }
  auto olddir = bela::StringCat(appLocation, L"\\old");
  baulk::fs::MakeDir(olddir, ec);

  auto newBaulkExec = bela::StringCat(outdir, L"\\bin\\baulk-exec.exe");
  if (bela::PathExists(newBaulkExec)) {
    UpdateBaulkExec(newBaulkExec);
  }
  auto newBaulkUpdate = bela::StringCat(outdir, L"\\bin\\baulk-update.exe");
  if (bela::PathExists(newBaulkUpdate)) {
    auto baulkUpdateNew = bela::StringCat(appLocation, L"\\bin\\baulk-update.exe");
    auto baulkUpdateDel = bela::StringCat(appLocation, L"\\bin\\baulk-update.del");
    if (MoveFileExW(baulkUpdateNew.data(), baulkUpdateDel.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) ==
        TRUE) {
    }
    if (MoveFileExW(newBaulkUpdate.data(), baulkUpdateNew.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) ==
        TRUE) {
      return PostUpdate();
    }
  }
  return 0;
}

} // namespace baulk::update

void Usage() {
  constexpr std::wstring_view usage = LR"(baulk-update - Baulk self update utility
Usage: baulk-update [option]
  -h|--help        Show usage text and quit
  -v|--version     Show version number and quit
  -V|--verbose     Make the operation more talkative
  -F|--force       Turn on force mode. such as force update frozen package
  -A|--user-agent  Send User-Agent <name> to server
  -k|--insecure    Allow insecure server connections when using SSL
  --preview        Download the latest preview version
  --https-proxy    Use this proxy. Equivalent to setting the environment variable 'HTTPS_PROXY'

)";
  bela::terminal::WriteAuto(stderr, usage);
}

void Version() {
  bela::FPrintF(stdout, L"baulk-update %s\nRelease:    %s\nCommit:     %s\nBuild Time: %s\n", BAULK_VERSION,
                BAULK_REFNAME, BAULK_REVISION, BAULK_BUILD_TIME);
}

bool ParseArgv(int argc, wchar_t **argv) {
  bela::ParseArgv pa(argc, argv);
  pa.Add(L"help", bela::no_argument, 'h')
      .Add(L"version", bela::no_argument, 'v')
      .Add(L"verbose", bela::no_argument, 'V')
      .Add(L"force", bela::no_argument, L'F')
      .Add(L"insecure", bela::no_argument, L'k')
      .Add(L"profile", bela::required_argument, 'P')
      .Add(L"user-agent", bela::required_argument, 'A')
      .Add(L"preview", bela::no_argument, 1001)
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
          baulk::update::IsDebugMode = true;
          HttpClient::DefaultClient().DebugMode() = true;
          break;
        case 'F':
          baulk::update::IsForceMode = true;
          break;
        case 'k':
          HttpClient::DefaultClient().InsecureMode() = true;
          break;
        case 'A':
          if (wcslen(oa) != 0) {
            HttpClient::DefaultClient().UserAgent() = oa;
          }
          break;
        case 1001:
          baulk::update::IsPreview = true;
          break;
        case 1002:
          SetEnvironmentVariableW(L"HTTPS_PROXY", oa);
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

int wmain(int argc, wchar_t **argv) {
  bela::error_code ec;
  if (!baulk::vfs::InitializeFastPathFs(ec)) {
    bela::FPrintF(stderr, L"baulk-exec InitializeFastPathFs error %s\n", ec);
    return 1;
  }
  if (!ParseArgv(argc, argv)) {
    return 1;
  }
  return baulk::update::UpdateBaulk();
}
