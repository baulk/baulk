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
  if (shell.empty() || NameEquals(shell, L"pwsh")) {
    if (auto pwsh = baulk::pwsh::PwshExePath(); !pwsh.empty()) {
      ea.Append(pwsh);
      return true;
    }
  }
  if (NameEquals(shell, L"pwsh-preview")) {
    if (auto pwshpreview = baulk::pwsh::PwshCorePreview(); !pwshpreview.empty()) {
      ea.Append(pwshpreview);
      return true;
    }
  }
  if (NameEquals(shell, L"bash")) {
    // git for windows
    bela::error_code ec;
    if (auto gw = baulk::regutils::GitForWindowsInstallPath(ec); gw) {
      if (auto bash = bela::StringCat(*gw, L"\\bin\\bash.exe"); bela::PathExists(bash)) {
        ea.Append(bash).Append(L"-i").Append(L"-l");
        return true;
      }
    }
  }
  if (NameEquals(shell, L"wsl")) {
    ea.Append(L"wsl.exe");
    return true;
  }
  if (!shell.empty()) {
    ea.Append(shell);
    return true;
  }
  ea.Append(L"cmd.exe");
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
  ec = bela::make_error_code(1, L"unable found baulk.exe");
  return std::nullopt;
}

void ApplyBaulkNewExec(std::wstring_view baulkexec) {
  auto dir = bela::DirName(baulkexec);
  auto baulkexecnew = bela::StringCat(dir, L"\\baulk-exec.new");
  if (bela::PathFileIsExists(baulkexecnew)) {
    if (MoveFileExW(baulkexecnew.data(), baulkexec.data(),
                    MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) != TRUE) {
    }
  }
}

bool Executor::PrepareArgv(bela::EscapeArgv &ea, bela::error_code &ec) {
  if (!conhost) {
    if (auto wt = FindWindowsTerminal(); wt) {
      ea.Assign(*wt).Append(L"--");
    }
  }
  auto baulkexec = SearchBaulkExec(ec);
  if (!baulkexec) {
    return false;
  }
  ApplyBaulkNewExec(*baulkexec);
  ea.Append(*baulkexec);
  if (cleanup) {
    ea.Append(L"--cleanup");
  }
  if (usevs) {
    ea.Append(L"--vs");
    if (!arch.empty()) {
      ea.Append(L"-A").Append(arch);
    }
    if (clang) {
      ea.Append(L"--clang");
    }
  }
  if (!cwd.empty()) {
    ea.Append(L"-W").Append(cwd);
  }
  for (const auto &e : venvs) {
    ea.Append(L"-E").Append(e);
  }
  UseShell(shell, ea);
  return true;
}

} // namespace baulkterminal