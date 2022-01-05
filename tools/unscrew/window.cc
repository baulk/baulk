//
#include "window.hpp"
#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <VersionHelpers.h>

namespace baulk::unscrew {

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

LRESULT MainWindow::OnCreate(WPARAM wParam, LPARAM lParam) {
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
  PAINTSTRUCT ps;
  BeginPaint(m_hWnd, &ps);
  EndPaint(m_hWnd, &ps);
  return S_OK;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
  case WM_CREATE:
    return OnCreate(wParam, lParam);
  case WM_PAINT:
    return OnPaint(wParam, lParam);
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  default:
    break;
  }
  return DefWindowProcW(m_hWnd, uMsg, wParam, lParam);
}

} // namespace baulk::unscrew