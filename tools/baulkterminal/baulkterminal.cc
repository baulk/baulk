//
#include <bela/base.hpp>
#include <bela/path.hpp>
#include <bela/env.hpp>
#include <bela/escapeargv.hpp>
#include <bela/finaly.hpp>
#include <bela/picker.hpp>

std::optional<std::wstring> FindWindowsTerminal() {
  auto wt = bela::ExpandEnv(L"%LOCALAPPDATA%\\Microsoft\\WindowsApps\\wt.exe");
  if (!bela::PathExists(wt)) {
    return std::nullopt;
  }
  return std::make_optional(std::move(wt));
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
  //
  auto wt = FindWindowsTerminal();
  if (!wt) {
    bela::BelaMessageBox(nullptr, L"unable open Windows Terminal",
                         L"no windows terminal installed", nullptr,
                         bela::mbs_t::FATAL);
    return 1;
  }
  bela::EscapeArgv ea;
  ea.Assign(*wt);
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (CreateProcessW(nullptr, ea.data(), nullptr, nullptr, FALSE,
                     CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si,
                     &pi) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::BelaMessageBox(nullptr, L"unable open Windows Terminal", ec.data(),
                         nullptr, bela::mbs_t::FATAL);
    return -1;
  }
  CloseHandle(pi.hThread);
  SetConsoleCtrlHandler(nullptr, TRUE);
  auto closer = bela::finally([&] {
    SetConsoleCtrlHandler(nullptr, FALSE);
    CloseHandle(pi.hProcess);
  });
  return 0;
}