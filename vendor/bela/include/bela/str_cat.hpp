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
#ifndef BELA_STRCAT_HPP
#define BELA_STRCAT_HPP
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include "numbers.hpp"
#include "codecvt.hpp"

namespace bela {
namespace strings_internal {
template <size_t max_size> struct AlphaNumBuffer {
  std::array<wchar_t, max_size> data;
  size_t size;
};

} // namespace strings_internal
inline std::wstring_view NullSafeStringView(const wchar_t *str) {
  return (str == nullptr) ? std::wstring_view() : std::wstring_view(str);
}
// Enum that specifies the number of significant digits to return in a `Hex` or
// `Dec` conversion and fill character to use. A `kZeroPad2` value, for example,
// would produce hexadecimal strings such as "0a","0f" and a 'kSpacePad5' value
// would produce hexadecimal strings such as "    a","    f".
enum PadSpec : uint8_t {
  kNoPad = 1,
  kZeroPad2,
  kZeroPad3,
  kZeroPad4,
  kZeroPad5,
  kZeroPad6,
  kZeroPad7,
  kZeroPad8,
  kZeroPad9,
  kZeroPad10,
  kZeroPad11,
  kZeroPad12,
  kZeroPad13,
  kZeroPad14,
  kZeroPad15,
  kZeroPad16,
  kZeroPad17,
  kZeroPad18,
  kZeroPad19,
  kZeroPad20,

  kSpacePad2 = kZeroPad2 + 64,
  kSpacePad3,
  kSpacePad4,
  kSpacePad5,
  kSpacePad6,
  kSpacePad7,
  kSpacePad8,
  kSpacePad9,
  kSpacePad10,
  kSpacePad11,
  kSpacePad12,
  kSpacePad13,
  kSpacePad14,
  kSpacePad15,
  kSpacePad16,
  kSpacePad17,
  kSpacePad18,
  kSpacePad19,
  kSpacePad20,
};

// -----------------------------------------------------------------------------
// Hex
// -----------------------------------------------------------------------------
//
// `Hex` stores a set of hexadecimal string conversion parameters for use
// within `AlphaNum` string conversions.
struct Hex {
  uint64_t value;
  uint8_t width;
  wchar_t fill;

  template <typename Int>
  explicit Hex(Int v, PadSpec spec = bela::kNoPad,
               typename std::enable_if<sizeof(Int) == 1 && !std::is_pointer<Int>::value>::type * = nullptr)
      : Hex(spec, static_cast<uint8_t>(v)) {}
  template <typename Int>
  explicit Hex(Int v, PadSpec spec = bela::kNoPad,
               typename std::enable_if<sizeof(Int) == 2 && !std::is_pointer<Int>::value>::type * = nullptr)
      : Hex(spec, static_cast<uint16_t>(v)) {}
  template <typename Int>
  explicit Hex(Int v, PadSpec spec = bela::kNoPad,
               typename std::enable_if<sizeof(Int) == 4 && !std::is_pointer<Int>::value>::type * = nullptr)
      : Hex(spec, static_cast<uint32_t>(v)) {}
  template <typename Int>
  explicit Hex(Int v, PadSpec spec = bela::kNoPad,
               typename std::enable_if<sizeof(Int) == 8 && !std::is_pointer<Int>::value>::type * = nullptr)
      : Hex(spec, static_cast<uint64_t>(v)) {}
  template <typename Pointee>
  explicit Hex(Pointee *v, PadSpec spec = bela::kNoPad) : Hex(spec, reinterpret_cast<uintptr_t>(v)) {}

private:
  Hex(PadSpec spec, uint64_t v)
      : value(v), width(spec == bela::kNoPad       ? 1
                        : spec >= bela::kSpacePad2 ? spec - bela::kSpacePad2 + 2
                                                   : spec - bela::kZeroPad2 + 2),
        fill(spec >= bela::kSpacePad2 ? L' ' : L'0') {}
};

// -----------------------------------------------------------------------------
// Dec
// -----------------------------------------------------------------------------
//
// `Dec` stores a set of decimal string conversion parameters for use
// within `AlphaNum` string conversions.  Dec is slower than the default
// integer conversion, so use it only if you need padding.
struct Dec {
  uint64_t value;
  uint8_t width;
  wchar_t fill;
  bool neg;

  template <typename Int>
  explicit Dec(Int v, PadSpec spec = bela::kNoPad, typename std::enable_if<(sizeof(Int) <= 8)>::type * = nullptr)
      : value(v >= 0 ? static_cast<uint64_t>(v) : uint64_t{0} - static_cast<uint64_t>(v)),
        width(spec == bela::kNoPad       ? 1
              : spec >= bela::kSpacePad2 ? spec - bela::kSpacePad2 + 2
                                         : spec - bela::kZeroPad2 + 2),
        fill(spec >= bela::kSpacePad2 ? L' ' : L'0'), neg(v < 0) {}
};

// -----------------------------------------------------------------------------
// AlphaNum
// -----------------------------------------------------------------------------
//
// The `AlphaNum` class acts as the main parameter type for `StringCat()` and
// `StrAppend()`, providing efficient conversion of numeric, boolean, and
// hexadecimal values (through the `Hex` type) into strings.
class AlphaNum {
public:
  // No bool ctor -- bools convert to an integral type.
  // A bool ctor would also convert incoming pointers (bletch).

  AlphaNum(int x) // NOLINT(runtime/explicit)
      : piece_(digits_, numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}
  AlphaNum(unsigned int x) // NOLINT(runtime/explicit)
      : piece_(digits_, numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}
  AlphaNum(long x) // NOLINT(*)
      : piece_(digits_, numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}
  AlphaNum(unsigned long x) // NOLINT(*)
      : piece_(digits_, numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}
  AlphaNum(long long x) // NOLINT(*)
      : piece_(digits_, numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}
  AlphaNum(unsigned long long x) // NOLINT(*)
      : piece_(digits_, numbers_internal::FastIntToBuffer(x, digits_) - &digits_[0]) {}

  AlphaNum(float f) // NOLINT(runtime/explicit)
      : piece_(digits_, numbers_internal::SixDigitsToBuffer(f, digits_)) {}
  AlphaNum(double f) // NOLINT(runtime/explicit)
      : piece_(digits_, numbers_internal::SixDigitsToBuffer(f, digits_)) {}

  AlphaNum(Hex hex); // NOLINT(runtime/explicit)
  AlphaNum(Dec dec); // NOLINT(runtime/explicit)

  template <size_t size>
  AlphaNum( // NOLINT(runtime/explicit)
      const strings_internal::AlphaNumBuffer<size> &buf)
      : piece_(&buf.data[0], buf.size) {}

  AlphaNum(const wchar_t *c_str) : piece_(c_str == nullptr ? L"" : c_str) {} // NOLINT(runtime/explicit)
  AlphaNum(std::wstring_view pc) : piece_(pc) {}                             // NOLINT(runtime/explicit)

  template <typename Allocator>
  AlphaNum( // NOLINT(runtime/explicit)
      const std::basic_string<wchar_t, std::char_traits<wchar_t>, Allocator> &str)
      : piece_(str) {}
  /// WARNING wchar_t or char16_t must not  surrogate . U+D800~U+DFFF
  AlphaNum(wchar_t c) : piece_(digits_, 1) { digits_[0] = c; }
  AlphaNum(char16_t ch) : piece_(digits_, 1) { digits_[0] = static_cast<wchar_t>(ch); }
  AlphaNum(char32_t ch) : piece_(digits_, bela::encode_into_unchecked(ch, digits_)) {
    static_assert(sizeof(digits_) / sizeof(wchar_t) > bela::kMaxEncodedUTF16Size, "difits_ buffer is too small");
  }

  AlphaNum(const AlphaNum &) = delete;
  AlphaNum &operator=(const AlphaNum &) = delete;

  std::wstring_view::size_type size() const { return piece_.size(); }
  const wchar_t *data() const { return piece_.data(); }
  std::wstring_view Piece() const { return piece_; }

  // Normal enums are already handled by the integer formatters.
  // This overload matches only scoped enums.
  template <typename T, typename = typename std::enable_if<std::is_enum<T>{} && !std::is_convertible<T, int>{}>::type>
  AlphaNum(T e) // NOLINT(runtime/explicit)
      : AlphaNum(static_cast<typename std::underlying_type<T>::type>(e)) {}

  // vector<bool>::reference and const_reference require special help to
  // convert to `AlphaNum` because it requires two user defined conversions.
  template <typename T,
            typename std::enable_if<std::is_class<T>::value &&
                                    (std::is_same<T, std::vector<bool>::reference>::value ||
                                     std::is_same<T, std::vector<bool>::const_reference>::value)>::type * = nullptr>
  AlphaNum(T e) : AlphaNum(static_cast<bool>(e)) {} // NOLINT(runtime/explicit)

private:
  std::wstring_view piece_;
  wchar_t digits_[numbers_internal::kFastToBufferSize];
};

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

namespace strings_internal {

// Do not call directly - this is not part of the public API.
std::wstring CatPieces(std::initializer_list<std::wstring_view> pieces);
void AppendPieces(std::wstring *dest, std::initializer_list<std::wstring_view> pieces);

} // namespace strings_internal

[[nodiscard]] inline std::wstring StringCat() { return std::wstring(); }

[[nodiscard]] inline std::wstring StringCat(const AlphaNum &a) { return std::wstring(a.data(), a.size()); }

[[nodiscard]] std::wstring StringCat(const AlphaNum &a, const AlphaNum &b);
[[nodiscard]] std::wstring StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c);
[[nodiscard]] std::wstring StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d);

// Support 5 or more arguments
template <typename... AV>
[[nodiscard]] inline std::wstring StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d,
                                            const AlphaNum &e, const AV &...args) {
  return strings_internal::CatPieces(
      {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(), static_cast<const AlphaNum &>(args).Piece()...});
}

// -----------------------------------------------------------------------------
// StrAppend()
// -----------------------------------------------------------------------------
//
// Appends a string or set of strings to an existing string, in a similar
// fashion to `StringCat()`.
//
// WARNING: `StrAppend(&str, a, b, c, ...)` requires that none of the
// a, b, c, parameters be a reference into str. For speed, `StrAppend()` does
// not try to check each of its input arguments to be sure that they are not
// a subset of the string being appended to. That is, while this will work:
//
//   std::string s = "foo";
//   s += s;
//
// This output is undefined:
//
//   std::string s = "foo";
//   StrAppend(&s, s);
//
// This output is undefined as well, since `absl::string_view` does not own its
// data:
//
//   std::string s = "foobar";
//   absl::string_view p = s;
//   StrAppend(&s, p);

inline void StrAppend(std::wstring *) {}
void StrAppend(std::wstring *dest, const AlphaNum &a);
void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b);
void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c);
void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d);

// Support 5 or more arguments
template <typename... AV>
inline void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d,
                      const AlphaNum &e, const AV &...args) {
  strings_internal::AppendPieces(
      dest, {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(), static_cast<const AlphaNum &>(args).Piece()...});
}

// Helper function for the future StringCat default floating-point format, %.6g
// This is fast.
inline strings_internal::AlphaNumBuffer<numbers_internal::kSixDigitsToBufferSize> SixDigits(double d) {
  strings_internal::AlphaNumBuffer<numbers_internal::kSixDigitsToBufferSize> result{0};
  result.size = numbers_internal::SixDigitsToBuffer(d, &result.data[0]);
  return result;
}

} // namespace bela

#endif
