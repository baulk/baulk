//
#ifndef BAULK_EXTRACTOR_INTERNAL_HPP
#define BAULK_EXTRACTOR_INTERNAL_HPP
#include <baulk/archive.hpp>

namespace baulk::archive {
class ZipExtractor : public Extractor {
public:
  ZipExtractor(const ExtractorOptions &opts_, bela::io::FD &&fd_) : opts{opts_}, fd{std::move(fd_)} {}
  ZipExtractor(const ZipExtractor &) = delete;
  ZipExtractor &operator=(const ZipExtractor &) = delete;
  bool Extract(bela::error_code &ec);

private:
  const ExtractorOptions &opts;
  bela::io::FD fd;
};

class UniversalExtractor : public Extractor {
public:
  bool Extract(bela::error_code &ec);

private:
  const ExtractorOptions &opts;
  bela::io::FD fd;
};

} // namespace baulk::archive

#endif