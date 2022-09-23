///
#ifndef BELA_SRC__FORMAT__WRITER_HPP
#define BELA_SRC__FORMAT__WRITER_HPP
#include <cstring>
#include <wchar.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include <bitset>
#include <bela/codecvt.hpp>
#include <bela/charconv.hpp>
#include <bela/__format/args.hpp>
#include <bela/ascii.hpp>

namespace bela::format_internal {
class buffer_view {
public:
  buffer_view(wchar_t *data_, size_t cap_) : data(data_), cap(cap_) {}
  buffer_view(const buffer_view &) = delete;
  ~buffer_view() {
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
  buffer_view &append(std::wstring_view str) {
    if (len + str.size() < cap) {
      memcpy(data + len, str.data(), str.size() * sizeof(wchar_t));
      len += str.size();
      return *this;
    }
    ow = true;
    return *this;
  }
  buffer_view &append(const wchar_t *str, size_t dl) {
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

constexpr auto make_unsigned_unchecked(int64_t val, bool &sign) {
  if (sign = val < 0; sign) {
    return static_cast<uint64_t>(-val);
  }
  return static_cast<uint64_t>(val);
}

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
  // append a char
  void append(wchar_t ch) { c.push_back(ch); }
  void append(std::wstring_view sv) { c.append(sv); }
  void append(std::wstring_view sv, size_t width, wchar_t pc, bool align_left) {
    auto len = sv.size();
    if (width <= len) {
      c.append(sv);
      return;
    }
    if (align_left) {
      c.append(sv);
      fill_n(pc, width - len);
      return;
    }
    fill_n(pc, width - len);
    c.append(sv);
  }
  void append_signed_numeric(std::wstring_view sv, size_t width, wchar_t pc, bool align_left) {
    auto len = sv.size();
    if (width <= len + 1) {
      c.push_back(L'-');
      c.append(sv);
      return;
    }
    width--;
    if (align_left) {
      c.push_back(L'-');
      c.append(sv);
      fill_n(pc, width - len);
      return;
    }
    if (pc == '0') {
      c.push_back(L'-');
      fill_n(pc, width - len);
      c.append(sv);
      return;
    }
    fill_n(pc, width - len);
    c.push_back(L'-');
    c.append(sv);
  }

  void append_numeric_auto(std::wstring_view sv, size_t width, wchar_t pc, bool align_left, bool sign) {
    if (sign) {
      append_signed_numeric(sv, width, pc, align_left);
      return;
    }
    append(sv, width, pc, align_left);
  }

  // append double
  void append_fixed(double d, size_t width, size_t frac_width, wchar_t pc, bool align_left) {
    wchar_t buffer[64];
    bool sign = false;
    if (d < 0) {
      d = -d;
      sign = true;
    }
    if (auto sv = bela::to_chars_view(buffer, d, std::chars_format::fixed, static_cast<int>(frac_width)); !sv.empty()) {
      append_numeric_auto(sv, width, pc, align_left, sign);
    }
  }

  void append_scientific(double d, size_t width, size_t frac_width, wchar_t pc, bool align_left) {
    wchar_t buffer[64];
    bool sign = false;
    if (d < 0) {
      d = -d;
      sign = true;
    }
    if (auto sv = bela::to_chars_view(buffer, d, std::chars_format::scientific, static_cast<int>(frac_width));
        !sv.empty()) {
      append_numeric_auto(sv, width, pc, align_left, sign);
    }
  }
  void append_double_hex(double d, size_t width, size_t frac_width, wchar_t pc, bool align_left, bool uppercase) {
    wchar_t buffer[64];
    bool sign = false;
    if (d < 0) {
      d = -d;
      sign = true;
    }
    if (auto sv = bela::to_chars_view(buffer, d, std::chars_format::hex, static_cast<int>(frac_width)); !sv.empty()) {
      if (uppercase) {
        for (size_t i = 0; i < sv.size(); i++) {
          buffer[i] = bela::ascii_toupper(buffer[i]);
        }
      }
      append_numeric_auto(sv, width, pc, align_left, sign);
    }
  }

  // append unsigned int
  void append_fast_numeric(uint64_t i, size_t width, wchar_t pc, bool align_left) {
    wchar_t buffer[64];
    append(bela::to_chars_view(buffer, i), width, pc, align_left);
  }

  // append signed int
  void append_fast_numeric(int64_t i, size_t width, wchar_t pc, bool align_left) {
    wchar_t buffer[64];
    bool sign = false;
    auto sv = bela::to_chars_view(buffer, make_unsigned_unchecked(i, sign));
    append_numeric_auto(sv, width, pc, align_left, sign);
  }

  // append unsigned int
  void append_numeric(uint64_t i, int base, size_t width, wchar_t pc, bool align_left) {
    wchar_t buffer[64];
    append(bela::to_chars_view(buffer, i, base), width, pc, align_left);
  }

  // append signed int
  void append_numeric(int64_t i, int base, size_t width, wchar_t pc, bool align_left) {
    wchar_t buffer[64];
    bool sign = false;
    auto sv = bela::to_chars_view(buffer, make_unsigned_unchecked(i, sign), base);
    append_numeric_auto(sv, width, pc, align_left, sign);
  }

  // append unsigned int
  void append_numeric_hex(uint64_t i, size_t width, wchar_t pc, bool align_left) {
    wchar_t buffer[32];
    auto sv = bela::to_chars_view(buffer, i, 16);
    for (size_t i = 0; i < sv.size(); i++) {
      buffer[i] = bela::ascii_toupper(buffer[i]);
    }
    append(sv, width, pc, align_left);
  }

  // append signed int
  void append_numeric_hex(int64_t i, size_t width, wchar_t pc, bool align_left) {
    wchar_t buffer[32];
    bool sign = false;
    auto sv = bela::to_chars_view(buffer, make_unsigned_unchecked(i, sign), 16);
    for (size_t i = 0; i < sv.size(); i++) {
      buffer[i] = bela::ascii_toupper(buffer[i]);
    }
    append_numeric_auto(sv, width, pc, align_left, sign);
  }

  void append_unicode(char32_t ch, size_t width, wchar_t pc, bool align_left) {
    wchar_t digits[bela::kMaxEncodedUTF16Size + 2];
    append({digits, bela::encode_into_unchecked(ch, digits)}, width, pc, align_left);
  }

  void append_unified(const FormatArg &a, size_t width, size_t frac_width, wchar_t pc, bool align_left) {
    switch (a.type) {
    case __types::__boolean:
      // Avoid padding with zeros
      if (pc == '0') {
        pc = ' ';
      }
      append(a.character.c == 0 ? L"false" : L"true", width, pc, align_left);
      return;
    case __types::__character:
      append_unicode(a.character.c, width, pc, align_left);
      return;
    case __types::__float:
      append_fixed(a.floating.d, width, 6, pc, align_left);
      return;
    case __types::__unsigned_integral:
      append_fast_numeric(a.unsigned_integral.i, width, pc, align_left);
      return;
    case __types::__signed_integral:
      append_fast_numeric(a.signed_integral.i, width, pc, align_left);
      return;
    case __types::__pointer:
      return;
    case __types::__u16strings:
      append(a.make_u16string_view(), width, pc, align_left);
      return;
    case __types::__u8strings:
      append(bela::encode_into<char8_t, wchar_t>(a.make_u8string_view()), width, pc, align_left);
      return;
    case __types::__u32strings:
      append(bela::encode_into<wchar_t>(a.make_u32string_view()), width, pc, align_left);
      return;
    default:
      break;
    }
    c.append(L"%!v");
  }

  // append_boolean
  void append_boolean(const FormatArg &a, size_t width, wchar_t pc, bool align_left) {
    // Avoid padding with zeros
    if (pc == '0') {
      pc = ' ';
    }
    switch (a.type) {
    case __types::__boolean:
      [[fallthrough]];
    case __types::__character:
      append(a.character.c == 0 ? L"false" : L"true", width, pc, align_left);
      return;
    case __types::__signed_integral:
      append(a.signed_integral.i == 0 ? L"false" : L"true", width, pc, align_left);
      return;
    case __types::__unsigned_integral:
      append(a.unsigned_integral.i == 0 ? L"false" : L"true", width, pc, align_left);
      return;
    default:
      break;
    }
    c.append(L"%!b");
  }

  void append_character(const FormatArg &a, size_t width, wchar_t pc, bool align_left) {
    switch (a.type) {
    case __types::__character:
      append_unicode(a.character.c, width, pc, align_left);
      return;
    case __types::__signed_integral:
      append_unicode(static_cast<char32_t>(a.signed_integral.i), width, pc, align_left);
      return;
    case __types::__unsigned_integral:
      append_unicode(static_cast<char32_t>(a.unsigned_integral.i), width, pc, align_left);
      return;
    default:
      break;
    }
    c.append(L"%!c");
  }
  void append_string_unified(const FormatArg &a, size_t width, wchar_t pc, bool align_left) {
    switch (a.type) {
    case __types::__u16strings:
      append(a.make_u16string_view(), width, pc, align_left);
      return;
    case __types::__u8strings:
      append(bela::encode_into<char8_t, wchar_t>(a.make_u8string_view()), width, pc, align_left);
      return;
    case __types::__u32strings:
      append(bela::encode_into<wchar_t>(a.make_u32string_view()), width, pc, align_left);
      return;
    default:
      break;
    }
    c.append(L"%!s");
  }

  void append_numeric(const FormatArg &a, int base, size_t width, size_t frac_width, wchar_t pc, bool align_left) {
    switch (a.type) {
    case __types::__boolean:
      [[fallthrough]];
    case __types::__character:
      append_numeric(static_cast<uint64_t>(a.character.c), base, width, pc, align_left);
      return;
    case __types::__signed_integral:
      append_numeric(a.signed_integral.i, base, width, pc, align_left);
      return;
    case __types::__unsigned_integral:
      append_numeric(a.unsigned_integral.i, base, width, pc, align_left);
      return;
    case __types::__pointer:
      append_numeric(static_cast<uint64_t>(a.value), base, width, pc, align_left);
      return;
    case __types::__float:
      append_double_hex(a.floating.d, width, frac_width, pc, align_left, false);
      return;
    default:
      break;
    }
    if (base == 8) {
      c.append(L"%!o");
      return;
    }
    if (base == 16) {
      c.append(L"%!x");
      return;
    }
    c.append(L"%!?");
  }

  void append_fast_numeric(const FormatArg &a, size_t width, wchar_t pc, bool align_left) {
    switch (a.type) {
    case __types::__boolean:
      [[fallthrough]];
    case __types::__character:
      append_fast_numeric(static_cast<uint64_t>(a.character.c), width, pc, align_left);
      return;
    case __types::__signed_integral:
      append_fast_numeric(a.signed_integral.i, width, pc, align_left);
      return;
    case __types::__unsigned_integral:
      append_fast_numeric(a.unsigned_integral.i, width, pc, align_left);
      return;
    case __types::__pointer:
      append_fast_numeric(static_cast<uint64_t>(a.value), width, pc, align_left);
      return;
    case __types::__float:
      append_fast_numeric(static_cast<int64_t>(a.floating.d), width, pc, align_left);
      return;
    default:
      break;
    }
    c.append(L"%!?");
  }
  void append_numeric_hex(const FormatArg &a, size_t width, size_t frac_width, wchar_t pc, bool align_left) {
    switch (a.type) {
    case __types::__boolean:
      [[fallthrough]];
    case __types::__character:
      append_numeric_hex(static_cast<uint64_t>(a.character.c), width, pc, align_left);
      return;
    case __types::__signed_integral:
      append_numeric_hex(a.signed_integral.i, width, pc, align_left);
      return;
    case __types::__unsigned_integral:
      append_numeric_hex(a.unsigned_integral.i, width, pc, align_left);
      return;
    case __types::__pointer:
      append_numeric_hex(static_cast<uint64_t>(a.value), width, pc, align_left);
      return;
    case __types::__float:
      append_double_hex(a.floating.d, width, frac_width, pc, align_left, true);
      return;
    default:
      break;
    }
    c.append(L"%!X");
  }

  void append_unicode_point(const FormatArg &a, size_t width, wchar_t pc, bool align_left) {
    // Avoid padding with zeros
    if (pc == '0') {
      pc = ' ';
    }
    char32_t val = 0;
    switch (a.type) {
    case __types::__character:
      val = a.character.c;
      break;
    case __types::__signed_integral:
      val = static_cast<char32_t>(a.signed_integral.i);
      break;
    case __types::__unsigned_integral:
      val = static_cast<char32_t>(a.unsigned_integral.i);
      break;
    default:
      c.append(L"%!U");
      return;
    }
    wchar_t buffer[32];
    if (val > 0xFFFF) {
      buffer[0] = L'U';
    } else {
      buffer[0] = L'u';
    }
    buffer[1] = L'+';
    if (auto res = bela::to_chars(buffer + 2, buffer + 18, val, 16); res) {
      auto size_end = static_cast<size_t>(res.ptr - buffer);
      for (size_t i = 2; i < size_end; i++) {
        buffer[i] = bela::ascii_toupper(buffer[i]);
      }
      append(std::wstring_view{buffer, size_end}, width, pc, align_left);
    }
  }

  void append_pointer(const FormatArg &a, size_t width, wchar_t pc, bool align_left) {
    // Avoid padding with zeros
    if (pc == '0') {
      pc = ' ';
    }
    uint64_t val = 0;
    switch (a.type) {
    case __types::__signed_integral:
      val = static_cast<uint64_t>(a.signed_integral.i);
      break;
    case __types::__unsigned_integral:
      val = a.unsigned_integral.i;
      break;
    case __types::__pointer:
      val = a.value;
      break;
    default:
      c.append(L"%!p");
      return;
    }
    wchar_t buffer[32];
    buffer[0] = L'0';
    buffer[1] = L'x';
    if (auto res = bela::to_chars(buffer + 2, buffer + 32, val, 16); res) {
      auto size_end = static_cast<size_t>(res.ptr - buffer);
      for (size_t i = 2; i < size_end; i++) {
        buffer[i] = bela::ascii_toupper(buffer[i]);
      }
      append(std::wstring_view{buffer, size_end}, width, pc, align_left);
    }
  }
  bool overflow() const;

private:
  C &c;
};

template <> inline bool Writer<std::wstring>::overflow() const {
  // std::wstring can resize. so always return false
  return false;
}
template <> inline bool Writer<buffer_view>::overflow() const {
  // no allocated buffer need check overflow
  return c.overflow();
}

} // namespace bela::format_internal

#endif