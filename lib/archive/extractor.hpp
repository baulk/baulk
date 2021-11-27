//
#ifndef BAULK_EXTRACTOR_INTERNAL_HPP
#define BAULK_EXTRACTOR_INTERNAL_HPP
#include <baulk/archive.hpp>

namespace baulk::archive {
class ZipExtractor : public Extractor {
public:
  bool Extract(bela::error_code &ec);

private:
  const ExtractorOptions &opts;
  bela::io::FD fd;
};

class TarExtractor : public Extractor {
public:
  bool Extract(bela::error_code &ec);

private:
  const ExtractorOptions &opts;
  bela::io::FD fd;
};

} // namespace baulk::archive

#endif