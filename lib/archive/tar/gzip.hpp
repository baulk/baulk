///
#ifndef BAULK_ARCHIVE_TAR_GZIP_HPP
#define BAULK_ARCHIVE_TAR_GZIP_HPP
#include "tarinternal.hpp"

namespace baulk::archive::tar::gzip {
class Reader : public bela::io::Reader {
public:
  Reader(bela::io::Reader *lr) : r(lr) {}
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  ssize_t Read(void *buffer, size_t len, bela::error_code &ec);

private:
  bela::io::Reader *r{nullptr};
};
} // namespace baulk::archive::tar::gzip

#endif