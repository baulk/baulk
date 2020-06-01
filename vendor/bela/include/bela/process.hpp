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
constexpr wchar_t *string_nullable(std::wstring &str) { return str.empty() ? nullptr : str.data(); }

enum CaptureMode {
  CAPTURE_OUT = 0,
  CAPTURE_ERR = 0x1,
  CAPTURE_USEIN = 0x2,
  CAPTURE_USEERR = 0x4,
};

class Process {
public:
  Process() = default;
  Process(const Process &) = delete;
  Process &operator=(const Process &) = delete;
  Process &Chdir(std::wstring_view dir) {
    cwd = dir;
    return *this;
  }
  Process &SetEnv(std::wstring_view key, std::wstring_view val, bool force = false) {
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
    exitcode = CaptureInternal(ea.data(), CAPTURE_OUT);
    return exitcode;
  }
  template <typename... Args>
  int CaptureWithMode(DWORD flags, std::wstring_view cmd, Args... args) {
    bela::EscapeArgv ea(cmd, args...);
    exitcode = CaptureInternal(ea.data(), flags);
    return exitcode;
  }

private:
  int ExecuteInternal(wchar_t *cmdline);
  int CaptureInternal(wchar_t *cmdline, DWORD flags);
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