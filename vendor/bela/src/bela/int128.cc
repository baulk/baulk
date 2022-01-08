// Copyright 2017 The Abseil Authors.
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

#include <bela/int128.hpp>

#include <cstddef>
#include <cassert>
#include <iomanip>
#include <string>
#include <type_traits>

#define BELA_INTERNAL_ASSUME(cond) assert(cond)

namespace bela {

const uint128 kuint128max = MakeUint128(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max());

namespace {

// Returns the 0-based position of the last set bit (i.e., most significant bit)
// in the given uint128. The argument is not 0.
//
// For example:
//   Given: 5 (decimal) == 101 (binary)
//   Returns: 2
inline BELA_ATTRIBUTE_ALWAYS_INLINE int Fls128(uint128 n) {
  if (uint64_t hi = Uint128High64(n)) {
    BELA_INTERNAL_ASSUME(hi != 0);
    return 127 - std::countl_zero(hi);
  }
  const uint64_t low = Uint128Low64(n);
  BELA_INTERNAL_ASSUME(low != 0);
  return 63 - std::countl_zero(low);
}

// Long division/modulo for uint128 implemented using the shift-subtract
// division algorithm adapted from:
// https://stackoverflow.com/questions/5386377/division-without-using
inline void DivModImpl(uint128 dividend, uint128 divisor, uint128 *quotient_ret, uint128 *remainder_ret) {
  assert(divisor != 0);

  if (divisor > dividend) {
    *quotient_ret = 0;
    *remainder_ret = dividend;
    return;
  }

  if (divisor == dividend) {
    *quotient_ret = 1;
    *remainder_ret = 0;
    return;
  }

  uint128 denominator = divisor;
  uint128 quotient = 0;

  // Left aligns the MSB of the denominator and the dividend.
  const int shift = Fls128(dividend) - Fls128(denominator);
  denominator <<= shift;

  // Uses shift-subtract algorithm to divide dividend by denominator. The
  // remainder will be left in dividend.
  for (int i = 0; i <= shift; ++i) {
    quotient <<= 1;
    if (dividend >= denominator) {
      dividend -= denominator;
      quotient |= 1;
    }
    denominator >>= 1;
  }

  *quotient_ret = quotient;
  *remainder_ret = dividend;
}

template <typename T> uint128 MakeUint128FromFloat(T v) {
  static_assert(std::is_floating_point<T>::value);

  // Rounding behavior is towards zero, same as for built-in types.

  // Undefined behavior if v is NaN or cannot fit into uint128.
  assert(std::isfinite(v) && v > -1 &&
         (std::numeric_limits<T>::max_exponent <= 128 || v < std::ldexp(static_cast<T>(1), 128)));

  if (v >= std::ldexp(static_cast<T>(1), 64)) {
    auto hi = static_cast<uint64_t>(std::ldexp(v, -64));
    auto lo = static_cast<uint64_t>(v - std::ldexp(static_cast<T>(hi), 64));
    return MakeUint128(hi, lo);
  }

  return MakeUint128(0, static_cast<uint64_t>(v));
}

#if defined(__clang__) && !defined(__SSE3__)
// Workaround for clang bug: https://bugs.llvm.org/show_bug.cgi?id=38289
// Casting from long double to uint64_t is miscompiled and drops bits.
// It is more work, so only use when we need the workaround.
uint128 MakeUint128FromFloat(long double v) {
  // Go 50 bits at a time, that fits in a double
  static_assert(std::numeric_limits<double>::digits >= 50);
  static_assert(std::numeric_limits<long double>::digits <= 150);
  // Undefined behavior if v is not finite or cannot fit into uint128.
  assert(std::isfinite(v) && v > -1 && v < std::ldexp(1.0L, 128));

  v = std::ldexp(v, -100);
  auto w0 = static_cast<uint64_t>(static_cast<double>(std::trunc(v)));
  v = std::ldexp(v - static_cast<double>(w0), 50);
  auto w1 = static_cast<uint64_t>(static_cast<double>(std::trunc(v)));
  v = std::ldexp(v - static_cast<double>(w1), 50);
  auto w2 = static_cast<uint64_t>(static_cast<double>(std::trunc(v)));
  return (static_cast<uint128>(w0) << 100) | (static_cast<uint128>(w1) << 50) | static_cast<uint128>(w2);
}
#endif // __clang__ && !__SSE3__
} // namespace

uint128::uint128(float v) : uint128(MakeUint128FromFloat(v)) {}
uint128::uint128(double v) : uint128(MakeUint128FromFloat(v)) {}
uint128::uint128(long double v) : uint128(MakeUint128FromFloat(v)) {}

#if !defined(BELA_HAVE_INTRINSIC_INT128)
uint128 operator/(uint128 lhs, uint128 rhs) {
  uint128 quotient = 0;
  uint128 remainder = 0;
  DivModImpl(lhs, rhs, &quotient, &remainder);
  return quotient;
}

uint128 operator%(uint128 lhs, uint128 rhs) {
  uint128 quotient = 0;
  uint128 remainder = 0;
  DivModImpl(lhs, rhs, &quotient, &remainder);
  return remainder;
}
#endif // !defined(BELA_HAVE_INTRINSIC_INT128)

namespace {

uint128 UnsignedAbsoluteValue(int128 v) {
  // Cast to uint128 before possibly negating because -Int128Min() is undefined.
  return Int128High64(v) < 0 ? -uint128(v) : uint128(v);
}

} // namespace

#if !defined(BELA_HAVE_INTRINSIC_INT128)
namespace {

template <typename T> int128 MakeInt128FromFloat(T v) {
  // Conversion when v is NaN or cannot fit into int128 would be undefined
  // behavior if using an intrinsic 128-bit integer.
  assert(std::isfinite(v) && (std::numeric_limits<T>::max_exponent <= 127 ||
                              (v >= -std::ldexp(static_cast<T>(1), 127) && v < std::ldexp(static_cast<T>(1), 127))));

  // We must convert the absolute value and then negate as needed, because
  // floating point types are typically sign-magnitude. Otherwise, the
  // difference between the high and low 64 bits when interpreted as two's
  // complement overwhelms the precision of the mantissa.
  uint128 result = v < 0 ? -MakeUint128FromFloat(-v) : MakeUint128FromFloat(v);
  return MakeInt128(int128_internal::BitCastToSigned(Uint128High64(result)), Uint128Low64(result));
}

} // namespace

int128::int128(float v) : int128(MakeInt128FromFloat(v)) {}
int128::int128(double v) : int128(MakeInt128FromFloat(v)) {}
int128::int128(long double v) : int128(MakeInt128FromFloat(v)) {}

int128 operator/(int128 lhs, int128 rhs) {
  assert(lhs != Int128Min() || rhs != -1); // UB on two's complement.

  uint128 quotient = 0;
  uint128 remainder = 0;
  DivModImpl(UnsignedAbsoluteValue(lhs), UnsignedAbsoluteValue(rhs), &quotient, &remainder);
  if ((Int128High64(lhs) < 0) != (Int128High64(rhs) < 0)) {
    quotient = -quotient;
  }
  return MakeInt128(int128_internal::BitCastToSigned(Uint128High64(quotient)), Uint128Low64(quotient));
}

int128 operator%(int128 lhs, int128 rhs) {
  assert(lhs != Int128Min() || rhs != -1); // UB on two's complement.

  uint128 quotient = 0;
  uint128 remainder = 0;
  DivModImpl(UnsignedAbsoluteValue(lhs), UnsignedAbsoluteValue(rhs), &quotient, &remainder);
  if (Int128High64(lhs) < 0) {
    remainder = -remainder;
  }
  return MakeInt128(int128_internal::BitCastToSigned(Uint128High64(remainder)), Uint128Low64(remainder));
}
#endif // BELA_HAVE_INTRINSIC_INT128

} // namespace bela
