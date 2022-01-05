///
#ifndef BAULK_ARCHIVE_TAR_ZSTD_HPP
#define BAULK_ARCHIVE_TAR_ZSTD_HPP
#include "tarinternal.hpp"
#define ZSTD_STATIC_LINKING_ONLY 1
#include <zstd.h>

namespace baulk::archive::tar::zstd {
class Reader : public ExtractReader {
public:
  Reader(ExtractReader *lr) : r(lr) {}
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  ~Reader();
  bool Initialize(bela::error_code &ec);
  ssize_t Read(void *buffer, size_t len, bela::error_code &ec);
  bool Discard(int64_t len, bela::error_code &ec);
  bool WriteTo(const Writer &w, int64_t filesize, int64_t &extracted, bela::error_code &ec);

private:
  bool decompress(bela::error_code &ec);
  ExtractReader *r{nullptr};
  ZSTD_DCtx *zds{nullptr};
  Buffer outb;
  Buffer inb;
  ZSTD_inBuffer in{0};
};
} // namespace baulk::archive::tar::zstd

#endif