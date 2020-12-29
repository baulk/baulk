///
#include "zipinternal.hpp"
#include <bzlib.h>

namespace baulk::archive::zip {

// bzip2
bool Reader::decompressBz2(const File &file, const Receiver &receiver, int64_t &decompressed,
                           bela::error_code &ec) const {
  //
  bz_stream bzs;
  memset(&bzs, 0, sizeof(bzs));
  return false;
}

} // namespace baulk::archive::zip