//
#include "gzip.hpp"

namespace baulk::archive::tar::gzip {
Reader::~Reader() {
  //
}
bool Reader::Initialize(bela::error_code &ec) {
  //
  return true;
}
ssize_t Reader::Read(void *buffer, size_t len, bela::error_code &ec) {
  //
  return -1;
}
} // namespace baulk::archive::tar::gzip