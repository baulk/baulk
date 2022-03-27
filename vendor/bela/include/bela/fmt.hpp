/////
#ifndef BELA_FMT_HPP
#define BELA_FMT_HPP
#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include "types.hpp"

// The unix compilers use a 32-bit wchar_t
// The windows compilers, gcc and MSVC, both define a 16 bit wchar_t.
// If you need to port this code to a non-Windows system, please be aware.

namespace bela {
namespace format_internal {
enum class __types {
  __boolean,
  __character,
  __integral, // short,int,
  __unsigned_integral,
  __float,
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
  // %b
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
  requires bela::strict_signed_integral<I> FormatArg(I i) : type(__types::__integral) {
    integer.i = i;
    integer.width = sizeof(I);
  }
  // unsigned integral
  template <typename U>
  requires bela::strict_unsigned_integral<U> FormatArg(U u) : type(__types::__unsigned_integral) {
    integer.i = static_cast<int64_t>(u);
    integer.width = sizeof(U);
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
    u16_strings.store(t.native()); // store native (const std::wstring &)
  }
  template <typename T>
  requires has_string_view<T> FormatArg(const T &t) : type(__types::__u16strings) {
    u16_strings.store(t.string_view()); // store native (const std::wstring &)
  }

  // Any pointer value that can be cast to a "void*".
  template <class T>
  requires bela::not_character<T> FormatArg(T *p) : value(reinterpret_cast<intptr_t>(p)), type(__types::__pointer) {}

  // -------------------------------
  std::u8string_view u8string_view_cast() const { return {u8_strings.data, u8_strings.len}; }
  std::wstring_view u16string_view_cast() const { return {u16_strings.data, u16_strings.len}; }
  std::u32string_view u32string_view_cast() const { return {u32_strings.data, u32_strings.len}; }
  /// Convert To integer
  uint64_t uint64_cast(bool *sign = nullptr) const noexcept {
    switch (type) {
    case __types::__pointer:
      return value;
    case __types::__float: {
      union {
        double d;
        uint64_t i;
      } x;
      x.d = floating.d;
      return x.i;
    }
    case __types::__character: {
      return static_cast<uint32_t>(character.c);
    }
    default:
      break;
    }
    int64_t i = integer.i;
    if (sign != nullptr) {
      if (type == __types::__unsigned_integral || !(*sign = i < 0)) {
        return static_cast<uint64_t>(i);
      }
      if (integer.width == 1) {
        auto i8_ = static_cast<int8_t>(i);
        return static_cast<uint32_t>(0 - i8_);
      }
      if (integer.width == 2) {
        auto i16_ = static_cast<int16_t>(i);
        return static_cast<uint32_t>(0 - i16_);
      }
      if (integer.width == 4) {
        auto i32_ = static_cast<int32_t>(i);
        return static_cast<uint32_t>(0 - i32_);
      }
      return static_cast<uint64_t>(0 - i);
    }
    if (integer.width < sizeof(int64_t)) {
      i &= (1LL << (8 * integer.width)) - 1;
    }
    return static_cast<uint64_t>(i);
  }
  bool is_integral_superset() const {
    return type != __types::__u8strings && type != __types::__u16strings && type != __types::__u32strings;
  }
  union {
    struct {
      int64_t i;
      size_t width;
    } integer;
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
      void store(std::wstring_view sv) {
        data = sv.data();
        len = sv.size();
      }
    } u16_strings;
    struct {
      const char32_t *data;
      size_t len;
    } u32_strings;
    const uintptr_t value;
  };
  const __types type;
};

// Format function
ssize_t StrFormatInternal(wchar_t *buf, size_t N, const wchar_t *fmt, const FormatArg *args, size_t max_args);
std::wstring StrFormatInternal(const wchar_t *fmt, const FormatArg *args, size_t max_args);
size_t StrAppendFormatInternal(std::wstring *buf, const wchar_t *fmt, const FormatArg *args, size_t max_args);
} // namespace format_internal

size_t StrAppendFormat(std::wstring *buf, const wchar_t *fmt);
template <typename... Args> size_t StrAppendFormat(std::wstring *buf, const wchar_t *fmt, const Args &...args) {
  const format_internal::FormatArg arg_array[] = {args...};
  return format_internal::StrAppendFormatInternal(buf, fmt, arg_array, sizeof...(args));
}

template <typename... Args> ssize_t StrFormat(wchar_t *buf, size_t N, const wchar_t *fmt, const Args &...args) {
  const format_internal::FormatArg arg_array[] = {args...};
  return format_internal::StrFormatInternal(buf, N, fmt, arg_array, sizeof...(args));
}

template <size_t N, typename... Args> ssize_t StrFormat(wchar_t (&buf)[N], const wchar_t *fmt, const Args &...args) {
  // Use Arg() object to record type information and then copy arguments to an
  // array to make it easier to iterate over them.
  const format_internal::FormatArg arg_array[] = {args...};
  return format_internal::StrFormatInternal(buf, N, fmt, arg_array, sizeof...(args));
}

template <typename... Args> std::wstring StrFormat(const wchar_t *fmt, const Args &...args) {
  const format_internal::FormatArg arg_array[] = {args...};
  return format_internal::StrFormatInternal(fmt, arg_array, sizeof...(args));
}

// Fast-path when we don't actually need to substitute any arguments.
ssize_t StrFormat(wchar_t *buf, size_t N, const wchar_t *fmt);
std::wstring StrFormat(const wchar_t *fmt);
template <size_t N> inline ssize_t StrFormat(wchar_t (&buf)[N], const wchar_t *fmt) { return StrFormat(buf, N, fmt); }

} // namespace bela

#endif
