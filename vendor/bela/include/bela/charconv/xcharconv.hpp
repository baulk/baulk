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

namespace bela {
enum class chars_format {
  scientific = 0b001,
  fixed = 0b010,
  hex = 0b100,
  general = fixed | scientific,
};
[[nodiscard]] constexpr chars_format operator&(chars_format L, chars_format R) noexcept {
  using I = std::underlying_type_t<chars_format>;
  return static_cast<chars_format>(static_cast<I>(L) & static_cast<I>(R));
}
[[nodiscard]] constexpr chars_format operator|(chars_format L, chars_format R) noexcept {
  using I = std::underlying_type_t<chars_format>;
  return static_cast<chars_format>(static_cast<I>(L) | static_cast<I>(R));
}
struct to_chars_result {
  wchar_t *ptr;
  std::errc ec;
};
template <typename T> T _Min_value(T a, T b) { return a < b ? a : b; }
template <typename T> T _Max_value(T a, T b) { return a > b ? a : b; }
template <typename T> void ArrayCopy(void *dst, const T *src, size_t n) { memcpy(dst, src, n * sizeof(T)); }

} // namespace bela

#endif
