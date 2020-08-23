//
#ifndef BAULK_BAULKENV_HPP
#define BAULK_BAULKENV_HPP
#include <bela/path.hpp>
#include <bela/base.hpp>
#include <bela/simulator.hpp>
#include <bela/terminal.hpp>

namespace baulk::env {

#ifdef _M_X64
// Always build x64 binary
[[maybe_unused]] constexpr std::wstring_view HostArch = L"x64"; // Hostx64 x64
#else
[[maybe_unused]] constexpr std::wstring_view HostArch = L"x86"; // Hostx86 x86
#endif

struct BaulkVirtualEnv {
  std::wstring name;
  std::vector<std::wstring> paths;
  std::vector<std::wstring> envs;
  std::vector<std::wstring> includes;
  std::vector<std::wstring> libs;
  std::vector<std::wstring> dependencies;
};

struct Searcher {
  Searcher(bela::env::Simulator &simulator_, std::wstring_view arch_ = HostArch) : simulator{simulator_}, arch(arch_) {
    if (arch.empty()) {
      arch = HostArch;
    }
  }
  using vector_t = std::vector<std::wstring>;
  bela::env::Simulator &simulator;
  std::wstring arch;
  std::wstring baulkroot;
  std::wstring baulkvfs;
  std::wstring baulketc;
  std::wstring baulkbindir;
  vector_t paths;
  vector_t libs;
  vector_t includes;
  vector_t libpaths;
  vector_t availableEnv;
  bool IsDebugMode{false};
  template <typename... Args> bela::ssize_t DbgPrint(const wchar_t *fmt, Args... args) {
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

  template <typename... Args> bela::ssize_t DbgPrintEx(char32_t prefix, const wchar_t *fmt, Args... args) {
    if (!IsDebugMode) {
      return 0;
    }
    const bela::format_internal::FormatArg arg_array[] = {args...};
    auto str = bela::StringCat(L"\x1b[32m* ", prefix, L" ");
    bela::format_internal::StrAppendFormatInternal(&str, fmt, arg_array, sizeof...(args));
    if (str.back() == '\n') {
      str.pop_back();
    }
    str.append(L"\x1b[0m\n");
    return bela::terminal::WriteAuto(stderr, str);
  }
  inline bela::ssize_t DbgPrintEx(char32_t prefix, const wchar_t *fmt) {
    if (!IsDebugMode) {
      return 0;
    }
    std::wstring_view msg(fmt);
    if (!msg.empty() && msg.back() == '\n') {
      msg.remove_suffix(1);
    }
    return bela::terminal::WriteAuto(stderr, bela::StringCat(L"\x1b[32m", prefix, L" ", msg, L"\x1b[0m\n"));
  }
  bool JoinEnvInternal(vector_t &vec, std::wstring &&p) {
    if (bela::PathExists(p)) {
      vec.emplace_back(std::move(p));
      return true;
    }
    return false;
  }
  void JoinForceEnv(vector_t &vec, std::wstring_view p) { vec.emplace_back(std::wstring(p)); }
  void JoinForceEnv(vector_t &vec, std::wstring_view a, std::wstring_view b) {
    vec.emplace_back(bela::StringCat(a, b));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view p) { return JoinEnvInternal(vec, std::wstring(p)); }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b) {
    auto p = bela::StringCat(a, b);
    return JoinEnvInternal(vec, std::wstring(p));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c) {
    auto p = bela::StringCat(a, b, c);
    return JoinEnvInternal(vec, std::wstring(p));
  }
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c, std::wstring_view d) {
    auto p = bela::StringCat(a, b, c, d);
    return JoinEnvInternal(vec, std::wstring(p));
  }
  template <typename... Args>
  bool JoinEnv(vector_t &vec, std::wstring_view a, std::wstring_view b, std::wstring_view c, std::wstring_view d,
               Args... args) {
    auto p = bela::strings_internal::CatPieces({a, b, c, d, args...});
    return JoinEnvInternal(vec, std::wstring(p));
  }
  bool InitializeWindowsKitEnv(bela::error_code &ec);
  bool InitializeVisualStudioEnv(bool clang, bela::error_code &ec);
  bool InitializeBaulk(bela::error_code &ec);
  bool InitializeGit(bool cleanup, bela::error_code &ec);
  bool InitializeVirtualEnv(const std::vector<std::wstring> &venvs, bela::error_code &ec);
  bool FlushEnv();
};

} // namespace baulk::env

#endif