// Porting from MSVC STL
// xcharconv.h internal header

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BELA_CHARCONV_FWD_HPP
#define BELA_CHARCONV_FWD_HPP
#include <type_traits>
#include <system_error>
#include <cstring>
#include <cstdint>
#include <bela/macros.hpp>
#include <charconv>

namespace bela {
using chars_format = std::chars_format;
struct to_chars_result {
  wchar_t *ptr;
  std::errc ec;
  [[nodiscard]] friend bool operator==(const to_chars_result &, const to_chars_result &) = default;
};
template <typename T> T _Min_value(T a, T b) { return a < b ? a : b; }
template <typename T> T _Max_value(T a, T b) { return a > b ? a : b; }
template <typename T> void ArrayCopy(void *dst, const T *src, size_t n) { memcpy(dst, src, n * sizeof(T)); }

} // namespace bela

#endif
