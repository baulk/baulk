///
#ifndef BAULK_HPP
#define BAULK_HPP
#include <bela/base.hpp>
#include <bela/stdwriter.hpp>

namespace baulk {
extern bool IsDebugMode;
constexpr size_t UerAgentMaximumLength = 64;
extern wchar_t UserAgent[UerAgentMaximumLength];
template <typename... Args>
bela::ssize_t DbgPrint(const wchar_t *fmt, Args... args) {
  if (!IsDebugMode) {
    return 0;
  }
  const bela::format_internal::FormatArg arg_array[] = {args...};
  std::wstring str;
  str.append(L"\x1b[33m");
  bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array,
                                                 sizeof...(args));
  str.append(L"\x1b[0m");
  return bela::FileWrite(stderr, str);
}
inline bela::ssize_t DbgPrint(const wchar_t *fmt) {
  if (!IsDebugMode) {
    return 0;
  }
  return bela::FileWrite(stderr, bela::StringCat(L"\x1b[33m", fmt, L"\x1b[0m"));
}

// Env functions
bool InitializeBaulkEnv(int argc, wchar_t *const *argv,
                        std::wstring_view profile);
std::wstring_view BaulkRoot();
std::wstring_view BaulkBucketUrl();

// package base
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