////////////////////////
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winnt.h>
#include <Shellapi.h>
#include <Shlobj.h>
#include <stdio.h>
#include <stdlib.h>

#define BLASTLINK_TARGET L"F:\\Clangbuilder\\sources\\template\\argv_test.exe"

int LinkToApp(const wchar_t *target) {
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (!CreateProcessW(BLASTLINK_TARGET, GetCommandLineW(), nullptr, nullptr,
                      FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si,
                      &pi)) {
    return -1;
  }
  CloseHandle(pi.hThread);
  SetConsoleCtrlHandler(nullptr, TRUE);
  WaitForSingleObject(pi.hProcess, INFINITE);
  SetConsoleCtrlHandler(nullptr, FALSE);
  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  CloseHandle(pi.hProcess);
  return exitCode;
}

int wmain() {
  ///
  ExitProcess(LinkToApp(BLASTLINK_TARGET));
}