#ifndef BAULK_WINDOWS_HPP
#define BAULK_WINDOWS_HPP
#include <bela/base.hpp>
#include <bela/semver.hpp>

namespace baulk::windows {
constexpr bela::version windows10_2004(10, 0, 19041, 0);
constexpr bela::version windows10_20h2(10, 0, 19042, 0);
constexpr bela::version windows10_21h1(10, 0, 19043, 0);
constexpr bela::version windows11_21h2(10, 0, 22000, 0);
inline bela::version GetWindowsVersion() {
  using ntstatus = LONG;
  constexpr auto status_success = ntstatus(0x00000000);
  ntstatus WINAPI RtlGetVersion(PRTL_OSVERSIONINFOW);
  auto ntdll = ::GetModuleHandleW(L"ntdll.dll");
  if (ntdll == nullptr) {
    return windows10_2004;
  }
  auto rtl_get_version = reinterpret_cast<decltype(RtlGetVersion) *>(GetProcAddress(ntdll, "RtlGetVersion"));
  if (rtl_get_version == nullptr) {
    return windows10_2004;
  }
  RTL_OSVERSIONINFOW rovi = {0};
  rovi.dwOSVersionInfoSize = sizeof(rovi);
  if (rtl_get_version(&rovi) != status_success) {
    return windows10_2004;
  }
  return bela::version(rovi.dwMajorVersion, rovi.dwMinorVersion, rovi.dwBuildNumber, 0);
}

constexpr bool mica_materials_enabled(const bela::version &osversion) { return osversion >= windows10_21h1; }
constexpr bool title_bar_customization_enabled(const bela::version &osversion) { return osversion >= windows10_21h1; }
} // namespace baulk::windows

#endif