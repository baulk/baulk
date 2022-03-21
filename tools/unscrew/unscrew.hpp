//
#ifndef BAULK_UNSCREW_HPP
#define BAULK_UNSCREW_HPP
#include <bela/base.hpp>
#include <filesystem>

namespace baulk {
class Executor {
public:
  Executor() = default;
  Executor(const Executor &) = delete;
  Executor &operator=(const Executor &) = delete;
  bool ParseArgv(bela::error_code &ec);
  bool Execute(bela::error_code &ec);

private:
  std::vector<std::filesystem::path> archives;
  // bela::comptr<IProgressDialog> bar;
};
} // namespace baulk

#endif