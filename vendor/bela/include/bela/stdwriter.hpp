/////////
#ifndef BELA_STDWRITER_HPP
#define BELA_STDWRITER_HPP
#pragma once
#include <cstdio>
#include <string_view>
#include <string>
#include "fmt.hpp"

namespace bela {
//
ssize_t FileWrite(FILE *out, std::wstring_view msg);
template <typename... Args>
ssize_t FPrintF(FILE *out, const wchar_t *fmt, Args... args) {
  const format_internal::FormatArg arg_array[] = {args...};
  auto str =
      format_internal::StrFormatInternal(fmt, arg_array, sizeof...(args));
  return FileWrite(out, str);
}

inline ssize_t FPrintF(FILE *out, const wchar_t *fmt) {
  auto str = StrFormat(fmt);
  return FileWrite(out, str);
}
std::wstring FileTypeName(FILE *file);
// IsTerminal include console PTY and cygwin pipe
bool IsTerminal(FILE *fd);
bool IsCygwinTerminal(FILE *fd);
uint32_t TerminalWidth();
} // namespace bela

#endif
