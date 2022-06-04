#ifndef BAULK_GRAPHICS_HPP
#define BAULK_GRAPHICS_HPP
#include <bela/base.hpp>
#include <bela/color.hpp>
#include <bela/win32.hpp>
#include <dwmapi.h>

namespace baulk::windows {
constexpr bool mica_materials_enabled(const bela::windows_version &ver) { return ver >= bela::windows::win11_21h2; }
constexpr bool mica_system_backdrop_enabled(const bela::windows_version &ver) {
  constexpr bela::windows_version win11_22523(10, 0, 22523);
  return ver >= win11_22523;
}
constexpr bool title_bar_customization_enabled(const bela::windows_version &ver) {
  return ver >= bela::windows::win10_21h1;
}

namespace dwm {
// clang-format off
// Window attributes
enum window11_attributes {
  DWMWA_USE_HOSTBACKDROPBRUSH = 17,           // [set] BOOL, Allows the use of host backdrop brushes for the window.
  DWMWA_USE_IMMERSIVE_DARK_MODE = 20,         // [set] BOOL, Allows a window to either use the accent color, or dark, according to the user Color Mode preferences.
  DWMWA_WINDOW_CORNER_PREFERENCE = 33,        // [set] WINDOW_CORNER_PREFERENCE, Controls the policy that rounds top-level window corners
  DWMWA_BORDER_COLOR = 34,                    // [set] COLORREF, The color of the thin border around a top-level window
  DWMWA_CAPTION_COLOR = 35,                   // [set] COLORREF, The color of the caption
  DWMWA_TEXT_COLOR = 36,                      // [set] COLORREF, The color of the caption text
  DWMWA_VISIBLE_FRAME_BORDER_THICKNESS = 37,  // [get] UINT, width of the visible border around a thick frame window
  DWMWA_SYSTEMBACKDROP_TYPE = 38,             // [get, set] SYSTEMBACKDROP_TYPE, Controls the system-drawn backdrop material of a window, including behind the non-client area.
  DWMWA_MICA_EFFECT = 1029,                   // [set] BOOL
};
enum window_corner_preference {
  /*
   * Let the system decide whether or not to round window corners
   */
  DWMWCP_DEFAULT = 0,

  /*
   * Never round window corners
   */
  DWMWCP_DONOTROUND = 1,

  /*
   * Round the corners if appropriate
   */
  DWMWCP_ROUND = 2,

  /*
   * Round the corners if appropriate, with a small radius
   */
  DWMWCP_ROUNDSMALL = 3,

  /*
   * Round the corners of the window in the same style as system popup menus
   */
  DWMWCP_MENU = 4

};

// Types used with DWMWA_SYSTEMBACKDROP_TYPE
enum DWM_SYSTEMBACKDROP_TYPE
{
    DWMSBT_AUTO,             // [Default] Let DWM automatically decide the system-drawn backdrop for this window.
    DWMSBT_NONE,             // Do not draw any system backdrop.
    DWMSBT_MAINWINDOW,       // Draw the backdrop material effect corresponding to a long-lived window.
    DWMSBT_TRANSIENTWINDOW,  // Draw the backdrop material effect corresponding to a transient window.
    DWMSBT_TABBEDWINDOW,     // Draw the backdrop material effect corresponding to a window with a tabbed title bar.
};

// clang-format on

// Use this constant to reset any window part colors to the system default behavior
constexpr auto dwmwa_color_default = 0xFFFFFFFFu;

// Use this constant to specify that a window part should not be rendered
constexpr auto dwmwa_color_none = 0xFFFFFFFEu;

enum DWM_BOOL { DWMWCP_FALSE = 0, DWMWCP_TRUE = 1 };

} // namespace dwm

// PersonalizeMode
struct PersonalizeMode {
  bela::color background;
  bela::color text;
  bela::color border;
  bela::color disable;
};
constexpr auto LightMode = PersonalizeMode{
    .background = bela::color(243, 243, 243),
    .text = bela::color(0, 0, 0),
    .border = bela::color(245, 245, 245),
    .disable = bela::color(77, 77, 77),
};
constexpr auto DarkMode = PersonalizeMode{
    .background = bela::color(26, 31, 52),
    .text = bela::color(255, 255, 255),
    .border = bela::color(51, 52, 54),
    .disable = bela::color(204, 204, 204),
};

constexpr auto CustomMode = PersonalizeMode{
    .background = bela::color(204, 204, 204),
    .text = bela::color(0, 0, 0),
    .border = bela::color(245, 245, 245),
    .disable = bela::color(77, 77, 77),
};

// HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize
// SystemUsesLightTheme  DWORD 0 or 1 // Changes the taskbar and system tray color
// AppsUseLightTheme     DWORD 0 or 1 // Changes app colors

struct PersonalizeThemes {
  PersonalizeMode Mode{LightMode};
  bool SystemUsesLightTheme{true};
  bool AppsUseLightTheme{true};
  bool Load(bela::error_code &ec);
};

inline bool PersonalizeThemes::Load(bela::error_code &ec) {
  HKEY hkey = nullptr;
  auto closer = bela::finally([&] {
    if (hkey != nullptr) {
      RegCloseKey(hkey);
    }
  });
  if (RegOpenKeyW(HKEY_CURRENT_USER, LR"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)", &hkey) != 0) {
    ec = bela::make_system_error_code(L"RegOpenKeyW: ");
    return false;
  }
  DWORD regType = 0;
  DWORD value = 0;
  DWORD bufsize = sizeof(DWORD);
  if (RegQueryValueExW(hkey, L"SystemUsesLightTheme", nullptr, &regType, reinterpret_cast<PBYTE>(&value), &bufsize) !=
      0) {
    ec = bela::make_system_error_code(L"RegQueryValueExW: ");
    return false;
  }
  if (regType != REG_DWORD) {
    ec = bela::make_error_code(bela::ErrGeneral, L"SystemUsesLightTheme not REG_DWORD: ", regType);
    return false;
  }
  SystemUsesLightTheme = (value == 1);
  bufsize = sizeof(DWORD);
  if (RegQueryValueExW(hkey, L"AppsUseLightTheme", nullptr, &regType, reinterpret_cast<PBYTE>(&value), &bufsize) != 0) {
    ec = bela::make_system_error_code(L"AppsUseLightTheme: ");
    return false;
  }
  if (regType != REG_DWORD) {
    ec = bela::make_error_code(bela::ErrGeneral, L"AppsUseLightTheme not REG_DWORD: ", regType);
    return false;
  }
  AppsUseLightTheme = (value == 1);
  if (!AppsUseLightTheme) {
    Mode = DarkMode;
  }
  return true;
}

inline bool PersonalizeWindowUI(HWND hWnd, const PersonalizeThemes &themes) {
  COLORREF captionColor = themes.Mode.background;
  DwmSetWindowAttribute(hWnd, dwm::DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
  COLORREF titleColor = themes.Mode.text;
  DwmSetWindowAttribute(hWnd, dwm::DWMWA_TEXT_COLOR, &titleColor, sizeof(titleColor));
  COLORREF borderColor = themes.Mode.border;
  DwmSetWindowAttribute(hWnd, dwm::DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
  enum DWM_BOOL { DWMWCP_FALSE = 0, DWMWCP_TRUE = 1 };
  DWM_BOOL darkPreference = themes.AppsUseLightTheme ? DWMWCP_FALSE : DWMWCP_FALSE;
  DwmSetWindowAttribute(hWnd, dwm::DWMWA_USE_IMMERSIVE_DARK_MODE, &darkPreference, sizeof(darkPreference));
  return true;
}

inline bool EnableMicaMaterials(HWND hWnd) {
  // DWM_SYSTEMBACKDROP_TYPE
  //
  auto flag = dwm::DWMSBT_MAINWINDOW;
  DwmSetWindowAttribute(hWnd, dwm::DWMWA_SYSTEMBACKDROP_TYPE, &flag, sizeof(flag));
  return true;
}

inline bool EnableRoundWindow(HWND hWnd) {
  auto preference = dwm::DWMWCP_ROUND;
  return DwmSetWindowAttribute(hWnd, dwm::DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference)) == S_OK;
}

inline bool DisableRoundWindow(HWND hWnd) {
  auto preference = dwm::DWMWCP_DONOTROUND;
  return DwmSetWindowAttribute(hWnd, dwm::DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference)) == S_OK;
}

// DPI scale
constexpr int scale_cast(int value, UINT dpi) { return static_cast<int>(static_cast<float>(value) * dpi / 96); }
constexpr auto rect_height(const RECT &rc) { return rc.bottom - rc.top; }
constexpr auto rect_width(const RECT &rc) { return rc.right - rc.left; }
} // namespace baulk::windows

#endif