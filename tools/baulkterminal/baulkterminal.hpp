//
#ifndef BAULKTERMINAL_HPP
#define BAULKTERMINAL_HPP
#pragma once
#include <bela/base.hpp>
#include <bela/simulator.hpp>
#include <bela/escapeargv.hpp>
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
  bool PrepareArgv(bela::EscapeArgv &ea, bela::error_code &ec);

private:
  std::wstring shell;
  std::wstring cwd;
  std::wstring arch;
  std::vector<std::wstring> venvs;
  bool usevs{false};
  bool clang{false};
  bool cleanup{false};
  bool conhost{false};
};

} // namespace baulkterminal

#endif