//
#ifndef BAULK_UNSCREW_HPP
#define BAULK_UNSCREW_HPP
#include <bela/base.hpp>
#include <bela/comutils.hpp>
#include <filesystem>
#include "extractor.hpp"

namespace baulk {
class Executor {
public:
  Executor() = default;
  Executor(const Executor &) = delete;
  Executor &operator=(const Executor &) = delete;
  bool ParseArgv(bela::error_code &ec);
  bool Execute(bela::error_code &ec);

private:
  std::vector<fs::path> archive_files;
  fs::path destination;
  baulk::ExtractorOptions opts;
};
} // namespace baulk

#endif