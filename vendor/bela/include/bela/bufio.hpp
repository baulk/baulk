//
#ifndef BELA_BUFIO_HPP
#define BELA_BUFIO_HPP
#include <algorithm>
#include "base.hpp"
#include "types.hpp"

namespace bela::bufio {
constexpr ssize_t default_buffer_size = 4096;
// Fixed capacity size bufio.Reader implementation
template <ssize_t Size = default_buffer_size> class Reader {
public:
  Reader(HANDLE r) : fd(r) {}
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  ssize_t Buffered() const { return w - r; }
  ssize_t Read(void *buffer, ssize_t len, bela::error_code &ec) {
    if (buffer == nullptr || len == 0) {
      ec = bela::make_error_code(L"short read");
      return -1;
    }
    if (r == w) {
      if (static_cast<size_t>(len) > sizeof(data)) {
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
      if (!fsread(data, sizeof(data), w, ec)) {
        return -1;
      }
      if (w == 0) {
        ec = bela::make_error_code(L"unexpected EOF");
        return -1;
      }
    }

    auto n = (std::min)(w - r, len);
    memcpy(buffer, data + r, n);
    r += n;
    return n;
  }
  ssize_t ReadFull(void *buffer, ssize_t len, bela::error_code &ec) {
    auto p = reinterpret_cast<uint8_t *>(buffer);
    ssize_t n = 0;
    for (; n < len;) {
      auto nn = Read(p + n, len - n, ec);
      if (nn == -1) {
        return -1;
      }
      n += nn;
    }
    if (n < len) {
      ec = bela::make_error_code(L"unexpected EOF");
      return -1;
    }
    return n;
  }

  constexpr int size() const { return Size; }

private:
  HANDLE fd;
  uint8_t data[Size];
  ssize_t w{0};
  ssize_t r{0};
  bool fsread(void *b, ssize_t len, ssize_t &rlen, bela::error_code &ec) {
    DWORD dwSize = {0};
    if (ReadFile(fd, b, static_cast<DWORD>(len), &dwSize, nullptr) != TRUE) {
      ec = bela::make_system_error_code(L"ReadFile: ");
      return false;
    }
    rlen = static_cast<ssize_t>(len);
    return true;
  }
};
} // namespace bela::bufio

#endif