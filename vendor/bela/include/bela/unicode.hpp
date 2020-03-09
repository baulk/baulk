////
// Thanks https://github.com/BobSteagall/utf_utils
#ifndef BELA_UNICODE_HPP
#define BELA_UNICODE_HPP
#include <cstdint>
#include <cstddef>
#include <string>

namespace bela {
namespace unicode {
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0482r6.html R6
#ifndef _BELA_HAS_CHAR8_T // TRANSITION, LLVM#41063
// Clang can't mangle char8_t on Windows, but the feature-test macro is defined.
// This issue has been fixed for the 8.0.1 release.
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L &&                      \
    !(defined(__clang__) && __clang_major__ == 8 && __clang_minor__ == 0 &&    \
      __clang_patchlevel__ == 0)
#define _BELA_HAS_CHAR8_T 1
// char8_t exists
#else // ^^^ Compiler supports char8_t / doesn't support char8_t vvv
#define _BELA_HAS_CHAR8_T 0
using char8_t = unsigned char;
#endif // Detect char8_t
#endif // _BELA_HAS_CHAR8_T

using std::ptrdiff_t;
namespace details {
enum CharClass : uint8_t {
  ILL = 0,  //- C0..C1, F5..FF  ILLEGAL octets that should never appear in a
            // UTF-8 sequence
            //
  ASC = 1,  //- 00..7F          ASCII leading byte range
            //
  CR1 = 2,  //- 80..8F          Continuation range 1
  CR2 = 3,  //- 90..9F          Continuation range 2
  CR3 = 4,  //- A0..BF          Continuation range 3
            //
  L2A = 5,  //- C2..DF          Leading byte range A / 2-byte sequence
            //
  L3A = 6,  //- E0              Leading byte range A / 3-byte sequence
  L3B = 7,  //- E1..EC, EE..EF  Leading byte range B / 3-byte sequence
  L3C = 8,  //- ED              Leading byte range C / 3-byte sequence
            //
  L4A = 9,  //- F0              Leading byte range A / 4-byte sequence
  L4B = 10, //- F1..F3          Leading byte range B / 4-byte sequence
  L4C = 11, //- F4              Leading byte range C / 4-byte sequence
};
enum State : uint8_t {
  BGN = 0,   //- Start
  ERR = 12,  //- Invalid sequence
             //
  CS1 = 24,  //- Continuation state 1
  CS2 = 36,  //- Continuation state 2
  CS3 = 48,  //- Continuation state 3
             //
  P3A = 60,  //- Partial 3-byte sequence state A
  P3B = 72,  //- Partial 3-byte sequence state B
             //
  P4A = 84,  //- Partial 4-byte sequence state A
  P4B = 96,  //- Partial 4-byte sequence state B
             //
  END = BGN, //- Start and End are the same state!
  err = ERR, //- For readability in the state transition table
};

struct FirstUnitInfo {
  char8_t mFirstOctet;
  State mNextState;
};
struct alignas(2048) LookupTables {
  FirstUnitInfo maFirstUnitTable[256];
  CharClass maOctetCategory[256];
  State maTransitions[108];
  std::uint8_t maFirstOctetMask[16];
};

extern LookupTables const smTables;
extern char const *smClassNames[12];
extern char const *smStateNames[9];

inline int32_t advancewstable(char8_t const *&it, char8_t const *end,
                              char32_t &rune) noexcept {
  char32_t unit = 0;
  int32_t type = 0;
  int32_t curr = 0;
  unit = *it++;
  type =
      smTables
          .maOctetCategory[unit]; //- Get the first code unit's character class
  rune = smTables.maFirstOctetMask[type] & unit; //- Apply the first octet mask
  curr = smTables.maTransitions[type];           //- Look up the second state

  while (curr > ERR) {
    if (it < end) {
      unit = *it++; //- Cache the current code unit
      rune = (rune << 6) | (unit & 0x3F);
      //- Adjust code point with continuation bits
      type = smTables.maOctetCategory[unit];
      //- Look up the code unit's character class
      curr = smTables.maTransitions[curr + type]; //- Look up the next state
    } else {
      return ERR;
    }
  }
  return curr;
}

} // namespace details

inline std::uint32_t char32tochar8(char32_t rune, char8_t *dest) noexcept {
  if (rune <= 0x7F) {
    dest[0] = static_cast<char8_t>(rune);
    return 1;
  }
  if (rune <= 0x7FF) {
    dest[0] = static_cast<char8_t>(0xC0 | ((rune >> 6) & 0x1F));
    dest[1] = static_cast<char8_t>(0x80 | (rune & 0x3F));
    return 2;
  }
  if (rune <= 0xFFFF) {
    dest[0] = static_cast<char8_t>(0xE0 | ((rune >> 12) & 0x0F));
    dest[1] = static_cast<char8_t>(0x80 | ((rune >> 6) & 0x3F));
    dest[2] = static_cast<char8_t>(0x80 | (rune & 0x3F));
    return 3;
  }
  if (rune <= 0x10FFFF) {
    dest[0] = static_cast<char8_t>(0xF0 | ((rune >> 18) & 0x07));
    dest[1] = static_cast<char8_t>(0x80 | ((rune >> 12) & 0x3F));
    dest[2] = static_cast<char8_t>(0x80 | ((rune >> 6) & 0x3F));
    dest[3] = static_cast<char8_t>(0x80 | (rune & 0x3F));
    return 4;
  }
  return 0;
}

template <typename T, typename Allocator>
inline size_t
char32tochar8(char32_t rune,
              std::basic_string<T, std::char_traits<T>, Allocator> &dest) {
  static_assert(sizeof(T) == 1, "Only supports one-byte character basic types");
  if (rune <= 0x7F) {
    dest += static_cast<T>(rune);
    return 1;
  }
  if (rune <= 0x7FF) {
    dest += static_cast<T>(rune);
    dest += static_cast<T>(rune);
    return 2;
  }
  if (rune <= 0xFFFF) {
    dest += static_cast<T>(0xE0 | ((rune >> 12) & 0x0F));
    dest += static_cast<T>(0x80 | ((rune >> 6) & 0x3F));
    dest += static_cast<T>(0x80 | (rune & 0x3F));
    return 3;
  }
  if (rune <= 0x10FFFF) {
    dest += static_cast<T>(0xF0 | ((rune >> 18) & 0x07));
    dest += static_cast<T>(0x80 | ((rune >> 12) & 0x3F));
    dest += static_cast<T>(0x80 | ((rune >> 6) & 0x3F));
    dest += static_cast<T>(0x80 | (rune & 0x3F));
    return 4;
  }
  return 0;
}

inline constexpr bool issurrogate(char32_t rune) {
  return (rune >= 0xD800 && rune <= 0xDFFF);
}

inline size_t char32tochar16(char32_t rune, char16_t *dest) {
  if (rune <= 0xFFFF) {
    dest[0] = issurrogate(rune) ? 0xFFFD : static_cast<char16_t>(rune);
    return 1;
  }
  if (rune > 0x0010FFFF) {
    dest[0] = 0xFFFD;
    return 1;
  }
  dest[0] = static_cast<char16_t>(0xD7C0 + (rune >> 10));
  dest[1] = static_cast<char16_t>(0xDC00 + (rune & 0x3FF));
  return 2;
}

template <typename T = char16_t, typename Allocator>
inline size_t
char32tochar16(char32_t rune,
               std::basic_string<T, std::char_traits<T>, Allocator> &dest) {
  static_assert(sizeof(T) == 2, "Only supports one-byte character basic types");
  if (rune <= 0xFFFF) {
    dest.push_back(issurrogate(rune) ? 0xFFFD : static_cast<T>(rune));
    return 1;
  }
  if (rune > 0x0010FFFF) {
    dest += 0xFFFD;
    return 1;
  }
  dest += static_cast<T>(0xD7C0 + (rune >> 10));
  dest += static_cast<T>(0xDC00 + (rune & 0x3FF));
  return 2;
}

inline std::u16string mbrtoc16(std::string_view src) {
  std::u16string us;
  us.reserve(src.size());
  auto it = reinterpret_cast<const char8_t *>(src.data());
  auto end = it + src.size();
  char32_t rune = 0;
  while (it < end) {
    if (*it < 0x80) {
      us.push_back(*it++);
      continue;
    }
    if (details::advancewstable(it, end, rune) == details::ERR) {
      break;
    }
    char32tochar16(rune, us);
  }
  return us;
}

#ifdef _WIN32
// under Windows. wchar_t size 2Byte. UTF-16
inline std::wstring mbrtowc(std::string_view src) {
  std::wstring us;
  us.reserve(src.size());
  auto it = reinterpret_cast<const char8_t *>(src.data());
  auto end = it + src.size();
  char32_t rune = 0;
  while (it < end) {
    if (*it < 0x80) {
      us.push_back(*it++);
      continue;
    }
    if (details::advancewstable(it, end, rune) == details::ERR) {
      break;
    }
    char32tochar16(rune, us);
  }
  return us;
}
inline std::wstring ToWide(std::string_view src) { return mbrtowc(src); }
#else
inline std::wstring mbrtowc(std::string_view src) {
  std::wstring us;
  us.reserve(src.size());
  auto it = reinterpret_cast<const char8_t *>(src.data());
  auto end = it + src.size();
  char32_t rune = 0;
  while (it < end) {
    if (*it < 0x80) {
      us.push_back(*it++);
      continue;
    }
    if (details::advancewstable(it, end, rune) == details::ERR) {
      break;
    }
    us.push_back(static_cast<wchar_t>(rune));
  }
  return us;
}
inline std::u16string ToWide(std::string_view src) { return mbrtoc16(src); }
#endif

template <typename T = char16_t>
inline std::string u16tomb(const T *data, size_t len) {
  static_assert(sizeof(T) == 2, "Only supports 2Byte character basic types");
  std::string s;
  s.reserve(len * 4 / 2);
  auto it = reinterpret_cast<const char16_t *>(data);
  auto end = it + len;
  while (it < end) {
    char32_t rune = *it++;
    if (rune >= 0xD800 && rune <= 0xDBFF) {
      if (it >= end) {
        return s;
      }
      char32_t ch2 = *it;
      if (ch2 < 0xDC00 || ch2 > 0xDFFF) {
        /* We don't have the 16 bits following the high surrogate. */
        return s;
      }
      rune = ((rune - 0xD800) << 10) + (ch2 - 0xDC00) + 0x0010000UL;
      ++it;
    }
    char32tochar8(rune, s);
  }
  return s;
}
#ifdef _WIN32
inline std::string ToNarrow(std::wstring_view ws) {
  return u16tomb(ws.data(), ws.size());
}
#endif
inline std::string ToNarrow(std::u16string_view us) {
  return u16tomb(us.data(), us.size());
}

} // namespace unicode
} // namespace bela

#endif
