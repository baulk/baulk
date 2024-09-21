//////////
#ifndef BELA_CODECVT_HPP
#define BELA_CODECVT_HPP
#include <string>
#include <vector>
#include <span>
#include "types.hpp"

/*
 * UTF-8 to UTF-16
 * Table from https://woboq.com/blog/utf-8-processing-using-simd.html
 *
 * +-------------------------------------+-------------------+
 * | UTF-8                               | UTF-16LE (HI LO)  |
 * +-------------------------------------+-------------------+
 * | 0aaaaaaa                            | 00000000 0aaaaaaa |
 * +-------------------------------------+-------------------+
 * | 110bbbbb 10aaaaaa                   | 00000bbb bbaaaaaa |
 * +-------------------------------------+-------------------+
 * | 1110cccc 10bbbbbb 10aaaaaa          | ccccbbbb bbaaaaaa |
 * +-------------------------------------+-------------------+
 * | 11110ddd 10ddcccc 10bbbbbb 10aaaaaa | 110110uu uuccccbb |
 * + uuuu = ddddd - 1                    | 110111bb bbaaaaaa |
 * +-------------------------------------+-------------------+
 */

namespace bela {
constexpr bool rune_is_surrogate(char32_t rune) { return (rune >= 0xD800 && rune <= 0xDFFF); }

constexpr const size_t kMaxEncodedUTF8Size = 4;
constexpr const size_t kMaxEncodedUTF16Size = 2;

template <typename CharT = char8_t>
  requires bela::u8_character<CharT>
[[nodiscard]] constexpr size_t encode_into_unchecked(char32_t rune, CharT *dest) {
  if (rune <= 0x7F) {
    dest[0] = static_cast<CharT>(rune);
    return 1;
  }
  if (rune <= 0x7FF) {
    dest[0] = static_cast<CharT>(0xC0 | ((rune >> 6) & 0x1F));
    dest[1] = static_cast<CharT>(0x80 | (rune & 0x3F));
    return 2;
  }
  if (rune <= 0xFFFF) {
    dest[0] = static_cast<CharT>(0xE0 | ((rune >> 12) & 0x0F));
    dest[1] = static_cast<CharT>(0x80 | ((rune >> 6) & 0x3F));
    dest[2] = static_cast<CharT>(0x80 | (rune & 0x3F));
    return 3;
  }
  if (rune <= 0x10FFFF) {
    dest[0] = static_cast<CharT>(0xF0 | ((rune >> 18) & 0x07));
    dest[1] = static_cast<CharT>(0x80 | ((rune >> 12) & 0x3F));
    dest[2] = static_cast<CharT>(0x80 | ((rune >> 6) & 0x3F));
    dest[3] = static_cast<CharT>(0x80 | (rune & 0x3F));
    return 4;
  }
  return 0;
}

template <typename CharT = char16_t>
  requires bela::u16_character<CharT>
[[nodiscard]] constexpr size_t encode_into_unchecked(char32_t rune, CharT *dest) {
  if (rune <= 0xFFFF) {
    dest[0] = rune_is_surrogate(rune) ? 0xFFFD : static_cast<CharT>(rune);
    return 1;
  }
  if (rune > 0x0010FFFF) {
    dest[0] = 0xFFFD;
    return 1;
  }
  dest[0] = static_cast<CharT>(0xD7C0 + (rune >> 10));
  dest[1] = static_cast<CharT>(0xDC00 + (rune & 0x3FF));
  return 2;
}

template <typename CharT = char8_t>
  requires bela::u8_character<CharT>
[[nodiscard]] constexpr std::basic_string_view<CharT, std::char_traits<CharT>> encode_into(char32_t rune, CharT *dest,
                                                                                           size_t len) {
  using string_view_t = std::basic_string_view<CharT, std::char_traits<CharT>>;
  if (rune <= 0x7F) {
    if (len == 0) {
      return string_view_t();
    }
    dest[0] = static_cast<CharT>(rune);
    return string_view_t(dest, 1);
  }
  if (rune <= 0x7FF) {
    if (len < 2) {
      return string_view_t();
    }
    dest[0] = static_cast<CharT>(0xC0 | ((rune >> 6) & 0x1F));
    dest[1] = static_cast<CharT>(0x80 | (rune & 0x3F));
    return string_view_t(dest, 2);
  }
  if (rune <= 0xFFFF) {
    if (len < 3) {
      return string_view_t();
    }
    dest[0] = static_cast<CharT>(0xE0 | ((rune >> 12) & 0x0F));
    dest[1] = static_cast<CharT>(0x80 | ((rune >> 6) & 0x3F));
    dest[2] = static_cast<CharT>(0x80 | (rune & 0x3F));
    return string_view_t(dest, 3);
  }
  if (rune <= 0x10FFFF && len >= 4) {
    dest[0] = static_cast<CharT>(0xF0 | ((rune >> 18) & 0x07));
    dest[1] = static_cast<CharT>(0x80 | ((rune >> 12) & 0x3F));
    dest[2] = static_cast<CharT>(0x80 | ((rune >> 6) & 0x3F));
    dest[3] = static_cast<CharT>(0x80 | (rune & 0x3F));
    return string_view_t(dest, 4);
  }
  return string_view_t();
}

template <typename CharT = char16_t>
  requires bela::u16_character<CharT>
[[nodiscard]] constexpr std::basic_string_view<CharT, std::char_traits<CharT>> encode_into(char32_t rune, CharT *dest,
                                                                                           size_t len) {
  using string_view_t = std::basic_string_view<CharT, std::char_traits<CharT>>;
  if (len == 0) {
    return string_view_t();
  }
  if (rune <= 0xFFFF) {
    dest[0] = rune_is_surrogate(rune) ? 0xFFFD : static_cast<CharT>(rune);
    return string_view_t(dest, 1);
  }
  if (rune > 0x0010FFFF) {
    dest[0] = 0xFFFD;
    return string_view_t(dest, 1);
  }
  if (len < 2) {
    return string_view_t();
  }
  dest[0] = static_cast<CharT>(0xD7C0 + (rune >> 10));
  dest[1] = static_cast<CharT>(0xDC00 + (rune & 0x3FF));
  return string_view_t(dest, 2);
}

template <typename CharT = char8_t, size_t N>
  requires bela::character<CharT>
[[nodiscard]] constexpr std::basic_string_view<CharT, std::char_traits<CharT>> encode_into(char32_t rune,
                                                                                           CharT (&dest)[N]) {
  return encode_into<CharT>(rune, dest, N);
}

// bela codecvt code
namespace codecvt_internal {

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
 * left as-is for anyone who may want to do such conversion, which was
 * allowed in earlier algorithms.
 */
// clang-format off
static constexpr const char trailing_bytes_from_utf8[256] = {
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
constexpr const char32_t offset_from_utf8[6] = {0x00000000UL, 0x00003080UL, 0x000E2080UL,
                                                0x03C82080UL, 0xFA082080UL, 0x82082080UL};

constexpr char32_t decode_rune(const char8_t *it, int nbytes) {
  char32_t ch = 0;
  switch (nbytes) {
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
  ch -= offset_from_utf8[nbytes];
  return ch;
}
} // namespace codecvt_internal

// Encode UTF8 to UTF16
template <typename From, typename To, typename Allocator = std::allocator<To>>
  requires bela::u8_character<From> && bela::u16_character<To>
[[nodiscard]] std::basic_string<To, std::char_traits<To>, Allocator> encode_into(std::basic_string_view<From> sv) {
  using string_t = std::basic_string<To, std::char_traits<To>, Allocator>;
  auto it = reinterpret_cast<const char8_t *>(sv.data());
  auto end = it + sv.size();
  string_t us;
  us.reserve(sv.size());
  while (it < end) {
    uint16_t nb = codecvt_internal::trailing_bytes_from_utf8[static_cast<uint8_t>(*it)];
    if (nb >= end - it) {
      break;
    }
    // https://docs.microsoft.com/en-us/cpp/cpp/attributes?view=vs-2019
    auto rune = codecvt_internal::decode_rune(it, nb);
    it += nb + 1;
    if (rune <= 0xFFFF) {
      if (rune >= 0xD800 && rune <= 0xDBFF) {
        us += static_cast<To>(0xFFFD);
        continue;
      }
      us += static_cast<To>(rune);
      continue;
    }
    if (rune > 0x10FFFF) {
      us += static_cast<To>(0xFFFD);
      continue;
    }
    rune -= 0x10000U;
    us += static_cast<To>((rune >> 10) + 0xD800);
    us += static_cast<To>((rune & 0x3FF) + 0xDC00);
  }
  return us;
}

// Encode UTF16 to UTF8
template <typename From, typename To, typename Allocator = std::allocator<To>>
  requires bela::u16_character<From> && bela::u8_character<To>
[[nodiscard]] std::basic_string<To, std::char_traits<To>, Allocator> encode_into(std::basic_string_view<From> sv) {
  using string_t = std::basic_string<To, std::char_traits<To>, Allocator>;
  string_t s;
  s.reserve(sv.size());
  auto it = sv.data();
  auto end = it + sv.size();
  while (it < end) {
    char32_t rune = *it++;
    if (rune >= 0xD800 && rune <= 0xDBFF) {
      if (it >= end) {
        return s;
      }
      char32_t rune2 = *it;
      if (rune2 < 0xDC00 || rune2 > 0xDFFF) {
        break;
      }
      rune = ((rune - 0xD800) << 10) + (rune2 - 0xDC00) + 0x10000U;
      ++it;
    }
    if (rune <= 0x7F) {
      s += static_cast<To>(rune);
      continue;
    }
    if (rune <= 0x7FF) {
      s += static_cast<To>(0xC0 | ((rune >> 6) & 0x1F));
      s += static_cast<To>(0x80 | (rune & 0x3F));
      continue;
    }
    if (rune <= 0xFFFF) {
      s += static_cast<To>(0xE0 | ((rune >> 12) & 0x0F));
      s += static_cast<To>(0x80 | ((rune >> 6) & 0x3F));
      s += static_cast<To>(0x80 | (rune & 0x3F));
      continue;
    }
    if (rune <= 0x10FFFF) {
      s += static_cast<To>(0xF0 | ((rune >> 18) & 0x07));
      s += static_cast<To>(0x80 | ((rune >> 12) & 0x3F));
      s += static_cast<To>(0x80 | ((rune >> 6) & 0x3F));
      s += static_cast<To>(0x80 | (rune & 0x3F));
      continue;
    }
  }
  return s;
}

template <typename To, typename Allocator = std::allocator<To>>
  requires bela::u16_character<To>
[[nodiscard]] std::basic_string<To, std::char_traits<To>, Allocator> encode_into(std::u32string_view sv) {
  std::basic_string<To, std::char_traits<To>, Allocator> s;
  s.reserve(sv.size());
  for (auto rune : sv) {
    if (rune <= 0xFFFF) {
      s += rune_is_surrogate(rune) ? 0xFFFD : static_cast<To>(rune);
      continue;
    }
    if (rune > 0x0010FFFF) {
      s += 0xFFFD;
      continue;
    }
    s += static_cast<To>(0xD7C0 + (rune >> 10));
    s += static_cast<To>(0xDC00 + (rune & 0x3FF));
  }
  return s;
}

template <typename To, typename Allocator = std::allocator<To>>
  requires bela::u8_character<To>
[[nodiscard]] std::basic_string<To, std::char_traits<To>, Allocator> encode_into(std::u32string_view sv) {
  std::basic_string<To, std::char_traits<To>, Allocator> s;
  s.reserve(sv.size());
  for (auto rune : sv) {
    if (rune <= 0x7F) {
      s += static_cast<To>(rune);
      continue;
    }
    if (rune <= 0x7FF) {
      s += static_cast<To>(0xC0 | ((rune >> 6) & 0x1F));
      s += static_cast<To>(0x80 | (rune & 0x3F));
      continue;
    }
    if (rune <= 0xFFFF) {
      s += static_cast<To>(0xE0 | ((rune >> 12) & 0x0F));
      s += static_cast<To>(0x80 | ((rune >> 6) & 0x3F));
      s += static_cast<To>(0x80 | (rune & 0x3F));
      continue;
    }
    if (rune <= 0x10FFFF) {
      s += static_cast<To>(0xF0 | ((rune >> 18) & 0x07));
      s += static_cast<To>(0x80 | ((rune >> 12) & 0x3F));
      s += static_cast<To>(0x80 | ((rune >> 6) & 0x3F));
      s += static_cast<To>(0x80 | (rune & 0x3F));
      continue;
    }
  }
  return s;
}

template <typename CharT = char8_t>
  requires bela::u16_character<CharT>
[[nodiscard]] constexpr size_t string_length(std::basic_string_view<CharT, std::char_traits<CharT>> sv) {
  size_t len = 0;
  auto it = sv.data();
  auto end = it + sv.size();
  while (it < end) {
    if (char32_t rune = *it++; rune >= 0xD800 && rune <= 0xDBFF) {
      if (it >= end) {
        break;
      }
      if (char32_t rune2 = *it; rune2 < 0xDC00 || rune2 > 0xDFFF) {
        break;
      }
      ++it;
    }
    len++;
  }
  return len;
}

template <typename CharT = char8_t>
  requires bela::u8_character<CharT>
[[nodiscard]] constexpr size_t string_length(std::basic_string_view<CharT, std::char_traits<CharT>> sv) {
  size_t len = 0;
  auto it = sv.data();
  auto end = it + sv.size();
  while (it < end) {
    unsigned short nb = codecvt_internal::trailing_bytes_from_utf8[static_cast<uint8_t>(*it)];
    if (nb >= end - it) {
      break;
    }
    len++;
    it += nb + 1;
  }
  return len;
}

template <typename CharT = char8_t, size_t N>
  requires bela::character<CharT>
[[nodiscard]] constexpr size_t string_length(CharT (&str)[N]) {
  // string_length
  return string_length<CharT>({str, N});
}

// CodePoint rune width
size_t rune_width(char32_t rune);

// Calculate UTF16 string width under terminal (Monospace font)
template <typename CharT = char16_t>
  requires bela::u16_character<CharT>
size_t string_width(std::basic_string_view<CharT, std::char_traits<CharT>> sv) {
  size_t width = 0;
  auto it = sv.data();
  auto end = it + sv.size();
  while (it < end) {
    char32_t rune = *it++;
    if (rune >= 0xD800 && rune <= 0xDBFF) {
      if (it >= end) {
        break;
      }
      char32_t rune2 = *it;
      if (rune2 < 0xDC00 || rune2 > 0xDFFF) {
        break;
      }
      rune = ((rune - 0xD800) << 10) + (rune2 - 0xDC00) + 0x10000U;
      ++it;
    }
    width += rune_width(rune);
  }
  return width;
}

// Calculate UTF8 string width under terminal (Monospace font)
template <typename CharT = char8_t>
  requires bela::u8_character<CharT>
size_t string_width(std::basic_string_view<CharT, std::char_traits<CharT>> sv) {
  size_t width = 0;
  auto it = reinterpret_cast<const char8_t *>(sv.data());
  auto end = it + sv.size();
  while (it < end) {
    unsigned short nb = codecvt_internal::trailing_bytes_from_utf8[static_cast<uint8_t>(*it)];
    if (nb >= end - it) {
      break;
    }
    width += rune_width(codecvt_internal::decode_rune(it, nb));
    it += nb + 1;
  }
  return width;
}

template <typename CharT = char8_t, size_t N>
  requires bela::character<CharT>
[[nodiscard]] constexpr size_t string_width(CharT (&str)[N]) {
  // string_length
  return string_width<CharT>({str, N});
}

inline char32_t RuneNext(std::u16string_view &sv) {
  if (sv.empty()) {
    return 0;
  }
  char32_t rune = sv[0];
  if (!bela::rune_is_surrogate(rune)) {
    sv.remove_prefix(1);
    return rune;
  }
  char32_t rune_part = sv[1];
  sv.remove_prefix(2);
  return ((rune - 0xD800) << 10) + (rune_part - 0xDC00) + 0x10000U;
}

inline char32_t RuneNext(std::u8string_view &sv) {
  if (sv.empty()) {
    return 0;
  }
  char32_t rune = sv[0];
  if (rune < 128) {
    sv.remove_prefix(1);
    return rune;
  }
  auto nbytes = codecvt_internal::trailing_bytes_from_utf8[static_cast<uint8_t>(rune)];
  if (sv.size() < static_cast<size_t>(nbytes)) {
    sv.remove_prefix(1);
    return 0;
  }
  rune = codecvt_internal::decode_rune(sv.data(), nbytes);
  sv.remove_prefix(nbytes + 1);
  return rune;
}

} // namespace bela

#endif
