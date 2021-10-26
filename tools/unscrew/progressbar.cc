//
//////////////////
#include <bela/base.hpp>
#include <CommCtrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <bela/picker.hpp>
#include <bela/comutils.hpp>

namespace baulk::unscrew {

HRESULT WINAPI taskdialog_callback_impl(__in HWND hWnd, __in UINT msg, __in WPARAM wParam, __in LPARAM lParam,
                                        __in LONG_PTR lpRefData) {
  UNREFERENCED_PARAMETER(lpRefData);
  UNREFERENCED_PARAMETER(wParam);
  switch (msg) {
  case TDN_CREATED:
    ::SetForegroundWindow(hWnd);
    // https://docs.microsoft.com/en-us/windows/win32/controls/tdm-set-progress-bar-range
    break;
  case TDN_RADIO_BUTTON_CLICKED:
    break;
  case TDN_BUTTON_CLICKED:
    break;
  case TDN_TIMER:
    SendMessageW(hWnd, TDM_SET_PROGRESS_BAR_POS, 10, 0);
    break;
  case TDN_HYPERLINK_CLICKED:
    ShellExecuteW(hWnd, nullptr, reinterpret_cast<LPCWSTR>(lParam), nullptr, nullptr, SW_SHOWNORMAL);
    break;
  }
  return S_OK;
}

bool UnscrewMessageBox(HWND hWnd, LPCWSTR pszWindowTitle, LPCWSTR pszContent) {
  TASKDIALOGCONFIG tc;
  memset(&tc, 0, sizeof(tc));
  int nButton = 0;
  int nRadioButton = 0;
  tc.cbSize = sizeof(tc);
  tc.hwndParent = hWnd;
  tc.hInstance = GetModuleHandleW(nullptr);
  tc.pszWindowTitle = pszWindowTitle;
  tc.pszContent = pszContent;
  tc.pszExpandedInformation = nullptr;
  tc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SHOW_PROGRESS_BAR |
               TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CALLBACK_TIMER;
  tc.nDefaultRadioButton = nRadioButton;
  tc.pszCollapsedControlText = L"More information";
  tc.pszExpandedControlText = L"Less information";
  tc.pfCallback = taskdialog_callback_impl;
  if (hWnd) {
    auto hIcon = reinterpret_cast<HICON>(SendMessageW(hWnd, WM_GETICON, ICON_BIG, 0));
    if (hIcon) {
      tc.hMainIcon = hIcon;
      tc.dwFlags |= TDF_USE_HICON_MAIN;
    }
  }
  return (TaskDialogIndirect(&tc, &nButton, &nRadioButton, nullptr) == S_OK);
}

} // namespace baulk::unscrew