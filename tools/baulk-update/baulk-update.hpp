///
#ifndef BAULK_UPDATE_HPP
#define BAULK_UPDATE_HPP
#include <bela/match.hpp>
#include <bela/strip.hpp>
#include <bela/io.hpp>
#include <bela/terminal.hpp>

// https://www.catch22.net/tuts/win32/self-deleting-executables
// https://stackoverflow.com/questions/10319526/understanding-a-self-deleting-program-in-c

namespace baulk::update {
extern bool IsDebugMode;
extern bool IsForceMode;
template <typename... Args> bela::ssize_t DbgPrint(const wchar_t *fmt, const Args &...args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  std::wstring str;
  str.append(L"\x1b[33m* ");
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
  if (str.back() == '\n') {
    str.pop_back();
  }
  str.append(L"\x1b[0m\n");
  return bela::terminal::WriteAuto(stderr, str);
}
inline bela::ssize_t DbgPrint(const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  std::wstring_view msg(fmt);
  if (!msg.empty() && msg.back() == '\n') {
    msg.remove_suffix(1);
  }
  return bela::terminal::WriteAuto(stderr, bela::StringCat(L"\x1b[33m* ", msg, L"\x1b[0m\n"));
}

} // namespace baulk::update

#endif