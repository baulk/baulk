//
#include "tarinternal.hpp"

namespace baulk::archive::tar {

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