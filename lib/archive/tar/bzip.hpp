///
#ifndef BAULK_ARCHIVE_TAR_BZIP_HPP
#define BAULK_ARCHIVE_TAR_BZIP_HPP
#include "tarinternal.hpp"
#include <bzlib.h>

namespace baulk::archive::tar::bzip {
class Reader : public bela::io::Reader {
public:
  Reader(bela::io::Reader *lr) : r(lr) {} // source reader
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  ~Reader();
  bool Initialize(bela::error_code &ec);
  ssize_t Read(void *buffer, size_t len, bela::error_code &ec);

private:
  bela::io::Reader *r{nullptr};
  bz_stream *bzs{nullptr};
  Buffer in;
  Buffer out;
};
} // namespace baulk::archive::tar::bzip

#endif