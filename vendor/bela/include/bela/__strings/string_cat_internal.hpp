#ifndef BELA__STRINGS_STRING_CAT_HPP
#define BELA__STRINGS_STRING_CAT_HPP
#include <string>
#include <array>
#include <string_view>
#include <bela/types.hpp>
#include <bela/numbers.hpp>
#include <bela/codecvt.hpp>
#include <bela/charconv.hpp>

namespace bela {
namespace strings_internal {

template <typename T> class Literal;
template <> class Literal<char> {
public:
  static constexpr std::string_view StringTrue = "true";
  static constexpr std::string_view StringFalse = "false";
};
template <> class Literal<wchar_t> {
public:
  static constexpr std::wstring_view StringTrue = L"true";
  static constexpr std::wstring_view StringFalse = L"false";
};
template <> class Literal<char16_t> {
public:
  static constexpr std::u16string_view StringTrue = u"true";
  static constexpr std::u16string_view StringFalse = u"false";
};
template <> class Literal<char8_t> {
public:
  static constexpr std::u8string_view StringTrue = u8"true";
  static constexpr std::u8string_view StringFalse = u8"false";
};

template <typename CharT>
requires bela::character<CharT>
auto boolean_string_view(bool v) { return v ? Literal<CharT>::StringTrue : Literal<CharT>::StringFalse; }

template <typename CharT>
requires bela::character<CharT>
auto make_string_view(const CharT *str) {
  using string_view_t = std::basic_string_view<CharT, std::char_traits<CharT>>;
  return (str == nullptr) ? string_view_t() : string_view_t(str);
}

template <typename To>
requires bela::u16_character<To>
auto u16string_view_cast(std::wstring_view sv) {
  using string_view_t = std::basic_string_view<To, std::char_traits<To>>;
  return string_view_t{reinterpret_cast<const To *>(sv.data()), sv.size()};
}

} // namespace strings_internal
// Enum that specifies the number of significant digits to return in a `Hex` or
// `Dec` conversion and fill character to use. A `kZeroPad2` value, for example,
// would produce hexadecimal strings such as "0a","0f" and a 'kSpacePad5' value
// would produce hexadecimal strings such as "    a","    f".
enum PadSpec : uint8_t {
  kNoPad = 1,
  kZeroPad2,
  kZeroPad3,
  kZeroPad4,
  kZeroPad5,
  kZeroPad6,
  kZeroPad7,
  kZeroPad8,
  kZeroPad9,
  kZeroPad10,
  kZeroPad11,
  kZeroPad12,
  kZeroPad13,
  kZeroPad14,
  kZeroPad15,
  kZeroPad16,
  kZeroPad17,
  kZeroPad18,
  kZeroPad19,
  kZeroPad20,

  kSpacePad2 = kZeroPad2 + 64,
  kSpacePad3,
  kSpacePad4,
  kSpacePad5,
  kSpacePad6,
  kSpacePad7,
  kSpacePad8,
  kSpacePad9,
  kSpacePad10,
  kSpacePad11,
  kSpacePad12,
  kSpacePad13,
  kSpacePad14,
  kSpacePad15,
  kSpacePad16,
  kSpacePad17,
  kSpacePad18,
  kSpacePad19,
  kSpacePad20,
};

// -----------------------------------------------------------------------------
// Hex
// -----------------------------------------------------------------------------
//
// `Hex` stores a set of hexadecimal string conversion parameters for use
// within `AlphaNum` string conversions.
struct Hex {
  uint64_t value;
  uint8_t width;
  char fill;

  template <typename Int>
  requires(sizeof(Int) == 1 && !std::is_pointer_v<Int>) constexpr explicit Hex(Int v, PadSpec spec = kNoPad)
      : Hex(spec, static_cast<uint8_t>(v)) {}
  template <typename Int>
  requires(sizeof(Int) == 2 && !std::is_pointer_v<Int>) constexpr explicit Hex(Int v, PadSpec spec = kNoPad)
      : Hex(spec, static_cast<uint16_t>(v)) {}
  template <typename Int>
  requires(sizeof(Int) == 4 && !std::is_pointer_v<Int>) constexpr explicit Hex(Int v, PadSpec spec = kNoPad)
      : Hex(spec, static_cast<uint32_t>(v)) {}
  template <typename Int>
  requires(sizeof(Int) == 8 && !std::is_pointer_v<Int>) constexpr explicit Hex(Int v, PadSpec spec = kNoPad)
      : Hex(spec, static_cast<uint64_t>(v)) {}
  template <typename Pointer>
  requires std::is_pointer_v<Pointer>
  explicit Hex(Pointer *v, PadSpec spec = kNoPad) : Hex(spec, reinterpret_cast<uintptr_t>(v)) {}

private:
  constexpr Hex(PadSpec spec, uint64_t v)
      : value(v), width(spec == kNoPad       ? 1
                        : spec >= kSpacePad2 ? spec - kSpacePad2 + 2
                                             : spec - kZeroPad2 + 2),
        fill(spec >= kSpacePad2 ? ' ' : '0') {}
};

// -----------------------------------------------------------------------------
// Dec
// -----------------------------------------------------------------------------
//
// `Dec` stores a set of decimal string conversion parameters for use
// within `AlphaNum` string conversions.  Dec is slower than the default
// integer conversion, so use it only if you need padding.
struct Dec {
  uint64_t value;
  uint8_t width;
  char fill;
  bool neg;

  template <typename Int>
  requires std::integral<Int>
  constexpr explicit Dec(Int v, PadSpec spec = kNoPad)
      : value(v >= 0 ? static_cast<uint64_t>(v) : uint64_t{0} - static_cast<uint64_t>(v)),
        width(spec == kNoPad       ? 1
              : spec >= kSpacePad2 ? spec - kSpacePad2 + 2
                                   : spec - kZeroPad2 + 2),
        fill(spec >= kSpacePad2 ? ' ' : '0'), neg(v < 0) {}
};

// has_u16_native support
//  bela::error_code
//  std::filesystem::path
template <typename A>
concept has_u16_native = requires(const A &a) {
  { a.native() } -> std::same_as<const std::wstring &>;
};

constexpr int charconv_buffer_size = 32;

template <typename CharT> class basic_alphanum {
public:
  using string_view_t = std::basic_string_view<CharT, std::char_traits<CharT>>;
  template <typename I>
  requires bela::strict_integral<I> basic_alphanum(I x) : piece_(bela::to_chars_view(digits_, x)) {}
  template <typename F>
  requires std::floating_point<F> basic_alphanum(F x)
      : piece_(bela::to_chars_view(digits_, x, std::chars_format::general)) {}
  template <typename From>
  requires bela::compatible_character<From, CharT> basic_alphanum(const From *c_str)
      : piece_(strings_internal::make_string_view(reinterpret_cast<const CharT *>(c_str))) {}
  template <typename From>
  requires bela::compatible_character<From, CharT>
  basic_alphanum(std::basic_string_view<From, std::char_traits<From>> pc)
      : piece_{reinterpret_cast<const CharT *>(pc.data()), pc.size()} {}

  template <typename From, typename Allocator>
  requires bela::compatible_character<From, CharT>
  basic_alphanum(const std::basic_string<From, std::char_traits<From>, Allocator> &str)
      : piece_{reinterpret_cast<const CharT *>(str.data()), str.size()} {}
  template <typename From>
  requires bela::compatible_character<From, CharT> basic_alphanum(From ch) : piece_(digits_, 1) {
    digits_[0] = static_cast<CharT>(ch);
  }
  basic_alphanum(bool b) : piece_{strings_internal::boolean_string_view<CharT>(b)} {}
  basic_alphanum(char32_t ch) : piece_(digits_, bela::encode_into_unchecked(ch, digits_)) {}

  basic_alphanum(Hex hex); // NOLINT(runtime/explicit)
  basic_alphanum(Dec dec); // NOLINT(runtime/explicit)

  basic_alphanum(const basic_alphanum &) = delete;
  basic_alphanum &operator=(const basic_alphanum &) = delete;

  // Extended type support
  // eg: std::filesystem::path
  template <typename E>
  requires(has_u16_native<E> &&std::same_as<CharT, wchar_t>) basic_alphanum(const E &e) : piece_{e.native()} {}
  template <typename E>
  requires(has_u16_native<E> &&std::same_as<CharT, char16_t>) basic_alphanum(const E &e)
      : piece_{strings_internal::u16string_view_cast<CharT>(e.native())} {}

  // Normal enums are already handled by the integer formatters.
  // This overload matches only scoped enums.
  template <typename T>
  requires(std::is_enum_v<T> &&std::integral<std::underlying_type_t<T>>) basic_alphanum(T e)
      : piece_(bela::to_chars_view(digits_, bela::integral_cast(e))) {}

  // vector<bool>::reference and const_reference require special help to
  // convert to `AlphaNum` because it requires two user defined conversions.
  template <typename T>
  requires(std::is_class_v<T> && (std::is_same_v<T, std::vector<bool>::reference> ||
                                  std::is_same_v<T, std::vector<bool>::const_reference>)) basic_alphanum(T e)
      : basic_alphanum(static_cast<bool>(e)) {} // NOLINT(runtime/explicit)

  // template <typename T>
  // requires has_u16_native<T> basic_alphanum(const T &t) : piece_(t.native()) {}

  auto size() const { return piece_.size(); }
  const auto *data() const { return piece_.data(); }
  auto Piece() const { return piece_; }

private:
  string_view_t piece_;
  CharT digits_[charconv_buffer_size];
};

using AlphaNum = basic_alphanum<wchar_t>;
using AlphaNumNarrow = basic_alphanum<char>;

namespace strings_internal {
template <typename CharT, typename Allocator = std::allocator<CharT>>
requires bela::character<CharT>
auto string_cat_pieces(std::initializer_list<std::basic_string_view<CharT, std::char_traits<CharT>>> pieces) {
  std::basic_string<CharT, std::char_traits<CharT>, Allocator> result;
  size_t total_size = 0;
  for (const auto piece : pieces) {
    total_size += piece.size();
  }
  result.resize(total_size);

  auto *const begin = &result[0];
  auto *out = begin;
  for (const auto piece : pieces) {
    const size_t this_size = piece.size();
    memcpy(out, piece.data(), this_size * sizeof(CharT));
    out += this_size;
  }
  assert(out == begin + result.size());
  return result;
}

template <typename CharT, typename Allocator = std::allocator<CharT>>
requires bela::character<CharT>
void string_append_pieces(std::basic_string<CharT, std::char_traits<CharT>, Allocator> *dest,
                          std::initializer_list<std::basic_string_view<CharT, std::char_traits<CharT>>> pieces) {
  size_t old_size = dest->size();
  size_t total_size = old_size;
  for (const auto piece : pieces) {
    total_size += piece.size();
  }
  dest->resize(total_size);

  auto *const begin = &(*dest)[0];
  auto *out = begin + old_size;
  for (const auto piece : pieces) {
    const size_t this_size = piece.size();
    memcpy(out, piece.data(), this_size * sizeof(CharT));
    out += this_size;
  }
  assert(out == begin + dest->size());
}

template <typename CharT>
requires bela::character<CharT> size_t buffer_unchecked_concat(CharT *out, const basic_alphanum<CharT> &a) {
  if (a.size() != 0) {
    memcpy(out, a.data(), a.size() * sizeof(CharT));
  }
  return a.size();
}
} // namespace strings_internal

template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
requires bela::character<CharT>
[[nodiscard]] inline std::basic_string<CharT, std::char_traits<CharT>, Allocator> string_cat() {
  return std::basic_string<CharT, std::char_traits<CharT>, Allocator>();
}

template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
requires bela::character<CharT>
[[nodiscard]] inline std::basic_string<CharT, std::char_traits<CharT>, Allocator>
string_cat(const basic_alphanum<CharT> &a) {
  return std::basic_string<CharT, std::char_traits<CharT>, Allocator>(a.Piece());
}

template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
requires bela::character<CharT>
[[nodiscard]] inline std::basic_string<CharT, std::char_traits<CharT>, Allocator>
string_cat(const basic_alphanum<CharT> &a, const basic_alphanum<CharT> &b) {
  std::basic_string<CharT, std::char_traits<CharT>, Allocator> s;
  s.resize(a.size() + b.size());
  auto out = s.data();
  out += strings_internal::buffer_unchecked_concat(out, a);
  out += strings_internal::buffer_unchecked_concat(out, b);
  return s;
}

template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
requires bela::character<CharT>
[[nodiscard]] inline std::basic_string<CharT, std::char_traits<CharT>, Allocator>
string_cat(const basic_alphanum<CharT> &a, const basic_alphanum<CharT> &b, const basic_alphanum<CharT> &c) {
  std::basic_string<CharT, std::char_traits<CharT>, Allocator> s;
  s.resize(a.size() + b.size() + c.size());
  auto out = s.data();
  out += strings_internal::buffer_unchecked_concat(out, a);
  out += strings_internal::buffer_unchecked_concat(out, b);
  out += strings_internal::buffer_unchecked_concat(out, c);
  return s;
}

template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
requires bela::character<CharT>
[[nodiscard]] inline std::basic_string<CharT, std::char_traits<CharT>, Allocator>
string_cat(const basic_alphanum<CharT> &a, const basic_alphanum<CharT> &b, const basic_alphanum<CharT> &c,
           const basic_alphanum<CharT> &d) {
  std::basic_string<CharT, std::char_traits<CharT>, Allocator> s;
  s.resize(a.size() + b.size() + c.size() + d.size());
  auto out = s.data();
  out += strings_internal::buffer_unchecked_concat(out, a);
  out += strings_internal::buffer_unchecked_concat(out, b);
  out += strings_internal::buffer_unchecked_concat(out, c);
  out += strings_internal::buffer_unchecked_concat(out, d);
  return s;
}

template <typename CharT, typename Allocator, typename... AV>
requires bela::character<CharT>
[[nodiscard]] inline auto string_cat_with_allocator(const basic_alphanum<CharT> &a, const basic_alphanum<CharT> &b,
                                                    const basic_alphanum<CharT> &c, const basic_alphanum<CharT> &d,
                                                    const basic_alphanum<CharT> &e, const AV &...args) {
  return strings_internal::string_cat_pieces<CharT, Allocator>(
      {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(),
       static_cast<const basic_alphanum<CharT> &>(args).Piece()...});
}

template <typename CharT, typename... AV>
requires bela::character<CharT>
[[nodiscard]] inline auto string_cat(const basic_alphanum<CharT> &a, const basic_alphanum<CharT> &b,
                                     const basic_alphanum<CharT> &c, const basic_alphanum<CharT> &d,
                                     const basic_alphanum<CharT> &e, const AV &...args) {
  return strings_internal::string_cat_pieces({a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(),
                                              static_cast<const basic_alphanum<CharT> &>(args).Piece()...});
}

template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
requires bela::character<CharT>
void string_append(std::basic_string<CharT, std::char_traits<CharT>, Allocator> *dest, const basic_alphanum<CharT> &a) {
  dest->append(a.Piece());
}

template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
requires bela::character<CharT>
void string_append(std::basic_string<CharT, std::char_traits<CharT>, Allocator> *dest, const basic_alphanum<CharT> &a,
                   const basic_alphanum<CharT> &b) {
  auto oldsize = dest->size();
  dest->resize(oldsize + a.size() + b.size());
  auto out = dest->data() + oldsize;
  out += strings_internal::buffer_unchecked_concat(out, a);
  out += strings_internal::buffer_unchecked_concat(out, b);
}

template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
requires bela::character<CharT>
void string_append(std::basic_string<CharT, std::char_traits<CharT>, Allocator> *dest, const basic_alphanum<CharT> &a,
                   const basic_alphanum<CharT> &b, const basic_alphanum<CharT> &c) {
  auto oldsize = dest->size();
  dest->resize(oldsize + a.size() + b.size() + c.size());
  auto out = dest->data() + oldsize;
  out += strings_internal::buffer_unchecked_concat(out, a);
  out += strings_internal::buffer_unchecked_concat(out, b);
  out += strings_internal::buffer_unchecked_concat(out, c);
}

template <typename CharT = wchar_t, typename Allocator = std::allocator<CharT>>
requires bela::character<CharT>
void string_append(std::basic_string<CharT, std::char_traits<CharT>, Allocator> *dest, const basic_alphanum<CharT> &a,
                   const basic_alphanum<CharT> &b, const basic_alphanum<CharT> &c, const basic_alphanum<CharT> &d) {
  auto oldsize = dest->size();
  dest->resize(oldsize + a.size() + b.size() + c.size() + d.size());
  auto out = dest->data() + oldsize;
  out += strings_internal::buffer_unchecked_concat(out, a);
  out += strings_internal::buffer_unchecked_concat(out, b);
  out += strings_internal::buffer_unchecked_concat(out, c);
  out += strings_internal::buffer_unchecked_concat(out, d);
}

template <typename CharT, typename Allocator, typename... AV>
inline void string_append_with_allocator(std::basic_string<CharT, std::char_traits<CharT>, Allocator> *dest,
                                         const basic_alphanum<CharT> &a, const basic_alphanum<CharT> &b,
                                         const basic_alphanum<CharT> &c, const basic_alphanum<CharT> &d,
                                         const basic_alphanum<CharT> &e, const AV &...args) {
  strings_internal::string_append_pieces<wchar_t>(dest, {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(),
                                                         static_cast<const basic_alphanum<CharT> &>(args).Piece()...});
}

} // namespace bela

#endif