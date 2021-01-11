///
#ifndef BAULK_ARCHIVE_TAR_GZIP_HPP
#define BAULK_ARCHIVE_TAR_GZIP_HPP
#include "tarinternal.hpp"
#include "zlib.h"

namespace baulk::archive::tar::gzip {
class Reader : public bela::io::Reader {
public:
  Reader(bela::io::Reader *lr) : r(lr) {}
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  ~Reader();
  bool Initialize(bela::error_code &ec);
  ssize_t Read(void *buffer, size_t len, bela::error_code &ec);

private:
  ssize_t CopyBuffer(void *buffer, size_t len, bela::error_code &ec);
  bela::io::Reader *r{nullptr};
  z_stream *zs{nullptr};
  Buffer out;
  Buffer in;
};
} // namespace baulk::archive::tar::gzip

#endif