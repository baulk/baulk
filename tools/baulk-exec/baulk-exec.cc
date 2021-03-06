///
#include <bela/terminal.hpp>
#include <bela/parseargv.hpp>
#include <bela/process.hpp>
#include <bela/subsitute.hpp>
#include <bela/strip.hpp>
#include <bela/time.hpp>
#include <bela/pe.hpp>
#include <baulkenv.hpp>
#include <baulkrev.hpp>
#include <pwsh.hpp>
#include "baulk-exec.hpp"

namespace baulk::exec {
bool IsDebugMode = false;
// Usage
void Usage() {
  constexpr std::wstring_view usage = LR"(baulk-exec - Baulk extend executor
Usage: baulk-exec [option] <command> [<args>] ...
  -h|--help            Show usage text and quit
  -v|--version         Show version number and quit
  -V|--verbose         Make the operation more talkative
  -C|--cleanup         Create clean environment variables to avoid interference
  -W|--cwd             Set the command startup directory
  -A|--arch            Select a specific arch, use native architecture by default
  -E|--venv            Choose to load a specific package virtual environment
  --vs                 Load Visual Studio related environment variables
  --vs-preview         Load Visual Studio (Preview) related environment variables
  --clang              Add Visual Studio's built-in clang to the PATH environment variable
  --time               Summarize command system resource usage

Example:
  baulk-exec -V --vs TUNNEL_DEBUG=1 pwsh

Built-in alias:
  winsh          A fake shell. It may be pwsh or powershell and cmd.
  pwsh           PowerShell Core
  pwsh-preview   PowerShell Core Preview

)";
  bela::terminal::WriteAuto(stderr, usage);
}

void Version() {
  bela::FPrintF(stdout, L"baulk-exec %s\nRelease:    %s\nCommit:     %s\nBuild Time: %s\n", BAULK_VERSION,
                BAULK_REFNAME, BAULK_REVISION, BAULK_BUILD_TIME);
}

inline bela::Duration FromKernelTick(FILETIME ft) {
  auto tick = std::bit_cast<int64_t, FILETIME>(ft);
  return bela::Microseconds(tick / 10);
}

struct Summarizer {
  bela::Time startTime;
  bela::Time endTime;
  bela::Time creationTime;
  bela::Time exitTime;
  bela::Duration kernelTime;
  bela::Duration userTime;
  void SummarizeTime(HANDLE hProcess) {
    FILETIME creation_time{0};
    FILETIME exit_time{0};
    FILETIME kernel_time{0};
    FILETIME user_time{0};
    if (GetProcessTimes(hProcess, &creation_time, &exit_time, &kernel_time, &user_time) != TRUE) {
      auto ec = bela::make_system_error_code(L"GetProcessTimes() ");
      bela::FPrintF(stderr, L"summarize time: %s\n", ec.message);
      return;
    }
    bela::Time zeroTime;
    creationTime = bela::FromFileTime(creation_time);
    exitTime = bela::FromFileTime(exit_time);
    kernelTime = FromKernelTick(kernel_time);
    userTime = FromKernelTick(user_time);
  }
  void PrintTime() {
    bela::FPrintF(stderr, L"Creation: %s\n", bela::FormatDuration(creationTime - startTime));
    bela::FPrintF(stderr, L"Kernel:   %s\n", bela::FormatDuration(kernelTime));
    bela::FPrintF(stderr, L"User:     %s\n", bela::FormatDuration(userTime));
    bela::FPrintF(stderr, L"Exit:     %s\n", bela::FormatDuration(exitTime - startTime));
    bela::FPrintF(stderr, L"Wall:     %s\n", bela::FormatDuration(endTime - startTime));
  }
};

class Executor {
public:
  Executor() = default;
  Executor(const Executor &) = delete;
  Executor &operator=(const Executor &) = delete;
  bool ParseArgv(int argc, wchar_t **argv);
  int Exec();

private:
  bela::env::Simulator simulator;
  argv_t argv;
  std::wstring cwd;
  bool cleanup{false};
  bool console{true};
  bool summarizeTime{false};
  Summarizer summarizer;
  bool LookPath(std::wstring_view exe, std::wstring &file);
  bool FindArg0(std::wstring_view arg0, std::wstring &target);
};

bool Executor::LookPath(std::wstring_view cmd, std::wstring &file) {
  if (!simulator.LookPath(cmd, file)) {
    return false;
  }
  bela::error_code ec;
  auto realexe = bela::RealPathEx(file, ec);
  if (!realexe) {
    DbgPrint(L"resolve realpath %s %s\n", file, ec.message);
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
    bela::FPrintF(stderr, L"unable run '%s' error: \x1b[31m%s\x1b[0m\n", arg0, ec.message);
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
    bela::FPrintF(stderr, L"run command '\x1b[34m%s\x1b[0m' use time: \n", ea.sv());
    summarizer.PrintTime();
  }
  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  return exitCode;
}

// ParseArgv todo
bool Executor::ParseArgv(int argc, wchar_t **cargv) {
  bela::ParseArgv pa(argc, cargv, true);
  pa.Add(L"help", bela::no_argument, L'h')
      .Add(L"version", bela::no_argument, L'v')
      .Add(L"verbose", bela::no_argument, L'V')
      .Add(L"cleanup", bela::no_argument, L'C') // cleanup environment
      .Add(L"arch", bela::required_argument, L'A')
      .Add(L"cwd", bela::required_argument, L'W')
      .Add(L"venv", bela::required_argument, L'E')
      .Add(L"vs", bela::no_argument, 1000) // load visual studio environment
      .Add(L"vs-preview", bela::no_argument, 1001)
      .Add(L"clang", bela::no_argument, 1002)
      .Add(L"time", bela::no_argument, 1004);
  bool usevs = false;
  bool usevspreview = false;
  bool clang = false;
  std::wstring arch;
  std::vector<std::wstring> venvs;
  bela::error_code ec;
  auto ret = pa.Execute(
      [&](int val, const wchar_t *oa, const wchar_t *) {
        switch (val) {
        case 'h':
          Usage();
          exit(0);
        case 'v':
          Version();
          exit(0);
        case 'C':
          cleanup = true;
          break;
        case 'V':
          IsDebugMode = true;
          break;
        case 'W':
          cwd = oa;
          break;
        case 'A': {
          auto larch = bela::AsciiStrToLower(oa);
          if (larch == L"x64" || larch == L"arm64" || larch == L"x86" || larch == L"arm") {
            arch = larch;
            break;
          }
          bela::FPrintF(stderr, L"baulk: invalid arch '%s'\n", oa);
        } break;
        case 'E':
          venvs.push_back(oa);
          break;
        case 1000:
          if (!usevspreview) {
            usevs = true;
          }
          break;
        case 1001:
          if (!usevs) {
            usevspreview = true;
          }
          break;
        case 1002:
          clang = true;
          break;
        case 1004:
          summarizeTime = true;
          break;
        default:
          break;
        }
        return true;
      },
      ec);
  if (!ret) {
    bela::FPrintF(stderr, L"ParseArgv: \x1b[31m%s\x1b[0m\n", ec.message);
    return false;
  }
  const auto &Argv = pa.UnresolvedArgs();
  if (pa.UnresolvedArgs().empty()) {
    bela::FPrintF(stderr, L"no command input\n");
    Usage();
    exit(1);
  }
  if (cleanup) {
    DbgPrint(L"use cleaned env");
    simulator.InitializeCleanupEnv();
  } else {
    simulator.InitializeEnv();
  }

  for (size_t i = 0; i < Argv.size(); i++) {
    const auto arg = Argv[i];
    // support KEY=VALUE env setter
    auto pos = arg.find(L'=');
    if (pos == std::wstring::npos) {
      // FIND ARGS
      for (size_t j = i; j < Argv.size(); j++) {
        argv.emplace_back(Argv[j]);
      }
      // BREAK ARGS PARSE
      break;
    }
    simulator.SetEnv(arg.substr(0, pos), arg.substr(pos + 1));
  }

  baulk::env::Searcher searcher(simulator, arch);
  searcher.IsDebugMode = baulk::exec::IsDebugMode;
  if (!searcher.InitializeBaulk(ec)) {
    bela::FPrintF(stderr, L"InitializeBaulk failed %s\n", ec.message);
    return false;
  }
  searcher.InitializeGit(cleanup, ec);
  // initialize vs env
  // We maintain a tolerant attitude towards initializing the Visual Studio environment. If the initialization fails,
  // the baulk-exec operation will not be suspended.
  auto vsInitialize = [&]() {
    if (!usevs) {
      DbgPrint(L"Turn off visual studio env");
      return false;
    }
    DbgPrint(L"Turn on visual studio env");
    if (!searcher.InitializeVisualStudioEnv(false, clang, ec)) {
      bela::FPrintF(stderr, L"Initialize visual studio env failed %s\n", ec.message);
      return false;
    }
    if (!searcher.InitializeWindowsKitEnv(ec)) {
      bela::FPrintF(stderr, L"Initialize Windows Kit (SDK) env failed %s\n", ec.message);
      return false;
    }
    return true;
  };
  auto vsPreviewInitialize = [&]() {
    if (!usevspreview) {
      DbgPrint(L"Turn off visual studio (Preview) env");
      return false;
    }
    DbgPrint(L"Turn on visual studio (Preview) env");
    if (!searcher.InitializeVisualStudioEnv(true, clang, ec)) {
      bela::FPrintF(stderr, L"Initialize visual studio (Preview) env failed %s\n", ec.message);
      return false;
    }
    if (!searcher.InitializeWindowsKitEnv(ec)) {
      bela::FPrintF(stderr, L"Initialize Windows Kit (SDK) env failed %s\n", ec.message);
      return false;
    }
    return true;
  };

  if (vsInitialize()) {
    DbgPrint(L"Initialize visual studio env done");
  }
  if (vsPreviewInitialize()) {
    DbgPrint(L"Initialize visual studio (Preview) env done");
  }
  if (!searcher.InitializeVirtualEnv(venvs, ec)) {
    bela::FPrintF(stderr, L"parse venv: \x1b[31m%s\x1b[0m\n", ec.message);
  }
  if (IsDebugMode && !searcher.availableEnv.empty()) {
    auto as = bela::StrJoin(searcher.availableEnv, L" ");
    DbgPrint(L"Turn on venv: %s", as);
  }
  searcher.FlushEnv();
  return true;
}
} // namespace baulk::exec

int wmain(int argc, wchar_t **argv) {
  baulk::exec::Executor executor;
  if (!executor.ParseArgv(argc, argv)) {
    return 1;
  }
  return executor.Exec();
}
