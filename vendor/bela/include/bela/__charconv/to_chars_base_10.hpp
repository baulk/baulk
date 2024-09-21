// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _BELA___CHARCONV_TO_CHARS_BASE_10_H
#define _BELA___CHARCONV_TO_CHARS_BASE_10_H

#include "tables.hpp"
#include <cstdint>
#include <cstring>

namespace bela::__itoa {

template <class I> constexpr wchar_t *__append1(wchar_t *__buffer, I __value) noexcept {
  *__buffer = L'0' + static_cast<wchar_t>(__value);
  return __buffer + 1;
}

template <class I> constexpr wchar_t *__append2(wchar_t *__buffer, I __value) noexcept {
  std::memcpy(__buffer, &__digits_base_10[(__value) * 2], 2 * sizeof(wchar_t));
  return __buffer + 2;
}

template <class I> constexpr wchar_t *__append3(wchar_t *__buffer, I __value) noexcept {
  return __itoa::__append2(__itoa::__append1(__buffer, (__value) / 100), (__value) % 100);
}

template <class I> constexpr wchar_t *__append4(wchar_t *__buffer, I __value) noexcept {
  return __itoa::__append2(__itoa::__append2(__buffer, (__value) / 100), (__value) % 100);
}

template <class I> constexpr wchar_t *__append2_no_zeros(wchar_t *__buffer, I __value) noexcept {
  if (__value < 10) {
    return __itoa::__append1(__buffer, __value);
  }
  return __itoa::__append2(__buffer, __value);
}

template <class I> constexpr wchar_t *__append4_no_zeros(wchar_t *__buffer, I __value) noexcept {
  if (__value < 100) {
    return __itoa::__append2_no_zeros(__buffer, __value);
  }
  if (__value < 1000) {
    return __itoa::__append3(__buffer, __value);
  }
  return __itoa::__append4(__buffer, __value);
}

template <class I> constexpr wchar_t *__append8_no_zeros(wchar_t *__buffer, I __value) noexcept {
  if (__value < 10000) {
    return __itoa::__append4_no_zeros(__buffer, __value);
  }
  __buffer = __itoa::__append4_no_zeros(__buffer, __value / 10000);
  __buffer = __itoa::__append4(__buffer, __value % 10000);
  return __buffer;
}

constexpr inline wchar_t *__base_10_u32(uint32_t __value, wchar_t *__buffer) noexcept {
  if (__value < 100000000) {
    return __itoa::__append8_no_zeros(__buffer, __value);
  }
  // __value = aabbbbcccc in decimal
  const uint32_t __a = __value / 100000000; // 1 to 42
  __value %= 100000000;

  __buffer = __itoa::__append2_no_zeros(__buffer, __a);
  __buffer = __itoa::__append4(__buffer, __value / 10000);
  __buffer = __itoa::__append4(__buffer, __value % 10000);

  return __buffer;
}

constexpr inline wchar_t *__base_10_u64(uint64_t __value, wchar_t *__buffer) noexcept {
  if (__value < 100000000) {
    return __itoa::__append8_no_zeros(__buffer, static_cast<uint32_t>(__value));
  }
  if (__value < 10000000000000000) {
    const uint32_t __v0 = static_cast<uint32_t>(__value / 100000000);
    const uint32_t __v1 = static_cast<uint32_t>(__value % 100000000);

    __buffer = __itoa::__append8_no_zeros(__buffer, __v0);
    __buffer = __itoa::__append4(__buffer, __v1 / 10000);
    __buffer = __itoa::__append4(__buffer, __v1 % 10000);
    return __buffer;
  }
  const uint32_t __a = static_cast<uint32_t>(__value / 10000000000000000); // 1 to 1844
  __value %= 10000000000000000;

  __buffer = __itoa::__append4_no_zeros(__buffer, __a);

  const uint32_t __v0 = static_cast<uint32_t>(__value / 100000000);
  const uint32_t __v1 = static_cast<uint32_t>(__value % 100000000);
  __buffer = __itoa::__append4(__buffer, __v0 / 10000);
  __buffer = __itoa::__append4(__buffer, __v0 % 10000);
  __buffer = __itoa::__append4(__buffer, __v1 / 10000);
  __buffer = __itoa::__append4(__buffer, __v1 % 10000);

  return __buffer;
}

} // namespace bela::__itoa

#endif // _BELA___CHARCONV_TO_CHARS_BASE_10_H