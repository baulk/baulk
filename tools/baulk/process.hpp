///
#ifndef BAULK_PROCESS_HPP
#define BAULK_PROCESS_HPP
#include <bela/base.hpp>
#include <bela/phmap.hpp>
#include <bela/escapeargv.hpp>
#include <bela/env.hpp>
#include <bela/finaly.hpp>
#include <bela/stdwriter.hpp>

namespace baulk {
constexpr const wchar_t *string_nullable(std::wstring_view str) {
  return str.empty() ? nullptr : str.data();
}
constexpr wchar_t *string_nullable(std::wstring &str) {
  return str.empty() ? nullptr : str.data();
}
class Process {
public:
  Process() = default;
  Process(const Process &) = delete;
  Process &operator=(const Process &) = delete;
  Process &Chdir(std::wstring_view dir) {
    cwd = dir;
    return *this;
  }
  Process &SetEnv(std::wstring_view key, std::wstring_view val,
                  bool force = false) {
    derivator.SetEnv(key, val, force);
    return *this;
  }
  Process &SetEnvStrings(std::wstring_view env_) {
    env = env_;
    return *this;
  }
  template <typename... Args> int Execute(std::wstring_view cmd, Args... args) {
    bela::EscapeArgv ea(cmd, args...);
    exitcode = ExecuteInternal(ea.data());
    return exitcode;
  }
  const bela::error_code &ErrorCode() const { return ec; }
  const DWORD ExitCode() const { return exitcode; }

private:
  int ExecuteInternal(wchar_t *cmdline);
  std::wstring cwd;
  std::wstring env;
  bela::env::Derivator derivator;
  bela::error_code ec;
  DWORD pid{0};
  DWORD exitcode{0};
};

class ProcessCapture {
public:
  ProcessCapture() = default;
  ProcessCapture(const ProcessCapture &) = delete;
  ProcessCapture &operator=(const ProcessCapture &) = delete;
  ProcessCapture &Chdir(std::wstring_view dir) {
    cwd = dir;
    return *this;
  }
  ProcessCapture &SetEnv(std::wstring_view key, std::wstring_view val,
                         bool force = false) {
    derivator.SetEnv(key, val, force);
    return *this;
  }
  ProcessCapture &SetEnvStrings(std::wstring_view env_) {
    env = env_;
    return *this;
  }
  template <typename... Args> int Execute(std::wstring_view cmd, Args... args) {
    bela::EscapeArgv ea(cmd, args...);
    exitcode = ExecuteInternal(ea.data());
    return exitcode;
  }
  const bela::error_code &ErrorCode() const { return ec; }
  const DWORD ExitCode() const { return exitcode; }
  std::string_view Out() const { return out; }

private:
  int ExecuteInternal(wchar_t *cmdline);
  std::wstring cwd;
  std::wstring env;
  std::string out;
  bela::env::Derivator derivator;
  bela::error_code ec;
  DWORD pid{0};
  DWORD exitcode{0};
};

} // namespace baulk

#endif
