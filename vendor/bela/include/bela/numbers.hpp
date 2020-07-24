// ---------------------------------------------------------------------------
// Copyright (c) 2020, Force Charlie
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
#ifndef BELA_NUMBERS_HPP
#define BELA_NUMBERS_HPP
#pragma once
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <string>
#include <type_traits>
#include <string_view>

namespace bela {
namespace numbers_internal {
// safe_strto?() functions for implementing SimpleAtoi()
bool safe_strto32_base(std::wstring_view text, int32_t *value, int base);
bool safe_strto64_base(std::wstring_view text, int64_t *value, int base);
bool safe_strtou32_base(std::wstring_view text, uint32_t *value, int base);
bool safe_strtou64_base(std::wstring_view text, uint64_t *value, int base);

static const int kFastToBufferSize = 32;
static const int kSixDigitsToBufferSize = 16;

// Helper function for fast formatting of floating-point values.
// The result is the same as printf's "%g", a.k.a. "%.6g"; that is, six
// significant digits are returned, trailing zeros are removed, and numbers
// outside the range 0.0001-999999 are output using scientific notation
// (1.23456e+06). This routine is heavily optimized.
// Required buffer size is `kSixDigitsToBufferSize`.
size_t SixDigitsToBuffer(double d, wchar_t *buffer);

// These functions are intended for speed. All functions take an output buffer
// as an argument and return a pointer to the last byte they wrote, which is the
// terminating '\0'. At most `kFastToBufferSize` bytes are written.
wchar_t *FastIntToBuffer(int32_t, wchar_t *);
wchar_t *FastIntToBuffer(uint32_t, wchar_t *);
wchar_t *FastIntToBuffer(int64_t, wchar_t *);
wchar_t *FastIntToBuffer(uint64_t, wchar_t *);

// For enums and integer types that are not an exact match for the types above,
// use templates to call the appropriate one of the four overloads above.
template <typename int_type> wchar_t *FastIntToBuffer(int_type i, wchar_t *buffer) {
  static_assert(sizeof(i) <= 64 / 8, "FastIntToBuffer works only with 64-bit-or-less integers.");
  // TODO(jorg): This signed-ness check is used because it works correctly
  // with enums, and it also serves to check that int_type is not a pointer.
  // If one day something like std::is_signed<enum E> works, switch to it.
  if constexpr (static_cast<int_type>(1) - 2 < 0) { // Signed
    if constexpr (sizeof(i) > 32 / 8) {             // 33-bit to 64-bit
      return FastIntToBuffer(static_cast<int64_t>(i), buffer);
    } else { // 32-bit or less
      return FastIntToBuffer(static_cast<int32_t>(i), buffer);
    }
  } else {                              // Unsigned
    if constexpr (sizeof(i) > 32 / 8) { // 33-bit to 64-bit
      return FastIntToBuffer(static_cast<uint64_t>(i), buffer);
    } else { // 32-bit or less
      return FastIntToBuffer(static_cast<uint32_t>(i), buffer);
    }
  }
}

// Implementation of SimpleAtoi, generalized to support arbitrary base (used
// with base different from 10 elsewhere in Abseil implementation).
template <typename int_type> bool safe_strtoi_base(std::wstring_view s, int_type *out, int base) {
  static_assert(sizeof(*out) == 4 || sizeof(*out) == 8,
                "SimpleAtoi works only with 32-bit or 64-bit integers.");
  static_assert(!std::is_floating_point<int_type>::value, "Use SimpleAtof or SimpleAtod instead.");
  bool parsed;
  // TODO(jorg): This signed-ness check is used because it works correctly
  // with enums, and it also serves to check that int_type is not a pointer.
  // If one day something like std::is_signed<enum E> works, switch to it.
  if constexpr (static_cast<int_type>(1) - 2 < 0) { // Signed
    if constexpr (sizeof(*out) == 64 / 8) {         // 64-bit
      int64_t val;
      parsed = numbers_internal::safe_strto64_base(s, &val, base);
      *out = static_cast<int_type>(val);
    } else { // 32-bit
      int32_t val;
      parsed = numbers_internal::safe_strto32_base(s, &val, base);
      *out = static_cast<int_type>(val);
    }
  } else {                                  // Unsigned
    if constexpr (sizeof(*out) == 64 / 8) { // 64-bit
      uint64_t val;
      parsed = numbers_internal::safe_strtou64_base(s, &val, base);
      *out = static_cast<int_type>(val);
    } else { // 32-bit
      uint32_t val;
      parsed = numbers_internal::safe_strtou32_base(s, &val, base);
      *out = static_cast<int_type>(val);
    }
  }
  return parsed;
}

} // namespace numbers_internal

template <typename I> bool SimpleAtoi(std::wstring_view s, I *out) {
  return numbers_internal::safe_strtoi_base(s, out, 10);
}
// bool SimpleAtof(std::wstring_view str, float *out);
// bool SimpleAtod(std::wstring_view str, double *out);
bool SimpleAtob(std::wstring_view str, bool *out);

} // namespace bela

#endif
