//
#ifndef BAULKTERMINAL_HPP
#define BAULKTERMINAL_HPP
#pragma once
#include <bela/base.hpp>
#include <bela/env.hpp>
#include <vector>

namespace baulkterminal {
constexpr const wchar_t *string_nullable(std::wstring_view str) {
  return str.empty() ? nullptr : str.data();
}
constexpr wchar_t *string_nullable(std::wstring &str) { return str.empty() ? nullptr : str.data(); }

struct Boolean {
  bool initialized{false};
  bool value{false};
  explicit operator bool() const noexcept { return value; }
  bool operator()() const noexcept { return value; }
  Boolean() = default;
  Boolean(const Boolean &) = default;
  Boolean &operator=(const Boolean &) = default;
  // other initialize
  Boolean(bool val) : initialized(true), value(val) {}
  Boolean &operator=(bool val) {
    initialized = true;
    value = val;
    return *this;
  }
  Boolean &Delay(bool val) {
    if (!initialized) {
      value = val;
      initialized = true;
    }
    return *this;
  }
};

class Executor {
public:
  Executor() = default;
  Executor(const Executor &) = delete;
  Executor &operator=(const Executor &) = delete;
  bool ParseArgv(bela::error_code &ec);
  bool PrepareEnv(bela::error_code &ec);
  std::optional<std::wstring> MakeEnv(bela::error_code &ec);
  bool IsConhost() const { return conhost.value; } //
  std::wstring_view Cwd() const { return cwd; }
  std::wstring &Cwd() { return cwd; }
  std::wstring MakeShell() const;

private:
  bool InitializeBaulkEnv(bela::error_code &ec);
  bela::env::Derivator dev;
  std::wstring manifest; // env manifest file
  std::wstring shell;
  std::wstring cwd;
  std::wstring arch;
  std::vector<std::wstring> venvs;
  Boolean usevs;
  Boolean clang;
  Boolean cleanup;
  Boolean conhost;
};

} // namespace baulkterminal

#endif