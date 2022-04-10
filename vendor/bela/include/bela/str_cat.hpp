// ---------------------------------------------------------------------------
// Copyright (C) 2022, Bela contributors
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
#ifndef BELA_STRCAT_HPP
#define BELA_STRCAT_HPP
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include "__strings/string_cat_internal.hpp"

// -----------------------------------------------------------------------------
// StringCat()
// -----------------------------------------------------------------------------
//
// Merges given strings or numbers, using no delimiter(s).
//
// `StringCat()` is designed to be the fastest possible way to construct a
// string out of a mix of raw C strings, string_views, strings, bool values, and
// numeric values.
//
// Don't use `StringCat()` for user-visible strings. The localization process
// works poorly on strings built up out of fragments.
//
// For clarity and performance, don't use `StringCat()` when appending to a
// string. Use `StrAppend()` instead. In particular, avoid using any of these
// (anti-)patterns:
//
//   str.append(StringCat(...))
//   str += StringCat(...)
//   str = StringCat(str, ...)
//
// The last case is the worst, with a potential to change a loop
// from a linear time operation with O(1) dynamic allocations into a
// quadratic time operation with O(n) dynamic allocations.
//
// See `StrAppend()` below for more information.

namespace bela {

namespace strings_internal {
void AppendPieces(std::wstring *dest, std::initializer_list<std::wstring_view> pieces);

} // namespace strings_internal

[[nodiscard]] inline std::wstring StringCat() { return std::wstring(); }
[[nodiscard]] inline std::wstring StringCat(const AlphaNum &a) { return std::wstring(a.Piece()); }
[[nodiscard]] inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b) { return string_cat<wchar_t>(a, b); }
[[nodiscard]] inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c) {
  return string_cat<wchar_t>(a, b, c);
}
[[nodiscard]] inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
                                            const AlphaNum &d) {
  return string_cat<wchar_t>(a, b, c, d);
}
// Support 5 or more arguments
template <typename... AV>
[[nodiscard]] inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d,
                                            const AlphaNum &e, const AV &...args) {
  return strings_internal::string_cat_pieces<wchar_t>(
      {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(), static_cast<const AlphaNum &>(args).Piece()...});
}

inline void StrAppend(std::wstring *) {}
inline void StrAppend(std::wstring *dest, const AlphaNum &a) { dest->append(a.Piece()); }
inline void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b) { string_append(dest, a, b); }
inline void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c) {
  string_append(dest, a, b, c);
}
inline void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d) {
  string_append(dest, a, b, d);
}

// Support 5 or more arguments
template <typename... AV>
inline void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d,
                      const AlphaNum &e, const AV &...args) {
  strings_internal::string_append_pieces<wchar_t>(
      dest, {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(), static_cast<const AlphaNum &>(args).Piece()...});
}

// Narrow char StringCat and StringAppend
//
[[nodiscard]] inline std::string StringNarrowCat() { return std::string(); }
[[nodiscard]] inline std::string StringNarrowCat(const AlphaNumNarrow &a) { return std::string(a.Piece()); }
[[nodiscard]] inline std::string StringNarrowCat(const AlphaNumNarrow &a, const AlphaNumNarrow &b) {
  return string_cat<char>(a, b);
}
[[nodiscard]] inline std::string StringNarrowCat(const AlphaNumNarrow &a, const AlphaNumNarrow &b,
                                                 const AlphaNumNarrow &c) {
  return string_cat<char>(a, b, c);
}
[[nodiscard]] inline std::string StringNarrowCat(const AlphaNumNarrow &a, const AlphaNumNarrow &b,
                                                 const AlphaNumNarrow &c, const AlphaNumNarrow &d) {
  return string_cat<char>(a, b, c, d);
}

// Support 5 or more arguments
template <typename... AV>
[[nodiscard]] inline std::string StringNarrowCat(const AlphaNumNarrow &a, const AlphaNumNarrow &b,
                                                 const AlphaNumNarrow &c, const AlphaNumNarrow &d,
                                                 const AlphaNumNarrow &e, const AV &...args) {
  return strings_internal::string_cat_pieces<char>(
      {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(), static_cast<const AlphaNumNarrow &>(args).Piece()...});
}

// --------------
inline void StrAppend(std::string *) {}
inline void StrAppend(std::string *dest, const AlphaNumNarrow &a) {
  dest->append(a.Piece()); //
}
inline void StrAppend(std::string *dest, const AlphaNumNarrow &a, const AlphaNumNarrow &b) {
  string_append(dest, a, b);
}

inline void StrAppend(std::string *dest, const AlphaNumNarrow &a, const AlphaNumNarrow &b, const AlphaNumNarrow &c) {
  string_append(dest, a, b, c);
}

inline void StrAppend(std::string *dest, const AlphaNumNarrow &a, const AlphaNumNarrow &b, const AlphaNumNarrow &c,
                      const AlphaNumNarrow &d) {
  string_append(dest, a, b, d);
}

template <typename... AV>
inline void StrAppend(std::string *dest, const AlphaNumNarrow &a, const AlphaNumNarrow &b, const AlphaNumNarrow &c,
                      const AlphaNumNarrow &d, const AlphaNumNarrow &e, const AV &...args) {
  strings_internal::string_append_pieces<wchar_t>(dest, {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(),
                                                         static_cast<const AlphaNumNarrow &>(args).Piece()...});
}

} // namespace bela

#endif
