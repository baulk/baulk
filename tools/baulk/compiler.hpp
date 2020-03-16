//
#ifndef BAULK_COMPILER_HPP
#define BAULK_COMPILER_HPP
#include "process.hpp"

namespace baulk::compiler {
class Executor {
public:
  Executor() = default;
  Executor(const Executor &) = delete;
  Executor &operator=(const Executor &) = delete;
  bool Initialize(bela::error_code &ec);
  template <typename... Args>
  int Execute(std::wstring_view cwd, std::wstring_view cmd, Args... args) {
    ec.message.clear();
    ec.code = 0;
    baulk::Process process;
    process.SetEnvStrings(env);
    if (!cwd.empty()) {
      process.Chdir(cwd);
    }
    if (auto exitcode = process.Execute(cmd, std::forward<Args>(args)...);
        exitcode != 0) {
      ec = process.ErrorCode();
      return exitcode;
    }
    return 0;
  }
  const bela::error_code &LastErrorCode() const { return ec; }
  bool Initialized() const { return initialized; }

private:
  std::wstring env;
  bela::error_code ec;
  bool initialized{false};
};
} // namespace baulk::compiler

#endif