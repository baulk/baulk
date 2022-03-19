///
// https://github.com/madler/sunzip/blob/master/sunzip.c
#include "zipinternal.hpp"
#include <zlib.h>
// deflate64
#include "../deflate64/infback9.h"

namespace baulk::archive::zip {
constexpr DWORD CHUNK = 131072;
struct inflate64Writer {
  const Writer &w;
  Summator sum;
  uint64_t count{0};
  bool canceled{false};
};
int put(void *out_desc, unsigned char *buf, unsigned len) {
  auto w = reinterpret_cast<inflate64Writer *>(out_desc);
  // w->crc32val = crc32_fast(buf, len, w->crc32val);
  w->sum.Update(buf, len);
  w->count += len;
  if (!w->w(buf, len)) {
    w->canceled = true;
    return 1;
  }
  return 0;
}

struct inflate64Reader {
  HANDLE fd{INVALID_HANDLE_VALUE};
  uint8_t *buf{nullptr};
  int64_t count{0};
  int64_t offset{0};
  int64_t size{0};
};

unsigned get(void *in_desc, unsigned char **buf) {
  auto r = reinterpret_cast<inflate64Reader *>(in_desc);
  auto next = r->buf;
  if (buf != nullptr) {
    *buf = next;
  }
  auto want = (std::min)(CHUNK, static_cast<DWORD>(r->size - r->offset));
  if (want == 0) {
    return 0;
  }
  DWORD got = 0;
  for (;;) {
    if (::ReadFile(r->fd, next, want, &got, nullptr) != TRUE) {
      return 0;
    }
    next += got;
    want -= got;
    if (got == 0 || want == 0) {
      break;
    }
  }
  auto len = CHUNK - want;
  r->count += len;
  r->offset += len;
  return len;
}

// DEFLATE64
bool Reader::decompressDeflate64(const File &file, const Writer &w, bela::error_code &ec) const {
  Buffer window(65536);
  Buffer chunk(CHUNK);
  z_stream zs;
  memset(&zs, 0, sizeof(zs));
  zs.zalloc = baulk::mem::allocate_zlib;
  zs.zfree = baulk::mem::deallocate_simple;
  auto ret = inflateBack9Init(&zs, window.data());
  if (ret != Z_OK) {
    ec = bela::make_error_code(ret == Z_MEM_ERROR ? L"not enough memory (!)" : L"internal error");
    return false;
  }
  auto closer = bela::finally([&] { inflateBack9End(&zs); });
  inflate64Writer iw{
      //
      .w = w,
      .sum = Summator(file.crc32sum), //
      .count = 0,
      .canceled = false //
  };
  inflate64Reader r{fd.NativeFD(), chunk.data(), 0, 0, static_cast<int64_t>(file.compressedSize)};
  ret = inflateBack9(&zs, get, &r, put, &iw);
  if (iw.canceled) {
    ec = bela::make_error_code(ErrCanceled, L"canceled");
    return false;
  }
  if (ret != Z_STREAM_END) {
    ec = bela::make_error_code(L"deflate64 compressed data corrupted");
    return false;
  }
  if (!iw.sum.Valid()) {
    ec = bela::make_error_code(ErrGeneral, L"crc32 want ", file.crc32sum, L" got ", iw.sum.Current(), L" not match");
    return false;
  }
  return true;
}
} // namespace baulk::archive::zip