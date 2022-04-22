#include "baulk-lnk.hpp"
#include <bela/picker.hpp>

namespace baulk {
bool IsDebugMode = false;
}
int WINAPI wWinMain(HINSTANCE /*unused*/, HINSTANCE /*unused*/, LPWSTR /*unused*/, int /*unused*/) {
  baulk::IsDebugMode = IsTrue(bela::GetEnv(L"BAULK_DEBUG"));
  bela::error_code ec;
  auto target = ResolveTarget(ec);
  if (!target) {
    bela::BelaMessageBox(nullptr, L"unable detect launcher target:", ec.message.data(), nullptr, bela::mbs_t::FATAL);
    return 1;
  }
  baulk::DbgPrint(L"resolve target: %s", *target);
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
    bela::BelaMessageBox(nullptr, L"unable create process:", ec.message.data(), nullptr, bela::mbs_t::FATAL);
    return -1;
  }
  CloseHandle(pi.hThread);
  if (!isconsole) {
    return 0;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  return static_cast<int>(exitCode);
}
