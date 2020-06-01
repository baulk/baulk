//======================================================
// Ascii for wchar_t
// This implementation is strictly Ascii, if you want to convert a
// case-insensitive or other detection of a language greater than 0xFF, use
// <cwctype>
//======================================================
#ifndef BELA_ASCII_HPP
#define BELA_ASCII_HPP
#pragma once
#include <cctype>
#include <cstring>
#include <string>
#include <algorithm>

// https://devblogs.microsoft.com/oldnewthing/?p=94836
// What is __wchar_t (with the leading double underscores) and why am I getting
// errors about it? Since the intrinsic wchar_t is a 16-bit unsigned integer in
// Visual C++, it is binary-compatible with unsigned short,

namespace bela {
namespace ascii_internal {

template <typename T, size_t N> bool ArrayChr(T (&a)[N], T c) {
  for (auto i : a) {
    if (i == c) {
      return true;
    }
  }
  return false;
}

// Declaration for an array of bitfields holding character information.
extern const unsigned char kPropertyBits[256];

// Declaration for the array of characters to upper-case characters.
extern const char kToUpper[256];

// Declaration for the array of characters to lower-case characters.
extern const char kToLower[256];
} // namespace ascii_internal
// ascii_isalpha()
//
// Determines whether the given character is an alphabetic character.
inline bool ascii_isalpha(wchar_t c) {
  return (c < 0xFF && ascii_internal::kPropertyBits[c] & 0x01) != 0;
}

// ascii_isalnum()
//
// Determines whether the given character is an alphanumeric character.
inline bool ascii_isalnum(wchar_t c) {
  return (c < 0xFF && ascii_internal::kPropertyBits[c] & 0x04) != 0;
}

// ascii_isspace()
//
// Determines whether the given character is a whitespace character (space,
// tab, vertical tab, formfeed, linefeed, or carriage return).
inline bool ascii_isspace(wchar_t c) {
  // if (c > 0xFF) {
  // }
  // return (ascii_internal::kPropertyBits[c] & 0x08) != 0;
  wchar_t wspace[] = {' ',    '\t',   '\n',   '\r',   11,     12,     0x0085,
                      0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006,
                      0x2008, 0x2009, 0x200a, 0x2028, 0x2029, 0x205f, 0x3000};
  return ascii_internal::ArrayChr(wspace, c);
}

// ascii_ispunct()
//
// Determines whether the given character is a punctuation character.
inline bool ascii_ispunct(wchar_t c) {
  return (c < 0xFF && (ascii_internal::kPropertyBits[c] & 0x10) != 0);
}

// ascii_isblank()
//
// Determines whether the given character is a blank character (tab or space).
inline bool ascii_isblank(wchar_t c) {
  return (c < 0xFF && ascii_internal::kPropertyBits[c] & 0x20) != 0;
}

// ascii_iscntrl()
// wchar_t on Windows is 2Byte
// Determines whether the given character is a control character.
inline bool ascii_iscntrl(wchar_t c) {
  return (c < 0xFF && ascii_internal::kPropertyBits[c] & 0x40) != 0;
}

// ascii_isxdigit()
//
// Determines whether the given character can be represented as a hexadecimal
// digit character (i.e. {0-9} or {A-F}).
inline bool ascii_isxdigit(wchar_t c) {
  return (c < 0xFF && ascii_internal::kPropertyBits[c] & 0x80) != 0;
}

// ascii_isdigit()
//
// Determines whether the given character can be represented as a decimal
// digit character (i.e. {0-9}).
inline bool ascii_isdigit(wchar_t c) { return c >= '0' && c <= '9'; }

// ascii_isprint()
//
// Determines whether the given character is printable, including whitespace.
inline bool ascii_isprint(wchar_t c) { return c >= 32 && c < 127; }

// ascii_isgraph()
//
// Determines whether the given character has a graphical representation.
inline bool ascii_isgraph(wchar_t c) { return c > 32 && c < 127; }

// ascii_isupper()
//
// Determines whether the given character is uppercase.
inline bool ascii_isupper(wchar_t c) { return c >= 'A' && c <= 'Z'; }

// ascii_islower()
//
// Determines whether the given character is lowercase.
inline bool ascii_islower(wchar_t c) { return c >= 'a' && c <= 'z'; }

// ascii_isascii()
//
// Determines whether the given character is ASCII.
inline bool ascii_isascii(wchar_t c) { return c < 128; }

// ascii_tolower()
//
// Returns an ASCII character, converting to lowercase if uppercase is
// passed. Note that character values > 127 are simply returned.
inline char ascii_tolower(wchar_t c) { return (c > 0xFF ? c : ascii_internal::kToLower[c]); }

void AsciiStrToLower(std::wstring *s);
inline std::wstring AsciiStrToLower(std::wstring_view s) {
  std::wstring result(s);
  AsciiStrToLower(&result);
  return result;
}

inline wchar_t ascii_toupper(wchar_t c) { return (c > 0xFF ? c : ascii_internal::kToUpper[c]); }
// Converts the characters in `s` to uppercase, changing the contents of `s`.
void AsciiStrToUpper(std::wstring *s);

// Creates an uppercase string from a given std::wstring_view.
inline std::wstring AsciiStrToUpper(std::wstring_view s) {
  std::wstring result(s);
  AsciiStrToUpper(&result);
  return result;
}

// Returns std::wstring_view with whitespace stripped from the beginning of the
// given string_view.
inline std::wstring_view StripLeadingAsciiWhitespace(std::wstring_view str) {
  auto it = std::find_if_not(str.begin(), str.end(), ascii_isspace);
  return str.substr(it - str.begin());
}

// Strips in place whitespace from the beginning of the given string.
inline void StripLeadingAsciiWhitespace(std::wstring *str) {
  auto it = std::find_if_not(str->begin(), str->end(), ascii_isspace);
  str->erase(str->begin(), it);
}

// Returns std::wstring_view with whitespace stripped from the end of the given
// string_view.
inline std::wstring_view StripTrailingAsciiWhitespace(std::wstring_view str) {
  auto it = std::find_if_not(str.rbegin(), str.rend(), ascii_isspace);
  return str.substr(0, str.rend() - it);
}

// Strips in place whitespace from the end of the given string
inline void StripTrailingAsciiWhitespace(std::wstring *str) {
  auto it = std::find_if_not(str->rbegin(), str->rend(), ascii_isspace);
  str->erase(str->rend() - it);
}

// Returns std::wstring_view with whitespace stripped from both ends of the
// given string_view.
inline std::wstring_view StripAsciiWhitespace(std::wstring_view str) {
  return StripTrailingAsciiWhitespace(StripLeadingAsciiWhitespace(str));
}

// Strips in place whitespace from both ends of the given string
inline void StripAsciiWhitespace(std::wstring *str) {
  StripTrailingAsciiWhitespace(str);
  StripLeadingAsciiWhitespace(str);
}

// Removes leading, trailing, and consecutive internal whitespace.
void RemoveExtraAsciiWhitespace(std::wstring *);
} // namespace bela

#endif
