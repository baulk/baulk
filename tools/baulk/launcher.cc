//
#include <bela/pe.hpp>
#include <bela/subsitute.hpp>
#include <bela/finaly.hpp>
#include <bela/path.hpp>
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

// write UTF8
bool LinkSourceStore(std::wstring_view path, std::string_view source,
                     bela::error_code &ec) {
  auto FileHandle = ::CreateFileW(
      path.data(), FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ,
      nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    ec = bela::make_system_error_code();
    return false;
  }
  DWORD written = 0;
  auto p = source.data();
  auto size = source.size();
  while (size > 0) {
    auto len = (std::min)(size, static_cast<size_t>(4096));
    if (WriteFile(FileHandle, p, static_cast<DWORD>(len), &written, nullptr) !=
        TRUE) {
      ec = bela::make_system_error_code();
      break;
    }
    size -= written;
    p += written;
  }
  CloseHandle(FileHandle);
  return size == 0;
}

// ConvertToUTF8
bool LinkSourceStore(std::wstring_view path, std::wstring_view source,
                     bela::error_code &ec) {
  auto u8source = bela::ToNarrow(source);
  return LinkSourceStore(path, u8source, ec);
}

class LinkExecutor {
public:
  LinkExecutor() = default;
  LinkExecutor(const LinkExecutor &) = delete;
  LinkExecutor &operator=(const LinkExecutor &) = delete;
  ~LinkExecutor() {
    //
  }
  bool Initialize(const baulk::Package &pkg, bela::error_code &ec);
  bool Compile(std::wstring_view root, bela::error_code &ec);

private:
  std::wstring baulktemp;
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

bool LinkExecutor::Initialize(const baulk::Package &pkg, bela::error_code &ec) {
  if (baulktemp = MakeTempDir(ec); baulktemp.empty()) {
    return false;
  }
  if (!baulk::fs::MakeDir(baulktemp, ec)) {
    return false;
  }
  // --------------> write
  return false;
}

bool LinkExecutor::Compile(std::wstring_view root, bela::error_code &ec) {
  //
  return false;
}

bool MakeLaunchers(std::wstring_view root, const baulk::Package &pkg,
                   bool forceoverwrite, bela::error_code &ec) {
  auto pkgroot = bela::StringCat(root, L"\\pkg\\", pkg.name);
  return false;
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
