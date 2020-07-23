///
#include <baulkrev.hpp>
#include <bela/match.hpp>
#include <bela/strip.hpp>
#include <bela/io.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/process.hpp>
#include <bela/parseargv.hpp>
#include <jsonex.hpp>
#include <zip.hpp>
#include "fs.hpp"
#include "net.hpp"
#include "outfile.hpp"

// https://www.catch22.net/tuts/win32/self-deleting-executables
// https://stackoverflow.com/questions/10319526/understanding-a-self-deleting-program-in-c

namespace baulk {
bool IsDebugMode = false;
bool IsForceMode = false;
constexpr size_t UerAgentMaximumLength = 64;
wchar_t UserAgent[UerAgentMaximumLength] = L"Wget/5.0 (Baulk)";
std::wstring BaulkRoot;
std::wstring locale;
std::wstring_view BaulkLocale() { return locale; }
void InitializeBaulk() {
  bela::error_code ec;
  if (auto exedir = bela::ExecutableParent(ec); exedir) {
    BaulkRoot.assign(std::move(*exedir));
    bela::PathStripName(BaulkRoot);
  } else {
    bela::FPrintF(stderr, L"unable find executable path: %s\n", ec.message);
    BaulkRoot = L".";
  }
  locale.resize(64);
  if (auto n = GetUserDefaultLocaleName(locale.data(), 64); n != 0 && n < 64) {
    locale.resize(n);
    return;
  }
  locale.clear();
}

template <typename... Args> bela::ssize_t DbgPrint(const wchar_t *fmt, Args... args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  std::wstring str;
  str.append(L"\x1b[33m* ");
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
  if (str.back() == '\n') {
    str.pop_back();
  }
  str.append(L"\x1b[0m\n");
  return bela::terminal::WriteAuto(stderr, str);
}
inline bela::ssize_t DbgPrint(const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  std::wstring_view msg(fmt);
  if (!msg.empty() && msg.back() == '\n') {
    msg.remove_suffix(1);
  }
  return bela::terminal::WriteAuto(stderr, bela::StringCat(L"\x1b[33m* ", msg, L"\x1b[0m\n"));
}

template <typename... Args>
bela::ssize_t DbgPrintEx(char32_t prefix, const wchar_t *fmt, Args... args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  auto str = bela::StringCat(L"\x1b[32m* ", prefix, L" ");
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
  if (str.back() == '\n') {
    str.pop_back();
  }
  str.append(L"\x1b[0m\n");
  return bela::terminal::WriteAuto(stderr, str);
}
inline bela::ssize_t DbgPrintEx(char32_t prefix, const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  std::wstring_view msg(fmt);
  if (!msg.empty() && msg.back() == '\n') {
    msg.remove_suffix(1);
  }
  return bela::terminal::WriteAuto(stderr,
                                   bela::StringCat(L"\x1b[32m", prefix, L" ", msg, L"\x1b[0m\n"));
}

constexpr const std::wstring_view archfilesuffix() {
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

int32_t OnEntry(void *handle, void *userdata, mz_zip_file *file_info, const char *path) {
  bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mx %s\x1b[0m", file_info->filename);
  return 0;
}

bool Decompress(std::wstring_view src, std::wstring_view outdir, bela::error_code &ec) {
  baulk::archive::zip::zip_closure closure{nullptr, nullptr, OnEntry};
  auto flush = bela::finally([] { bela::FPrintF(stderr, L"\n"); });
  return baulk::archive::zip::ZipExtract(src, outdir, ec, &closure);
}

bool ResolveBaulkRev(std::wstring &releasename) {
  bela::process::Process ps;
  auto baulkexe = bela::StringCat(BaulkRoot, L"\\bin\\baulk.exe");
  if (ps.Capture(baulkexe, L"--version") != 0) {
    if (auto ec = ps.ErrorCode(); ec) {
      bela::FPrintF(stderr, L"run %s error %s\n", baulkexe, ec.message);
    }
    releasename.assign(BAULK_RELEASE_NAME);
    return false;
  }
  auto out = bela::ToWide(ps.Out());

  std::vector<std::wstring_view> lines = bela::StrSplit(out, bela::ByChar('\n'), bela::SkipEmpty());
  constexpr std::wstring_view releaseprefix = L"Release:";
  for (auto line : lines) {
    line = bela::StripAsciiWhitespace(line);
    if (!bela::StartsWith(line, releaseprefix)) {
      continue;
    }
    auto relname = bela::StripAsciiWhitespace(line.substr(releaseprefix.size()));
    releasename.assign(relname);
    return true;
  }
  releasename.assign(BAULK_RELEASE_NAME);
  return false;
}

bool ReleaseIsUpgradableFallback(std::wstring &url, std::wstring &version,
                                 std::wstring_view oldver) {
  // https://github.com/baulk/baulk/releases/latest
  constexpr std::wstring_view latest = L"https://github.com/baulk/baulk/releases/latest";
  bela::error_code ec;
  auto resp = baulk::net::RestGet(latest, ec);
  if (!resp) {
    bela::FPrintF(stderr, L"baulk upgrade get latest: \x1b[31m%s\x1b[0m\n", ec.message);
    return false;
  }
  constexpr std::string_view urlprefix = "/baulk/baulk/releases/download/";
  size_t index = 0;
  for (;;) {
    auto pos = resp->body.find(urlprefix, index);
    if (pos == std::wstring::npos) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s is up to date\x1b[0m\n", oldver);
      return false;
    }
    auto pos2 = resp->body.find('"', pos);
    if (pos2 == std::wstring::npos) {
      bela::FPrintF(stderr, L"baulk upgrade get %s: \x1b[31minvalid html url\x1b[0m\n", oldver);
      return false;
    }
    std::wstring filename = bela::ToWide(resp->body.data() + pos, pos2 - pos);

    index = pos2 + 1;
    std::vector<std::wstring_view> svv =
        bela::StrSplit(filename, bela::ByChar('/'), bela::SkipEmpty());
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
  if (!ResolveBaulkRev(releasename)) {
    bela::FPrintF(stderr, L"unable detect baulk meta, use baulk-update release name '%s'\n",
                  releasename);
  }
  constexpr std::wstring_view releaseprefix = L"refs/tags/";
  if (!bela::StartsWith(releasename, releaseprefix)) {
    baulk::DbgPrint(L"baulk refname '%s' not public release", releasename);
    if (!baulk::IsForceMode) {
      return false;
    }
  }
  auto oldver = bela::StripPrefix(releasename, releaseprefix);
  baulk::DbgPrint(L"detect current version %s", oldver);
  bela::error_code ec;
  auto resp = baulk::net::RestGet(L"https://api.github.com/repos/baulk/baulk/releases/latest", ec);
  if (!resp) {
    bela::FPrintF(stderr, L"baulk upgrade self get metadata: \x1b[31m%s\x1b[0m\n", ec.message);
    return false;
  }
  try {
    /* code */
    auto obj = nlohmann::json::parse(resp->body);
    auto tagname = bela::ToWide(obj["tag_name"].get<std::string_view>());
    if (oldver == tagname) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s is up to date\x1b[0m", version);
      return false;
    }
    version = tagname;
    auto it = obj.find("assets");
    if (it == obj.end()) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s build is not yet complete\x1b[0m", tagname);
      return false;
    }
    for (const auto &p : it.value()) {
      auto name = bela::ToWide(p["name"].get<std::string_view>());
      if (!bela::EndsWithIgnoreCase(name, archfilesuffix())) {
        continue;
      }
      url = bela::ToWide(p["browser_download_url"].get<std::string_view>());
      return true;
    }
    //
  } catch (const std::exception &) {
    return ReleaseIsUpgradableFallback(url, version, oldver);
  }
  return false;
}

bool UpdateFile(std::wstring_view src, std::wstring_view target) {
  if (!bela::PathExists(src)) {
    // ignore
    baulk::DbgPrint(L"%s not exists", src);
    return true;
  }
  bela::error_code ec;
  if (!baulk::fs::MakeParentDir(target, ec)) {
    baulk::DbgPrint(L"failed MakeParentDir %s", ec.message);
    return false;
  }
  if (MoveFileExW(src.data(), target.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) ==
      TRUE) {
    if (!baulk::IsDebugMode) {
      bela::FPrintF(stderr, L"\x1b[2K\r\x1b[33mupdate %s done\x1b[0m", target);
      return true;
    }
    baulk::DbgPrint(L"update %s done", target);
    return true;
  }
  ec = bela::make_system_error_code();
  return false;
}

int ExecuteInternal(wchar_t *cmdline) {
  // run a new process and wait
  auto cwd = bela::StringCat(baulk::BaulkRoot, L"\\bin");
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (CreateProcessW(nullptr, cmdline, nullptr, nullptr, FALSE,
                     CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, nullptr, cwd.data(), &si,
                     &pi) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"run post script failed %s\n", ec.message);
    return -1;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return 0;
}

int PostUpdate() {
  constexpr std::wstring_view pwshcommand =
      L"Sleep 1; Remove-Item -Force -ErrorAction SilentlyContinue baulk-update.exe;Move-Item "
      L"baulk-update-new.exe baulk-update.exe";
  bela::EscapeArgv ea(L"powershell", L"-Command", pwshcommand);
  return ExecuteInternal(ea.data());
}

int BaulkUpdate() {
  std::wstring url;
  std::wstring version;
  if (!ReleaseIsUpgradable(url, version)) {
    return 0;
  }
  auto filename = baulk::net::UrlFileName(url);
  bela::FPrintF(stderr, L"\x1b[32mNew release url %s\x1b[0m\n", url);

  auto pkgtmpdir = bela::StringCat(baulk::BaulkRoot, L"\\bin\\pkgs\\.pkgtmp");
  bela::error_code ec;
  if (!baulk::fs::MakeDir(pkgtmpdir, ec)) {
    bela::FPrintF(stderr, L"baulk unable make %s error: %s\n", pkgtmpdir, ec.message);
    return 1;
  }
  auto baulkfile = baulk::net::WinGet(url, pkgtmpdir, true, ec);
  if (!baulkfile) {
    bela::FPrintF(stderr, L"baulk get %s: \x1b[31m%s\x1b[0m\n", url, ec.message);
    return 1;
  }
  auto outdir = baulk::ArchiveExtensionConversion(*baulkfile);
  if (bela::PathExists(outdir)) {
    baulk::fs::PathRemoveEx(outdir, ec);
  }
  baulk::DbgPrint(L"Decompress %s to %s\n", filename, outdir);
  if (!baulk::Decompress(*baulkfile, outdir, ec)) {
    bela::FPrintF(stderr, L"baulk decompress %s error: %s\n", *baulkfile, ec.message);
    return 1;
  }
  baulk::fs::FlatPackageInitialize(outdir, outdir, ec);

  std::filesystem::path opath(outdir);
  for (const auto &p : std::filesystem::recursive_directory_iterator(outdir)) {
    if (p.is_directory()) {
      continue;
    }
    std::error_code e;
    auto relative = std::filesystem::relative(p.path(), opath, e);
    if (e) {
      continue;
    }
    auto relativepath = relative.wstring();
    // skip all config

    DbgPrint(L"found %s", relativepath);
    auto target = bela::StringCat(baulk::BaulkRoot, L"\\", relativepath);
    if (bela::EqualsIgnoreCase(relativepath, L"config\\baulk.json") && bela::PathExists(target)) {
      continue;
    }
    UpdateFile(p.path().wstring(), target);
  }
  if (!baulk::IsDebugMode) {
    bela::FPrintF(stderr, L"\x1b[2K\r\x1b[32mbaulk has been upgraded to %s\x1b[0m\n", version);
  }

  auto srcupdater = bela::StringCat(outdir, L"\\bin\\baulk-update.exe");
  if (bela::PathExists(srcupdater)) {
    auto targeter = bela::StringCat(baulk::BaulkRoot, L"\\bin\\baulk-update-new.exe");
    if (MoveFileExW(srcupdater.data(), targeter.data(),
                    MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) == TRUE) {
      return PostUpdate();
    }
  }
  return 0;
}

} // namespace baulk

void Usage() {
  constexpr std::wstring_view usage = LR"(baulk-update - Baulk self update utility
Usage: baulk-update [option]
  -h|--help        Show usage text and quit
  -v|--version     Show version number and quit
  -V|--verbose     Make the operation more talkative
  -F|--force       Turn on force mode. such as force update frozen package
  -A|--user-agent  Send User-Agent <name> to server
  --https-proxy    Use this proxy. Equivalent to setting the environment variable 'HTTPS_PROXY'

)";
  bela::terminal::WriteAuto(stderr, usage);
}

void Version() {
  bela::FPrintF(stdout, L"baulk-update %s\nRelease:    %s\nCommit:     %s\nBuild Time: %s\n",
                BAULK_VERSION, BAULK_REFNAME, BAULK_REVISION, BAULK_BUILD_TIME);
}

bool ParseArgv(int argc, wchar_t **argv) {
  bela::ParseArgv pa(argc, argv);
  pa.Add(L"help", bela::no_argument, 'h')
      .Add(L"version", bela::no_argument, 'v')
      .Add(L"verbose", bela::no_argument, 'V')
      .Add(L"force", bela::no_argument, L'F')
      .Add(L"", bela::no_argument, L'f')
      .Add(L"profile", bela::required_argument, 'P')
      .Add(L"user-agent", bela::required_argument, 'A')
      .Add(L"https-proxy", bela::required_argument, 1001);

  bela::error_code ec;
  auto result = pa.Execute(
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
          break;
        case 'f':
          [[fallthrough]];
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
    bela::FPrintF(stderr, L"baulk-update ParseArgv error: %s\n", ec.message);
    return false;
  }
  return true;
}

int wmain(int argc, wchar_t **argv) {
  if (!ParseArgv(argc, argv)) {
    return 1;
  }
  baulk::InitializeBaulk();
  return baulk::BaulkUpdate();
}
