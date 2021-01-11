//
#include "bzip.hpp"

namespace baulk::archive::tar::bzip {
Reader::~Reader() {
  if (bzs != nullptr) {
    BZ2_bzDecompressEnd(bzs);
    baulk::archive::archive_internal::Deallocate(bzs, 1);
  }
}
bool Reader::Initialize(bela::error_code &ec) {
  bzs = baulk::archive::archive_internal::Allocate<bz_stream>(1);
  memset(bzs, 0, sizeof(bz_stream));
  if (auto ret = BZ2_bzDecompressInit(bzs, 0, 0); ret != BZ_OK) {
    ec = bela::make_error_code(ret, L"BZ2_bzDecompressInit error");
    return false;
  }
  out.grow(outsize);
  in.grow(insize);
  return true;
}

ssize_t Reader::Read(void *buffer, size_t len, bela::error_code &ec) {
  //if (out.size() != 0) {
  //  auto minsize = (std::min)(len, out.size());
  //  memcpy(buffer, out.data(), minsize);
  //  out.size() -= minsize;
  //  return minsize;
  //}
  if (bzs->avail_out == 0) {
  }
  return -1;
}
} // namespace baulk::archive::tar::bzip