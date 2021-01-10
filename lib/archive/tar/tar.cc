//
#include "tarinternal.hpp"

namespace baulk::archive::tar {
ssize_t FileReader::Read(void *buffer, size_t len, bela::error_code &ec) { return -1; }
ssize_t FileReader::ReadAt(void *buffer, size_t len, int64_t pos, bela::error_code &ec) { return -1; }
bool FileReader::PositionAt(uint64_t pos, bela::error_code &ec) const {
  auto li = *reinterpret_cast<LARGE_INTEGER *>(&pos);
  LARGE_INTEGER oli{0};
  if (SetFilePointerEx(fd, li, &oli, SEEK_SET) != TRUE) {
    ec = bela::make_system_error_code(L"SetFilePointerEx: ");
    return false;
  }
  return true;
}
bela::ssize_t Reader::Read(void *buffer, size_t size, bela::error_code &ec) {
  if (r == nullptr) {
    return -1;
  }
  return r->Read(buffer, size, ec);
}

// ReadAtLeast
bela::ssize_t Reader::ReadAtLeast(void *buffer, size_t size, bela::error_code &ec) {
  size_t rbytes = 0;
  auto p = reinterpret_cast<uint8_t *>(buffer);
  while (rbytes < size) {
    auto sz = Read(p + rbytes, size - rbytes, ec);
    if (sz < 0) {
      return -1;
    }
    if (sz == 0) {
      break;
    }
    rbytes += sz;
  }
  return rbytes;
}

std::optional<File> Reader::Next(bela::error_code &ec) {
  //
  return std::nullopt;
}

} // namespace baulk::archive::tar