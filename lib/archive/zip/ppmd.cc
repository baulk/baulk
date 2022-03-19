///
#include <bela/types.hpp>
#include <bela/endian.hpp>
#include "zipinternal.hpp"
#include "../ppmd/Ppmd8.h"

namespace baulk::archive::zip {
using bela::ssize_t;
constexpr auto BufferSize = static_cast<size_t>(1) << 20;
class SectionReader {
public:
  SectionReader(HANDLE fd_, int64_t len) : fd(fd_), size(len) { cacheb.grow(32 * 1024); }
  SectionReader(const SectionReader &) = delete;
  SectionReader &operator=(const SectionReader &) = delete;
  [[nodiscard]] ssize_t Buffered() const { return w - r; }
  [[nodiscard]] int64_t AvailableBytes() const { return size - offset; }
  ssize_t Read(void *buffer, ssize_t len) {
    if (buffer == nullptr || len == 0) {
      ec = bela::make_error_code(L"buffer is nil");
      return -1;
    }
    if (r == w) {
      if (static_cast<size_t>(len) > cacheb.capacity()) {
        // Large read, empty buffer.
        // Read directly into p to avoid copy.
        ssize_t rlen = 0;
        if (!fsread(buffer, len, rlen, ec)) {
          return -1;
        }
        return rlen;
      }
      w = 0;
      r = 0;
      // section EOF support
      DWORD dwRead = static_cast<DWORD>((std::min)(cacheb.capacity(), static_cast<size_t>(size - offset)));
      if (dwRead == 0) {
        ec = bela::make_error_code(ERROR_HANDLE_EOF, L"unexpected EOF");
        return -1;
      }
      if (!fsread(cacheb.data(), dwRead, w, ec)) {
        return -1;
      }
      if (w == 0) {
        ec = bela::make_error_code(ERROR_HANDLE_EOF, L"unexpected EOF");
        return -1;
      }
    }
    auto n = (std::min)(w - r, len);
    memcpy(buffer, cacheb.data() + r, n);
    r += n;
    return n;
  }
  ssize_t ReadFull(void *buffer, ssize_t len) {
    auto p = reinterpret_cast<uint8_t *>(buffer);
    ssize_t n = 0;
    for (; n < len;) {
      auto nn = Read(p + n, len - n);
      if (nn == -1) {
        return -1;
      }
      n += nn;
    }
    if (n < len) {
      ec = bela::make_error_code(ERROR_HANDLE_EOF, L"unexpected EOF");
      return -1;
    }
    return n;
  }
  const auto &ErrorCode() { return ec; }

private:
  // reference please don't close it
  Buffer cacheb;
  HANDLE fd{INVALID_HANDLE_VALUE};
  int64_t size{0};
  int64_t offset{0};
  ssize_t w{0};
  ssize_t r{0};
  bela::error_code ec;
  bool fsread(void *b, ssize_t len, ssize_t &rlen, bela::error_code &ec) {
    DWORD dwSize = {0};
    if (ReadFile(fd, b, static_cast<DWORD>(len), &dwSize, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"ReadFile: ");
      return false;
    }
    rlen = static_cast<ssize_t>(len);
    offset += len;
    return true;
  }
};

struct CByteInToLook {
  IByteIn vt;
  SectionReader *sr{nullptr};
};

Byte ppmd_read(const IByteIn *pp) {
  if (pp == nullptr) {
    return 0;
  }
  CByteInToLook *p = CONTAINER_FROM_VTBL(pp, CByteInToLook, vt);
  if (p->sr == nullptr) {
    return 0;
  }
  Byte buf[8] = {0};
  if (p->sr->ReadFull(buf, 1) != 1) {
    return 0;
  }
  return buf[0];
}
static void *SzBigAlloc(ISzAllocPtr p, size_t size) {
  (void)p;
  return mi_malloc(size);
}
static void SzBigFree(ISzAllocPtr p, void *address) {
  (void)p;
  mi_free(address);
}

const ISzAlloc g_BigAlloc = {SzBigAlloc, SzBigFree};

bool Reader::decompressPpmd(const File &file, const Writer &w, bela::error_code &ec) const {
  SectionReader sr(fd.NativeFD(), file.compressedSize);
  CByteInToLook s;
  s.vt.Read = ppmd_read;
  s.sr = &sr;
  CPpmd8 _ppmd = {nullptr};
  _ppmd.Stream.In = reinterpret_cast<IByteIn *>(&s);
  Ppmd8_Construct(&_ppmd);
  auto closer = bela::finally([&] { Ppmd8_Free(&_ppmd, &g_BigAlloc); });
  uint8_t buf[8];
  if (sr.ReadFull(buf, 2) != 2) {
    ec = sr.ErrorCode();
    return false;
  }
  uint32_t val = bela::cast_fromle<uint16_t>(buf);
  uint32_t order = (val & 0xF) + 1;
  uint32_t mem = ((val >> 4) & 0xFF) + 1;
  uint32_t restor = (val >> 12);
  if (order < 2 || restor > 2) {
    ec = bela::make_error_code(L"PPMd compressed data corrupted");
    return false;
  }
  if (!Ppmd8_Alloc(&_ppmd, mem << 20, &g_BigAlloc)) {
    ec = bela::make_error_code(L"Allocate Memory Failed");
    return false;
  }
  if (!Ppmd8_Init_RangeDec(&_ppmd)) {
    ec = bela::make_error_code(L"Ppmd8_RangeDec_Init");
    return false;
  }
  Ppmd8_Init(&_ppmd, order, restor);
  Buffer out(BufferSize);
  Summator sum(file.crc32sum);
  for (;;) {
    auto ob = out.data();
    auto size = BufferSize;
    size_t i = 0;
    do {
      auto sym = Ppmd8_DecodeSymbol(&_ppmd);
      if (sym < 0) {
        break;
      }
      ob[i] = static_cast<uint8_t>(sym);
      i++;
    } while (i != size);
    if (i == 0) {
      break;
    }
    sum.Update(ob, i);
    if (!w(ob, i)) {
      ec = bela::make_error_code(ErrCanceled, L"canceled");
      return false;
    }
    if (sr.AvailableBytes() == 0 && sr.Buffered() == 0) {
      break;
    }
  }
  if (!sum.Valid()) {
    ec = bela::make_error_code(ErrGeneral, L"crc32 want ", file.crc32sum, L" got ", sum.Current(), L" not match");
    return false;
  }
  return true;
}
} // namespace baulk::archive::zip
