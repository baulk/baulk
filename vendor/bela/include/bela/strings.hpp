// ---------------------------------------------------------------------------
// Copyright © 2025, Bela contributors
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
#ifndef BELA_STRINGS_HPP
#define BELA_STRINGS_HPP
#include <cstring>
#include <cstdlib>
#include <cwctype>
#include <cwchar>
#include "macros.hpp"
#include "types.hpp"
#include "ascii.hpp"

namespace bela {

// https://docs.microsoft.com/en-us/cpp/intrinsics/intrinsics-available-on-all-architectures?view=msvc-170
// __builtin_memcpy
// __builtin_wmemcmp
// __builtin_wcslen
// __builtin_wmemchr
// __builtin_u8strlen
// __builtin_u8memchr
// __builtin_char_memchr
//

#if BELA_HAVE_BUILTIN(__builtin_memcmp) || (defined(__GNUC__) && !defined(__clang__)) ||                               \
    (defined(_MSC_VER) && _MSC_VER >= 1928)
constexpr bool BytesEqual(const void *b1, const void *b2, size_t size) { return __builtin_memcmp(b1, b2, size) == 0; }
#else
inline bool BytesEqual(const void *b1, const void *b2, size_t size) { return memcmp(b1, b2, size) == 0; }
#endif

#if BELA_HAVE_BUILTIN(__builtin_memcpy) || (defined(__GNUC__) && !defined(__clang__))
template <typename T>
  requires bela::standard_layout<T>
constexpr auto MemCopy(T *dest, const T *src, size_t n) noexcept {
  if (n != 0) {
    __builtin_memcpy(dest, src, sizeof(T) * n);
  }
}
#else
// MSVC no __builtin_memcpy
template <typename T>
  requires bela::standard_layout<T>
inline void MemCopy(T *dest, const T *src, size_t n) noexcept {
  if (n != 0) {
    memcpy(dest, src, sizeof(T) * n);
  }
}
#endif

constexpr const size_t MaximumPos = static_cast<size_t>(-1);
#if BELA_HAVE_BUILTIN(__builtin_memcmp) || (defined(__GNUC__) && !defined(__clang__)) ||                               \
    (defined(_MSC_VER) && _MSC_VER >= 1928)
constexpr size_t CharFind(const wchar_t *begin, const wchar_t *end, wchar_t ch) {
  if (auto p = __builtin_wmemchr(begin, ch, end - begin); p != nullptr) {
    return p - begin;
  }
  return MaximumPos;
}
#else
inline size_t CharFind(const wchar_t *begin, const wchar_t *end, wchar_t ch) {
  if (auto p = wmemchr(begin, ch, end - begin); p != nullptr) {
    return p - begin;
  }
  return MaximumPos;
}
#endif

constexpr size_t CharFind(const char16_t *begin, const char16_t *end, char16_t ch) {
  for (auto p = begin; p != end; p++) {
    if (*p == ch) {
      return p - begin;
    }
  }
  return MaximumPos;
}

#if BELA_HAVE_BUILTIN(__builtin_char_memchr) || (defined(__GNUC__) && !defined(__clang__)) ||                          \
    (defined(_MSC_VER) && _MSC_VER >= 1928)
constexpr size_t CharFind(const char *begin, const char *end, char ch) {
  if (auto p = __builtin_char_memchr(begin, ch, end - begin); p != nullptr) {
    return p - begin;
  }
  return MaximumPos;
}
#else
inline size_t CharFind(const char *begin, const char *end, char ch) {
  if (auto p = memchr(begin, ch, end - begin); p != nullptr) {
    return reinterpret_cast<const char8_t *>(p) - begin;
  }
  return MaximumPos;
}
#endif

// clang no __builtin_u8memchr
#if BELA_HAVE_BUILTIN(__builtin_u8memchr) || (defined(_MSC_VER) && _MSC_VER >= 1928 && !defined(__clang__))
constexpr size_t CharFind(const char8_t *begin, const char8_t *end, char8_t ch) {
  if (auto p = __builtin_u8memchr(begin, ch, end - begin); p != nullptr) {
    return p - begin;
  }
  return MaximumPos;
}
#else
inline size_t CharFind(const char8_t *begin, const char8_t *end, char8_t ch) {
  if (auto p = memchr(begin, ch, end - begin); p != nullptr) {
    return reinterpret_cast<const char8_t *>(p) - begin;
  }
  return MaximumPos;
}
#endif

constexpr char16_t ascii_tolower(char16_t c) { return (c > 0x7F ? c : ascii_internal::kToLower[c]); }
constexpr char16_t ascii_toupper(char16_t c) { return (c > 0x7F ? c : ascii_internal::kToUpper[c]); }
constexpr char8_t ascii_tolower(char8_t c) { return static_cast<char8_t>(ascii_internal::kToLower[c]); }
constexpr char8_t ascii_toupper(char8_t c) { return static_cast<char8_t>(ascii_internal::kToUpper[c]); }

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