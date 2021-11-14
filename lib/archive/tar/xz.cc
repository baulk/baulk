//
#include "xz.hpp"

namespace baulk::archive::tar::xz {
constexpr size_t xzoutsize = 256 * 1024;
constexpr size_t xzinsize = 256 * 1024;
Reader::~Reader() {
  if (xzs != nullptr) {
    baulk::mem::deallocate(xzs);
  }
}
bool Reader::Initialize(bela::error_code &ec) {
  xzs = baulk::mem::Allocate<lzma_stream>();
  memset(xzs, 0, sizeof(lzma_stream));
  auto ret = lzma_stream_decoder(xzs, UINT64_MAX, LZMA_CONCATENATED);
  if (ret != LZMA_OK) {
    ec = bela::make_error_code(ret, L"lzma_stream_decoder error ", ret);
    return false;
  }
  out.grow(xzoutsize);
  in.grow(xzinsize);
  xzs->next_out = out.data();
  xzs->avail_out = xzoutsize;
  return true;
}

// ReadAtLeast
bela::ssize_t Reader::ReadAtLeast(void *buffer, size_t size, bela::error_code &ec) {
  size_t rbytes = 0;
  auto p = reinterpret_cast<uint8_t *>(buffer);
  while (rbytes < size) {
    auto sz = r->Read(p + rbytes, size - rbytes, ec);
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

inline bela::error_code XzErrorCode(lzma_ret ret) {
  switch (ret) {
  case LZMA_MEM_ERROR:
    return bela::make_error_code(L"memory error");
  case LZMA_FORMAT_ERROR:
    return bela::make_error_code(L"File format not recognized");
  case LZMA_OPTIONS_ERROR:
    return bela::make_error_code(L"Unsupported compression options");
  case LZMA_DATA_ERROR:
    return bela::make_error_code(L"File is corrupt");
  case LZMA_BUF_ERROR:
    return bela::make_error_code(L"Unexpected end of input");
  default:
    break;
  }
  return bela::make_error_code(bela::ErrGeneral, L"Internal error (bug) ret=", ret);
}

bool Reader::decompress(bela::error_code &ec) {
  if (ret == LZMA_STREAM_END) {
    ec = bela::make_error_code(bela::ErrEnded, L"xz stream end");
    return false;
  }
  for (;;) {
    if (xzs->avail_in == 0 || xzs->total_in == 0) {
      auto n = r->Read(in.data(), xzinsize, ec);
      if (n < 0) {
        return n;
      }
      xzs->next_in = in.data();
      xzs->avail_in = static_cast<size_t>(n);
    }
    ret = lzma_code(xzs, xzs->avail_in == 0 ? LZMA_FINISH : LZMA_RUN);
    if (xzs->avail_out == 0 || ret == LZMA_STREAM_END) {
      auto have = xzoutsize - xzs->avail_out;
      out.pos() = 0;
      out.size() = have;
      xzs->next_out = out.data();
      xzs->avail_out = xzoutsize;
      return true;
    }
    if (ret != LZMA_OK) {
      ec = XzErrorCode(ret);
      return false;
    }
  }
  return false;
}

ssize_t Reader::Read(void *buffer, size_t len, bela::error_code &ec) {
  if (out.pos() == out.size()) {
    if (!decompress(ec)) {
      return -1;
    }
  }
  auto minsize = (std::min)(len, out.size() - out.pos());
  memcpy(buffer, out.data() + out.pos(), minsize);
  out.pos() += minsize;
  return minsize;
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

} // namespace baulk::archive::tar::xz