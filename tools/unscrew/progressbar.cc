//
//////////////////
#include <bela/base.hpp>
#include <CommCtrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <bela/picker.hpp>
#include <bela/comutils.hpp>
#include <bela/io.hpp>
#include "progressbar.hpp"

namespace baulk::unscrew {
static std::atomic_bool canceled{false};
HRESULT WINAPI taskdialog_callback_impl(__in HWND hWnd, __in UINT msg, __in WPARAM wParam, __in LPARAM lParam,
                                        __in LONG_PTR lpRefData) {
  UNREFERENCED_PARAMETER(lpRefData);
  UNREFERENCED_PARAMETER(wParam);
  HRESULT hr = S_OK;
  switch (msg) {
  case TDN_CREATED:
    ::SetForegroundWindow(hWnd);
    // SendMessageW(hWnd, TDM_SET_PROGRESS_BAR_RANGE, MAKEWPARAM(0, 100), 0);
    //  https://docs.microsoft.com/en-us/windows/win32/controls/tdm-set-progress-bar-range
    break;
  case TDN_DESTROYED:

    break;
  case TDN_RADIO_BUTTON_CLICKED:
    break;
  case TDN_BUTTON_CLICKED:
    if (IDCANCEL == wParam) {
      canceled = true;
      SendMessageW(hWnd, TDM_SET_PROGRESS_BAR_STATE, PBST_PAUSED, 0);
      return S_FALSE;
    }

    // MessageBoxW(hWnd, L"destroyed", L"destroyed", MB_OK);
    //  TDM_SET_PROGRESS_BAR_MARQUEE
    break;
  case TDN_TIMER: {
    if (!canceled) {
      // wParam is the number of milliseconds since the dialog was created or this notification returned TRUE
      int dwTickCount = (int)wParam / 200;
      auto msg = bela::StringCat(L"Extract file-", dwTickCount);
      SendMessageW(hWnd, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)msg.data());
      if (dwTickCount < 100) {
        SendMessageW(hWnd, TDM_SET_PROGRESS_BAR_POS, dwTickCount, 0);
      } else if (dwTickCount < 130) {
        SendMessageW(hWnd, TDM_SET_PROGRESS_BAR_POS, 100, 0);
      } else {
        // Return S_FALSE to reset dwTickCount
        hr = TRUE;
      }
    }
  }

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
  tc.dwCommonButtons = TDCBF_CANCEL_BUTTON | TDCBF_CLOSE_BUTTON;
  tc.hInstance = GetModuleHandleW(nullptr);
  tc.pszWindowTitle = pszWindowTitle;
  tc.pszMainInstruction = L"---";
  tc.pszContent = pszContent;
  tc.pszExpandedInformation = nullptr;
  tc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT |
               TDF_SHOW_PROGRESS_BAR | TDF_CALLBACK_TIMER;
  tc.nDefaultRadioButton = nRadioButton;
  tc.pfCallback = taskdialog_callback_impl;
  if (hWnd) {
    if (auto hIcon = reinterpret_cast<HICON>(SendMessageW(hWnd, WM_GETICON, ICON_BIG, 0)); hIcon) {
      tc.hMainIcon = hIcon;
      tc.dwFlags |= TDF_USE_HICON_MAIN;
    }
  }
  return (TaskDialogIndirect(&tc, &nButton, &nRadioButton, nullptr) == S_OK);
}
bool ProgressBar::NewProgressBar(std::wstring_view path, bela::error_code &ec) {
  if (size = bela::io::Size(path, ec); size == bela::SizeUnInitialized) {
    return false;
  }
  return true;
}
bool ProgressBar::SetProgress(int64_t completed) {
  //
  return true;
}

} // namespace baulk::unscrew