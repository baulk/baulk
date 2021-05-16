//
#ifndef BELA_TERMINAL_HPP
#define BELA_TERMINAL_HPP
#include "base.hpp"
#include "fmt.hpp"

namespace bela {
namespace terminal {
//

// Is same terminal maybe console or Cygwin pty
bool IsSameTerminal(HANDLE fd);
bool IsSameTerminal(FILE *fd);

// Is console terminal
bool IsTerminal(HANDLE fd);
bool IsTerminal(FILE *fd);

// Is cygwin terminal
bool IsCygwinTerminal(HANDLE fd);
bool IsCygwinTerminal(FILE *fd);

struct terminal_size {
  uint32_t columns{0};
  uint32_t rows{0};
};
bool TerminalSize(HANDLE fd, terminal_size &sz);
bool TerminalSize(FILE *fd, terminal_size &sz);
bela::ssize_t WriteTerminal(HANDLE fd, std::wstring_view data);
bela::ssize_t WriteSameFile(HANDLE fd, std::wstring_view data);
bela::ssize_t WriteSameFile(HANDLE fd, std::string_view data);
// Warning, currently bela :: terminal :: WriteAuto does not support redirect
// operations like freopen, please do not use this
bela::ssize_t WriteAuto(FILE *fd, std::wstring_view data);
// UTF-8 version cache write
bela::ssize_t WriteAuto(FILE *fd, std::string_view data);
// If you need to call freopen, you should use the following:
bela::ssize_t WriteAutoFallback(FILE *fd, std::wstring_view data);
// UTF-8 version direct write
bela::ssize_t WriteAutoFallback(FILE *fd, std::string_view data);
} // namespace terminal
template <typename... Args> ssize_t FPrintF(FILE *out, const wchar_t *fmt, const Args &...args) {
  const format_internal::FormatArg arg_array[] = {args...};
  auto str = format_internal::StrFormatInternal(fmt, arg_array, sizeof...(args));
  return bela::terminal::WriteAuto(out, str);
}

inline ssize_t FPrintF(FILE *out, const wchar_t *fmt) {
  auto str = StrFormat(fmt);
  return bela::terminal::WriteAuto(out, str);
}

template <typename... Args> ssize_t FPrintFallbackF(FILE *out, const wchar_t *fmt, const Args &...args) {
  const format_internal::FormatArg arg_array[] = {args...};
  auto str = format_internal::StrFormatInternal(fmt, arg_array, sizeof...(args));
  return bela::terminal::WriteAutoFallback(out, str);
}

inline ssize_t FPrintFallbackF(FILE *out, const wchar_t *fmt) {
  auto str = StrFormat(fmt);
  return bela::terminal::WriteAutoFallback(out, str);
}

} // namespace bela
#endif