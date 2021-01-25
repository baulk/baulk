///
#ifndef BAULK_ARCHIVE_TAR_XZ_HPP
#define BAULK_ARCHIVE_TAR_XZ_HPP
#include "tarinternal.hpp"
#include <lzma.h>

namespace baulk::archive::tar::xz {
class Reader : public ExtractReader {
public:
  Reader(ExtractReader *lr) : r(lr) {}
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  ~Reader();
  bool Initialize(bela::error_code &ec);
  ssize_t Read(void *buffer, size_t len, bela::error_code &ec);
  bool WriteTo(const Writer &w, int64_t filesize, int64_t &extracted, bela::error_code &ec);

private:
  bool decompress(bela::error_code &ec);
  bela::ssize_t ReadAtLeast(void *buffer, size_t size, bela::error_code &ec);
  ExtractReader *r{nullptr};
  lzma_stream *xzs{nullptr};
  Buffer in;
  Buffer out;
  lzma_ret ret{LZMA_OK};
};
} // namespace baulk::archive::tar::xz

#endif