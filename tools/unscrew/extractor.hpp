///
#ifndef EXTRACTOR_HPP
#define EXTRACTOR_HPP
#include <bela/base.hpp>
#include <bela/io.hpp>
#include <filesystem>
#include <ShlObj_core.h>
#include <baulk/archive/extractor.hpp>

namespace baulk {
using baulk::archive::ExtractorOptions;
class Extractor {
public:
  virtual bool Extract(IProgressDialog *bar, bela::error_code &ec) = 0;
};

std::shared_ptr<Extractor> MakeExtractor(const std::filesystem::path &archive_file, const std::filesystem::path &dest,
                                         const ExtractorOptions &opts, bela::error_code &ec);

} // namespace baulk

#endif