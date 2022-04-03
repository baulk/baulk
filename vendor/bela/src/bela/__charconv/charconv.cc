// charconv code
#include <bela/charconv.hpp>
#include "ryu_tables.hpp"

namespace bela {
// Port from libcxx
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
namespace __itoa {

template <typename T> inline wchar_t *append1(wchar_t *buffer, T i) noexcept {
  *buffer = '0' + static_cast<char>(i);
  return buffer + 1;
}

template <typename T> inline wchar_t *append2(wchar_t *buffer, T i) noexcept {
  memcpy(buffer, &__DIGIT_TABLE<wchar_t>[(i)*2], 2 * sizeof(wchar_t));
  return buffer + 2;
}

template <typename T> inline wchar_t *append3(wchar_t *buffer, T i) noexcept {
  return append2(append1(buffer, (i) / 100), (i) % 100);
}

template <typename T> inline wchar_t *append4(wchar_t *buffer, T i) noexcept {
  return append2(append2(buffer, (i) / 100), (i) % 100);
}

template <typename T> inline wchar_t *append2_no_zeros(wchar_t *buffer, T v) noexcept {
  if (v < 10) {
    return append1(buffer, v);
  }
  return append2(buffer, v);
}

template <typename T> inline wchar_t *append4_no_zeros(wchar_t *buffer, T v) noexcept {
  if (v < 100) {
    return append2_no_zeros(buffer, v);
  }
  if (v < 1000) {
    return append3(buffer, v);
  }
  return append4(buffer, v);
}

template <typename T> inline wchar_t *append8_no_zeros(wchar_t *buffer, T v) noexcept {
  if (v < 10000) {
    buffer = append4_no_zeros(buffer, v);
  } else {
    buffer = append4_no_zeros(buffer, v / 10000);
    buffer = append4(buffer, v % 10000);
  }
  return buffer;
}

wchar_t *__u32toa(uint32_t value, wchar_t *buffer) noexcept {
  if (value < 100000000) {
    buffer = append8_no_zeros(buffer, value);
  } else {
    // value = aabbbbcccc in decimal
    const uint32_t a = value / 100000000; // 1 to 42
    value %= 100000000;

    buffer = append2_no_zeros(buffer, a);
    buffer = append4(buffer, value / 10000);
    buffer = append4(buffer, value % 10000);
  }

  return buffer;
}

wchar_t *__u64toa(uint64_t value, wchar_t *buffer) noexcept {
  if (value < 100000000) {
    auto v = static_cast<uint32_t>(value);
    buffer = append8_no_zeros(buffer, v);
  } else if (value < 10000000000000000) {
    const auto v0 = static_cast<uint32_t>(value / 100000000);
    const auto v1 = static_cast<uint32_t>(value % 100000000);

    buffer = append8_no_zeros(buffer, v0);
    buffer = append4(buffer, v1 / 10000);
    buffer = append4(buffer, v1 % 10000);
  } else {
    const auto a = static_cast<uint32_t>(value / 10000000000000000); // 1 to 1844
    value %= 10000000000000000;

    buffer = append4_no_zeros(buffer, a);

    const auto v0 = static_cast<uint32_t>(value / 100000000);
    const auto v1 = static_cast<uint32_t>(value % 100000000);
    buffer = append4(buffer, v0 / 10000);
    buffer = append4(buffer, v0 % 10000);
    buffer = append4(buffer, v1 / 10000);
    buffer = append4(buffer, v1 % 10000);
  }

  return buffer;
}

} // namespace __itoa

} // namespace bela