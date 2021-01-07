///
#include <bela/types.hpp>
#include "zipinternal.hpp"
#include "../ppmd/Ppmd8.h"

namespace baulk::archive::zip {
using bela::ssize_t;
class SectionReader {
public:
  SectionReader() = default;
  SectionReader(const SectionReader &) = delete;
  SectionReader &operator=(const SectionReader &) = delete;
  bool OpenReader(HANDLE fd_, int64_t pos, int64_t sz, bela::error_code &ec);

private:
  // reference please don't close it
  Buffer buffer;
  HANDLE fd{INVALID_HANDLE_VALUE};
  int64_t size{0};
  int64_t offset{0};
  ssize_t w{0};
  ssize_t r{0};
};

bool SectionReader::OpenReader(HANDLE fd_, int64_t pos, int64_t sz, bela::error_code &ec) {
  fd = fd_;
  auto li = *reinterpret_cast<LARGE_INTEGER *>(&pos);
  LARGE_INTEGER oli{0};
  if (SetFilePointerEx(fd, li, &oli, SEEK_SET) != TRUE) {
    ec = bela::make_system_error_code(L"SetFilePointerEx: ");
    return false;
  }
  size = sz;
  buffer.grow(32 * 1024);
  return true;
}

bool Reader::decompressPpmd(const File &file, const Receiver &receiver, int64_t &decompressed,
                            bela::error_code &ec) const {
  ///
  CPpmd8 pd;
  memset(&pd, 0, sizeof(CPpmd8));
  Ppmd8_Construct(&pd);

  return false;
}
} // namespace baulk::archive::zip
