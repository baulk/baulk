///
#include <bela/base.hpp>
#include <bela/terminal.hpp>
#include <bela/codecvt.hpp>
#include <bela/path.hpp>
#include <baulk/archive.hpp>
#include <baulk/archive/zip.hpp>
#include <baulk/fs.hpp>
#include <baulk/archive/extract.hpp>
#include "baulk.hpp"

namespace baulk::zip {

bool Decompress(std::wstring_view src, std::wstring_view outdir, bela::error_code &ec) {
  baulk::archive::ZipExtractor extractor(baulk::IsQuietMode);
  if (!extractor.OpenReader(src, outdir, ec)) {
    return false;
  }
  return extractor.Extract(ec);
}

} // namespace baulk::zip