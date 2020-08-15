///
#include <bela/terminal.hpp>
#include <bela/parseargv.hpp>
#include <bela/process.hpp>
#include <bela/subsitute.hpp>
#include <bela/strip.hpp>
#include <bela/pe.hpp>
#include <baulkenv.hpp>
#include <baulkrev.hpp>
#include <pwsh.hpp>
#include "baulk-exec.hpp"

namespace baulk::exec {
bool IsDebugMode = false;
//

void Usage() {
  constexpr std::wstring_view usage = LR"(baulk-exec - Baulk extend executor
Usage: baulk-exec [option] command args ...
  -h|--help        Show usage text and quit
  -v|--version     Show version number and quit
  -V|--verbose     Make the operation more talkative
  -C|--cleanup     Create clean environment variables to avoid interference
  -W|--cwd         Set the command startup directory
  -A|--arch        Select a specific arch, use native architecture by default
  -E|--venv        Choose to load a specific package virtual environment
  --vs             Load Visual Studio related environment variables
  --clang          Add Visual Studio's built-in clang to the PATH environment variable

example:
  baulk-exec -V --vs TUNNEL_DEBUG=1 pwsh

)";
  bela::terminal::WriteAuto(stderr, usage);
}

void Version() {
  bela::FPrintF(stdout, L"baulk %s\nRelease:    %s\nCommit:     %s\nBuild Time: %s\n",
                BAULK_RELEASE_VERSION, BAULK_RELEASE_NAME, BAULK_RELEASE_COMMIT, BAULK_BUILD_TIME);
}

std::wstring AvailableEnvTitle(const std::vector<std::wstring> &venvs) {
  constexpr std::wstring_view bk = L"Baulk Terminal \U0001F496";
  if (venvs.empty()) {
    return std::wstring(bk);
  }
  auto firstenv = venvs[0];
  if (venvs.size() > 1) {
    return bela::StringCat(bk, L" [", firstenv, L"...+", venvs.size() - 1, L"]");
  }
  return bela::StringCat(bk, L" [", firstenv, L"]");
}

class TitleManager {
public:
  TitleManager() = default;
  ~TitleManager() {
    if (!title.empty()) {
      SetConsoleTitleW(title.data());
    }
  }
  bool UpdateTitle(std::wstring_view newtitle) {
    wchar_t buffer[512] = {0};
    auto n = GetConsoleTitleW(buffer, 512);
    if (n == 0 || n >= 512) {
      return false;
    }
    SetConsoleTitleW(newtitle.data());
    title = buffer;
    return true;
  }

private:
  std::wstring title;
};

class Executor {
public:
  Executor() = default;
  Executor(const Executor &) = delete;
  Executor &operator=(const Executor &) = delete;
  bool ParseArgv(int argc, wchar_t **argv, TitleManager &tm);
  int Exec();

private:
  bela::env::Simulator simulator;
  argv_t argv;
  std::wstring cwd;
  bool cleanup{false};
  bool console{true};
  bool LookupPath(std::wstring_view exe, std::wstring &file);
};

bool Executor::LookupPath(std::wstring_view cmd, std::wstring &file) {
  if (!simulator.LookupPath(cmd, file)) {
    return false;
  }
  bela::error_code ec;
  auto realexe = bela::RealPathEx(file, ec);
  if (!realexe) {
    DbgPrint(L"resolve realpath %s %s\n", file, ec.message);
    return true;
  }
  DbgPrint(L"resolve realpath %s\n", *realexe);
  if (auto pe = bela::pe::Expose(*realexe, ec); pe) {
    console = (pe->subsystem == bela::pe::Subsystem::CUI);
  }
  return true;
}

inline bool IsPwshTarget(std::wstring_view arg0, std::wstring &target) {
  if (NameEquals(arg0, L"pwsh")) {
    if (auto pwshcore = baulk::pwsh::PwshCore(); !pwshcore.empty()) {
      target.assign(std::move(pwshcore));
      DbgPrint(L"Found installed pwsh %s", target);
      return true;
    }
  }
  if (NameEquals(arg0, L"pwsh-preview")) {
    if (auto pwshpreview = baulk::pwsh::PwshCorePreview(); !pwshpreview.empty()) {
      target.assign(std::move(pwshpreview));
      DbgPrint(L"Found installed pwsh-preview %s", target);
      return true;
    }
  }
  return false;
}

int Executor::Exec() {
  std::wstring target;
  std::wstring_view arg0(argv[0]);
  if (!cleanup || !IsPwshTarget(arg0, target)) {
    if (!LookupPath(arg0, target)) {
      bela::FPrintF(stderr, L"\x1b[31mbaulk-exec: unable lookup %s in path\n%s\x1b[0m", arg0,
                    bela::StrJoin(simulator.Paths(), L"\n"));
      return 1;
    }
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
  if (CreateProcessW(string_nullable(target), ea.data(), nullptr, nullptr, FALSE,
                     CREATE_UNICODE_ENVIRONMENT, string_nullable(env), string_nullable(cwd), &si,
                     &pi) != TRUE) {
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
  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  return exitCode;
}

// ParseArgv todo
bool Executor::ParseArgv(int argc, wchar_t **cargv, TitleManager &tm) {
  bela::ParseArgv pa(argc, cargv, true);
  pa.Add(L"help", bela::no_argument, L'h')
      .Add(L"version", bela::no_argument, L'v')
      .Add(L"verbose", bela::no_argument, L'V')
      .Add(L"cleanup", bela::no_argument, L'C') // cleanup environment
      .Add(L"arch", bela::required_argument, L'A')
      .Add(L"cwd", bela::required_argument, L'W')
      .Add(L"venv", bela::required_argument, L'E')
      .Add(L"vs", bela::no_argument, 1001) // load visual studio environment
      .Add(L"clang", bela::no_argument, 1002);
  bool usevs = false;
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
        case 1001:
          usevs = true;
          break;
        case 1002:
          clang = true;
          break;
        default:
          break;
        }
        return true;
      },
      ec);
  if (!ret) {
    bela::FPrintF(stderr, L"ParseArgv: %s\n", ec.message);
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
    auto pos = arg.find(L'=');
    if (pos == std::wstring::npos) {
      for (size_t j = i; j < Argv.size(); j++) {
        argv.emplace_back(Argv[j]);
      }
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
  if (usevs) {
    DbgPrint(L"visual studio env enabled");
    if (!searcher.InitializeVisualStudioEnv(clang, ec)) {
      bela::FPrintF(stderr, L"InitializeVisualStudioEnv failed %s\n", ec.message);
      return false;
    }
    if (!searcher.InitializeWindowsKitEnv(ec)) {
      bela::FPrintF(stderr, L"InitializeWindowsKitEnv failed %s\n", ec.message);
      return false;
    }
  }
  searcher.InitializeVirtualEnv(venvs, ec);
  if (ec) {
    bela::FPrintF(stderr, L"parse venv: \x1b[31m%s\x1b[0m\n", ec.message);
  }
  if (IsDebugMode && !searcher.availableEnv.empty()) {
    auto as = bela::StrJoin(searcher.availableEnv, L" ");
    DbgPrint(L"Turn on venv: %s", as);
  }
  searcher.FlushEnv();
  auto newtitle = AvailableEnvTitle(searcher.availableEnv);
  tm.UpdateTitle(newtitle);
  return true;
}
} // namespace baulk::exec

int wmain(int argc, wchar_t **argv) {
  baulk::exec::Executor executor;
  baulk::exec::TitleManager tm;
  if (!executor.ParseArgv(argc, argv, tm)) {
    return 1;
  }
  return executor.Exec();
}
