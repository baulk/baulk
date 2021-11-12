//
#ifndef BELA_UI_HPP
#define BELA_UI_HPP
#include "base.hpp"
#include "color.hpp"
#include "win32.hpp"

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

constexpr bool mica_materials_enabled(const bela::windows_version &ver) { return ver >= win11_21h2; }
constexpr bool title_bar_customization_enabled(const bela::windows_version &ver) { return ver >= win10_21h1; }
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