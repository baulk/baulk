//
#include "gzip.hpp"

namespace baulk::archive::tar::gzip {
Reader::~Reader() {
  if (zs != nullptr) {
    inflateEnd(zs);
    baulk::archive::archive_internal::Deallocate(zs, 1);
  }
}
bool Reader::Initialize(bela::error_code &ec) {
  zs = baulk::archive::archive_internal::Allocate<z_stream>(1);
  memset(zs, 0, sizeof(z_stream));
  if (auto zerr = inflateInit2(zs, MAX_WBITS + 16); zerr != Z_OK) {
    ec = bela::make_error_code(ErrGeneral, bela::ToWide(zError(zerr)));
    return false;
  }
  out.grow(outsize);
  in.grow(insize);
  return true;
}

ssize_t Reader::CopyBuffer(void *buffer, size_t len, bela::error_code &ec) {
  auto minsize = (std::min)(len, out.size() - out.pos());
  memcpy(buffer, out.data(), minsize);
  out.pos() += minsize;
  return minsize;
}

// Read data
ssize_t Reader::Read(void *buffer, size_t len, bela::error_code &ec) {
  if (in.pos() != in.size()) {
    return CopyBuffer(buffer, len, ec);
  }
  if (zs->avail_out != 0) {
    auto n = r->Read(in.data(), in.capacity(), ec);
    if (n <= 0) {
      return n;
    }
    zs->next_in = in.data();
    zs->avail_in = n;
  }
  zs->avail_out = static_cast<int>(out.capacity());
  zs->next_out = out.data();
  auto ret = ::inflate(zs, Z_NO_FLUSH);
  switch (ret) {
  case Z_NEED_DICT:
    ret = Z_DATA_ERROR;
    [[fallthrough]];
  case Z_DATA_ERROR:
    [[fallthrough]];
  case Z_MEM_ERROR:
    ec = bela::make_error_code(ret, bela::ToWide(zError(ret)));
    return -1;
  default:
    break;
  }
  out.pos() = 0;
  out.size() = outsize - zs->avail_out;
  return CopyBuffer(buffer, len, ec);
}
} // namespace baulk::archive::tar::gzip