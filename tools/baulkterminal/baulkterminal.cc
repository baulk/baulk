//
#include <bela/base.hpp>
#include <bela/path.hpp>
#include <bela/env.hpp>
#include <bela/escapeargv.hpp>
#include <bela/parseargv.hpp>
#include <bela/picker.hpp>
#include <bela/str_replace.hpp>
#include <shellapi.h>
#include "baulkterminal.hpp"
#include "../baulk/version.h"

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
)";
  bela::BelaMessageBox(nullptr, L"Baulk Terminal Launcher", usage,
                       BAULK_APPLINK, bela::mbs_t::ABOUT);
}

//
struct Options {
  bool cleanup{false};
  bool enablevs{false};
  bool conhost{false};
  bool clang{false};
  std::wstring shell;
  std::wstring cwd;
  bool Parse(bela::error_code &ec);
  std::wstring BaulkShell();
};

bool Options::Parse(bela::error_code &ec) {
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
      .Add(L"vs", bela::no_argument, L'V') // load visual studio environment
      .Add(L"shell", bela::required_argument, L'S')
      .Add(L"cwd", bela::required_argument, L'W')
      .Add(L"conhost", bela::no_argument, 1001)
      .Add(L"clang", bela::no_argument, 1002); // disable windows termainl
  auto result = pa.Execute(
      [this](int val, const wchar_t *oa, const wchar_t *) {
        switch (val) {
        case 'h':
          BaulkMessage();
          ExitProcess(0);
        case 'v':
          bela::BelaMessageBox(nullptr, L"Baulk Terminal Launcher",
                               BAULK_APPVERSION, BAULK_APPLINK,
                               bela::mbs_t::ABOUT);
          ExitProcess(0);
        case 'C':
          cleanup = true;
          break;
        case 'V':
          enablevs = true;
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
        default:
          break;
        }
        return true;
      },
      ec);
  if (!result) {
    return false;
  }
  return true;
}

bool LookupPwshCore(std::wstring &ps) {
  bool success = false;
  auto psdir = bela::ExpandEnv(L"%ProgramFiles%\\Powershell");
  if (!bela::PathExists(psdir)) {
    psdir = bela::ExpandEnv(L"%ProgramW6432%\\Powershell");
    if (!bela::PathExists(psdir)) {
      return false;
    }
  }
  WIN32_FIND_DATAW wfd;
  auto findstr = bela::StringCat(psdir, L"\\*");
  HANDLE hFind = FindFirstFileW(findstr.c_str(), &wfd);
  if (hFind == INVALID_HANDLE_VALUE) {
    return false; /// Not found
  }
  do {
    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      auto pscore = bela::StringCat(psdir, L"\\", wfd.cFileName, L"\\pwsh.exe");
      if (bela::PathExists(pscore)) {
        ps.assign(std::move(pscore));
        success = true;
        break;
      }
    }
  } while (FindNextFileW(hFind, &wfd));
  FindClose(hFind);
  return success;
}

bool LookupPwshDesktop(std::wstring &ps) {
  WCHAR pszPath[MAX_PATH]; /// by default , System Dir Length <260
  // https://docs.microsoft.com/en-us/windows/desktop/api/sysinfoapi/nf-sysinfoapi-getsystemdirectoryw
  auto N = GetSystemDirectoryW(pszPath, MAX_PATH);
  if (N == 0) {
    return false;
  }
  pszPath[N] = 0;
  ps = bela::StringCat(pszPath, L"\\WindowsPowerShell\\v1.0\\powershell.exe");
  return true;
}

std::wstring PwshExePath() {
  std::wstring pwshexe;
  if (LookupPwshCore(pwshexe)) {
    return pwshexe;
  }
  if (LookupPwshDesktop(pwshexe)) {
    return pwshexe;
  }
  return L"";
}

std::wstring Options::BaulkShell() {
  if (shell.empty() || bela::EqualsIgnoreCase(L"pwsh", shell)) {
    return PwshExePath();
  }
  if (bela::EqualsIgnoreCase(L"bash", shell)) {
    // git for windows
  }
  if (bela::EqualsIgnoreCase(L"wsl", shell)) {
    //
    return L"wsl.exe";
  }
  return L"cmd.exe";
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
  baulkterminal::Options opt;
  bela::error_code ec;
  if (!opt.Parse(ec)) {
    bela::BelaMessageBox(nullptr, L"BaulkTerminal: Parse Argv error", ec.data(),
                         BAULK_APPLINKE, bela::mbs_t::FATAL);
    return 1;
  }
  bela::EscapeArgv ea;

  if (!opt.conhost) {
    if (auto wt = FindWindowsTerminal(); wt) {
      ea.Append(*wt);
      if (!opt.cwd.empty()) {
        ea.Append(L"--startingDirectory");
        ea.Append(opt.cwd);
      }
    }
  }
  ea.Append(opt.BaulkShell());
  auto env = baulkterminal::MakeEnv(opt.enablevs, opt.clang, opt.cleanup, ec);
  if (!env) {
    bela::BelaMessageBox(nullptr, L"unable initialize baulkterminal", ec.data(),
                         nullptr, bela::mbs_t::FATAL);
    return 1;
  }
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  SecureZeroMemory(&si, sizeof(si));
  SecureZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  if (CreateProcessW(
          nullptr, ea.data(), nullptr, nullptr, FALSE,
          CREATE_UNICODE_ENVIRONMENT, baulkterminal::string_nullable(*env),
          baulkterminal::string_nullable(opt.cwd), &si, &pi) != TRUE) {
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
