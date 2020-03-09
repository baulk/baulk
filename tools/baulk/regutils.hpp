//
#ifndef BAULK_REGUTILS_HPP
#define BAULK_REGUTILS_HPP
#include <bela/base.hpp>
#include <bela/finaly.hpp>

namespace baulk::regutils {
struct WindowsSDK {
  std::wstring InstallationFolder;
  std::wstring ProductVersion;
};
// HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Microsoft
// SDKs\Windows\v10.0
// InstallationFolder
// ProductVersion

inline std::optional<WindowsSDK> LookupWindowsSDK(bela::error_code &ec) {
  HKEY hkey = nullptr;
  if (RegOpenKeyW(
          HKEY_LOCAL_MACHINE,
          LR"(SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0)",
          &hkey) != ERROR_SUCCESS) {
    if (RegOpenKeyW(HKEY_LOCAL_MACHINE,
                    LR"(SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0)",
                    &hkey) != ERROR_SUCCESS) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
  }
  auto deleter = bela::finally([&] { RegCloseKey(hkey); });
  WindowsSDK winsdk;
  wchar_t buffer[4096];
  DWORD regtype = 0;
  DWORD bufsize = sizeof(buffer);
  if (RegQueryValueExW(hkey, L"InstallationFolder", nullptr, &regtype,
                       reinterpret_cast<LPBYTE>(buffer),
                       &bufsize) != ERROR_SUCCESS) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (regtype != REG_SZ) {
    ec = bela::make_error_code(1, L"InstallationFolder not REG_SZ: ", regtype);
    return std::nullopt;
  }
  winsdk.InstallationFolder.assign(buffer);
  if (!winsdk.InstallationFolder.empty() &&
      winsdk.InstallationFolder.back() == L'\\') {
    winsdk.InstallationFolder.pop_back();
  }
  bufsize = sizeof(buffer);
  if (RegQueryValueExW(hkey, L"ProductVersion", nullptr, &regtype,
                       reinterpret_cast<LPBYTE>(buffer),
                       &bufsize) != ERROR_SUCCESS) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (regtype != REG_SZ) {
    ec = bela::make_error_code(1, L"ProductVersion not REG_SIZE: ", regtype);
    return std::nullopt;
  }
  winsdk.ProductVersion.assign(buffer);
  return std::make_optional(std::move(winsdk));
}

} // namespace baulk::regutils

#endif