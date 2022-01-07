///
#ifndef BAULK_EXEC_HPP
#define BAULK_EXEC_HPP
#include <bela/terminal.hpp>
#include <bela/match.hpp>
#include <bela/strip.hpp>
#include <bela/time.hpp>

namespace baulk::exec {
extern bool IsDebugMode;
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

using argv_t = std::vector<std::wstring_view>;
constexpr const wchar_t *string_nullable(std::wstring_view str) { return str.empty() ? nullptr : str.data(); }
constexpr wchar_t *string_nullable(std::wstring &str) { return str.empty() ? nullptr : str.data(); }

inline bool NameEquals(const std::wstring_view arg, const std::wstring_view exe) {
  return bela::EqualsIgnoreCase(bela::StripSuffix(arg, L".exe"), exe);
}

struct Summarizer {
  bela::Time startTime;
  bela::Time endTime;
  bela::Time creationTime;
  bela::Time exitTime;
  bela::Duration kernelTime;
  bela::Duration userTime;
  void SummarizeTime(HANDLE hProcess);
  void PrintTime() const;
};

class Executor {
public:
  Executor() = default;
  Executor(const Executor &) = delete;
  Executor &operator=(const Executor &) = delete;
  bool ParseArgv(int argc, wchar_t **argv);
  int Exec();

private:
  bela::env::Simulator simulator;
  argv_t argv;
  std::wstring cwd;
  bool cleanup{false};
  bool console{true};
  bool summarizeTime{false};
  Summarizer summarizer;
  bool LookPath(std::wstring_view cmd, std::wstring &file);
  bool FindArg0(std::wstring_view arg0, std::wstring &target);
};

} // namespace baulk::exec

#endif