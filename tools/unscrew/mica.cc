#include <bela/base.hpp>
#include <dwmapi.h>
#include <VersionHelpers.h>
#include "resource.h"

template <class DERIVED_TYPE> class BaseWindow {
public:
  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    DERIVED_TYPE *pThis = NULL;

    if (uMsg == WM_NCCREATE) {
      CREATESTRUCTW *pCreate = reinterpret_cast<CREATESTRUCTW *>(lParam);
      pThis = reinterpret_cast<DERIVED_TYPE *>(pCreate->lpCreateParams);
      SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
      pThis->m_hWnd = hWnd;
    } else {
      pThis = reinterpret_cast<DERIVED_TYPE *>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }
    if (pThis) {
      return pThis->HandleMessage(uMsg, wParam, lParam);
    } else {
      return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
  }

  BaseWindow() : m_hWnd(NULL) {}

  BOOL Create(PCWSTR lpWindowName, DWORD dwStyle, DWORD dwExStyle = WS_EX_NOREDIRECTIONBITMAP, int x = CW_USEDEFAULT,
              int y = CW_USEDEFAULT, int nWidth = CW_USEDEFAULT, int nHeight = CW_USEDEFAULT, HWND hWndParent = 0,
              HMENU hMenu = 0) {
    WNDCLASS wc = {0};

    wc.lpfnWndProc = DERIVED_TYPE::WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = ClassName();
    wc.hIcon = LoadIconW(wc.hInstance, MAKEINTRESOURCEW(ICON_BAULK_APP));

    RegisterClassW(&wc);

    m_hWnd = CreateWindowEx(dwExStyle, ClassName(), lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu,
                            GetModuleHandleW(NULL), this);

    return (m_hWnd ? TRUE : FALSE);
  }

  HWND Window() const { return m_hWnd; }

protected:
  virtual PCWSTR ClassName() const = 0;
  virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

  HWND m_hWnd;
};

class MainWindow : public BaseWindow<MainWindow> {
public:
  PCWSTR ClassName() const { return L"Unscrew.Mica"; }
  LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
constexpr auto windowflag = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {
  MainWindow win;

  if (!win.Create(nullptr, windowflag, WS_EX_NOREDIRECTIONBITMAP | WS_EX_APPWINDOW, CW_USEDEFAULT,
                  CW_USEDEFAULT, 1200, 400)) {
    return 0;
  }

  ShowWindow(win.Window(), nCmdShow);

  // Run the message loop.

  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
//   case WM_NCPAINT: {
//     WINDOWINFO wi{0};
//     wi.cbSize=sizeof(WINDOWINFO);
//     GetWindowInfo(m_hWnd, &wi);
//   }
//     return DefWindowProcW(m_hWnd, uMsg, wParam, lParam);
  case WM_CREATE: {
    // Mica
    enum DWMWINDOWATTRIBUTE { DWMWA_USE_IMMERSIVE_DARK_MODE = 20, DWMWA_MICA_EFFECT = 1029 };

    enum DWM_BOOL { DWMWCP_FALSE = 0, DWMWCP_TRUE = 1 };
    MARGINS margins = {-1};
    ::DwmExtendFrameIntoClientArea(m_hWnd, &margins);
    // Dark mode

    DWM_BOOL darkPreference = DWMWCP_FALSE;
    DwmSetWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkPreference, sizeof(darkPreference));

    // Mica
    DWM_BOOL micaPreference = DWMWCP_TRUE;
    DwmSetWindowAttribute(m_hWnd, DWMWA_MICA_EFFECT, &micaPreference, sizeof(micaPreference));
    //SetWindowTextW(m_hWnd,L"Extract file");
  }
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

  default:
    return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
  }
  return TRUE;
}
