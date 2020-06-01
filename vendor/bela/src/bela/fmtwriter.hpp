////
#ifndef BELA_FMTWRITER_INTERNAL_HPP
#define BELA_FMTWRITER_INTERNAL_HPP
#include <cstring>
#include <wchar.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include <bela/fmt.hpp>
#include <bela/codecvt.hpp>

namespace bela::format_internal {
class buffer {
public:
  buffer(wchar_t *data_, size_t cap_) : data(data_), cap(cap_) {}
  buffer(const buffer &) = delete;
  ~buffer() {
    if (len < cap) {
      data[len] = 0;
    }
  }
  void push_back(wchar_t ch) {
    if (len < cap) {
      data[len++] = ch;
      return;
    }
    ow = true;
  }
  buffer &append(std::wstring_view s) {
    if (len + s.size() < cap) {
      memcpy(data + len, s.data(), s.size() * sizeof(wchar_t));
      len += s.size();
      return *this;
    }
    ow = true;
    return *this;
  }
  buffer &append(const wchar_t *str, size_t dl) {
    if (len + dl < cap) {
      memcpy(data + len, str, dl * sizeof(wchar_t));
      len += dl;
      return *this;
    }
    ow = true;
    return *this;
  }
  bool overflow() const { return ow; }
  size_t length() const { return len; }

private:
  wchar_t *data{nullptr};
  size_t len{0};
  size_t cap{0};
  bool ow{false};
};

constexpr const size_t kFastToBufferSize = 32;
const wchar_t *AlphaNum(uint64_t value, wchar_t *digits, size_t width, int base, wchar_t fill = ' ',
                        bool u = false);

template <typename C = std::wstring> class Writer {
public:
  Writer(C &c_) : c(c_) {}
  Writer(const Writer &) = delete;
  Writer &operator=(const Writer &) = delete;
  // fill_n
  void fill_n(wchar_t ch, size_t n) {
    for (size_t i = 0; i < n; i++) {
      c.push_back(ch);
    }
  }
  // append string
  Writer &Append(const wchar_t *data, size_t len, size_t width = 0, wchar_t kc = ' ',
                 bool la = false) {
    if (width < len) {
      c.append(data, len);
      return *this;
    }
    if (la) {
      c.append(data, len);
      fill_n(kc, width - len);
      return *this;
    }
    fill_n(kc, width - len);
    c.append(data, len);
    return *this;
  }
  Writer &Add(wchar_t ch) {
    c.push_back(ch);
    return *this;
  }
  // Add unicode
  Writer &AddUnicode(char32_t ch, size_t width, wchar_t kc = ' ', bool la = false) {
    constexpr size_t kMaxEncodedUTF16Size = 2;
    wchar_t digits[kMaxEncodedUTF16Size + 2];
    if (ch < 0xFFFF) {
      digits[0] = static_cast<wchar_t>(ch);
      return Append(digits, 1, width, kc, la);
    }
    auto n = char32tochar16(ch, reinterpret_cast<char16_t *>(digits), kMaxEncodedUTF16Size + 2);

    return Append(digits, n, width, kc, la);
  }
  // Add unicode point
  Writer &AddUnicodePoint(char32_t ch) {
    wchar_t digits[kFastToBufferSize + 1];
    const auto dend = digits + kFastToBufferSize;
    auto val = static_cast<uint32_t>(ch);
    if (val > 0xFFFF) {
      Append(L"U+", 2);
      auto p = AlphaNum(val, digits, 8, 16, '0', true);
      Append(p, dend - p);
      return *this;
    }
    Append(L"u+", 2);
    auto p = AlphaNum(val, digits, 4, 16, '0', true);
    Append(p, dend - p);
    return *this;
  }
  // Add boolean
  Writer &AddBoolean(bool b) {
    if (b) {
      Append(L"true", sizeof("true") - 1);
    } else {
      Append(L"false", sizeof("false") - 1);
    }
    return *this;
  }

  Writer &Floating(double d, uint32_t width, uint32_t frac_width, wchar_t pc) {
    if (std::signbit(d)) {
      Add('-');
      d = -d;
    }
    if (std::isnan(d)) {
      Append(L"nan", 3);
      return *this;
    }
    if (std::isinf(d)) {
      Append(L"inf", 3);
      return *this;
    }
    wchar_t digits[kFastToBufferSize];
    const auto dend = digits + kFastToBufferSize;
    auto ui64 = static_cast<int64_t>(d);
    uint64_t frac = 0;
    uint32_t scale = 0;
    if (frac_width > 0) {
      scale = 1;
      for (int n = frac_width; n != 0; n--) {
        scale *= 10;
      }
      frac = (uint64_t)(std::round((d - (double)ui64) * scale));
      if (frac == scale) {
        ui64++;
        frac = 0;
      }
    }
    auto p = AlphaNum(ui64, digits, width, 10, pc);
    Append(p, dend - p);
    if (frac_width != 0) {
      Add('.');
      p = AlphaNum(frac, digits, frac_width, 10, pc);
      Append(p, dend - p);
    }
    return *this;
  }
  bool overflow() const;

private:
  C &c;
};

template <> inline bool Writer<std::wstring>::overflow() const {
  // std::wstring can resize. so always return false
  return false;
}
template <> inline bool Writer<buffer>::overflow() const {
  // no allocated buffer need check overflow
  return c.overflow();
}
} // namespace bela::format_internal

#endif
