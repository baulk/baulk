//////
#include <cstring>
#include <cwchar>
#include <algorithm>
#include <vector>
#include <cmath>
#include <bela/fmt.hpp>
#include <bela/codecvt.hpp>
#include <bela/numbers.hpp>
#include <bela/strings.hpp>
#include "writer.hpp"

namespace bela {
namespace format_internal {
using StringWriter = Writer<std::wstring>;
using BufferWriter = Writer<buffer_view>;

/// because format string is Null-terminated_string
template <typename T>
bool StrFormatInternal(Writer<T> &w, const std::wstring_view fmt, const FormatArg *args, size_t max_args) {
  if (args == nullptr || max_args == 0) {
    return false;
  }
  auto it = fmt.data();
  auto end = it + fmt.size();
  size_t ca = 0;
  wchar_t pc;
  size_t width;
  size_t frac_width;
  bool align_left = false;

  while (it < end) {
    ///  Fast search %,
    auto pos = CharFind(it, end, '%');
    if (pos == bela::MaximumPos) {
      w.append({it, static_cast<size_t>(end - it)});
      return !w.overflow();
    }
    w.append({it, pos});
    pc = ' ';
    /// fmt endswith '\0'
    it += pos + 1;
    while (*it == '0' && it < end) {
      it++;
      pc = '0';
    }
    if (it >= end) {
      break;
    }

    align_left = (*it == '-');
    if (align_left) {
      it++;
      pc = ' ';
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
      w.append('%');
      w.append(*it);
      it++;
      continue;
    }
    switch (*it) {
    case 'b':
      w.append_boolean(args[ca], width, pc, align_left);
      ca++;
      break;
    case 'c':
      w.append_character(args[ca], width, pc, align_left);
      ca++;
      break;
    case 's':
      w.append_string_unified(args[ca], width, pc, align_left);
      ca++;
      break;
    case 'd':
      w.append_fast_numeric(args[ca], width, pc, align_left);
      ca++;
      break;
    case 'o':
      w.append_numeric(args[ca], 8, width, frac_width, pc, align_left);
      ca++;
      break;
    case 'x':
      w.append_numeric(args[ca], 16, width, frac_width, pc, align_left);
      ca++;
      break;
    case 'X':
      w.append_numeric_hex(args[ca], width, frac_width, pc, align_left);
      ca++;
      break;
    case 'U':
      w.append_unicode_point(args[ca], width, pc, align_left);
      ca++;
      break;
    case 'f':
      if (args[ca].type == __types::__float) {
        w.append_fixed(args[ca].floating.d, width, frac_width, pc, align_left);
      } else {
        w.append(L"%!f");
      }
      ca++;
      break;
    case 'e':
      if (args[ca].type == __types::__float) {
        w.append_scientific(args[ca].floating.d, width, frac_width, pc, align_left);
      } else {
        w.append(L"%!e");
      }
      ca++;
      break;
    case 'a':
      if (args[ca].type == __types::__float) {
        w.append_double_hex(args[ca].floating.d, width, frac_width, pc, align_left, false);
      } else {
        w.append(L"%!f");
      }
      ca++;
      break;
    case 'v':
      w.append_unified(args[ca], width, frac_width, pc, align_left);
      ca++;
      break;
    case 'p':
      w.append_pointer(args[ca], width, pc, align_left);
      ca++;
      break;
    case '%':
      w.append(L'%');
      break;
    default:
      // % and other
      w.append(L'%');
      w.append(*it);
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
  buffer_view buffer_(buf, N);
  BufferWriter bw(buffer_);
  if (!StrFormatInternal(bw, fmt, args, max_args)) {
    return -1;
  }
  return static_cast<ssize_t>(buffer_.length());
}
} // namespace format_internal

ssize_t StrFormat(wchar_t *buf, size_t N, const wchar_t *fmt) {
  format_internal::buffer_view buffer_(buf, N);
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
