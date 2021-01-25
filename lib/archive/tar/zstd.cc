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
  // Guarantee to successfully flush at least one complete compressed block in all circumstances.
  const auto binsize = ZSTD_DStreamInSize();
  outb.grow(boutsize);
  inb.grow(binsize);
  return true;
}

bool Reader::decompress(bela::error_code &ec) {
  for (;;) {
    if (in.pos == in.size) {
      auto n = r->Read(inb.data(), inb.capacity(), ec);
      if (n <= 0) {
        return false;
      }
      in.src = inb.data();
      in.size = n;
      in.pos = 0;
    }
    ZSTD_outBuffer out{outb.data(), outb.capacity(), 0};
    auto result = ZSTD_decompressStream(zds, &out, &in);
    if (ZSTD_isError(result) != 0) {
      ec = bela::make_error_code(ErrGeneral, L"ZSTD_decompressStream: ", bela::ToWide(ZSTD_getErrorName(result)));
      return false;
    }
    outb.pos() = 0;
    outb.size() = out.pos;
    if (out.pos != 0) {
      break;
    }
    // Need more data
  }
  return true;
}

ssize_t Reader::Read(void *buffer, size_t len, bela::error_code &ec) {
  if (outb.pos() == outb.size()) {
    if (!decompress(ec)) {
      return -1;
    }
  }
  auto minsize = (std::min)(len, outb.size() - outb.pos());
  memcpy(buffer, outb.data() + outb.pos(), minsize);
  outb.pos() += minsize;
  return minsize;
}

// Avoid multiple memory copies
bool Reader::WriteTo(const Writer &w, int64_t filesize, int64_t &extracted, bela::error_code &ec) {
  while (filesize > 0) {
    if (outb.pos() == outb.size()) {
      if (!decompress(ec)) {
        return false;
      }
    }
    auto minsize = (std::min)(static_cast<size_t>(filesize), outb.size() - outb.pos());
    auto p = outb.data() + outb.pos();
    outb.pos() += minsize;
    filesize -= minsize;
    extracted += minsize;
    if (!w(p, minsize, ec)) {
      return false;
    }
  }
  return true;
}

} // namespace baulk::archive::tar::zstd