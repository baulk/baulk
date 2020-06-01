///
#include <cassert>
#include <bela/memutil.hpp>
#include <bela/strcat.hpp>

namespace bela {
AlphaNum::AlphaNum(Hex hex) {
  wchar_t *const end = &digits_[numbers_internal::kFastToBufferSize];
  wchar_t *writer = end;
  uint64_t value = hex.value;
  static constexpr const wchar_t hexdigits[] = L"0123456789abcdef";
  do {
    *--writer = hexdigits[value & 0xF];
    value >>= 4;
  } while (value != 0);

  wchar_t *beg;
  if (end - writer < hex.width) {
    beg = end - hex.width;
    std::fill_n(beg, writer - beg, hex.fill);
  } else {
    beg = writer;
  }

  piece_ = std::wstring_view(beg, end - beg);
}

AlphaNum::AlphaNum(Dec dec) {
  assert(dec.width <= numbers_internal::kFastToBufferSize);
  wchar_t *const end = &digits_[numbers_internal::kFastToBufferSize];
  wchar_t *const minfill = end - dec.width;
  wchar_t *writer = end;
  uint64_t value = dec.value;
  bool neg = dec.neg;
  while (value > 9) {
    *--writer = '0' + (value % 10);
    value /= 10;
  }
  *--writer = static_cast<wchar_t>('0' + value);
  if (neg)
    *--writer = '-';

  ptrdiff_t fillers = writer - minfill;
  if (fillers > 0) {
    // Tricky: if the fill character is ' ', then it's <fill><+/-><digits>
    // But...: if the fill character is '0', then it's <+/-><fill><digits>
    bool add_sign_again = false;
    if (neg && dec.fill == '0') { // If filling with '0',
      ++writer;                   // ignore the sign we just added
      add_sign_again = true;      // and re-add the sign later.
    }
    writer -= fillers;
    std::fill_n(writer, fillers, dec.fill);
    if (add_sign_again)
      *--writer = '-';
  }

  piece_ = std::wstring_view(writer, end - writer);
}

// ----------------------------------------------------------------------
// StringCat()
//    This merges the given strings or integers, with no delimiter. This
//    is designed to be the fastest possible way to construct a string out
//    of a mix of raw C strings, string_views, strings, and integer values.
// ----------------------------------------------------------------------

// Append is merely a version of memcpy that returns the address of the byte
// after the area just overwritten.
static wchar_t *Append(wchar_t *out, const AlphaNum &x) {
  // memcpy is allowed to overwrite arbitrary memory, so doing this after the
  // call would force an extra fetch of x.size().
  wchar_t *after = out + x.size();
  strings_internal::memcopy(out, x.data(), x.size());
  return after;
}

std::wstring StringCat(const AlphaNum &a, const AlphaNum &b) {
  std::wstring result;
  result.resize(a.size() + b.size());
  wchar_t *const begin = &result[0];
  wchar_t *out = begin;
  out = Append(out, a);
  out = Append(out, b);
  assert(out == begin + result.size());
  return result;
}

std::wstring StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c) {
  std::wstring result;
  result.resize(a.size() + b.size() + c.size());
  wchar_t *const begin = &result[0];
  wchar_t *out = begin;
  out = Append(out, a);
  out = Append(out, b);
  out = Append(out, c);
  assert(out == begin + result.size());
  return result;
}

std::wstring StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d) {
  std::wstring result;
  result.resize(a.size() + b.size() + c.size() + d.size());
  wchar_t *const begin = &result[0];
  wchar_t *out = begin;
  out = Append(out, a);
  out = Append(out, b);
  out = Append(out, c);
  out = Append(out, d);
  assert(out == begin + result.size());
  return result;
}

namespace strings_internal {

// Do not call directly - these are not part of the public API.
std::wstring CatPieces(std::initializer_list<std::wstring_view> pieces) {
  std::wstring result;
  size_t total_size = 0;
  for (const std::wstring_view piece : pieces)
    total_size += piece.size();
  result.resize(total_size);

  wchar_t *const begin = &result[0];
  wchar_t *out = begin;
  for (const std::wstring_view piece : pieces) {
    const size_t this_size = piece.size();
    strings_internal::memcopy(out, piece.data(), this_size);
    out += this_size;
  }
  assert(out == begin + result.size());
  return result;
}

// It's possible to call StrAppend with an std::wstring_view that is itself a
// fragment of the string we're appending to.  However the results of this are
// random. Therefore, check for this in debug mode.  Use unsigned math so we
// only have to do one comparison. Note, there's an exception case: appending an
// empty string is always allowed.
#define ASSERT_NO_OVERLAP(dest, src)                                                               \
  assert(((src).size() == 0) ||                                                                    \
         (uintptr_t((src).data() - (dest).data()) > uintptr_t((dest).size())))

void AppendPieces(std::wstring *dest, std::initializer_list<std::wstring_view> pieces) {
  size_t old_size = dest->size();
  size_t total_size = old_size;
  for (const std::wstring_view piece : pieces) {
    total_size += piece.size();
  }
  dest->resize(total_size);

  wchar_t *const begin = &(*dest)[0];
  wchar_t *out = begin + old_size;
  for (const std::wstring_view piece : pieces) {
    const size_t this_size = piece.size();
    strings_internal::memcopy(out, piece.data(), this_size);
    out += this_size;
  }
  assert(out == begin + dest->size());
}

} // namespace strings_internal

void StrAppend(std::wstring *dest, const AlphaNum &a) {
  ASSERT_NO_OVERLAP(*dest, a);
  dest->append(a.data(), a.size());
}

void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b) {
  ASSERT_NO_OVERLAP(*dest, a);
  ASSERT_NO_OVERLAP(*dest, b);
  std::string::size_type old_size = dest->size();
  dest->resize(old_size + a.size() + b.size());
  wchar_t *const begin = &(*dest)[0];
  wchar_t *out = begin + old_size;
  out = Append(out, a);
  out = Append(out, b);
  assert(out == begin + dest->size());
}

void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c) {
  ASSERT_NO_OVERLAP(*dest, a);
  ASSERT_NO_OVERLAP(*dest, b);
  ASSERT_NO_OVERLAP(*dest, c);
  std::wstring::size_type old_size = dest->size();
  dest->resize(old_size + a.size() + b.size() + c.size());
  wchar_t *const begin = &(*dest)[0];
  wchar_t *out = begin + old_size;
  out = Append(out, a);
  out = Append(out, b);
  out = Append(out, c);
  assert(out == begin + dest->size());
}

void StrAppend(std::wstring *dest, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c,
               const AlphaNum &d) {
  ASSERT_NO_OVERLAP(*dest, a);
  ASSERT_NO_OVERLAP(*dest, b);
  ASSERT_NO_OVERLAP(*dest, c);
  ASSERT_NO_OVERLAP(*dest, d);
  std::wstring::size_type old_size = dest->size();
  dest->resize(old_size + a.size() + b.size() + c.size() + d.size());
  wchar_t *const begin = &(*dest)[0];
  wchar_t *out = begin + old_size;
  out = Append(out, a);
  out = Append(out, b);
  out = Append(out, c);
  out = Append(out, d);
  assert(out == begin + dest->size());
}

} // namespace bela
