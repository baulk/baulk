///
#include "tarinternal.hpp"
#include <bela/endian.hpp>
#include "zstd.hpp"
#include "bzip.hpp"
#include "brotli.hpp"
#include "gzip.hpp"
#include "xz.hpp"

namespace baulk::archive::tar {

std::shared_ptr<ExtractReader> MakeReader(FileReader &fd, int64_t offset, file_format_t afmt, bela::error_code &ec) {
  if (!fd.Seek(offset, ec)) {
    return nullptr;
  }
  switch (afmt) {
  case file_format_t::gz:
    if (auto r = std::make_shared<gzip::Reader>(&fd); r->Initialize(ec)) {
      return r;
    }
    break;
  case file_format_t::bz2:
    if (auto r = std::make_shared<bzip::Reader>(&fd); r->Initialize(ec)) {
      return r;
    }
    break;
  case file_format_t::zstd:
    if (auto r = std::make_shared<zstd::Reader>(&fd); r->Initialize(ec)) {
      return r;
    }
    break;
  case file_format_t::xz:
    if (auto r = std::make_shared<xz::Reader>(&fd); r->Initialize(ec)) {
      return r;
    }
    break;
  case file_format_t::brotli:
    if (auto r = std::make_shared<brotli::Reader>(&fd); r->Initialize(ec)) {
      return r;
    }
    break;
  default:
    break;
  }
  ec.code = ErrNoFilter;
  return nullptr;
}
} // namespace baulk::archive::tar
