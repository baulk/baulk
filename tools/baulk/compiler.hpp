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
  void Chdir(std::wstring_view dir) { cwd = dir; }
  template <typename... Args> int Execute(std::wstring_view cmd, Args... args) {
    baulk::Process process;
    process.SetEnvStrings(env);
    process.Chdir(cwd);
    if (auto exitcode = process.Execute(cmd, std::forward<Args>(args)...);
        exitcode != 0) {
      ec = process.ErrorCode();
      return exitcode;
    }
    return 0;
  }

private:
  std::wstring cwd;
  std::wstring env;
  bela::error_code ec;
};
} // namespace baulk::compiler

#endif