///
#ifndef BAULK_ARCHIVE_TAR_GZIP_HPP
#define BAULK_ARCHIVE_TAR_GZIP_HPP
#include "tarinternal.hpp"
#include <zlib-ng.h>

namespace baulk::archive::tar::gzip {
class Reader : public ExtractReader {
public:
  Reader(ExtractReader *lr) : r(lr) {}
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  ~Reader();
  bool Initialize(bela::error_code &ec);
  ssize_t Read(void *buffer, size_t len, bela::error_code &ec);
  bool Discard(int64_t len, bela::error_code &ec);
  bool WriteTo(const Writer &w, int64_t filesize,int64_t &extracted,  bela::error_code &ec);

private:
  bool decompress(bela::error_code &ec);
  ExtractReader *r{nullptr};
  zng_stream *zs{nullptr};
  Buffer out;
  Buffer in;
  int64_t pickBytes{0};
};
} // namespace baulk::archive::tar::gzip

#endif