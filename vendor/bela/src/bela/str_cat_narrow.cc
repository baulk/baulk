// PORT from absl::StrCat
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
#include <cassert>
#include <cstring>
#include <bit>
#include <bela/str_cat_narrow.hpp>
#include <bela/endian.hpp>
#ifdef __SSE4_2__
#include <x86intrin.h>
#endif

namespace bela::narrow {
namespace numbers_internal {
// clang-format off
constexpr const char kHexTable[513] =
    "000102030405060708090a0b0c0d0e0f"
    "101112131415161718191a1b1c1d1e1f"
    "202122232425262728292a2b2c2d2e2f"
    "303132333435363738393a3b3c3d3e3f"
    "404142434445464748494a4b4c4d4e4f"
    "505152535455565758595a5b5c5d5e5f"
    "606162636465666768696a6b6c6d6e6f"
    "707172737475767778797a7b7c7d7e7f"
    "808182838485868788898a8b8c8d8e8f"
    "909192939495969798999a9b9c9d9e9f"
    "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
    "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
    "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
    "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
    "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
    "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";

//clang-format on

// FastHexToBufferZeroPad16()
//
// Outputs `val` into `out` as if by `snprintf(out, 17, "%016x", val)` but
// without the terminating null character. Thus `out` must be of length >= 16.
// Returns the number of non-pad digits of the output (it can never be zero
// since 0 has one digit).
inline size_t FastHexToBufferZeroPad16(uint64_t val, char *out) {
#ifdef __SSE4_2__
  uint64_t be = bela::htons(val);
  const auto kNibbleMask = _mm_set1_epi8(0xf);
  const auto kHexDigits = _mm_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7',
                                        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f');
  auto v = _mm_loadl_epi64(reinterpret_cast<__m128i *>(&be)); // load lo dword
  auto v4 = _mm_srli_epi64(v, 4);                             // shift 4 right
  auto il = _mm_unpacklo_epi8(v4, v);              // interleave bytes
  auto m = _mm_and_si128(il, kNibbleMask);         // mask out nibbles
  auto hexchars = _mm_shuffle_epi8(kHexDigits, m); // hex chars
  _mm_storeu_si128(reinterpret_cast<__m128i *>(out), hexchars);
#else
  for (int i = 0; i < 8; ++i) {
    auto byte = (val >> (56 - 8 * i)) & 0xFF;
    auto *hex = &numbers_internal::kHexTable[byte * 2];
    std::memcpy(out + 2 * i, hex, 2);
  }
#endif
  // | 0x1 so that even 0 has 1 digit.
  return 16 - std::countl_zero(val | 0x1) / 4;
}
} // namespace numbers_internal

AlphaNum::AlphaNum(Hex hex) {
  char *const end = &digits_[kFastToBufferSize];
  auto real_width =
      numbers_internal::FastHexToBufferZeroPad16(hex.value, end - 16);
  if (real_width >= hex.width) {
    piece_ = std::string_view(end - real_width, real_width);
  } else {
    // Pad first 16 chars because FastHexToBufferZeroPad16 pads only to 16 and
    // max pad width can be up to 20.
    std::memset(end - 32, hex.fill, 16);
    // Patch up everything else up to the real_width.
    std::memset(end - real_width - 16, hex.fill, 16);
    piece_ = std::string_view(end - hex.width, hex.width);
  }
}

AlphaNum::AlphaNum(Dec dec) {
  char *const end = &digits_[kFastToBufferSize];
  char *const minfill = end - dec.width;
  char *writer = end;
  uint64_t value = dec.value;
  bool neg = dec.neg;
  while (value > 9) {
    *--writer = static_cast<char>(L'0' + (value % 10));
    value /= 10;
  }
  *--writer = static_cast<char>(L'0' + value);
  if (neg) {
    *--writer = '-';
  }

  ptrdiff_t fillers = writer - minfill;
  if (fillers > 0) {
    // Tricky: if the fill character is ' ', then it's <fill><+/-><digits>
    // But...: if the fill character is '0', then it's <+/-><fill><digits>
    bool add_sign_again = false;
    if (neg && dec.fill == L'0') { // If filling with '0',
      ++writer;                    // ignore the sign we just added
      add_sign_again = true;       // and re-add the sign later.
    }
    writer -= fillers;
    std::fill_n(writer, fillers, dec.fill);
    if (add_sign_again) {
      *--writer = L'-';
    }
  }

  piece_ = std::string_view(writer, end - writer);
}

// ----------------------------------------------------------------------
// StringCat()
//    This merges the given strings or integers, with no delimiter. This
//    is designed to be the fastest possible way to construct a string out
//    of a mix of raw C strings, string_views, strings, and integer values.
// ----------------------------------------------------------------------

// Append is merely a version of memcpy that returns the address of the byte
// after the area just overwritten.
static char *Append(char *out, const AlphaNum &x) {
  // memcpy is allowed to overwrite arbitrary memory, so doing this after the
  // call would force an extra fetch of x.size().
  char *after = out + x.size();
  memcpy(out, x.data(), x.size());
  return after;
}

std::string StringCat(const AlphaNum &a, const AlphaNum &b) {
  std::string result;
  result.resize(a.size() + b.size());
  char *const begin = &*result.begin();
  char *out = begin;
  out = Append(out, a);
  out = Append(out, b);
  assert(out == begin + result.size());
  return result;
}

std::string StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c) {
  std::string result;
  result.resize(a.size() + b.size() + c.size());
  char *const begin = &*result.begin();
  char *out = begin;
  out = Append(out, a);
  out = Append(out, b);
  out = Append(out, c);
  assert(out == begin + result.size());
  return result;
}

std::string StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
                      const AlphaNum &d) {
  std::string result;
  result.resize(a.size() + b.size() + c.size() + d.size());
  char *const begin = &*result.begin();
  char *out = begin;
  out = Append(out, a);
  out = Append(out, b);
  out = Append(out, c);
  out = Append(out, d);
  assert(out == begin + result.size());
  return result;
}

namespace strings_internal {

// Do not call directly - these are not part of the public API.
std::string CatPieces(std::initializer_list<std::string_view> pieces) {
  std::string result;
  size_t total_size = 0;
  for (const std::string_view piece : pieces) {
    total_size += piece.size();
  }
  result.resize(total_size);

  char *const begin = &*result.begin();
  char *out = begin;
  for (const std::string_view piece : pieces) {
    const size_t this_size = piece.size();
    memcpy(out, piece.data(), this_size);
    out += this_size;
  }
  assert(out == begin + result.size());
  return result;
}

// It's possible to call StrAppend with an std::string_view that is itself a
// fragment of the string we're appending to.  However the results of this are
// random. Therefore, check for this in debug mode.  Use unsigned math so we
// only have to do one comparison. Note, there's an exception case: appending an
// empty string is always allowed.
#define ASSERT_NO_OVERLAP(dest, src)                                           \
  assert(((src).size() == 0) ||                                                \
         (uintptr_t((src).data() - (dest).data()) > uintptr_t((dest).size())))

void AppendPieces(std::string *dest,
                  std::initializer_list<std::string_view> pieces) {
  size_t old_size = dest->size();
  size_t total_size = old_size;
  for (const std::string_view piece : pieces) {
    total_size += piece.size();
  }
  dest->resize(total_size);

  char *const begin = &*dest->begin();
  char *out = begin + old_size;
  for (const std::string_view piece : pieces) {
    const size_t this_size = piece.size();
    memcpy(out, piece.data(), this_size);
    out += this_size;
  }
  assert(out == begin + dest->size());
}

} // namespace strings_internal

void StrAppend(std::string *dest, const AlphaNum &a) {
  ASSERT_NO_OVERLAP(*dest, a);
  dest->append(a.data(), a.size());
}

void StrAppend(std::string *dest, const AlphaNum &a, const AlphaNum &b) {
  ASSERT_NO_OVERLAP(*dest, a);
  ASSERT_NO_OVERLAP(*dest, b);
  std::string::size_type old_size = dest->size();
  dest->resize(old_size + a.size() + b.size());
  char *const begin = &*dest->begin();
  char *out = begin + old_size;
  out = Append(out, a);
  out = Append(out, b);
  assert(out == begin + dest->size());
}

void StrAppend(std::string *dest, const AlphaNum &a, const AlphaNum &b,
               const AlphaNum &c) {
  ASSERT_NO_OVERLAP(*dest, a);
  ASSERT_NO_OVERLAP(*dest, b);
  ASSERT_NO_OVERLAP(*dest, c);
  std::string::size_type old_size = dest->size();
  dest->resize(old_size + a.size() + b.size() + c.size());
  char *const begin = &*dest->begin();
  char *out = begin + old_size;
  out = Append(out, a);
  out = Append(out, b);
  out = Append(out, c);
  assert(out == begin + dest->size());
}

void StrAppend(std::string *dest, const AlphaNum &a, const AlphaNum &b,
               const AlphaNum &c, const AlphaNum &d) {
  ASSERT_NO_OVERLAP(*dest, a);
  ASSERT_NO_OVERLAP(*dest, b);
  ASSERT_NO_OVERLAP(*dest, c);
  ASSERT_NO_OVERLAP(*dest, d);
  std::string::size_type old_size = dest->size();
  dest->resize(old_size + a.size() + b.size() + c.size() + d.size());
  char *const begin = &*dest->begin();
  char *out = begin + old_size;
  out = Append(out, a);
  out = Append(out, b);
  out = Append(out, c);
  out = Append(out, d);
  assert(out == begin + dest->size());
}
} // namespace bela::narrow
