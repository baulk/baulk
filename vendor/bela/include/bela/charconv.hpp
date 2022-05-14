// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// Port from libcxx
#ifndef BELA_CHARCONV_HPP
#define BELA_CHARCONV_HPP

/*
    charconv synopsis

namespace std {

  // floating-point format for primitive numerical conversion
  enum class chars_format {
    scientific = unspecified,
    fixed = unspecified,
    hex = unspecified,
    general = fixed | scientific
  };

  // 23.20.2, primitive numerical output conversion
  struct to_chars_result {
    wchar_t* ptr;
    errc ec;
    friend bool operator==(const to_chars_result&, const to_chars_result&) = default; // since C++20
  };

  to_chars_result to_chars(wchar_t* first, wchar_t* last, see below value,
                           int base = 10);
  to_chars_result to_chars(wchar_t* first, wchar_t* last, bool value,
                           int base = 10) = delete;

  to_chars_result to_chars(wchar_t* first, wchar_t* last, float value);
  to_chars_result to_chars(wchar_t* first, wchar_t* last, double value);
  to_chars_result to_chars(wchar_t* first, wchar_t* last, long double value);

  to_chars_result to_chars(wchar_t* first, wchar_t* last, float value,
                           chars_format fmt);
  to_chars_result to_chars(wchar_t* first, wchar_t* last, double value,
                           chars_format fmt);
  to_chars_result to_chars(wchar_t* first, wchar_t* last, long double value,
                           chars_format fmt);

  to_chars_result to_chars(wchar_t* first, wchar_t* last, float value,
                           chars_format fmt, int precision);
  to_chars_result to_chars(wchar_t* first, wchar_t* last, double value,
                           chars_format fmt, int precision);
  to_chars_result to_chars(wchar_t* first, wchar_t* last, long double value,
                           chars_format fmt, int precision);

  // 23.20.3, primitive numerical input conversion
  struct from_chars_result {
    const wchar_t* ptr;
    errc ec;
    friend bool operator==(const from_chars_result&, const from_chars_result&) = default; // since C++20
  };

  from_chars_result from_chars(const wchar_t* first, const wchar_t* last,
                               see below& value, int base = 10);

  from_chars_result from_chars(const wchar_t* first, const wchar_t* last,
                               float& value,
                               chars_format fmt = chars_format::general);
  from_chars_result from_chars(const wchar_t* first, const wchar_t* last,
                               double& value,
                               chars_format fmt = chars_format::general);
  from_chars_result from_chars(const wchar_t* first, const wchar_t* last,
                               long double& value,
                               chars_format fmt = chars_format::general);

} // namespace std

*/
#include <charconv>
#include <string_view>
#include "macros.hpp"
#include "utility.hpp"
#include "types.hpp"

namespace bela {
constexpr auto successful = std::errc{};

template <class I>
constexpr bool is_charconv_affirmed_v =
    bela::is_any_of_v<std::remove_cv_t<I>, char, signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t,
                      short, unsigned short, int, unsigned int, long, unsigned long, long long, unsigned long long>;
template <class I>
concept charconv_affirmed = is_charconv_affirmed_v<I>;
using std::chars_format;
struct to_chars_result {
  wchar_t *ptr;
  std::errc ec;
  friend bool operator==(const to_chars_result &, const to_chars_result &) = default; // since C++20
  [[nodiscard]] explicit operator bool() const noexcept { return ec == successful; }
};

namespace __itoa {
wchar_t *__u64toa(uint64_t __value, wchar_t *__buffer) noexcept;
wchar_t *__u32toa(uint32_t __value, wchar_t *__buffer) noexcept;
static constexpr uint64_t __pow10_64[] = {
    UINT64_C(0),
    UINT64_C(10),
    UINT64_C(100),
    UINT64_C(1000),
    UINT64_C(10000),
    UINT64_C(100000),
    UINT64_C(1000000),
    UINT64_C(10000000),
    UINT64_C(100000000),
    UINT64_C(1000000000),
    UINT64_C(10000000000),
    UINT64_C(100000000000),
    UINT64_C(1000000000000),
    UINT64_C(10000000000000),
    UINT64_C(100000000000000),
    UINT64_C(1000000000000000),
    UINT64_C(10000000000000000),
    UINT64_C(100000000000000000),
    UINT64_C(1000000000000000000),
    UINT64_C(10000000000000000000),
};

static constexpr uint32_t __pow10_32[] = {
    UINT32_C(0),      UINT32_C(10),      UINT32_C(100),      UINT32_C(1000),      UINT32_C(10000),
    UINT32_C(100000), UINT32_C(1000000), UINT32_C(10000000), UINT32_C(100000000), UINT32_C(1000000000),
};

template <typename I, typename = void> struct __traits_base {
  using type = uint64_t;

  static int __width(I __v) {
    auto __t = (64 - bela::__bela_ctz(static_cast<type>(__v | 1))) * 1233 >> 12;
    return __t - (__v < __pow10_64[__t]) + 1;
  }

  static wchar_t *__convert(I __v, wchar_t *__p) { return __u64toa(__v, __p); }
  static decltype(__pow10_64) &__pow() { return __pow10_64; }
};

template <typename I> struct __traits_base<I, decltype(void(uint32_t{std::declval<I>()}))> {
  using type = uint32_t;

  static int __width(I __v) {
    auto __t = (32 - bela::__bela_ctz(static_cast<type>(__v | 1))) * 1233 >> 12;
    return __t - (__v < __pow10_32[__t]) + 1;
  }

  static wchar_t *__convert(I __v, wchar_t *__p) { return __u32toa(__v, __p); }

  static decltype(__pow10_32) &__pow() { return __pow10_32; }
};

template <typename I> inline bool __mul_overflowed(unsigned char __a, I __b, unsigned char &__r) {
  auto __c = __a * __b;
  __r = __c;
  return __c > (std::numeric_limits<unsigned char>::max)();
}

template <typename I> inline bool __mul_overflowed(unsigned short __a, I __b, unsigned short &__r) {
  auto __c = __a * __b;
  __r = __c;
  return __c > (std::numeric_limits<unsigned short>::max)();
}

template <typename I> inline bool __mul_overflowed(I __a, I __b, I &__r) {
  static_assert(std::is_unsigned<I>::value, "");
#if !defined(_MSC_VER)
  return __builtin_mul_overflow(__a, __b, &__r);
#else
  bool __did = __b && ((std::numeric_limits<I>::max)() / __b) < __a;
  __r = __a * __b;
  return __did;
#endif
}

template <typename I, typename _Up> inline bool __mul_overflowed(I __a, _Up __b, I &__r) {
  return __mul_overflowed(__a, static_cast<I>(__b), __r);
}

template <typename I> struct __traits : __traits_base<I> {
  static constexpr int digits = std::numeric_limits<I>::digits10 + 1;
  using __traits_base<I>::__pow;
  using typename __traits_base<I>::type;

  // precondition: at least one non-zero character available
  static wchar_t const *__read(wchar_t const *__p, wchar_t const *__ep, type &__a, type &__b) {
    type __cprod[digits];
    int __j = digits - 1;
    int __i = digits;
    do {
      if (!('0' <= *__p && *__p <= '9')) {
        break;
      }
      __cprod[--__i] = *__p++ - '0';
    } while (__p != __ep && __i != 0);

    __a = __inner_product(__cprod + __i + 1, __cprod + __j, __pow() + 1, __cprod[__i]);
    if (__mul_overflowed(__cprod[__j], __pow()[__j - __i], __b)) {
      --__p;
    }
    return __p;
  }

  template <typename _It1, typename _It2, class _Up>
  static _Up __inner_product(_It1 __first1, _It1 __last1, _It2 __first2, _Up __init) {
    for (; __first1 < __last1; ++__first1, ++__first2) {
      __init = __init + *__first1 * *__first2;
    }
    return __init;
  }
};

} // namespace __itoa

template <typename I>
requires std::unsigned_integral<I>
inline I __complement(I __x) { return I(~__x + 1); }

template <typename I>
requires std::unsigned_integral<I> to_chars_result __to_chars_itoa(wchar_t *__first, wchar_t *__last, I __value) {
  using __tx = __itoa::__traits<I>;
  auto __diff = __last - __first;

  if (__tx::digits <= __diff || __tx::__width(__value) <= __diff) {
    return {__tx::__convert(__value, __first), successful};
  }
  return {__last, std::errc::value_too_large};
}

template <typename I>
requires std::signed_integral<I> to_chars_result __to_chars_itoa(wchar_t *__first, wchar_t *__last, I __value) {
  auto __x = bela::unsigned_cast(__value);
  if (__value < 0 && __first != __last) {
    *__first++ = '-';
    __x = __complement(__x);
  }
  return __to_chars_itoa(__first, __last, __x);
}

namespace __itoa {
  // https://reviews.llvm.org/D97705
  static constexpr wchar_t __base_2_lut[64] = {
      '0', '0', '0', '0', '0', '0', '0', '1', '0', '0', '1', '0', '0', '0', '1', '1', '0', '1', '0', '0', '0', '1',
      '0', '1', '0', '1', '1', '0', '0', '1', '1', '1', '1', '0', '0', '0', '1', '0', '0', '1', '1', '0', '1', '0',
      '1', '0', '1', '1', '1', '1', '0', '0', '1', '1', '0', '1', '1', '1', '1', '0', '1', '1', '1', '1'};

  static constexpr wchar_t __base_8_lut[128] = {
      '0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6', '0', '7', '1', '0', '1', '1', '1', '2',
      '1', '3', '1', '4', '1', '5', '1', '6', '1', '7', '2', '0', '2', '1', '2', '2', '2', '3', '2', '4', '2', '5',
      '2', '6', '2', '7', '3', '0', '3', '1', '3', '2', '3', '3', '3', '4', '3', '5', '3', '6', '3', '7', '4', '0',
      '4', '1', '4', '2', '4', '3', '4', '4', '4', '5', '4', '6', '4', '7', '5', '0', '5', '1', '5', '2', '5', '3',
      '5', '4', '5', '5', '5', '6', '5', '7', '6', '0', '6', '1', '6', '2', '6', '3', '6', '4', '6', '5', '6', '6',
      '6', '7', '7', '0', '7', '1', '7', '2', '7', '3', '7', '4', '7', '5', '7', '6', '7', '7'};

  static constexpr wchar_t __base_16_lut[512] = {
      '0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6', '0', '7', '0', '8', '0', '9', '0', 'a', '0',
      'b', '0', 'c', '0', 'd', '0', 'e', '0', 'f', '1', '0', '1', '1', '1', '2', '1', '3', '1', '4', '1', '5', '1', '6',
      '1', '7', '1', '8', '1', '9', '1', 'a', '1', 'b', '1', 'c', '1', 'd', '1', 'e', '1', 'f', '2', '0', '2', '1', '2',
      '2', '2', '3', '2', '4', '2', '5', '2', '6', '2', '7', '2', '8', '2', '9', '2', 'a', '2', 'b', '2', 'c', '2', 'd',
      '2', 'e', '2', 'f', '3', '0', '3', '1', '3', '2', '3', '3', '3', '4', '3', '5', '3', '6', '3', '7', '3', '8', '3',
      '9', '3', 'a', '3', 'b', '3', 'c', '3', 'd', '3', 'e', '3', 'f', '4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
      '4', '5', '4', '6', '4', '7', '4', '8', '4', '9', '4', 'a', '4', 'b', '4', 'c', '4', 'd', '4', 'e', '4', 'f', '5',
      '0', '5', '1', '5', '2', '5', '3', '5', '4', '5', '5', '5', '6', '5', '7', '5', '8', '5', '9', '5', 'a', '5', 'b',
      '5', 'c', '5', 'd', '5', 'e', '5', 'f', '6', '0', '6', '1', '6', '2', '6', '3', '6', '4', '6', '5', '6', '6', '6',
      '7', '6', '8', '6', '9', '6', 'a', '6', 'b', '6', 'c', '6', 'd', '6', 'e', '6', 'f', '7', '0', '7', '1', '7', '2',
      '7', '3', '7', '4', '7', '5', '7', '6', '7', '7', '7', '8', '7', '9', '7', 'a', '7', 'b', '7', 'c', '7', 'd', '7',
      'e', '7', 'f', '8', '0', '8', '1', '8', '2', '8', '3', '8', '4', '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
      '8', 'a', '8', 'b', '8', 'c', '8', 'd', '8', 'e', '8', 'f', '9', '0', '9', '1', '9', '2', '9', '3', '9', '4', '9',
      '5', '9', '6', '9', '7', '9', '8', '9', '9', '9', 'a', '9', 'b', '9', 'c', '9', 'd', '9', 'e', '9', 'f', 'a', '0',
      'a', '1', 'a', '2', 'a', '3', 'a', '4', 'a', '5', 'a', '6', 'a', '7', 'a', '8', 'a', '9', 'a', 'a', 'a', 'b', 'a',
      'c', 'a', 'd', 'a', 'e', 'a', 'f', 'b', '0', 'b', '1', 'b', '2', 'b', '3', 'b', '4', 'b', '5', 'b', '6', 'b', '7',
      'b', '8', 'b', '9', 'b', 'a', 'b', 'b', 'b', 'c', 'b', 'd', 'b', 'e', 'b', 'f', 'c', '0', 'c', '1', 'c', '2', 'c',
      '3', 'c', '4', 'c', '5', 'c', '6', 'c', '7', 'c', '8', 'c', '9', 'c', 'a', 'c', 'b', 'c', 'c', 'c', 'd', 'c', 'e',
      'c', 'f', 'd', '0', 'd', '1', 'd', '2', 'd', '3', 'd', '4', 'd', '5', 'd', '6', 'd', '7', 'd', '8', 'd', '9', 'd',
      'a', 'd', 'b', 'd', 'c', 'd', 'd', 'd', 'e', 'd', 'f', 'e', '0', 'e', '1', 'e', '2', 'e', '3', 'e', '4', 'e', '5',
      'e', '6', 'e', '7', 'e', '8', 'e', '9', 'e', 'a', 'e', 'b', 'e', 'c', 'e', 'd', 'e', 'e', 'e', 'f', 'f', '0', 'f',
      '1', 'f', '2', 'f', '3', 'f', '4', 'f', '5', 'f', '6', 'f', '7', 'f', '8', 'f', '9', 'f', 'a', 'f', 'b', 'f', 'c',
      'f', 'd', 'f', 'e', 'f', 'f'};

  template <unsigned B> struct __integral;

  template <> struct __integral<2> {
    template <typename I>
    requires std::unsigned_integral<I>
    static constexpr int __width(I __value) noexcept {
      // If value == 0 still need one digit. If the value != this has no
      // effect since the code scans for the most significant bit set. (Note
      // that __libcpp_clz doesn't work for 0.)
      return std::numeric_limits<I>::digits - __bela_clz(__value | 1);
    }

    template <typename I>
    requires std::unsigned_integral<I>
    static to_chars_result __to_chars(wchar_t *__first, wchar_t *__last, I __value) {
      ptrdiff_t __cap = __last - __first;
      int __n = __width(__value);
      if (__n > __cap)
        return {__last, std::errc::value_too_large};

      __last = __first + __n;
      wchar_t *__p = __last;
      const unsigned __divisor = 16;
      while (__value > __divisor) {
        unsigned __c = __value % __divisor;
        __value /= __divisor;
        __p -= 4;
        std::memcpy(__p, &__base_2_lut[4 * __c], 4 * sizeof(wchar_t));
      }
      do {
        unsigned __c = __value % 2;
        __value /= 2;
        *--__p = L"01"[__c];
      } while (__value != 0);
      return {__last, std::errc(0)};
    }
  };

  template <> struct __integral<8> {
    template <typename I>
    requires std::unsigned_integral<I>
    static constexpr int __width(I __value) noexcept {
      // If value == 0 still need one digit. If the value != this has no
      // effect since the code scans for the most significat bit set. (Note
      // that __libcpp_clz doesn't work for 0.)
      return ((std::numeric_limits<I>::digits - __bela_clz(__value | 1)) + 2) / 3;
    }

    template <typename I>
    requires std::unsigned_integral<I>
    static to_chars_result __to_chars(wchar_t *__first, wchar_t *__last, I __value) {
      ptrdiff_t __cap = __last - __first;
      int __n = __width(__value);
      if (__n > __cap)
        return {__last, std::errc::value_too_large};

      __last = __first + __n;
      wchar_t *__p = __last;
      unsigned __divisor = 64;
      while (__value > __divisor) {
        unsigned __c = __value % __divisor;
        __value /= __divisor;
        __p -= 2;
        std::memcpy(__p, &__base_8_lut[2 * __c], 2 * sizeof(wchar_t));
      }
      do {
        unsigned __c = __value % 8;
        __value /= 8;
        *--__p = L"01234567"[__c];
      } while (__value != 0);
      return {__last, std::errc(0)};
    }
  };

  template <> struct __integral<16> {
    template <typename I>
    requires std::unsigned_integral<I>
    static constexpr int __width(I __value) noexcept {
      // If value == 0 still need one digit. If the value != this has no
      // effect since the code scans for the most significat bit set. (Note
      // that __libcpp_clz doesn't work for 0.)
      return (std::numeric_limits<I>::digits - __bela_clz(__value | 1) + 3) / 4;
    }

    template <typename I>
    requires std::unsigned_integral<I>
    static to_chars_result __to_chars(wchar_t *__first, wchar_t *__last, I __value) {
      ptrdiff_t __cap = __last - __first;
      int __n = __width(__value);
      if (__n > __cap)
        return {__last, std::errc::value_too_large};

      __last = __first + __n;
      wchar_t *__p = __last;
      unsigned __divisor = 256;
      while (__value > __divisor) {
        unsigned __c = __value % __divisor;
        __value /= __divisor;
        __p -= 2;
        std::memcpy(__p, &__base_16_lut[2 * __c], 2 * sizeof(wchar_t));
      }
      if (__first != __last)
        do {
          unsigned __c = __value % 16;
          __value /= 16;
          *--__p = L"0123456789abcdef"[__c];
        } while (__value != 0);
      return {__last, std::errc(0)};
    }
  };
} // namespace __itoa

template <class I>
concept conversion_required = std::unsigned_integral<I> && sizeof(I) < sizeof(unsigned);

template <unsigned B, typename I>
requires(!conversion_required<I>) int __to_chars_integral_width(I __value) {
  return __itoa::__integral<B>::__width(__value);
}

template <unsigned B, typename I>
requires conversion_required<I>
int __to_chars_integral_width(I __value) {
  //
  return bela::__to_chars_integral_width<B>(static_cast<unsigned>(__value));
}

template <unsigned B, typename I>
requires(!conversion_required<I>) to_chars_result __to_chars_integral(wchar_t *__first, wchar_t *__last, I __value) {
  return __itoa::__integral<B>::__to_chars(__first, __last, __value);
}

template <unsigned B, typename I>
requires conversion_required<I> to_chars_result __to_chars_integral(wchar_t *__first, wchar_t *__last, I __value) {
  return bela::__to_chars_integral<B>(__first, __last, static_cast<unsigned>(__value));
}

template <typename I>
requires std::unsigned_integral<I>
int __to_chars_integral_width(I __value, unsigned __base) {
  unsigned __base_2 = __base * __base;
  unsigned __base_3 = __base_2 * __base;
  unsigned __base_4 = __base_2 * __base_2;

  int __r = 0;
  while (true) {
    if (__value < __base)
      return __r + 1;
    if (__value < __base_2)
      return __r + 2;
    if (__value < __base_3)
      return __r + 3;
    if (__value < __base_4)
      return __r + 4;

    __value /= __base_4;
    __r += 4;
  }

  bela::unreachable();
}

template <typename I>
requires std::unsigned_integral<I> to_chars_result __to_chars_integral(wchar_t *__first, wchar_t *__last, I __value,
                                                                       int __base) {
  if (__base == 10) {
    return __to_chars_itoa(__first, __last, __value);
  }
  switch (__base) {
  case 2:
    return __to_chars_integral<2>(__first, __last, __value);
  case 8:
    return __to_chars_integral<8>(__first, __last, __value);
  case 16:
    return __to_chars_integral<16>(__first, __last, __value);
  }
  ptrdiff_t __cap = __last - __first;
  int __n = __to_chars_integral_width(__value, __base);
  if (__n > __cap) {
    return {__last, std::errc::value_too_large};
  }

  __last = __first + __n;
  wchar_t *__p = __last;
  do {
    unsigned __c = __value % __base;
    __value /= __base;
    *--__p = L"0123456789abcdefghijklmnopqrstuvwxyz"[__c];
  } while (__value != 0);
  return {__last, successful};
}

template <typename I>
requires std::signed_integral<I> to_chars_result __to_chars_integral(wchar_t *__first, wchar_t *__last, I __value,
                                                                     int __base) {
  auto __x = bela::unsigned_cast(__value);
  if (__value < 0 && __first != __last) {
    *__first++ = '-';
    __x = __complement(__x);
  }
  return __to_chars_integral(__first, __last, __x, __base);
}

template <typename I>
requires bela::charconv_affirmed<I> to_chars_result to_chars(wchar_t *first, wchar_t *last, I val, const int base)
noexcept {
  _BELA_ASSERT(2 <= base && base <= 36, "base not in [2, 36]");
  if (base == 10) {
    return __to_chars_itoa(first, last, val);
  }
  return __to_chars_integral(first, last, val, base);
}

template <typename I>
requires bela::charconv_affirmed<I> to_chars_result to_chars(wchar_t *first, wchar_t *last, I val)
noexcept { return __to_chars_itoa(first, last, val); }

struct from_chars_result {
  const wchar_t *ptr;
  std::errc ec;
  friend bool operator==(const from_chars_result &, const from_chars_result &) = default; // since C++20
  [[nodiscard]] explicit operator bool() const noexcept { return ec == successful; }
};

template <typename _It, typename I, typename _Fn, typename... _Ts>
inline from_chars_result __sign_combinator(_It __first, _It __last, I &__value, _Fn __f, _Ts... __args) {
  using __tl = std::numeric_limits<I>;
  decltype(bela::unsigned_cast(__value)) __x;

  bool __neg = (__first != __last && *__first == '-');
  auto __r = __f(__neg ? __first + 1 : __first, __last, __x, __args...);
  switch (__r.ec) {
  case std::errc::invalid_argument:
    return {__first, __r.ec};
  case std::errc::result_out_of_range:
    return __r;
  default:
    break;
  }

  if (__neg) {
    if (__x <= __complement(bela::unsigned_cast((__tl::min)()))) {
      __x = __complement(__x);
      std::memcpy(&__value, &__x, sizeof(__x));
      return __r;
    }
  } else {
    if (__x <= bela::unsigned_cast((__tl::max)())) {
      __value = __x;
      return __r;
    }
  }

  return {__r.ptr, std::errc::result_out_of_range};
}

template <typename I> inline bool __in_pattern(I __c) { return '0' <= __c && __c <= '9'; }

struct __in_pattern_result {
  bool __ok;
  int __val;

  explicit operator bool() const { return __ok; }
};

template <typename I> inline __in_pattern_result __in_pattern(I __c, int __base) {
  if (__base <= 10) {
    return {'0' <= __c && __c < '0' + __base, __c - '0'};
  }
  if (__in_pattern(__c)) {
    return {true, __c - '0'};
  }
  if ('a' <= __c && __c < 'a' + __base - 10) {
    return {true, __c - 'a' + 10};
  }
  return {'A' <= __c && __c < 'A' + __base - 10, __c - 'A' + 10};
}

template <typename _It, typename I, typename _Fn, typename... _Ts>
inline from_chars_result __subject_seq_combinator(_It __first, _It __last, I &__value, _Fn __f, _Ts... __args) {
  auto __find_non_zero = [](_It __firstit, _It __lastit) {
    for (; __firstit != __lastit; ++__firstit)
      if (*__firstit != '0')
        break;
    return __firstit;
  };

  auto __p = __find_non_zero(__first, __last);
  if (__p == __last || !__in_pattern(*__p, __args...)) {
    if (__p == __first)
      return {__first, std::errc::invalid_argument};
    else {
      __value = 0;
      return {__p, {}};
    }
  }

  auto __r = __f(__p, __last, __value, __args...);
  if (__r.ec == std::errc::result_out_of_range) {
    for (; __r.ptr != __last; ++__r.ptr) {
      if (!__in_pattern(*__r.ptr, __args...))
        break;
    }
  }

  return __r;
}

template <typename I>
requires std::unsigned_integral<I>
inline from_chars_result __from_chars_atoi(const wchar_t *__first, const wchar_t *__last, I &__value) {
  using __tx = __itoa::__traits<I>;
  using __output_type = typename __tx::type;

  return __subject_seq_combinator(__first, __last, __value,
                                  [](const wchar_t *first, const wchar_t *last, I &__val) -> from_chars_result {
                                    __output_type __a, __b;
                                    auto __p = __tx::__read(first, last, __a, __b);
                                    if (__p == last || !__in_pattern(*__p)) {
                                      __output_type __m = (std::numeric_limits<I>::max)();
                                      if (__m >= __a && __m - __a >= __b) {
                                        __val = __a + __b;
                                        return {__p, successful};
                                      }
                                    }
                                    return {__p, std::errc::result_out_of_range};
                                  });
}

template <typename I>
requires std::signed_integral<I>
inline from_chars_result __from_chars_atoi(const wchar_t *__first, const wchar_t *__last, I &__value) {
  using __t = decltype(bela::unsigned_cast(__value));
  return __sign_combinator(__first, __last, __value, __from_chars_atoi<__t>);
}

template <typename I>
requires std::unsigned_integral<I>
inline from_chars_result __from_chars_integral(const wchar_t *__first, const wchar_t *__last, I &__value, int __base) {
  if (__base == 10) {
    return __from_chars_atoi(__first, __last, __value);
  }

  return __subject_seq_combinator(
      __first, __last, __value,
      [](const wchar_t *__p, const wchar_t *__lastp, I &__val, int base) -> from_chars_result {
        using __tl = std::numeric_limits<I>;
        auto __digits = __tl::digits / log2f(float(base));
        I __a = __in_pattern(*__p++, base).__val, __b = 0;

        for (int __i = 1; __p != __lastp; ++__i, ++__p) {
          if (auto __c = __in_pattern(*__p, base)) {
            if (__i < __digits - 1)
              __a = __a * base + __c.__val;
            else {
              if (!__itoa::__mul_overflowed(__a, base, __a))
                ++__p;
              __b = __c.__val;
              break;
            }
          } else
            break;
        }

        if (__p == __lastp || !__in_pattern(*__p, base)) {
          if ((__tl::max)() - __a >= __b) {
            __val = __a + __b;
            return {__p, successful};
          }
        }
        return {__p, std::errc::result_out_of_range};
      },
      __base);
}

template <typename I>
requires std::signed_integral<I>
inline from_chars_result __from_chars_integral(const wchar_t *__first, const wchar_t *__last, I &__value,
                                               const int __base) noexcept {
  using __t = decltype(bela::unsigned_cast(__value));
  return __sign_combinator(__first, __last, __value, __from_chars_integral<__t>, __base);
}

template <typename I>
requires bela::charconv_affirmed<I>
inline from_chars_result from_chars(const wchar_t *first, const wchar_t *last, I &value) noexcept {
  return __from_chars_atoi(first, last, value);
}

template <typename I>
requires bela::charconv_affirmed<I>
inline from_chars_result from_chars(const wchar_t *first, const wchar_t *last, I &value, const int base) noexcept {
  _BELA_ASSERT(2 <= base && base <= 36, "base not in [2, 36]");
  return __from_chars_integral(first, last, value, base);
}

////////////////// float
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const float value) noexcept;
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const double value) noexcept;
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const long double value) noexcept;
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const float value, const chars_format fmt) noexcept;
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const double value,
                         const chars_format fmt) noexcept;
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const long double value,
                         const chars_format fmt) noexcept;
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const float value, const chars_format fmt,
                         const int precision) noexcept;
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const double value, const chars_format fmt,
                         const int precision) noexcept;
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const long double value, const chars_format fmt,
                         const int precision) noexcept;

template <typename F>
requires std::floating_point<F>
inline auto to_chars(wchar_t *first, wchar_t *last, F value) noexcept { return to_chars(first, last, value); }

template <typename F>
requires std::floating_point<F>
inline auto to_chars(wchar_t *first, wchar_t *last, F value, chars_format fmt) {
  return to_chars(first, last, value, fmt);
}

from_chars_result from_chars(const wchar_t *const first, const wchar_t *const last, float &value,
                             const chars_format fmt = chars_format::general) noexcept;
from_chars_result from_chars(const wchar_t *const first, const wchar_t *const last, double &value,
                             const chars_format fmt = chars_format::general) noexcept;
from_chars_result from_chars(const wchar_t *const first, const wchar_t *const last, long double &value,
                             const chars_format fmt = chars_format::general) noexcept;

// Additional wrapper functions
template <std::size_t N, typename I>
requires bela::charconv_affirmed<I>
inline auto to_chars(wchar_t (&a)[N], I val, int base = 10) { return to_chars(a, a + N, val, base); }

template <std::size_t N, typename I>
requires bela::charconv_affirmed<I>
inline auto to_chars(char16_t (&a)[N], I val, int base = 10) {
  return to_chars(reinterpret_cast<wchar_t *>(a), reinterpret_cast<wchar_t *>(a + N), val, base);
}

template <std::size_t N, typename I>
requires bela::charconv_affirmed<I>
inline auto to_chars(char (&a)[N], I val, int base = 10) { return std::to_chars(a, a + N, val, base); }

template <std::size_t N, typename I>
requires bela::charconv_affirmed<I>
inline auto to_chars(char8_t (&a)[N], I val, int base = 10) {
  return std::to_chars(reinterpret_cast<char *>(a), reinterpret_cast<char *>(a + N), val, base);
}

template <std::size_t N, typename I>
requires bela::charconv_affirmed<I>
inline auto to_chars_view(wchar_t (&a)[N], I val, int base = 10) {
  if (auto res = to_chars(a, a + N, val, base); res) {
    return std::wstring_view{a, static_cast<size_t>(res.ptr - a)};
  }
  return std::wstring_view{};
}

template <std::size_t N, typename I>
requires bela::charconv_affirmed<I>
inline auto to_chars_view(char16_t (&a)[N], I val, int base = 10) {
  if (auto res = to_chars(reinterpret_cast<wchar_t *>(a), reinterpret_cast<wchar_t *>(a + N), val, base); res) {
    return std::u16string_view{a, static_cast<size_t>(reinterpret_cast<const char16_t *>(res.ptr) - a)};
  }
  return std::u16string_view{};
}

template <std::size_t N, typename F>
requires std::floating_point<F>
inline auto to_chars_view(wchar_t (&a)[N], F val) {
  if (auto res = to_chars(a, a + N, val); res) {
    return std::wstring_view{a, static_cast<size_t>(res.ptr - a)};
  }
  return std::wstring_view{};
}

template <std::size_t N, typename F>
requires std::floating_point<F>
inline auto to_chars_view(char16_t (&a)[N], F val) {
  if (auto res = to_chars(reinterpret_cast<wchar_t *>(a), reinterpret_cast<wchar_t *>(a + N), val); res) {
    return std::u16string_view{a, static_cast<size_t>(reinterpret_cast<const char16_t *>(res.ptr) - a)};
  }
  return std::u16string_view{};
}

template <std::size_t N, typename F>
requires std::floating_point<F>
inline auto to_chars_view(wchar_t (&a)[N], F val, chars_format fmt) {
  if (auto res = to_chars(a, a + N, val, fmt); res) {
    return std::wstring_view{a, static_cast<size_t>(res.ptr - a)};
  }
  return std::wstring_view{};
}

template <std::size_t N, typename F>
requires std::floating_point<F>
inline auto to_chars_view(char16_t (&a)[N], F val, chars_format fmt) {
  if (auto res = to_chars(reinterpret_cast<wchar_t *>(a), reinterpret_cast<wchar_t *>(a + N), val, fmt); res) {
    return std::u16string_view{a, static_cast<size_t>(reinterpret_cast<const char16_t *>(res.ptr) - a)};
  }
  return std::u16string_view{};
}

template <std::size_t N, typename F>
requires std::floating_point<F>
inline auto to_chars_view(wchar_t (&a)[N], F val, chars_format fmt, int precision) {
  if (auto res = to_chars(a, a + N, val, fmt, precision); res) {
    return std::wstring_view{a, static_cast<size_t>(res.ptr - a)};
  }
  return std::wstring_view{};
}

template <std::size_t N, typename F>
requires std::floating_point<F>
inline auto to_chars_view(char16_t (&a)[N], F val, chars_format fmt, int precision) {
  if (auto res = to_chars(reinterpret_cast<wchar_t *>(a), reinterpret_cast<wchar_t *>(a + N), val, fmt, precision);
      res) {
    return std::u16string_view{a, static_cast<size_t>(reinterpret_cast<const char16_t *>(res.ptr) - a)};
  }
  return std::u16string_view{};
}

template <typename I>
requires bela::charconv_affirmed<I>
inline auto from_string_view(std::wstring_view sv, I &value) {
  return __from_chars_atoi(sv.data(), sv.data() + sv.size(), value);
}

template <typename I>
requires bela::charconv_affirmed<I>
inline auto from_string_view(std::u16string_view sv, I &value) {
  return __from_chars_atoi(reinterpret_cast<const wchar_t *>(sv.data()),
                           reinterpret_cast<const wchar_t *>(sv.data() + sv.size()), value);
}

template <typename I>
requires bela::charconv_affirmed<I>
inline auto from_string_view(std::wstring_view sv, I &value, int base) {
  _BELA_ASSERT(2 <= base && base <= 36, "base not in [2, 36]");
  return __from_chars_integral(sv.data(), sv.data() + sv.size(), value, base);
}

template <typename I>
requires bela::charconv_affirmed<I>
inline auto from_string_view(std::u16string_view sv, I &value, int base) {
  _BELA_ASSERT(2 <= base && base <= 36, "base not in [2, 36]");
  return __from_chars_integral(reinterpret_cast<const wchar_t *>(sv.data()),
                               reinterpret_cast<const wchar_t *>(sv.data() + sv.size()), value, base);
}

// bela::charconv float
template <typename F>
requires std::floating_point<F>
inline auto from_string_view(std::wstring_view sv, F &value, chars_format fmt = chars_format::general) {
  return from_chars(sv.data(), sv.data() + sv.size(), value, fmt);
}

template <typename F>
requires std::floating_point<F>
inline auto from_string_view(std::u16string_view sv, F &value, chars_format fmt = chars_format::general) {
  return from_chars(reinterpret_cast<const wchar_t *>(sv.data()),
                    reinterpret_cast<const wchar_t *>(sv.data() + sv.size()), value, fmt);
}

// std::charconv
template <std::size_t N, typename I>
requires bela::charconv_affirmed<I>
inline auto to_chars_view(char (&a)[N], I val, int base = 10) {
  if (auto res = std::to_chars(a, a + N, val, base); res.ec == successful) {
    return std::string_view{a, static_cast<size_t>(res.ptr - a)};
  }
  return std::string_view{};
}

template <std::size_t N, typename I>
requires bela::charconv_affirmed<I>
inline auto to_chars_view(char8_t (&a)[N], I val, int base = 10) {
  if (auto res = std::to_chars(reinterpret_cast<char *>(a), reinterpret_cast<char *>(a + N), val, base);
      res.ec == successful) {
    return std::u8string_view{a, static_cast<size_t>(reinterpret_cast<const char8_t *>(res.ptr) - a)};
  }
  return std::u8string_view{};
}

///////// std::charconv float

template <std::size_t N, typename F>
requires std::floating_point<F>
inline auto to_chars_view(char (&a)[N], F val, chars_format fmt) {
  if (auto res = std::to_chars(a, a + N, val, fmt); res.ec == successful) {
    return std::string_view{a, static_cast<size_t>(res.ptr - a)};
  }
  return std::string_view{};
}

template <std::size_t N, typename F>
requires std::floating_point<F>
inline auto to_chars_view(char8_t (&a)[N], F val, chars_format fmt) {
  if (auto res = std::to_chars(reinterpret_cast<char *>(a), reinterpret_cast<char *>(a + N), val, fmt);
      res.ec == successful) {
    return std::u8string_view{a, static_cast<size_t>(reinterpret_cast<const char8_t *>(res.ptr) - a)};
  }
  return std::u8string_view{};
}

template <std::size_t N, typename F>
requires std::floating_point<F>
inline auto to_chars_view(char (&a)[N], F val, chars_format fmt, int precision) {
  if (auto res = std::to_chars(a, a + N, val, fmt, precision); res) {
    return std::string_view{a, static_cast<size_t>(res.ptr - a)};
  }
  return std::string_view{};
}

template <std::size_t N, typename F>
requires std::floating_point<F>
inline auto to_chars_view(char8_t (&a)[N], F val, chars_format fmt, int precision) {
  if (auto res = std::to_chars(reinterpret_cast<char *>(a), reinterpret_cast<char *>(a + N), fmt, precision); res) {
    return std::u8string_view{a, static_cast<size_t>(reinterpret_cast<const char *>(res.ptr) - a)};
  }
  return std::u8string_view{};
}

template <typename I>
requires bela::charconv_affirmed<I>
inline auto from_string_view(std::string_view sv, I &value, int base = 10) {
  _BELA_ASSERT(2 <= base && base <= 36, "base not in [2, 36]");
  return std::from_chars(sv.data(), sv.data() + sv.size(), value, base);
}

template <typename I>
requires bela::charconv_affirmed<I>
inline auto from_string_view(std::u8string_view sv, I &value, int base = 10) {
  _BELA_ASSERT(2 <= base && base <= 36, "base not in [2, 36]");
  return std::from_chars(reinterpret_cast<const char *>(sv.data()),
                         reinterpret_cast<const char *>(sv.data() + sv.size()), value, base);
}

template <typename F>
requires std::floating_point<F>
inline auto from_string_view(std::string_view sv, F &value, chars_format fmt = chars_format::general) {
  return std::from_chars(sv.data(), sv.data() + sv.size(), value, fmt);
}

template <typename F>
requires std::floating_point<F>
inline auto from_string_view(std::u8string_view sv, F &value, chars_format fmt = chars_format::general) {
  return std::from_chars(reinterpret_cast<const char *>(sv.data()),
                         reinterpret_cast<const char *>(sv.data() + sv.size()), value, fmt);
}

} // namespace bela

#endif