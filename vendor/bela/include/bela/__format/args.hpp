///
#ifndef BELA__FORMAT_ARGS_HPP
#define BELA__FORMAT_ARGS_HPP
#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <bela/types.hpp>

namespace bela::format_internal {
enum class __types : wchar_t {
  __boolean,
  __character,
  __float,
  __signed_integral, // short,int,
  __unsigned_integral,
  __u8strings,
  __u16strings,
  __u32strings,
  __pointer,
};

// has_native support
//  bela::error_code
//  std::filesystem::path
template <typename T>
concept has_native = requires(const T &a) {
  { a.native() } -> std::convertible_to<const std::wstring &>;
};

template <typename T>
concept has_string_view = requires(const T &a) {
  { a.string_view() } -> std::convertible_to<std::wstring_view>;
};

struct FormatArg {
  union {
    struct {
      uint64_t i;
      size_t width;
    } unsigned_integral;
    struct {
      int64_t i;
      size_t width;
    } signed_integral;
    struct {
      char32_t c;
      size_t width;
    } character;
    struct {
      double d;
      size_t width;
    } floating;
    struct {
      const char8_t *data;
      size_t len;
    } u8_strings;
    struct {
      const wchar_t *data;
      size_t len;
    } u16_strings;
    struct {
      const char32_t *data;
      size_t len;
    } u32_strings;
    const uintptr_t value;
  };
  const __types type;

  void u16strings_store(std::wstring_view sv) {
    u16_strings.data = sv.data();
    u16_strings.len = sv.size();
  }

  FormatArg(bool b) : type(__types::__boolean) {
    character.c = b ? 1 : 0;
    character.width = sizeof(bool);
  }
  // character
  template <typename C>
  requires bela::character<C> FormatArg(C c) : type(__types::__character) {
    character.c = c;
    character.width = sizeof(C);
  }
  // signed integral
  template <typename I>
  requires bela::strict_signed_integral<I> FormatArg(I i) : type(__types::__signed_integral) {
    signed_integral.i = i;
    signed_integral.width = sizeof(I);
  }
  // unsigned integral
  template <typename U>
  requires bela::strict_unsigned_integral<U> FormatArg(U u) : type(__types::__unsigned_integral) {
    unsigned_integral.i = u;
    unsigned_integral.width = sizeof(U);
  }
  // float double
  template <typename F>
  requires std::floating_point<F> FormatArg(F f) : type(__types::__float) {
    floating.d = f;
    floating.width = sizeof(F);
  }

  // UTF-8
  template <typename C>
  requires bela::u8_character<C> FormatArg(const C *str) : type(__types::__u8strings) {
    u8_strings.data = (str == nullptr) ? u8"(NULL)" : reinterpret_cast<const char8_t *>(str);
    u8_strings.len = (str == nullptr) ? sizeof("(NULL)") - 1 : std::char_traits<C>::length(str);
  }
  template <typename C>
  requires bela::u8_character<C> FormatArg(C *str) : type(__types::__u8strings) {
    u8_strings.data = (str == nullptr) ? u8"(NULL)" : reinterpret_cast<const char8_t *>(str);
    u8_strings.len = (str == nullptr) ? sizeof("(NULL)") - 1 : std::char_traits<C>::length(str);
  }
  // NOLINT(runtime/explicit)
  template <typename C, typename Allocator>
  requires bela::u8_character<C> FormatArg(const std::basic_string<C, std::char_traits<C>, Allocator> &str)
      : type(__types::__u8strings) {
    u8_strings.data = reinterpret_cast<const char8_t *>(str.data());
    u8_strings.len = str.size();
  }
  template <typename C>
  requires bela::u8_character<C> FormatArg(std::basic_string_view<C> sv) : type(__types::__u8strings) {
    u8_strings.data = reinterpret_cast<const char8_t *>(sv.data());
    u8_strings.len = sv.size();
  }

  // UTF-16
  template <typename C>
  requires bela::u16_character<C> FormatArg(const C *str) : type(__types::__u16strings) {
    u16_strings.data = (str == nullptr) ? L"(NULL)" : reinterpret_cast<const wchar_t *>(str);
    u16_strings.len = (str == nullptr) ? sizeof("(NULL)") - 1 : std::char_traits<C>::length(str);
  }
  template <typename C>
  requires bela::u16_character<C> FormatArg(C *str) : type(__types::__u16strings) {
    u16_strings.data = (str == nullptr) ? L"(NULL)" : reinterpret_cast<const wchar_t *>(str);
    u16_strings.len = (str == nullptr) ? sizeof("(NULL)") - 1 : std::char_traits<C>::length(str);
  }
  // NOLINT(runtime/explicit)
  template <typename C, typename Allocator>
  requires bela::u16_character<C> FormatArg(const std::basic_string<C, std::char_traits<C>, Allocator> &str)
      : type(__types::__u16strings) {
    u16_strings.data = reinterpret_cast<const wchar_t *>(str.data());
    u16_strings.len = str.size();
  }
  template <typename C>
  requires bela::u16_character<C> FormatArg(std::basic_string_view<C> sv) : type(__types::__u16strings) {
    u16_strings.data = reinterpret_cast<const wchar_t *>(sv.data());
    u16_strings.len = sv.size();
  }

  // UTF-32
  FormatArg(const char32_t *str) : type(__types::__u32strings) {
    u32_strings.data = (str == nullptr) ? U"(NULL)" : str;
    u32_strings.len = (str == nullptr) ? sizeof("(NULL)") - 1 : std::char_traits<char32_t>::length(str);
  }
  FormatArg(char32_t *str) : type(__types::__u32strings) {
    u32_strings.data = (str == nullptr) ? U"(NULL)" : str;
    u32_strings.len = (str == nullptr) ? sizeof("(NULL)") - 1 : std::char_traits<char32_t>::length(str);
  }
  template <typename Allocator>
  FormatArg(const std::basic_string<char32_t, std::char_traits<char32_t>, Allocator> &str)
      : type(__types::__u32strings) {
    u32_strings.data = str.data();
    u32_strings.len = str.size();
  }
  FormatArg(std::u32string_view sv) : type(__types::__u32strings) {
    u32_strings.data = sv.data();
    u32_strings.len = sv.size();
  }

  // Extended type support
  template <typename T>
  requires has_native<T> FormatArg(const T &t) : type(__types::__u16strings) {
    u16strings_store(t.native()); // store native (const std::wstring &)
  }
  template <typename T>
  requires has_string_view<T> FormatArg(const T &t) : type(__types::__u16strings) {
    u16strings_store(t.string_view()); // store native (const std::wstring &)
  }

  // Any pointer value that can be cast to a "void*".
  template <class T>
  requires bela::not_character<T> FormatArg(T *p) : value(reinterpret_cast<intptr_t>(p)), type(__types::__pointer) {}
  bool is_integral_superset() const {
    return type != __types::__u8strings && type != __types::__u16strings && type != __types::__u32strings;
  }
  auto make_unicode() const noexcept { return character.c; }
  auto make_u8string_view() const noexcept { return std::u8string_view{u8_strings.data, u8_strings.len}; }
  auto make_u16string_view() const noexcept { return std::wstring_view{u16_strings.data, u16_strings.len}; }
  auto make_u32string_view() const noexcept { return std::u32string_view{u32_strings.data, u32_strings.len}; }
  auto make_unsigned_unchecked() const noexcept { return unsigned_integral.i; }
  auto make_unsigned_unchecked(bool &sign) const noexcept {
    if (sign = signed_integral.i < 0; sign) {
      return static_cast<uint64_t>(-signed_integral.i);
    }
    return static_cast<uint64_t>(signed_integral.i);
  }
};

} // namespace bela::format_internal

#endif