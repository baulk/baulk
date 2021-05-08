// bela::process type
#ifndef BELA_PROCESS_HPP
#define BELA_PROCESS_HPP
#include "base.hpp"
#include "escapeargv.hpp"
#include "simulator.hpp"

namespace bela::process {
constexpr const wchar_t *string_nullable(std::wstring_view str) { return str.empty() ? nullptr : str.data(); }
constexpr wchar_t *string_nullable(std::wstring &str) { return str.empty() ? nullptr : str.data(); }

enum CaptureMode : DWORD {
  CAPTURE_OUT = 0, // default
  CAPTURE_ERR = 0x1,
  CAPTURE_USEIN = 0x2,
  CAPTURE_USEERR = 0x4,
};
using bela::env::Simulator;
class Process {
public:
  Process(const Simulator *simulator_ = nullptr) : simulator(simulator_) {}
  Process(const Process &) = delete;
  Process &operator=(const Process &) = delete;
  Process &Chdir(std::wstring_view dir) {
    cwd = dir;
    return *this;
  }
  const bela::error_code &ErrorCode() const { return ec; }
  const DWORD ExitCode() const { return exitcode; }
  std::string_view Out() const { return out; } // when call Capture
  template <typename... Args> int Execute(std::wstring_view cmd, Args... args) {
    bela::EscapeArgv ea(cmd, args...);
    exitcode = ExecuteInternal(cmd, ea.data());
    return exitcode;
  }
  template <typename... Args> int Capture(std::wstring_view cmd, Args... args) {
    bela::EscapeArgv ea(cmd, args...);
    exitcode = CaptureInternal(cmd, ea.data(), CAPTURE_OUT);
    return exitcode;
  }
  template <typename... Args> int CaptureWithMode(DWORD flags, std::wstring_view cmd, Args... args) {
    bela::EscapeArgv ea(cmd, args...);
    exitcode = CaptureInternal(cmd, ea.data(), flags);
    return exitcode;
  }

private:
  int ExecuteInternal(std::wstring_view file, wchar_t *cmdline);
  int CaptureInternal(std::wstring_view file, wchar_t *cmdline, DWORD flags);
  const Simulator *simulator{nullptr};
  std::wstring cwd;
  std::string out;
  bela::error_code ec;
  DWORD pid{0};
  DWORD exitcode{0};
};
} // namespace bela::process

#endif