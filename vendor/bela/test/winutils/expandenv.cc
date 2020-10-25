//
#include <bela/env.hpp>
#include <bela/terminal.hpp>

std::wstring ExpandEnv2(std::wstring_view s) {
  auto N = ExpandEnvironmentStringsW(s.data(), nullptr, 0);
  if (N == 0) {
    return L"";
  }
  std::wstring buf;
  buf.resize(N);
  // If the function succeeds, the return value is the number of TCHARs stored
  // in the destination buffer, including the terminating null character.
  N = ExpandEnvironmentStringsW(s.data(), buf.data(), N); // Include
  buf.resize(N - 1);
  return buf;
}

int wmain() {
  const wchar_t *svv[] = {
      //
      L"%SystemRoot%\\System32\\cmd.exe",                    // Normal
      L"^%SystemRoot%\\System32\\cmd.exe",                   // Normal
      L"^%SystemRoot^%\\System32\\cmd.exe",                  // Normal
      L"%%SystemRoot%%\\System32\\cmd.exe",                  // TODO
      L"%%%SystemRoot%%\\System32\\cmd.exe",                 // TODO
      L"%%%SystemRoot%%%\\System32\\cmd.exe",                // TODO
      L"%%%SystemRoot%%%\\System32\\cmd.exe;%COMPUTERNAME%", // TODO
      L"%NOTAENVNAME%someone",                               // NOT a env
      L"%%%%-----",                                          //
      L"----------%-------------",                           //
      L"-----------------------%"
      //
  };
  for (auto s : svv) {
    auto es = bela::WindowsExpandEnv(s);
    bela::FPrintF(stderr, L"[%s] Expand to [%s]\n", s, es);
    auto ss = bela::PathUnExpand(s);
    bela::FPrintF(stderr, L"PathUnExpand [%s]\n", ss);
  }
  auto ss = bela::PathUnExpand(L"%APPDATA%\\Microsoft\\WindowsApp\\wt.exe");
  bela::FPrintF(stderr, L"PathUnExpand [%s]\n", ss);
  return 0;
}