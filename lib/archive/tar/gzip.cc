//
#include "gzip.hpp"

namespace baulk::archive::tar::gzip {

Reader::~Reader() {
  if (zs != nullptr) {
    inflateEnd(zs);
    baulk::mem::deallocate(zs);
  }
}
bool Reader::Initialize(bela::error_code &ec) {
  zs = baulk::mem::allocate<z_stream>();
  memset(zs, 0, sizeof(z_stream));
  zs->zalloc = baulk::mem::allocate_zlib;
  zs->zfree = baulk::mem::deallocate_simple;
  if (auto zerr = inflateInit2(zs, MAX_WBITS + 16); zerr != Z_OK) {
    ec = bela::make_error_code(ErrExtractGeneral, bela::encode_into<char, wchar_t>(zError(zerr)));
    return false;
  }
  out.grow(outsize);
  in.grow(insize);
  return true;
}

bool Reader::decompress(bela::error_code &ec) {
  for (;;) {
    if (zs->avail_out != 0 || pickBytes == 0) {
      auto n = r->Read(in.data(), in.capacity(), ec);
      if (n <= 0) {
        return false;
      }
      pickBytes += static_cast<int64_t>(n);
      zs->next_in = in.data();
      zs->avail_in = static_cast<uint32_t>(n);
    }
    zs->avail_out = static_cast<int>(outsize);
    zs->next_out = out.data();
    auto ret = ::inflate(zs, Z_NO_FLUSH);
    switch (ret) {
    case Z_NEED_DICT:
      ret = Z_DATA_ERROR;
      [[fallthrough]];
    case Z_DATA_ERROR:
      [[fallthrough]];
    case Z_MEM_ERROR:
      ec = bela::make_error_code(ErrExtractGeneral, bela::encode_into<char, wchar_t>(zError(ret)));
      return false;
    default:
      break;
    }
    auto have = outsize - zs->avail_out;
    out.pos() = 0;
    out.size() = have;
    if (have != 0) {
      break;
    }
  }
  return true;
}

// Read data
ssize_t Reader::Read(void *buffer, size_t len, bela::error_code &ec) {
  if (out.pos() == out.size()) {
    if (!decompress(ec)) {
      return -1;
    }
  }
  auto minsize = (std::min)(len, out.size() - out.pos());
  memcpy(buffer, out.data() + out.pos(), minsize);
  out.pos() += minsize;
  return static_cast<ssize_t>(minsize);
}

bool Reader::Discard(int64_t len, bela::error_code &ec) {
  while (len > 0) {
    if (out.pos() == out.size()) {
      if (!decompress(ec)) {
        return false;
      }
    }
    // seek position
    auto minsize = (std::min)(static_cast<size_t>(len), out.size() - out.pos());
    out.pos() += minsize;
    len -= minsize;
  }
  return true;
}

// Avoid multiple memory copies
bool Reader::WriteTo(const Writer &w, int64_t filesize, int64_t &extracted, bela::error_code &ec) {
  while (filesize > 0) {
    if (out.pos() == out.size()) {
      if (!decompress(ec)) {
        return false;
      }
    }
    auto minsize = (std::min)(static_cast<size_t>(filesize), out.size() - out.pos());
    auto p = out.data() + out.pos();
    out.pos() += minsize;
    filesize -= minsize;
    extracted += minsize;
    if (!w(p, minsize, ec)) {
      return false;
    }
  }
  return true;
}

} // namespace baulk::archive::tar::gzip