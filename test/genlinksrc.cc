#include <bela/pe.hpp>
#include <bela/subsitute.hpp>
#include <bela/stdwriter.hpp>
#include <cstdio>

namespace baulk {
//
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
} // namespace baulk

void FlushFile(std::wstring_view str, std::wstring_view fn) {
  FILE *fd = nullptr;
  errno_t e = 0;
  if (_wfopen_s(&fd, fn.data(), L"w+") != 0) {
    return;
  }
  auto us = bela::ToNarrow(str);
  fwrite(us.data(), 1, us.size(), fd);
  fclose(fd);
}

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s target\n", argv[0]);
    return 1;
  }
  auto s1 = baulk::GenerateLinkSource(argv[1], bela::pe::Subsystem::CUI);
  auto s2 = baulk::GenerateLinkSource(argv[1], bela::pe::Subsystem::GUI);
  FlushFile(s1, L"genfile_console.cc");
  FlushFile(s2, L"genfile_windows.cc");
  return 0;
}