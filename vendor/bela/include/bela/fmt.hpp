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

#if defined(_MSC_VER)
// Define ssize_t inside of our namespace.
#if defined(_WIN64)
typedef __int64 __bela__ssize_t;
#else
typedef long __bela__ssize_t;
#endif
#else
typedef long __bela__ssize_t;
#endif

// The unix compilers use a 32-bit wchar_t
// The windows compilers, gcc and MSVC, both define a 16 bit wchar_t.
// If you need to port this code to a non-Windows system, please be aware.

namespace bela {
using ssize_t = __bela__ssize_t;

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
struct FormatArg {
  // %b
  FormatArg(bool b) : at(ArgType::BOOLEAN) {
    character.c = b ? 1 : 0;
    character.width = sizeof(bool);
  }
  // %c
  FormatArg(char c) : at(ArgType::CHARACTER) {
    character.c = c; /// if caset to uint64_t
    character.width = sizeof(char);
  }
  FormatArg(unsigned char c) : at(ArgType::CHARACTER) {
    character.c = c;
    character.width = sizeof(char);
  }
  FormatArg(wchar_t c) : at(ArgType::CHARACTER) {
    character.c = c;
    character.width = sizeof(wchar_t);
  }
  FormatArg(char16_t c) : at(ArgType::CHARACTER) {
    character.c = c;
    character.width = sizeof(char16_t);
  }
  FormatArg(char32_t c) : at(ArgType::CHARACTER) {
    character.c = c;
    character.width = sizeof(char32_t);
  }
  //%d
  FormatArg(signed short j) : at(ArgType::INTEGER) {
    integer.i = j;
    integer.width = sizeof(short);
  }
  FormatArg(unsigned short j) : at(ArgType::UINTEGER) {
    integer.i = j;
    integer.width = sizeof(short);
  }
  FormatArg(signed int j) : at(ArgType::INTEGER) {
    integer.i = j;
    integer.width = sizeof(int);
  }
  FormatArg(unsigned int j) : at(ArgType::UINTEGER) {
    integer.i = j;
    integer.width = sizeof(int);
  }
  FormatArg(signed long j) : at(ArgType::INTEGER) {
    integer.i = j;
    integer.width = sizeof(long);
  }
  FormatArg(unsigned long j) : at(ArgType::UINTEGER) {
    integer.i = j;
    integer.width = sizeof(long);
  }
  FormatArg(signed long long j) : at(ArgType::INTEGER) {
    integer.i = j;
    integer.width = sizeof(long long);
  }
  FormatArg(unsigned long long j) : at(ArgType::UINTEGER) {
    integer.i = j;
    integer.width = sizeof(long long);
  }

  // %f
  FormatArg(float f) : at(ArgType::FLOAT) {
    floating.d = f;
    floating.width = sizeof(float);
  }
  FormatArg(double f) : at(ArgType::FLOAT) {
    floating.d = f;
    floating.width = sizeof(double);
  }
  // wchar_t
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
  // support char16_t under Windows.
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
      unsigned char width;
    } integer;
    struct {
      char32_t c;
      unsigned char width;
    } character;
    struct {
      double d;
      unsigned char width;
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
ssize_t StrFormatInternal(wchar_t *buf, size_t sz, const wchar_t *fmt, const FormatArg *args,
                          size_t max_args);
std::wstring StrFormatInternal(const wchar_t *fmt, const FormatArg *args, size_t max_args);
size_t StrAppendFormatInternal(std::wstring *buf, const wchar_t *fmt, const FormatArg *args,
                               size_t max_args);
} // namespace format_internal

size_t StrAppendFormat(std::wstring *buf, const wchar_t *fmt);
template <typename... Args>
size_t StrAppendFormat(std::wstring *buf, const wchar_t *fmt, Args... args) {
  const format_internal::FormatArg arg_array[] = {args...};
  return format_internal::StrAppendFormatInternal(buf, fmt, arg_array, sizeof...(args));
}

template <typename... Args>
ssize_t StrFormat(wchar_t *buf, size_t N, const wchar_t *fmt, Args... args) {
  const format_internal::FormatArg arg_array[] = {args...};
  return format_internal::StrFormatInternal(buf, N, fmt, arg_array, sizeof...(args));
}

template <size_t N, typename... Args>
ssize_t StrFormat(wchar_t (&buf)[N], const wchar_t *fmt, Args... args) {
  // Use Arg() object to record type information and then copy arguments to an
  // array to make it easier to iterate over them.
  const format_internal::FormatArg arg_array[] = {args...};
  return format_internal::StrFormatInternal(buf, N, fmt, arg_array, sizeof...(args));
}

template <typename... Args> std::wstring StrFormat(const wchar_t *fmt, Args... args) {
  const format_internal::FormatArg arg_array[] = {args...};
  return format_internal::StrFormatInternal(fmt, arg_array, sizeof...(args));
}

// Fast-path when we don't actually need to substitute any arguments.
ssize_t StrFormat(wchar_t *buf, size_t N, const wchar_t *fmt);
std::wstring StrFormat(const wchar_t *fmt);
template <size_t N> inline ssize_t StrFormat(wchar_t (&buf)[N], const wchar_t *fmt) {
  return StrFormat(buf, N, fmt);
}

} // namespace bela

#endif
