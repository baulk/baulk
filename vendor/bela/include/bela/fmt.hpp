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
enum class ArgType {
  BOOLEAN,
  CHARACTER,
  INTEGER, // short,int,
  UINTEGER,
  FLOAT,
  STRING,
  USTRING,
  POINTER
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
  FormatArg(bool b) : at(ArgType::BOOLEAN) {
    character.c = b ? 1 : 0;
    character.width = sizeof(bool);
  }
  // character
  template <typename C>
  requires bela::character<C> FormatArg(C c) : at(ArgType::CHARACTER) {
    character.c = c;
    character.width = sizeof(C);
  }
  // signed integral
  template <typename I>
  requires bela::narrowly_signed_integral<I> FormatArg(I i) : at(ArgType::INTEGER) {
    integer.i = i;
    integer.width = sizeof(I);
  }
  // unsigned integral
  template <typename U>
  requires bela::narrowly_unsigned_integral<U> FormatArg(U u) : at(ArgType::UINTEGER) {
    integer.i = static_cast<int64_t>(u);
    integer.width = sizeof(U);
  }
  // float double
  template <typename F>
  requires std::floating_point<F> FormatArg(F f) : at(ArgType::FLOAT) {
    floating.d = f;
    floating.width = sizeof(F);
  }

  // UTF-16 support: wchar_t
  // A C-style text string. and wstring_view
  FormatArg(const wchar_t *str) : at(ArgType::STRING) {
    strings.data = (str == nullptr) ? L"(NULL)" : str;
    strings.len = (str == nullptr) ? sizeof("(NULL)") - 1 : wcslen(str);
  }
  FormatArg(wchar_t *str) : at(ArgType::STRING) {
    strings.data = (str == nullptr) ? L"(NULL)" : str;
    strings.len = (str == nullptr) ? sizeof("(NULL)") - 1 : wcslen(str);
  }
  template <typename Allocator>
  FormatArg( // NOLINT(runtime/explicit)
      const std::basic_string<wchar_t, std::char_traits<wchar_t>, Allocator> &str)
      : at(ArgType::STRING) {
    strings.data = str.data();
    strings.len = str.size();
  }
  FormatArg(std::wstring_view sv) : at(ArgType::STRING) {
    strings.data = sv.data();
    strings.len = sv.size();
  }

  // UTF-16 support: char16_t.
  FormatArg(const char16_t *str) : at(ArgType::STRING) {
    strings.data = (str == nullptr) ? L"(NULL)" : reinterpret_cast<const wchar_t *>(str);
    strings.len = (str == nullptr) ? sizeof("(NULL)") - 1 : wcslen(strings.data);
  }
  FormatArg(char16_t *str) : at(ArgType::STRING) {
    strings.data = (str == nullptr) ? L"(NULL)" : reinterpret_cast<const wchar_t *>(str);
    strings.len = (str == nullptr) ? sizeof("(NULL)") - 1 : wcslen(strings.data);
  }
  template <typename Allocator>
  FormatArg( // NOLINT(runtime/explicit)
      const std::basic_string<char16_t, std::char_traits<char16_t>, Allocator> &str)
      : at(ArgType::STRING) {
    strings.data = reinterpret_cast<const wchar_t *>(str.data());
    strings.len = str.size();
  }
  FormatArg(std::u16string_view sv) : at(ArgType::STRING) {
    strings.data = reinterpret_cast<const wchar_t *>(sv.data());
    strings.len = sv.size();
  }

  // UTF-8 support
  // A C-style text string. and string_view
  FormatArg(const char *str) : at(ArgType::USTRING) {
    ustring.data = (str == nullptr) ? "(NULL)" : str;
    ustring.len = (str == nullptr) ? sizeof("(NULL)") - 1 : strlen(str);
  }
  FormatArg(char *str) : at(ArgType::USTRING) {
    ustring.data = (str == nullptr) ? "(NULL)" : str;
    ustring.len = (str == nullptr) ? sizeof("(NULL)") - 1 : strlen(str);
  }
  template <typename Allocator>
  FormatArg( // NOLINT(runtime/explicit)
      const std::basic_string<char, std::char_traits<char>, Allocator> &str)
      : at(ArgType::USTRING) {
    ustring.data = str.data();
    ustring.len = str.size();
  }
  FormatArg(std::string_view sv) : at(ArgType::USTRING) {
    ustring.data = sv.data();
    ustring.len = sv.size();
  }

  // char8_t support
  FormatArg(const char8_t *str) : at(ArgType::USTRING) {
    ustring.data = (str == nullptr) ? "(NULL)" : reinterpret_cast<const char *>(str);
    ustring.len = (str == nullptr) ? sizeof("(NULL)") - 1 : strlen(reinterpret_cast<const char *>(str));
  }
  FormatArg(char8_t *str) : at(ArgType::USTRING) {
    ustring.data = (str == nullptr) ? "(NULL)" : reinterpret_cast<char *>(str);
    ustring.len = (str == nullptr) ? sizeof("(NULL)") - 1 : strlen(reinterpret_cast<const char *>(str));
  }
  template <typename Allocator>
  FormatArg( // NOLINT(runtime/explicit)
      const std::basic_string<char8_t, std::char_traits<char8_t>, Allocator> &str)
      : at(ArgType::USTRING) {
    ustring.data = reinterpret_cast<const char *>(str.data());
    ustring.len = str.size();
  }
  FormatArg(std::u8string_view sv) : at(ArgType::USTRING) {
    ustring.data = reinterpret_cast<const char *>(sv.data());
    ustring.len = sv.size();
  }
  
  // Extended type support

  template <typename T>
  requires has_native<T> FormatArg(const T &t) : at(ArgType::STRING) {
    strings.data = t.native().data();
    strings.len = t.native().size();
  }
  template <typename T>
  requires has_string_view<T> FormatArg(const T &t) : at(ArgType::STRING) {
    strings.data = t.string_view().data();
    strings.len = t.string_view().size();
  }

  // Any pointer value that can be cast to a "void*".
  template <class T> FormatArg(T *p) : ptr((void *)p), at(ArgType::POINTER) {}

  /// Convert To integer
  uint64_t ToInteger(bool *sign = nullptr) const noexcept {
    switch (at) {
    case ArgType::POINTER:
      return reinterpret_cast<uintptr_t>(ptr);
    case ArgType::FLOAT: {
      union {
        double d;
        uint64_t i;
      } x;
      x.d = floating.d;
      return x.i;
    }
    case ArgType::CHARACTER: {
      return static_cast<uint32_t>(character.c);
    }
    default:
      break;
    }
    int64_t i = integer.i;
    if (sign != nullptr) {
      if (at == ArgType::UINTEGER || !(*sign = i < 0)) {
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
      const wchar_t *data;
      size_t len;
    } strings;
    struct {
      const char *data;
      size_t len;
    } ustring;
    const void *ptr;
  };
  const ArgType at;
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
