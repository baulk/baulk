////
#ifndef BELA_FMTWRITER_INTERNAL_HPP
#define BELA_FMTWRITER_INTERNAL_HPP
#include <cstring>
#include <wchar.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include <bitset>
#include <bela/fmt.hpp>
#include <bela/codecvt.hpp>

namespace bela::format_internal {
using namespace std::string_view_literals;
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
  buffer &append(std::wstring_view str) {
    if (len + str.size() < cap) {
      memcpy(data + len, str.data(), str.size() * sizeof(wchar_t));
      len += str.size();
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
std::wstring_view u16string_view_cast(uint64_t value, wchar_t *digits, size_t width, int base, wchar_t fill = ' ',
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
  Writer &Append(std::wstring_view str, size_t width = 0, wchar_t kc = ' ', bool la = false) {
    auto len = str.size();
    if (width < len) {
      c.append(str);
      return *this;
    }
    if (la) {
      c.append(str);
      fill_n(kc, width - len);
      return *this;
    }
    fill_n(kc, width - len);
    c.append(str);
    return *this;
  }

  Writer &Add(wchar_t ch) {
    c.push_back(ch);
    return *this;
  }

  // Add unicode
  Writer &AddUnicode(char32_t ch, size_t width, wchar_t kc = ' ', bool la = false) {
    wchar_t digits[bela::kMaxEncodedUTF16Size + 2];
    return Append({digits, bela::encode_into_unchecked(ch, digits)}, width, kc, la);
  }
  // Add unicode point
  Writer &AddUnicodePoint(char32_t ch) {
    wchar_t digits[kFastToBufferSize + 1];
    const auto dend = digits + kFastToBufferSize;
    auto val = static_cast<uint32_t>(ch);
    if (val > 0xFFFF) {
      Append(L"U+"sv);
      Append(u16string_view_cast(val, digits, 8, 16, '0', true));
      return *this;
    }
    Append(L"u+"sv);
    Append(u16string_view_cast(val, digits, 4, 16, '0', true));
    return *this;
  }
  Writer &AddBinary(const FormatArg &a, size_t width, wchar_t kc = '0') {
    if (!a.is_integral_superset()) {
      return *this;
    }
    if (kc == ' ') {
      kc = '0';
    }
    wchar_t digits[72];
    auto u16string_view_binary_cast = [&](uint64_t v) -> std::wstring_view {
      auto end = digits + 72;
      auto writer = end;
      for (;;) {
        *--writer = L'0' + (v & 0x1);
        v >>= 1;
        if (v == 0) {
          break;
        }
      }
      return {writer, static_cast<size_t>(end - writer)};
    };
    Append(L"0b"sv);
    Append(u16string_view_binary_cast(a.uint64_cast()), width, kc);
    return *this;
  }
  // Add boolean
  Writer &AddBoolean(bool b) {
    if (b) {
      Append(L"true"sv);
    } else {
      Append(L"false"sv);
    }
    return *this;
  }

  Writer &Floating(double d, size_t width, size_t frac_width, wchar_t pc) {
    if (std::signbit(d)) {
      Add('-');
      d = -d;
    }
    if (std::isnan(d)) {
      Append(L"nan"sv);
      return *this;
    }
    if (std::isinf(d)) {
      Append(L"inf"sv);
      return *this;
    }
    wchar_t digits[kFastToBufferSize];
    const auto dend = digits + kFastToBufferSize;
    auto ui64 = static_cast<int64_t>(d);
    uint64_t frac = 0;
    uint32_t scale = 0;
    if (frac_width > 0) {
      scale = 1;
      for (size_t n = frac_width; n != 0; n--) {
        scale *= 10;
      }
      frac = (uint64_t)(std::round((d - (double)ui64) * scale));
      if (frac == scale) {
        ui64++;
        frac = 0;
      }
    }
    Append(u16string_view_cast(ui64, digits, width, 10, pc));
    if (frac_width != 0) {
      Add('.');
      Append(u16string_view_cast(frac, digits, frac_width, 10, pc));
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
