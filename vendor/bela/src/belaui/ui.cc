/// Thanks: https://github.com/ysc3839/win32-darkmode/blob/master/win32-darkmode/DarkMode.h
#include <bela/ui.hpp>
#include <mutex>
#include "loader.hpp"

namespace bela::windows {
namespace {
enum WINDOWCOMPOSITIONATTRIB {
  WCA_UNDEFINED = 0,
  WCA_NCRENDERING_ENABLED = 1,
  WCA_NCRENDERING_POLICY = 2,
  WCA_TRANSITIONS_FORCEDISABLED = 3,
  WCA_ALLOW_NCPAINT = 4,
  WCA_CAPTION_BUTTON_BOUNDS = 5,
  WCA_NONCLIENT_RTL_LAYOUT = 6,
  WCA_FORCE_ICONIC_REPRESENTATION = 7,
  WCA_EXTENDED_FRAME_BOUNDS = 8,
  WCA_HAS_ICONIC_BITMAP = 9,
  WCA_THEME_ATTRIBUTES = 10,
  WCA_NCRENDERING_EXILED = 11,
  WCA_NCADORNMENTINFO = 12,
  WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
  WCA_VIDEO_OVERLAY_ACTIVE = 14,
  WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
  WCA_DISALLOW_PEEK = 16,
  WCA_CLOAK = 17,
  WCA_CLOAKED = 18,
  WCA_ACCENT_POLICY = 19,
  WCA_FREEZE_REPRESENTATION = 20,
  WCA_EVER_UNCLOAKED = 21,
  WCA_VISUAL_OWNER = 22,
  WCA_HOLOGRAPHIC = 23,
  WCA_EXCLUDED_FROM_DDA = 24,
  WCA_PASSIVEUPDATEMODE = 25,
  WCA_USEDARKMODECOLORS = 26,
  WCA_LAST = 27
};

struct WINDOWCOMPOSITIONATTRIBDATA {
  WINDOWCOMPOSITIONATTRIB Attrib;
  PVOID pvData;
  SIZE_T cbData;
};

using HTHEME_ = HANDLE;

enum IMMERSIVE_HC_CACHE_MODE { IHCM_USE_CACHED_VALUE, IHCM_REFRESH };

struct Margins {
  int cxLeftWidth;    // width of left border that retains its size
  int cxRightWidth;   // width of right border that retains its size
  int cyTopHeight;    // height of top border that retains its size
  int cyBottomHeight; // height of bottom border that retains its size
};

using fnDwmSetWindowAttribute = HRESULT(WINAPI *)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
using fnDwmExtendFrameIntoClientArea = HRESULT(WINAPI *)(HWND hWnd, const Margins *pMarInset);
using fnSetWindowCompositionAttribute = BOOL(WINAPI *)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA *);
// 1809 17763
using fnShouldAppsUseDarkMode = bool(WINAPI *)();                                            // ordinal 132
using fnAllowDarkModeForWindow = bool(WINAPI *)(HWND hWnd, bool allow);                      // ordinal 133
using fnAllowDarkModeForApp = bool(WINAPI *)(bool allow);                                    // ordinal 135, in 1809
using fnFlushMenuThemes = void(WINAPI *)();                                                  // ordinal 136
using fnRefreshImmersiveColorPolicyState = void(WINAPI *)();                                 // ordinal 104
using fnIsDarkModeAllowedForWindow = bool(WINAPI *)(HWND hWnd);                              // ordinal 137
using fnGetIsImmersiveColorUsingHighContrast = bool(WINAPI *)(IMMERSIVE_HC_CACHE_MODE mode); // ordinal 106
using fnOpenNcThemeData = HTHEME_(WINAPI *)(HWND hWnd, LPCWSTR pszClassList);                // ordinal 49
// 1903 18362
using fnShouldSystemUseDarkMode = bool(WINAPI *)();                                 // ordinal 138
using fnSetPreferredAppMode = PreferredAppMode(WINAPI *)(PreferredAppMode appMode); // ordinal 135, in 1903
using fnIsDarkModeAllowedForApp = bool(WINAPI *)();                                 // ordinal 139

fnDwmSetWindowAttribute _DwmSetWindowAttribute{nullptr};
fnDwmExtendFrameIntoClientArea _DwmExtendFrameIntoClientArea{nullptr};
fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = nullptr;
fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode = nullptr;
fnAllowDarkModeForWindow _AllowDarkModeForWindow = nullptr;
fnAllowDarkModeForApp _AllowDarkModeForApp = nullptr;
fnFlushMenuThemes _FlushMenuThemes = nullptr;
fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = nullptr;
fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow = nullptr;
fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast = nullptr;
fnOpenNcThemeData _OpenNcThemeData = nullptr;
// 1903 18362
fnShouldSystemUseDarkMode _ShouldSystemUseDarkMode = nullptr;
fnSetPreferredAppMode _SetPreferredAppMode = nullptr;

bela::semver::version system_version;
bool is_mica_materials_enabled{false};
bool is_title_bar_customization_enabled{false};
bool dark_mode_supported{false};
std::once_flag initialize_once;
windows_internal::symbol_loader loader;
void ensureInitialized() {
  system_version = version();
  is_mica_materials_enabled = mica_materials_enabled(system_version);
  is_title_bar_customization_enabled = title_bar_customization_enabled(system_version);
  loader.ensure_system_loaded(_SetWindowCompositionAttribute, L"user32.dll", "SetWindowCompositionAttribute");
  loader.ensure_loaded(_DwmSetWindowAttribute, L"dwmapi.dll", "DwmSetWindowAttribute", true);
  loader.ensure_loaded(_DwmExtendFrameIntoClientArea, L"dwmapi.dll", "DwmExtendFrameIntoClientArea", true);
  loader.ensure_loaded(_OpenNcThemeData, L"uxtheme.dll", MAKEINTRESOURCEA(49));
  loader.ensure_loaded(_RefreshImmersiveColorPolicyState, L"uxtheme.dll", MAKEINTRESOURCEA(104));
  loader.ensure_loaded(_GetIsImmersiveColorUsingHighContrast, L"uxtheme.dll", MAKEINTRESOURCEA(106));
  loader.ensure_loaded(_ShouldAppsUseDarkMode, L"uxtheme.dll", MAKEINTRESOURCEA(132));
  loader.ensure_loaded(_AllowDarkModeForWindow, L"uxtheme.dll", MAKEINTRESOURCEA(133));
  if (system_version.build < 18362) {
    loader.ensure_loaded(_AllowDarkModeForApp, L"uxtheme.dll", MAKEINTRESOURCEA(135));
  } else {
    loader.ensure_loaded(_SetPreferredAppMode, L"uxtheme.dll", MAKEINTRESOURCEA(135));
  }
  loader.ensure_loaded(_IsDarkModeAllowedForWindow, L"uxtheme.dll", MAKEINTRESOURCEA(137));
  AllowDarkModeForApp(true);
  if (_RefreshImmersiveColorPolicyState != nullptr) {
    _RefreshImmersiveColorPolicyState();
  }
}
} // namespace
void EnsureInitialized() {
  std::call_once(initialize_once, [] { ensureInitialized(); });
}

bool IsHighContrast() {
  HIGHCONTRASTW highContrast = {sizeof(highContrast)};
  if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, FALSE)) {
    return highContrast.dwFlags & HCF_HIGHCONTRASTON;
  }
  return false;
}

bool IsColorSchemeChangeMessage(LPARAM lParam) {
  EnsureInitialized();
  if (_RefreshImmersiveColorPolicyState == nullptr || _GetIsImmersiveColorUsingHighContrast == nullptr) {
    return false;
  }
  bool is = false;
  if (lParam &&
      CompareStringOrdinal(reinterpret_cast<LPCWCH>(lParam), -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL) {
    _RefreshImmersiveColorPolicyState();
    is = true;
  }
  _GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
  return is;
}

bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam) {
  if (message == WM_SETTINGCHANGE) {
    return IsColorSchemeChangeMessage(lParam);
  }
  return false;
}

void AllowDarkModeForApp(bool allow) {
  EnsureInitialized();
  if (_AllowDarkModeForApp) {
    _AllowDarkModeForApp(allow);
    return;
  }
  if (_SetPreferredAppMode) {
    _SetPreferredAppMode(allow ? AllowDark : Default);
  }
}

bool AlloDarkModeForWindow(HWND hWnd, bool allow) {
  EnsureInitialized();
  if (_AllowDarkModeForWindow != nullptr) {
    return _AllowDarkModeForWindow(hWnd, allow);
  }
  return false;
}

//     DWMWA_USE_IMMERSIVE_DARK_MODE = 20,         // [set] BOOL, Allows a window to either use the accent color, or
//     dark, according to the user Color Mode preferences.
void RefreshTitleBarThemeColor(HWND hWnd, bool dark) {
  EnsureInitialized();
  if (_DwmSetWindowAttribute == nullptr) {
    return;
  }
  BOOL ldark = dark;
  if (system_version.build >= 20161) {
    // DWMA_USE_IMMERSIVE_DARK_MODE = 20
    _DwmSetWindowAttribute(hWnd, dwm::UseImmersiveDarkMode, &ldark, sizeof(dark));
    return;
  }
  if (system_version.build >= 18363 && _SetWindowCompositionAttribute != nullptr) {
    auto data = WINDOWCOMPOSITIONATTRIBDATA{WCA_USEDARKMODECOLORS, &ldark, sizeof(ldark)};
    _SetWindowCompositionAttribute(hWnd, &data);
    return;
  }
  _DwmSetWindowAttribute(hWnd, 0x13, &ldark, sizeof(ldark));
}

bool SetWindowCornerPreference(HWND hWnd, dwm::WindowCorner corner) {
  EnsureInitialized();
  if (!is_title_bar_customization_enabled || _DwmSetWindowAttribute == nullptr) {
    return true;
  }
  return _DwmSetWindowAttribute(hWnd, dwm::WindowCornerPreference, &corner, sizeof(corner)) == S_OK;
}
bool EnableMicaMaterials(HWND hWnd) {
  EnsureInitialized();
  Margins margins = {-1};
  _DwmExtendFrameIntoClientArea(hWnd, &margins);
  auto micaPreference = TRUE;
  if (_DwmSetWindowAttribute(hWnd, dwm::MicaEffect, &micaPreference, sizeof(micaPreference)) != S_OK) {
    return false;
  }
  return true;
}

} // namespace bela::windows