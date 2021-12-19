#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <dwmapi.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#include <stdbool.h>

static LRESULT
win32_custom_title_bar_example_window_callback(HWND handle, UINT message, WPARAM w_param,
                                               LPARAM l_param); // Implementation is at the end of the file

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int nCmdShow) {
  // Support high-dpi screens
  if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
    OutputDebugStringW(L"WARNING: could not set DPI awareness");
  }

  // Register a "Window Class"
  static const wchar_t *window_class_name = L"WIN32_CUSTOM_TITLEBAR_EXAMPLE";
  WNDCLASSEXW window_class = {0};
  {
    window_class.cbSize = sizeof(window_class);
    window_class.lpszClassName = window_class_name;
    // Set the procedure that will receive window messages (events)
    window_class.lpfnWndProc = win32_custom_title_bar_example_window_callback;
    // Ask to send WM_PAINT when resizing horizontally and vertically
    window_class.style = CS_HREDRAW | CS_VREDRAW;
  }
  RegisterClassExW(&window_class);

  int window_style = WS_THICKFRAME    // required for a standard resizeable window
                     | WS_SYSMENU     // Explicitly ask for the titlebar to support snapping via Win + ← / Win + →
                     | WS_MAXIMIZEBOX // Add maximize button to support maximizing via mouse dragging
                                      // to the top of the screen
                     | WS_MINIMIZEBOX // Add minimize button to support minimizing by clicking on the taskbar icon
                     | WS_VISIBLE;    // Make window visible after it is created (not important)
  CreateWindowExW(WS_EX_APPWINDOW, window_class_name, L"❤️ Extract baulk-win64-4.0.zip",
                  // The
                  window_style, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, 0, 0, 0, 0);

  // Run message loop
  for (MSG message = {0};;) {
    BOOL result = GetMessageW(&message, 0, 0, 0);
    if (result > 0) {
      TranslateMessage(&message);
      DispatchMessageW(&message);
    } else {
      break;
    }
  }

  return 0;
}

static int win32_dpi_scale(int value, UINT dpi) { return (int)((float)value * dpi / 96); }

// Adopted from:
// https://github.com/oberth/custom-chrome/blob/master/source/gui/window_helper.hpp#L52-L64
static RECT win32_titlebar_rect(HWND handle) {
  SIZE title_bar_size = {0};
  const int top_and_bottom_borders = 2;
  HTHEME theme = OpenThemeData(handle, L"WINDOW");
  UINT dpi = GetDpiForWindow(handle);
  GetThemePartSize(theme, NULL, WP_CAPTION, CS_ACTIVE, NULL, TS_TRUE, &title_bar_size);
  CloseThemeData(theme);

  int height = win32_dpi_scale(title_bar_size.cy, dpi) + top_and_bottom_borders;

  RECT rect;
  GetClientRect(handle, &rect);
  rect.bottom = rect.top + height;
  return rect;
}

// Set this to 0 to remove the fake shadow painting
#define WIN32_FAKE_SHADOW_HEIGHT 1

static RECT win32_fake_shadow_rect(HWND handle) {
  RECT rect;
  GetClientRect(handle, &rect);
  rect.bottom = rect.top + WIN32_FAKE_SHADOW_HEIGHT;
  return rect;
}

typedef struct {
  RECT close;
  RECT maximize;
  RECT minimize;
} CustomTitleBarButtonRects;

typedef enum {
  CustomTitleBarHoveredButton_None,
  CustomTitleBarHoveredButton_Minimize,
  CustomTitleBarHoveredButton_Maximize,
  CustomTitleBarHoveredButton_Close,
} CustomTitleBarHoveredButton;

static CustomTitleBarButtonRects win32_get_title_bar_button_rects(HWND handle, const RECT *title_bar_rect) {
  UINT dpi = GetDpiForWindow(handle);
  CustomTitleBarButtonRects button_rects;
  // Sadly SM_CXSIZE does not result in the right size buttons for Win10
  int button_width = win32_dpi_scale(47, dpi);
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

static bool win32_window_is_maximized(HWND handle) {
  WINDOWPLACEMENT placement = {0};
  placement.length = sizeof(WINDOWPLACEMENT);
  if (GetWindowPlacement(handle, &placement)) {
    return placement.showCmd == SW_SHOWMAXIMIZED;
  }
  return false;
}

static void win32_center_rect_in_rect(RECT *to_center, const RECT *outer_rect) {
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

static LRESULT win32_custom_title_bar_example_window_callback(HWND handle, UINT message, WPARAM w_param,
                                                              LPARAM l_param) {
  CustomTitleBarHoveredButton title_bar_hovered_button =
      (CustomTitleBarHoveredButton)GetWindowLongPtrW(handle, GWLP_USERDATA);

  switch (message) {
  // Handling this event allows us to extend client (paintable) area into the title bar region
  // The information is partially coming from:
  // https://docs.microsoft.com/en-us/windows/win32/dwm/customframe#extending-the-client-frame
  // Most important paragraph is:
  //   To remove the standard window frame, you must handle the WM_NCCALCSIZE message,
  //   specifically when its wParam value is TRUE and the return value is 0.
  //   By doing so, your application uses the entire window region as the client area,
  //   removing the standard frame.
  case WM_NCCALCSIZE: {
    if (!w_param)
      return DefWindowProc(handle, message, w_param, l_param);
    UINT dpi = GetDpiForWindow(handle);

    int frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
    int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
    int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

    NCCALCSIZE_PARAMS *params = (NCCALCSIZE_PARAMS *)l_param;
    RECT *requested_client_rect = params->rgrc;

    requested_client_rect->right -= frame_x + padding;
    requested_client_rect->left += frame_x + padding;
    requested_client_rect->bottom -= frame_y + padding;

    if (win32_window_is_maximized(handle)) {
      requested_client_rect->top += padding;
    }

    return 0;
  }
  case WM_CREATE: {
    RECT size_rect;
    GetWindowRect(handle, &size_rect);

    // Inform the application of the frame change to force redrawing with the new
    // client area that is extended into the title bar
    SetWindowPos(handle, NULL, size_rect.left, size_rect.top, size_rect.right - size_rect.left,
                 size_rect.bottom - size_rect.top, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
    break;
  }
  case WM_ACTIVATE: {
    RECT title_bar_rect = win32_titlebar_rect(handle);
    InvalidateRect(handle, &title_bar_rect, FALSE);
    return DefWindowProc(handle, message, w_param, l_param);
  }
  case WM_NCHITTEST: {
    // Let the default procedure handle resizing areas
    LRESULT hit = DefWindowProc(handle, message, w_param, l_param);
    switch (hit) {
    case HTNOWHERE:
    case HTRIGHT:
    case HTLEFT:
    case HTTOPLEFT:
    case HTTOP:
    case HTTOPRIGHT:
    case HTBOTTOMRIGHT:
    case HTBOTTOM:
    case HTBOTTOMLEFT: {
      return hit;
    }
    }

    // Looks like adjustment happening in NCCALCSIZE is messing with the detection
    // of the top hit area so manually fixing that.
    UINT dpi = GetDpiForWindow(handle);
    int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
    int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    POINT cursor_point = {0};
    cursor_point.x = LOWORD(l_param);
    cursor_point.y = HIWORD(l_param);
    ScreenToClient(handle, &cursor_point);
    if (cursor_point.y > 0 && cursor_point.y < frame_y + padding) {
      return HTTOP;
    }

    // Since we are drawing our own caption, this needs to be a custom test
    if (cursor_point.y < win32_titlebar_rect(handle).bottom) {
      return HTCAPTION;
    }

    return HTCLIENT;
  }
  case WM_PAINT: {
    bool has_focus = !!GetFocus();

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(handle, &ps);

    // Paint Background
    COLORREF bg_color = RGB(245, 245, 245);
    HBRUSH bg_brush = CreateSolidBrush(bg_color);
    FillRect(hdc, &ps.rcPaint, bg_brush);
    DeleteObject(bg_brush);

    // Paint Title Bar
    HTHEME theme = OpenThemeData(handle, L"WINDOW");

    COLORREF title_bar_color = RGB(245, 245, 245);
    HBRUSH title_bar_brush = CreateSolidBrush(title_bar_color);
    COLORREF title_bar_hover_color = RGB(130, 180, 160);
    HBRUSH title_bar_hover_brush = CreateSolidBrush(title_bar_hover_color);

    RECT title_bar_rect = win32_titlebar_rect(handle);

    // Title Bar Background
    FillRect(hdc, &title_bar_rect, title_bar_brush);
    DeleteObject(title_bar_brush);

    COLORREF title_bar_item_color = has_focus ? RGB(33, 33, 33) : RGB(127, 127, 127);

    HBRUSH button_icon_brush = CreateSolidBrush(title_bar_item_color);
    HPEN button_icon_pen = CreatePen(PS_SOLID, 1, title_bar_item_color);

    CustomTitleBarButtonRects button_rects = win32_get_title_bar_button_rects(handle, &title_bar_rect);

    UINT dpi = GetDpiForWindow(handle);
    int icon_dimension = win32_dpi_scale(10, dpi);

    { // Minimize button
      if (title_bar_hovered_button == CustomTitleBarHoveredButton_Minimize) {
        FillRect(hdc, &button_rects.minimize, title_bar_hover_brush);
      }
      RECT icon_rect = {0};
      icon_rect.right = icon_dimension;
      icon_rect.bottom = 1;
      win32_center_rect_in_rect(&icon_rect, &button_rects.minimize);
      FillRect(hdc, &icon_rect, button_icon_brush);
    }

    { // Maximize button
      if (title_bar_hovered_button == CustomTitleBarHoveredButton_Maximize) {
        FillRect(hdc, &button_rects.maximize, title_bar_hover_brush);
      }
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
      if (title_bar_hovered_button == CustomTitleBarHoveredButton_Close) {
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
      if (custom_pen)
        DeleteObject(custom_pen);
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
    GetWindowTextW(handle, title_text_buffer, buffer_length);
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
    if (old_font)
      SelectObject(hdc, old_font);
    CloseThemeData(theme);

    // Paint fake top shadow. Original is missing because of the client rect extension.
    // You might need to tweak the colors here based on the color scheme of your app
    // or just remove it if you decide it is not worth it.
    static const COLORREF shadow_color = RGB(100, 100, 100);
    COLORREF fake_top_shadow_color = has_focus ? shadow_color
                                               : RGB((GetRValue(title_bar_color) + GetRValue(shadow_color)) / 2,
                                                     (GetGValue(title_bar_color) + GetGValue(shadow_color)) / 2,
                                                     (GetBValue(title_bar_color) + GetBValue(shadow_color)) / 2);
    HBRUSH fake_top_shadow_brush = CreateSolidBrush(fake_top_shadow_color);
    RECT fake_top_shadow_rect = win32_fake_shadow_rect(handle);
    FillRect(hdc, &fake_top_shadow_rect, fake_top_shadow_brush);
    DeleteObject(fake_top_shadow_brush);

    EndPaint(handle, &ps);
    break;
  }
  // Track when mouse hovers each of the title bar buttons to draw the highlight correctly
  case WM_NCMOUSEMOVE: {
    POINT cursor_point;
    GetCursorPos(&cursor_point);
    ScreenToClient(handle, &cursor_point);

    RECT title_bar_rect = win32_titlebar_rect(handle);
    CustomTitleBarButtonRects button_rects = win32_get_title_bar_button_rects(handle, &title_bar_rect);

    CustomTitleBarHoveredButton new_hovered_button = CustomTitleBarHoveredButton_None;
    if (PtInRect(&button_rects.close, cursor_point)) {
      new_hovered_button = CustomTitleBarHoveredButton_Close;
    } else if (PtInRect(&button_rects.minimize, cursor_point)) {
      new_hovered_button = CustomTitleBarHoveredButton_Minimize;
    } else if (PtInRect(&button_rects.maximize, cursor_point)) {
      new_hovered_button = CustomTitleBarHoveredButton_Maximize;
    }
    if (new_hovered_button != title_bar_hovered_button) {
      // You could do tighter invalidation here but probably doesn't matter
      InvalidateRect(handle, &button_rects.close, FALSE);
      InvalidateRect(handle, &button_rects.minimize, FALSE);
      InvalidateRect(handle, &button_rects.maximize, FALSE);

      SetWindowLongPtrW(handle, GWLP_USERDATA, (LONG_PTR)new_hovered_button);
    }
    return DefWindowProc(handle, message, w_param, l_param);
  }
  // If the mouse gets into the client area then no title bar buttons are hovered
  // so need to reset the hover state
  case WM_MOUSEMOVE: {
    if (title_bar_hovered_button) {
      RECT title_bar_rect = win32_titlebar_rect(handle);
      // You could do tighter invalidation here but probably doesn't matter
      InvalidateRect(handle, &title_bar_rect, FALSE);
      SetWindowLongPtrW(handle, GWLP_USERDATA, (LONG_PTR)CustomTitleBarHoveredButton_None);
    }
    return DefWindowProc(handle, message, w_param, l_param);
  }
  // Handle mouse down and mouse up in the caption area to handle clicks on the buttons
  case WM_NCLBUTTONDOWN: {
    // Clicks on buttons will be handled in WM_NCLBUTTONUP, but we still need
    // to remove default handling of the click to avoid it counting as drag.
    //
    // Ideally you also want to check that the mouse hasn't moved out or too much
    // between DOWN and UP messages.
    if (title_bar_hovered_button) {
      return 0;
    }
    // Default handling allows for dragging and double click to maximize
    return DefWindowProc(handle, message, w_param, l_param);
  }
  // Map button clicks to the right messages for the window
  case WM_NCLBUTTONUP: {
    if (title_bar_hovered_button == CustomTitleBarHoveredButton_Close) {
      PostMessageW(handle, WM_CLOSE, 0, 0);
      return 0;
    } else if (title_bar_hovered_button == CustomTitleBarHoveredButton_Minimize) {
      ShowWindow(handle, SW_MINIMIZE);
      return 0;
    } else if (title_bar_hovered_button == CustomTitleBarHoveredButton_Maximize) {
      int mode = win32_window_is_maximized(handle) ? SW_NORMAL : SW_MAXIMIZE;
      ShowWindow(handle, mode);
      return 0;
    }
    return DefWindowProc(handle, message, w_param, l_param);
  }
  case WM_SETCURSOR: {
    // Show an arrow instead of the busy cursor
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    break;
  }
  case WM_DESTROY: {
    PostQuitMessage(0);
    return 0;
  }
  }

  return DefWindowProc(handle, message, w_param, l_param);
}