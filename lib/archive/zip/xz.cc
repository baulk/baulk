///
#include "zipinternal.hpp"
#include <lzma.h>

namespace baulk::archive::zip {
enum header_state { INCOMPLETE, OUTPUT, DONE };

#define HEADER_BYTES_ZIP 9
#define HEADER_MAGIC_LENGTH 4
#define HEADER_MAGIC1_OFFSET 0
#define HEADER_MAGIC2_OFFSET 2
#define HEADER_SIZE_OFFSET 9
#define HEADER_SIZE_LENGTH 8
#define HEADER_PARAMETERS_LENGTH 5
#define HEADER_LZMA_ALONE_LENGTH (HEADER_PARAMETERS_LENGTH + HEADER_SIZE_LENGTH)

// XZ
bool Reader::decompressXz(const File &file, const Receiver &receiver, int64_t &decompressed,
                          bela::error_code &ec) const {
  //
  return false;
}
// LZMA2
bool Reader::decompressLZMA2(const File &file, const Receiver &receiver, int64_t &decompressed,
                             bela::error_code &ec) const {
  //
  return false;
}

} // namespace baulk::archive::zip