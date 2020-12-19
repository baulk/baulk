//////////
// see:
// https://github.com/llvm-mirror/llvm/blob/master/lib/Support/ConvertUTF.cpp
//
#include <bela/codecvt.hpp>

namespace bela {

namespace codecvt_internal {
template <typename T> class Literal;
template <> class Literal<char> {
public:
  static constexpr std::string_view Empty = "\"\"";
  static constexpr std::string_view UnicodePrefix = "\\U";
};
template <> class Literal<wchar_t> {
public:
#if defined(_MSC_VER) || defined(_LIBCPP_VERSION)
  static constexpr std::wstring_view Empty = L"\"\"";
  static constexpr std::wstring_view UnicodePrefix = L"\\U";
#else
  // libstdc++ call wcslen is bad
  static constexpr std::wstring_view Empty{L"\"\"", sizeof("\"\"") - 1};
  static constexpr std::wstring_view UnicodePrefix = {L"\\U", sizeof("\\U") - 1};
#endif
};
template <> class Literal<char16_t> {
public:
  static constexpr std::u16string_view Empty = u"\"\"";
  static constexpr std::u16string_view UnicodePrefix = u"\\U";
};
} // namespace codecvt_internal

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
 * left as-is for anyone who may want to do such conversion, which was
 * allowed in earlier algorithms.
 */
// clang-format off
static const char trailingbytesu8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};
// clang-format on
constexpr const char32_t offsetfromu8[6] = {0x00000000UL, 0x00003080UL, 0x000E2080UL,
                                            0x03C82080UL, 0xFA082080UL, 0x82082080UL};

// char32_t - type for UTF-32 character representation, required to be large
// enough to represent any UTF-32 code unit (32 bits). It has the same size,
// signedness, and alignment as std::uint_least32_t, but is a distinct type.

inline constexpr bool IsSurrogate(char32_t rune) { return (rune >= 0xD800 && rune <= 0xDFFF); }

size_t char32tochar16(char32_t rune, char16_t *dest, size_t dlen) {
  if (dlen == 0 || dest == nullptr) {
    return 0;
  }
  if (rune <= 0xFFFF) {
    dest[0] = IsSurrogate(rune) ? 0xFFFD : static_cast<char16_t>(rune);
    return 1;
  }
  if (rune > 0x0010FFFF) {
    dest[0] = 0xFFFD;
    return 1;
  }
  if (dlen < 2) {
    return 0;
  }
  dest[0] = static_cast<char16_t>(0xD7C0 + (rune >> 10));
  dest[1] = static_cast<char16_t>(0xDC00 + (rune & 0x3FF));
  return 2;
}

static inline size_t char32tochar8_internal(char32_t rune, char *dest) {
  if (rune <= 0x7F) {
    dest[0] = static_cast<char>(rune);
    return 1;
  }
  if (rune <= 0x7FF) {
    dest[0] = static_cast<char>(0xC0 | ((rune >> 6) & 0x1F));
    dest[1] = static_cast<char>(0x80 | (rune & 0x3F));
    return 2;
  }
  if (rune <= 0xFFFF) {
    dest[0] = static_cast<char>(0xE0 | ((rune >> 12) & 0x0F));
    dest[1] = static_cast<char>(0x80 | ((rune >> 6) & 0x3F));
    dest[2] = static_cast<char>(0x80 | (rune & 0x3F));
    return 3;
  }
  if (rune <= 0x10FFFF) {
    dest[0] = static_cast<char>(0xF0 | ((rune >> 18) & 0x07));
    dest[1] = static_cast<char>(0x80 | ((rune >> 12) & 0x3F));
    dest[2] = static_cast<char>(0x80 | ((rune >> 6) & 0x3F));
    dest[3] = static_cast<char>(0x80 | (rune & 0x3F));
    return 4;
  }
  return 0;
}

size_t char32tochar8(char32_t rune, char *dest, size_t dlen) {
  constexpr const size_t kMaxEncodedUTF8Size = 4;
  if (dlen < kMaxEncodedUTF8Size) {
    return 0;
  }
  return char32tochar8_internal(rune, dest);
}

std::string c16tomb(const char16_t *data, size_t len) {
  std::string s;
  s.reserve(len);
  auto it = data;
  auto end = it + len;
  char buffer[8] = {0};
  while (it < end) {
    char32_t ch = *it++;
    if (ch >= 0xD800 && ch <= 0xDBFF) {
      if (it >= end) {
        return s;
      }
      char32_t ch2 = *it;
      if (ch2 < 0xDC00 || ch2 > 0xDFFF) {
        break;
      }
      ch = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000U;
      ++it;
    }
    auto bw = char32tochar8_internal(ch, buffer);
    s.append(reinterpret_cast<const char *>(buffer), bw);
  }
  return s;
}

inline char32_t AnnexU8(const char8_t *it, int nb) {
  char32_t ch = 0;
  switch (nb) {
  case 5:
    ch += *it++;
    ch <<= 6; /* remember, illegal UTF-8 */
    [[fallthrough]];
  case 4:
    ch += *it++;
    ch <<= 6; /* remember, illegal UTF-8 */
    [[fallthrough]];
  case 3:
    ch += *it++;
    ch <<= 6;
    [[fallthrough]];
  case 2:
    ch += *it++;
    ch <<= 6;
    [[fallthrough]];
  case 1:
    ch += *it++;
    ch <<= 6;
    [[fallthrough]];
  case 0:
    ch += *it++;
  }
  ch -= offsetfromu8[nb];
  return ch;
}

template <typename T, typename Allocator>
bool mbrtoc16(const char8_t *s, size_t len, std::basic_string<T, std::char_traits<T>, Allocator> &container) {
  if (s == nullptr || len == 0) {
    return false;
  }
  container.reserve(len);
  auto it = reinterpret_cast<const char8_t *>(s);
  auto end = it + len;
  while (it < end) {
    unsigned short nb = trailingbytesu8[*it];
    if (nb >= end - it) {
      return true;
    }
    // https://docs.microsoft.com/en-us/cpp/cpp/attributes?view=vs-2019
    auto ch = AnnexU8(it, nb);
    it += nb + 1;
    if (ch <= 0xFFFF) {
      if (ch >= 0xD800 && ch <= 0xDBFF) {
        container += static_cast<T>(0xFFFD);
        continue;
      }
      container += static_cast<T>(ch);
      continue;
    }
    if (ch > 0x10FFFF) {
      container += static_cast<T>(0xFFFD);
      continue;
    }
    ch -= 0x10000U;
    container += static_cast<T>((ch >> 10) + 0xD800);
    container += static_cast<T>((ch & 0x3FF) + 0xDC00);
  }
  //
  return true;
}

std::wstring mbrtowc(const char8_t *str, size_t len) {
  std::wstring s;
  if (!mbrtoc16(str, len, s)) {
    s.clear();
  }
  return s;
}
std::u16string mbrtoc16(const char8_t *str, size_t len) {
  std::u16string s;
  if (!mbrtoc16(str, len, s)) {
    s.clear();
  }
  return s;
}

template <size_t N, typename T> inline std::basic_string_view<T> EncodeUnicode(T (&buf)[N], char32_t ch) {
  T *end = buf + N;
  T *writer = end;
  uint64_t value = ch;
  static const char hexdigits[] = "0123456789ABCDEF";
  do {
    *--writer = static_cast<T>(hexdigits[value & 0xF]);
    value >>= 4;
  } while (value != 0);

  T *beg;
  if (end - writer < 8) {
    beg = end - 8;
    std::fill_n(beg, writer - beg, '0');
  } else {
    beg = writer;
  }
  return std::basic_string_view<T>(beg, end - beg);
}

// EscapeNonBMP UTF-8
std::string EscapeNonBMP(std::string_view sv) {
  if (sv.empty()) {
    return "";
  }
  std::string s;
  s.reserve(sv.size());
  char ub[10] = {0};
  auto len = sv.size();
  auto it = reinterpret_cast<const char8_t *>(sv.data());
  auto end = it + len;
  while (it < end) {
    auto c = *it;
    if (c == '\\') {
      s += "\\\\";
      it++;
      continue;
    }
    if (c < 0x80) {
      s += c;
      it++;
      continue;
    }
    unsigned short nb = trailingbytesu8[*it];
    if (nb >= end - it) {
      return s;
    }
    auto ch = AnnexU8(it, nb);
    if (ch <= 0xFFFF) {
      s.append(reinterpret_cast<const char *>(it), nb + 1);
      it += nb + 1;
      continue;
    }
    s.append(codecvt_internal::Literal<char>::UnicodePrefix).append(EncodeUnicode(ub, ch));
    it += nb + 1;
  }
  return s;
}

template <typename T> std::basic_string<T> EscapeNonBMPInternal(std::u16string_view sv) {
  std::basic_string<T> s;
  s.reserve(sv.size());
  auto it = sv.data();
  auto end = it + sv.size();
  T buffer[10] = {0};
  while (it < end) {
    char32_t ch = *it++;
    if (ch >= 0xD800 && ch <= 0xDBFF) {
      if (it >= end) {
        return s;
      }
      char32_t ch2 = *it;
      if (ch2 < 0xDC00 || ch2 > 0xDFFF) {
        break;
      }
      ch = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000U;
      ++it;
    }
    if (ch < 0xFFFF) {
      s += static_cast<T>(ch);
      continue;
    }
    s.append(codecvt_internal::Literal<T>::UnicodePrefix).append(EncodeUnicode(buffer, ch));
  }
  return s;
}

// EscapeNonBMP UTF-16
std::wstring EscapeNonBMP(std::wstring_view sv) {
  return EscapeNonBMPInternal<wchar_t>({reinterpret_cast<const char16_t *>(sv.data()), sv.size()});
}
std::u16string EscapeNonBMP(std::u16string_view sv) { return EscapeNonBMPInternal<char16_t>(sv); }

// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
// https://vt100.net/

// Calculate UTF-8 string display width
size_t StringWidth(std::string_view str) {
  size_t width = 0;
  auto it = reinterpret_cast<const char8_t *>(str.data());
  auto end = it + str.size();
  while (it < end) {
    if (*it == 0x1b) {
      while (it < end) {
        // We only support strip ANSI color
        if (*it == 'm') {
          it++;
          break;
        }
        it++;
      }
      if (it >= end) {
        break;
      }
    }
    unsigned short nb = trailingbytesu8[*it];
    if (nb >= end - it) {
      break;
    }
    auto ch = AnnexU8(it, nb);
    it += nb + 1;
    width += CalculateWidth(ch);
  }
  return 0;
}

// Calculate UTF-16 string display width
size_t StringWidth(std::u16string_view str) {
  size_t width = 0;
  auto it = str.data();
  auto end = it + str.size();
  while (it < end) {
    if (*it == 0x1b) {
      while (it < end) {
        // We only support strip ANSI color
        if (*it == L'm') {
          it++;
          break;
        }
        it++;
      }
      if (it >= end) {
        break;
      }
    }
    char32_t ch = *it++;
    if (ch >= 0xD800 && ch <= 0xDBFF) {
      if (it >= end) {
        break;
      }
      char32_t ch2 = *it;
      if (ch2 < 0xDC00 || ch2 > 0xDFFF) {
        break;
      }
      ch = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000U;
      ++it;
    }
    width += CalculateWidth(ch);
  }
  return width;
}

} // namespace bela
