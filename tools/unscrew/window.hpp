//
#ifndef BAULK_UNSCREW_WINDOW_HPP
#define BAULK_UNSCREW_WINDOW_HPP
#include <bela/base.hpp>
#include "resource.h"

namespace baulk::unscrew {
// https://github.com/oberth/custom-chrome/wiki
// https://kubyshkin.name/posts/win32-window-custom-title-bar-caption/
struct CustomTitleBarButtonRects {
  RECT close;
  RECT maximize;
  RECT minimize;
};

enum CustomTitleBarHoveredButton {
  CustomTitleBarHoveredButton_None,
  CustomTitleBarHoveredButton_Minimize,
  CustomTitleBarHoveredButton_Maximize,
  CustomTitleBarHoveredButton_Close,
};

constexpr int ScaleDPI(int value, UINT dpi) { return static_cast<int>(static_cast<float>(value) * dpi / 96); }
bool IsWindowsVersionOrGreater(int major, int minor, int buildNumber);
template <class Derived> class BaseWindow {
public:
  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_NCCREATE) {
      CREATESTRUCTW *pCreate = reinterpret_cast<CREATESTRUCTW *>(lParam);
      auto wnd = reinterpret_cast<Derived *>(pCreate->lpCreateParams);
      SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wnd));
      wnd->m_hWnd = hWnd;
      return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    if (auto wnd = reinterpret_cast<Derived *>(GetWindowLongPtrW(hWnd, GWLP_USERDATA)); wnd != nullptr) {
      return wnd->HandleMessage(uMsg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
  }

  BaseWindow() = default;

  BOOL MakeWindow(PCWSTR lpWindowName) {
    isMicaEnabled = IsWindowsVersionOrGreater(10, 0, 19041);
    hInstance = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc = {
        .cbSize = sizeof(WNDCLASSEXW),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = Derived::WindowProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInstance,
        .hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(ICON_BAULK_APP)),
        .hCursor = nullptr,
        .hbrBackground = nullptr,
        .lpszMenuName = nullptr,
        .lpszClassName = ClassName(),
        .hIconSm = nullptr,
    };
    RegisterClassExW(&wc);
    // WS_EX_NOREDIRECTIONBITMAP
    // https://blog.lindexi.com/post/WPF-%E4%BD%BF%E7%94%A8-Composition-API-%E5%81%9A%E9%AB%98%E6%80%A7%E8%83%BD%E6%B8%B2%E6%9F%93.html
    // https://docs.microsoft.com/en-us/archive/msdn-magazine/2014/june/windows-with-c-high-performance-window-layering-using-the-windows-composition-engine
    auto extend_style = isMicaEnabled ? (WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP) : WS_EX_APPWINDOW;
    constexpr auto window_style =
        WS_SYSMENU       // Explicitly ask for the titlebar to support snapping via Win + ← / Win + →
        | WS_MINIMIZEBOX // Add minimize button to support minimizing by clicking on the taskbar icon
        | WS_VISIBLE;    // Make window visible after it is created (not important)
    m_hWnd = CreateWindowExW(extend_style, ClassName(), lpWindowName, window_style, CW_USEDEFAULT, CW_USEDEFAULT, 1000,
                             600, nullptr, nullptr, hInstance, this);

    return (m_hWnd ? TRUE : FALSE);
  }

  HWND Window() const { return m_hWnd; }

protected:
  virtual PCWSTR ClassName() const = 0;
  virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
  HINSTANCE hInstance{nullptr};
  HWND m_hWnd{nullptr};
  bool isMicaEnabled{false};
};

struct Widget {
  HWND hWnd{nullptr};
  int X{0};
  int Y{0};
  int W{0};
  int H{0};
  void Visible(BOOL v) { EnableWindow(hWnd, v); }
  bool IsVisible() const { return IsWindowVisible(hWnd) == TRUE; }
  std::wstring Content() {
    auto n = GetWindowTextLengthW(hWnd);
    if (n <= 0) {
      return L"";
    }
    std::wstring str;
    str.resize(n + 1);
    auto k = GetWindowTextW(hWnd, str.data(), n + 1);
    str.resize(k);
    return str;
  }
  void Content(std::wstring_view text) {
    //
    ::SetWindowTextW(hWnd, text.data());
  }
};

class MainWindow : public BaseWindow<MainWindow> {
private:
  LRESULT OnNccalcsize(WPARAM wParam, LPARAM lParam);
  LRESULT OnNchittest(WPARAM wParam, LPARAM lParam);
  LRESULT OnNcMouseMove(WPARAM wParam, LPARAM lParam);
  LRESULT OnCreate(WPARAM wParam, LPARAM lParam);
  LRESULT OnPaint(WPARAM wParam, LPARAM lParam);
  bool MakeWidget(Widget &w, LPCWSTR cn, LPCWSTR text, DWORD ds, int x, int y, int cx, int cy, ptrdiff_t id) {
    constexpr const auto wndex = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_NOPARENTNOTIFY;
    w.X = x;
    w.Y = y;
    w.W = cx;
    w.H = cy;
    w.hWnd = ::CreateWindowExW(wndex, cn, text, ds, MulDiv(w.X, dpiX, 96), MulDiv(w.Y, dpiX, 96), MulDiv(w.W, dpiX, 96),
                               MulDiv(w.H, dpiX, 96), m_hWnd, reinterpret_cast<HMENU>(id), hInstance, nullptr);
    if (w.hWnd == nullptr) {
      return false;
    }
    if (hFont != nullptr) {
      ::SendMessageW(w.hWnd, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
    return true;
  }

public:
  MainWindow();
  ~MainWindow();
  PCWSTR ClassName() const { return L"Unscrew.Window.Mica"; }
  LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
  bool InitializeMica();
  CustomTitleBarHoveredButton mouseHovered{CustomTitleBarHoveredButton_None};
  Widget wProgress;
  HFONT hFont{nullptr}; // GDI font
  int dpiX{96};
};

} // namespace baulk::unscrew

#endif