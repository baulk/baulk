// Enhanced string type functions
#ifndef BELA_STRINGS_HPP
#define BELA_STRINGS_HPP
#include <string>
#include <string_view>
#include "ascii.hpp"

namespace bela {
inline char16_t ascii_tolower(char16_t c) { return (c > 0xFF ? c : ascii_internal::kToLower[c]); }
inline char16_t ascii_toupper(char16_t c) { return (c > 0xFF ? c : ascii_internal::kToUpper[c]); }
inline char8_t ascii_tolower(char8_t c) { return static_cast<char8_t>(ascii_internal::kToLower[c]); }
inline char8_t ascii_toupper(char8_t c) { return static_cast<char8_t>(ascii_internal::kToUpper[c]); }

// Returns std::u16string_view with whitespace stripped from the beginning of the
// given u16string_view.
inline std::u16string_view StripLeadingAsciiWhitespace(std::u16string_view str) {
  // auto it = std::find_if_not(str.begin(), str.end(), ascii_isspace);
  for (auto it = str.begin(); it != str.end(); it++) {
    if (!ascii_isspace(static_cast<wchar_t>(*it))) {
      str.remove_prefix(it - str.begin());
      break;
    }
  }
  return str;
}

// Returns std::u8string_view with whitespace stripped from the beginning of the
// given u8string_view.
inline std::u8string_view StripLeadingAsciiWhitespace(std::u8string_view str) {
  // auto it = std::find_if_not(str.begin(), str.end(), ascii_isspace);
  for (auto it = str.begin(); it != str.end(); it++) {
    if (!ascii_isspace(static_cast<char8_t>(*it))) {
      str.remove_prefix(it - str.begin());
      break;
    }
  }
  return str;
}

// Strips in place whitespace from the beginning of the given u8string.
inline void StripLeadingAsciiWhitespace(std::u8string *str) {
  // auto it = std::find_if_not(str->begin(), str->end(), ascii_isspace);
  for (auto it = str->begin(); it != str->end(); it++) {
    if (!ascii_isspace(*it)) {
      str->erase(str->begin(), it);
      break;
    }
  }
}
// Strips in place whitespace from the beginning of the given u16string.
inline void StripLeadingAsciiWhitespace(std::u16string *str) {
  // auto it = std::find_if_not(str->begin(), str->end(), ascii_isspace);
  for (auto it = str->begin(); it != str->end(); it++) {
    if (!ascii_isspace(static_cast<wchar_t>(*it))) {
      str->erase(str->begin(), it);
      break;
    }
  }
}

} // namespace bela

#endif