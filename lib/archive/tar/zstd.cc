//
#include "zstd.hpp"

namespace baulk::archive::tar::zstd {
Reader::~Reader() {
  if (zds != nullptr) {
    ZSTD_freeDCtx(zds);
  }
}
bool Reader::Initialize(bela::error_code &ec) {
  zds = ZSTD_createDCtx();
  if (zds == nullptr) {
    ec = bela::make_error_code(L"ZSTD_createDStream() out of memory");
    return false;
  }
  const auto boutsize = ZSTD_DStreamOutSize();
  const auto binsize = ZSTD_DStreamInSize();
  outb.grow(boutsize);
  inb.grow(insize);
  return true;
}
ssize_t Reader::CopyBuffer(void *buffer, size_t len, bela::error_code &ec) {
  auto minsize = (std::min)(len, outb.size() - outb.pos());
  memcpy(buffer, outb.data(), minsize);
  outb.pos() += minsize;
  return minsize;
}

ssize_t Reader::Read(void *buffer, size_t len, bela::error_code &ec) {
  if (outb.pos() < outb.size()) {
    return CopyBuffer(buffer, len, ec);
  }
  if (in.pos == in.size) {
    auto n = r->Read(inb.data(), inb.capacity(), ec);
    if (n <= 0) {
      return n;
    }
    in.src = inb.data();
    in.size = n;
    in.pos = 0;
  }
  ZSTD_outBuffer out{outb.data(), outb.capacity(), 0};
  auto result = ZSTD_decompressStream(zds, &out, &in);
  if (ZSTD_isError(result) != 0) {
    ec = bela::make_error_code(ErrGeneral, L"ZSTD_decompressStream: ", bela::ToWide(ZSTD_getErrorName(result)));
    return -1;
  }
  outb.pos() = 0;
  outb.size() = out.pos;
  return CopyBuffer(buffer, len, ec);
}
} // namespace baulk::archive::tar::zstd