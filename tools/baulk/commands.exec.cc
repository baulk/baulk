// exec command
#include <bela/stdwriter.hpp>
#include <bela/pe.hpp>
#include <bela/base.hpp>
#include <bela/escapeargv.hpp>
#include <bela/path.hpp>
#include <filesystem>
#include "baulk.hpp"
#include "commands.hpp"
#include "fs.hpp"

namespace baulk::commands {

inline bool IsSubsytemConsole(std::wstring_view exe) {
  bela::error_code ec;
  auto realexe = bela::RealPathEx(exe, ec);
  if (!realexe) {
    return false;
  }
  auto pe = bela::pe::Expose(*realexe, ec);
  if (!pe) {
    return true;
  }
  return pe->subsystem == bela::pe::Subsystem::CUI;
}
int cmd_exec(const argv_t &argv) {
  if (argv.empty()) {
    bela::FPrintF(stderr, L"usage: baulk exec [command [arguments ...]]\n");
    return 1;
  }
  std::wstring target;
  auto arg0 = argv[0];
  if (!bela::ExecutableExistsInPath(arg0, target)) {
    bela::FPrintF(stderr, L"unable found target: '%s'\n", arg0);
    return 1;
  }
  baulk::DbgPrint(L"Find %s ==> %s", arg0, target);
  auto isconsole = IsSubsytemConsole(target);
  bela::EscapeArgv ea;
  ea.Assign(target);
  for (size_t i = 1; i < argv.size(); i++) {
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
} // namespace baulk::commands
