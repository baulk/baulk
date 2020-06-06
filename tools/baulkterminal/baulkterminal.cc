//
#include <bela/base.hpp>
#include <bela/path.hpp>
#include <bela/env.hpp>
#include <bela/escapeargv.hpp>
#include <bela/parseargv.hpp>
#include <bela/picker.hpp>
#include <bela/str_replace.hpp>
#include <shellapi.h>
#include <baulkversion.h>
#include "baulkterminal.hpp"

namespace baulkterminal {

void BaulkMessage() {
  constexpr wchar_t usage[] = LR"(baulkterminal - Baulk Terminal Launcher
Usage: baulkterminal [option] ...
  -h|--help
               Show usage text and quit
  -v|--version
               Show version number and quit
  -C|--cleanup
               Create clean environment variables to avoid interference
  -V|--vs
               Load Visual Studio related environment variables
  -S|--shell
               The shell you want to start. allowed: pwsh, bash, cmd, wsl
  -W|--cwd
               Set the shell startup directory
  --conhost
               Use conhost not Windows terminal
  --clang
               Add Visual Studio's built-in clang to the PATH environment variable
  --manifest
               Baulkterminal startup manifest file
)";
  bela::BelaMessageBox(nullptr, L"Baulk Terminal Launcher", usage, BAULK_APPLINK,
                       bela::mbs_t::ABOUT);
}

bool Executor::ParseArgv(bela::error_code &ec) {
  int Argc = 0;
  auto Argv = CommandLineToArgvW(GetCommandLineW(), &Argc);
  if (Argv == nullptr) {
    ec = bela::make_system_error_code();
    return false;
  }
  auto closer = bela::finally([&] { LocalFree(Argv); });
  bela::ParseArgv pa(Argc, Argv);
  pa.Add(L"help", bela::no_argument, L'h')
      .Add(L"version", bela::no_argument, L'v')
      .Add(L"cleanup", bela::no_argument, L'C') // cleanup environment
      .Add(L"vs", bela::no_argument, L'V')      // load visual studio environment
      .Add(L"shell", bela::required_argument, L'S')
      .Add(L"cwd", bela::required_argument, L'W')
      .Add(L"conhost", bela::no_argument, 1001) // disable windows termainl
      .Add(L"clang", bela::no_argument, 1002)
      .Add(L"manifest", bela::required_argument, 1003);
  auto ret = pa.Execute(
      [this](int val, const wchar_t *oa, const wchar_t *) {
        switch (val) {
        case 'h':
          BaulkMessage();
          ExitProcess(0);
        case 'v':
          bela::BelaMessageBox(nullptr, L"Baulk Terminal Launcher", BAULK_APPVERSION, BAULK_APPLINK,
                               bela::mbs_t::ABOUT);
          ExitProcess(0);
        case 'C':
          cleanup = true;
          break;
        case 'V':
          usevs = true;
          break;
        case 'S':
          shell = oa;
          break;
        case 'W':
          cwd = oa;
          break;
        case 1001:
          conhost = true;
          break;
        case 1002:
          clang = true;
          break;
        case 1003:
          manifest = oa;
          break;
        default:
          break;
        }
        return true;
      },
      ec);
  if (!ret) {
    return false;
  }
  // CommandLineToArgvW BUG
  if (cwd.size() == 3 && isalpha(cwd[0]) && cwd[1] == L':' && cwd[2] == '"') {
    cwd.back() = L'\\';
  }
  return true;
}
} // namespace baulkterminal

std::optional<std::wstring> FindWindowsTerminal() {
  auto wt = bela::ExpandEnv(L"%LOCALAPPDATA%\\Microsoft\\WindowsApps\\wt.exe");
  if (!bela::PathExists(wt)) {
    return std::nullopt;
  }
  return std::make_optional(std::move(wt));
}

class dotcom_global_initializer {
public:
  dotcom_global_initializer() {
    auto hr = CoInitialize(NULL);
    if (FAILED(hr)) {
      auto ec = bela::make_system_error_code();
      MessageBoxW(nullptr, ec.data(), L"CoInitialize", IDOK);
      exit(1);
    }
  }
  ~dotcom_global_initializer() { CoUninitialize(); }
};

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
  dotcom_global_initializer di;
  baulkterminal::Executor executor;
  bela::error_code ec;
  if (!executor.ParseArgv(ec)) {
    bela::BelaMessageBox(nullptr, L"BaulkTerminal: Parse Argv error", ec.data(), BAULK_APPLINKE,
                         bela::mbs_t::FATAL);
    return 1;
  }
  if (!executor.PrepareEnv(ec)) {
    bela::BelaMessageBox(nullptr, L"BaulkTerminal: Prepare env error", ec.data(), BAULK_APPLINKE,
                         bela::mbs_t::FATAL);
    return 1;
  }
  bela::EscapeArgv ea;
  if (!executor.IsConhost()) {
    if (auto wt = FindWindowsTerminal(); wt) {
      ea.Append(*wt);
      if (!executor.Cwd().empty()) {
        ea.Append(L"--startingDirectory");
        ea.Append(executor.Cwd());
      }
    }
  }
  auto shell = executor.MakeShell();
  ea.Append(shell);
  if (bela::EndsWithIgnoreCase(shell, L"bash.exe")) {
    ea.Append(L"-i").Append(L"-l");
  }
  auto env = executor.MakeEnv(ec);
  if (!env) {
    bela::BelaMessageBox(nullptr, L"unable initialize baulkterminal", ec.data(), nullptr,
                         bela::mbs_t::FATAL);
    return 1;
  }
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (CreateProcessW(nullptr, ea.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT,
                     baulkterminal::string_nullable(*env),
                     baulkterminal::string_nullable(executor.Cwd()), &si, &pi) != TRUE) {
    auto ec = bela::make_system_error_code();
    bela::BelaMessageBox(nullptr, L"unable open Windows Terminal", ec.data(), nullptr,
                         bela::mbs_t::FATAL);
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
