//////
#include <cstring>
#include <wchar.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include <bela/fmt.hpp>
#include <bela/codecvt.hpp>
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

const wchar_t *Decimal(uint64_t value, wchar_t *digits, bool sign) {
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
  return writer;
}

const wchar_t *AlphaNum(uint64_t value, wchar_t *digits, size_t width, int base, wchar_t fill, bool u) {
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
  return beg;
}

using StringWriter = Writer<std::wstring>;
using BufferWriter = Writer<buffer>;

/// because format string is Null-terminated_string
template <typename T> bool StrFormatInternal(Writer<T> &w, const wchar_t *fmt, const FormatArg *args, size_t max_args) {
  if (args == nullptr || max_args == 0) {
    return false;
  }
  wchar_t digits[kFastToBufferSize];
  const auto dend = digits + kFastToBufferSize;
  auto it = fmt;
  auto end = it + wcslen(fmt);
  size_t ca = 0;
  wchar_t pc;
  uint32_t width;
  uint32_t frac_width;
  bool left = false;
  while (it < end) {
    ///  Fast search %,
    auto pos = memsearch(it, end, '%');
    if (pos == npos) {
      w.Append(it, end - it);
      return !w.overflow();
    }
    w.Append(it, pos);
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
    switch (*it) {
    case 'b':
      if (ca >= max_args) {
        return false;
      }
      switch (args[ca].at) {
      case ArgType::BOOLEAN:
      case ArgType::CHARACTER:
        w.AddBoolean(args[ca].character.c != 0);
        break;
      case ArgType::INTEGER:
      case ArgType::UINTEGER:
        w.AddBoolean(args[ca].integer.i != 0);
        break;
      default:
        break;
      }
      ca++;
      break;
    case 'c':
      if (ca >= max_args) {
        return false;
      }
      switch (args[ca].at) {
      case ArgType::CHARACTER:
        w.AddUnicode(args[ca].character.c, width, args[ca].character.width);
        break;
      case ArgType::UINTEGER:
      case ArgType::INTEGER:
        w.AddUnicode(static_cast<char32_t>(args[ca].integer.i), width,
                     args[ca].integer.width > 2 ? 4 : args[ca].integer.width);
        break;
      default:
        break;
      }
      ca++;
      break;
    case 's':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at == ArgType::STRING) {
        w.Append(args[ca].strings.data, args[ca].strings.len, width, pc, left);
      } else if (args[ca].at == ArgType::USTRING) {
        auto ws = bela::ToWide(args[ca].ustring.data, args[ca].ustring.len);
        w.Append(ws.data(), ws.size(), width, pc, left);
      }
      ca++;
      break;
    case 'd':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at != ArgType::STRING) {
        bool sign = false;
        size_t off = 0;
        auto val = args[ca].ToInteger(&sign);
        if (sign) {
          pc = ' '; /// when sign ignore '0
        }
        auto p = Decimal(val, digits + off, sign);
        w.Append(p, dend - p + off, width, pc, left);
      }
      ca++;
      break;
    case 'o':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at != ArgType::STRING) {
        auto val = args[ca].ToInteger();
        auto p = AlphaNum(val, digits, 0, 8);
        w.Append(p, dend - p, width, pc, left);
      }
      ca++;
      break;
    case 'x':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at != ArgType::STRING) {
        auto val = args[ca].ToInteger();
        auto p = AlphaNum(val, digits, 0, 16);
        w.Append(p, dend - p, width, pc, left);
      }
      ca++;
      break;
    case 'X':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at != ArgType::STRING) {
        auto val = args[ca].ToInteger();
        auto p = AlphaNum(val, digits, 0, 16, ' ', true);
        w.Append(p, dend - p, width, pc, left);
      }
      ca++;
      break;
    case 'U':
      if (ca >= max_args) {
        return false;
      }
      switch (args[ca].at) {
      case ArgType::CHARACTER:
        w.AddUnicodePoint(args[ca].character.c);
        break;
      case ArgType::INTEGER:
      case ArgType::UINTEGER:
        w.AddUnicodePoint(static_cast<char32_t>(args[ca].integer.i));
        break;
      default:
        break;
      }
      ca++;
      break;
    case 'f':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at == ArgType::FLOAT) {
        w.Floating(args[ca].floating.d, width, frac_width, pc);
      }
      ca++;
      break;
    case 'a':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at == ArgType::FLOAT) {
        union {
          double d;
          uint64_t i;
        } x;
        x.d = args[ca].floating.d;
        auto p = AlphaNum(x.i, digits, 0, 16);
        w.Append(p, dend - p, width, pc, left);
      }
      ca++;
      break;
    case 'p':
      if (ca >= max_args) {
        return false;
      }
      if (args[ca].at == ArgType::POINTER) {
        auto ptr = reinterpret_cast<ptrdiff_t>(args[ca].ptr);
        constexpr auto plen = sizeof(intptr_t) * 2;
        auto p = AlphaNum(ptr, digits, plen, 16, '0', true);
        w.Append(L"0x", 2);    /// Force append 0x to pointer
        w.Append(p, dend - p); // 0xffff00000;
      }
      ca++;
      break;
    default:
      // % and other
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
