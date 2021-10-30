//
#include "window.hpp"
#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <VersionHelpers.h>

namespace baulk::unscrew {
// Adopted from:
// https://github.com/oberth/custom-chrome/blob/master/source/gui/window_helper.hpp#L52-L64
inline RECT win32_titlebar_rect(HWND handle) {
  SIZE title_bar_size = {0};
  const int top_and_bottom_borders = 2;
  HTHEME theme = OpenThemeData(handle, L"WINDOW");
  UINT dpi = GetDpiForWindow(handle);
  GetThemePartSize(theme, NULL, WP_CAPTION, CS_ACTIVE, NULL, TS_TRUE, &title_bar_size);
  CloseThemeData(theme);
  int height = ScaleDPI(title_bar_size.cy, dpi) + top_and_bottom_borders;
  RECT rect;
  GetClientRect(handle, &rect);
  rect.bottom = rect.top + height;
  return rect;
}

// Set this to 0 to remove the fake shadow painting
#define WIN32_FAKE_SHADOW_HEIGHT 1

inline RECT win32_fake_shadow_rect(HWND handle) {
  RECT rect;
  GetClientRect(handle, &rect);
  rect.bottom = rect.top + WIN32_FAKE_SHADOW_HEIGHT;
  return rect;
}

inline CustomTitleBarButtonRects win32_get_title_bar_button_rects(HWND handle, const RECT *title_bar_rect) {
  UINT dpi = GetDpiForWindow(handle);
  CustomTitleBarButtonRects button_rects;
  // Sadly SM_CXSIZE does not result in the right size buttons for Win10
  int button_width = ScaleDPI(47, dpi);
  button_rects.close = *title_bar_rect;
  button_rects.close.top += WIN32_FAKE_SHADOW_HEIGHT;

  button_rects.close.left = button_rects.close.right - button_width;
  button_rects.maximize = button_rects.close;
  button_rects.maximize.left -= button_width;
  button_rects.maximize.right -= button_width;
  button_rects.minimize = button_rects.maximize;
  button_rects.minimize.left -= button_width;
  button_rects.minimize.right -= button_width;
  return button_rects;
}

inline bool win32_window_is_maximized(HWND handle) {
  WINDOWPLACEMENT placement = {0};
  placement.length = sizeof(WINDOWPLACEMENT);
  if (GetWindowPlacement(handle, &placement)) {
    return placement.showCmd == SW_SHOWMAXIMIZED;
  }
  return false;
}

inline void win32_center_rect_in_rect(RECT *to_center, const RECT *outer_rect) {
  int to_width = to_center->right - to_center->left;
  int to_height = to_center->bottom - to_center->top;
  int outer_width = outer_rect->right - outer_rect->left;
  int outer_height = outer_rect->bottom - outer_rect->top;

  int padding_x = (outer_width - to_width) / 2;
  int padding_y = (outer_height - to_height) / 2;

  to_center->left = outer_rect->left + padding_x;
  to_center->top = outer_rect->top + padding_y;
  to_center->right = to_center->left + to_width;
  to_center->bottom = to_center->top + to_height;
}

template <typename T> void FreeObj(T *t) {
  if (*t != nullptr) {
    DeleteObject(*t);
    *t = nullptr;
  }
}

MainWindow::MainWindow() {
  //
}

MainWindow::~MainWindow() {
  //
  FreeObj(&hFont);
}

// InitializeMica: only windows 11 or later support mica material
// https://github.com/microsoft/microsoft-ui-xaml/issues/2236
bool MainWindow::InitializeMica() {
  // Mica
  enum DWMWINDOWATTRIBUTE {
    DWMWA_USE_IMMERSIVE_DARK_MODE = 20,
    DWMWA_BORDER_COLOR = 34,
    DWMWA_CAPTION_COLOR = 35,
    DWMWA_VISIBLE_FRAME_BORDER_THICKNESS = 37,
    DWMWA_MICA_EFFECT = 1029
  };

  enum DWM_BOOL { DWMWCP_FALSE = 0, DWMWCP_TRUE = 1 };

  DWM_BOOL value = DWMWCP_TRUE;
  auto color = RGB(243, 243, 243);
  DwmSetWindowAttribute(m_hWnd, DWMWA_CAPTION_COLOR, reinterpret_cast<void *>(&color), sizeof(color));
  COLORREF borderd = RGB(245, 245, 245);
  DwmSetWindowAttribute(m_hWnd, DWMWA_BORDER_COLOR, reinterpret_cast<void *>(&borderd), sizeof(borderd));
  UINT thickness = 2;
  DwmSetWindowAttribute(m_hWnd, DWMWA_VISIBLE_FRAME_BORDER_THICKNESS, reinterpret_cast<void *>(&thickness),
                        sizeof(thickness));
  // Mica
  MARGINS margins = {-1};
  ::DwmExtendFrameIntoClientArea(m_hWnd, &margins);
  // Dark mode
  DWM_BOOL darkPreference = DWMWCP_FALSE;
  DwmSetWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkPreference, sizeof(darkPreference));
  // Mica
  DWM_BOOL micaPreference = DWMWCP_TRUE;
  DwmSetWindowAttribute(m_hWnd, DWMWA_MICA_EFFECT, &micaPreference, sizeof(micaPreference));
  DWM_BOOL hostbackdropbrush = DWMWCP_TRUE;
  DwmSetWindowAttribute(m_hWnd, DWMWA_USE_HOSTBACKDROPBRUSH, &hostbackdropbrush, sizeof(hostbackdropbrush));
  return true;
}

// Handling this event allows us to extend client (paintable) area into the title bar region
// The information is partially coming from:
// https://docs.microsoft.com/en-us/windows/win32/dwm/customframe#extending-the-client-frame
// Most important paragraph is:
//   To remove the standard window frame, you must handle the WM_NCCALCSIZE message,
//   specifically when its wParam value is TRUE and the return value is 0.
//   By doing so, your application uses the entire window region as the client area,
//   removing the standard frame.
LRESULT MainWindow::OnNccalcsize(WPARAM wParam, LPARAM lParam) {
  if (wParam == 0) {
    return DefWindowProc(m_hWnd, WM_NCCALCSIZE, wParam, lParam);
  }
  UINT dpi = GetDpiForWindow(m_hWnd);

  int frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
  int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
  int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

  auto params = reinterpret_cast<NCCALCSIZE_PARAMS *>(lParam);
  RECT *requested_client_rect = params->rgrc;

  requested_client_rect->right -= frame_x + padding;
  requested_client_rect->left += frame_x + padding;
  requested_client_rect->bottom -= frame_y + padding;

  if (win32_window_is_maximized(m_hWnd)) {
    requested_client_rect->top += padding;
  }
  return 0;
}

LRESULT MainWindow::OnNchittest(WPARAM wParam, LPARAM lParam) {
  // Let the default procedure handle resizing areas
  LRESULT hit = DefWindowProcW(m_hWnd, WM_NCHITTEST, wParam, lParam);
  switch (hit) {
  case HTNOWHERE:
    [[fallthrough]];
  case HTRIGHT:
    [[fallthrough]];
  case HTLEFT:
    [[fallthrough]];
  case HTTOPLEFT:
    [[fallthrough]];
  case HTTOP:
    [[fallthrough]];
  case HTTOPRIGHT:
    [[fallthrough]];
  case HTBOTTOMRIGHT:
    [[fallthrough]];
  case HTBOTTOM:
    [[fallthrough]];
  case HTBOTTOMLEFT:
    return hit;
  default:
    break;
  }

  // Looks like adjustment happening in NCCALCSIZE is messing with the detection
  // of the top hit area so manually fixing that.
  UINT dpi = GetDpiForWindow(m_hWnd);
  int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
  int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
  POINT cursor_point = {0};
  cursor_point.x = LOWORD(lParam);
  cursor_point.y = HIWORD(lParam);
  ScreenToClient(m_hWnd, &cursor_point);
  if (cursor_point.y > 0 && cursor_point.y < frame_y + padding) {
    return HTTOP;
  }

  // Since we are drawing our own caption, this needs to be a custom test
  if (cursor_point.y < win32_titlebar_rect(m_hWnd).bottom) {
    return HTCAPTION;
  }

  return HTCLIENT;
}

LRESULT MainWindow::OnNcMouseMove(WPARAM wParam, LPARAM lParam) {
  POINT cursor_point;
  GetCursorPos(&cursor_point);
  ScreenToClient(m_hWnd, &cursor_point);

  RECT title_bar_rect = win32_titlebar_rect(m_hWnd);
  CustomTitleBarButtonRects button_rects = win32_get_title_bar_button_rects(m_hWnd, &title_bar_rect);

  CustomTitleBarHoveredButton new_hovered_button = CustomTitleBarHoveredButton_None;
  if (PtInRect(&button_rects.close, cursor_point)) {
    new_hovered_button = CustomTitleBarHoveredButton_Close;
  } else if (PtInRect(&button_rects.minimize, cursor_point)) {
    new_hovered_button = CustomTitleBarHoveredButton_Minimize;
  } else if (PtInRect(&button_rects.maximize, cursor_point)) {
    new_hovered_button = CustomTitleBarHoveredButton_Maximize;
  }
  if (new_hovered_button != mouseHovered) {
    // You could do tighter invalidation here but probably doesn't matter
    InvalidateRect(m_hWnd, &button_rects.close, FALSE);
    InvalidateRect(m_hWnd, &button_rects.minimize, FALSE);
    InvalidateRect(m_hWnd, &button_rects.maximize, FALSE);
    mouseHovered = new_hovered_button;
  }
  return DefWindowProcW(m_hWnd, WM_NCMOUSEMOVE, wParam, lParam);
}

LRESULT MainWindow::OnCreate(WPARAM wParam, LPARAM lParam) {
  if (isMicaEnabled) {
    InitializeMica();
  }
  RECT size_rect;
  GetWindowRect(m_hWnd, &size_rect);
  // Inform the application of the frame change to force redrawing with the new
  // client area that is extended into the title bar
  SetWindowPos(m_hWnd, NULL, size_rect.left, size_rect.top, size_rect.right - size_rect.left,
               size_rect.bottom - size_rect.top, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
  constexpr const auto ps = WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE;
  MakeWidget(wProgress, PROGRESS_CLASSW, L"", ps, 125, 180, 420, 27, IDC_PROGRESS_BAR);
  return S_OK;
}

LRESULT MainWindow::OnPaint(WPARAM wParam, LPARAM lParam) {
  auto hasFocus = GetFocus() != nullptr;

  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(m_hWnd, &ps);

  // Paint Background
  COLORREF bg_color = RGB(245, 245, 245);
  HBRUSH bg_brush = CreateSolidBrush(bg_color);
  FillRect(hdc, &ps.rcPaint, bg_brush);
  DeleteObject(bg_brush);

  // Paint Title Bar
  HTHEME theme = OpenThemeData(m_hWnd, L"WINDOW");

  COLORREF title_bar_color = RGB(245, 245, 245);
  HBRUSH title_bar_brush = CreateSolidBrush(title_bar_color);
  COLORREF title_bar_hover_color = RGB(130, 180, 160);
  HBRUSH title_bar_hover_brush = CreateSolidBrush(title_bar_hover_color);

  RECT title_bar_rect = win32_titlebar_rect(m_hWnd);

  // Title Bar Background
  FillRect(hdc, &title_bar_rect, title_bar_brush);
  DeleteObject(title_bar_brush);

  COLORREF title_bar_item_color = hasFocus ? RGB(33, 33, 33) : RGB(127, 127, 127);

  HBRUSH button_icon_brush = CreateSolidBrush(title_bar_item_color);
  HPEN button_icon_pen = CreatePen(PS_SOLID, 1, title_bar_item_color);

  CustomTitleBarButtonRects button_rects = win32_get_title_bar_button_rects(m_hWnd, &title_bar_rect);

  UINT dpi = GetDpiForWindow(m_hWnd);
  int icon_dimension = ScaleDPI(10, dpi);

  { // Minimize button
    if (mouseHovered == CustomTitleBarHoveredButton_Minimize) {
      FillRect(hdc, &button_rects.minimize, title_bar_hover_brush);
    }
    RECT icon_rect = {0};
    icon_rect.right = icon_dimension;
    icon_rect.bottom = 1;
    win32_center_rect_in_rect(&icon_rect, &button_rects.minimize);
    FillRect(hdc, &icon_rect, button_icon_brush);
  }

  { // Maximize button
    // if (mouseHovered == CustomTitleBarHoveredButton_Maximize) {
    //   FillRect(hdc, &button_rects.maximize, title_bar_hover_brush);
    // }
    RECT icon_rect = {0};
    icon_rect.right = icon_dimension;
    icon_rect.bottom = icon_dimension;
    win32_center_rect_in_rect(&icon_rect, &button_rects.maximize);
    SelectObject(hdc, button_icon_pen);
    SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, icon_rect.left, icon_rect.top, icon_rect.right, icon_rect.bottom);
  }

  { // Close button
    HPEN custom_pen = 0;
    if (mouseHovered == CustomTitleBarHoveredButton_Close) {
      HBRUSH fill_brush = CreateSolidBrush(RGB(0xCC, 0, 0));
      FillRect(hdc, &button_rects.close, fill_brush);
      DeleteObject(fill_brush);
      custom_pen = CreatePen(PS_SOLID, 1, RGB(0xFF, 0xFF, 0xFF));
      SelectObject(hdc, custom_pen);
    }
    RECT icon_rect = {0};
    icon_rect.right = icon_dimension;
    icon_rect.bottom = icon_dimension;
    win32_center_rect_in_rect(&icon_rect, &button_rects.close);
    MoveToEx(hdc, icon_rect.left, icon_rect.top, NULL);
    LineTo(hdc, icon_rect.right + 1, icon_rect.bottom + 1);
    MoveToEx(hdc, icon_rect.left, icon_rect.bottom, NULL);
    LineTo(hdc, icon_rect.right + 1, icon_rect.top - 1);
    if (custom_pen) {
      DeleteObject(custom_pen);
    }
  }
  DeleteObject(title_bar_hover_brush);
  DeleteObject(button_icon_brush);
  DeleteObject(button_icon_pen);

  // Draw window title
  LOGFONT logical_font;
  HFONT old_font = NULL;
  if (SUCCEEDED(GetThemeSysFont(theme, TMT_CAPTIONFONT, &logical_font))) {
    HFONT theme_font = CreateFontIndirect(&logical_font);
    old_font = (HFONT)SelectObject(hdc, theme_font);
  }

  wchar_t title_text_buffer[255] = {0};
  int buffer_length = sizeof(title_text_buffer) / sizeof(title_text_buffer[0]);
  GetWindowTextW(m_hWnd, title_text_buffer, buffer_length);
  RECT title_bar_text_rect = title_bar_rect;
  // Add padding on the left
  int text_padding = 10; // There seems to be no good way to get this offset
  title_bar_text_rect.left += text_padding;
  // Add padding on the right for the buttons
  title_bar_text_rect.right = button_rects.minimize.left - text_padding;
  DTTOPTS draw_theme_options = {sizeof(draw_theme_options)};
  draw_theme_options.dwFlags = DTT_TEXTCOLOR;
  draw_theme_options.crText = title_bar_item_color;
  DrawThemeTextEx(theme, hdc, 0, 0, title_text_buffer, -1, DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS,
                  &title_bar_text_rect, &draw_theme_options);
  if (old_font) {
    SelectObject(hdc, old_font);
  }
  CloseThemeData(theme);

  // Paint fake top shadow. Original is missing because of the client rect extension.
  // You might need to tweak the colors here based on the color scheme of your app
  // or just remove it if you decide it is not worth it.
  static const COLORREF shadow_color = RGB(100, 100, 100);
  COLORREF fake_top_shadow_color = hasFocus ? shadow_color
                                            : RGB((GetRValue(title_bar_color) + GetRValue(shadow_color)) / 2,
                                                  (GetGValue(title_bar_color) + GetGValue(shadow_color)) / 2,
                                                  (GetBValue(title_bar_color) + GetBValue(shadow_color)) / 2);
  HBRUSH fake_top_shadow_brush = CreateSolidBrush(fake_top_shadow_color);
  RECT fake_top_shadow_rect = win32_fake_shadow_rect(m_hWnd);
  FillRect(hdc, &fake_top_shadow_rect, fake_top_shadow_brush);
  DeleteObject(fake_top_shadow_brush);

  EndPaint(m_hWnd, &ps);
  return S_OK;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
  case WM_NCCALCSIZE:
    return OnNccalcsize(wParam, lParam);
  case WM_CREATE:
    return OnCreate(wParam, lParam);
  case WM_ACTIVATE: {
    RECT title_bar_rect = win32_titlebar_rect(m_hWnd);
    InvalidateRect(m_hWnd, &title_bar_rect, FALSE);
  } break;
  case WM_NCHITTEST:
    return OnNchittest(wParam, lParam);
  case WM_PAINT:
    return OnPaint(wParam, lParam);
  case WM_NCMOUSEMOVE:
    return OnNcMouseMove(wParam, lParam);
  // If the mouse gets into the client area then no title bar buttons are hovered
  // so need to reset the hover state
  case WM_MOUSEMOVE:
    if (mouseHovered != CustomTitleBarHoveredButton_None) {
      RECT title_bar_rect = win32_titlebar_rect(m_hWnd);
      // You could do tighter invalidation here but probably doesn't matter
      InvalidateRect(m_hWnd, &title_bar_rect, FALSE);
      mouseHovered = CustomTitleBarHoveredButton_None;
    }
    break;
  // Handle mouse down and mouse up in the caption area to handle clicks on the buttons
  case WM_NCLBUTTONDOWN:
    // Clicks on buttons will be handled in WM_NCLBUTTONUP, but we still need
    // to remove default handling of the click to avoid it counting as drag.
    //
    // Ideally you also want to check that the mouse hasn't moved out or too much
    // between DOWN and UP messages.
    if (mouseHovered != CustomTitleBarHoveredButton_None) {
      return 0;
    }
    // Default handling allows for dragging and double click to maximize
    break;
  // Map button clicks to the right messages for the window
  case WM_NCLBUTTONUP: {
    if (mouseHovered == CustomTitleBarHoveredButton_Close) {
      PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
      return 0;
    }
    if (mouseHovered == CustomTitleBarHoveredButton_Minimize) {
      ShowWindow(m_hWnd, SW_MINIMIZE);
      return 0;
    }
    break;
  }
  case WM_SETCURSOR: {
    // Show an arrow instead of the busy cursor
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    break;
  }
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  default:
    break;
  }
  return DefWindowProcW(m_hWnd, uMsg, wParam, lParam);
}

} // namespace baulk::unscrew