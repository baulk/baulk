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
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <utility>
#include <cassert>
#include <cmath>
#include <bit> //C++20
#include <bela/ascii.hpp>
#include <bela/numbers.hpp>
#include <bela/strings.hpp>
#include <bela/match.hpp>

namespace bela {

inline void PutTwoDigits(size_t i, wchar_t *buf) {
  static const wchar_t two_ASCII_digits[100][2] = {
      {'0', '0'}, {'0', '1'}, {'0', '2'}, {'0', '3'}, {'0', '4'}, {'0', '5'}, {'0', '6'}, {'0', '7'}, {'0', '8'},
      {'0', '9'}, {'1', '0'}, {'1', '1'}, {'1', '2'}, {'1', '3'}, {'1', '4'}, {'1', '5'}, {'1', '6'}, {'1', '7'},
      {'1', '8'}, {'1', '9'}, {'2', '0'}, {'2', '1'}, {'2', '2'}, {'2', '3'}, {'2', '4'}, {'2', '5'}, {'2', '6'},
      {'2', '7'}, {'2', '8'}, {'2', '9'}, {'3', '0'}, {'3', '1'}, {'3', '2'}, {'3', '3'}, {'3', '4'}, {'3', '5'},
      {'3', '6'}, {'3', '7'}, {'3', '8'}, {'3', '9'}, {'4', '0'}, {'4', '1'}, {'4', '2'}, {'4', '3'}, {'4', '4'},
      {'4', '5'}, {'4', '6'}, {'4', '7'}, {'4', '8'}, {'4', '9'}, {'5', '0'}, {'5', '1'}, {'5', '2'}, {'5', '3'},
      {'5', '4'}, {'5', '5'}, {'5', '6'}, {'5', '7'}, {'5', '8'}, {'5', '9'}, {'6', '0'}, {'6', '1'}, {'6', '2'},
      {'6', '3'}, {'6', '4'}, {'6', '5'}, {'6', '6'}, {'6', '7'}, {'6', '8'}, {'6', '9'}, {'7', '0'}, {'7', '1'},
      {'7', '2'}, {'7', '3'}, {'7', '4'}, {'7', '5'}, {'7', '6'}, {'7', '7'}, {'7', '8'}, {'7', '9'}, {'8', '0'},
      {'8', '1'}, {'8', '2'}, {'8', '3'}, {'8', '4'}, {'8', '5'}, {'8', '6'}, {'8', '7'}, {'8', '8'}, {'8', '9'},
      {'9', '0'}, {'9', '1'}, {'9', '2'}, {'9', '3'}, {'9', '4'}, {'9', '5'}, {'9', '6'}, {'9', '7'}, {'9', '8'},
      {'9', '9'}};
  MemCopy(buf, two_ASCII_digits[i], 2);
}

inline void PutTwoDigits(size_t i, char *buf) {
  static const char two_ASCII_digits[100][2] = {
      {'0', '0'}, {'0', '1'}, {'0', '2'}, {'0', '3'}, {'0', '4'}, {'0', '5'}, {'0', '6'}, {'0', '7'}, {'0', '8'},
      {'0', '9'}, {'1', '0'}, {'1', '1'}, {'1', '2'}, {'1', '3'}, {'1', '4'}, {'1', '5'}, {'1', '6'}, {'1', '7'},
      {'1', '8'}, {'1', '9'}, {'2', '0'}, {'2', '1'}, {'2', '2'}, {'2', '3'}, {'2', '4'}, {'2', '5'}, {'2', '6'},
      {'2', '7'}, {'2', '8'}, {'2', '9'}, {'3', '0'}, {'3', '1'}, {'3', '2'}, {'3', '3'}, {'3', '4'}, {'3', '5'},
      {'3', '6'}, {'3', '7'}, {'3', '8'}, {'3', '9'}, {'4', '0'}, {'4', '1'}, {'4', '2'}, {'4', '3'}, {'4', '4'},
      {'4', '5'}, {'4', '6'}, {'4', '7'}, {'4', '8'}, {'4', '9'}, {'5', '0'}, {'5', '1'}, {'5', '2'}, {'5', '3'},
      {'5', '4'}, {'5', '5'}, {'5', '6'}, {'5', '7'}, {'5', '8'}, {'5', '9'}, {'6', '0'}, {'6', '1'}, {'6', '2'},
      {'6', '3'}, {'6', '4'}, {'6', '5'}, {'6', '6'}, {'6', '7'}, {'6', '8'}, {'6', '9'}, {'7', '0'}, {'7', '1'},
      {'7', '2'}, {'7', '3'}, {'7', '4'}, {'7', '5'}, {'7', '6'}, {'7', '7'}, {'7', '8'}, {'7', '9'}, {'8', '0'},
      {'8', '1'}, {'8', '2'}, {'8', '3'}, {'8', '4'}, {'8', '5'}, {'8', '6'}, {'8', '7'}, {'8', '8'}, {'8', '9'},
      {'9', '0'}, {'9', '1'}, {'9', '2'}, {'9', '3'}, {'9', '4'}, {'9', '5'}, {'9', '6'}, {'9', '7'}, {'9', '8'},
      {'9', '9'}};

  memcpy(buf, two_ASCII_digits[i], 2);
}

bool SimpleAtob(std::wstring_view str, bool *out) {
  if (EqualsIgnoreCase(str, L"true") || EqualsIgnoreCase(str, L"t") || EqualsIgnoreCase(str, L"yes") ||
      EqualsIgnoreCase(str, L"y") || EqualsIgnoreCase(str, L"1")) {
    *out = true;
    return true;
  }
  if (EqualsIgnoreCase(str, L"false") || EqualsIgnoreCase(str, L"f") || EqualsIgnoreCase(str, L"no") ||
      EqualsIgnoreCase(str, L"n") || EqualsIgnoreCase(str, L"0")) {
    *out = false;
    return true;
  }
  return false;
}

bool SimpleAtob(std::string_view str, bool *out) {
  if (EqualsIgnoreCase(str, "true") || EqualsIgnoreCase(str, "t") || EqualsIgnoreCase(str, "yes") ||
      EqualsIgnoreCase(str, "y") || EqualsIgnoreCase(str, "1")) {
    *out = true;
    return true;
  }
  if (EqualsIgnoreCase(str, "false") || EqualsIgnoreCase(str, "f") || EqualsIgnoreCase(str, "no") ||
      EqualsIgnoreCase(str, "n") || EqualsIgnoreCase(str, "0")) {
    *out = false;
    return true;
  }
  return false;
}

const wchar_t one_ASCII_final_digits[10][2]{
    {'0', 0}, {'1', 0}, {'2', 0}, {'3', 0}, {'4', 0}, {'5', 0}, {'6', 0}, {'7', 0}, {'8', 0}, {'9', 0},
};

const wchar_t one_ASCII_final_digits_narrow[10][2]{
    {'0', 0}, {'1', 0}, {'2', 0}, {'3', 0}, {'4', 0}, {'5', 0}, {'6', 0}, {'7', 0}, {'8', 0}, {'9', 0},
};

namespace numbers_internal {
wchar_t *FastIntToBuffer(uint32_t i, wchar_t *buffer) {
  uint32_t digits;
  // The idea of this implementation is to trim the number of divides to as few
  // as possible, and also reducing memory stores and branches, by going in
  // steps of two digits at a time rather than one whenever possible.
  // The huge-number case is first, in the hopes that the compiler will output
  // that case in one branch-free block of code, and only output conditional
  // branches into it from below.
  if (i >= 1000000000) {    // >= 1,000,000,000
    digits = i / 100000000; //      100,000,000
    i -= digits * 100000000;
    PutTwoDigits(digits, buffer);
    buffer += 2;
  lt100_000_000:
    digits = i / 1000000; // 1,000,000
    i -= digits * 1000000;
    PutTwoDigits(digits, buffer);
    buffer += 2;
  lt1_000_000:
    digits = i / 10000; // 10,000
    i -= digits * 10000;
    PutTwoDigits(digits, buffer);
    buffer += 2;
  lt10_000:
    digits = i / 100;
    i -= digits * 100;
    PutTwoDigits(digits, buffer);
    buffer += 2;
  lt100:
    digits = i;
    PutTwoDigits(digits, buffer);
    buffer += 2;
    *buffer = 0;
    return buffer;
  }

  if (i < 100) {
    digits = i;
    if (i >= 10) {
      goto lt100;
    }
    MemCopy(buffer, one_ASCII_final_digits[i], 2);
    return buffer + 1;
  }
  if (i < 10000) { //    10,000
    if (i >= 1000) {
      goto lt10_000;
    }
    digits = i / 100;
    i -= digits * 100;
    *buffer++ = static_cast<wchar_t>('0' + digits);
    goto lt100;
  }
  if (i < 1000000) { //    1,000,000
    if (i >= 100000) {
      goto lt1_000_000;
    }
    digits = i / 10000; //    10,000
    i -= digits * 10000;
    *buffer++ = static_cast<wchar_t>('0' + digits);
    goto lt10_000;
  }
  if (i < 100000000) { //    100,000,000
    if (i >= 10000000) {
      goto lt100_000_000;
    }
    digits = i / 1000000; //   1,000,000
    i -= digits * 1000000;
    *buffer++ = static_cast<wchar_t>('0' + digits);
    goto lt1_000_000;
  }
  // we already know that i < 1,000,000,000
  digits = i / 100000000; //   100,000,000
  i -= digits * 100000000;
  *buffer++ = static_cast<wchar_t>('0' + digits);
  goto lt100_000_000;
}

wchar_t *FastIntToBuffer(int32_t i, wchar_t *buffer) {
  uint32_t u = i;
  if (i < 0) {
    *buffer++ = '-';
    // We need to do the negation in modular (i.e., "unsigned")
    // arithmetic; MSVC++ apprently warns for plain "-u", so
    // we write the equivalent expression "0 - u" instead.
    u = 0 - u;
  }
  return FastIntToBuffer(u, buffer);
}

wchar_t *FastIntToBuffer(uint64_t i, wchar_t *buffer) {
  auto u32 = static_cast<uint32_t>(i);
  if (u32 == i) {
    return FastIntToBuffer(u32, buffer);
  }

  // Here we know i has at least 10 decimal digits.
  uint64_t top_1to11 = i / 1000000000;
  u32 = static_cast<uint32_t>(i - top_1to11 * 1000000000);
  auto top_1to11_32 = static_cast<uint32_t>(top_1to11);

  if (top_1to11_32 == top_1to11) {
    buffer = FastIntToBuffer(top_1to11_32, buffer);
  } else {
    // top_1to11 has more than 32 bits too; print it in two steps.
    auto top_8to9 = static_cast<uint32_t>(top_1to11 / 100);
    auto mid_2 = static_cast<uint32_t>(top_1to11 - top_8to9 * 100);
    buffer = FastIntToBuffer(top_8to9, buffer);
    PutTwoDigits(mid_2, buffer);
    buffer += 2;
  }

  // We have only 9 digits now, again the maximum uint32_t can handle fully.
  uint32_t digits = u32 / 10000000; // 10,000,000
  u32 -= digits * 10000000;
  PutTwoDigits(digits, buffer);
  buffer += 2;
  digits = u32 / 100000; // 100,000
  u32 -= digits * 100000;
  PutTwoDigits(digits, buffer);
  buffer += 2;
  digits = u32 / 1000; // 1,000
  u32 -= digits * 1000;
  PutTwoDigits(digits, buffer);
  buffer += 2;
  digits = u32 / 10;
  u32 -= digits * 10;
  PutTwoDigits(digits, buffer);
  buffer += 2;
  MemCopy(buffer, one_ASCII_final_digits[u32], 2);
  return buffer + 1;
}

wchar_t *FastIntToBuffer(int64_t i, wchar_t *buffer) {
  uint64_t u = i;
  if (i < 0) {
    *buffer++ = '-';
    u = 0 - u;
  }
  return FastIntToBuffer(u, buffer);
}

char *FastIntToBuffer(uint32_t i, char *buffer) {
  uint32_t digits;
  // The idea of this implementation is to trim the number of divides to as few
  // as possible, and also reducing memory stores and branches, by going in
  // steps of two digits at a time rather than one whenever possible.
  // The huge-number case is first, in the hopes that the compiler will output
  // that case in one branch-free block of code, and only output conditional
  // branches into it from below.
  if (i >= 1000000000) {    // >= 1,000,000,000
    digits = i / 100000000; //      100,000,000
    i -= digits * 100000000;
    PutTwoDigits(digits, buffer);
    buffer += 2;
  lt100_000_000:
    digits = i / 1000000; // 1,000,000
    i -= digits * 1000000;
    PutTwoDigits(digits, buffer);
    buffer += 2;
  lt1_000_000:
    digits = i / 10000; // 10,000
    i -= digits * 10000;
    PutTwoDigits(digits, buffer);
    buffer += 2;
  lt10_000:
    digits = i / 100;
    i -= digits * 100;
    PutTwoDigits(digits, buffer);
    buffer += 2;
  lt100:
    digits = i;
    PutTwoDigits(digits, buffer);
    buffer += 2;
    *buffer = 0;
    return buffer;
  }

  if (i < 100) {
    digits = i;
    if (i >= 10) {
      goto lt100;
    }
    memcpy(buffer, one_ASCII_final_digits_narrow[i], 2);
    return buffer + 1;
  }
  if (i < 10000) { //    10,000
    if (i >= 1000) {
      goto lt10_000;
    }
    digits = i / 100;
    i -= digits * 100;
    *buffer++ = static_cast<char>('0' + digits);
    goto lt100;
  }
  if (i < 1000000) { //    1,000,000
    if (i >= 100000) {
      goto lt1_000_000;
    }
    digits = i / 10000; //    10,000
    i -= digits * 10000;
    *buffer++ = static_cast<char>('0' + digits);
    goto lt10_000;
  }
  if (i < 100000000) { //    100,000,000
    if (i >= 10000000) {
      goto lt100_000_000;
    }
    digits = i / 1000000; //   1,000,000
    i -= digits * 1000000;
    *buffer++ = static_cast<char>('0' + digits);
    goto lt1_000_000;
  }
  // we already know that i < 1,000,000,000
  digits = i / 100000000; //   100,000,000
  i -= digits * 100000000;
  *buffer++ = static_cast<char>('0' + digits);
  goto lt100_000_000;
}

char *FastIntToBuffer(int32_t i, char *buffer) {
  uint32_t u = i;
  if (i < 0) {
    *buffer++ = '-';
    // We need to do the negation in modular (i.e., "unsigned")
    // arithmetic; MSVC++ apprently warns for plain "-u", so
    // we write the equivalent expression "0 - u" instead.
    u = 0 - u;
  }
  return FastIntToBuffer(u, buffer);
}

char *FastIntToBuffer(uint64_t i, char *buffer) {
  auto u32 = static_cast<uint32_t>(i);
  if (u32 == i) {
    return FastIntToBuffer(u32, buffer);
  }

  // Here we know i has at least 10 decimal digits.
  uint64_t top_1to11 = i / 1000000000;
  u32 = static_cast<uint32_t>(i - top_1to11 * 1000000000);
  auto top_1to11_32 = static_cast<uint32_t>(top_1to11);

  if (top_1to11_32 == top_1to11) {
    buffer = FastIntToBuffer(top_1to11_32, buffer);
  } else {
    // top_1to11 has more than 32 bits too; print it in two steps.
    auto top_8to9 = static_cast<uint32_t>(top_1to11 / 100);
    auto mid_2 = static_cast<uint32_t>(top_1to11 - top_8to9 * 100);
    buffer = FastIntToBuffer(top_8to9, buffer);
    PutTwoDigits(mid_2, buffer);
    buffer += 2;
  }

  // We have only 9 digits now, again the maximum uint32_t can handle fully.
  uint32_t digits = u32 / 10000000; // 10,000,000
  u32 -= digits * 10000000;
  PutTwoDigits(digits, buffer);
  buffer += 2;
  digits = u32 / 100000; // 100,000
  u32 -= digits * 100000;
  PutTwoDigits(digits, buffer);
  buffer += 2;
  digits = u32 / 1000; // 1,000
  u32 -= digits * 1000;
  PutTwoDigits(digits, buffer);
  buffer += 2;
  digits = u32 / 10;
  u32 -= digits * 10;
  PutTwoDigits(digits, buffer);
  buffer += 2;
  memcpy(buffer, one_ASCII_final_digits_narrow[u32], 2);
  return buffer + 1;
}

char *FastIntToBuffer(int64_t i, char *buffer) {
  uint64_t u = i;
  if (i < 0) {
    *buffer++ = '-';
    u = 0 - u;
  }
  return FastIntToBuffer(u, buffer);
}

// Given a 128-bit number expressed as a pair of uint64_t, high half first,
// return that number multiplied by the given 32-bit value.  If the result is
// too large to fit in a 128-bit number, divide it by 2 until it fits.
inline std::pair<uint64_t, uint64_t> Mul32(std::pair<uint64_t, uint64_t> num, uint32_t mul) {
  uint64_t bits0_31 = num.second & 0xFFFFFFFF;
  uint64_t bits32_63 = num.second >> 32;
  uint64_t bits64_95 = num.first & 0xFFFFFFFF;
  uint64_t bits96_127 = num.first >> 32;

  // The picture so far: each of these 64-bit values has only the lower 32 bits
  // filled in.
  // bits96_127:          [ 00000000 xxxxxxxx ]
  // bits64_95:                    [ 00000000 xxxxxxxx ]
  // bits32_63:                             [ 00000000 xxxxxxxx ]
  // bits0_31:                                       [ 00000000 xxxxxxxx ]

  bits0_31 *= mul;
  bits32_63 *= mul;
  bits64_95 *= mul;
  bits96_127 *= mul;

  // Now the top halves may also have value, though all 64 of their bits will
  // never be set at the same time, since they are a result of a 32x32 bit
  // multiply.  This makes the carry calculation slightly easier.
  // bits96_127:          [ mmmmmmmm | mmmmmmmm ]
  // bits64_95:                    [ | mmmmmmmm mmmmmmmm | ]
  // bits32_63:                      |        [ mmmmmmmm | mmmmmmmm ]
  // bits0_31:                       |                 [ | mmmmmmmm mmmmmmmm ]
  // eventually:        [ bits128_up | ...bits64_127.... | ..bits0_63... ]

  uint64_t bits0_63 = bits0_31 + (bits32_63 << 32);
  uint64_t bits64_127 =
      bits64_95 + (bits96_127 << 32) + (bits32_63 >> 32) + static_cast<unsigned long long>(bits0_63 < bits0_31);
  uint64_t bits128_up = (bits96_127 >> 32) + static_cast<unsigned long long>(bits64_127 < bits64_95);
  if (bits128_up == 0) {
    return {bits64_127, bits0_63};
  }

  int shift = 64 - std::countl_zero(bits128_up);
  uint64_t lo = (bits0_63 >> shift) + (bits64_127 << (64 - shift));
  uint64_t hi = (bits64_127 >> shift) + (bits128_up << (64 - shift));
  return {hi, lo};
}
// Compute num * 5 ^ expfive, and return the first 128 bits of the result,
// where the first bit is always a one.  So PowFive(1, 0) starts 0b100000,
// PowFive(1, 1) starts 0b101000, PowFive(1, 2) starts 0b110010, etc.
inline std::pair<uint64_t, uint64_t> PowFive(uint64_t num, int expfive) {
  std::pair<uint64_t, uint64_t> result = {num, 0};
  while (expfive >= 13) {
    // 5^13 is the highest power of five that will fit in a 32-bit integer.
    result = Mul32(result, 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5);
    expfive -= 13;
  }
  constexpr int powers_of_five[13] = {1,
                                      5,
                                      5 * 5,
                                      5 * 5 * 5,
                                      5 * 5 * 5 * 5,
                                      5 * 5 * 5 * 5 * 5,
                                      5 * 5 * 5 * 5 * 5 * 5,
                                      5 * 5 * 5 * 5 * 5 * 5 * 5,
                                      5 * 5 * 5 * 5 * 5 * 5 * 5 * 5,
                                      5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5,
                                      5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5,
                                      5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5,
                                      5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5};
  result = Mul32(result, powers_of_five[expfive & 15]);
  int shift = std::countl_zero(result.first);
  if (shift != 0) {
    result.first = (result.first << shift) + (result.second >> (64 - shift));
    result.second = (result.second << shift);
  }
  return result;
}

struct ExpDigits {
  int32_t exponent;
  wchar_t digits[6];
};

// SplitToSix converts value, a positive double-precision floating-point number,
// into a base-10 exponent and 6 ASCII digits, where the first digit is never
// zero.  For example, SplitToSix(1) returns an exponent of zero and a digits
// array of {'1', '0', '0', '0', '0', '0'}.  If value is exactly halfway between
// two possible representations, e.g. value = 100000.5, then "round to even" is
// performed.
inline ExpDigits SplitToSix(const double value) {
  ExpDigits exp_dig;
  int exp = 5;
  double d = value;
  // First step: calculate a close approximation of the output, where the
  // value d will be between 100,000 and 999,999, representing the digits
  // in the output ASCII array, and exp is the base-10 exponent.  It would be
  // faster to use a table here, and to look up the base-2 exponent of value,
  // however value is an IEEE-754 64-bit number, so the table would have 2,000
  // entries, which is not cache-friendly.
  if (d >= 999999.5) {
    if (d >= 1e+261) {
      exp += 256;
      d *= 1e-256;
    }
    if (d >= 1e+133) {
      exp += 128;
      d *= 1e-128;
    }
    if (d >= 1e+69) {
      exp += 64;
      d *= 1e-64;
    }
    if (d >= 1e+37) {
      exp += 32;
      d *= 1e-32;
    }
    if (d >= 1e+21) {
      exp += 16;
      d *= 1e-16;
    }
    if (d >= 1e+13) {
      exp += 8;
      d *= 1e-8;
    }
    if (d >= 1e+9) {
      exp += 4;
      d *= 1e-4;
    }
    if (d >= 1e+7) {
      exp += 2;
      d *= 1e-2;
    }
    if (d >= 1e+6) {
      exp += 1;
      d *= 1e-1;
    }
  } else {
    if (d < 1e-250) {
      exp -= 256;
      d *= 1e256;
    }
    if (d < 1e-122) {
      exp -= 128;
      d *= 1e128;
    }
    if (d < 1e-58) {
      exp -= 64;
      d *= 1e64;
    }
    if (d < 1e-26) {
      exp -= 32;
      d *= 1e32;
    }
    if (d < 1e-10) {
      exp -= 16;
      d *= 1e16;
    }
    if (d < 1e-2) {
      exp -= 8;
      d *= 1e8;
    }
    if (d < 1e+2) {
      exp -= 4;
      d *= 1e4;
    }
    if (d < 1e+4) {
      exp -= 2;
      d *= 1e2;
    }
    if (d < 1e+5) {
      exp -= 1;
      d *= 1e1;
    }
  }
  // At this point, d is in the range [99999.5..999999.5) and exp is in the
  // range [-324..308]. Since we need to round d up, we want to add a half
  // and truncate.
  // However, the technique above may have lost some precision, due to its
  // repeated multiplication by constants that each may be off by half a bit
  // of precision.  This only matters if we're close to the edge though.
  // Since we'd like to know if the fractional part of d is close to a half,
  // we multiply it by 65536 and see if the fractional part is close to 32768.
  // (The number doesn't have to be a power of two,but powers of two are faster)
  auto d64k = static_cast<uint64_t>(d * 65536);
  int dddddd; // A 6-digit decimal integer.
  if ((d64k % 65536) == 32767 || (d64k % 65536) == 32768) {
    // OK, it's fairly likely that precision was lost above, which is
    // not a surprise given only 52 mantissa bits are available.  Therefore
    // redo the calculation using 128-bit numbers.  (64 bits are not enough).

    // Start out with digits rounded down; maybe add one below.
    dddddd = static_cast<int>(d64k / 65536);

    // mantissa is a 64-bit integer representing M.mmm... * 2^63.  The actual
    // value we're representing, of course, is M.mmm... * 2^exp2.
    int exp2;
    double m = std::frexp(value, &exp2);
    auto mantissa = static_cast<uint64_t>(m * (32768.0 * 65536.0 * 65536.0 * 65536.0));
    // std::frexp returns an m value in the range [0.5, 1.0), however we
    // can't multiply it by 2^64 and convert to an integer because some FPUs
    // throw an exception when converting an number higher than 2^63 into an
    // integer - even an unsigned 64-bit integer!  Fortunately it doesn't matter
    // since m only has 52 significant bits anyway.
    mantissa <<= 1;
    exp2 -= 64; // not needed, but nice for debugging

    // OK, we are here to compare:
    //     (dddddd + 0.5) * 10^(exp-5)  vs.  mantissa * 2^exp2
    // so we can round up dddddd if appropriate.  Those values span the full
    // range of 600 orders of magnitude of IEE 64-bit floating-point.
    // Fortunately, we already know they are very close, so we don't need to
    // track the base-2 exponent of both sides.  This greatly simplifies the
    // the math since the 2^exp2 calculation is unnecessary and the power-of-10
    // calculation can become a power-of-5 instead.

    std::pair<uint64_t, uint64_t> edge;
    std::pair<uint64_t, uint64_t> val;
    if (exp >= 6) {
      // Compare (dddddd + 0.5) * 5 ^ (exp - 5) to mantissa
      // Since we're tossing powers of two, 2 * dddddd + 1 is the
      // same as dddddd + 0.5
      edge = PowFive(2 * dddddd + 1, exp - 5);

      val.first = mantissa;
      val.second = 0;
    } else {
      // We can't compare (dddddd + 0.5) * 5 ^ (exp - 5) to mantissa as we did
      // above because (exp - 5) is negative.  So we compare (dddddd + 0.5) to
      // mantissa * 5 ^ (5 - exp)
      edge = PowFive(2 * dddddd + 1, 0);

      val = PowFive(mantissa, 5 - exp);
    }
    // printf("exp=%d %016lx %016lx vs %016lx %016lx\n", exp, val.first,
    //        val.second, edge.first, edge.second);
    if (val > edge) {
      dddddd++;
    } else if (val == edge) {
      dddddd += (dddddd & 1);
    }
  } else {
    // Here, we are not close to the edge.
    dddddd = static_cast<int>((d64k + 32768) / 65536);
  }
  if (dddddd == 1000000) {
    dddddd = 100000;
    exp += 1;
  }
  exp_dig.exponent = exp;

  int two_digits = dddddd / 10000;
  dddddd -= two_digits * 10000;
  PutTwoDigits(two_digits, &exp_dig.digits[0]);

  two_digits = dddddd / 100;
  dddddd -= two_digits * 100;
  PutTwoDigits(two_digits, &exp_dig.digits[2]);

  PutTwoDigits(dddddd, &exp_dig.digits[4]);
  return exp_dig;
}

struct ExpDigitsA {
  int32_t exponent;
  char digits[6];
};

// SplitToSix converts value, a positive double-precision floating-point number,
// into a base-10 exponent and 6 ASCII digits, where the first digit is never
// zero.  For example, SplitToSix(1) returns an exponent of zero and a digits
// array of {'1', '0', '0', '0', '0', '0'}.  If value is exactly halfway between
// two possible representations, e.g. value = 100000.5, then "round to even" is
// performed.
inline ExpDigitsA SplitToSixA(const double value) {
  ExpDigitsA exp_dig;
  int exp = 5;
  double d = value;
  // First step: calculate a close approximation of the output, where the
  // value d will be between 100,000 and 999,999, representing the digits
  // in the output ASCII array, and exp is the base-10 exponent.  It would be
  // faster to use a table here, and to look up the base-2 exponent of value,
  // however value is an IEEE-754 64-bit number, so the table would have 2,000
  // entries, which is not cache-friendly.
  if (d >= 999999.5) {
    if (d >= 1e+261) {
      exp += 256;
      d *= 1e-256;
    }
    if (d >= 1e+133) {
      exp += 128;
      d *= 1e-128;
    }
    if (d >= 1e+69) {
      exp += 64;
      d *= 1e-64;
    }
    if (d >= 1e+37) {
      exp += 32;
      d *= 1e-32;
    }
    if (d >= 1e+21) {
      exp += 16;
      d *= 1e-16;
    }
    if (d >= 1e+13) {
      exp += 8;
      d *= 1e-8;
    }
    if (d >= 1e+9) {
      exp += 4;
      d *= 1e-4;
    }
    if (d >= 1e+7) {
      exp += 2;
      d *= 1e-2;
    }
    if (d >= 1e+6) {
      exp += 1;
      d *= 1e-1;
    }
  } else {
    if (d < 1e-250) {
      exp -= 256;
      d *= 1e256;
    }
    if (d < 1e-122) {
      exp -= 128;
      d *= 1e128;
    }
    if (d < 1e-58) {
      exp -= 64;
      d *= 1e64;
    }
    if (d < 1e-26) {
      exp -= 32;
      d *= 1e32;
    }
    if (d < 1e-10) {
      exp -= 16;
      d *= 1e16;
    }
    if (d < 1e-2) {
      exp -= 8;
      d *= 1e8;
    }
    if (d < 1e+2) {
      exp -= 4;
      d *= 1e4;
    }
    if (d < 1e+4) {
      exp -= 2;
      d *= 1e2;
    }
    if (d < 1e+5) {
      exp -= 1;
      d *= 1e1;
    }
  }
  // At this point, d is in the range [99999.5..999999.5) and exp is in the
  // range [-324..308]. Since we need to round d up, we want to add a half
  // and truncate.
  // However, the technique above may have lost some precision, due to its
  // repeated multiplication by constants that each may be off by half a bit
  // of precision.  This only matters if we're close to the edge though.
  // Since we'd like to know if the fractional part of d is close to a half,
  // we multiply it by 65536 and see if the fractional part is close to 32768.
  // (The number doesn't have to be a power of two,but powers of two are faster)
  auto d64k = static_cast<uint64_t>(d * 65536);
  int dddddd; // A 6-digit decimal integer.
  if ((d64k % 65536) == 32767 || (d64k % 65536) == 32768) {
    // OK, it's fairly likely that precision was lost above, which is
    // not a surprise given only 52 mantissa bits are available.  Therefore
    // redo the calculation using 128-bit numbers.  (64 bits are not enough).

    // Start out with digits rounded down; maybe add one below.
    dddddd = static_cast<int>(d64k / 65536);

    // mantissa is a 64-bit integer representing M.mmm... * 2^63.  The actual
    // value we're representing, of course, is M.mmm... * 2^exp2.
    int exp2;
    double m = std::frexp(value, &exp2);
    auto mantissa = static_cast<uint64_t>(m * (32768.0 * 65536.0 * 65536.0 * 65536.0));
    // std::frexp returns an m value in the range [0.5, 1.0), however we
    // can't multiply it by 2^64 and convert to an integer because some FPUs
    // throw an exception when converting an number higher than 2^63 into an
    // integer - even an unsigned 64-bit integer!  Fortunately it doesn't matter
    // since m only has 52 significant bits anyway.
    mantissa <<= 1;
    exp2 -= 64; // not needed, but nice for debugging

    // OK, we are here to compare:
    //     (dddddd + 0.5) * 10^(exp-5)  vs.  mantissa * 2^exp2
    // so we can round up dddddd if appropriate.  Those values span the full
    // range of 600 orders of magnitude of IEE 64-bit floating-point.
    // Fortunately, we already know they are very close, so we don't need to
    // track the base-2 exponent of both sides.  This greatly simplifies the
    // the math since the 2^exp2 calculation is unnecessary and the power-of-10
    // calculation can become a power-of-5 instead.

    std::pair<uint64_t, uint64_t> edge;
    std::pair<uint64_t, uint64_t> val;
    if (exp >= 6) {
      // Compare (dddddd + 0.5) * 5 ^ (exp - 5) to mantissa
      // Since we're tossing powers of two, 2 * dddddd + 1 is the
      // same as dddddd + 0.5
      edge = PowFive(2 * dddddd + 1, exp - 5);

      val.first = mantissa;
      val.second = 0;
    } else {
      // We can't compare (dddddd + 0.5) * 5 ^ (exp - 5) to mantissa as we did
      // above because (exp - 5) is negative.  So we compare (dddddd + 0.5) to
      // mantissa * 5 ^ (5 - exp)
      edge = PowFive(2 * dddddd + 1, 0);

      val = PowFive(mantissa, 5 - exp);
    }
    // printf("exp=%d %016lx %016lx vs %016lx %016lx\n", exp, val.first,
    //        val.second, edge.first, edge.second);
    if (val > edge) {
      dddddd++;
    } else if (val == edge) {
      dddddd += (dddddd & 1);
    }
  } else {
    // Here, we are not close to the edge.
    dddddd = static_cast<int>((d64k + 32768) / 65536);
  }
  if (dddddd == 1000000) {
    dddddd = 100000;
    exp += 1;
  }
  exp_dig.exponent = exp;

  int two_digits = dddddd / 10000;
  dddddd -= two_digits * 10000;
  PutTwoDigits(two_digits, &exp_dig.digits[0]);

  two_digits = dddddd / 100;
  dddddd -= two_digits * 100;
  PutTwoDigits(two_digits, &exp_dig.digits[2]);

  PutTwoDigits(dddddd, &exp_dig.digits[4]);
  return exp_dig;
}

// Helper function for fast formatting of floating-point.
// The result is the same as "%g", a.k.a. "%.6g".
size_t SixDigitsToBuffer(double d, wchar_t *const buffer) {
  static_assert(std::numeric_limits<float>::is_iec559, "IEEE-754/IEC-559 support only");

  wchar_t *out = buffer; // we write data to out, incrementing as we go, but
                         // FloatToBuffer always returns the address of the
                         // buffer passed in.

  if (std::isnan(d)) {
    MemCopy(out, L"nan\0", 4); // NOLINT(runtime/printf)
    return 3;
  }
  if (d == 0) { // +0 and -0 are handled here
    if (std::signbit(d)) {
      *out++ = '-';
    }
    *out++ = '0';
    *out = 0;
    return out - buffer;
  }
  if (d < 0) {
    *out++ = '-';
    d = -d;
  }
  if (d > (std::numeric_limits<double>::max)()) {
    MemCopy(out, L"inf\0", 4); // NOLINT(runtime/printf)
    return out + 3 - buffer;
  }

  auto exp_dig = SplitToSix(d);
  int exp = exp_dig.exponent;
  const wchar_t *digits = exp_dig.digits;
  out[0] = L'0';
  out[1] = L'.';
  switch (exp) {
  case 5:
    MemCopy(out, &digits[0], 6), out += 6;
    *out = 0;
    return out - buffer;
  case 4:
    MemCopy(out, &digits[0], 5), out += 5;
    if (digits[5] != '0') {
      *out++ = '.';
      *out++ = digits[5];
    }
    *out = 0;
    return out - buffer;
  case 3:
    MemCopy(out, &digits[0], 4), out += 4;
    if ((digits[5] | digits[4]) != '0') {
      *out++ = '.';
      *out++ = digits[4];
      if (digits[5] != '0') {
        *out++ = digits[5];
      }
    }
    *out = 0;
    return out - buffer;
  case 2:
    MemCopy(out, &digits[0], 3), out += 3;
    *out++ = '.';
    MemCopy(out, &digits[3], 3);
    out += 3;
    while (out[-1] == '0') {
      --out;
    }
    if (out[-1] == '.') {
      --out;
    }
    *out = 0;
    return out - buffer;
  case 1:
    MemCopy(out, &digits[0], 2), out += 2;
    *out++ = '.';
    MemCopy(out, &digits[2], 4);
    out += 4;
    while (out[-1] == '0') {
      --out;
    }
    if (out[-1] == '.') {
      --out;
    }
    *out = 0;
    return out - buffer;
  case 0:
    MemCopy(out, &digits[0], 1), out += 1;
    *out++ = '.';
    MemCopy(out, &digits[1], 5);
    out += 5;
    while (out[-1] == '0') {
      --out;
    }
    if (out[-1] == '.') {
      --out;
    }
    *out = 0;
    return out - buffer;
  case -4:
    out[2] = '0';
    ++out;
    [[fallthrough]];
  case -3:
    out[2] = '0';
    ++out;
    [[fallthrough]];
  case -2:
    out[2] = '0';
    ++out;
    [[fallthrough]];
  case -1:
    out += 2;
    MemCopy(out, &digits[0], 6);
    out += 6;
    while (out[-1] == '0') {
      --out;
    }
    *out = 0;
    return out - buffer;
  }
  out[0] = digits[0];
  out += 2;
  MemCopy(out, &digits[1], 5), out += 5;
  while (out[-1] == '0') {
    --out;
  }
  if (out[-1] == '.') {
    --out;
  }
  *out++ = 'e';
  if (exp > 0) {
    *out++ = '+';
  } else {
    *out++ = '-';
    exp = -exp;
  }
  if (exp > 99) {
    int dig1 = exp / 100;
    exp -= dig1 * 100;
    *out++ = static_cast<wchar_t>(L'0' + dig1);
  }
  PutTwoDigits(exp, out);
  out += 2;
  *out = 0;
  return out - buffer;
}

// Helper function for fast formatting of floating-point.
// The result is the same as "%g", a.k.a. "%.6g".
size_t SixDigitsToBuffer(double d, char *const buffer) {
  static_assert(std::numeric_limits<float>::is_iec559, "IEEE-754/IEC-559 support only");

  char *out = buffer; // we write data to out, incrementing as we go, but
                      // FloatToBuffer always returns the address of the
                      // buffer passed in.

  if (std::isnan(d)) {
    memcpy(out, "nan\0", 4); // NOLINT(runtime/printf)
    return 3;
  }
  if (d == 0) { // +0 and -0 are handled here
    if (std::signbit(d)) {
      *out++ = '-';
    }
    *out++ = '0';
    *out = 0;
    return out - buffer;
  }
  if (d < 0) {
    *out++ = '-';
    d = -d;
  }
  if (d > (std::numeric_limits<double>::max)()) {
    memcpy(out, "inf\0", 4); // NOLINT(runtime/printf)
    return out + 3 - buffer;
  }

  auto exp_dig = SplitToSixA(d);
  int exp = exp_dig.exponent;
  const char *digits = exp_dig.digits;
  out[0] = '0';
  out[1] = '.';
  switch (exp) {
  case 5:
    memcpy(out, &digits[0], 6), out += 6;
    *out = 0;
    return out - buffer;
  case 4:
    memcpy(out, &digits[0], 5), out += 5;
    if (digits[5] != '0') {
      *out++ = '.';
      *out++ = digits[5];
    }
    *out = 0;
    return out - buffer;
  case 3:
    memcpy(out, &digits[0], 4), out += 4;
    if ((digits[5] | digits[4]) != '0') {
      *out++ = '.';
      *out++ = digits[4];
      if (digits[5] != '0') {
        *out++ = digits[5];
      }
    }
    *out = 0;
    return out - buffer;
  case 2:
    memcpy(out, &digits[0], 3), out += 3;
    *out++ = '.';
    memcpy(out, &digits[3], 3);
    out += 3;
    while (out[-1] == '0') {
      --out;
    }
    if (out[-1] == '.') {
      --out;
    }
    *out = 0;
    return out - buffer;
  case 1:
    memcpy(out, &digits[0], 2), out += 2;
    *out++ = '.';
    memcpy(out, &digits[2], 4);
    out += 4;
    while (out[-1] == '0') {
      --out;
    }
    if (out[-1] == '.') {
      --out;
    }
    *out = 0;
    return out - buffer;
  case 0:
    memcpy(out, &digits[0], 1), out += 1;
    *out++ = '.';
    memcpy(out, &digits[1], 5);
    out += 5;
    while (out[-1] == '0') {
      --out;
    }
    if (out[-1] == '.') {
      --out;
    }
    *out = 0;
    return out - buffer;
  case -4:
    out[2] = '0';
    ++out;
    [[fallthrough]];
  case -3:
    out[2] = '0';
    ++out;
    [[fallthrough]];
  case -2:
    out[2] = '0';
    ++out;
    [[fallthrough]];
  case -1:
    out += 2;
    memcpy(out, &digits[0], 6);
    out += 6;
    while (out[-1] == '0') {
      --out;
    }
    *out = 0;
    return out - buffer;
  }
  out[0] = digits[0];
  out += 2;
  memcpy(out, &digits[1], 5), out += 5;
  while (out[-1] == '0') {
    --out;
  }
  if (out[-1] == '.') {
    --out;
  }
  *out++ = 'e';
  if (exp > 0) {
    *out++ = '+';
  } else {
    *out++ = '-';
    exp = -exp;
  }
  if (exp > 99) {
    int dig1 = exp / 100;
    exp -= dig1 * 100;
    *out++ = static_cast<char>('0' + dig1);
  }
  PutTwoDigits(exp, out);
  out += 2;
  *out = 0;
  return out - buffer;
}

// Represents integer values of digits.
// Uses 36 to indicate an invalid character since we support
// bases up to 36.
static const int8_t kAsciiToInt[256] = {
    36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, // 16 36s.
    36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
    36, 36, 36, 36, 36, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  36, 36, 36, 36, 36, 36, 36, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 36, 36, 36, 36, 36,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
    36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
    36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
    36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
    36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
    36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36};

// Parse the sign and optional hex or oct prefix in text.
inline bool safe_parse_sign_and_base(std::wstring_view *text /*inout*/, int *base_ptr /*inout*/,
                                     bool *negative_ptr /*output*/) {
  if (text->data() == nullptr) {
    return false;
  }

  const wchar_t *start = text->data();
  const wchar_t *end = start + text->size();
  int base = *base_ptr;

  // Consume whitespace.
  while (start < end && ascii_isspace(start[0])) {
    ++start;
  }
  while (start < end && ascii_isspace(end[-1])) {
    --end;
  }
  if (start >= end) {
    return false;
  }

  // Consume sign.
  *negative_ptr = (start[0] == '-');
  if (*negative_ptr || start[0] == '+') {
    ++start;
    if (start >= end) {
      return false;
    }
  }

  // Consume base-dependent prefix.
  //  base 0: "0x" -> base 16, "0" -> base 8, default -> base 10
  //  base 16: "0x" -> base 16
  // Also validate the base.
  if (base == 0) {
    if (end - start >= 2 && start[0] == '0' && (start[1] == 'x' || start[1] == 'X')) {
      base = 16;
      start += 2;
      if (start >= end) {
        // "0x" with no digits after is invalid.
        return false;
      }
    } else if (end - start >= 1 && start[0] == '0') {
      base = 8;
      start += 1;
    } else {
      base = 10;
    }
  } else if (base == 16) {
    if (end - start >= 2 && start[0] == '0' && (start[1] == 'x' || start[1] == 'X')) {
      start += 2;
      if (start >= end) {
        // "0x" with no digits after is invalid.
        return false;
      }
    }
  } else if (base >= 2 && base <= 36) {
    // okay
  } else {
    return false;
  }
  *text = std::wstring_view(start, end - start);
  *base_ptr = base;
  return true;
}

// Parse the sign and optional hex or oct prefix in text.
inline bool safe_parse_sign_and_base(std::string_view *text /*inout*/, int *base_ptr /*inout*/,
                                     bool *negative_ptr /*output*/) {
  if (text->data() == nullptr) {
    return false;
  }

  const char *start = text->data();
  const char *end = start + text->size();
  int base = *base_ptr;

  // Consume whitespace.
  while (start < end && ascii_isspace(static_cast<char8_t>(start[0]))) {
    ++start;
  }
  while (start < end && ascii_isspace(static_cast<char8_t>(end[-1]))) {
    --end;
  }
  if (start >= end) {
    return false;
  }

  // Consume sign.
  *negative_ptr = (start[0] == '-');
  if (*negative_ptr || start[0] == '+') {
    ++start;
    if (start >= end) {
      return false;
    }
  }

  // Consume base-dependent prefix.
  //  base 0: "0x" -> base 16, "0" -> base 8, default -> base 10
  //  base 16: "0x" -> base 16
  // Also validate the base.
  if (base == 0) {
    if (end - start >= 2 && start[0] == '0' && (start[1] == 'x' || start[1] == 'X')) {
      base = 16;
      start += 2;
      if (start >= end) {
        // "0x" with no digits after is invalid.
        return false;
      }
    } else if (end - start >= 1 && start[0] == '0') {
      base = 8;
      start += 1;
    } else {
      base = 10;
    }
  } else if (base == 16) {
    if (end - start >= 2 && start[0] == '0' && (start[1] == 'x' || start[1] == 'X')) {
      start += 2;
      if (start >= end) {
        // "0x" with no digits after is invalid.
        return false;
      }
    }
  } else if (base >= 2 && base <= 36) {
    // okay
  } else {
    return false;
  }
  *text = std::string_view(start, end - start);
  *base_ptr = base;
  return true;
}

// Consume digits.
//
// The classic loop:
//
//   for each digit
//     value = value * base + digit
//   value *= sign
//
// The classic loop needs overflow checking.  It also fails on the most
// negative integer, -2147483648 in 32-bit two's complement representation.
//
// My improved loop:
//
//  if (!negative)
//    for each digit
//      value = value * base
//      value = value + digit
//  else
//    for each digit
//      value = value * base
//      value = value - digit
//
// Overflow checking becomes simple.

// Lookup tables per IntType:
// vmax/base and vmin/base are precomputed because division costs at least 8ns.
// TODO(junyer): Doing this per base instead (i.e. an array of structs, not a
// struct of arrays) would probably be better in terms of d-cache for the most
// commonly used bases.
template <typename IntType> struct LookupTables {
  static const IntType kVmaxOverBase[];
  static const IntType kVminOverBase[];
};

// An array initializer macro for X/base where base in [0, 36].
// However, note that lookups for base in [0, 1] should never happen because
// base has been validated to be in [2, 36] by safe_parse_sign_and_base().
#define X_OVER_BASE_INITIALIZER(X)                                                                                     \
  {                                                                                                                    \
      0,        0,        (X) / 2,  (X) / 3,  (X) / 4,  (X) / 5,  (X) / 6,  (X) / 7,  (X) / 8,  (X) / 9,               \
      (X) / 10, (X) / 11, (X) / 12, (X) / 13, (X) / 14, (X) / 15, (X) / 16, (X) / 17, (X) / 18, (X) / 19,              \
      (X) / 20, (X) / 21, (X) / 22, (X) / 23, (X) / 24, (X) / 25, (X) / 26, (X) / 27, (X) / 28, (X) / 29,              \
      (X) / 30, (X) / 31, (X) / 32, (X) / 33, (X) / 34, (X) / 35, (X) / 36,                                            \
  }

// This kVmaxOverBase is generated with
//  for (int base = 2; base < 37; ++base) {
//    bela::uint128 max = std::numeric_limits<bela::uint128>::max();
//    auto result = max / base;
//    std::cout << "    MakeUint128(" << bela::uint128High64(result) << "u, "
//              << bela::uint128Low64(result) << "u),\n";
//  }
// See https://godbolt.org/z/aneYsb
//
// uint128& operator/=(uint128) is not constexpr, so hardcode the resulting
// array to avoid a static initializer.
template <>
const uint128 LookupTables<uint128>::kVmaxOverBase[] = {
    0,
    0,
    MakeUint128(9223372036854775807U, 18446744073709551615U),
    MakeUint128(6148914691236517205U, 6148914691236517205U),
    MakeUint128(4611686018427387903U, 18446744073709551615U),
    MakeUint128(3689348814741910323U, 3689348814741910323U),
    MakeUint128(3074457345618258602U, 12297829382473034410U),
    MakeUint128(2635249153387078802U, 5270498306774157604U),
    MakeUint128(2305843009213693951U, 18446744073709551615U),
    MakeUint128(2049638230412172401U, 14347467612885206812U),
    MakeUint128(1844674407370955161U, 11068046444225730969U),
    MakeUint128(1676976733973595601U, 8384883669867978007U),
    MakeUint128(1537228672809129301U, 6148914691236517205U),
    MakeUint128(1418980313362273201U, 4256940940086819603U),
    MakeUint128(1317624576693539401U, 2635249153387078802U),
    MakeUint128(1229782938247303441U, 1229782938247303441U),
    MakeUint128(1152921504606846975U, 18446744073709551615U),
    MakeUint128(1085102592571150095U, 1085102592571150095U),
    MakeUint128(1024819115206086200U, 16397105843297379214U),
    MakeUint128(970881267037344821U, 16504981539634861972U),
    MakeUint128(922337203685477580U, 14757395258967641292U),
    MakeUint128(878416384462359600U, 14054662151397753612U),
    MakeUint128(838488366986797800U, 13415813871788764811U),
    MakeUint128(802032351030850070U, 4812194106185100421U),
    MakeUint128(768614336404564650U, 12297829382473034410U),
    MakeUint128(737869762948382064U, 11805916207174113034U),
    MakeUint128(709490156681136600U, 11351842506898185609U),
    MakeUint128(683212743470724133U, 17080318586768103348U),
    MakeUint128(658812288346769700U, 10540996613548315209U),
    MakeUint128(636094623231363848U, 15266270957552732371U),
    MakeUint128(614891469123651720U, 9838263505978427528U),
    MakeUint128(595056260442243600U, 9520900167075897608U),
    MakeUint128(576460752303423487U, 18446744073709551615U),
    MakeUint128(558992244657865200U, 8943875914525843207U),
    MakeUint128(542551296285575047U, 9765923333140350855U),
    MakeUint128(527049830677415760U, 8432797290838652167U),
    MakeUint128(512409557603043100U, 8198552921648689607U),
};

// This kVmaxOverBase generated with
//   for (int base = 2; base < 37; ++base) {
//    bela::int128 max = std::numeric_limits<bela::int128>::max();
//    auto result = max / base;
//    std::cout << "\tMakeInt128(" << bela::int128High64(result) << ", "
//              << bela::int128Low64(result) << "u),\n";
//  }
// See https://godbolt.org/z/7djYWz
//
// int128& operator/=(int128) is not constexpr, so hardcode the resulting array
// to avoid a static initializer.
template <>
const int128 LookupTables<int128>::kVmaxOverBase[] = {
    0,
    0,
    MakeInt128(4611686018427387903, 18446744073709551615U),
    MakeInt128(3074457345618258602, 12297829382473034410U),
    MakeInt128(2305843009213693951, 18446744073709551615U),
    MakeInt128(1844674407370955161, 11068046444225730969U),
    MakeInt128(1537228672809129301, 6148914691236517205U),
    MakeInt128(1317624576693539401, 2635249153387078802U),
    MakeInt128(1152921504606846975, 18446744073709551615U),
    MakeInt128(1024819115206086200, 16397105843297379214U),
    MakeInt128(922337203685477580, 14757395258967641292U),
    MakeInt128(838488366986797800, 13415813871788764811U),
    MakeInt128(768614336404564650, 12297829382473034410U),
    MakeInt128(709490156681136600, 11351842506898185609U),
    MakeInt128(658812288346769700, 10540996613548315209U),
    MakeInt128(614891469123651720, 9838263505978427528U),
    MakeInt128(576460752303423487, 18446744073709551615U),
    MakeInt128(542551296285575047, 9765923333140350855U),
    MakeInt128(512409557603043100, 8198552921648689607U),
    MakeInt128(485440633518672410, 17475862806672206794U),
    MakeInt128(461168601842738790, 7378697629483820646U),
    MakeInt128(439208192231179800, 7027331075698876806U),
    MakeInt128(419244183493398900, 6707906935894382405U),
    MakeInt128(401016175515425035, 2406097053092550210U),
    MakeInt128(384307168202282325, 6148914691236517205U),
    MakeInt128(368934881474191032, 5902958103587056517U),
    MakeInt128(354745078340568300, 5675921253449092804U),
    MakeInt128(341606371735362066, 17763531330238827482U),
    MakeInt128(329406144173384850, 5270498306774157604U),
    MakeInt128(318047311615681924, 7633135478776366185U),
    MakeInt128(307445734561825860, 4919131752989213764U),
    MakeInt128(297528130221121800, 4760450083537948804U),
    MakeInt128(288230376151711743, 18446744073709551615U),
    MakeInt128(279496122328932600, 4471937957262921603U),
    MakeInt128(271275648142787523, 14106333703424951235U),
    MakeInt128(263524915338707880, 4216398645419326083U),
    MakeInt128(256204778801521550, 4099276460824344803U),
};

// This kVminOverBase generated with
//  for (int base = 2; base < 37; ++base) {
//    bela::int128 min = std::numeric_limits<bela::int128>::min();
//    auto result = min / base;
//    std::cout << "\tMakeInt128(" << bela::int128High64(result) << ", "
//              << bela::int128Low64(result) << "u),\n";
//  }
//
// See https://godbolt.org/z/7djYWz
//
// int128& operator/=(int128) is not constexpr, so hardcode the resulting array
// to avoid a static initializer.
template <>
const int128 LookupTables<int128>::kVminOverBase[] = {
    0,
    0,
    MakeInt128(-4611686018427387904, 0U),
    MakeInt128(-3074457345618258603, 6148914691236517206U),
    MakeInt128(-2305843009213693952, 0U),
    MakeInt128(-1844674407370955162, 7378697629483820647U),
    MakeInt128(-1537228672809129302, 12297829382473034411U),
    MakeInt128(-1317624576693539402, 15811494920322472814U),
    MakeInt128(-1152921504606846976, 0U),
    MakeInt128(-1024819115206086201, 2049638230412172402U),
    MakeInt128(-922337203685477581, 3689348814741910324U),
    MakeInt128(-838488366986797801, 5030930201920786805U),
    MakeInt128(-768614336404564651, 6148914691236517206U),
    MakeInt128(-709490156681136601, 7094901566811366007U),
    MakeInt128(-658812288346769701, 7905747460161236407U),
    MakeInt128(-614891469123651721, 8608480567731124088U),
    MakeInt128(-576460752303423488, 0U),
    MakeInt128(-542551296285575048, 8680820740569200761U),
    MakeInt128(-512409557603043101, 10248191152060862009U),
    MakeInt128(-485440633518672411, 970881267037344822U),
    MakeInt128(-461168601842738791, 11068046444225730970U),
    MakeInt128(-439208192231179801, 11419412998010674810U),
    MakeInt128(-419244183493398901, 11738837137815169211U),
    MakeInt128(-401016175515425036, 16040647020617001406U),
    MakeInt128(-384307168202282326, 12297829382473034411U),
    MakeInt128(-368934881474191033, 12543785970122495099U),
    MakeInt128(-354745078340568301, 12770822820260458812U),
    MakeInt128(-341606371735362067, 683212743470724134U),
    MakeInt128(-329406144173384851, 13176245766935394012U),
    MakeInt128(-318047311615681925, 10813608594933185431U),
    MakeInt128(-307445734561825861, 13527612320720337852U),
    MakeInt128(-297528130221121801, 13686293990171602812U),
    MakeInt128(-288230376151711744, 0U),
    MakeInt128(-279496122328932601, 13974806116446630013U),
    MakeInt128(-271275648142787524, 4340410370284600381U),
    MakeInt128(-263524915338707881, 14230345428290225533U),
    MakeInt128(-256204778801521551, 14347467612885206813U),
};

template <typename IntType>
const IntType LookupTables<IntType>::kVmaxOverBase[] = X_OVER_BASE_INITIALIZER(std::numeric_limits<IntType>::max());

template <typename IntType>
const IntType LookupTables<IntType>::kVminOverBase[] = X_OVER_BASE_INITIALIZER(std::numeric_limits<IntType>::min());

#undef X_OVER_BASE_INITIALIZER

template <typename IntType> inline bool safe_parse_positive_int(std::wstring_view text, int base, IntType *value_p) {
  IntType value = 0;
  const IntType vmax = std::numeric_limits<IntType>::max();
  assert(vmax > 0);
  assert(base >= 0);
  assert(vmax >= static_cast<IntType>(base));
  const IntType vmax_over_base = LookupTables<IntType>::kVmaxOverBase[base];
  const wchar_t *start = text.data();
  const wchar_t *end = start + text.size();
  // loop over digits
  for (; start < end; ++start) {
    auto c = static_cast<unsigned char>(start[0]);
    int digit = kAsciiToInt[c];
    if (digit >= base) {
      *value_p = value;
      return false;
    }
    if (value > vmax_over_base) {
      *value_p = vmax;
      return false;
    }
    value *= base;
    if (value > vmax - digit) {
      *value_p = vmax;
      return false;
    }
    value += digit;
  }
  *value_p = value;
  return true;
}

template <typename IntType> inline bool safe_parse_negative_int(std::wstring_view text, int base, IntType *value_p) {
  IntType value = 0;
  const IntType vmin = std::numeric_limits<IntType>::min();
  assert(vmin < 0);
  assert(vmin <= 0 - base);
  IntType vmin_over_base = LookupTables<IntType>::kVminOverBase[base];
  // 2003 c++ standard [expr.mul]
  // "... the sign of the remainder is implementation-defined."
  // Although (vmin/base)*base + vmin%base is always vmin.
  // 2011 c++ standard tightens the spec but we cannot rely on it.
  // TODO(junyer): Handle this in the lookup table generation.
  if (vmin % base > 0) {
    vmin_over_base += 1;
  }
  const wchar_t *start = text.data();
  const wchar_t *end = start + text.size();
  // loop over digits
  for (; start < end; ++start) {
    auto c = static_cast<unsigned char>(start[0]);
    int digit = kAsciiToInt[c];
    if (digit >= base) {
      *value_p = value;
      return false;
    }
    if (value < vmin_over_base) {
      *value_p = vmin;
      return false;
    }
    value *= base;
    if (value < vmin + digit) {
      *value_p = vmin;
      return false;
    }
    value -= digit;
  }
  *value_p = value;
  return true;
}

// Input format based on POSIX.1-2008 strtol
// http://pubs.opengroup.org/onlinepubs/9699919799/functions/strtol.html
template <typename IntType> inline bool safe_int_internal(std::wstring_view text, IntType *value_p, int base) {
  *value_p = 0;
  bool negative;
  if (!safe_parse_sign_and_base(&text, &base, &negative)) {
    return false;
  }
  if (!negative) {
    return safe_parse_positive_int(text, base, value_p);
  }
  return safe_parse_negative_int(text, base, value_p);
}

template <typename IntType> inline bool safe_uint_internal(std::wstring_view text, IntType *value_p, int base) {
  *value_p = 0;
  bool negative;
  if (!safe_parse_sign_and_base(&text, &base, &negative) || negative) {
    return false;
  }
  return safe_parse_positive_int(text, base, value_p);
}
bool safe_strto32_base(std::wstring_view text, int32_t *value, int base) {
  return safe_int_internal<int32_t>(text, value, base);
}

bool safe_strto64_base(std::wstring_view text, int64_t *value, int base) {
  return safe_int_internal<int64_t>(text, value, base);
}

bool safe_strto128_base(std::wstring_view text, int128 *value, int base) {
  return safe_int_internal<bela::int128>(text, value, base);
}

bool safe_strtou32_base(std::wstring_view text, uint32_t *value, int base) {
  return safe_uint_internal<uint32_t>(text, value, base);
}

bool safe_strtou64_base(std::wstring_view text, uint64_t *value, int base) {
  return safe_uint_internal<uint64_t>(text, value, base);
}

bool safe_strtou128_base(std::wstring_view text, uint128 *value, int base) {
  return safe_uint_internal<bela::uint128>(text, value, base);
}

template <typename IntType> inline bool safe_parse_positive_int(std::string_view text, int base, IntType *value_p) {
  IntType value = 0;
  const IntType vmax = std::numeric_limits<IntType>::max();
  assert(vmax > 0);
  assert(base >= 0);
  assert(vmax >= static_cast<IntType>(base));
  const IntType vmax_over_base = LookupTables<IntType>::kVmaxOverBase[base];
  const char *start = text.data();
  const char *end = start + text.size();
  // loop over digits
  for (; start < end; ++start) {
    auto c = static_cast<unsigned char>(start[0]);
    int digit = kAsciiToInt[c];
    if (digit >= base) {
      *value_p = value;
      return false;
    }
    if (value > vmax_over_base) {
      *value_p = vmax;
      return false;
    }
    value *= base;
    if (value > vmax - digit) {
      *value_p = vmax;
      return false;
    }
    value += digit;
  }
  *value_p = value;
  return true;
}

template <typename IntType> inline bool safe_parse_negative_int(std::string_view text, int base, IntType *value_p) {
  IntType value = 0;
  const IntType vmin = std::numeric_limits<IntType>::min();
  assert(vmin < 0);
  assert(vmin <= 0 - base);
  IntType vmin_over_base = LookupTables<IntType>::kVminOverBase[base];
  // 2003 c++ standard [expr.mul]
  // "... the sign of the remainder is implementation-defined."
  // Although (vmin/base)*base + vmin%base is always vmin.
  // 2011 c++ standard tightens the spec but we cannot rely on it.
  // TODO(junyer): Handle this in the lookup table generation.
  if (vmin % base > 0) {
    vmin_over_base += 1;
  }
  const char *start = text.data();
  const char *end = start + text.size();
  // loop over digits
  for (; start < end; ++start) {
    auto c = static_cast<unsigned char>(start[0]);
    int digit = kAsciiToInt[c];
    if (digit >= base) {
      *value_p = value;
      return false;
    }
    if (value < vmin_over_base) {
      *value_p = vmin;
      return false;
    }
    value *= base;
    if (value < vmin + digit) {
      *value_p = vmin;
      return false;
    }
    value -= digit;
  }
  *value_p = value;
  return true;
}

// Input format based on POSIX.1-2008 strtol
// http://pubs.opengroup.org/onlinepubs/9699919799/functions/strtol.html
template <typename IntType> inline bool safe_int_internal(std::string_view text, IntType *value_p, int base) {
  *value_p = 0;
  bool negative;
  if (!safe_parse_sign_and_base(&text, &base, &negative)) {
    return false;
  }
  if (!negative) {
    return safe_parse_positive_int(text, base, value_p);
  }
  return safe_parse_negative_int(text, base, value_p);
}

template <typename IntType> inline bool safe_uint_internal(std::string_view text, IntType *value_p, int base) {
  *value_p = 0;
  bool negative;
  if (!safe_parse_sign_and_base(&text, &base, &negative) || negative) {
    return false;
  }
  return safe_parse_positive_int(text, base, value_p);
}
bool safe_strto32_base(std::string_view text, int32_t *value, int base) {
  return safe_int_internal<int32_t>(text, value, base);
}

bool safe_strto64_base(std::string_view text, int64_t *value, int base) {
  return safe_int_internal<int64_t>(text, value, base);
}

bool safe_strto128_base(std::string_view text, int128 *value, int base) {
  return safe_int_internal<bela::int128>(text, value, base);
}

bool safe_strtou32_base(std::string_view text, uint32_t *value, int base) {
  return safe_uint_internal<uint32_t>(text, value, base);
}

bool safe_strtou64_base(std::string_view text, uint64_t *value, int base) {
  return safe_uint_internal<uint64_t>(text, value, base);
}

bool safe_strtou128_base(std::string_view text, uint128 *value, int base) {
  return safe_uint_internal<bela::uint128>(text, value, base);
}

} // namespace numbers_internal

} // namespace bela
