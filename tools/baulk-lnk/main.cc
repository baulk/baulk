#include "baulk-lnk.hpp"

int wmain(int argc, wchar_t **argv) {
  IsDebugMode = IsTrue(bela::GetEnv(L"BAULK_DEBUG"));
  bela::error_code ec;
  auto target = ResolveTarget(ec);
  if (!target) {
    bela::FPrintF(stderr, L"unable detect launcher target: %s\n", ec.message);
    return 1;
  }
  DbgPrint(L"resolve target: %s\n", *target);
  auto isconsole = IsSubsytemConsole(*target);
  std::wstring newcmd(GetCommandLineW());
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (CreateProcessW(target->data(), newcmd.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr,
                     nullptr, &si, &pi) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"baulk-lnk, unable create lnk process: %s\n", ec.message);
    return -1;
  }
  CloseHandle(pi.hThread);
  SetConsoleCtrlHandler(nullptr, TRUE);
  auto closer = bela::finally([&] {
    SetConsoleCtrlHandler(nullptr, FALSE);
    CloseHandle(pi.hProcess);
  });
  if (!isconsole) {
    return 0;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  return static_cast<int>(exitCode);
}
