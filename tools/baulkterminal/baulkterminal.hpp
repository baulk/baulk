//
#ifndef BAULKTERMINAL_HPP
#define BAULKTERMINAL_HPP
#pragma once
#include <bela/base.hpp>
#include <bela/simulator.hpp>
#include <vector>

namespace baulkterminal {
constexpr const wchar_t *string_nullable(std::wstring_view str) {
  return str.empty() ? nullptr : str.data();
}
constexpr wchar_t *string_nullable(std::wstring &str) { return str.empty() ? nullptr : str.data(); }

class Executor {
public:
  Executor() = default;
  Executor(const Executor &) = delete;
  Executor &operator=(const Executor &) = delete;
  bool ParseArgv(bela::error_code &ec);
  bool PrepareEnv(bela::error_code &ec);
  std::wstring MakeEnv() { return simulator.MakeEnv(); }
  bool IsConhost() const { return conhost; } //
  std::wstring_view Cwd() const { return cwd; }
  std::wstring &Cwd() { return cwd; }
  std::wstring MakeShell() const;
  std::wstring Title() const {
    if (availableEnv.empty()) {
      return L"Baulk Terminal";
    }
    auto firstenv = availableEnv[0];
    if (availableEnv.size() > 1) {
      return bela::StringCat(L"Baulk Terminal [", firstenv, L"...+", availableEnv.size() - 1, L"]");
    }
    return bela::StringCat(L"Baulk Terminal [", firstenv, L"]");
  }

private:
  bela::env::Simulator simulator;
  std::wstring shell;
  std::wstring cwd;
  std::wstring arch;
  std::vector<std::wstring> venvs;
  std::vector<std::wstring> availableEnv;
  bool usevs{false};
  bool clang{false};
  bool cleanup{false};
  bool conhost{false};
};

} // namespace baulkterminal

#endif