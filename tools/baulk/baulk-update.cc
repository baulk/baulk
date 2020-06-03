///
#include <baulkrev.hpp>
#include <bela/match.hpp>
#include <bela/strip.hpp>
#include <bela/io.hpp>
#include <bela/terminal.hpp>
#include <bela/path.hpp>
#include <bela/process.hpp>
#include <jsonex.hpp>
#include <zip.hpp>
#include "fs.hpp"
#include "net.hpp"

// https://www.catch22.net/tuts/win32/self-deleting-executables
// https://stackoverflow.com/questions/10319526/understanding-a-self-deleting-program-in-c

namespace baulk {
bool IsDebugMode = true;
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

// https://api.github.com/repos/baulk/baulk/releases/latest todo
bool ReleaseIsUpgradable(std::wstring &url, bool forcemode) {
  std::wstring releasename;
  if (!ResolveBaulkRev(releasename)) {
    bela::FPrintF(stderr, L"unable detect baulk meta, use baulk-update release name '%s'\n",
                  releasename);
  }
  constexpr std::wstring_view releaseprefix = L"refs/tags/";
  if (!bela::StartsWith(releasename, releaseprefix)) {
    baulk::DbgPrint(L"baulk refname '%s' not public release", releasename);
    if (!forcemode) {
      return false;
    }
  }
  auto release = bela::StripPrefix(releasename, releaseprefix);
  baulk::DbgPrint(L"detect current release %s", release);
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
    if (release == tagname) {
      bela::FPrintF(stderr, L"\x1b[33mbaulk/%s is up to date\x1b[0m", release);
      return false;
    }
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
  } catch (const std::exception &e) {
    bela::FPrintF(stderr, L"baulk upgrade self decode metadata: \x1b[31m%s\x1b[0m\n", e.what());
    return false;
  }
  return false;
}

bool UpdateFile(std::wstring_view src, std::wstring_view target) {
  if (!bela::PathExists(src)) {
    // ignore
    baulk::DbgPrint(L"%s not exists", src);
    return true;
  }
  if (MoveFileExW(src.data(), target.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) ==
      TRUE) {
    baulk::DbgPrint(L"update %s done", target);
    return true;
  }
  return true;
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

int BaulkUpdate(bool forcemode) {
  std::wstring url;
  if (!ReleaseIsUpgradable(url, forcemode)) {
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
  std::wstring outdir(*baulkfile);
  if (auto pos = outdir.rfind('.'); pos != std::wstring::npos) {
    outdir.resize(pos);
  } else {
    outdir.append(L".out");
  }
  if (bela::PathExists(outdir)) {
    baulk::fs::PathRemoveEx(outdir, ec);
  }
  baulk::DbgPrint(L"Decompress %s to %s\n", filename, outdir);
  if (!baulk::Decompress(*baulkfile, outdir, ec)) {
    bela::FPrintF(stderr, L"baulk decompress %s error: %s\n", *baulkfile, ec.message);
    return 1;
  }
  baulk::fs::FlatPackageInitialize(outdir, outdir, ec);
  constexpr std::wstring_view files[] = {
      // baulk
      L"\\bin\\baulk.exe",             // baulk main
      L"\\bin\\baulk-lnk.exe",         // baulk-lnk
      L"\\bin\\baulk-exec.exe",        // baulk-lnk
      L"\\bin\\ssh-askpass-baulk.exe", // ssh-askpass-baulk
      L"\\baulkterminal.exe"
      //
  };
  for (const auto f : files) {
    auto src = bela::StringCat(outdir, f);
    auto target = bela::StringCat(baulk::BaulkRoot, f);
    UpdateFile(src, target);
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

int wmain(int argc, wchar_t **argv) {
  constexpr std::wstring_view forcestr = L"--force";
  bool forcemode = false;
  for (int i = 1; i < argc; i++) {
    if (forcestr == argv[i]) {
      forcemode = true;
    }
  }
  baulk::InitializeBaulk();
  return baulk::BaulkUpdate(forcemode);
}