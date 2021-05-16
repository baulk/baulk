//
#ifndef BAULK_COMPILER_HPP
#define BAULK_COMPILER_HPP
#include <bela/process.hpp>
#include <bela/path.hpp>
#include <bela/simulator.hpp>

namespace baulk::compiler {
class Executor {
public:
  Executor() = default;
  Executor(const Executor &) = delete;
  Executor &operator=(const Executor &) = delete;
  bool Initialize();
  template <typename... Args> int Execute(std::wstring_view cwd, std::wstring_view cmd, Args... args) {
    ec.message.clear();
    ec.code = 0;
    bela::process::Process process(&simulator);
    process.Chdir(cwd); // change cwd
    if (auto exitcode = process.Execute(cmd, std::forward<Args>(args)...); exitcode != 0) {
      ec = process.ErrorCode();
      return exitcode;
    }
    return 0;
  }
  const bela::error_code &LastErrorCode() const { return ec; }
  bool Initialized() const { return initialized; }

private:
  bela::env::Simulator simulator;
  bela::error_code ec;
  bool initialized{false};
};
} // namespace baulk::compiler

#endif