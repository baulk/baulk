/// Env helper
#ifndef BELA_ENV_HPP
#define BELA_ENV_HPP
#include <string>
#include <string_view>
#include <shared_mutex>
#include <span>
#include "match.hpp"
#include "str_split.hpp"
#include "str_join.hpp"
#include "phmap.hpp"
#include "base.hpp"

namespace bela {
constexpr const wchar_t Separator = L';';
constexpr const std::wstring_view Separators = L";";

template <DWORD Len = 256> bool LookupEnv(std::wstring_view key, std::wstring &val) {
  val.resize(Len);
  auto len = GetEnvironmentVariableW(key.data(), val.data(), Len);
  if (len == 0) {
    val.clear();
    return !(GetLastError() == ERROR_ENVVAR_NOT_FOUND);
  }
  if (len < Len) {
    val.resize(len);
    return true;
  }
  val.resize(len);
  auto nlen = GetEnvironmentVariableW(key.data(), val.data(), len);
  if (nlen < len) {
    val.resize(nlen);
  }
  return true;
}

// GetEnv You should set the appropriate size for the initial allocation
// according to your needs.
// https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-getenvironmentvariable
template <DWORD Len = 256> std::wstring GetEnv(std::wstring_view key) {
  std::wstring val;
  bela::LookupEnv<Len>(key, val);
  return val;
}

inline void MakePathEnv(std::vector<std::wstring> &paths) {
  auto systemroot = bela::GetEnv(L"SystemRoot");
  auto system32_env = bela::StringCat(systemroot, L"\\System32");
  paths.emplace_back(system32_env);                             // C:\\Windows\\System32
  paths.emplace_back(systemroot);                               // C:\\Windows
  paths.emplace_back(bela::StringCat(system32_env, L"\\Wbem")); // C:\\Windows\\System32\\Wbem
  // C:\\Windows\\System32\\WindowsPowerShell\\v1.0
  paths.emplace_back(bela::StringCat(system32_env, L"\\WindowsPowerShell\\v1.0"));
}

std::wstring WindowsExpandEnv(std::wstring_view sv);
std::wstring PathUnExpand(std::wstring_view sv);

template <typename Range> std::wstring JoinEnv(const Range &range) {
  return bela::strings_internal::JoinRange(range, Separators);
}

template <typename T> std::wstring JoinEnv(std::initializer_list<T> il) {
  return bela::strings_internal::JoinRange(il, Separators);
}

template <typename... T> std::wstring JoinEnv(const std::tuple<T...> &value) {
  return bela::strings_internal::JoinAlgorithm(value, Separators, AlphaNumFormatter());
}

template <typename... Args> std::wstring InsertEnv(std::wstring_view key, const Args &...arg) {
  std::wstring_view svv[] = {arg...};
  auto prepend = bela::JoinEnv(svv);
  auto val = bela::GetEnv(key);
  return bela::StringCat(prepend, Separators, val);
}

template <typename... Args> std::wstring AppendEnv(std::wstring_view key, const Args &...arg) {
  std::wstring_view svv[] = {arg...};
  auto ended = bela::JoinEnv(svv);
  auto val = bela::GetEnv(key);
  return bela::StringCat(val, Separators, ended);
}

} // namespace bela

#endif
