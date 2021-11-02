#ifndef BAULK_WINDOWS_HPP
#define BAULK_WINDOWS_HPP
#include <bela/base.hpp>
#include <bela/semver.hpp>

namespace baulk::windows {
constexpr bela::version win10_rs5(10, 0, 17763, 0);
constexpr bela::version win10_19h1(10, 0, 18362, 0);
constexpr bela::version win10_19h2(10, 0, 18363, 0);
constexpr bela::version win10_20h1(10, 0, 19041, 0);
constexpr bela::version win10_20h2(10, 0, 19042, 0);
constexpr bela::version win10_21h1(10, 0, 19043, 0);
constexpr bela::version win10_21h2(10, 0, 19044, 0);
constexpr bela::version win11_21h2(10, 0, 22000, 0);
constexpr bela::version server_2022(10, 0, 20348, 0);
inline bela::version version() {
  using ntstatus = LONG;
  constexpr auto status_success = ntstatus(0x00000000);
  ntstatus WINAPI RtlGetVersion(PRTL_OSVERSIONINFOW);
  auto ntdll = ::GetModuleHandleW(L"ntdll.dll");
  if (ntdll == nullptr) {
    return win10_20h1;
  }
  auto rtl_get_version = reinterpret_cast<decltype(RtlGetVersion) *>(GetProcAddress(ntdll, "RtlGetVersion"));
  if (rtl_get_version == nullptr) {
    return win10_20h1;
  }
  RTL_OSVERSIONINFOW rovi = {0};
  rovi.dwOSVersionInfoSize = sizeof(rovi);
  if (rtl_get_version(&rovi) != status_success) {
    return win10_20h1;
  }
  return bela::version(rovi.dwMajorVersion, rovi.dwMinorVersion, rovi.dwBuildNumber, 0);
}

constexpr bool mica_materials_enabled(const bela::version &ver) { return ver >= win11_21h2; }
constexpr bool title_bar_customization_enabled(const bela::version &ver) { return ver >= win10_21h1; }
} // namespace baulk::windows

#endif