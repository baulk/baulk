//
#ifndef BELA_UI_HPP
#define BELA_UI_HPP
#include "base.hpp"
#include "semver.hpp"
#include "color.hpp"

namespace bela::windows {
namespace dwm {
constexpr uint32_t UseHostbackdropbrush = 17;
constexpr uint32_t UseDarkModeBeforce19h2 = 19;
constexpr uint32_t UseImmersiveDarkMode = 20;
constexpr uint32_t WindowCornerPreference = 33;
constexpr uint32_t BorderColor = 34;
constexpr uint32_t CaptionColor = 35;
constexpr uint32_t TextColor = 36;
constexpr uint32_t VisibleFrameBorderThickness = 37;
constexpr uint32_t MicaEffect = 1029;
enum class WindowCorner : int {
  Default = 0,
  DoNotRound = 1,
  Round = 2,
  RoundSmall = 3,
};
} // namespace dwm
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
constexpr int scale_cast(int value, UINT dpi) { return static_cast<int>(static_cast<float>(value) * dpi / 96); }
constexpr auto measure_height(const RECT &rc) { return rc.bottom - rc.top; }
constexpr auto measure_width(const RECT &rc) { return rc.right - rc.left; }

bool SetWindowCornerPreference(HWND hWnd, dwm::WindowCorner corner);
enum PreferredAppMode { Default, AllowDark, ForceDark, ForceLight, Max };
bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam);
void RefreshTitleBarThemeColor(HWND hWnd, bool dark);
void SetDarkModeForWindow(HWND hWnd, PreferredAppMode mode);
bool EnableMicaMaterials(HWND hWnd);
} // namespace bela::windows

#endif