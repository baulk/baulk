//
#ifndef BAULK_REGISTRY_HPP
#define BAULK_REGISTRY_HPP
#include <bela/base.hpp>
#include <bela/match.hpp>
#include <bela/strip.hpp>

namespace baulk::registry {
struct WindowsSDK {
  std::wstring InstallationFolder;
  std::wstring ProductVersion;
};
// HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Microsoft
// SDKs\Windows\v10.0
// InstallationFolder
// ProductVersion

constexpr auto ok = ERROR_SUCCESS;

inline std::optional<std::wstring> GitForWindowsInstallPath(bela::error_code &ec) {
  HKEY hkey = nullptr;
  if (RegOpenKeyW(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\GitForWindows)", &hkey) != ok) {
    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\WOW6432Node\GitForWindows)", &hkey) != ok) {
      if (RegOpenKeyW(HKEY_CURRENT_USER, LR"(SOFTWARE\GitForWindows)", &hkey) != ok) {
        if (RegOpenKeyW(HKEY_CURRENT_USER, LR"(SOFTWARE\WOW6432Node\GitForWindows)", &hkey) != ok) {
          ec = bela::make_system_error_code();
          return std::nullopt;
        }
      }
    }
  }
  auto closer = bela::finally([&] { RegCloseKey(hkey); });
  wchar_t buffer[4096];
  DWORD type = 0;
  DWORD bufsize = sizeof(buffer);
  if (RegQueryValueExW(hkey, L"InstallPath", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufsize) != ok) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (type != REG_SZ) {
    ec = bela::make_error_code(bela::ErrGeneral, L"InstallPath not REG_SZ: ", type);
    return std::nullopt;
  }
  return std::make_optional<std::wstring>(buffer);
}

inline std::optional<WindowsSDK> LookupWindowsSDK(bela::error_code &ec) {
  HKEY hkey = nullptr;
  if (RegOpenKeyW(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0)", &hkey) != ok) {
    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0)", &hkey) != ok) {
      ec = bela::make_system_error_code();
      return std::nullopt;
    }
  }
  auto closer = bela::finally([&] { RegCloseKey(hkey); });
  WindowsSDK winsdk;
  constexpr size_t wlen = 4096;
  wchar_t buffer[wlen] = {0};
  DWORD type = 0;
  DWORD bufsize = sizeof(buffer);
  if (RegQueryValueExW(hkey, L"InstallationFolder", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufsize) != ok) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (type != REG_SZ) {
    ec = bela::make_error_code(bela::ErrGeneral, L"InstallationFolder not REG_SZ: ", type);
    return std::nullopt;
  }
  winsdk.InstallationFolder = bela::StripSuffix(buffer, L"\\");
  bufsize = sizeof(buffer);
  if (RegQueryValueExW(hkey, L"ProductVersion", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufsize) != ok) {
    ec = bela::make_system_error_code();
    return std::nullopt;
  }
  if (type != REG_SZ) {
    ec = bela::make_error_code(bela::ErrGeneral, L"ProductVersion not REG_SIZE: ", type);
    return std::nullopt;
  }
  winsdk.ProductVersion.assign(buffer);
  return std::make_optional(std::move(winsdk));
}

} // namespace baulk::registry

#endif
