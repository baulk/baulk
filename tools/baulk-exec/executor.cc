//
#include <bela/time.hpp>
#include <bela/parseargv.hpp>
#include <bela/process.hpp>
#include <bela/subsitute.hpp>
#include <bela/strip.hpp>
#include <bela/pe.hpp>
#include <baulk/pwsh.hpp>
#include "baulk-exec.hpp"

namespace baulk::exec {
inline bela::Duration FromKernelTick(FILETIME ft) {
  auto tick = std::bit_cast<int64_t, FILETIME>(ft);
  return bela::Microseconds(tick / 10);
}

void Summarizer::SummarizeTime(HANDLE hProcess) {
  FILETIME creation_time{0};
  FILETIME exit_time{0};
  FILETIME kernel_time{0};
  FILETIME user_time{0};
  if (GetProcessTimes(hProcess, &creation_time, &exit_time, &kernel_time, &user_time) != TRUE) {
    auto ec = bela::make_system_error_code(L"GetProcessTimes() ");
    bela::FPrintF(stderr, L"summarize time: %s\n", ec);
    return;
  }
  bela::Time zeroTime;
  creationTime = bela::FromFileTime(creation_time);
  exitTime = bela::FromFileTime(exit_time);
  kernelTime = FromKernelTick(kernel_time);
  userTime = FromKernelTick(user_time);
}

void Summarizer::PrintTime() {
  bela::FPrintF(stderr, L"Creation: %s\n", bela::FormatDuration(creationTime - startTime));
  bela::FPrintF(stderr, L"Kernel:   %s\n", bela::FormatDuration(kernelTime));
  bela::FPrintF(stderr, L"User:     %s\n", bela::FormatDuration(userTime));
  bela::FPrintF(stderr, L"Exit:     %s\n", bela::FormatDuration(exitTime - startTime));
  bela::FPrintF(stderr, L"Wall:     %s\n", bela::FormatDuration(endTime - startTime));
}

bool Executor::LookPath(std::wstring_view cmd, std::wstring &file) {
  if (!simulator.LookPath(cmd, file)) {
    return false;
  }
  bela::error_code ec;
  auto realexe = bela::RealPathEx(file, ec);
  if (!realexe) {
    DbgPrint(L"resolve realpath %s %s\n", file, ec);
    return true;
  }
  DbgPrint(L"resolve realpath %s\n", *realexe);
  console = bela::pe::IsSubsystemConsole(*realexe);
  DbgPrint(L"executable %s is subsystem console: %v\n", *realexe, console);
  return true;
}

bool Executor::FindArg0(std::wstring_view arg0, std::wstring &target) {
  if (NameEquals(arg0, L"winsh")) {
    if (auto pwshcore = baulk::pwsh::PwshCore(); !pwshcore.empty()) {
      target.assign(std::move(pwshcore));
      DbgPrint(L"Found installed pwsh %s", target);
      return true;
    }
    if (bela::env::LookPath(L"powershell", target, true)) {
      DbgPrint(L"Found powershell %s", target);
      return true;
    }
    if (bela::env::LookPath(L"cmd", target, true)) {
      DbgPrint(L"Found cmd %s", target);
      return true;
    }
    return false;
  }
  if (NameEquals(arg0, L"pwsh")) {
    if (auto pwshcore = baulk::pwsh::PwshExePath(); !pwshcore.empty()) {
      target.assign(std::move(pwshcore));
      DbgPrint(L"Found installed pwsh %s", target);
      return true;
    }
    return false;
  }
  if (NameEquals(arg0, L"pwsh-preview")) {
    if (auto pwshpreview = baulk::pwsh::PwshCorePreview(); !pwshpreview.empty()) {
      target.assign(std::move(pwshpreview));
      DbgPrint(L"Found installed pwsh-preview %s", target);
      return true;
    }
    return false;
  }
  if (!LookPath(arg0, target)) {
    bela::FPrintF(stderr, L"\x1b[31mbaulk-exec: unable lookup %s in path\n%s\x1b[0m", arg0,
                  bela::StrJoin(simulator.Paths(), L"\n"));
    return false;
  }
  return true;
}

int Executor::Exec() {
  std::wstring target;
  std::wstring_view arg0(argv[0]);
  if (!FindArg0(arg0, target)) {
    return 1;
  }
  DbgPrint(L"target %s subsystem is console: %b", target, console);
  bela::EscapeArgv ea;
  ea.Assign(arg0);
  for (size_t i = 1; i < argv.size(); i++) {
    ea.Append(argv[i]);
  }
  auto env = simulator.MakeEnv();
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (summarizeTime) {
    summarizer.startTime = bela::Now();
  }
  if (CreateProcessW(string_nullable(target), ea.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT,
                     string_nullable(env), string_nullable(cwd), &si, &pi) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::FPrintF(stderr, L"baulk-exec: unable run '%s' error: \x1b[31m%s\x1b[0m\n", arg0, ec);
    return -1;
  }
  CloseHandle(pi.hThread);
  SetConsoleCtrlHandler(nullptr, TRUE);
  auto closer = bela::finally([&] {
    SetConsoleCtrlHandler(nullptr, FALSE);
    CloseHandle(pi.hProcess);
  });
  if (!console) {
    return 0;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  if (summarizeTime) {
    summarizer.endTime = bela::Now();
    summarizer.SummarizeTime(pi.hProcess);
    bela::FPrintF(stderr, L"baulk-exec: run command '\x1b[34m%s\x1b[0m' use time: \n", ea.sv());
    summarizer.PrintTime();
  }
  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  return exitCode;
}

} // namespace baulk::exec