// bela::process type
#ifndef BELA_PROCESS_HPP
#define BELA_PROCESS_HPP
#include "base.hpp"
#include "env.hpp"
#include "escapeargv.hpp"

namespace bela::process {
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
  Process &SetEnv(std::wstring_view key, std::wstring_view val,
                  bool force = false) {
    de.SetEnv(key, val, force);
    return *this;
  }
  Process &SetEnvStrings(std::wstring_view env_) {
    env = env_;
    return *this;
  }
  const bela::error_code &ErrorCode() const { return ec; }
  const DWORD ExitCode() const { return exitcode; }
  std::string_view Out() const { return out; } // when call Capture
  template <typename... Args> int Execute(std::wstring_view cmd, Args... args) {
    bela::EscapeArgv ea(cmd, args...);
    exitcode = ExecuteInternal(ea.data());
    return exitcode;
  }
  template <typename... Args> int Capture(std::wstring_view cmd, Args... args) {
    bela::EscapeArgv ea(cmd, args...);
    exitcode = CaptureInternal(ea.data());
    return exitcode;
  }

private:
  int ExecuteInternal(wchar_t *cmdline);
  int CaptureInternal(wchar_t *cmdline);
  std::wstring cwd;
  std::wstring env;
  std::string out;
  bela::env::Derivator de;
  bela::error_code ec;
  DWORD pid{0};
  DWORD exitcode{0};
};
} // namespace bela::process

#endif