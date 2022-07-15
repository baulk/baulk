///
#ifndef BAULK_GENERATED_HPP
#define BAULK_GENERATED_HPP
#include <string_view>
#include <format>
#include <bela/str_cat.hpp>
#include <bela/str_split.hpp>
#include <bela/str_replace.hpp>
#include <bela/subsitute.hpp>
#include <bela/pe.hpp>
#include <bela/io.hpp>

namespace baulk::generated {
namespace template_internal {
constexpr std::wstring_view console_exe_source = LR"(#define WIN32_LEAN_AND_MEAN
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

constexpr std::wstring_view windows_exe_source = LR"(#define WIN32_LEAN_AND_MEAN
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

constexpr std::wstring_view resources = LR"(//Baulk generated resource script.
#include "windows.h"

VS_VERSION_INFO VERSIONINFO
FILEVERSION {}, {}, {}, {}
PRODUCTVERSION {}, {}, {}, {}
FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
FILEFLAGS 0x1L
#else
FILEFLAGS 0x0L
#endif
FILEOS 0x40004L
FILETYPE 0x1L
FILESUBTYPE 0x0L
BEGIN
BLOCK "StringFileInfo"
BEGIN
BLOCK "000904b0"
BEGIN
VALUE "CompanyName", L"{}"
VALUE "FileDescription", L"{}"
VALUE "FileVersion", L"{}"
VALUE "InternalName", L"{}"
VALUE "LegalCopyright", L"{}"
VALUE "OriginalFilename", L"{}"
VALUE "ProductName", L"{}"
VALUE "ProductVersion", L"{}"
END
END
BLOCK "VarFileInfo"
BEGIN
VALUE "Translation", 0x9, 1200
END
END

)";

} // namespace template_internal

struct VersionPart {
  int MajorPart{0};
  int MinorPart{0};
  int BuildPart{0};
  int PrivatePart{0};
};

inline VersionPart MakeVersionPart(std::wstring_view vs) {
  VersionPart vp;
  std::vector<std::wstring_view> vsv = bela::StrSplit(vs, bela::ByChar('.'), bela::SkipEmpty());
  if (!vsv.empty()) {
    (void)bela::SimpleAtoi(vsv[0], &vp.MajorPart);
  }
  if (vsv.size() > 1) {
    (void)bela::SimpleAtoi(vsv[1], &vp.MinorPart);
  }
  if (vsv.size() > 2) {
    (void)bela::SimpleAtoi(vsv[2], &vp.BuildPart);
  }
  if (vsv.size() > 3) {
    (void)bela::SimpleAtoi(vsv[3], &vp.PrivatePart);
  }
  return vp;
}

inline bool MakeResource(bela::pe::Version &version, std::wstring_view file, bela::error_code &ec) {
  auto copyright = bela::StrReplaceAll(version.LegalCopyright, {{L"(c)", L"\xA9"}, {L"(C)", L"\xA9"}});
  auto fv = MakeVersionPart(version.FileVersion);
  auto pv = MakeVersionPart(version.ProductVersion);
  auto ws = std::format(template_internal::resources,                           // template
                        fv.MajorPart, fv.MinorPart, fv.BuildPart, fv.BuildPart, // FILEVERSION
                        pv.MajorPart, pv.MinorPart, pv.BuildPart, pv.BuildPart, // PRODUCTVERSION
                        version.CompanyName,                                    // CompanyName
                        version.FileDescription,                                // FileDescription
                        version.FileVersion,                                    // FileVersion
                        version.InternalName,                                   // InternalName
                        copyright,                                              // LegalCopyright
                        version.OriginalFileName,                               // OriginalFilename
                        version.ProductName,                                    // ProductName
                        version.ProductVersion                                  // ProductVersion

  );
  return bela::io::WriteText(ws, file, ec);
}

inline bool MakeSource(std::wstring_view target, std::wstring_view file, bool consoleExe, bela::error_code &ec) {
  auto source_code = [&]() -> std::wstring {
    auto escape_target = bela::StrReplaceAll(target, {{L"\\", L"\\\\"}});
    if (consoleExe) {
      return bela::Substitute(template_internal::console_exe_source, escape_target);
    }
    return bela::Substitute(template_internal::windows_exe_source, escape_target);
  }();
  return bela::io::WriteText(source_code, file, ec);
}

} // namespace baulk::generated
#endif