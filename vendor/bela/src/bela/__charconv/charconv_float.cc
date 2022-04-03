// Port from Micrsoft STL
// charconv standard header

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <bela/charconv.hpp>
#include "ryu.hpp"

namespace bela {
using std::errc;

template <class FloatingType> struct Floating_type_traits;

template <> struct Floating_type_traits<float> {
  static constexpr int32_t Mantissa_bits = 24;             // FLT_MANT_DIG
  static constexpr int32_t Exponent_bits = 8;              // sizeof(float) * CHAR_BIT - FLT_MANT_DIG
  static constexpr int32_t Maximum_binary_exponent = 127;  // FLT_MAX_EXP - 1
  static constexpr int32_t Minimum_binary_exponent = -126; // FLT_MIN_EXP - 1
  static constexpr int32_t Exponent_bias = 127;
  static constexpr int32_t Sign_shift = 31;     // Exponent_bits + Mantissa_bits - 1
  static constexpr int32_t Exponent_shift = 23; // Mantissa_bits - 1

  using Uint_type = uint32_t;

  static constexpr uint32_t Exponent_mask = 0x000000FFU;             // (1u << Exponent_bits) - 1
  static constexpr uint32_t Normal_mantissa_mask = 0x00FFFFFFU;      // (1u << Mantissa_bits) - 1
  static constexpr uint32_t Denormal_mantissa_mask = 0x007FFFFFU;    // (1u << (Mantissa_bits - 1)) - 1
  static constexpr uint32_t Special_nan_mantissa_mask = 0x00400000U; // 1u << (Mantissa_bits - 2)
  static constexpr uint32_t Shifted_sign_mask = 0x80000000U;         // 1u << Sign_shift
  static constexpr uint32_t Shifted_exponent_mask = 0x7F800000U;     // Exponent_mask << Exponent_shift
};

template <> struct Floating_type_traits<double> {
  static constexpr int32_t Mantissa_bits = 53;              // DBL_MANT_DIG
  static constexpr int32_t Exponent_bits = 11;              // sizeof(double) * CHAR_BIT - DBL_MANT_DIG
  static constexpr int32_t Maximum_binary_exponent = 1023;  // DBL_MAX_EXP - 1
  static constexpr int32_t Minimum_binary_exponent = -1022; // DBL_MIN_EXP - 1
  static constexpr int32_t Exponent_bias = 1023;
  static constexpr int32_t Sign_shift = 63;     // Exponent_bits + Mantissa_bits - 1
  static constexpr int32_t Exponent_shift = 52; // Mantissa_bits - 1

  using Uint_type = uint64_t;

  static constexpr uint64_t Exponent_mask = 0x00000000000007FFU;             // (1ULL << Exponent_bits) - 1
  static constexpr uint64_t Normal_mantissa_mask = 0x001FFFFFFFFFFFFFU;      // (1ULL << Mantissa_bits) - 1
  static constexpr uint64_t Denormal_mantissa_mask = 0x000FFFFFFFFFFFFFU;    // (1ULL << (Mantissa_bits - 1)) - 1
  static constexpr uint64_t Special_nan_mantissa_mask = 0x0008000000000000U; // 1ULL << (Mantissa_bits - 2)
  static constexpr uint64_t Shifted_sign_mask = 0x8000000000000000U;         // 1ULL << Sign_shift
  static constexpr uint64_t Shifted_exponent_mask = 0x7FF0000000000000U;     // Exponent_mask << Exponent_shift
};

template <> struct Floating_type_traits<long double> : Floating_type_traits<double> {};

[[nodiscard]] inline uint32_t bit_scan_reverse(const uint32_t value) noexcept {
  unsigned long Index; // Intentionally uninitialized for better codegen

  if (_BitScanReverse(&Index, value) != 0u) {
    return Index + 1;
  }

  return 0;
}

[[nodiscard]] inline uint32_t bit_scan_reverse(const uint64_t value) noexcept {
  unsigned long Index; // Intentionally uninitialized for better codegen

#ifdef _WIN64
  if (_BitScanReverse64(&Index, value) != 0u) {
    return Index + 1;
  }
#else  // ^^^ 64-bit ^^^ / vvv 32-bit vvv
  uint32_t _Ui32 = static_cast<uint32_t>(value >> 32);

  if (_BitScanReverse(&Index, _Ui32)) {
    return Index + 1 + 32;
  }

  _Ui32 = static_cast<uint32_t>(value);

  if (_BitScanReverse(&Index, _Ui32)) {
    return Index + 1;
  }
#endif // ^^^ 32-bit ^^^

  return 0;
}

inline constexpr wchar_t Charconv_digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b',
                                              'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                              'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

[[nodiscard]] inline unsigned char Digit_from_char(const wchar_t C) noexcept {
  auto ch = static_cast<uint16_t>(C);
  if (ch > 255) {
    return 255;
  }
  static constexpr unsigned char digit_from_byte[] = {
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255, 255, 255, 255, 255, 255, 10,
      11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
      33,  34,  35,  255, 255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,
      23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
  static_assert(std::size(digit_from_byte) == 256);

  return digit_from_byte[static_cast<unsigned char>(C)];
}

// vvvvvvvvvv DERIVED FROM corecrt_internal_big_integer.h vvvvvvvvvv

// A lightweight, sufficiently functional high-precision integer type for use in the binary floating-point <=> decimal
// string conversions. We define only the operations (and in some cases, parts of operations) that are actually used.

// We require sufficient precision to represent the reciprocal of the smallest representable value (the smallest
// denormal, 2^-1074). During parsing, we may also consider up to 768 decimal digits. For this, we require an
// additional log2(10^768) bits of precision. Finally, we require 54 bits of space for pre-division numerator shifting,
// because double explicitly stores 52 bits, implicitly stores 1 bit, and we need 1 more bit for rounding.

// PERFORMANCE NOTE: We intentionally do not initialize the Mydata array when a Big_integer_flt object is constructed.
// Profiling showed that zero-initialization caused a substantial performance hit. Initialization of the Mydata
// array is not necessary: all operations on the Big_integer_flt type are carefully written to only access elements at
// indices [0, Myused), and all operations correctly update Myused as the utilized size increases.

// Big_integer_flt Xval{}; is direct-list-initialization (N4750 11.6.4 [dcl.init.list]/1).
// N4750 11.6.4 [dcl.init.list]/3.5:
// "Otherwise, if the initializer list has no elements and T is a class type with a default constructor,
// the object is value-initialized."
// N4750 11.6 [dcl.init]/8, /8.1:
// "To value-initialize an object of type T means:
// - if T is a (possibly cv-qualified) class type (Clause 12) with either no default constructor (15.1)
// or a default constructor that is user-provided or deleted, then the object is default-initialized;"
// N4750 11.6 [dcl.init]/7, /7.1:
// "To default-initialize an object of type T means:
// - If T is a (possibly cv-qualified) class type (Clause 12), constructors are considered. The applicable constructors
// are enumerated (16.3.1.3), and the best one for the initializer () is chosen through overload resolution (16.3).
// The constructor thus selected is called, with an empty argument list, to initialize the object."
// N4750 15.6.2 [class.base.init]/9, /9.3:
// "In a non-delegating constructor, if a given potentially constructed subobject is not designated by a
// mem-initializer-id (including the case where there is no mem-initializer-list because the constructor has no
// ctor-initializer), then [...] - otherwise, the entity is default-initialized (11.6)."
// N4750 11.6 [dcl.init]/7, /7.2, /7.3:
// "To default-initialize an object of type T means: [...]
// - If T is an array type, each element is default-initialized.
// - Otherwise, no initialization is performed."
// Therefore, Mydata's elements are not initialized.
struct Big_integer_flt {
  Big_integer_flt() noexcept {}

  Big_integer_flt(const Big_integer_flt &other) noexcept : Myused(other.Myused) {
    memcpy(Mydata, other.Mydata, other.Myused * sizeof(uint32_t));
  }

  Big_integer_flt &operator=(const Big_integer_flt &other) noexcept {
    Myused = other.Myused;
    memmove(Mydata, other.Mydata, other.Myused * sizeof(uint32_t));
    return *this;
  }

  [[nodiscard]] bool operator<(const Big_integer_flt &Rhs) const noexcept {
    if (Myused != Rhs.Myused) {
      return Myused < Rhs.Myused;
    }

    for (uint32_t Ix = Myused - 1; Ix != static_cast<uint32_t>(-1); --Ix) {
      if (Mydata[Ix] != Rhs.Mydata[Ix]) {
        return Mydata[Ix] < Rhs.Mydata[Ix];
      }
    }

    return false;
  }

  static constexpr uint32_t Maximum_bits = 1074   // 1074 bits required to represent 2^1074
                                           + 2552 // ceil(log2(10^768))
                                           + 54;  // shift space

  static constexpr uint32_t Element_bits = 32;

  static constexpr uint32_t Element_count = (Maximum_bits + Element_bits - 1) / Element_bits;

  uint32_t Myused{0};             // The number of elements currently in use
  uint32_t Mydata[Element_count]; // The number, stored in little-endian form
};

[[nodiscard]] inline Big_integer_flt Make_big_integer_flt_one() noexcept {
  Big_integer_flt Xval{};
  Xval.Mydata[0] = 1;
  Xval.Myused = 1;
  return Xval;
}

[[nodiscard]] inline uint32_t bit_scan_reverse(const Big_integer_flt &Xval) noexcept {
  if (Xval.Myused == 0) {
    return 0;
  }

  const uint32_t Bx = Xval.Myused - 1;

  assert(Xval.Mydata[Bx] != 0); // Big_integer_flt should always be trimmed

  unsigned long Index; // Intentionally uninitialized for better codegen

  _BitScanReverse(&Index, Xval.Mydata[Bx]); // assumes Xval.Mydata[Bx] != 0

  return Index + 1 + Bx * Big_integer_flt::Element_bits;
}

// Shifts the high-precision integer Xval by Nx bits to the left. Returns true if the left shift was successful;
// false if it overflowed. When overflow occurs, the high-precision integer is reset to zero.
[[nodiscard]] inline bool Shift_left(Big_integer_flt &Xval, const uint32_t Nx) noexcept {
  if (Xval.Myused == 0) {
    return true;
  }

  const uint32_t Unit_shift = Nx / Big_integer_flt::Element_bits;
  const uint32_t Bit_shift = Nx % Big_integer_flt::Element_bits;

  if (Xval.Myused + Unit_shift > Big_integer_flt::Element_count) {
    // Unit shift will overflow.
    Xval.Myused = 0;
    return false;
  }

  if (Bit_shift == 0) {
    memmove(Xval.Mydata + Unit_shift, Xval.Mydata, Xval.Myused * sizeof(uint32_t));
    Xval.Myused += Unit_shift;
  } else {
    const bool Bit_shifts_into_next_unit =
        Bit_shift > (Big_integer_flt::Element_bits - bit_scan_reverse(Xval.Mydata[Xval.Myused - 1]));

    const uint32_t New_used = Xval.Myused + Unit_shift + static_cast<uint32_t>(Bit_shifts_into_next_unit);

    if (New_used > Big_integer_flt::Element_count) {
      // Bit shift will overflow.
      Xval.Myused = 0;
      return false;
    }

    const uint32_t Msb_bits = Bit_shift;
    const uint32_t Lsb_bits = Big_integer_flt::Element_bits - Msb_bits;

    const uint32_t Lsb_mask = (1UL << Lsb_bits) - 1UL;
    const uint32_t Msb_mask = ~Lsb_mask;

    // If Unit_shift == 0, this will wraparound, which is okay.
    for (uint32_t Dest_index = New_used - 1; Dest_index != Unit_shift - 1; --Dest_index) {
      // performance note: PSLLDQ and PALIGNR instructions could be more efficient here

      // If Bit_shifts_into_next_unit, the first iteration will trigger the bounds check below, which is okay.
      const uint32_t Upper_source_index = Dest_index - Unit_shift;

      // When Dest_index == Unit_shift, this will wraparound, which is okay (see bounds check below).
      const uint32_t Lower_source_index = Dest_index - Unit_shift - 1;

      const uint32_t Upper_source = Upper_source_index < Xval.Myused ? Xval.Mydata[Upper_source_index] : 0;
      const uint32_t Lower_source = Lower_source_index < Xval.Myused ? Xval.Mydata[Lower_source_index] : 0;

      const uint32_t Shifted_upper_source = (Upper_source & Lsb_mask) << Msb_bits;
      const uint32_t Shifted_lower_source = (Lower_source & Msb_mask) >> Lsb_bits;

      const uint32_t Combined_shifted_source = Shifted_upper_source | Shifted_lower_source;

      Xval.Mydata[Dest_index] = Combined_shifted_source;
    }

    Xval.Myused = New_used;
  }

  memset(Xval.Mydata, 0, Unit_shift * sizeof(uint32_t));

  return true;
}

// Adds a 32-bit value to the high-precision integer Xval. Returns true if the addition was successful;
// false if it overflowed. When overflow occurs, the high-precision integer is reset to zero.
[[nodiscard]] inline bool Add(Big_integer_flt &Xval, const uint32_t value) noexcept {
  if (value == 0) {
    return true;
  }

  uint32_t Carry = value;
  for (uint32_t Ix = 0; Ix != Xval.Myused; ++Ix) {
    const uint64_t result = static_cast<uint64_t>(Xval.Mydata[Ix]) + Carry;
    Xval.Mydata[Ix] = static_cast<uint32_t>(result);
    Carry = static_cast<uint32_t>(result >> 32);
  }

  if (Carry != 0) {
    if (Xval.Myused < Big_integer_flt::Element_count) {
      Xval.Mydata[Xval.Myused] = Carry;
      ++Xval.Myused;
    } else {
      Xval.Myused = 0;
      return false;
    }
  }

  return true;
}

[[nodiscard]] inline uint32_t Add_carry(uint32_t &Ux1, const uint32_t Ux2, const uint32_t U_carry) noexcept {
  const uint64_t Uu = static_cast<uint64_t>(Ux1) + Ux2 + U_carry;
  Ux1 = static_cast<uint32_t>(Uu);
  return static_cast<uint32_t>(Uu >> 32);
}

[[nodiscard]] inline uint32_t Add_multiply_carry(uint32_t &U_add, const uint32_t U_mul_1, const uint32_t U_mul_2,
                                                 const uint32_t U_carry) noexcept {
  const uint64_t Uu_res = static_cast<uint64_t>(U_mul_1) * U_mul_2 + U_add + U_carry;
  U_add = static_cast<uint32_t>(Uu_res);
  return static_cast<uint32_t>(Uu_res >> 32);
}

[[nodiscard]] inline uint32_t Multiply_core(uint32_t *const Multiplicand, const uint32_t Multiplicand_count,
                                            const uint32_t Multiplier) noexcept {
  uint32_t Carry = 0;
  for (uint32_t Ix = 0; Ix != Multiplicand_count; ++Ix) {
    const uint64_t result = static_cast<uint64_t>(Multiplicand[Ix]) * Multiplier + Carry;
    Multiplicand[Ix] = static_cast<uint32_t>(result);
    Carry = static_cast<uint32_t>(result >> 32);
  }

  return Carry;
}

// Multiplies the high-precision Multiplicand by a 32-bit Multiplier. Returns true if the multiplication
// was successful; false if it overflowed. When overflow occurs, the Multiplicand is reset to zero.
[[nodiscard]] inline bool Multiply(Big_integer_flt &Multiplicand, const uint32_t Multiplier) noexcept {
  if (Multiplier == 0) {
    Multiplicand.Myused = 0;
    return true;
  }

  if (Multiplier == 1) {
    return true;
  }

  if (Multiplicand.Myused == 0) {
    return true;
  }

  const uint32_t Carry = Multiply_core(Multiplicand.Mydata, Multiplicand.Myused, Multiplier);
  if (Carry != 0) {
    if (Multiplicand.Myused < Big_integer_flt::Element_count) {
      Multiplicand.Mydata[Multiplicand.Myused] = Carry;
      ++Multiplicand.Myused;
    } else {
      Multiplicand.Myused = 0;
      return false;
    }
  }

  return true;
}

// This high-precision integer multiplication implementation was translated from the implementation of
// System.Numerics.BigIntegerBuilder.Mul in the .NET Framework sources. It multiplies the Multiplicand
// by the Multiplier and returns true if the multiplication was successful; false if it overflowed.
// When overflow occurs, the Multiplicand is reset to zero.
[[nodiscard]] inline bool Multiply(Big_integer_flt &Multiplicand, const Big_integer_flt &Multiplier) noexcept {
  if (Multiplicand.Myused == 0) {
    return true;
  }

  if (Multiplier.Myused == 0) {
    Multiplicand.Myused = 0;
    return true;
  }

  if (Multiplier.Myused == 1) {
    return Multiply(Multiplicand, Multiplier.Mydata[0]); // when overflow occurs, resets to zero
  }

  if (Multiplicand.Myused == 1) {
    const uint32_t Small_multiplier = Multiplicand.Mydata[0];
    Multiplicand = Multiplier;
    return Multiply(Multiplicand, Small_multiplier); // when overflow occurs, resets to zero
  }

  // We prefer more iterations on the inner loop and fewer on the outer:
  const bool Multiplier_is_shorter = Multiplier.Myused < Multiplicand.Myused;
  const uint32_t *const Rgu1 = Multiplier_is_shorter ? Multiplier.Mydata : Multiplicand.Mydata;
  const uint32_t *const Rgu2 = Multiplier_is_shorter ? Multiplicand.Mydata : Multiplier.Mydata;

  const uint32_t Cu1 = Multiplier_is_shorter ? Multiplier.Myused : Multiplicand.Myused;
  const uint32_t Cu2 = Multiplier_is_shorter ? Multiplicand.Myused : Multiplier.Myused;

  Big_integer_flt result{};
  for (uint32_t Iu1 = 0; Iu1 != Cu1; ++Iu1) {
    const uint32_t U_cur = Rgu1[Iu1];
    if (U_cur == 0) {
      if (Iu1 == result.Myused) {
        result.Mydata[Iu1] = 0;
        result.Myused = Iu1 + 1;
      }

      continue;
    }

    uint32_t U_carry = 0;
    uint32_t Iu_res = Iu1;
    for (uint32_t Iu2 = 0; Iu2 != Cu2 && Iu_res != Big_integer_flt::Element_count; ++Iu2, ++Iu_res) {
      if (Iu_res == result.Myused) {
        result.Mydata[Iu_res] = 0;
        result.Myused = Iu_res + 1;
      }

      U_carry = Add_multiply_carry(result.Mydata[Iu_res], U_cur, Rgu2[Iu2], U_carry);
    }

    while (U_carry != 0 && Iu_res != Big_integer_flt::Element_count) {
      if (Iu_res == result.Myused) {
        result.Mydata[Iu_res] = 0;
        result.Myused = Iu_res + 1;
      }

      U_carry = Add_carry(result.Mydata[Iu_res++], 0, U_carry);
    }

    if (Iu_res == Big_integer_flt::Element_count) {
      Multiplicand.Myused = 0;
      return false;
    }
  }

  // Store the result in the Multiplicand and compute the actual number of elements used:
  Multiplicand = result;
  return true;
}

// Multiplies the high-precision integer Xval by 10^_Power. Returns true if the multiplication was successful;
// false if it overflowed. When overflow occurs, the high-precision integer is reset to zero.
[[nodiscard]] inline bool Multiply_by_power_of_ten(Big_integer_flt &Xval, const uint32_t Power) noexcept {
  // To improve performance, we use a table of precomputed powers of ten, from 10^10 through 10^380, in increments
  // of ten. In its unpacked form, as an array of Big_integer_flt objects, this table consists mostly of zero
  // elements. Thus, we store the table in a packed form, trimming leading and trailing zero elements. We provide an
  // index that is used to unpack powers from the table, using the function that appears after this function in this
  // file.

  // The minimum value representable with double-precision is 5E-324.
  // With this table we can thus compute most multiplications with a single multiply.

  static constexpr uint32_t Large_power_data[] = {
      0x540be400, 0x00000002, 0x63100000, 0x6bc75e2d, 0x00000005, 0x40000000, 0x4674edea, 0x9f2c9cd0, 0x0000000c,
      0xb9f56100, 0x5ca4bfab, 0x6329f1c3, 0x0000001d, 0xb5640000, 0xc40534fd, 0x926687d2, 0x6c3b15f9, 0x00000044,
      0x10000000, 0x946590d9, 0xd762422c, 0x9a224501, 0x4f272617, 0x0000009f, 0x07950240, 0x245689c1, 0xc5faa71c,
      0x73c86d67, 0xebad6ddc, 0x00000172, 0xcec10000, 0x63a22764, 0xefa418ca, 0xcdd17b25, 0x6bdfef70, 0x9dea3e1f,
      0x0000035f, 0xe4000000, 0xcdc3fe6e, 0x66bc0c6a, 0x2e391f32, 0x5a450203, 0x71d2f825, 0xc3c24a56, 0x000007da,
      0xa82e8f10, 0xaab24308, 0x8e211a7c, 0xf38ace40, 0x84c4ce0b, 0x7ceb0b27, 0xad2594c3, 0x00001249, 0xdd1a4000,
      0xcc9f54da, 0xdc5961bf, 0xc75cabab, 0xf505440c, 0xd1bc1667, 0xfbb7af52, 0x608f8d29, 0x00002a94, 0x21000000,
      0x17bb8a0c, 0x56af8ea4, 0x06479fa9, 0x5d4bb236, 0x80dc5fe0, 0xf0feaa0a, 0xa88ed940, 0x6b1a80d0, 0x00006323,
      0x324c3864, 0x8357c796, 0xe44a42d5, 0xd9a92261, 0xbd3c103d, 0x91e5f372, 0xc0591574, 0xec1da60d, 0x102ad96c,
      0x0000e6d3, 0x1e851000, 0x6e4f615b, 0x187b2a69, 0x0450e21c, 0x2fdd342b, 0x635027ee, 0xa6c97199, 0x8e4ae916,
      0x17082e28, 0x1a496e6f, 0x0002196e, 0x32400000, 0x04ad4026, 0xf91e7250, 0x2994d1d5, 0x665bcdbb, 0xa23b2e96,
      0x65fa7ddb, 0x77de53ac, 0xb020a29b, 0xc6bff953, 0x4b9425ab, 0x0004e34d, 0xfbc32d81, 0x5222d0f4, 0xb70f2850,
      0x5713f2f3, 0xdc421413, 0xd6395d7d, 0xf8591999, 0x0092381c, 0x86b314d6, 0x7aa577b9, 0x12b7fe61, 0x000b616a,
      0x1d11e400, 0x56c3678d, 0x3a941f20, 0x9b09368b, 0xbd706908, 0x207665be, 0x9b26c4eb, 0x1567e89d, 0x9d15096e,
      0x7132f22b, 0xbe485113, 0x45e5a2ce, 0x001a7f52, 0xbb100000, 0x02f79478, 0x8c1b74c0, 0xb0f05d00, 0xa9dbc675,
      0xe2d9b914, 0x650f72df, 0x77284b4c, 0x6df6e016, 0x514391c2, 0x2795c9cf, 0xd6e2ab55, 0x9ca8e627, 0x003db1a6,
      0x40000000, 0xf4ecd04a, 0x7f2388f0, 0x580a6dc5, 0x43bf046f, 0xf82d5dc3, 0xee110848, 0xfaa0591c, 0xcdf4f028,
      0x192ea53f, 0xbcd671a0, 0x7d694487, 0x10f96e01, 0x791a569d, 0x008fa475, 0xb9b2e100, 0x8288753c, 0xcd3f1693,
      0x89b43a6b, 0x089e87de, 0x684d4546, 0xfddba60c, 0xdf249391, 0x3068ec13, 0x99b44427, 0xb68141ee, 0x5802cac3,
      0xd96851f1, 0x7d7625a2, 0x014e718d, 0xfb640000, 0xf25a83e6, 0x9457ad0f, 0x0080b511, 0x2029b566, 0xd7c5d2cf,
      0xa53f6d7d, 0xcdb74d1c, 0xda9d70de, 0xb716413d, 0x71d0ca4e, 0xd7e41398, 0x4f403a90, 0xf9ab3fe2, 0x264d776f,
      0x030aafe6, 0x10000000, 0x09ab5531, 0xa60c58d2, 0x566126cb, 0x6a1c8387, 0x7587f4c1, 0x2c44e876, 0x41a047cf,
      0xc908059e, 0xa0ba063e, 0xe7cfc8e8, 0xe1fac055, 0xef0144b2, 0x24207eb0, 0xd1722573, 0xe4b8f981, 0x071505ae,
      0x7a3b6240, 0xcea45d4f, 0x4fe24133, 0x210f6d6d, 0xe55633f2, 0x25c11356, 0x28ebd797, 0xd396eb84, 0x1e493b77,
      0x471f2dae, 0x96ad3820, 0x8afaced1, 0x4edecddb, 0x5568c086, 0xb2695da1, 0x24123c89, 0x107d4571, 0x1c410000,
      0x6e174a27, 0xec62ae57, 0xef2289aa, 0xb6a2fbdd, 0x17e1efe4, 0x3366bdf2, 0x37b48880, 0xbfb82c3e, 0x19acde91,
      0xd4f46408, 0x35ff6a4e, 0x67566a0e, 0x40dbb914, 0x782a3bca, 0x6b329b68, 0xf5afc5d9, 0x266469bc, 0xe4000000,
      0xfb805ff4, 0xed55d1af, 0x9b4a20a8, 0xab9757f8, 0x01aefe0a, 0x4a2ca67b, 0x1ebf9569, 0xc7c41c29, 0xd8d5d2aa,
      0xd136c776, 0x93da550c, 0x9ac79d90, 0x254bcba8, 0x0df07618, 0xf7a88809, 0x3a1f1074, 0xe54811fc, 0x59638ead,
      0x97cbe710, 0x26d769e8, 0xb4e4723e, 0x5b90aa86, 0x9c333922, 0x4b7a0775, 0x2d47e991, 0x9a6ef977, 0x160b40e7,
      0x0c92f8c4, 0xf25ff010, 0x25c36c11, 0xc9f98b42, 0x730b919d, 0x05ff7caf, 0xb0432d85, 0x2d2b7569, 0xa657842c,
      0xd01fef10, 0xc77a4000, 0xe8b862e5, 0x10d8886a, 0xc8cd98e5, 0x108955c5, 0xd059b655, 0x58fbbed4, 0x03b88231,
      0x034c4519, 0x194dc939, 0x1fc500ac, 0x794cc0e2, 0x3bc980a1, 0xe9b12dd1, 0x5e6d22f8, 0x7b38899a, 0xce7919d8,
      0x78c67672, 0x79e5b99f, 0xe494034e, 0x00000001, 0xa1000000, 0x6c5cd4e9, 0x9be47d6f, 0xf93bd9e7, 0x77626fa1,
      0xc68b3451, 0xde2b59e8, 0xcf3cde58, 0x2246ff58, 0xa8577c15, 0x26e77559, 0x17776753, 0xebe6b763, 0xe3fd0a5f,
      0x33e83969, 0xa805a035, 0xf631b987, 0x211f0f43, 0xd85a43db, 0xab1bf596, 0x683f19a2, 0x00000004, 0xbe7dfe64,
      0x4bc9042f, 0xe1f5edb0, 0x8fa14eda, 0xe409db73, 0x674fee9c, 0xa9159f0d, 0xf6b5b5d6, 0x7338960e, 0xeb49c291,
      0x5f2b97cc, 0x0f383f95, 0x2091b3f6, 0xd1783714, 0xc1d142df, 0x153e22de, 0x8aafdf57, 0x77f5e55f, 0xa3e7ca8b,
      0x032f525b, 0x42e74f3d, 0x0000000a, 0xf4dd1000, 0x5d450952, 0xaeb442e1, 0xa3b3342e, 0x3fcda36f, 0xb4287a6e,
      0x4bc177f7, 0x67d2c8d0, 0xaea8f8e0, 0xadc93b67, 0x6cc856b3, 0x959d9d0b, 0x5b48c100, 0x4abe8a3d, 0x52d936f4,
      0x71dbe84d, 0xf91c21c5, 0x4a458109, 0xd7aad86a, 0x08e14c7c, 0x759ba59c, 0xe43c8800, 0x00000017, 0x92400000,
      0x04f110d4, 0x186472be, 0x8736c10c, 0x1478abfb, 0xfc51af29, 0x25eb9739, 0x4c2b3015, 0xa1030e0b, 0x28fe3c3b,
      0x7788fcba, 0xb89e4358, 0x733de4a4, 0x7c46f2c2, 0x8f746298, 0xdb19210f, 0x2ea3b6ae, 0xaa5014b2, 0xea39ab8d,
      0x97963442, 0x01dfdfa9, 0xd2f3d3fe, 0xa0790280, 0x00000037, 0x509c9b01, 0xc7dcadf1, 0x383dad2c, 0x73c64d37,
      0xea6d67d0, 0x519ba806, 0xc403f2f8, 0xa052e1a2, 0xd710233a, 0x448573a9, 0xcf12d9ba, 0x70871803, 0x52dc3a9b,
      0xe5b252e8, 0x0717fb4e, 0xbe4da62f, 0x0aabd7e1, 0x8c62ed4f, 0xceb9ec7b, 0xd4664021, 0xa1158300, 0xcce375e6,
      0x842f29f2, 0x00000081, 0x7717e400, 0xd3f5fb64, 0xa0763d71, 0x7d142fe9, 0x33f44c66, 0xf3b8f12e, 0x130f0d8e,
      0x734c9469, 0x60260fa8, 0x3c011340, 0xcc71880a, 0x37a52d21, 0x8adac9ef, 0x42bb31b4, 0xd6f94c41, 0xc88b056c,
      0xe20501b8, 0x5297ed7c, 0x62c361c4, 0x87dad8aa, 0xb833eade, 0x94f06861, 0x13cc9abd, 0x8dc1d56a, 0x0000012d,
      0x13100000, 0xc67a36e8, 0xf416299e, 0xf3493f0a, 0x77a5a6cf, 0xa4be23a3, 0xcca25b82, 0x3510722f, 0xbe9d447f,
      0xa8c213b8, 0xc94c324e, 0xbc9e33ad, 0x76acfeba, 0x2e4c2132, 0x3e13cd32, 0x70fe91b4, 0xbb5cd936, 0x42149785,
      0x46cc1afd, 0xe638ddf8, 0x690787d2, 0x1a02d117, 0x3eb5f1fe, 0xc3b9abae, 0x1c08ee6f, 0x000002be, 0x40000000,
      0x8140c2aa, 0x2cf877d9, 0x71e1d73d, 0xd5e72f98, 0x72516309, 0xafa819dd, 0xd62a5a46, 0x2a02dcce, 0xce46ddfe,
      0x2713248d, 0xb723d2ad, 0xc404bb19, 0xb706cc2b, 0x47b1ebca, 0x9d094bdc, 0xc5dc02ca, 0x31e6518e, 0x8ec35680,
      0x342f58a8, 0x8b041e42, 0xfebfe514, 0x05fffc13, 0x6763790f, 0x66d536fd, 0xb9e15076, 0x00000662, 0x67b06100,
      0xd2010a1a, 0xd005e1c0, 0xdb12733b, 0xa39f2e3f, 0x61b29de2, 0x2a63dce2, 0x942604bc, 0x6170d59b, 0xc2e32596,
      0x140b75b9, 0x1f1d2c21, 0xb8136a60, 0x89d23ba2, 0x60f17d73, 0xc6cad7df, 0x0669df2b, 0x24b88737, 0x669306ed,
      0x19496eeb, 0x938ddb6f, 0x5e748275, 0xc56e9a36, 0x3690b731, 0xc82842c5, 0x24ae798e, 0x00000ede, 0x41640000,
      0xd5889ac1, 0xd9432c99, 0xa280e71a, 0x6bf63d2e, 0x8249793d, 0x79e7a943, 0x22fde64a, 0xe0d6709a, 0x05cacfef,
      0xbd8da4d7, 0xe364006c, 0xa54edcb3, 0xa1a8086e, 0x748f459e, 0xfc8e54c8, 0xcc74c657, 0x42b8c3d4, 0x57d9636e,
      0x35b55bcc, 0x6c13fee9, 0x1ac45161, 0xb595badb, 0xa1f14e9d, 0xdcf9e750, 0x07637f71, 0xde2f9f2b, 0x0000229d,
      0x10000000, 0x3c5ebd89, 0xe3773756, 0x3dcba338, 0x81d29e4f, 0xa4f79e2c, 0xc3f9c774, 0x6a1ce797, 0xac5fe438,
      0x07f38b9c, 0xd588ecfa, 0x3e5ac1ac, 0x85afccce, 0x9d1f3f70, 0xe82d6dd3, 0x177d180c, 0x5e69946f, 0x648e2ce1,
      0x95a13948, 0x340fe011, 0xb4173c58, 0x2748f694, 0x7c2657bd, 0x758bda2e, 0x3b8090a0, 0x2ddbb613, 0x6dcf4890,
      0x24e4047e, 0x00005099};

  struct Unpack_index {
    uint16_t Offset; // The offset of this power's initial element in the array
    uint8_t Zeroes;  // The number of omitted leading zero elements
    uint8_t Size;    // The number of elements present for this power
  };

  static constexpr Unpack_index Large_power_indices[] = {
      {0, 0, 2},     {2, 0, 3},     {5, 0, 4},    {9, 1, 4},     {13, 1, 5},    {18, 1, 6},    {24, 2, 6},
      {30, 2, 7},    {37, 2, 8},    {45, 3, 8},   {53, 3, 9},    {62, 3, 10},   {72, 4, 10},   {82, 4, 11},
      {93, 4, 12},   {105, 5, 12},  {117, 5, 13}, {130, 5, 14},  {144, 5, 15},  {159, 6, 15},  {174, 6, 16},
      {190, 6, 17},  {207, 7, 17},  {224, 7, 18}, {242, 7, 19},  {261, 8, 19},  {280, 8, 21},  {301, 8, 22},
      {323, 9, 22},  {345, 9, 23},  {368, 9, 24}, {392, 10, 24}, {416, 10, 25}, {441, 10, 26}, {467, 10, 27},
      {494, 11, 27}, {521, 11, 28}, {549, 11, 29}};

  for (uint32_t Large_power = Power / 10; Large_power != 0;) {
    const uint32_t Current_power = (std::min)(Large_power, static_cast<uint32_t>(std::size(Large_power_indices)));

    const Unpack_index &Index = Large_power_indices[Current_power - 1];
    Big_integer_flt Multiplier{};
    Multiplier.Myused = static_cast<uint32_t>(Index.Size + Index.Zeroes);

    const uint32_t *const Source = Large_power_data + Index.Offset;

    memset(Multiplier.Mydata, 0, Index.Zeroes * sizeof(uint32_t));
    memcpy(Multiplier.Mydata + Index.Zeroes, Source, Index.Size * sizeof(uint32_t));

    if (!Multiply(Xval, Multiplier)) { // when overflow occurs, resets to zero
      return false;
    }

    Large_power -= Current_power;
  }

  static constexpr uint32_t Small_powers_of_ten[9] = {10,        100,        1'000,       10'000,       100'000,
                                                      1'000'000, 10'000'000, 100'000'000, 1'000'000'000};

  const uint32_t Small_power = Power % 10;

  if (Small_power == 0) {
    return true;
  }

  return Multiply(Xval, Small_powers_of_ten[Small_power - 1]); // when overflow occurs, resets to zero
}

// Computes the number of zeroes higher than the most significant set bit in _Ux
[[nodiscard]] inline uint32_t Count_sequential_high_zeroes(const uint32_t Ux) noexcept {
  unsigned long Index; // Intentionally uninitialized for better codegen
  return _BitScanReverse(&Index, Ux) != 0u ? 31 - Index : 32;
}

// This high-precision integer division implementation was translated from the implementation of
// System.Numerics.BigIntegerBuilder.ModDivCore in the .NET Framework sources.
// It computes both quotient and remainder: the remainder is stored in the Numerator argument,
// and the least significant 64 bits of the quotient are returned from the function.
[[nodiscard]] inline uint64_t Divide(Big_integer_flt &Numerator, const Big_integer_flt &Denominator) noexcept {
  // If the Numerator is zero, then both the quotient and remainder are zero:
  if (Numerator.Myused == 0) {
    return 0;
  }

  // If the Denominator is zero, then uh oh. We can't divide by zero:
  assert(Denominator.Myused != 0); // Division by zero

  uint32_t Max_numerator_element_index = Numerator.Myused - 1;
  const uint32_t Max_denominator_element_index = Denominator.Myused - 1;

  // The Numerator and Denominator are both nonzero.
  // If the Denominator is only one element wide, we can take the fast route:
  if (Max_denominator_element_index == 0) {
    const uint32_t Small_denominator = Denominator.Mydata[0];

    if (Max_numerator_element_index == 0) {
      const uint32_t Small_numerator = Numerator.Mydata[0];

      if (Small_denominator == 1) {
        Numerator.Myused = 0;
        return Small_numerator;
      }

      Numerator.Mydata[0] = Small_numerator % Small_denominator;
      Numerator.Myused = Numerator.Mydata[0] > 0 ? 1U : 0U;
      return Small_numerator / Small_denominator;
    }

    if (Small_denominator == 1) {
      uint64_t Quotient = Numerator.Mydata[1];
      Quotient <<= 32;
      Quotient |= Numerator.Mydata[0];
      Numerator.Myused = 0;
      return Quotient;
    }

    // We count down in the next loop, so the last assignment to Quotient will be the correct one.
    uint64_t Quotient = 0;

    uint64_t Uu = 0;
    for (uint32_t Iv = Max_numerator_element_index; Iv != static_cast<uint32_t>(-1); --Iv) {
      Uu = (Uu << 32) | Numerator.Mydata[Iv];
      Quotient = (Quotient << 32) + static_cast<uint32_t>(Uu / Small_denominator);
      Uu %= Small_denominator;
    }

    Numerator.Mydata[1] = static_cast<uint32_t>(Uu >> 32);
    Numerator.Mydata[0] = static_cast<uint32_t>(Uu);

    if (Numerator.Mydata[1] > 0) {
      Numerator.Myused = 2U;
    } else if (Numerator.Mydata[0] > 0) {
      Numerator.Myused = 1U;
    } else {
      Numerator.Myused = 0U;
    }

    return Quotient;
  }

  if (Max_denominator_element_index > Max_numerator_element_index) {
    return 0;
  }

  const uint32_t Cu_den = Max_denominator_element_index + 1;
  const auto Cu_diff = static_cast<int32_t>(Max_numerator_element_index - Max_denominator_element_index);

  // Determine whether the result will have Cu_diff or Cu_diff + 1 digits:
  int32_t Cu_quo = Cu_diff;
  for (auto Iu = static_cast<int32_t>(Max_numerator_element_index);; --Iu) {
    if (Iu < Cu_diff) {
      ++Cu_quo;
      break;
    }

    if (Denominator.Mydata[Iu - Cu_diff] != Numerator.Mydata[Iu]) {
      if (Denominator.Mydata[Iu - Cu_diff] < Numerator.Mydata[Iu]) {
        ++Cu_quo;
      }

      break;
    }
  }

  if (Cu_quo == 0) {
    return 0;
  }

  // Get the uint to use for the trial divisions. We normalize so the high bit is set:
  uint32_t U_den = Denominator.Mydata[Cu_den - 1];
  uint32_t U_den_next = Denominator.Mydata[Cu_den - 2];

  const uint32_t Cbit_shift_left = Count_sequential_high_zeroes(U_den);
  const uint32_t Cbit_shift_right = 32 - Cbit_shift_left;
  if (Cbit_shift_left > 0) {
    U_den = (U_den << Cbit_shift_left) | (U_den_next >> Cbit_shift_right);
    U_den_next <<= Cbit_shift_left;

    if (Cu_den > 2) {
      U_den_next |= Denominator.Mydata[Cu_den - 3] >> Cbit_shift_right;
    }
  }

  uint64_t Quotient = 0;
  for (int32_t Iu = Cu_quo; --Iu >= 0;) {
    // Get the high (normalized) bits of the Numerator:
    const uint32_t U_num_hi = (Iu + Cu_den <= Max_numerator_element_index) ? Numerator.Mydata[Iu + Cu_den] : 0;

    uint64_t Uu_num =
        (static_cast<uint64_t>(U_num_hi) << 32) | static_cast<uint64_t>(Numerator.Mydata[Iu + Cu_den - 1]);

    uint32_t U_num_next = Numerator.Mydata[Iu + Cu_den - 2];
    if (Cbit_shift_left > 0) {
      Uu_num = (Uu_num << Cbit_shift_left) | (U_num_next >> Cbit_shift_right);
      U_num_next <<= Cbit_shift_left;

      if (Iu + Cu_den >= 3) {
        U_num_next |= Numerator.Mydata[Iu + Cu_den - 3] >> Cbit_shift_right;
      }
    }

    // Divide to get the quotient digit:
    uint64_t Uu_quo = Uu_num / U_den;
    uint64_t Uu_rem = static_cast<uint32_t>(Uu_num % U_den);

    if (Uu_quo > UINT32_MAX) {
      Uu_rem += U_den * (Uu_quo - UINT32_MAX);
      Uu_quo = UINT32_MAX;
    }

    while (Uu_rem <= UINT32_MAX && Uu_quo * U_den_next > ((Uu_rem << 32) | U_num_next)) {
      --Uu_quo;
      Uu_rem += U_den;
    }

    // Multiply and subtract. Note that Uu_quo may be one too large.
    // If we have a borrow at the end, we'll add the Denominator back on and decrement Uu_quo.
    if (Uu_quo > 0) {
      uint64_t Uu_borrow = 0;

      for (uint32_t Iu2 = 0; Iu2 < Cu_den; ++Iu2) {
        Uu_borrow += Uu_quo * Denominator.Mydata[Iu2];

        const auto U_sub = static_cast<uint32_t>(Uu_borrow);
        Uu_borrow >>= 32;
        if (Numerator.Mydata[Iu + Iu2] < U_sub) {
          ++Uu_borrow;
        }

        Numerator.Mydata[Iu + Iu2] -= U_sub;
      }

      if (U_num_hi < Uu_borrow) {
        // Add, tracking carry:
        uint32_t U_carry = 0;
        for (uint32_t Iu2 = 0; Iu2 < Cu_den; ++Iu2) {
          const uint64_t Sum = static_cast<uint64_t>(Numerator.Mydata[Iu + Iu2]) +
                               static_cast<uint64_t>(Denominator.Mydata[Iu2]) + U_carry;

          Numerator.Mydata[Iu + Iu2] = static_cast<uint32_t>(Sum);
          U_carry = static_cast<uint32_t>(Sum >> 32);
        }

        --Uu_quo;
      }

      Max_numerator_element_index = Iu + Cu_den - 1;
    }

    Quotient = (Quotient << 32) + static_cast<uint32_t>(Uu_quo);
  }

  // Trim the remainder:
  for (uint32_t Ix = Max_numerator_element_index + 1; Ix < Numerator.Myused; ++Ix) {
    Numerator.Mydata[Ix] = 0;
  }

  uint32_t Used = Max_numerator_element_index + 1;

  while (Used != 0 && Numerator.Mydata[Used - 1] == 0) {
    --Used;
  }

  Numerator.Myused = Used;

  return Quotient;
}

// ^^^^^^^^^^ DERIVED FROM corecrt_internal_big_integer.h ^^^^^^^^^^

// vvvvvvvvvv DERIVED FROM corecrt_internal_strtox.h vvvvvvvvvv

// This type is used to hold a partially-parsed string representation of a floating-point number.
// The number is stored in the following form:

// [sign] 0._Mymantissa * B^Myexponent

// The _Mymantissa buffer stores the mantissa digits in big-endian, binary-coded decimal form. The Mymantissa_count
// stores the number of digits present in the _Mymantissa buffer. The base B is not stored; it must be tracked
// separately. Note that the base of the mantissa digits may not be the same as B (e.g., for hexadecimal
// floating-point, the mantissa digits are in base 16 but the exponent is a base 2 exponent).

// We consider up to 768 decimal digits during conversion. In most cases, we require nowhere near this many digits
// of precision to compute the correctly rounded binary floating-point value for the input string. The worst case is
// (2 - 3 * 2^-53) * 2^-1022, which has an exact decimal representation of 768 decimal digits after trimming zeroes.
// This value is exactly between 0x1.ffffffffffffep-1022 and 0x1.fffffffffffffp-1022. For round-to-nearest,
// ties-to-even behavior, we also need to consider whether there are any nonzero trailing decimal digits.

// NOTE: The mantissa buffer count here must be kept in sync with the precision of the Big_integer_flt type.
struct Floating_point_string {
  bool Myis_negative;
  int32_t Myexponent;
  uint32_t Mymantissa_count;
  uint8_t Mymantissa[768];
};

// Stores a positive or negative zero into the result object
template <class FloatingType> void Assemble_floating_point_zero(const bool Is_negative, FloatingType &result) noexcept {
  using Floating_traits = Floating_type_traits<FloatingType>;
  using Uint_type = typename Floating_traits::Uint_type;

  Uint_type Sign_component = Is_negative;
  Sign_component <<= Floating_traits::Sign_shift;

  result = std::bit_cast<FloatingType>(Sign_component);
}

// Stores a positive or negative infinity into the result object
template <class FloatingType>
void Assemble_floating_point_infinity(const bool Is_negative, FloatingType &result) noexcept {
  using Floating_traits = Floating_type_traits<FloatingType>;
  using Uint_type = typename Floating_traits::Uint_type;

  Uint_type Sign_component = Is_negative;
  Sign_component <<= Floating_traits::Sign_shift;

  const Uint_type Exponent_component = Floating_traits::Shifted_exponent_mask;

  result = std::bit_cast<FloatingType>(Sign_component | Exponent_component);
}

// Determines whether a mantissa should be rounded up according to round_to_nearest given [1] the value of the least
// significant bit of the mantissa, [2] the value of the next bit after the least significant bit (the "round" bit)
// and [3] whether any trailing bits after the round bit are set.

// The mantissa is treated as an unsigned integer magnitude.

// For this function, "round up" is defined as "increase the magnitude" of the mantissa. (Note that this means that
// if we need to round a negative value to the next largest representable value, we return false, because the next
// largest representable value has a smaller magnitude.)
[[nodiscard]] inline bool Should_round_up(const bool Lsb_bit, const bool Round_bit, const bool Has_tail_bits) noexcept {
  // If there are no insignificant set bits, the value is exactly-representable and should not be rounded.
  // We could detect this with:
  // const bool _Is_exactly_representable = !Round_bit && !Has_tail_bits;
  // if (_Is_exactly_representable) { return false; }
  // However, this is unnecessary given the logic below.

  // If there are insignificant set bits, we need to round according to round_to_nearest.
  // We need to handle two cases: we round up if either [1] the value is slightly greater
  // than the midpoint between two exactly-representable values or [2] the value is exactly the midpoint
  // between two exactly-representable values and the greater of the two is even (this is "round-to-even").
  return Round_bit && (Has_tail_bits || Lsb_bit);
}

// Computes value / 2^_Shift, then rounds the result according to round_to_nearest.
// By the time we call this function, we will already have discarded most digits.
// The caller must pass true for Has_zero_tail if all discarded bits were zeroes.
[[nodiscard]] inline uint64_t Right_shift_with_rounding(const uint64_t value, const uint32_t Shift,
                                                        const bool Has_zero_tail) noexcept {
  constexpr uint32_t Total_number_of_bits = 64;
  if (Shift >= Total_number_of_bits) {
    if (Shift == Total_number_of_bits) {
      constexpr uint64_t Extra_bits_mask = (1ULL << (Total_number_of_bits - 1)) - 1;
      constexpr uint64_t Round_bit_mask = (1ULL << (Total_number_of_bits - 1));

      const bool Round_bit = (value & Round_bit_mask) != 0;
      const bool Tail_bits = !Has_zero_tail || (value & Extra_bits_mask) != 0;

      // We round up the answer to 1 if the answer is greater than 0.5. Otherwise, we round down the answer to 0
      // if either [1] the answer is less than 0.5 or [2] the answer is exactly 0.5.
      return static_cast<uint64_t>(Round_bit && Tail_bits);
    } // If we'd need to shift 65 or more bits, the answer is less than 0.5 and is always rounded to zero:
    return 0;
  }

  // Reference implementation with suboptimal codegen:
  // const uint64_t _Extra_bits_mask = (1ULL << (_Shift - 1)) - 1;
  // const uint64_t Round_bit_mask  = (1ULL << (_Shift - 1));
  // const uint64_t Lsb_bit_mask    = 1ULL << _Shift;

  // const bool Lsb_bit   = (value & Lsb_bit_mask) != 0;
  // const bool Round_bit = (value & Round_bit_mask) != 0;
  // const bool _Tail_bits = !Has_zero_tail || (value & _Extra_bits_mask) != 0;

  // return (value >> _Shift) + Should_round_up(Lsb_bit, Round_bit, _Tail_bits);

  // Example for optimized implementation: Let _Shift be 8.
  // Bit index: ...[8]76543210
  //    value: ...[L]RTTTTTTT
  // By focusing on the bit at index _Shift, we can avoid unnecessary branching and shifting.

  // Bit index: ...[8]76543210
  //  Lsb_bit: ...[L]RTTTTTTT
  const uint64_t Lsb_bit = value;

  //  Bit index: ...9[8]76543210
  // Round_bit: ...L[R]TTTTTTT0
  const uint64_t Round_bit = value << 1;

  // We can detect (without branching) whether any of the trailing bits are set.
  // Due to Should_round below, this computation will be used if and only if R is 1, so we can assume that here.
  //      Bit index: ...9[8]76543210
  //     Round_bit: ...L[1]TTTTTTT0
  // Has_tail_bits: ....[H]........

  // If all of the trailing bits T are 0, and Has_zero_tail is true,
  // then `Round_bit - static_cast<uint64_t>(Has_zero_tail)` will produce 0 for H (due to R being 1).
  // If any of the trailing bits T are 1, or Has_zero_tail is false,
  // then `Round_bit - static_cast<uint64_t>(Has_zero_tail)` will produce 1 for H (due to R being 1).
  const uint64_t Has_tail_bits = Round_bit - static_cast<uint64_t>(Has_zero_tail);

  // Finally, we can use Should_round_up() logic with bitwise-AND and bitwise-OR,
  // selecting just the bit at index _Shift.
  const uint64_t Should_round = ((Round_bit & (Has_tail_bits | Lsb_bit)) >> Shift) & uint64_t{1};

  // This rounding technique is dedicated to the memory of Peppermint. =^..^=
  return (value >> Shift) + Should_round;
}

// Converts the floating-point value [sign] (mantissa / 2^(precision-1)) * 2^exponent into the correct form for
// FloatingType and stores the result into the result object.
// The caller must ensure that the mantissa and exponent are correctly computed such that either:
// [1] min_exponent <= exponent <= max_exponent && 2^(precision-1) <= mantissa <= 2^precision, or
// [2] exponent == min_exponent && 0 < mantissa <= 2^(precision-1).
// (The caller should round the mantissa before calling this function. The caller doesn't need to renormalize the
// mantissa when the mantissa carries over to a higher bit after rounding up.)

// This function correctly handles overflow and stores an infinity in the result object.
// (The result overflows if and only if exponent == max_exponent && mantissa == 2^precision)
template <class FloatingType>
void Assemble_floating_point_value_no_shift(const bool Is_negative, const int32_t Exponent,
                                            const typename Floating_type_traits<FloatingType>::Uint_type Mantissa,
                                            FloatingType &result) noexcept {
  // The following code assembles floating-point values based on an alternative interpretation of the IEEE 754 binary
  // floating-point format. It is valid for all of the following cases:
  // [1] normal value,
  // [2] normal value, needs renormalization and exponent increment after rounding up the mantissa,
  // [3] normal value, overflows after rounding up the mantissa,
  // [4] subnormal value,
  // [5] subnormal value, becomes a normal value after rounding up the mantissa.

  // Examples for float:
  // | Case |     Input     | Exponent |  Exponent  |  Exponent  |  Rounded  | Result Bits |     Result      |
  // |      |               |          | + Bias - 1 |  Component |  Mantissa |             |                 |
  // | ---- | ------------- | -------- | ---------- | ---------- | --------- | ----------- | --------------- |
  // | [1]  | 1.000000p+0   |     +0   |    126     | 0x3f000000 |  0x800000 | 0x3f800000  | 0x1.000000p+0   |
  // | [2]  | 1.ffffffp+0   |     +0   |    126     | 0x3f000000 | 0x1000000 | 0x40000000  | 0x1.000000p+1   |
  // | [3]  | 1.ffffffp+127 |   +127   |    253     | 0x7e800000 | 0x1000000 | 0x7f800000  |     inf         |
  // | [4]  | 0.fffffep-126 |   -126   |      0     | 0x00000000 |  0x7fffff | 0x007fffff  | 0x0.fffffep-126 |
  // | [5]  | 0.ffffffp-126 |   -126   |      0     | 0x00000000 |  0x800000 | 0x00800000  | 0x1.000000p-126 |
  using Floating_traits = Floating_type_traits<FloatingType>;
  using Uint_type = typename Floating_traits::Uint_type;

  Uint_type Sign_component = Is_negative;
  Sign_component <<= Floating_traits::Sign_shift;

  auto Exponent_component = static_cast<uint32_t>(Exponent + (Floating_traits::Exponent_bias - 1));
  Exponent_component <<= Floating_traits::Exponent_shift;

  result = std::bit_cast<FloatingType>(Sign_component | (Exponent_component + Mantissa));
}

// Converts the floating-point value [sign] (mantissa / 2^(precision-1)) * 2^exponent into the correct form for
// FloatingType and stores the result into the result object. The caller must ensure that the mantissa and exponent
// are correctly computed such that either [1] the most significant bit of the mantissa is in the correct position for
// the FloatingType, or [2] the exponent has been correctly adjusted to account for the shift of the mantissa that will
// be required.

// This function correctly handles range errors and stores a zero or infinity in the result object
// on underflow and overflow errors, respectively. This function correctly forms denormal numbers when required.

// If the provided mantissa has more bits of precision than can be stored in the result object, the mantissa is
// rounded to the available precision. Thus, if possible, the caller should provide a mantissa with at least one
// more bit of precision than is required, to ensure that the mantissa is correctly rounded.
// (The caller should not round the mantissa before calling this function.)
template <class FloatingType>
[[nodiscard]] errc Assemble_floating_point_value(const uint64_t Initial_mantissa, const int32_t Initial_exponent,
                                                 const bool Is_negative, const bool Has_zero_tail,
                                                 FloatingType &result) noexcept {
  using Traits = Floating_type_traits<FloatingType>;

  // Assume that the number is representable as a normal value.
  // Compute the number of bits by which we must adjust the mantissa to shift it into the correct position,
  // and compute the resulting base two exponent for the normalized mantissa:
  const uint32_t Initial_mantissa_bits = bit_scan_reverse(Initial_mantissa);
  const auto Normal_mantissa_shift = static_cast<int32_t>(Traits::Mantissa_bits - Initial_mantissa_bits);
  const int32_t Normal_exponent = Initial_exponent - Normal_mantissa_shift;

  if (Normal_exponent > Traits::Maximum_binary_exponent) {
    // The exponent is too large to be represented by the floating-point type; report the overflow condition:
    Assemble_floating_point_infinity(Is_negative, result);
    return errc::result_out_of_range; // Overflow example: "1e+1000"
  }

  uint64_t Mantissa = Initial_mantissa;
  int32_t Exponent = Normal_exponent;
  errc Error_code{};

  if (Normal_exponent < Traits::Minimum_binary_exponent) {
    // The exponent is too small to be represented by the floating-point type as a normal value, but it may be
    // representable as a denormal value.

    // The exponent of subnormal values (as defined by the mathematical model of floating-point numbers, not the
    // exponent field in the bit representation) is equal to the minimum exponent of normal values.
    Exponent = Traits::Minimum_binary_exponent;

    // Compute the number of bits by which we need to shift the mantissa in order to form a denormal number.
    const int32_t Denormal_mantissa_shift = Initial_exponent - Exponent;

    if (Denormal_mantissa_shift < 0) {
      Mantissa = Right_shift_with_rounding(Mantissa, static_cast<uint32_t>(-Denormal_mantissa_shift), Has_zero_tail);

      // from_chars in MSVC STL and strto[f|d|ld] in UCRT reports underflow only when the result is zero after
      // rounding to the floating-point format. This behavior is different from IEEE 754 underflow exception.
      if (Mantissa == 0) {
        Error_code = errc::result_out_of_range; // Underflow example: "1e-1000"
      }

      // When we round the mantissa, the result may be so large that the number becomes a normal value.
      // For example, consider the single-precision case where the mantissa is 0x01ffffff and a right shift
      // of 2 is required to shift the value into position. We perform the shift in two steps: we shift by
      // one bit, then we shift again and round using the dropped bit. The initial shift yields 0x00ffffff.
      // The rounding shift then yields 0x007fffff and because the least significant bit was 1, we add 1
      // to this number to round it. The final result is 0x00800000.

      // 0x00800000 is 24 bits, which is more than the 23 bits available in the mantissa.
      // Thus, we have rounded our denormal number into a normal number.

      // We detect this case here and re-adjust the mantissa and exponent appropriately, to form a normal number.
      // This is handled by Assemble_floating_point_value_no_shift.
    } else {
      Mantissa <<= Denormal_mantissa_shift;
    }
  } else {
    if (Normal_mantissa_shift < 0) {
      Mantissa = Right_shift_with_rounding(Mantissa, static_cast<uint32_t>(-Normal_mantissa_shift), Has_zero_tail);

      // When we round the mantissa, it may produce a result that is too large. In this case,
      // we divide the mantissa by two and increment the exponent (this does not change the value).
      // This is handled by Assemble_floating_point_value_no_shift.

      // The increment of the exponent may have generated a value too large to be represented.
      // In this case, report the overflow:
      if (Mantissa > Traits::Normal_mantissa_mask && Exponent == Traits::Maximum_binary_exponent) {
        Error_code = errc::result_out_of_range; // Overflow example: "1.ffffffp+127" for float
                                                // Overflow example: "1.fffffffffffff8p+1023" for double
      }
    } else {
      Mantissa <<= Normal_mantissa_shift;
    }
  }

  // Assemble the floating-point value from the computed components:
  using Uint_type = typename Traits::Uint_type;

  Assemble_floating_point_value_no_shift(Is_negative, Exponent, static_cast<Uint_type>(Mantissa), result);

  return Error_code;
}

// This function is part of the fast track for integer floating-point strings. It takes an integer and a sign and
// converts the value into its FloatingType representation, storing the result in the result object. If the value
// is not representable, +/-infinity is stored and overflow is reported (since this function deals with only integers,
// underflow is impossible).
template <class FloatingType>
[[nodiscard]] errc Assemble_floating_point_value_from_big_integer_flt(const Big_integer_flt &Integer_value,
                                                                      const uint32_t Integer_bits_of_precision,
                                                                      const bool Is_negative,
                                                                      const bool Has_nonzero_fractional_part,
                                                                      FloatingType &result) noexcept {
  using Traits = Floating_type_traits<FloatingType>;

  const int32_t Base_exponent = Traits::Mantissa_bits - 1;

  // Very fast case: If we have 64 bits of precision or fewer,
  // we can just take the two low order elements from the Big_integer_flt:
  if (Integer_bits_of_precision <= 64) {
    const int32_t Exponent = Base_exponent;

    const uint32_t Mantissa_low = Integer_value.Myused > 0 ? Integer_value.Mydata[0] : 0;
    const uint32_t Mantissa_high = Integer_value.Myused > 1 ? Integer_value.Mydata[1] : 0;
    const uint64_t Mantissa = Mantissa_low + (static_cast<uint64_t>(Mantissa_high) << 32);

    return Assemble_floating_point_value(Mantissa, Exponent, Is_negative, !Has_nonzero_fractional_part, result);
  }

  const uint32_t Top_element_bits = Integer_bits_of_precision % 32;
  const uint32_t Top_element_index = Integer_bits_of_precision / 32;

  const uint32_t Middle_element_index = Top_element_index - 1;
  const uint32_t Bottom_element_index = Top_element_index - 2;

  // Pretty fast case: If the top 64 bits occupy only two elements, we can just combine those two elements:
  if (Top_element_bits == 0) {
    const auto Exponent = static_cast<int32_t>(Base_exponent + Bottom_element_index * 32);

    const uint64_t Mantissa = Integer_value.Mydata[Bottom_element_index] +
                              (static_cast<uint64_t>(Integer_value.Mydata[Middle_element_index]) << 32);

    bool Has_zero_tail = !Has_nonzero_fractional_part;
    for (uint32_t Ix = 0; Has_zero_tail && Ix != Bottom_element_index; ++Ix) {
      Has_zero_tail = Integer_value.Mydata[Ix] == 0;
    }

    return Assemble_floating_point_value(Mantissa, Exponent, Is_negative, Has_zero_tail, result);
  }

  // Not quite so fast case: The top 64 bits span three elements in the Big_integer_flt. Assemble the three pieces:
  const uint32_t Top_element_mask = (1U << Top_element_bits) - 1;
  const uint32_t Top_element_shift = 64 - Top_element_bits; // Left

  const uint32_t Middle_element_shift = Top_element_shift - 32; // Left

  const uint32_t Bottom_element_bits = 32 - Top_element_bits;
  const uint32_t Bottom_element_mask = ~Top_element_mask;
  const uint32_t Bottom_element_shift = 32 - Bottom_element_bits; // Right

  const auto Exponent = static_cast<int32_t>(Base_exponent + Bottom_element_index * 32 + Top_element_bits);

  const uint64_t Mantissa =
      (static_cast<uint64_t>(Integer_value.Mydata[Top_element_index] & Top_element_mask) << Top_element_shift) +
      (static_cast<uint64_t>(Integer_value.Mydata[Middle_element_index]) << Middle_element_shift) +
      (static_cast<uint64_t>(Integer_value.Mydata[Bottom_element_index] & Bottom_element_mask) >> Bottom_element_shift);

  bool Has_zero_tail =
      !Has_nonzero_fractional_part && (Integer_value.Mydata[Bottom_element_index] & Top_element_mask) == 0;

  for (uint32_t Ix = 0; Has_zero_tail && Ix != Bottom_element_index; ++Ix) {
    Has_zero_tail = Integer_value.Mydata[Ix] == 0;
  }

  return Assemble_floating_point_value(Mantissa, Exponent, Is_negative, Has_zero_tail, result);
}

// Accumulates the decimal digits in [first_digit, last_digit) into the result high-precision integer.
// This function assumes that no overflow will occur.
inline void Accumulate_decimal_digits_into_big_integer_flt(const uint8_t *const first_digit,
                                                           const uint8_t *const last_digit,
                                                           Big_integer_flt &result) noexcept {
  // We accumulate nine digit chunks, transforming the base ten string into base one billion on the fly,
  // allowing us to reduce the number of high-precision multiplication and addition operations by 8/9.
  uint32_t Accumulator = 0;
  uint32_t Accumulator_count = 0;
  for (const uint8_t *It = first_digit; It != last_digit; ++It) {
    if (Accumulator_count == 9) {
      [[maybe_unused]] const bool Success1 = Multiply(result, 1'000'000'000); // assumes no overflow
      assert(Success1);
      [[maybe_unused]] const bool Success2 = Add(result, Accumulator); // assumes no overflow
      assert(Success2);

      Accumulator = 0;
      Accumulator_count = 0;
    }

    Accumulator *= 10;
    Accumulator += *It;
    ++Accumulator_count;
  }

  if (Accumulator_count != 0) {
    [[maybe_unused]] const bool Success3 = Multiply_by_power_of_ten(result, Accumulator_count); // assumes no overflow
    assert(Success3);
    [[maybe_unused]] const bool Success4 = Add(result, Accumulator); // assumes no overflow
    assert(Success4);
  }
}

// The core floating-point string parser for decimal strings. After a subject string is parsed and converted
// into a Floating_point_string object, if the subject string was determined to be a decimal string,
// the object is passed to this function. This function converts the decimal real value to floating-point.
template <class FloatingType>
[[nodiscard]] errc Convert_decimal_string_to_floating_type(const Floating_point_string &data, FloatingType &result,
                                                           bool Has_zero_tail) noexcept {
  using Traits = Floating_type_traits<FloatingType>;

  // To generate an N bit mantissa we require N + 1 bits of precision. The extra bit is used to correctly round
  // the mantissa (if there are fewer bits than this available, then that's totally okay;
  // in that case we use what we have and we don't need to round).
  const auto Required_bits_of_precision = static_cast<uint32_t>(Traits::Mantissa_bits + 1);

  // The input is of the form 0.mantissa * 10^exponent, where 'mantissa' are the decimal digits of the mantissa
  // and 'exponent' is the decimal exponent. We decompose the mantissa into two parts: an integer part and a
  // fractional part. If the exponent is positive, then the integer part consists of the first 'exponent' digits,
  // or all present digits if there are fewer digits. If the exponent is zero or negative, then the integer part
  // is empty. In either case, the remaining digits form the fractional part of the mantissa.
  const uint32_t Positive_exponent = static_cast<uint32_t>((std::max)(0, data.Myexponent));
  const uint32_t Integer_digits_present = (std::min)(Positive_exponent, data.Mymantissa_count);
  const uint32_t Integer_digits_missing = Positive_exponent - Integer_digits_present;
  const uint8_t *const Integer_first = data.Mymantissa;
  const uint8_t *const Integer_last = data.Mymantissa + Integer_digits_present;

  const uint8_t *const Fractional_first = Integer_last;
  const uint8_t *const Fractional_last = data.Mymantissa + data.Mymantissa_count;
  const auto Fractional_digits_present = static_cast<uint32_t>(Fractional_last - Fractional_first);

  // First, we accumulate the integer part of the mantissa into a Big_integer_flt:
  Big_integer_flt Integer_value{};
  Accumulate_decimal_digits_into_big_integer_flt(Integer_first, Integer_last, Integer_value);

  if (Integer_digits_missing > 0) {
    if (!Multiply_by_power_of_ten(Integer_value, Integer_digits_missing)) {
      Assemble_floating_point_infinity(data.Myis_negative, result);
      return errc::result_out_of_range; // Overflow example: "1e+2000"
    }
  }

  // At this point, the Integer_value contains the value of the integer part of the mantissa. If either
  // [1] this number has more than the required number of bits of precision or
  // [2] the mantissa has no fractional part, then we can assemble the result immediately:
  const uint32_t Integer_bits_of_precision = bit_scan_reverse(Integer_value);
  {
    const bool Has_zero_fractional_part = Fractional_digits_present == 0 && Has_zero_tail;

    if (Integer_bits_of_precision >= Required_bits_of_precision || Has_zero_fractional_part) {
      return Assemble_floating_point_value_from_big_integer_flt(Integer_value, Integer_bits_of_precision,
                                                                data.Myis_negative, !Has_zero_fractional_part, result);
    }
  }

  // Otherwise, we did not get enough bits of precision from the integer part, and the mantissa has a fractional
  // part. We parse the fractional part of the mantissa to obtain more bits of precision. To do this, we convert
  // the fractional part into an actual fraction N/M, where the numerator N is computed from the digits of the
  // fractional part, and the denominator M is computed as the power of 10 such that N/M is equal to the value
  // of the fractional part of the mantissa.
  Big_integer_flt Fractional_numerator{};
  Accumulate_decimal_digits_into_big_integer_flt(Fractional_first, Fractional_last, Fractional_numerator);

  const uint32_t Fractional_denominator_exponent =
      data.Myexponent < 0 ? Fractional_digits_present + static_cast<uint32_t>(-data.Myexponent)
                          : Fractional_digits_present;

  Big_integer_flt Fractional_denominator = Make_big_integer_flt_one();
  if (!Multiply_by_power_of_ten(Fractional_denominator, Fractional_denominator_exponent)) {
    // If there were any digits in the integer part, it is impossible to underflow (because the exponent
    // cannot possibly be small enough), so if we underflow here it is a true underflow and we return zero.
    Assemble_floating_point_zero(data.Myis_negative, result);
    return errc::result_out_of_range; // Underflow example: "1e-2000"
  }

  // Because we are using only the fractional part of the mantissa here, the numerator is guaranteed to be smaller
  // than the denominator. We normalize the fraction such that the most significant bit of the numerator is in the
  // same position as the most significant bit in the denominator. This ensures that when we later shift the
  // numerator N bits to the left, we will produce N bits of precision.
  const uint32_t Fractional_numerator_bits = bit_scan_reverse(Fractional_numerator);
  const uint32_t Fractional_denominator_bits = bit_scan_reverse(Fractional_denominator);

  const uint32_t Fractional_shift = Fractional_denominator_bits > Fractional_numerator_bits
                                        ? Fractional_denominator_bits - Fractional_numerator_bits
                                        : 0;

  if (Fractional_shift > 0) {
    [[maybe_unused]] const bool Shift_success1 =
        Shift_left(Fractional_numerator, Fractional_shift); // assumes no overflow
    assert(Shift_success1);
  }

  const uint32_t Required_fractional_bits_of_precision = Required_bits_of_precision - Integer_bits_of_precision;

  uint32_t Remaining_bits_of_precision_required = Required_fractional_bits_of_precision;
  if (Integer_bits_of_precision > 0) {
    // If the fractional part of the mantissa provides no bits of precision and cannot affect rounding,
    // we can just take whatever bits we got from the integer part of the mantissa. This is the case for numbers
    // like 5.0000000000000000000001, where the significant digits of the fractional part start so far to the
    // right that they do not affect the floating-point representation.

    // If the fractional shift is exactly equal to the number of bits of precision that we require,
    // then no fractional bits will be part of the result, but the result may affect rounding.
    // This is e.g. the case for large, odd integers with a fractional part greater than or equal to .5.
    // Thus, we need to do the division to correctly round the result.
    if (Fractional_shift > Remaining_bits_of_precision_required) {
      return Assemble_floating_point_value_from_big_integer_flt(
          Integer_value, Integer_bits_of_precision, data.Myis_negative,
          Fractional_digits_present != 0 || !Has_zero_tail, result);
    }

    Remaining_bits_of_precision_required -= Fractional_shift;
  }

  // If there was no integer part of the mantissa, we will need to compute the exponent from the fractional part.
  // The fractional exponent is the power of two by which we must multiply the fractional part to move it into the
  // range [1.0, 2.0). This will either be the same as the shift we computed earlier, or one greater than that shift:
  const uint32_t Fractional_exponent =
      Fractional_numerator < Fractional_denominator ? Fractional_shift + 1 : Fractional_shift;

  [[maybe_unused]] const bool Shift_success2 =
      Shift_left(Fractional_numerator, Remaining_bits_of_precision_required); // assumes no overflow
  assert(Shift_success2);

  uint64_t Fractional_mantissa = Divide(Fractional_numerator, Fractional_denominator);

  Has_zero_tail = Has_zero_tail && Fractional_numerator.Myused == 0;

  // We may have produced more bits of precision than were required. Check, and remove any "extra" bits:
  const uint32_t Fractional_mantissa_bits = bit_scan_reverse(Fractional_mantissa);
  if (Fractional_mantissa_bits > Required_fractional_bits_of_precision) {
    const uint32_t Shift = Fractional_mantissa_bits - Required_fractional_bits_of_precision;
    Has_zero_tail = Has_zero_tail && (Fractional_mantissa & ((1ULL << Shift) - 1)) == 0;
    Fractional_mantissa >>= Shift;
  }

  // Compose the mantissa from the integer and fractional parts:
  const uint32_t Integer_mantissa_low = Integer_value.Myused > 0 ? Integer_value.Mydata[0] : 0;
  const uint32_t Integer_mantissa_high = Integer_value.Myused > 1 ? Integer_value.Mydata[1] : 0;
  const uint64_t Integer_mantissa = Integer_mantissa_low + (static_cast<uint64_t>(Integer_mantissa_high) << 32);

  const uint64_t Complete_mantissa = (Integer_mantissa << Required_fractional_bits_of_precision) + Fractional_mantissa;

  // Compute the final exponent:
  // * If the mantissa had an integer part, then the exponent is one less than the number of bits we obtained
  // from the integer part. (It's one less because we are converting to the form 1.11111,
  // with one 1 to the left of the decimal point.)
  // * If the mantissa had no integer part, then the exponent is the fractional exponent that we computed.
  // Then, in both cases, we subtract an additional one from the exponent,
  // to account for the fact that we've generated an extra bit of precision, for use in rounding.
  const int32_t Final_exponent = Integer_bits_of_precision > 0 ? static_cast<int32_t>(Integer_bits_of_precision - 2)
                                                               : -static_cast<int32_t>(Fractional_exponent) - 1;

  return Assemble_floating_point_value(Complete_mantissa, Final_exponent, data.Myis_negative, Has_zero_tail, result);
}

template <class FloatingType>
[[nodiscard]] errc Convert_hexadecimal_string_to_floating_type(const Floating_point_string &data, FloatingType &result,
                                                               bool Has_zero_tail) noexcept {
  using Traits = Floating_type_traits<FloatingType>;

  uint64_t Mantissa = 0;
  int32_t Exponent = data.Myexponent + Traits::Mantissa_bits - 1;

  // Accumulate bits into the mantissa buffer
  const uint8_t *const Mantissa_last = data.Mymantissa + data.Mymantissa_count;
  const uint8_t *Mantissa_it = data.Mymantissa;
  while (Mantissa_it != Mantissa_last && Mantissa <= Traits::Normal_mantissa_mask) {
    Mantissa *= 16;
    Mantissa += *Mantissa_it++;
    Exponent -= 4; // The exponent is in binary; log2(16) == 4
  }

  while (Has_zero_tail && Mantissa_it != Mantissa_last) {
    Has_zero_tail = *Mantissa_it++ == 0;
  }

  return Assemble_floating_point_value(Mantissa, Exponent, data.Myis_negative, Has_zero_tail, result);
}

// ^^^^^^^^^^ DERIVED FROM corecrt_internal_strtox.h ^^^^^^^^^^

// C11 6.4.2.1 "General"
// digit: one of
//     0 1 2 3 4 5 6 7 8 9

// C11 6.4.4.1 "Integer constants"
// hexadecimal-digit: one of
//     0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F

// C11 6.4.4.2 "Floating constants" (without floating-suffix, hexadecimal-prefix)
// amended by C11 7.22.1.3 "The strtod, strtof, and strtold functions" making exponents optional
// LWG-3080: "the sign '+' may only appear in the exponent part"

// digit-sequence:
//     digit
//     digit-sequence digit

// hexadecimal-digit-sequence:
//     hexadecimal-digit
//     hexadecimal-digit-sequence hexadecimal-digit

// sign: one of
//     + -

// decimal-floating-constant:
//     fractional-constant exponent-part[opt]
//     digit-sequence exponent-part[opt]

// fractional-constant:
//     digit-sequence[opt] . digit-sequence
//     digit-sequence .

// exponent-part:
//     e sign[opt] digit-sequence
//     E sign[opt] digit-sequence

// hexadecimal-floating-constant:
//     hexadecimal-fractional-constant binary-exponent-part[opt]
//     hexadecimal-digit-sequence binary-exponent-part[opt]

// hexadecimal-fractional-constant:
//     hexadecimal-digit-sequence[opt] . hexadecimal-digit-sequence
//     hexadecimal-digit-sequence .

// binary-exponent-part:
//     p sign[opt] digit-sequence
//     P sign[opt] digit-sequence

template <class Floating>
[[nodiscard]] from_chars_result Ordinary_floating_from_chars(const wchar_t *const first, const wchar_t *const last,
                                                             Floating &value, const chars_format fmt,
                                                             const bool Minus_sign, const wchar_t *next) noexcept {
  // vvvvvvvvvv DERIVED FROM corecrt_internal_strtox.h WITH SIGNIFICANT MODIFICATIONS vvvvvvvvvv

  const bool Is_hexadecimal = fmt == chars_format::hex;
  const int base{Is_hexadecimal ? 16 : 10};

  // PERFORMANCE NOTE: Fp_string is intentionally left uninitialized. Zero-initialization is quite expensive
  // and is unnecessary. The benefit of not zero-initializing is greatest for short inputs.
  Floating_point_string Fp_string;

  // Record the optional minus sign:
  Fp_string.Myis_negative = Minus_sign;

  uint8_t *const Mantissa_first = Fp_string.Mymantissa;
  uint8_t *const Mantissa_last = std::end(Fp_string.Mymantissa);
  uint8_t *Mantissa_it = Mantissa_first;

  // [Whole_begin, Whole_end) will contain 0 or more digits/hexits
  const wchar_t *const Whole_begin = next;

  // Skip past any leading zeroes in the mantissa:
  for (; next != last && *next == '0'; ++next) {
  }
  const wchar_t *const Leading_zero_end = next;

  // Scan the integer part of the mantissa:
  for (; next != last; ++next) {
    const unsigned char Digit_value = Digit_from_char(*next);

    if (Digit_value >= base) {
      break;
    }

    if (Mantissa_it != Mantissa_last) {
      *Mantissa_it++ = Digit_value;
    }
  }
  const wchar_t *const Whole_end = next;

  // Defend against Exponent_adjustment integer overflow. (These values don't need to be strict.)
  constexpr ptrdiff_t Maximum_adjustment = 1'000'000;
  constexpr ptrdiff_t Minimum_adjustment = -1'000'000;

  // The exponent adjustment holds the number of digits in the mantissa buffer that appeared before the radix point.
  // It can be negative, and leading zeroes in the integer part are ignored. Examples:
  // For "03333.111", it is 4.
  // For "00000.111", it is 0.
  // For "00000.001", it is -2.
  int Exponent_adjustment = static_cast<int>((std::min)(Whole_end - Leading_zero_end, Maximum_adjustment));

  // [Whole_end, Dot_end) will contain 0 or 1 '.' characters
  if (next != last && *next == '.') {
    ++next;
  }
  const wchar_t *const Dot_end = next;

  // [Dot_end, Frac_end) will contain 0 or more digits/hexits

  // If we haven't yet scanned any nonzero digits, continue skipping over zeroes,
  // updating the exponent adjustment to account for the zeroes we are skipping:
  if (Exponent_adjustment == 0) {
    for (; next != last && *next == '0'; ++next) {
    }

    Exponent_adjustment = static_cast<int>((std::max)(Dot_end - next, Minimum_adjustment));
  }

  // Scan the fractional part of the mantissa:
  bool Has_zero_tail = true;

  for (; next != last; ++next) {
    const unsigned char Digit_value = Digit_from_char(*next);

    if (Digit_value >= base) {
      break;
    }

    if (Mantissa_it != Mantissa_last) {
      *Mantissa_it++ = Digit_value;
    } else {
      Has_zero_tail = Has_zero_tail && Digit_value == 0;
    }
  }
  const wchar_t *const Frac_end = next;

  // We must have at least 1 digit/hexit
  if (Whole_begin == Whole_end && Dot_end == Frac_end) {
    return {first, errc::invalid_argument};
  }

  const wchar_t Exponent_prefix{Is_hexadecimal ? L'p' : L'e'};

  bool Exponent_is_negative = false;
  int Exponent = 0;

  constexpr int Maximum_temporary_decimal_exponent = 5200;
  constexpr int Minimum_temporary_decimal_exponent = -5200;

  if (fmt != chars_format::fixed // N4713 23.20.3 [charconv.from.chars]/7.3
                                 // "if fmt has chars_format::fixed set but not chars_format::scientific,
                                 // the optional exponent part shall not appear"
      && next != last && (static_cast<unsigned char>(*next) | 0x20) == Exponent_prefix) { // found exponent prefix
    const wchar_t *Unread = next + 1;

    if (Unread != last && (*Unread == '+' || *Unread == '-')) { // found optional sign
      Exponent_is_negative = *Unread == '-';
      ++Unread;
    }

    while (Unread != last) {
      const unsigned char Digit_value = Digit_from_char(*Unread);

      if (Digit_value >= 10) {
        break;
      }

      // found decimal digit

      if (Exponent <= Maximum_temporary_decimal_exponent) {
        Exponent = Exponent * 10 + Digit_value;
      }

      ++Unread;
      next = Unread; // consume exponent-part/binary-exponent-part
    }

    if (Exponent_is_negative) {
      Exponent = -Exponent;
    }
  }

  // [Frac_end, Exponent_end) will either be empty or contain "[EPep] sign[opt] digit-sequence"
  const wchar_t *const Exponent_end = next;

  if (fmt == chars_format::scientific &&
      Frac_end == Exponent_end) { // N4713 23.20.3 [charconv.from.chars]/7.2
                                  // "if fmt has chars_format::scientific set but not chars_format::fixed,
                                  // the otherwise optional exponent part shall appear"
    return {first, errc::invalid_argument};
  }

  // Remove trailing zeroes from mantissa:
  while (Mantissa_it != Mantissa_first && *(Mantissa_it - 1) == 0) {
    --Mantissa_it;
  }

  // If the mantissa buffer is empty, the mantissa was composed of all zeroes (so the mantissa is 0).
  // All such strings have the value zero, regardless of what the exponent is (because 0 * b^n == 0 for all b and n).
  // We can return now. Note that we defer this check until after we scan the exponent, so that we can correctly
  // update next to point past the end of the exponent.
  if (Mantissa_it == Mantissa_first) {
    assert(Has_zero_tail);
    Assemble_floating_point_zero(Fp_string.Myis_negative, value);
    return {next, errc{}};
  }

  // Before we adjust the exponent, handle the case where we detected a wildly
  // out of range exponent during parsing and clamped the value:
  if (Exponent > Maximum_temporary_decimal_exponent) {
    Assemble_floating_point_infinity(Fp_string.Myis_negative, value);
    return {next, errc::result_out_of_range}; // Overflow example: "1e+9999"
  }

  if (Exponent < Minimum_temporary_decimal_exponent) {
    Assemble_floating_point_zero(Fp_string.Myis_negative, value);
    return {next, errc::result_out_of_range}; // Underflow example: "1e-9999"
  }

  // In hexadecimal floating constants, the exponent is a base 2 exponent. The exponent adjustment computed during
  // parsing has the same base as the mantissa (so, 16 for hexadecimal floating constants).
  // We therefore need to scale the base 16 multiplier to base 2 by multiplying by log2(16):
  const int Exponent_adjustment_multiplier{Is_hexadecimal ? 4 : 1};

  Exponent += Exponent_adjustment * Exponent_adjustment_multiplier;

  // Verify that after adjustment the exponent isn't wildly out of range (if it is, it isn't representable
  // in any supported floating-point format).
  if (Exponent > Maximum_temporary_decimal_exponent) {
    Assemble_floating_point_infinity(Fp_string.Myis_negative, value);
    return {next, errc::result_out_of_range}; // Overflow example: "10e+5199"
  }

  if (Exponent < Minimum_temporary_decimal_exponent) {
    Assemble_floating_point_zero(Fp_string.Myis_negative, value);
    return {next, errc::result_out_of_range}; // Underflow example: "0.001e-5199"
  }

  Fp_string.Myexponent = Exponent;
  Fp_string.Mymantissa_count = static_cast<uint32_t>(Mantissa_it - Mantissa_first);

  if (Is_hexadecimal) {
    const errc ec = Convert_hexadecimal_string_to_floating_type(Fp_string, value, Has_zero_tail);
    return {next, ec};
  }
  const errc ec = Convert_decimal_string_to_floating_type(Fp_string, value, Has_zero_tail);
  return {next, ec};
}

[[nodiscard]] inline bool Starts_with_case_insensitive(const wchar_t *first, const wchar_t *const last,
                                                       const wchar_t *Lowercase) noexcept {
  // pre: Lowercase contains only ['a', 'z'] and is null-terminated
  for (; first != last && *Lowercase != '\0'; ++first, ++Lowercase) {
    if ((static_cast<unsigned char>(*first) | 0x20) != *Lowercase) {
      return false;
    }
  }

  return *Lowercase == '\0';
}

template <class Floating>
[[nodiscard]] from_chars_result Infinity_from_chars(const wchar_t *const first, const wchar_t *const last,
                                                    Floating &value, const bool Minus_sign,
                                                    const wchar_t *next) noexcept {
  // pre: next points at 'i' (case-insensitively)
  if (!Starts_with_case_insensitive(next + 1, last, L"nf")) { // definitely invalid
    return {first, errc::invalid_argument};
  }

  // definitely inf
  next += 3;

  if (Starts_with_case_insensitive(next, last, L"inity")) { // definitely infinity
    next += 5;
  }

  Assemble_floating_point_infinity(Minus_sign, value);

  return {next, errc{}};
}

template <class Floating>
[[nodiscard]] from_chars_result Nan_from_chars(const wchar_t *const first, const wchar_t *const last, Floating &value,
                                               bool Minus_sign, const wchar_t *next) noexcept {
  // pre: next points at 'n' (case-insensitively)
  if (!Starts_with_case_insensitive(next + 1, last, L"an")) { // definitely invalid
    return {first, errc::invalid_argument};
  }

  // definitely nan
  next += 3;

  bool Quiet = true;

  if (next != last && *next == '(') { // possibly nan(n-char-sequence[opt])
    const wchar_t *const Seq_begin = next + 1;

    for (const wchar_t *Temp = Seq_begin; Temp != last; ++Temp) {
      if (*Temp == ')') { // definitely nan(n-char-sequence[opt])
        next = Temp + 1;

        if (Temp - Seq_begin == 3 && Starts_with_case_insensitive(Seq_begin, Temp, L"ind")) { // definitely nan(ind)
          // The UCRT considers indeterminate NaN to be negative quiet NaN with no payload bits set.
          // It parses "nan(ind)" and "-nan(ind)" identically.
          Minus_sign = true;
        } else if (Temp - Seq_begin == 4 &&
                   Starts_with_case_insensitive(Seq_begin, Temp, L"snan")) { // definitely nan(snan)
          Quiet = false;
        }

        break;
      }
      if (*Temp == '_' || ('0' <= *Temp && *Temp <= '9') || ('A' <= *Temp && *Temp <= 'Z') ||
          ('a' <= *Temp && *Temp <= 'z')) { // possibly nan(n-char-sequence[opt]), keep going
      } else {                              // definitely nan, not nan(n-char-sequence[opt])
        break;
      }
    }
  }

  // Intentional behavior difference between the UCRT and the STL:
  // strtod()/strtof() parse plain "nan" as being a quiet NaN with all payload bits set.
  // numeric_limits::quiet_NaN() returns a quiet NaN with no payload bits set.
  // This implementation of from_chars() has chosen to be consistent with numeric_limits.

  using Traits = Floating_type_traits<Floating>;
  using Uint_type = typename Traits::Uint_type;

  Uint_type Uint_value = Traits::Shifted_exponent_mask;

  if (Minus_sign) {
    Uint_value |= Traits::Shifted_sign_mask;
  }

  if (Quiet) {
    Uint_value |= Traits::Special_nan_mantissa_mask;
  } else {
    Uint_value |= 1;
  }

  value = std::bit_cast<Floating>(Uint_value);

  return {next, errc{}};
}

template <class Floating>
[[nodiscard]] from_chars_result Floating_from_chars(const wchar_t *const first, const wchar_t *const last,
                                                    Floating &value, const chars_format fmt) noexcept {

  _BELA_ASSERT(fmt == chars_format::general || fmt == chars_format::scientific || fmt == chars_format::fixed ||
                   fmt == chars_format::hex,
               "invalid format in from_chars()");

  bool Minus_sign = false;

  const wchar_t *next = first;

  if (next == last) {
    return {first, errc::invalid_argument};
  }

  if (*next == '-') {
    Minus_sign = true;
    ++next;

    if (next == last) {
      return {first, errc::invalid_argument};
    }
  }

  // Distinguish ordinary numbers versus inf/nan with a single test.
  // ordinary numbers start with ['.'] ['0', '9'] ['A', 'F'] ['a', 'f']
  // inf/nan start with ['I'] ['N'] ['i'] ['n']
  // All other starting characters are invalid.
  // Setting the 0x20 bit folds these ranges in a useful manner.
  // ordinary (and some invalid) starting characters are folded to ['.'] ['0', '9'] ['a', 'f']
  // inf/nan starting characters are folded to ['i'] ['n']
  // These are ordered: ['.'] ['0', '9'] ['a', 'f'] < ['i'] ['n']
  // Note that invalid starting characters end up on both sides of this test.
  const auto Folded_start = static_cast<unsigned char>(static_cast<unsigned char>(*next) | 0x20);

  if (Folded_start <= 'f') { // possibly an ordinary number
    return Ordinary_floating_from_chars(first, last, value, fmt, Minus_sign, next);
  }
  if (Folded_start == 'i') { // possibly inf
    return Infinity_from_chars(first, last, value, Minus_sign, next);
  } else if (Folded_start == 'n') { // possibly nan
    return Nan_from_chars(first, last, value, Minus_sign, next);
  } else { // definitely invalid
    return {first, errc::invalid_argument};
  }
}

from_chars_result from_chars(const wchar_t *const first, const wchar_t *const last, float &value,
                             const chars_format fmt) noexcept /* strengthened */ {
  return Floating_from_chars(first, last, value, fmt);
}
from_chars_result from_chars(const wchar_t *const first, const wchar_t *const last, double &value,
                             const chars_format fmt) noexcept /* strengthened */ {
  return Floating_from_chars(first, last, value, fmt);
}
from_chars_result from_chars(const wchar_t *const first, const wchar_t *const last, long double &value,
                             const chars_format fmt) noexcept /* strengthened */ {
  double d; // intentionally default-init
  const from_chars_result result = Floating_from_chars(first, last, d, fmt);

  if (result.ec == errc{}) {
    value = d;
  }

  return result;
}

template <class Floating>
[[nodiscard]] to_chars_result Floating_to_chars_hex_precision(wchar_t *first, wchar_t *const last, const Floating value,
                                                              int precision) noexcept {

  // * Determine the effective precision.
  // * Later, we'll decrement precision when printing each hexit after the decimal point.

  // The hexits after the decimal point correspond to the explicitly stored fraction bits.
  // float explicitly stores 23 fraction bits. 23 / 4 == 5.75, which is 6 hexits.
  // double explicitly stores 52 fraction bits. 52 / 4 == 13, which is 13 hexits.
  constexpr int Full_precision = std::is_same_v<Floating, float> ? 6 : 13;
  constexpr int Adjusted_explicit_bits = Full_precision * 4;

  if (precision < 0) {
    // C11 7.21.6.1 "The fprintf function"/5: "A negative precision argument is taken as if the precision were
    // omitted." /8: "if the precision is missing and FLT_RADIX is a power of 2, then the precision is sufficient
    // for an exact representation of the value"
    precision = Full_precision;
  }

  // * Extract the Ieee_mantissa and Ieee_exponent.
  using Traits = Floating_type_traits<Floating>;
  using Uint_type = typename Traits::Uint_type;

  const auto Uint_value = std::bit_cast<Uint_type>(value);
  const Uint_type Ieee_mantissa = Uint_value & Traits::Denormal_mantissa_mask;
  const auto Ieee_exponent = static_cast<int32_t>(Uint_value >> Traits::Exponent_shift);

  // * Prepare the Adjusted_mantissa. This is aligned to hexit boundaries,
  // * with the implicit bit restored (0 for zero values and subnormal values, 1 for normal values).
  // * Also calculate the Unbiased_exponent. This unifies the processing of zero, subnormal, and normal values.
  Uint_type Adjusted_mantissa;

  if constexpr (std::is_same_v<Floating, float>) {
    Adjusted_mantissa = Ieee_mantissa << 1; // align to hexit boundary (23 isn't divisible by 4)
  } else {
    Adjusted_mantissa = Ieee_mantissa; // already aligned (52 is divisible by 4)
  }

  int32_t Unbiased_exponent;

  if (Ieee_exponent == 0) { // zero or subnormal
    // implicit bit is 0

    if (Ieee_mantissa == 0) { // zero
      // C11 7.21.6.1 "The fprintf function"/8: "If the value is zero, the exponent is zero."
      Unbiased_exponent = 0;
    } else { // subnormal
      Unbiased_exponent = 1 - Traits::Exponent_bias;
    }
  } else {                                                       // normal
    Adjusted_mantissa |= Uint_type{1} << Adjusted_explicit_bits; // implicit bit is 1
    Unbiased_exponent = Ieee_exponent - Traits::Exponent_bias;
  }

  // Unbiased_exponent is within [-126, 127] for float, [-1022, 1023] for double.

  // * Decompose Unbiased_exponent into Sign_character and Absolute_exponent.
  char Sign_character;
  uint32_t Absolute_exponent;

  if (Unbiased_exponent < 0) {
    Sign_character = '-';
    Absolute_exponent = static_cast<uint32_t>(-Unbiased_exponent);
  } else {
    Sign_character = '+';
    Absolute_exponent = static_cast<uint32_t>(Unbiased_exponent);
  }

  // Absolute_exponent is within [0, 127] for float, [0, 1023] for double.

  // * Perform a single bounds check.
  {
    int32_t Exponent_length;

    if (Absolute_exponent < 10) {
      Exponent_length = 1;
    } else if (Absolute_exponent < 100) {
      Exponent_length = 2;
    } else if constexpr (std::is_same_v<Floating, float>) {
      Exponent_length = 3;
    } else if (Absolute_exponent < 1000) {
      Exponent_length = 3;
    } else {
      Exponent_length = 4;
    }

    // precision might be enormous; avoid integer overflow by testing it separately.
    ptrdiff_t Buffer_size = last - first;

    if (Buffer_size < precision) {
      return {last, errc::value_too_large};
    }

    Buffer_size -= precision;

    const int32_t Length_excluding_precision = 1                                     // leading hexit
                                               + static_cast<int32_t>(precision > 0) // possible decimal point
                                               // excluding `+ precision`, hexits after decimal point
                                               + 2                // "p+" or "p-"
                                               + Exponent_length; // exponent

    if (Buffer_size < Length_excluding_precision) {
      return {last, errc::value_too_large};
    }
  }

  // * Perform rounding when we've been asked to omit hexits.
  if (precision < Full_precision) {
    // precision is within [0, 5] for float, [0, 12] for double.

    // Dropped_bits is within [4, 24] for float, [4, 52] for double.
    const int Dropped_bits = (Full_precision - precision) * 4;

    // Perform rounding by adding an appropriately-shifted bit.

    // This can propagate carries all the way into the leading hexit. Examples:
    // "0.ff9" rounded to a precision of 2 is "1.00".
    // "1.ff9" rounded to a precision of 2 is "2.00".

    // Note that the leading hexit participates in the rounding decision. Examples:
    // "0.8" rounded to a precision of 0 is "0".
    // "1.8" rounded to a precision of 0 is "2".

    // Reference implementation with suboptimal codegen:
    // const bool Lsb_bit       = (Adjusted_mantissa & (Uint_type{1} << Dropped_bits)) != 0;
    // const bool Round_bit     = (Adjusted_mantissa & (Uint_type{1} << (Dropped_bits - 1))) != 0;
    // const bool Has_tail_bits = (Adjusted_mantissa & ((Uint_type{1} << (Dropped_bits - 1)) - 1)) != 0;
    // const bool Should_round = Should_round_up(Lsb_bit, Round_bit, Has_tail_bits);
    // Adjusted_mantissa += Uint_type{Should_round} << Dropped_bits;

    // Example for optimized implementation: Let Dropped_bits be 8.
    //          Bit index: ...[8]76543210
    // Adjusted_mantissa: ...[L]RTTTTTTT (not depicting known details, like hexit alignment)
    // By focusing on the bit at index Dropped_bits, we can avoid unnecessary branching and shifting.

    // Bit index: ...[8]76543210
    //  Lsb_bit: ...[L]RTTTTTTT
    const Uint_type Lsb_bit = Adjusted_mantissa;

    //  Bit index: ...9[8]76543210
    // Round_bit: ...L[R]TTTTTTT0
    const Uint_type Round_bit = Adjusted_mantissa << 1;

    // We can detect (without branching) whether any of the trailing bits are set.
    // Due to Should_round below, this computation will be used if and only if R is 1, so we can assume that here.
    //      Bit index: ...9[8]76543210
    //     Round_bit: ...L[1]TTTTTTT0
    // Has_tail_bits: ....[H]........

    // If all of the trailing bits T are 0, then `Round_bit - 1` will produce 0 for H (due to R being 1).
    // If any of the trailing bits T are 1, then `Round_bit - 1` will produce 1 for H (due to R being 1).
    const Uint_type Has_tail_bits = Round_bit - 1;

    // Finally, we can use Should_round_up() logic with bitwise-AND and bitwise-OR,
    // selecting just the bit at index Dropped_bits. This is the appropriately-shifted bit that we want.
    const Uint_type Should_round = Round_bit & (Has_tail_bits | Lsb_bit) & (Uint_type{1} << Dropped_bits);

    // This rounding technique is dedicated to the memory of Peppermint. =^..^=
    Adjusted_mantissa += Should_round;
  }

  // * Print the leading hexit, then mask it away.
  {
    const auto Nibble = static_cast<uint32_t>(Adjusted_mantissa >> Adjusted_explicit_bits);
    assert(Nibble < 3);
    const char Leading_hexit = static_cast<char>('0' + Nibble);

    *first++ = Leading_hexit;

    constexpr Uint_type Mask = (Uint_type{1} << Adjusted_explicit_bits) - 1;
    Adjusted_mantissa &= Mask;
  }

  // * Print the decimal point and trailing hexits.

  // C11 7.21.6.1 "The fprintf function"/8:
  // "if the precision is zero and the # flag is not specified, no decimal-point character appears."
  if (precision > 0) {
    *first++ = '.';

    int32_t Number_of_bits_remaining = Adjusted_explicit_bits; // 24 for float, 52 for double

    for (;;) {
      assert(Number_of_bits_remaining >= 4);
      assert(Number_of_bits_remaining % 4 == 0);
      Number_of_bits_remaining -= 4;

      const auto Nibble = static_cast<uint32_t>(Adjusted_mantissa >> Number_of_bits_remaining);
      assert(Nibble < 16);
      const wchar_t Hexit = Charconv_digits[Nibble];

      *first++ = Hexit;

      // precision is the number of hexits that still need to be printed.
      --precision;
      if (precision == 0) {
        break; // We're completely done with this phase.
      }
      // Otherwise, we need to keep printing hexits.

      if (Number_of_bits_remaining == 0) {
        // We've finished printing Adjusted_mantissa, so all remaining hexits are '0'.
        wmemset(first, '0', static_cast<size_t>(precision));
        first += precision;
        break;
      }

      // Mask away the hexit that we just printed, then keep looping.
      // (We skip this when breaking out of the loop above, because Adjusted_mantissa isn't used later.)
      const Uint_type Mask = (Uint_type{1} << Number_of_bits_remaining) - 1;
      Adjusted_mantissa &= Mask;
    }
  }

  // * Print the exponent.

  // C11 7.21.6.1 "The fprintf function"/8: "The exponent always contains at least one digit, and only as many more
  // digits as necessary to represent the decimal exponent of 2."

  // Performance note: We should take advantage of the known ranges of possible exponents.

  *first++ = 'p';
  *first++ = Sign_character;

  // We've already printed '-' if necessary, so uint32_t Absolute_exponent avoids testing that again.
  return bela::to_chars(first, last, Absolute_exponent);
}

template <class Floating>
[[nodiscard]] to_chars_result Floating_to_chars_hex_shortest(wchar_t *first, wchar_t *const last,
                                                             const Floating value) noexcept {

  // This prints "1.728p+0" instead of "2.e5p-1".
  // This prints "0.000002p-126" instead of "1p-149" for float.
  // This prints "0.0000000000001p-1022" instead of "1p-1074" for double.
  // This prioritizes being consistent with printf's de facto behavior (and hex-precision's behavior)
  // over minimizing the number of characters printed.

  using Traits = Floating_type_traits<Floating>;
  using Uint_type = typename Traits::Uint_type;

  const auto Uint_value = std::bit_cast<Uint_type>(value);

  if (Uint_value == 0) { // zero detected; write "0p+0" and return
    // C11 7.21.6.1 "The fprintf function"/8: "If the value is zero, the exponent is zero."
    // Special-casing zero is necessary because of the exponent.
    const wchar_t *const Str = L"0p+0";
    const size_t Len = 4;

    if (last - first < static_cast<ptrdiff_t>(Len)) {
      return {last, errc::value_too_large};
    }

    memcpy(first, Str, Len * sizeof(wchar_t));

    return {first + Len, errc{}};
  }

  const Uint_type Ieee_mantissa = Uint_value & Traits::Denormal_mantissa_mask;
  const auto Ieee_exponent = static_cast<int32_t>(Uint_value >> Traits::Exponent_shift);

  char Leading_hexit; // implicit bit
  int32_t Unbiased_exponent;

  if (Ieee_exponent == 0) { // subnormal
    Leading_hexit = '0';
    Unbiased_exponent = 1 - Traits::Exponent_bias;
  } else { // normal
    Leading_hexit = '1';
    Unbiased_exponent = Ieee_exponent - Traits::Exponent_bias;
  }

  // Performance note: Consider avoiding per-character bounds checking when there's plenty of space.

  if (first == last) {
    return {last, errc::value_too_large};
  }

  *first++ = Leading_hexit;

  if (Ieee_mantissa == 0) {
    // The fraction bits are all 0. Trim them away, including the decimal point.
  } else {
    if (first == last) {
      return {last, errc::value_too_large};
    }

    *first++ = '.';

    // The hexits after the decimal point correspond to the explicitly stored fraction bits.
    // float explicitly stores 23 fraction bits. 23 / 4 == 5.75, so we'll print at most 6 hexits.
    // double explicitly stores 52 fraction bits. 52 / 4 == 13, so we'll print at most 13 hexits.
    Uint_type Adjusted_mantissa;
    int32_t Number_of_bits_remaining;

    if constexpr (std::is_same_v<Floating, float>) {
      Adjusted_mantissa = Ieee_mantissa << 1; // align to hexit boundary (23 isn't divisible by 4)
      Number_of_bits_remaining = 24;          // 23 fraction bits + 1 alignment bit
    } else {
      Adjusted_mantissa = Ieee_mantissa; // already aligned (52 is divisible by 4)
      Number_of_bits_remaining = 52;     // 52 fraction bits
    }

    // do-while: The condition Adjusted_mantissa != 0 is initially true - we have nonzero fraction bits and we've
    // printed the decimal point. Each iteration, we print a hexit, mask it away, and keep looping if we still have
    // nonzero fraction bits. If there would be trailing '0' hexits, this trims them. If there wouldn't be trailing
    // '0' hexits, the same condition works (as we print the final hexit and mask it away); we don't need to test
    // Number_of_bits_remaining.
    do {
      assert(Number_of_bits_remaining >= 4);
      assert(Number_of_bits_remaining % 4 == 0);
      Number_of_bits_remaining -= 4;

      const auto Nibble = static_cast<uint32_t>(Adjusted_mantissa >> Number_of_bits_remaining);
      assert(Nibble < 16);
      const wchar_t Hexit = Charconv_digits[Nibble];

      if (first == last) {
        return {last, errc::value_too_large};
      }

      *first++ = Hexit;

      const Uint_type Mask = (Uint_type{1} << Number_of_bits_remaining) - 1;
      Adjusted_mantissa &= Mask;

    } while (Adjusted_mantissa != 0);
  }

  // C11 7.21.6.1 "The fprintf function"/8: "The exponent always contains at least one digit, and only as many more
  // digits as necessary to represent the decimal exponent of 2."

  // Performance note: We should take advantage of the known ranges of possible exponents.

  // float: Unbiased_exponent is within [-126, 127].
  // double: Unbiased_exponent is within [-1022, 1023].

  if (last - first < 2) {
    return {last, errc::value_too_large};
  }

  *first++ = 'p';

  if (Unbiased_exponent < 0) {
    *first++ = '-';
    Unbiased_exponent = -Unbiased_exponent;
  } else {
    *first++ = '+';
  }

  // We've already printed '-' if necessary, so static_cast<uint32_t> avoids testing that again.
  return bela::to_chars(first, last, static_cast<uint32_t>(Unbiased_exponent));
}

// For general precision, we can use lookup tables to avoid performing trial formatting.

// For a simple example, imagine counting the number of digits D in an integer, and needing to know
// whether D is less than 3, equal to 3/4/5/6, or greater than 6. We could use a lookup table:
// D | Largest integer with D digits
// 2 |      99
// 3 |     999
// 4 |   9'999
// 5 |  99'999
// 6 | 999'999
// 7 | table end
// Looking up an integer in this table with lower_bound() will work:
// * Too-small integers, like 7, 70, and 99, will cause lower_bound() to return the D == 2 row. (If all we care
//   about is whether D is less than 3, then it's okay to smash the D == 1 and D == 2 cases together.)
// * Integers in [100, 999] will cause lower_bound() to return the D == 3 row, and so forth.
// * Too-large integers, like 1'000'000 and above, will cause lower_bound() to return the end of the table. If we
//   compute D from that index, this will be considered D == 7, which will activate any "greater than 6" logic.

// Floating-point is slightly more complicated.

// The ordinary lookup tables are for X within [-5, 38] for float, and [-5, 308] for double.
// (-5 absorbs too-negative exponents, outside the P > X >= -4 criterion. 38 and 308 are the maximum exponents.)
// Due to the P > X condition, we can use a subset of the table for X within [-5, P - 1], suitably clamped.

// When P is small, rounding can affect X. For example:
// For P == 1, the largest double with X == 0 is: 9.4999999999999982236431605997495353221893310546875
// For P == 2, the largest double with X == 0 is: 9.949999999999999289457264239899814128875732421875
// For P == 3, the largest double with X == 0 is: 9.9949999999999992184029906638897955417633056640625

// Exponent adjustment is a concern for P within [1, 7] for float, and [1, 15] for double (determined via
// brute force). While larger values of P still perform rounding, they can't trigger exponent adjustment.
// This is because only values with repeated '9' digits can undergo exponent adjustment during rounding,
// and floating-point granularity limits the number of consecutive '9' digits that can appear.

// So, we need special lookup tables for small values of P.
// These tables have varying lengths due to the P > X >= -4 criterion. For example:
// For P == 1, need table entries for X: -5, -4, -3, -2, -1, 0
// For P == 2, need table entries for X: -5, -4, -3, -2, -1, 0, 1
// For P == 3, need table entries for X: -5, -4, -3, -2, -1, 0, 1, 2
// For P == 4, need table entries for X: -5, -4, -3, -2, -1, 0, 1, 2, 3

// We can concatenate these tables for compact storage, using triangular numbers to access them.
// The table for P begins at index (P - 1) * (P + 10) / 2 with length P + 5.

// For both the ordinary and special lookup tables, after an index I is returned by lower_bound(), X is I - 5.

// We need to special-case the floating-point value 0.0, which is considered to have X == 0.
// Otherwise, the lookup tables would consider it to have a highly negative X.

// Finally, because we're working with positive floating-point values,
// representation comparisons behave identically to floating-point comparisons.

// The following code generated the lookup tables for the scientific exponent X. Don't remove this code.

template <class Floating> struct General_precision_tables;

template <> struct General_precision_tables<float> {
  static constexpr int Max_special_P = 7;

  static constexpr uint32_t Special_X_table[63] = {
      0x38C73ABCU, 0x3A79096BU, 0x3C1BA5E3U, 0x3DC28F5CU, 0x3F733333U, 0x4117FFFFU, 0x38D0AAA7U, 0x3A826AA8U,
      0x3C230553U, 0x3DCBC6A7U, 0x3F7EB851U, 0x411F3333U, 0x42C6FFFFU, 0x38D19C3FU, 0x3A8301A7U, 0x3C23C211U,
      0x3DCCB295U, 0x3F7FDF3BU, 0x411FEB85U, 0x42C7E666U, 0x4479DFFFU, 0x38D1B468U, 0x3A8310C1U, 0x3C23D4F1U,
      0x3DCCCA2DU, 0x3F7FFCB9U, 0x411FFDF3U, 0x42C7FD70U, 0x4479FCCCU, 0x461C3DFFU, 0x38D1B6D2U, 0x3A831243U,
      0x3C23D6D4U, 0x3DCCCC89U, 0x3F7FFFACU, 0x411FFFCBU, 0x42C7FFBEU, 0x4479FFAEU, 0x461C3FCCU, 0x47C34FBFU,
      0x38D1B710U, 0x3A83126AU, 0x3C23D704U, 0x3DCCCCC6U, 0x3F7FFFF7U, 0x411FFFFAU, 0x42C7FFF9U, 0x4479FFF7U,
      0x461C3FFAU, 0x47C34FF9U, 0x497423F7U, 0x38D1B716U, 0x3A83126EU, 0x3C23D709U, 0x3DCCCCCCU, 0x3F7FFFFFU,
      0x411FFFFFU, 0x42C7FFFFU, 0x4479FFFFU, 0x461C3FFFU, 0x47C34FFFU, 0x497423FFU, 0x4B18967FU};

  static constexpr int Max_P = 39;

  static constexpr uint32_t Ordinary_X_table[44] = {
      0x38D1B717U, 0x3A83126EU, 0x3C23D70AU, 0x3DCCCCCCU, 0x3F7FFFFFU, 0x411FFFFFU, 0x42C7FFFFU, 0x4479FFFFU,
      0x461C3FFFU, 0x47C34FFFU, 0x497423FFU, 0x4B18967FU, 0x4CBEBC1FU, 0x4E6E6B27U, 0x501502F8U, 0x51BA43B7U,
      0x5368D4A5U, 0x551184E7U, 0x56B5E620U, 0x58635FA9U, 0x5A0E1BC9U, 0x5BB1A2BCU, 0x5D5E0B6BU, 0x5F0AC723U,
      0x60AD78EBU, 0x6258D726U, 0x64078678U, 0x65A96816U, 0x6753C21BU, 0x69045951U, 0x6AA56FA5U, 0x6C4ECB8FU,
      0x6E013F39U, 0x6FA18F07U, 0x7149F2C9U, 0x72FC6F7CU, 0x749DC5ADU, 0x76453719U, 0x77F684DFU, 0x799A130BU,
      0x7B4097CEU, 0x7CF0BDC2U, 0x7E967699U, 0x7F7FFFFFU};
};

template <> struct General_precision_tables<double> {
  static constexpr int Max_special_P = 15;

  static constexpr uint64_t Special_X_table[195] = {
      0x3F18E757928E0C9DU, 0x3F4F212D77318FC5U, 0x3F8374BC6A7EF9DBU, 0x3FB851EB851EB851U, 0x3FEE666666666666U,
      0x4022FFFFFFFFFFFFU, 0x3F1A1554FBDAD751U, 0x3F504D551D68C692U, 0x3F8460AA64C2F837U, 0x3FB978D4FDF3B645U,
      0x3FEFD70A3D70A3D7U, 0x4023E66666666666U, 0x4058DFFFFFFFFFFFU, 0x3F1A3387ECC8EB96U, 0x3F506034F3FD933EU,
      0x3F84784230FCF80DU, 0x3FB99652BD3C3611U, 0x3FEFFBE76C8B4395U, 0x4023FD70A3D70A3DU, 0x4058FCCCCCCCCCCCU,
      0x408F3BFFFFFFFFFFU, 0x3F1A368D04E0BA6AU, 0x3F506218230C7482U, 0x3F847A9E2BCF91A3U, 0x3FB99945B6C3760BU,
      0x3FEFFF972474538EU, 0x4023FFBE76C8B439U, 0x4058FFAE147AE147U, 0x408F3F9999999999U, 0x40C387BFFFFFFFFFU,
      0x3F1A36DA54164F19U, 0x3F506248748DF16FU, 0x3F847ADA91B16DCBU, 0x3FB99991361DC93EU, 0x3FEFFFF583A53B8EU,
      0x4023FFF972474538U, 0x4058FFF7CED91687U, 0x408F3FF5C28F5C28U, 0x40C387F999999999U, 0x40F869F7FFFFFFFFU,
      0x3F1A36E20F35445DU, 0x3F50624D49814ABAU, 0x3F847AE09BE19D69U, 0x3FB99998C2DA04C3U, 0x3FEFFFFEF39085F4U,
      0x4023FFFF583A53B8U, 0x4058FFFF2E48E8A7U, 0x408F3FFEF9DB22D0U, 0x40C387FF5C28F5C2U, 0x40F869FF33333333U,
      0x412E847EFFFFFFFFU, 0x3F1A36E2D51EC34BU, 0x3F50624DC5333A0EU, 0x3F847AE136800892U, 0x3FB9999984200AB7U,
      0x3FEFFFFFE5280D65U, 0x4023FFFFEF39085FU, 0x4058FFFFEB074A77U, 0x408F3FFFE5C91D14U, 0x40C387FFEF9DB22DU,
      0x40F869FFEB851EB8U, 0x412E847FE6666666U, 0x416312CFEFFFFFFFU, 0x3F1A36E2E8E94FFCU, 0x3F50624DD191D1FDU,
      0x3F847AE145F6467DU, 0x3FB999999773D81CU, 0x3FEFFFFFFD50CE23U, 0x4023FFFFFE5280D6U, 0x4058FFFFFDE7210BU,
      0x408F3FFFFD60E94EU, 0x40C387FFFE5C91D1U, 0x40F869FFFDF3B645U, 0x412E847FFD70A3D7U, 0x416312CFFE666666U,
      0x4197D783FDFFFFFFU, 0x3F1A36E2EAE3F7A7U, 0x3F50624DD2CE7AC8U, 0x3F847AE14782197BU, 0x3FB9999999629FD9U,
      0x3FEFFFFFFFBB47D0U, 0x4023FFFFFFD50CE2U, 0x4058FFFFFFCA501AU, 0x408F3FFFFFBCE421U, 0x40C387FFFFD60E94U,
      0x40F869FFFFCB923AU, 0x412E847FFFBE76C8U, 0x416312CFFFD70A3DU, 0x4197D783FFCCCCCCU, 0x41CDCD64FFBFFFFFU,
      0x3F1A36E2EB16A205U, 0x3F50624DD2EE2543U, 0x3F847AE147A9AE94U, 0x3FB9999999941A39U, 0x3FEFFFFFFFF920C8U,
      0x4023FFFFFFFBB47DU, 0x4058FFFFFFFAA19CU, 0x408F3FFFFFF94A03U, 0x40C387FFFFFBCE42U, 0x40F869FFFFFAC1D2U,
      0x412E847FFFF97247U, 0x416312CFFFFBE76CU, 0x4197D783FFFAE147U, 0x41CDCD64FFF99999U, 0x4202A05F1FFBFFFFU,
      0x3F1A36E2EB1BB30FU, 0x3F50624DD2F14FE9U, 0x3F847AE147ADA3E3U, 0x3FB9999999990CDCU, 0x3FEFFFFFFFFF5014U,
      0x4023FFFFFFFF920CU, 0x4058FFFFFFFF768FU, 0x408F3FFFFFFF5433U, 0x40C387FFFFFF94A0U, 0x40F869FFFFFF79C8U,
      0x412E847FFFFF583AU, 0x416312CFFFFF9724U, 0x4197D783FFFF7CEDU, 0x41CDCD64FFFF5C28U, 0x4202A05F1FFF9999U,
      0x42374876E7FF7FFFU, 0x3F1A36E2EB1C34C3U, 0x3F50624DD2F1A0FAU, 0x3F847AE147AE0938U, 0x3FB9999999998B86U,
      0x3FEFFFFFFFFFEE68U, 0x4023FFFFFFFFF501U, 0x4058FFFFFFFFF241U, 0x408F3FFFFFFFEED1U, 0x40C387FFFFFFF543U,
      0x40F869FFFFFFF294U, 0x412E847FFFFFEF39U, 0x416312CFFFFFF583U, 0x4197D783FFFFF2E4U, 0x41CDCD64FFFFEF9DU,
      0x4202A05F1FFFF5C2U, 0x42374876E7FFF333U, 0x426D1A94A1FFEFFFU, 0x3F1A36E2EB1C41BBU, 0x3F50624DD2F1A915U,
      0x3F847AE147AE135AU, 0x3FB9999999999831U, 0x3FEFFFFFFFFFFE3DU, 0x4023FFFFFFFFFEE6U, 0x4058FFFFFFFFFEA0U,
      0x408F3FFFFFFFFE48U, 0x40C387FFFFFFFEEDU, 0x40F869FFFFFFFEA8U, 0x412E847FFFFFFE52U, 0x416312CFFFFFFEF3U,
      0x4197D783FFFFFEB0U, 0x41CDCD64FFFFFE5CU, 0x4202A05F1FFFFEF9U, 0x42374876E7FFFEB8U, 0x426D1A94A1FFFE66U,
      0x42A2309CE53FFEFFU, 0x3F1A36E2EB1C4307U, 0x3F50624DD2F1A9E4U, 0x3F847AE147AE145EU, 0x3FB9999999999975U,
      0x3FEFFFFFFFFFFFD2U, 0x4023FFFFFFFFFFE3U, 0x4058FFFFFFFFFFDCU, 0x408F3FFFFFFFFFD4U, 0x40C387FFFFFFFFE4U,
      0x40F869FFFFFFFFDDU, 0x412E847FFFFFFFD5U, 0x416312CFFFFFFFE5U, 0x4197D783FFFFFFDEU, 0x41CDCD64FFFFFFD6U,
      0x4202A05F1FFFFFE5U, 0x42374876E7FFFFDFU, 0x426D1A94A1FFFFD7U, 0x42A2309CE53FFFE6U, 0x42D6BCC41E8FFFDFU,
      0x3F1A36E2EB1C4328U, 0x3F50624DD2F1A9F9U, 0x3F847AE147AE1477U, 0x3FB9999999999995U, 0x3FEFFFFFFFFFFFFBU,
      0x4023FFFFFFFFFFFDU, 0x4058FFFFFFFFFFFCU, 0x408F3FFFFFFFFFFBU, 0x40C387FFFFFFFFFDU, 0x40F869FFFFFFFFFCU,
      0x412E847FFFFFFFFBU, 0x416312CFFFFFFFFDU, 0x4197D783FFFFFFFCU, 0x41CDCD64FFFFFFFBU, 0x4202A05F1FFFFFFDU,
      0x42374876E7FFFFFCU, 0x426D1A94A1FFFFFBU, 0x42A2309CE53FFFFDU, 0x42D6BCC41E8FFFFCU, 0x430C6BF52633FFFBU};

  static constexpr int Max_P = 309;

  static constexpr uint64_t Ordinary_X_table[314] = {
      0x3F1A36E2EB1C432CU, 0x3F50624DD2F1A9FBU, 0x3F847AE147AE147AU, 0x3FB9999999999999U, 0x3FEFFFFFFFFFFFFFU,
      0x4023FFFFFFFFFFFFU, 0x4058FFFFFFFFFFFFU, 0x408F3FFFFFFFFFFFU, 0x40C387FFFFFFFFFFU, 0x40F869FFFFFFFFFFU,
      0x412E847FFFFFFFFFU, 0x416312CFFFFFFFFFU, 0x4197D783FFFFFFFFU, 0x41CDCD64FFFFFFFFU, 0x4202A05F1FFFFFFFU,
      0x42374876E7FFFFFFU, 0x426D1A94A1FFFFFFU, 0x42A2309CE53FFFFFU, 0x42D6BCC41E8FFFFFU, 0x430C6BF52633FFFFU,
      0x4341C37937E07FFFU, 0x4376345785D89FFFU, 0x43ABC16D674EC7FFU, 0x43E158E460913CFFU, 0x4415AF1D78B58C3FU,
      0x444B1AE4D6E2EF4FU, 0x4480F0CF064DD591U, 0x44B52D02C7E14AF6U, 0x44EA784379D99DB4U, 0x45208B2A2C280290U,
      0x4554ADF4B7320334U, 0x4589D971E4FE8401U, 0x45C027E72F1F1281U, 0x45F431E0FAE6D721U, 0x46293E5939A08CE9U,
      0x465F8DEF8808B024U, 0x4693B8B5B5056E16U, 0x46C8A6E32246C99CU, 0x46FED09BEAD87C03U, 0x4733426172C74D82U,
      0x476812F9CF7920E2U, 0x479E17B84357691BU, 0x47D2CED32A16A1B1U, 0x48078287F49C4A1DU, 0x483D6329F1C35CA4U,
      0x48725DFA371A19E6U, 0x48A6F578C4E0A060U, 0x48DCB2D6F618C878U, 0x4911EFC659CF7D4BU, 0x49466BB7F0435C9EU,
      0x497C06A5EC5433C6U, 0x49B18427B3B4A05BU, 0x49E5E531A0A1C872U, 0x4A1B5E7E08CA3A8FU, 0x4A511B0EC57E6499U,
      0x4A8561D276DDFDC0U, 0x4ABABA4714957D30U, 0x4AF0B46C6CDD6E3EU, 0x4B24E1878814C9CDU, 0x4B5A19E96A19FC40U,
      0x4B905031E2503DA8U, 0x4BC4643E5AE44D12U, 0x4BF97D4DF19D6057U, 0x4C2FDCA16E04B86DU, 0x4C63E9E4E4C2F344U,
      0x4C98E45E1DF3B015U, 0x4CCF1D75A5709C1AU, 0x4D03726987666190U, 0x4D384F03E93FF9F4U, 0x4D6E62C4E38FF872U,
      0x4DA2FDBB0E39FB47U, 0x4DD7BD29D1C87A19U, 0x4E0DAC74463A989FU, 0x4E428BC8ABE49F63U, 0x4E772EBAD6DDC73CU,
      0x4EACFA698C95390BU, 0x4EE21C81F7DD43A7U, 0x4F16A3A275D49491U, 0x4F4C4C8B1349B9B5U, 0x4F81AFD6EC0E1411U,
      0x4FB61BCCA7119915U, 0x4FEBA2BFD0D5FF5BU, 0x502145B7E285BF98U, 0x50559725DB272F7FU, 0x508AFCEF51F0FB5EU,
      0x50C0DE1593369D1BU, 0x50F5159AF8044462U, 0x512A5B01B605557AU, 0x516078E111C3556CU, 0x5194971956342AC7U,
      0x51C9BCDFABC13579U, 0x5200160BCB58C16CU, 0x52341B8EBE2EF1C7U, 0x526922726DBAAE39U, 0x529F6B0F092959C7U,
      0x52D3A2E965B9D81CU, 0x53088BA3BF284E23U, 0x533EAE8CAEF261ACU, 0x53732D17ED577D0BU, 0x53A7F85DE8AD5C4EU,
      0x53DDF67562D8B362U, 0x5412BA095DC7701DU, 0x5447688BB5394C25U, 0x547D42AEA2879F2EU, 0x54B249AD2594C37CU,
      0x54E6DC186EF9F45CU, 0x551C931E8AB87173U, 0x5551DBF316B346E7U, 0x558652EFDC6018A1U, 0x55BBE7ABD3781ECAU,
      0x55F170CB642B133EU, 0x5625CCFE3D35D80EU, 0x565B403DCC834E11U, 0x569108269FD210CBU, 0x56C54A3047C694FDU,
      0x56FA9CBC59B83A3DU, 0x5730A1F5B8132466U, 0x5764CA732617ED7FU, 0x5799FD0FEF9DE8DFU, 0x57D03E29F5C2B18BU,
      0x58044DB473335DEEU, 0x583961219000356AU, 0x586FB969F40042C5U, 0x58A3D3E2388029BBU, 0x58D8C8DAC6A0342AU,
      0x590EFB1178484134U, 0x59435CEAEB2D28C0U, 0x59783425A5F872F1U, 0x59AE412F0F768FADU, 0x59E2E8BD69AA19CCU,
      0x5A17A2ECC414A03FU, 0x5A4D8BA7F519C84FU, 0x5A827748F9301D31U, 0x5AB7151B377C247EU, 0x5AECDA62055B2D9DU,
      0x5B22087D4358FC82U, 0x5B568A9C942F3BA3U, 0x5B8C2D43B93B0A8BU, 0x5BC19C4A53C4E697U, 0x5BF6035CE8B6203DU,
      0x5C2B843422E3A84CU, 0x5C6132A095CE492FU, 0x5C957F48BB41DB7BU, 0x5CCADF1AEA12525AU, 0x5D00CB70D24B7378U,
      0x5D34FE4D06DE5056U, 0x5D6A3DE04895E46CU, 0x5DA066AC2D5DAEC3U, 0x5DD4805738B51A74U, 0x5E09A06D06E26112U,
      0x5E400444244D7CABU, 0x5E7405552D60DBD6U, 0x5EA906AA78B912CBU, 0x5EDF485516E7577EU, 0x5F138D352E5096AFU,
      0x5F48708279E4BC5AU, 0x5F7E8CA3185DEB71U, 0x5FB317E5EF3AB327U, 0x5FE7DDDF6B095FF0U, 0x601DD55745CBB7ECU,
      0x6052A5568B9F52F4U, 0x60874EAC2E8727B1U, 0x60BD22573A28F19DU, 0x60F2357684599702U, 0x6126C2D4256FFCC2U,
      0x615C73892ECBFBF3U, 0x6191C835BD3F7D78U, 0x61C63A432C8F5CD6U, 0x61FBC8D3F7B3340BU, 0x62315D847AD00087U,
      0x6265B4E5998400A9U, 0x629B221EFFE500D3U, 0x62D0F5535FEF2084U, 0x630532A837EAE8A5U, 0x633A7F5245E5A2CEU,
      0x63708F936BAF85C1U, 0x63A4B378469B6731U, 0x63D9E056584240FDU, 0x64102C35F729689EU, 0x6444374374F3C2C6U,
      0x647945145230B377U, 0x64AF965966BCE055U, 0x64E3BDF7E0360C35U, 0x6518AD75D8438F43U, 0x654ED8D34E547313U,
      0x6583478410F4C7ECU, 0x65B819651531F9E7U, 0x65EE1FBE5A7E7861U, 0x6622D3D6F88F0B3CU, 0x665788CCB6B2CE0CU,
      0x668D6AFFE45F818FU, 0x66C262DFEEBBB0F9U, 0x66F6FB97EA6A9D37U, 0x672CBA7DE5054485U, 0x6761F48EAF234AD3U,
      0x679671B25AEC1D88U, 0x67CC0E1EF1A724EAU, 0x680188D357087712U, 0x6835EB082CCA94D7U, 0x686B65CA37FD3A0DU,
      0x68A11F9E62FE4448U, 0x68D56785FBBDD55AU, 0x690AC1677AAD4AB0U, 0x6940B8E0ACAC4EAEU, 0x6974E718D7D7625AU,
      0x69AA20DF0DCD3AF0U, 0x69E0548B68A044D6U, 0x6A1469AE42C8560CU, 0x6A498419D37A6B8FU, 0x6A7FE52048590672U,
      0x6AB3EF342D37A407U, 0x6AE8EB0138858D09U, 0x6B1F25C186A6F04CU, 0x6B537798F428562FU, 0x6B88557F31326BBBU,
      0x6BBE6ADEFD7F06AAU, 0x6BF302CB5E6F642AU, 0x6C27C37E360B3D35U, 0x6C5DB45DC38E0C82U, 0x6C9290BA9A38C7D1U,
      0x6CC734E940C6F9C5U, 0x6CFD022390F8B837U, 0x6D3221563A9B7322U, 0x6D66A9ABC9424FEBU, 0x6D9C5416BB92E3E6U,
      0x6DD1B48E353BCE6FU, 0x6E0621B1C28AC20BU, 0x6E3BAA1E332D728EU, 0x6E714A52DFFC6799U, 0x6EA59CE797FB817FU,
      0x6EDB04217DFA61DFU, 0x6F10E294EEBC7D2BU, 0x6F451B3A2A6B9C76U, 0x6F7A6208B5068394U, 0x6FB07D457124123CU,
      0x6FE49C96CD6D16CBU, 0x7019C3BC80C85C7EU, 0x70501A55D07D39CFU, 0x708420EB449C8842U, 0x70B9292615C3AA53U,
      0x70EF736F9B3494E8U, 0x7123A825C100DD11U, 0x7158922F31411455U, 0x718EB6BAFD91596BU, 0x71C33234DE7AD7E2U,
      0x71F7FEC216198DDBU, 0x722DFE729B9FF152U, 0x7262BF07A143F6D3U, 0x72976EC98994F488U, 0x72CD4A7BEBFA31AAU,
      0x73024E8D737C5F0AU, 0x7336E230D05B76CDU, 0x736C9ABD04725480U, 0x73A1E0B622C774D0U, 0x73D658E3AB795204U,
      0x740BEF1C9657A685U, 0x74417571DDF6C813U, 0x7475D2CE55747A18U, 0x74AB4781EAD1989EU, 0x74E10CB132C2FF63U,
      0x75154FDD7F73BF3BU, 0x754AA3D4DF50AF0AU, 0x7580A6650B926D66U, 0x75B4CFFE4E7708C0U, 0x75EA03FDE214CAF0U,
      0x7620427EAD4CFED6U, 0x7654531E58A03E8BU, 0x768967E5EEC84E2EU, 0x76BFC1DF6A7A61BAU, 0x76F3D92BA28C7D14U,
      0x7728CF768B2F9C59U, 0x775F03542DFB8370U, 0x779362149CBD3226U, 0x77C83A99C3EC7EAFU, 0x77FE494034E79E5BU,
      0x7832EDC82110C2F9U, 0x7867A93A2954F3B7U, 0x789D9388B3AA30A5U, 0x78D27C35704A5E67U, 0x79071B42CC5CF601U,
      0x793CE2137F743381U, 0x79720D4C2FA8A030U, 0x79A6909F3B92C83DU, 0x79DC34C70A777A4CU, 0x7A11A0FC668AAC6FU,
      0x7A46093B802D578BU, 0x7A7B8B8A6038AD6EU, 0x7AB137367C236C65U, 0x7AE585041B2C477EU, 0x7B1AE64521F7595EU,
      0x7B50CFEB353A97DAU, 0x7B8503E602893DD1U, 0x7BBA44DF832B8D45U, 0x7BF06B0BB1FB384BU, 0x7C2485CE9E7A065EU,
      0x7C59A742461887F6U, 0x7C9008896BCF54F9U, 0x7CC40AABC6C32A38U, 0x7CF90D56B873F4C6U, 0x7D2F50AC6690F1F8U,
      0x7D63926BC01A973BU, 0x7D987706B0213D09U, 0x7DCE94C85C298C4CU, 0x7E031CFD3999F7AFU, 0x7E37E43C8800759BU,
      0x7E6DDD4BAA009302U, 0x7EA2AA4F4A405BE1U, 0x7ED754E31CD072D9U, 0x7F0D2A1BE4048F90U, 0x7F423A516E82D9BAU,
      0x7F76C8E5CA239028U, 0x7FAC7B1F3CAC7433U, 0x7FE1CCF385EBC89FU, 0x7FEFFFFFFFFFFFFFU};
};

template <class Floating>
[[nodiscard]] inline to_chars_result Floating_to_chars_general_precision(wchar_t *first, wchar_t *const last,
                                                                         const Floating value, int precision) noexcept {

  using Traits = Floating_type_traits<Floating>;
  using Uint_type = typename Traits::Uint_type;

  const auto Uint_value = std::bit_cast<Uint_type>(value);

  if (Uint_value == 0) { // zero detected; write "0" and return; precision is irrelevant due to zero-trimming
    if (first == last) {
      return {last, errc::value_too_large};
    }

    *first++ = '0';

    return {first, errc{}};
  }

  // C11 7.21.6.1 "The fprintf function"/5:
  // "A negative precision argument is taken as if the precision were omitted."
  // /8: "g,G [...] Let P equal the precision if nonzero, 6 if the precision is omitted,
  // or 1 if the precision is zero."

  // Performance note: It's possible to rewrite this for branchless codegen,
  // but profiling will be necessary to determine whether that's faster.
  if (precision < 0) {
    precision = 6;
  } else if (precision == 0) {
    precision = 1;
  } else if (precision < 1'000'000) {
    // precision is ok.
  } else {
    // Avoid integer overflow.
    // Due to general notation's zero-trimming behavior, we can simply clamp precision.
    // This is further clamped below.
    precision = 1'000'000;
  }

  // precision is now the Standard's P.

  // /8: "Then, if a conversion with style E would have an exponent of X:
  // - if P > X >= -4, the conversion is with style f (or F) and precision P - (X + 1).
  // - otherwise, the conversion is with style e (or E) and precision P - 1."

  // /8: "Finally, [...] any trailing zeros are removed from the fractional portion of the result
  // and the decimal-point character is removed if there is no fractional portion remaining."

  using Tables = General_precision_tables<Floating>;

  const Uint_type *Table_begin;
  const Uint_type *Table_end;

  if (precision <= Tables::Max_special_P) {
    Table_begin = Tables::Special_X_table + (precision - 1) * (precision + 10) / 2;
    Table_end = Table_begin + precision + 5;
  } else {
    Table_begin = Tables::Ordinary_X_table;
    Table_end = Table_begin + (std::min)(precision, Tables::Max_P) + 5;
  }

  // Profiling indicates that linear search is faster than binary search for small tables.
  // Performance note: lambda captures may have a small performance cost.
  const Uint_type *const Table_lower_bound = [=] {
    if constexpr (!std::is_same_v<Floating, float>) {
      if (precision > 155) { // threshold determined via profiling
        return std::lower_bound(Table_begin, Table_end, Uint_value, std::less{});
      }
    }

    return std::find_if(Table_begin, Table_end, [=](const Uint_type Elem) { return Uint_value <= Elem; });
  }();

  const ptrdiff_t Table_index = Table_lower_bound - Table_begin;
  const int Scientific_exponent_X = static_cast<int>(Table_index - 5);
  const bool Use_fixed_notation = precision > Scientific_exponent_X && Scientific_exponent_X >= -4;

  // Performance note: it might (or might not) be faster to modify Ryu Printf to perform zero-trimming.
  // Such modifications would involve a fairly complicated state machine (notably, both '0' and '9' digits would
  // need to be buffered, due to rounding), and that would have performance costs due to increased branching.
  // Here, we're using a simpler approach: writing into a local buffer, manually zero-trimming, and then copying into
  // the output range. The necessary buffer size is reasonably small, the zero-trimming logic is simple and fast,
  // and the final copying is also fast.

  constexpr int Max_output_length =
      std::is_same_v<Floating, float> ? 117 : 773; // cases: 0x1.fffffep-126f and 0x1.fffffffffffffp-1022
  constexpr int Max_fixed_precision =
      std::is_same_v<Floating, float> ? 37 : 66; // cases: 0x1.fffffep-14f and 0x1.fffffffffffffp-14
  constexpr int Max_scientific_precision =
      std::is_same_v<Floating, float> ? 111 : 766; // cases: 0x1.fffffep-126f and 0x1.fffffffffffffp-1022

  // Note that Max_output_length is determined by scientific notation and is more than enough for fixed notation.
  // 0x1.fffffep+127f is 39 digits, plus 1 for '.', plus Max_fixed_precision for '0' digits, equals 77.
  // 0x1.fffffffffffffp+1023 is 309 digits, plus 1 for '.', plus Max_fixed_precision for '0' digits, equals 376.

  wchar_t Buffer[Max_output_length];
  const wchar_t *const Significand_first = Buffer; // e.g. "1.234"
  const wchar_t *Significand_last = nullptr;
  const wchar_t *Exponent_first = nullptr; // e.g. "e-05"
  const wchar_t *Exponent_last = nullptr;
  int Effective_precision; // number of digits printed after the decimal point, before trimming

  // Write into the local buffer.
  // Clamping Effective_precision allows Buffer to be as small as possible, and increases efficiency.
  if (Use_fixed_notation) {
    Effective_precision = (std::min)(precision - (Scientific_exponent_X + 1), Max_fixed_precision);
    const to_chars_result Buf_result =
        Floating_to_chars_fixed_precision(Buffer, std::end(Buffer), value, Effective_precision);
    assert(Buf_result.ec == errc{});
    Significand_last = Buf_result.ptr;
  } else {
    Effective_precision = (std::min)(precision - 1, Max_scientific_precision);
    const to_chars_result Buf_result =
        Floating_to_chars_scientific_precision(Buffer, std::end(Buffer), value, Effective_precision);
    assert(Buf_result.ec == errc{});
    Significand_last = std::find(Buffer, Buf_result.ptr, 'e');
    Exponent_first = Significand_last;
    Exponent_last = Buf_result.ptr;
  }

  // If we printed a decimal point followed by digits, perform zero-trimming.
  if (Effective_precision > 0) {
    while (Significand_last[-1] == '0') { // will stop at '.' or a nonzero digit
      --Significand_last;
    }

    if (Significand_last[-1] == '.') {
      --Significand_last;
    }
  }

  // Copy the significand to the output range.
  const ptrdiff_t Significand_distance = Significand_last - Significand_first;
  if (last - first < Significand_distance) {
    return {last, errc::value_too_large};
  }
  memcpy(first, Significand_first, static_cast<size_t>(Significand_distance) * sizeof(wchar_t));
  first += Significand_distance;

  // Copy the exponent to the output range.
  if (!Use_fixed_notation) {
    const ptrdiff_t Exponent_distance = Exponent_last - Exponent_first;
    if (last - first < Exponent_distance) {
      return {last, errc::value_too_large};
    }
    memcpy(first, Exponent_first, static_cast<size_t>(Exponent_distance) * sizeof(wchar_t));
    first += Exponent_distance;
  }

  return {first, errc{}};
}

enum class Floating_to_chars_overload { Plain, Format_only, Format_precision };

template <Floating_to_chars_overload Overload, class Floating>
[[nodiscard]] to_chars_result Floating_to_chars(wchar_t *first, wchar_t *const last, Floating value,
                                                const chars_format fmt, const int precision) noexcept {

  if constexpr (Overload == Floating_to_chars_overload::Plain) {
    assert(fmt == chars_format{}); // plain overload must pass chars_format{} internally
  } else {
    _BELA_ASSERT(fmt == chars_format::general || fmt == chars_format::scientific || fmt == chars_format::fixed ||
                     fmt == chars_format::hex,
                 "invalid format in to_chars()");
  }

  using Traits = Floating_type_traits<Floating>;
  using Uint_type = typename Traits::Uint_type;

  auto Uint_value = std::bit_cast<Uint_type>(value);

  const bool Was_negative = (Uint_value & Traits::Shifted_sign_mask) != 0;

  if (Was_negative) { // sign bit detected; write minus sign and clear sign bit
    if (first == last) {
      return {last, errc::value_too_large};
    }

    *first++ = '-';

    Uint_value &= ~Traits::Shifted_sign_mask;
    value = std::bit_cast<Floating>(Uint_value);
  }

  if ((Uint_value & Traits::Shifted_exponent_mask) == Traits::Shifted_exponent_mask) {
    // inf/nan detected; write appropriate string and return
    const wchar_t *Str;
    size_t Len;

    const Uint_type Mantissa = Uint_value & Traits::Denormal_mantissa_mask;

    if (Mantissa == 0) {
      Str = L"inf";
      Len = 3;
    } else if (Was_negative && Mantissa == Traits::Special_nan_mantissa_mask) {
      // When a NaN value has the sign bit set, the quiet bit set, and all other mantissa bits cleared,
      // the UCRT interprets it to mean "indeterminate", and indicates this by printing "-nan(ind)".
      Str = L"nan(ind)";
      Len = 8;
    } else if ((Mantissa & Traits::Special_nan_mantissa_mask) != 0) {
      Str = L"nan";
      Len = 3;
    } else {
      Str = L"nan(snan)";
      Len = 9;
    }

    if (last - first < static_cast<ptrdiff_t>(Len)) {
      return {last, errc::value_too_large};
    }

    memcpy(first, Str, Len * sizeof(wchar_t));

    return {first + Len, errc{}};
  }

  if constexpr (Overload == Floating_to_chars_overload::Plain) {
    return Floating_to_chars_ryu(first, last, value, chars_format{});
  } else if constexpr (Overload == Floating_to_chars_overload::Format_only) {
    if (fmt == chars_format::hex) {
      return Floating_to_chars_hex_shortest(first, last, value);
    }

    return Floating_to_chars_ryu(first, last, value, fmt);
  } else if constexpr (Overload == Floating_to_chars_overload::Format_precision) {
    switch (fmt) {
    case chars_format::scientific:
      return Floating_to_chars_scientific_precision(first, last, value, precision);
    case chars_format::fixed:
      return Floating_to_chars_fixed_precision(first, last, value, precision);
    case chars_format::general:
      return Floating_to_chars_general_precision(first, last, value, precision);
    case chars_format::hex:
    default: // avoid warning C4715: not all control paths return a value
      return Floating_to_chars_hex_precision(first, last, value, precision);
    }
  }
}

to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const float value) noexcept /* strengthened */ {
  return Floating_to_chars<Floating_to_chars_overload::Plain>(first, last, value, chars_format{}, 0);
}
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const double value) noexcept
/* strengthened */ {
  return Floating_to_chars<Floating_to_chars_overload::Plain>(first, last, value, chars_format{}, 0);
}
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const long double value) noexcept
/* strengthened */ {
  return Floating_to_chars<Floating_to_chars_overload::Plain>(first, last, static_cast<double>(value), chars_format{},
                                                              0);
}
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const float value, const chars_format fmt) noexcept
/* strengthened */ {
  return Floating_to_chars<Floating_to_chars_overload::Format_only>(first, last, value, fmt, 0);
}
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const double value, const chars_format fmt) noexcept
/* strengthened */ {
  return Floating_to_chars<Floating_to_chars_overload::Format_only>(first, last, value, fmt, 0);
}
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const long double value,
                         const chars_format fmt) noexcept /* strengthened */ {
  return Floating_to_chars<Floating_to_chars_overload::Format_only>(first, last, static_cast<double>(value), fmt, 0);
}

to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const float value, const chars_format fmt,
                         const int precision) noexcept /* strengthened */ {
  return Floating_to_chars<Floating_to_chars_overload::Format_precision>(first, last, value, fmt, precision);
}
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const double value, const chars_format fmt,
                         const int precision) noexcept /* strengthened */ {
  return Floating_to_chars<Floating_to_chars_overload::Format_precision>(first, last, value, fmt, precision);
}
to_chars_result to_chars(wchar_t *const first, wchar_t *const last, const long double value, const chars_format fmt,
                         const int precision) noexcept /* strengthened */ {
  return Floating_to_chars<Floating_to_chars_overload::Format_precision>(first, last, static_cast<double>(value), fmt,
                                                                         precision);
}

} // namespace bela