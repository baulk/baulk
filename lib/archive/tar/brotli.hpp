///
#ifndef BAULK_ARCHIVE_TAR_BROTLI_HPP
#define BAULK_ARCHIVE_TAR_BROTLI_HPP
#include "tarinternal.hpp"
#include <brotli/decode.h>

namespace baulk::archive::tar::brotli {
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
  BrotliDecoderState *state{nullptr};
  Buffer in;
  Buffer out;
  int64_t pickBytes{0};
  const unsigned char *inptr{nullptr};
  size_t avail_in{0};
  size_t totalout{ 0 };
  BrotliDecoderResult result{};
};
} // namespace baulk::archive::tar::brotli

#endif