///
#ifndef BAULK_ARCHIVE_TAR_ZSTD_HPP
#define BAULK_ARCHIVE_TAR_ZSTD_HPP
#include "tarinternal.hpp"
#include <zstd.h>

namespace baulk::archive::tar::zstd {
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
  ZSTD_DCtx *zds{nullptr};
  Buffer outb;
  Buffer inb;
  ZSTD_inBuffer in{0};
};
} // namespace baulk::archive::tar::zstd

#endif