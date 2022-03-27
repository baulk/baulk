//////
#include <cstring>
#include <cwchar>
#include <algorithm>
#include <vector>
#include <cmath>
#include <bela/fmt.hpp>
#include <bela/codecvt.hpp>
#include <bela/numbers.hpp>
#include "fmtwriter.hpp"

namespace bela {
namespace format_internal {
constexpr const size_t npos = static_cast<size_t>(-1);
size_t memsearch(const wchar_t *begin, const wchar_t *end, int ch) {
  for (auto it = begin; it != end; it++) {
    if (*it == ch) {
      return it - begin;
    }
  }
  return npos;
}

std::wstring_view u16string_view_decimal_cast(uint64_t value, wchar_t *digits, bool sign) {
  wchar_t *const end = digits + kFastToBufferSize;
  wchar_t *writer = end;
  constexpr const wchar_t dec[] = L"0123456789";
  do {
    *--writer = dec[value % 10];
    value = value / 10;
  } while (value != 0);
  if (sign) {
    *--writer = L'-';
  }
  return {writer, static_cast<size_t>(end - writer)};
}

std::wstring_view u16string_view_cast(uint64_t value, wchar_t *digits, size_t width, int base, wchar_t fill, bool u) {
  wchar_t *const end = digits + kFastToBufferSize;
  wchar_t *writer = end;
  constexpr const wchar_t hex[] = L"0123456789abcdef";
  constexpr const wchar_t uhex[] = L"0123456789ABCDEF";
  auto w = (std::min)(width, kFastToBufferSize);
  switch (base) {
  case 8:
    do {
      *--writer = static_cast<wchar_t>('0' + (value & 0x7));
      value >>= 3;
    } while (value != 0);
    break;
  case 16:
    if (u) {
      do {
        *--writer = uhex[value & 0xF];
        value >>= 4;
      } while (value != 0);
    } else {
      do {
        *--writer = hex[value & 0xF];
        value >>= 4;
      } while (value != 0);
    }
    break;
  default:
    do {
      *--writer = hex[value % 10];
      value = value / 10;
    } while (value != 0);
    break;
  }
  wchar_t *beg;
  if ((size_t)(end - writer) < w) {
    beg = end - w;
    std::fill_n(beg, writer - beg, fill);
  } else {
    beg = writer;
  }
  return {beg, static_cast<size_t>(end - beg)};
}

using StringWriter = Writer<std::wstring>;
using BufferWriter = Writer<buffer>;

/// because format string is Null-terminated_string
template <typename T>
bool StrFormatInternal(Writer<T> &w, const std::wstring_view fmt, const FormatArg *args, size_t max_args) {
  if (args == nullptr || max_args == 0) {
    return false;
  }
  wchar_t digits[kFastToBufferSize];
  const auto dend = digits + kFastToBufferSize;
  auto it = fmt.data();
  auto end = it + fmt.size();
  size_t ca = 0;
  wchar_t pc;
  size_t width;
  size_t frac_width;
  bool left = false;
  while (it < end) {
    ///  Fast search %,
    auto pos = memsearch(it, end, '%');
    if (pos == npos) {
      w.Append({it, static_cast<size_t>(end - it)});
      return !w.overflow();
    }
    w.Append({it, pos});
    /// fmt endswith '\0'
    it += pos + 1;
    if (it >= end) {
      break;
    }
    left = (*it == '-');
    if (left) {
      it++;
      pc = ' ';
    } else {
      pc = (*it == '0') ? '0' : ' ';
    }
    // Parse ---
    width = 0;
    frac_width = 0;
    while (*it >= '0' && *it <= '9') {
      width = width * 10 + (*it++ - '0');
    }
    if (*it == '.') {
      it++;
      while (*it >= '0' && *it <= '9') {
        frac_width = frac_width * 10 + (*it++ - '0');
      }
    }
    if (ca >= max_args) {
      w.Add('%');
      w.Add(*it);
      it++;
      continue;
    }
    switch (*it) {
    case 'B':
      w.AddBinary(args[ca], width, pc);
      ca++;
      break;
    case 'b':
      switch (args[ca].type) {
      case __types::__boolean:
      case __types::__character:
        w.AddBoolean(args[ca].character.c != 0);
        break;
      case __types::__integral:
      case __types::__unsigned_integral:
        w.AddBoolean(args[ca].integer.i != 0);
        break;
      default:
        break;
      }
      ca++;
      break;
    case 'c':
      switch (args[ca].type) {
      case __types::__character:
        w.AddUnicode(args[ca].character.c, width, pc);
        break;
      case __types::__unsigned_integral:
        [[fallthrough]];
      case __types::__integral:
        w.AddUnicode(static_cast<char32_t>(args[ca].integer.i), width, pc);
        break;
      default:
        break;
      }
      ca++;
      break;
    case 's':
      switch (args[ca].type) {
      case __types::__u16strings:
        w.Append(args[ca].u16string_view_cast(), width, pc, left);
        break;
      case __types::__u8strings:
        w.Append(bela::encode_into<char8_t, wchar_t>(args[ca].u8string_view_cast()), width, pc, left);
        break;
      case __types::__u32strings:
        w.Append(bela::encode_into<wchar_t>(args[ca].u32string_view_cast()), width, pc, left);
        break;
      default:
        break;
      }
      ca++;
      break;
    case 'd':
      if (args[ca].is_integral_superset()) {
        bool sign = false;
        size_t off = 0;
        auto val = args[ca].uint64_cast(&sign);
        if (sign) {
          pc = ' '; /// when sign ignore '0
        }
        w.Append(u16string_view_decimal_cast(val, digits + off, sign), width, pc, left);
      }
      ca++;
      break;
    case 'o':
      if (args[ca].is_integral_superset()) {
        w.Append(u16string_view_cast(args[ca].uint64_cast(), digits, 0, 8), width, pc, left);
      }
      ca++;
      break;
    case 'x':
      if (args[ca].is_integral_superset()) {
        w.Append(u16string_view_cast(args[ca].uint64_cast(), digits, 0, 16), width, pc, left);
      }
      ca++;
      break;
    case 'X':
      if (args[ca].is_integral_superset()) {
        w.Append(u16string_view_cast(args[ca].uint64_cast(), digits, 0, 16, ' ', true), width, pc, left);
      }
      ca++;
      break;
    case 'U':
      switch (args[ca].type) {
      case __types::__character:
        w.AddUnicodePoint(args[ca].character.c);
        break;
      case __types::__integral:
        [[fallthrough]];
      case __types::__unsigned_integral:
        w.AddUnicodePoint(static_cast<char32_t>(args[ca].integer.i));
        break;
      default:
        break;
      }
      ca++;
      break;
    case 'f':
      if (args[ca].type == __types::__float) {
        w.Floating(args[ca].floating.d, width, frac_width, pc);
      }
      ca++;
      break;
    case 'a':
      if (args[ca].type == __types::__float) {
        union {
          double d;
          uint64_t i;
        } x;
        x.d = args[ca].floating.d;
        w.Append(u16string_view_cast(x.i, digits, 0, 16), width, pc, left);
      }
      ca++;
      break;
    case 'v':
      switch (args[ca].type) {
      case __types::__boolean:
        w.AddBoolean(args[ca].character.c != 0);
        break;
      case __types::__character:
        w.AddUnicode(args[ca].character.c, width, pc);
        break;
      case __types::__float:
        w.Append({digits, numbers_internal::SixDigitsToBuffer(args[ca].floating.d, digits)});
        break;
      case __types::__integral:
      case __types::__unsigned_integral: {
        bool sign = false;
        size_t off = 0;
        auto val = args[ca].uint64_cast(&sign);
        if (sign) {
          pc = ' '; /// when sign ignore '0
        }
        w.Append(u16string_view_decimal_cast(val, digits + off, sign), width, pc, left);
      } break;
      case __types::__u16strings:
        w.Append(args[ca].u16string_view_cast(), width, pc, left);
        break;
      case __types::__u8strings:
        w.Append(bela::encode_into<char8_t, wchar_t>(args[ca].u8string_view_cast()), width, pc, left);
        break;
      case __types::__u32strings:
        w.Append(bela::encode_into<wchar_t>(args[ca].u32string_view_cast()), width, pc, left);
        break;
      default:
        break;
      }
      ca++;
      break;
    case 'p':
      if (args[ca].type == __types::__pointer) {
        w.Append(L"0x"sv); // Force append 0x to pointer
        w.Append(u16string_view_cast(args[ca].value, digits, sizeof(intptr_t) * 2, sizeof(uintptr_t) * 2, '0', true));
      }
      ca++;
      break;
    default:
      // % and other
      w.Add('%');
      w.Add(*it);
      break;
    }
    it++;
  }
  return !w.overflow();
}

size_t StrAppendFormatInternal(std::wstring *buf, const wchar_t *fmt, const FormatArg *args, size_t max_args) {
  StringWriter sw(*buf);
  if (!StrFormatInternal(sw, fmt, args, max_args)) {
    return 0;
  }
  return static_cast<size_t>(buf->size());
}

std::wstring StrFormatInternal(const wchar_t *fmt, const FormatArg *args, size_t max_args) {
  std::wstring s;
  StringWriter sw(s);
  if (!StrFormatInternal(sw, fmt, args, max_args)) {
    return L"";
  }
  return s;
}

ssize_t StrFormatInternal(wchar_t *buf, size_t N, const wchar_t *fmt, const FormatArg *args, size_t max_args) {
  buffer buffer_(buf, N);
  BufferWriter bw(buffer_);
  if (!StrFormatInternal(bw, fmt, args, max_args)) {
    return -1;
  }
  return static_cast<ssize_t>(buffer_.length());
}
} // namespace format_internal

ssize_t StrFormat(wchar_t *buf, size_t N, const wchar_t *fmt) {
  format_internal::buffer buffer_(buf, N);
  std::wstring s;
  const wchar_t *src = fmt;
  for (; *src != 0; ++src) {
    buffer_.push_back(*src);
    if (src[0] == '%' && src[1] == '%') {
      ++src;
    }
  }
  return buffer_.overflow() ? -1 : static_cast<ssize_t>(buffer_.length());
}

size_t StrAppendFormat(std::wstring *buf, const wchar_t *fmt) {
  const wchar_t *src = fmt;
  for (; *src != 0; ++src) {
    buf->push_back(*src);
    if (src[0] == '%' && src[1] == '%') {
      ++src;
    }
  }
  return buf->size();
}

std::wstring StrFormat(const wchar_t *fmt) {
  std::wstring s;
  const wchar_t *src = fmt;
  for (; *src != 0; ++src) {
    s.push_back(*src);
    if (src[0] == '%' && src[1] == '%') {
      ++src;
    }
  }
  return s;
}
} // namespace bela
