///
#ifndef BAULK_ARCHIVE_TAR_BZIP_HPP
#define BAULK_ARCHIVE_TAR_BZIP_HPP
#include "tarinternal.hpp"
#include <bzlib.h>

namespace baulk::archive::tar::bzip {
class Reader : public ExtractReader {
public:
  Reader(ExtractReader *lr) : r(lr) {} // source reader
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  ~Reader();
  bool Initialize(bela::error_code &ec);
  ssize_t Read(void *buffer, size_t len, bela::error_code &ec);
  bool WriteTo(const Writer &w, int64_t filesize, int64_t &extracted, bela::error_code &ec);

private:
  bool decompress(bela::error_code &ec);
  ExtractReader *r{nullptr};
  bz_stream *bzs{nullptr};
  Buffer in;
  Buffer out;
  int64_t pickBytes{0};
  int ret{BZ_OK};
};
} // namespace baulk::archive::tar::bzip

#endif