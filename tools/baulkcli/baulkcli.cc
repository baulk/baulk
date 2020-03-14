///
#include <bela/base.hpp>
#include <bela/escapeargv.hpp>
#include <bela/pe.hpp>
#include <bela/stdwriter.hpp>
#include <bela/finaly.hpp>
#include <filesystem>
namespace fs = std::filesystem;

bool IsSubsytemConsole(std::wstring_view exe) {
  bela::error_code ec;
  auto pe = bela::pe::Expose(exe, ec);
  if (!pe) {
    return true;
  }
  return pe->subsystem == bela::pe::Subsystem::CUI;
}

std::optional<std::wstring> ResolveTarget(std::wstring_view arg0,
                                          bela::error_code &ec) {
  auto launcher = fs::path(arg0).filename().wstring();

  return std::nullopt;
}

int wmain(int argc, wchar_t **argv) {
  bela::error_code ec;
  auto target = ResolveTarget(argv[0], ec);
  if (!target) {
    bela::FPrintF(stderr, L"unable detect launcher target: %s\n", ec.message);
    return 1;
  }
  auto isconsole = IsSubsytemConsole(*target);
  bela::EscapeArgv ea;
  ea.Assign(*target);
  for (int i = 1; i < argc; i++) {
    ea.Append(argv[i]);
  }
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (CreateProcessW(nullptr, ea.data(), nullptr, nullptr, FALSE,
                     CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si,
                     &pi) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"unable detect launcher target: %s\n", ec.message);
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
  return 0;
}