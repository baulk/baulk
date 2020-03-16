//
#include <bela/base.hpp>
#include <bela/path.hpp>
#include <bela/env.hpp>
#include <bela/escapeargv.hpp>
#include <bela/finaly.hpp>
#include <bela/picker.hpp>

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

std::optional<std::wstring> FindWindowsTerminal() {
  auto wt = bela::ExpandEnv(L"%LOCALAPPDATA%\\Microsoft\\WindowsApps\\wt.exe");
  if (!bela::PathExists(wt)) {
    return std::nullopt;
  }
  return std::make_optional(std::move(wt));
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
  auto pwshexe = PwshExePath();
  if (pwshexe.empty()) {
    bela::BelaMessageBox(nullptr, L"unable open Windows Terminal",
                         L"no windows terminal installed", nullptr,
                         bela::mbs_t::FATAL);
    return 1;
  }
  bela::error_code ec;
  auto exepath = bela::ExecutablePath(ec);
  if (!exepath) {
    bela::BelaMessageBox(nullptr, L"unable open Windows Terminal", ec.data(),
                         nullptr, bela::mbs_t::FATAL);
    return 1;
  }
  auto targetFile = bela::StringCat(*exepath, L"\\script\\baulk.ps1");
  if (!bela::PathExists(targetFile)) {
    bela::PathStripName(*exepath);
    targetFile = bela::StringCat(*exepath, L"\\script\\baulk.ps1");
    if (!bela::PathExists(targetFile)) {
      bela::BelaMessageBox(nullptr, L"unable find baulk.ps1",
                           L"please check $prefix baulk.ps1", nullptr,
                           bela::mbs_t::FATAL);
      return 1;
    }
  }
  bela::EscapeArgv ea;
  auto wt = FindWindowsTerminal();
  if (wt) {
    ea.Append(*wt);
  }
  ea.Append(pwshexe)
      .Append(L"-NoLogo")
      .Append(L"-NoExit")
      .Append(L"-File")
      .Append(targetFile);
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