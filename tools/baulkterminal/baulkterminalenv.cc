/// baulk terminal vsenv initialize
#include <bela/io.hpp>
#include <bela/process.hpp> // run vswhere
#include <bela/path.hpp>
#include <bela/match.hpp>
#include <bela/strip.hpp>
#include <filesystem>
#include <regutils.hpp>
#include <jsonex.hpp>
#include <baulkenv.hpp>
#include <pwsh.hpp>
#include "baulkterminal.hpp"

namespace baulkterminal {

inline bool NameEquals(std::wstring_view arg, std::wstring_view exe) {
  auto argexe = bela::StripSuffix(arg, L".exe");
  return bela::EqualsIgnoreCase(argexe, exe);
}

bool UseShell(std::wstring_view shell, bela::EscapeArgv &ea) {
  if (shell.empty()) {
    ea.Append(L"winsh");
    DbgPrint(L"Use winsh");
    return true;
  }
  if (NameEquals(shell, L"pwsh")) {
    // PowerShell 7 or PowerShell 5
    if (auto pwsh = baulk::pwsh::PwshExePath(); !pwsh.empty()) {
      ea.Append(pwsh);
      DbgPrint(L"Use pwsh: %s", pwsh);
      return true;
    }
  }
  if (NameEquals(shell, L"pwsh-preview")) {
    if (auto pwshpreview = baulk::pwsh::PwshCorePreview(); !pwshpreview.empty()) {
      ea.Append(pwshpreview);
      DbgPrint(L"Use pwsh-preview: %s", pwshpreview);
      return true;
    }
  }
  if (NameEquals(shell, L"bash")) {
    // git for windows
    bela::error_code ec;
    if (auto gw = baulk::regutils::GitForWindowsInstallPath(ec); gw) {
      if (auto bash = bela::StringCat(*gw, L"\\bin\\bash.exe"); bela::PathExists(bash)) {
        ea.Append(bash).Append(L"-i").Append(L"-l");
        DbgPrint(L"Use bash: %s", bash);
        return true;
      }
    }
  }
  if (NameEquals(shell, L"wsl")) {
    ea.Append(L"wsl.exe");
    DbgPrint(L"Use wsl.exe");
    return true;
  }
  if (!shell.empty()) {
    ea.Append(shell);
    DbgPrint(L"Use custom shell: %s", shell);
    return true;
  }
  ea.Append(L"cmd.exe");
  DbgPrint(L"Use fallback shell: cmd");
  return false;
}

template <size_t Len = 256> std::wstring GetCwd() {
  std::wstring s;
  s.resize(Len);
  auto len = GetCurrentDirectoryW(Len, s.data());
  if (len == 0) {
    return L"";
  }
  if (len < Len) {
    s.resize(len);
    return s;
  }
  s.resize(len);
  auto nlen = GetCurrentDirectoryW(len, s.data());
  if (nlen == 0 || nlen > len) {
    return L"";
  }
  s.resize(nlen);
  return s;
}

inline std::optional<std::wstring> FindWindowsTerminal() {
  auto wt = bela::WindowsExpandEnv(L"%LOCALAPPDATA%\\Microsoft\\WindowsApps\\wt.exe");
  if (!bela::PathExists(wt)) {
    return std::nullopt;
  }
  return std::make_optional(std::move(wt));
}

std::optional<std::wstring> SearchBaulkExec(bela::error_code &ec) {
  auto exepath = bela::ExecutableFinalPathParent(ec);
  if (!exepath) {
    return std::nullopt;
  }
  auto baulkexec = bela::StringCat(*exepath, L"\\baulk-exec.exe");
  if (bela::PathFileIsExists(baulkexec)) {
    return std::make_optional(std::move(baulkexec));
  }
  std::wstring bkroot(*exepath);
  for (size_t i = 0; i < 5; i++) {
    auto baulkexec = bela::StringCat(bkroot, L"\\bin\\baulk-exec.exe");
    if (bela::PathFileIsExists(baulkexec)) {
      return std::make_optional(std::move(baulkexec));
    }
    bela::PathStripName(bkroot);
  }
  ec = bela::make_error_code(bela::ErrGeneral, L"unable found baulk.exe");
  return std::nullopt;
}

void ApplyBaulkNewExec(std::wstring_view baulkexec) {
  auto dir = bela::DirName(baulkexec);
  auto baulkexecnew = bela::StringCat(dir, L"\\baulk-exec.new");
  if (bela::PathFileIsExists(baulkexecnew)) {
    DbgPrint(L"Found baulk-exec.new: %s trace apply it", baulkexecnew);
    if (MoveFileExW(baulkexecnew.data(), baulkexec.data(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) != TRUE) {
      auto ec = bela::make_system_error_code();
      DbgPrint(L"Apply new baulk-exec error: %s", ec.message);
    }
  }
  auto baulkexecdel = bela::StringCat(dir, L"\\baulk-exec.del");
  DeleteFileW(baulkexecdel.data());
}

bool Executor::PrepareArgv(bela::EscapeArgv &ea, bela::error_code &ec) {
  if (!conhost) {
    DbgPrint(L"Turn off conhost, Try to find Windows Terminal");
    if (auto wt = FindWindowsTerminal(); wt) {
      ea.Assign(*wt).Append(L"--title").Append(L"Windows Terminal \U0001F496 Baulk").Append(L"--");
      DbgPrint(L"Found Windows Terminal: %s", *wt);
    }
  }
  auto baulkexec = SearchBaulkExec(ec);
  if (!baulkexec) {
    return false;
  }
  DbgPrint(L"Found baulk-exec: %s", *baulkexec);
  ApplyBaulkNewExec(*baulkexec);
  ea.Append(*baulkexec);
  if (IsDebugMode) {
    ea.Append(L"-V");
  }
  if (cleanup) {
    ea.Append(L"--cleanup");
    DbgPrint(L"Turn on cleanup env");
  }
  if (usevspreview) {
    ea.Append(L"--vs-preview");
    DbgPrint(L"Turn on vs preview env");
    if (!arch.empty()) {
      ea.Append(L"-A").Append(arch);
      DbgPrint(L"Select arch: %s", arch);
    }
    if (clang) {
      ea.Append(L"--clang");
      DbgPrint(L"Turn on clang env");
    }
  } else if (usevs) {
    ea.Append(L"--vs");
    DbgPrint(L"Turn on vs env");
    if (!arch.empty()) {
      ea.Append(L"-A").Append(arch);
      DbgPrint(L"Select arch: %s", arch);
    }
    if (clang) {
      ea.Append(L"--clang");
      DbgPrint(L"Turn on clang env");
    }
  }
  if (!cwd.empty()) {
    ea.Append(L"-W").Append(cwd);
    DbgPrint(L"CWD: %s", cwd);
  }
  for (const auto &e : venvs) {
    ea.Append(L"-E").Append(e);
    DbgPrint(L"Enable virtual env: %s", e);
  }
  UseShell(shell, ea);
  return true;
}

} // namespace baulkterminal