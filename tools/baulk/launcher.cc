//
#include <bela/subsitute.hpp>
#include <bela/finaly.hpp>
#include <bela/path.hpp>
#include <bela/pe.hpp>
#include "baulk.hpp"
#include "fs.hpp"
#include "rcwriter.hpp"

namespace baulk {
// make launcher
namespace internal {
constexpr std::wstring_view consoletemplete = LR"(#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winnt.h>
inline constexpr size_t StringLength(const wchar_t *s) {
  const wchar_t *a = s;
  for (; *a != 0; a++) {
    ;
  }
  return a - s;
}
inline void StringZero(wchar_t *p, size_t len) {
  for (size_t i = 0; i < len; i++) {
    p[i] = 0;
  }
}
inline void *StringCopy(wchar_t *dest, const wchar_t *src, size_t n) {
  auto d = dest;
  for (; n; n--) {
    *d++ = *src++;
  }
  return dest;
}
inline wchar_t *StringAllocate(size_t count) {
  return reinterpret_cast<wchar_t *>(
      HeapAlloc(GetProcessHeap(), 0, sizeof(wchar_t) * count));
}
inline wchar_t *StringDup(const wchar_t *s) {
  auto l = StringLength(s);
  auto ds = StringAllocate(l + 1);
  StringCopy(ds, s, l);
  ds[l] = 0;
  return ds;
}
inline void StringFree(wchar_t *p) { HeapFree(GetProcessHeap(), 0, p); }
int wmain() {
  constexpr const wchar_t *target = L"$0";
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  auto cmdline = StringDup(GetCommandLineW());
  if (!CreateProcessW(target, cmdline, nullptr, nullptr, FALSE,
                      CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si, &pi)) {
    StringFree(cmdline);
    return -1;
  }
  StringFree(cmdline);
  CloseHandle(pi.hThread);
  SetConsoleCtrlHandler(nullptr, TRUE);
  WaitForSingleObject(pi.hProcess, INFINITE);
  SetConsoleCtrlHandler(nullptr, FALSE);
  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  CloseHandle(pi.hProcess);
  return exitCode;
}
)";

constexpr std::wstring_view windowstemplate = LR"(#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstddef>
inline constexpr size_t StringLength(const wchar_t *s) {
  const wchar_t *a = s;
  for (; *a != 0; a++) {
    ;
  }
  return a - s;
}
inline void StringZero(wchar_t *p, size_t len) {
  for (size_t i = 0; i < len; i++) {
    p[i] = 0;
  }
}
inline void *StringCopy(wchar_t *dest, const wchar_t *src, size_t n) {
  auto d = dest;
  for (; n; n--) {
    *d++ = *src++;
  }
  return dest;
}
inline wchar_t *StringAllocate(size_t count) {
  return reinterpret_cast<wchar_t *>(
      HeapAlloc(GetProcessHeap(), 0, sizeof(wchar_t) * count));
}
inline wchar_t *StringDup(const wchar_t *s) {
  auto l = StringLength(s);
  auto ds = StringAllocate(l + 1);
  StringCopy(ds, s, l);
  ds[l] = 0;
  return ds;
}
inline void StringFree(wchar_t *p) { HeapFree(GetProcessHeap(), 0, p); }
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
  constexpr const wchar_t *target = L"$0";
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  auto cmdline = StringDup(GetCommandLineW());
  if (!CreateProcessW(target, cmdline, nullptr, nullptr, FALSE,
                      CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si, &pi)) {
    StringFree(cmdline);
    return -1;
  }
  StringFree(cmdline);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return 0;
}
)";

constexpr std::wstring_view batchtemplate = LR"(@echo off
baulk exec "$0" %*)";
} // namespace internal

// GenerateLinkSource generate link sources
std::wstring GenerateLinkSource(std::wstring_view target,
                                bela::pe::Subsystem subs) {
  std::wstring escapetarget;
  escapetarget.reserve(target.size() + 10);
  for (auto c : target) {
    if (c == '\\') {
      escapetarget.append(L"\\\\");
      continue;
    }
    escapetarget.push_back(c);
  }
  if (subs == bela::pe::Subsystem::CUI) {
    return bela::Substitute(internal::consoletemplete, escapetarget);
  }
  return bela::Substitute(internal::windowstemplate, escapetarget);
}

class LinkExecutor {
public:
  LinkExecutor() = default;
  LinkExecutor(const LinkExecutor &) = delete;
  LinkExecutor &operator=(const LinkExecutor &) = delete;
  ~LinkExecutor() {
    if (!baulktemp.empty()) {
      std::error_code ec;
      std::filesystem::remove_all(baulktemp, ec);
    }
  }
  bool Initialize(bela::error_code &ec);
  bool Compile(const baulk::Package &pkg, std::wstring_view source,
               std::wstring_view linkdir, bela::error_code &ec);
  const std::vector<std::wstring> &LinkExes() const { return linkexes; }

private:
  std::wstring baulktemp;
  std::vector<std::wstring> linkexes;
};

std::wstring MakeTempDir(bela::error_code ec) {
  std::error_code e;
  auto tmppath = std::filesystem::temp_directory_path(e);
  if (e) {
    ec = bela::from_std_error_code(e);
    return L"";
  }
  auto tmpdir = tmppath.wstring();
  auto len = tmpdir.size();
  wchar_t X = 'A';
  bela::AlphaNum an(GetCurrentThreadId());
  for (wchar_t X = 'A'; X < 'Z'; X++) {
    bela::StrAppend(&tmpdir, L"\\BaulkTemp", X, an);
    if (std::filesystem::exists(tmpdir, e)) {
      return tmpdir;
    }
    tmpdir.resize(len);
  }
  ec = bela::make_error_code(1, L"cannot create tempdir");
  return L"";
}

bool LinkExecutor::Initialize(bela::error_code &ec) {
  if (baulktemp = MakeTempDir(ec); baulktemp.empty()) {
    return false;
  }
  if (!baulk::fs::MakeDir(baulktemp, ec)) {
    return false;
  }
  return true;
}

inline int SubsystemIndex(bela::pe::Subsystem subs) {
  if (subs == bela::pe::Subsystem::CUI) {
    return 0;
  }
  return 1;
}

inline std::wstring_view StripExtension(std::wstring_view filename) {
  auto pos = filename.rfind('.');
  if (pos == std::wstring_view::npos) {
    return filename;
  }
  return filename.substr(0, pos);
}

inline void StringIgnoreEmpty(std::wstring &s, std::wstring_view d) {
  if (s.empty()) {
    s = d;
  }
}

bool LinkExecutor::Compile(const baulk::Package &pkg, std::wstring_view source,
                           std::wstring_view linkdir, bela::error_code &ec) {
  //      bela::FPrintF(stderr,L"link '%s' to '%s' success\n",source,linkdir);
  constexpr const std::wstring_view entry[] = {L"-ENTRY:wmain",
                                               L"-ENTRY:wWinMain"};
  constexpr const std::wstring_view subsystemnane[] = {L"-SUBSYSTEM:CONSOLE",
                                                       L"-SUBSYSTEM:WINDOWS"};
  auto pe = bela::pe::Expose(source, ec);
  if (!pe) {
    // not pe subname
    return false;
  }
  auto index = SubsystemIndex(pe->subsystem);
  auto exename = baulk::fs::FileName(source);
  auto name = StripExtension(exename);
  auto cxxsrcname = bela::StringCat(name, L".cc");
  auto cxxsrc = bela::StringCat(baulktemp, L"\\", cxxsrcname);
  auto rcsrcname = bela::StringCat(name, L".rc");
  auto rcsrc = bela::StringCat(baulktemp, L"\\", rcsrcname);
  if (baulk::io::WriteText(GenerateLinkSource(source, pe->subsystem), cxxsrc,
                           ec)) {
    return false;
  }
  bool rcwrited = false;
  if (auto vi = bela::pe::ExposeVersion(source, ec); vi) {
    baulk::rc::Writer w;
    if (vi->CompanyName.empty()) {
      vi->CompanyName = bela::StringCat(pkg.name, L" contributors");
    }
    StringIgnoreEmpty(vi->FileDescription, pkg.description);
    StringIgnoreEmpty(vi->FileVersion, pkg.version);
    StringIgnoreEmpty(vi->ProductVersion, pkg.version);
    StringIgnoreEmpty(vi->ProductName, pkg.name);
    StringIgnoreEmpty(vi->OriginalFileName, exename);
    StringIgnoreEmpty(vi->InternalName, exename);
    rcwrited = w.WriteVersion(*vi, rcsrc, ec);
  } else {
    bela::pe::VersionInfo nvi;
    nvi.CompanyName = bela::StringCat(pkg.name, L" contributors");
    nvi.FileDescription = pkg.description;
    nvi.FileVersion = pkg.version;
    nvi.ProductVersion = pkg.version;
    nvi.ProductName = pkg.name;
    nvi.OriginalFileName = exename;
    nvi.InternalName = exename;
    baulk::rc::Writer w;
    rcwrited = w.WriteVersion(nvi, rcsrc, ec);
  }
  if (rcwrited) {
    if (!baulk::BaulkExecutor().Execute(baulktemp, L"rc", L"-nologo",
                                        rcsrcname)) {
      rcwrited = false;
    }
  }
  if (baulk::BaulkExecutor().Execute(baulktemp, L"cl", L"-c", L"-std:c++17",
                                     L"-nologo", L"-Os", cxxsrcname) != 0) {
    // compiler failed
    return false;
  }
  int exitcode = 0;
  if (rcwrited) {
    exitcode = baulk::BaulkExecutor().Execute(
        baulktemp, L"link", L"-nologo", L"-OPT:REF", L"-OPT:ICF",
        L"-NODEFAULTLIB", subsystemnane[index], entry[index],
        bela::StringCat(name, L".obj"), bela::StringCat(name, L".res"),
        L"kernel32.lib", L"user32.lib", bela::StringCat(L"-OUT:", exename));
  } else {
    exitcode = baulk::BaulkExecutor().Execute(
        baulktemp, L"link", L"-nologo", L"-OPT:REF", L"-OPT:ICF",
        L"-NODEFAULTLIB", subsystemnane[index], entry[index],
        bela::StringCat(cxxsrcname, L".obj"), L"kernel32.lib", L"user32.lib",
        bela::StringCat(L"-OUT:", exename));
  }
  if (exitcode != 0) {
    ec = baulk::BaulkExecutor().LastErrorCode();
    return false;
  }
  auto target = bela::StringCat(linkdir, L"\\", exename);
  auto newtarget = bela::StringCat(baulktemp, L"\\", exename);
  std::error_code e;
  if (std::filesystem::exists(target, e)) {
    std::filesystem::remove_all(target, e);
  }
  std::filesystem::rename(newtarget, target, e);
  if (!e) {
    ec = bela::from_std_error_code(e);
    return false;
  }
  linkexes.emplace_back(std::move(exename));
  return true;
}

bool MakeBatchLauncher(std::wstring_view root, const baulk::Package &pkg,
                       bool forceoverwrite, bela::error_code &ec) {

  return true;
}

bool MakeLaunchers(std::wstring_view root, const baulk::Package &pkg,
                   bool forceoverwrite, bela::error_code &ec) {
  auto pkgroot = bela::StringCat(root, L"bin\\pkg\\", pkg.name);
  auto linkdir = bela::StringCat(root, L"bin\\.linked");
  LinkExecutor executor;
  if (!executor.Initialize(ec)) {
    return false;
  }
  for (const auto &n : pkg.launchers) {
    auto source = bela::PathCat(pkgroot, n);
    if (!executor.Compile(pkg, source, linkdir, ec)) {
      bela::FPrintF(stderr, L"'%s' unable create launcher: \x1b[31m%s\x1b[0m\n",
                    source, ec.message);
    }
  }
  return true;
}

// create symlink
bool MakeSymlinks(std::wstring_view root, const baulk::Package &pkg,
                  bool forceoverwrite, bela::error_code &ec) {
  auto pkgroot = bela::StringCat(root, L"\\bin\\pkg\\", pkg.name);
  auto linked = bela::StringCat(root, L"\\bin\\pkg\\.linked\\");
  for (const auto &lnk : pkg.links) {
    auto src = bela::PathCat(pkgroot, lnk);
    auto fn = baulk::fs::FileName(src);
    auto lnk = bela::StringCat(linked, fn);
    if (bela::PathExists(lnk)) {
      if (forceoverwrite) {
        baulk::fs::PathRemove(lnk, ec);
      }
    }
    if (!baulk::fs::SymLink(src, lnk, ec)) {
      return false;
    }
  }
  return false;
}

bool MakeLinks(std::wstring_view root, const baulk::Package &pkg,
               bool forceoverwrite, bela::error_code &ec) {
  if (!pkg.links.empty() && !MakeSymlinks(root, pkg, forceoverwrite, ec)) {
    return false;
  }
  if (!pkg.launchers.empty() && !MakeLaunchers(root, pkg, forceoverwrite, ec)) {
    return false;
  }
  return true;
}
} // namespace baulk
