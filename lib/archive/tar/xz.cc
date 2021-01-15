//
#include "xz.hpp"

namespace baulk::archive::tar::xz {
constexpr size_t xzoutsize = 256 * 1024;
constexpr size_t xzinsize = 256 * 1024;
Reader::~Reader() {
  if (xzs != nullptr) {
    baulk::archive::archive_internal::Deallocate(xzs, 1);
  }
}
bool Reader::Initialize(bela::error_code &ec) {
  xzs = baulk::archive::archive_internal::Allocate<lzma_stream>(1);
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

ssize_t Reader::CopyBuffer(void *buffer, size_t len, bela::error_code &ec) {
  auto minsize = (std::min)(len, out.size() - out.pos());
  memcpy(buffer, out.data() + out.pos(), minsize);
  out.pos() += minsize;
  return minsize;
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

ssize_t Reader::Read(void *buffer, size_t len, bela::error_code &ec) {
  if (out.pos() != out.size()) {
    return CopyBuffer(buffer, len, ec);
  }
  if (ret == LZMA_STREAM_END) {
    ec = bela::make_error_code(bela::ErrEnded, L"xz stream end");
    return -1;
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
      return CopyBuffer(buffer, len, ec);
    }
    if (ret != LZMA_OK) {
      ec = XzErrorCode(ret);
      return -1;
    }
  }
  return -1;
}
} // namespace baulk::archive::tar::xz