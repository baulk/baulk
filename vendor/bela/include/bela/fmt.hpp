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
#include "__format/args.hpp"

// The unix compilers use a 32-bit wchar_t
// The windows compilers, gcc and MSVC, both define a 16 bit wchar_t.
// If you need to port this code to a non-Windows system, please be aware.

namespace bela {
/**
https://en.cppreference.com/w/c/io/fprintf
bela::FPrintF is very different from it, and strives to be type-safe and convenient to use
writes literal %. The full conversion specification must be %%.
b --- boolean (integral !=0)
c --- character (char char8_t char16_t char32_t)
s --- string support:
          std::string/std::string_view
          std::u8string/std::u8string_view
          std::wstring/std::wstring_view
          std::u16string/std::u16string_view
          std::u32string/std::u32string_view
d --- integral (convert to integral)
o --- convert to octal
x --- convert to hex
X --- convert to Upper hex
U --- print unicode endpoint
f --- converts floating-point number to the decimal notation in the style [-]ddd.ddd.
a --- For the conversion style [-]d.ddd±dd is used.
v --- fast convert string and print
p --- pointer printf 0xFFFFFFFFF


introductory character %
(optional) one or more flags that modify the behavior of the conversion:
  -: the result of the conversion is left-justified within the field (by default it is right-justified)
  +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is
preceded by minus only when it is negative)

space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the
result. It is ignored if flag is present. +
  # : alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is
undefined.
  0 : for integer and floating point number conversions, leading zeros are used to pad the field instead of space
characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this
flag results in undefined behavior. It is ignored if flag is present. -
**/

namespace format_internal {
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
