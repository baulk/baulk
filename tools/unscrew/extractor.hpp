///
#ifndef EXTRACTOR_HPP
#define EXTRACTOR_HPP
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <bela/comutils.hpp>
#include <baulk/archive.hpp>
#include <baulk/archive/zip.hpp>
#include <ShObjIdl.h>
#include <ShlObj_core.h>
#include <functional>

namespace baulk {
using OnEntries = std::function<bool(std::wstring_view filename)>;

class Extractor {
public:
  Extractor() = default;
  Extractor(const Extractor &) = delete;
  Extractor &operator=(const Extractor &) = delete;
  bool ParseArgv(bela::error_code &ec);
  bool Execute(bela::error_code &ec);

private:
  std::vector<std::filesystem::path> archives;
  // bela::comptr<IProgressDialog> bar;
};
} // namespace baulk

#endif