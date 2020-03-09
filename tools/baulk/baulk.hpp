///
#ifndef BAULK_HPP
#define BAULK_HPP
#include <bela/base.hpp>
#include <bela/stdwriter.hpp>

namespace baulk {
extern bool IsDebugMode;
template <typename... Args>
bela::ssize_t DbgPrint(const wchar_t *fmt, Args... args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  auto str =
      bela::format_internal::StrFormatInternal(fmt, arg_array, sizeof...(args));
  return bela::FileWrite(stderr, str);
}
inline bela::ssize_t DbgPrint(const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  return bela::FileWrite(stderr, fmt);
}

struct Package {
  std::wstring name;
  std::wstring description;
  std::wstring url;
  std::wstring version;
  std::vector<std::wstring> links;
  std::vector<std::wstring> launchers;
};
bool MakeLinks(std::wstring_view root, const baulk::Package &pkg,
               bool forceoverwrite, bela::error_code &ec);

std::optional<std::wstring> WinGetInternal(std::wstring_view url,
                                           std::wstring_view workdir,
                                           bool forceoverwrite,
                                           bela::error_code ec);
} // namespace baulk

#endif