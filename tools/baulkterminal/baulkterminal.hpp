//
#ifndef BAULKTERMINAL_HPP
#define BAULKTERMINAL_HPP
#pragma once
#include <bela/base.hpp>
#include <bela/simulator.hpp>
#include <bela/escapeargv.hpp>
#include <bela/fmt.hpp>
#include <vector>

namespace baulkterminal {
extern bool IsDebugMode;
int WriteTrace(std::wstring_view msg);
template <typename... Args> bela::ssize_t DbgPrint(const wchar_t *fmt, Args... args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  auto msg = bela::format_internal::StrFormatInternal(fmt, arg_array, sizeof...(args));
  std::wstring_view msgview(msg);
  if (!msgview.empty() && msgview.back() == '\n') {
    msgview.remove_suffix(1);
  }
  return WriteTrace(msgview);
}
inline bela::ssize_t DbgPrint(const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  std::wstring_view msg(fmt);
  if (!msg.empty() && msg.back() == '\n') {
    msg.remove_suffix(1);
  }
  return WriteTrace(msg);
}

constexpr const wchar_t *string_nullable(std::wstring_view str) { return str.empty() ? nullptr : str.data(); }
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