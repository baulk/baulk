//////////////////
#include <bela/base.hpp>
#include <CommCtrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <bela/picker.hpp>
#include <bela/comutils.hpp>

namespace bela {

HRESULT WINAPI taskdialog_callback_impl(__in HWND hWnd, __in UINT msg,
                                        __in WPARAM wParam, __in LPARAM lParam,
                                        __in LONG_PTR lpRefData) {
  UNREFERENCED_PARAMETER(lpRefData);
  UNREFERENCED_PARAMETER(wParam);
  switch (msg) {
  case TDN_CREATED:
    ::SetForegroundWindow(hWnd);
    break;
  case TDN_RADIO_BUTTON_CLICKED:
    break;
  case TDN_BUTTON_CLICKED:
    break;
  case TDN_HYPERLINK_CLICKED:
    ShellExecuteW(hWnd, nullptr, reinterpret_cast<LPCWSTR>(lParam), nullptr,
                  nullptr, SW_SHOWNORMAL);
    break;
  }
  return S_OK;
}

#define TKDWFLAGS                                                              \
  TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW |            \
      TDF_SIZE_TO_CONTENT
#define TKDWFLAGSEX                                                            \
  TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW |            \
      TDF_SIZE_TO_CONTENT | TDF_EXPAND_FOOTER_AREA | TDF_ENABLE_HYPERLINKS

bool BelaMessageBox(HWND hWnd, LPCWSTR pszWindowTitle, LPCWSTR pszContent,
                    LPCWSTR pszExpandedInfo, mbs_t mt) {
  TASKDIALOGCONFIG tc;
  memset(&tc, 0, sizeof(tc));
  int nButton = 0;
  int nRadioButton = 0;
  tc.cbSize = sizeof(tc);
  tc.hwndParent = hWnd;
  tc.hInstance = GetModuleHandleW(nullptr);
  tc.pszWindowTitle = pszWindowTitle;
  tc.pszContent = pszContent;
  tc.pszExpandedInformation = pszExpandedInfo;
  tc.dwFlags = (pszExpandedInfo != nullptr ? TKDWFLAGSEX : TKDWFLAGS);
  tc.nDefaultRadioButton = nRadioButton;
  tc.pszCollapsedControlText = L"More information";
  tc.pszExpandedControlText = L"Less information";
  tc.pfCallback = taskdialog_callback_impl;
  switch (mt) {
  case mbs_t::INFO:
    tc.pszMainIcon = TD_INFORMATION_ICON;
    break;
  case mbs_t::WARN:
    tc.pszMainIcon = TD_WARNING_ICON;
    break;
  case mbs_t::FATAL:
    tc.pszMainIcon = TD_ERROR_ICON;
    break;
  case mbs_t::ABOUT:
    if (hWnd) {
      auto hIcon =
          reinterpret_cast<HICON>(SendMessageW(hWnd, WM_GETICON, ICON_BIG, 0));
      if (hIcon) {
        tc.hMainIcon = hIcon;
        tc.dwFlags |= TDF_USE_HICON_MAIN;
      }
    }
    break;
  default:
    break;
  }
  return (TaskDialogIndirect(&tc, &nButton, &nRadioButton, nullptr) == S_OK);
}

std::optional<std::wstring> FilePicker(HWND hWnd, const wchar_t *title,
                                       const filter_t *fts, size_t flen) {
  bela::comptr<IFileOpenDialog> window;
  if (::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                         IID_IFileOpenDialog, (void **)&window) != S_OK) {
    return std::nullopt;
  }
  if (!window) {
    return std::nullopt;
  }
  if (flen != 0) {
    if (window->SetFileTypes(static_cast<UINT>(flen), fts) != S_OK) {
      return std::nullopt;
    }
  }
  if (window->SetTitle(title == nullptr ? L"Open File Picker" : title) !=
      S_OK) {
    return std::nullopt;
  }
  if (window->Show(hWnd) != S_OK) {
    return std::nullopt;
  }
  bela::comptr<IShellItem> item;
  if (window->GetResult(&item) != S_OK) {
    return std::nullopt;
  }
  LPWSTR pszFilePath = nullptr;
  if (item->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFilePath) !=
      S_OK) {
    return std::nullopt;
  }
  auto opt = std::make_optional<std::wstring>(pszFilePath);
  CoTaskMemFree(pszFilePath);
  return opt;
}

std::optional<std::wstring> FolderPicker(HWND hWnd, const wchar_t *title) {
  bela::comptr<IFileDialog> w;
  if (::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                         IID_IFileDialog, (void **)&w) != S_OK) {
    return std::nullopt;
  }
  DWORD dwo = 0;
  if (w->GetOptions(&dwo) != S_OK) {
    return std::nullopt;
  }
  w->SetOptions(dwo | FOS_PICKFOLDERS);
  w->SetTitle(title == nullptr ? L"Open Folder Picker" : title);
  bela::comptr<IShellItem> item;
  if (w->Show(hWnd) != S_OK) {
    return std::nullopt;
  }
  if (w->GetResult(&item) != S_OK) {
    return std::nullopt;
  }
  LPWSTR pszFolderPath = nullptr;
  if (item->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath) != S_OK) {
    return std::nullopt;
  }
  auto opt = std::make_optional<std::wstring>(pszFolderPath);
  CoTaskMemFree(pszFolderPath);
  return opt;
}
} // namespace bela