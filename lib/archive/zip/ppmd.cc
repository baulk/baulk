///
#include "zipinternal.hpp"
#include "../ppmd/Ppmd8.h"

namespace baulk::archive::zip {

template <size_t BufferSize = 256 * 1024> class FileReader {
public:
private:
};

bool Reader::decompressPpmd(const File &file, const Receiver &receiver, int64_t &decompressed,
                            bela::error_code &ec) const {
  ///
  CPpmd8 pd;
  memset(&pd, 0, sizeof(CPpmd8));
  Ppmd8_Construct(&pd);

  return false;
}
} // namespace baulk::archive::zip
