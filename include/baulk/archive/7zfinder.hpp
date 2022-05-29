///
#ifndef BAULK_7Z_FINDER_HPP
#define BAULK_7Z_FINDER_HPP
#include <bela/base.hpp>
#include <bela/str_cat.hpp>
#include <bela/strip.hpp>
#include <bela/path.hpp>

namespace baulk::archive {
namespace _7z_ {
inline bool find7z_internal(const wchar_t *K, std::wstring_view cmd, std::wstring &path, bela::error_code &ec) {
  HKEY hkey = nullptr;
  if (RegOpenKeyW(HKEY_LOCAL_MACHINE, K, &hkey) != ERROR_SUCCESS) {
    ec = bela::make_system_error_code();
    return false;
  }
  auto closer = bela::finally([&] { RegCloseKey(hkey); });
  wchar_t buffer[4096];
  DWORD t = 0;
  DWORD sz = sizeof(buffer);
  if (RegQueryValueExW(hkey, L"Path", nullptr, &t, reinterpret_cast<LPBYTE>(buffer), &sz) == ERROR_SUCCESS &&
      t == REG_SZ) {
    if (path = bela::StringCat(bela::StripSuffix(buffer, L"\\"), L"\\", cmd); bela::PathExists(path)) {
      return true;
    }
  }
#ifdef _M_X64
  if (RegQueryValueExW(hkey, L"Path64", nullptr, &t, reinterpret_cast<LPBYTE>(buffer), &sz) == ERROR_SUCCESS &&
      t == REG_SZ) {
    if (path = bela::StringCat(bela::StripSuffix(buffer, L"\\"), L"\\", cmd); bela::PathExists(path)) {
      return true;
    }
  }
#endif
  ec = bela::make_error_code(ERROR_NOT_FOUND, L"'", cmd, L"' not found");
  return false;
}
} // namespace _7z_

inline std::optional<std::wstring> Find7z(std::wstring_view cmd, bela::error_code &ec) {
  std::wstring p;
  if (_7z_::find7z_internal(L"SOFTWARE\\7-Zip", cmd, p, ec)) {
    return std::make_optional(std::move(p));
  }
  if (_7z_::find7z_internal(L"SOFTWARE\\7-Zip-Zstandard", cmd, p, ec)) {
    return std::make_optional(std::move(p));
  }
  return std::nullopt;
}
} // namespace baulk::archive

#endif