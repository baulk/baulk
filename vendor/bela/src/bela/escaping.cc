// ---------------------------------------------------------------------------
// Copyright (C) 2021, Bela contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Includes work from abseil-cpp (https://github.com/abseil/abseil-cpp)
// with modifications.
//
// Copyright 2019 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ---------------------------------------------------------------------------
#include <bela/escaping.hpp>
#include <bela/ascii.hpp>
#include <bela/str_cat.hpp>

namespace bela {
constexpr wchar_t uhc[] = L"0123456789ABCDEF";
// 0xFFFF
inline void char16encodehex(uint16_t ch, std::wstring &dest) {
  dest += L"\\x";
  dest += uhc[ch / 4096];
  dest += uhc[(ch % 4096) / 256];
  dest += uhc[(ch % 256) / 16];
  dest += uhc[ch % 16];
}

inline bool is_octal_digit(wchar_t c) { return (L'0' <= c) && (c <= L'7'); }

inline int hex_digit_to_int(wchar_t c) {
  static_assert('0' == 0x30 && 'A' == 0x41 && 'a' == 0x61, "Character set must be ASCII.");
  // assert(bela::ascii_isxdigit(c));
  int x = static_cast<unsigned char>(c);
  if (x > '9') {
    x += 9;
  }
  return x & 0xf;
}

constexpr bool issurrogate(char32_t rune) { return (rune >= 0xD800 && rune <= 0xDFFF); }
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

bool CUnescapeInternal(std::wstring_view source, bool leave_nulls_escaped, wchar_t *dest, ptrdiff_t *dest_len,
                       std::wstring *error) {
  wchar_t *d = dest;
  const wchar_t *p = source.data();
  const wchar_t *end = p + source.size();
  const wchar_t *last_byte = end - 1;

  // Small optimization for case where source = dest and there's no escaping
  while (p == d && p < end && *p != '\\') {
    static_cast<void>(p++), d++;
  }

  while (p < end) {
    if (*p != '\\') {
      *d++ = *p++;
    } else {
      if (++p > last_byte) { // skip past the '\\'
        if (error) {
          *error = L"String cannot end with \\";
        }
        return false;
      }
      switch (*p) {
      case L'a':
        *d++ = L'\a';
        break;
      case L'b':
        *d++ = L'\b';
        break;
      case L'f':
        *d++ = L'\f';
        break;
      case L'n':
        *d++ = L'\n';
        break;
      case L'r':
        *d++ = L'\r';
        break;
      case L't':
        *d++ = L'\t';
        break;
      case L'v':
        *d++ = L'\v';
        break;
      case L'\\':
        *d++ = L'\\';
        break;
      case L'?':
        *d++ = L'\?';
        break; // \?  Who knew?
      case L'\'':
        *d++ = L'\'';
        break;
      case L'"':
        *d++ = L'\"';
        break;
      case L'0':
      case L'1':
      case L'2':
      case L'3':
      case L'4':
      case L'5':
      case L'6':
      case L'7': {
        // octal digit: 1 to 3 digits
        const wchar_t *octal_start = p;
        unsigned int ch = *p - L'0';
        if (p < last_byte && is_octal_digit(p[1])) {
          ch = ch * 8 + *++p - L'0';
        }
        if (p < last_byte && is_octal_digit(p[1])) {
          ch = ch * 8 + *++p - L'0'; // now points at last digit
        }
        if (ch > 0xff) {
          if (error) {
            *error =
                bela::StringCat(L"Value of \\", std::wstring_view(octal_start, p + 1 - octal_start), L" exceeds 0xff");
          }
          return false;
        }
        if ((ch == 0) && leave_nulls_escaped) {
          // Copy the escape sequence for the null character
          const ptrdiff_t octal_size = p + 1 - octal_start;
          *d++ = '\\';
          memmove(d, octal_start, octal_size * sizeof(wchar_t));
          d += octal_size;
          break;
        }
        *d++ = static_cast<wchar_t>(ch);
        break;
      }
      case L'x':
      case L'X': {
        if (p >= last_byte) {
          if (error) {
            *error = L"String cannot end with \\x";
          }
          return false;
        } else if (!bela::ascii_isxdigit(p[1])) {
          if (error) {
            *error = L"\\x cannot be followed by a non-hex digit";
          }
          return false;
        }
        unsigned int ch = 0;
        const wchar_t *hex_start = p;
        while (p < last_byte && bela::ascii_isxdigit(p[1]))
          // Arbitrarily many hex digits
          ch = (ch << 4) + hex_digit_to_int(*++p);
        if (ch > 0xFF) {
          if (error) {
            *error = bela::StringCat(L"Value of \\", std::wstring_view(hex_start, p + 1 - hex_start), L" exceeds 0xff");
          }
          return false;
        }
        if ((ch == 0) && leave_nulls_escaped) {
          // Copy the escape sequence for the null character
          const ptrdiff_t hex_size = p + 1 - hex_start;
          *d++ = '\\';
          memmove(d, hex_start, hex_size * sizeof(wchar_t));
          d += hex_size;
          break;
        }
        *d++ = static_cast<wchar_t>(ch);
        break;
      }
      case 'u': {
        // \uhhhh => convert 4 hex digits to UTF-8
        char32_t rune = 0;
        const wchar_t *hex_start = p;
        if (p + 4 >= end) {
          if (error) {
            *error = bela::StringCat(L"\\u must be followed by 4 hex digits: \\",
                                     std::wstring_view(hex_start, p + 1 - hex_start));
          }
          return false;
        }
        for (int i = 0; i < 4; ++i) {
          // Look one char ahead.
          if (bela::ascii_isxdigit(p[1])) {
            rune = (rune << 4) + hex_digit_to_int(*++p); // Advance p.
          } else {
            if (error) {
              *error = bela::StringCat(L"\\u must be followed by 4 hex digits: \\",
                                       std::wstring_view(hex_start, p + 1 - hex_start));
            }
            return false;
          }
        }
        if ((rune == 0) && leave_nulls_escaped) {
          // Copy the escape sequence for the null character
          *d++ = '\\';
          memmove(d, hex_start, 5 * sizeof(wchar_t)); // u0000
          d += 5;
          break;
        }
        d += char32tochar16(rune, reinterpret_cast<char16_t *>(d));
        break;
      }
      case 'U': {
        // \Uhhhhhhhh => convert 8 hex digits to UTF-8
        char32_t rune = 0;
        const wchar_t *hex_start = p;
        if (p + 8 >= end) {
          if (error) {
            *error = bela::StringCat(L"\\U must be followed by 8 hex digits: \\",
                                     std::wstring_view(hex_start, p + 1 - hex_start));
          }
          return false;
        }
        for (int i = 0; i < 8; ++i) {
          // Look one char ahead.
          if (bela::ascii_isxdigit(p[1])) {
            // Don't change rune until we're sure this
            // is within the Unicode limit, but do advance p.
            uint32_t newrune = (rune << 4) + hex_digit_to_int(*++p);
            if (newrune > 0x10FFFF) {
              if (error) {
                *error = bela::StringCat(L"Value of \\", std::wstring_view(hex_start, p + 1 - hex_start),
                                         L" exceeds Unicode limit (0x10FFFF)");
              }
              return false;
            } else {
              rune = newrune;
            }
          } else {
            if (error) {
              *error = bela::StringCat(L"\\U must be followed by 8 hex digits: \\",
                                       std::wstring_view(hex_start, p + 1 - hex_start));
            }
            return false;
          }
        }
        if ((rune == 0) && leave_nulls_escaped) {
          // Copy the escape sequence for the null character
          *d++ = '\\';
          memmove(d, hex_start, 9 * sizeof(wchar_t)); // U00000000
          d += 9;
          break;
        }
        d += char32tochar16(rune, reinterpret_cast<char16_t *>(d));
        break;
      }
      default: {
        if (error) {
          *error = bela::StringCat(L"Unknown escape sequence: \\", *p);
        }
        return false;
      }
      }
      p++; // read past letter we escaped
    }
  }
  *dest_len = d - dest;
  return true;
}

// Unescape string
// \u2082
//\U00002082
bool CUnescape(std::wstring_view source, std::wstring *dest, std::wstring *error) {
  dest->resize(source.size());
  ptrdiff_t dest_size = 0;
  if (!CUnescapeInternal(source, false, dest->data(), &dest_size, error)) {
    return false;
  }
  dest->erase(dest_size);
  return true;
}
/// Escape UTF16 text.
std::wstring CEscape(std::wstring_view src) {
  std::wstring dest;
  dest.reserve(src.size() * 3 / 2);
  bool last_hex_escape = false;
  for (auto c : src) {
    switch (c) {
    case L'\n':
      dest += L"\\n";
      break;
    case L'\r':
      dest += L"\\r";
      break;
    case L'\t':
      dest += L"\\t";
      break;
    case L'\"':
      dest += L"\\\"";
      break;
    case L'\'':
      dest += L"\\'";
      break;
    case '\\':
      dest += L"\\\\";
      break;
    default:
      /// because 0xD800~0xDFFF is
      // Note that if we emit \xNN and the src character after that is a hex
      // digit then that digit must be escaped too to prevent it being
      // interpreted as part of the character code by C.
      if (c < 0x80 && (!bela::ascii_isprint(c) || (last_hex_escape && bela::ascii_isxdigit(c)))) {
        auto ch = static_cast<uint8_t>(c);
        dest += L"\\x";
        dest += uhc[ch / 16];
        dest += uhc[ch % 16];
        last_hex_escape = true;
        continue;
      }
      dest += c;
      break;
    }
  }
  return dest;
}
} // namespace bela
