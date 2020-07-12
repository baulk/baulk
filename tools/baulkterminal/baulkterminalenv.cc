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

std::wstring Executor::MakeShell() const {
  if (shell.empty() || NameEquals(shell, L"pwsh")) {
    if (auto pwsh = baulk::pwsh::PwshExePath(); !pwsh.empty()) {
      return pwsh;
    }
  }
  if (NameEquals(shell, L"pwsh-preview")) {
    if (auto pwshpreview = baulk::pwsh::PwshCorePreview(); !pwshpreview.empty()) {
      return pwshpreview;
    }
  }
  if (NameEquals(shell, L"bash")) {
    // git for windows
    bela::error_code ec;
    if (auto gw = baulk::regutils::GitForWindowsInstallPath(ec); gw) {
      if (auto bash = bela::StringCat(*gw, L"\\bin\\bash.exe"); bela::PathExists(bash)) {
        return bash;
      }
    }
  }
  if (NameEquals(shell, L"wsl")) {
    return L"wsl.exe";
  }
  if (!shell.empty() && bela::PathExists(shell)) {
    return shell;
  }
  return L"cmd.exe";
}

bool Executor::InitializeBaulkEnv(bela::error_code &ec) {
  FILE *fd = nullptr;
  if (auto eo = _wfopen_s(&fd, manifest.data(), L"rb"); eo != 0) {
    ec = bela::make_stdc_error_code(eo);
    return false;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  try {
    auto obj = nlohmann::json::parse(fd);
    if (auto it = obj.find("env"); it != obj.end()) {
      for (const auto &kv : it.value().items()) {
        auto value = bela::ToWide(kv.value().get<std::string_view>());
        dev.SetEnv(bela::ToWide(kv.key()), bela::ExpandEnv(value), true);
      }
    }
    baulk::json::JsonAssignor ja(obj);
    if (!clang.initialized) {
      clang = ja.boolean("clang");
    }
    if (!usevs.initialized) {
      usevs = ja.boolean("usevs");
    }
    if (!cleanup.initialized) {
      cleanup = ja.boolean("cleanup");
    }
    if (!conhost.initialized) {
      conhost = ja.boolean("conshot");
    }
  } catch (const std::exception &e) {
    ec = bela::make_error_code(1, bela::ToWide(e.what()));
    return false;
  }
  return true;
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

bool Executor::PrepareEnv(bela::error_code &ec) {
  if (!manifest.empty() && !InitializeBaulkEnv(ec)) {
    return false;
  }
  if (cwd.empty()) {
    cwd = GetCwd();
  }
  return true;
}

std::optional<std::wstring> Executor::MakeEnv(bela::error_code &ec) {
  baulk::env::Searcher searcher(dev, arch);
  if (!searcher.InitializeBaulk(ec)) {
    return std::nullopt;
  }
  searcher.InitializeGit(cleanup(), ec);
  if (usevs) {
    if (!searcher.InitializeVisualStudioEnv(clang(), ec)) {
      return std::nullopt;
    }
    if (!searcher.InitializeWindowsKitEnv(ec)) {
      return std::nullopt;
    }
  }
  searcher.InitializeVirtualEnv(venvs, ec);
  if (cleanup) {
    return std::make_optional(searcher.CleanupEnv());
  }
  return std::make_optional(searcher.MakeEnv());
}

} // namespace baulkterminal